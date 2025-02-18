// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/inspector/v8-inspector-session-impl.h"

#include "src/inspector/injected-script.h"
#include "src/inspector/inspected-context.h"
#include "src/inspector/protocol/Protocol.h"
#include "src/inspector/remote-object-id.h"
#include "src/inspector/search-util.h"
#include "src/inspector/string-util.h"
#include "src/inspector/v8-console-agent-impl.h"
#include "src/inspector/v8-debugger-agent-impl.h"
#include "src/inspector/v8-debugger.h"
#include "src/inspector/v8-heap-profiler-agent-impl.h"
#include "src/inspector/v8-inspector-impl.h"
#include "src/inspector/v8-profiler-agent-impl.h"
#include "src/inspector/v8-runtime-agent-impl.h"
#include "src/inspector/v8-schema-agent-impl.h"

namespace v8_inspector {

// static
bool V8InspectorSession::canDispatchMethod(const StringView& method)
{
    return stringViewStartsWith(method,
               protocol::Runtime::Metainfo::commandPrefix)
        || stringViewStartsWith(method,
            protocol::Debugger::Metainfo::commandPrefix)
        || stringViewStartsWith(method,
            protocol::Profiler::Metainfo::commandPrefix)
        || stringViewStartsWith(
            method, protocol::HeapProfiler::Metainfo::commandPrefix)
        || stringViewStartsWith(method,
            protocol::Console::Metainfo::commandPrefix)
        || stringViewStartsWith(method,
            protocol::Schema::Metainfo::commandPrefix);
}

// static
int V8ContextInfo::executionContextId(v8::Local<v8::Context> context)
{
    return InspectedContext::contextId(context);
}

std::unique_ptr<V8InspectorSessionImpl> V8InspectorSessionImpl::create(
    V8InspectorImpl* inspector, int contextGroupId, int sessionId,
    V8Inspector::Channel* channel, const StringView& state)
{
    return std::unique_ptr<V8InspectorSessionImpl>(new V8InspectorSessionImpl(
        inspector, contextGroupId, sessionId, channel, state));
}

V8InspectorSessionImpl::V8InspectorSessionImpl(V8InspectorImpl* inspector,
    int contextGroupId,
    int sessionId,
    V8Inspector::Channel* channel,
    const StringView& savedState)
    : m_contextGroupId(contextGroupId)
    , m_sessionId(sessionId)
    , m_inspector(inspector)
    , m_channel(channel)
    , m_customObjectFormatterEnabled(false)
    , m_dispatcher(this)
    , m_state(nullptr)
    , m_runtimeAgent(nullptr)
    , m_debuggerAgent(nullptr)
    , m_heapProfilerAgent(nullptr)
    , m_profilerAgent(nullptr)
    , m_consoleAgent(nullptr)
    , m_schemaAgent(nullptr)
{
    if (savedState.length()) {
        std::unique_ptr<protocol::Value> state = protocol::StringUtil::parseJSON(toString16(savedState));
        if (state)
            m_state = protocol::DictionaryValue::cast(std::move(state));
        if (!m_state)
            m_state = protocol::DictionaryValue::create();
    } else {
        m_state = protocol::DictionaryValue::create();
    }

    m_state->getBoolean("use_binary_protocol", &use_binary_protocol_);

    m_runtimeAgent.reset(new V8RuntimeAgentImpl(
        this, this, agentState(protocol::Runtime::Metainfo::domainName)));
    protocol::Runtime::Dispatcher::wire(&m_dispatcher, m_runtimeAgent.get());

    m_debuggerAgent.reset(new V8DebuggerAgentImpl(
        this, this, agentState(protocol::Debugger::Metainfo::domainName)));
    protocol::Debugger::Dispatcher::wire(&m_dispatcher, m_debuggerAgent.get());

    m_profilerAgent.reset(new V8ProfilerAgentImpl(
        this, this, agentState(protocol::Profiler::Metainfo::domainName)));
    protocol::Profiler::Dispatcher::wire(&m_dispatcher, m_profilerAgent.get());

    m_heapProfilerAgent.reset(new V8HeapProfilerAgentImpl(
        this, this, agentState(protocol::HeapProfiler::Metainfo::domainName)));
    protocol::HeapProfiler::Dispatcher::wire(&m_dispatcher,
        m_heapProfilerAgent.get());

    m_consoleAgent.reset(new V8ConsoleAgentImpl(
        this, this, agentState(protocol::Console::Metainfo::domainName)));
    protocol::Console::Dispatcher::wire(&m_dispatcher, m_consoleAgent.get());

    m_schemaAgent.reset(new V8SchemaAgentImpl(
        this, this, agentState(protocol::Schema::Metainfo::domainName)));
    protocol::Schema::Dispatcher::wire(&m_dispatcher, m_schemaAgent.get());

    //m_runtimeAgent->enable(); // weolar add

    if (savedState.length()) {
        m_runtimeAgent->restore();
        m_debuggerAgent->restore();
        m_heapProfilerAgent->restore();
        m_profilerAgent->restore();
        m_consoleAgent->restore();
    }
}

V8InspectorSessionImpl::~V8InspectorSessionImpl()
{
    discardInjectedScripts();
    m_consoleAgent->disable();
    m_profilerAgent->disable();
    m_heapProfilerAgent->disable();
    m_debuggerAgent->disable();
    m_runtimeAgent->disable();
    m_inspector->disconnect(this);
}

protocol::DictionaryValue* V8InspectorSessionImpl::agentState(
    const String16& name)
{
    protocol::DictionaryValue* state = m_state->getObject(name);
    if (!state) {
        std::unique_ptr<protocol::DictionaryValue> newState = protocol::DictionaryValue::create();
        state = newState.get();
        m_state->setObject(name, std::move(newState));
    }
    return state;
}

namespace {

    class MessageBuffer : public StringBuffer {
    public:
        static std::unique_ptr<MessageBuffer> create(
            std::unique_ptr<protocol::Serializable> message, bool binary)
        {
            return std::unique_ptr<MessageBuffer>(
                new MessageBuffer(std::move(message), binary));
        }

        const StringView& string() override
        {
            if (!m_serialized) {
                if (m_binary) {
                    // Encode binary response as an 8bit string buffer.
                    m_serialized.reset(
                        new BinaryStringBuffer(m_message->serializeToBinary()));
                } else {
                    m_serialized = StringBuffer::create(toStringView(m_message->serializeToJSON()));
                }
                m_message.reset(nullptr);
            }
            return m_serialized->string();
        }

    private:
        explicit MessageBuffer(std::unique_ptr<protocol::Serializable> message,
            bool binary)
            : m_message(std::move(message))
            , m_binary(binary)
        {
        }

        std::unique_ptr<protocol::Serializable> m_message;
        std::unique_ptr<StringBuffer> m_serialized;
        bool m_binary;
    };

} // namespace

void V8InspectorSessionImpl::sendProtocolResponse(
    int callId, std::unique_ptr<protocol::Serializable> message)
{
    m_channel->sendResponse(
        callId, MessageBuffer::create(std::move(message), use_binary_protocol_));
}

void V8InspectorSessionImpl::sendProtocolNotification(
    std::unique_ptr<protocol::Serializable> message)
{
    m_channel->sendNotification(
        MessageBuffer::create(std::move(message), use_binary_protocol_));
}

void V8InspectorSessionImpl::fallThrough(
    int callId, const String16& method,
    const protocol::ProtocolMessage& message)
{
    // There's no other layer to handle the command.
    UNREACHABLE();
}

void V8InspectorSessionImpl::flushProtocolNotifications()
{
    m_channel->flushProtocolNotifications();
}

void V8InspectorSessionImpl::reset()
{
    m_debuggerAgent->reset();
    m_runtimeAgent->reset();
    discardInjectedScripts();
}

void V8InspectorSessionImpl::discardInjectedScripts()
{
    m_inspectedObjects.clear();
    int sessionId = m_sessionId;
    m_inspector->forEachContext(m_contextGroupId,
        [&sessionId](InspectedContext* context) {
            context->discardInjectedScript(sessionId);
        });
}

Response V8InspectorSessionImpl::findInjectedScript(
    int contextId, InjectedScript*& injectedScript)
{
    injectedScript = nullptr;
    InspectedContext* context = m_inspector->getContext(m_contextGroupId, contextId);
    if (!context)
        return Response::Error("Cannot find context with specified id");
    injectedScript = context->getInjectedScript(m_sessionId);
    if (!injectedScript) {
        injectedScript = context->createInjectedScript(m_sessionId);
        if (m_customObjectFormatterEnabled)
            injectedScript->setCustomObjectFormatterEnabled(true);
    }
    return Response::OK();
}

Response V8InspectorSessionImpl::findInjectedScript(
    RemoteObjectIdBase* objectId, InjectedScript*& injectedScript)
{
    return findInjectedScript(objectId->contextId(), injectedScript);
}

void V8InspectorSessionImpl::releaseObjectGroup(const StringView& objectGroup)
{
    releaseObjectGroup(toString16(objectGroup));
}

void V8InspectorSessionImpl::releaseObjectGroup(const String16& objectGroup)
{
    int sessionId = m_sessionId;
    m_inspector->forEachContext(
        m_contextGroupId, [&objectGroup, &sessionId](InspectedContext* context) {
            InjectedScript* injectedScript = context->getInjectedScript(sessionId);
            if (injectedScript)
                injectedScript->releaseObjectGroup(objectGroup);
        });
}

bool V8InspectorSessionImpl::unwrapObject(
    std::unique_ptr<StringBuffer>* error, const StringView& objectId,
    v8::Local<v8::Value>* object, v8::Local<v8::Context>* context,
    std::unique_ptr<StringBuffer>* objectGroup)
{
    String16 objectGroupString;
    Response response = unwrapObject(toString16(objectId), object, context,
        objectGroup ? &objectGroupString : nullptr);
    if (!response.isSuccess()) {
        if (error) {
            String16 errorMessage = response.errorMessage();
            *error = StringBufferImpl::adopt(errorMessage);
        }
        return false;
    }
    if (objectGroup)
        *objectGroup = StringBufferImpl::adopt(objectGroupString);
    return true;
}

Response V8InspectorSessionImpl::unwrapObject(const String16& objectId,
    v8::Local<v8::Value>* object,
    v8::Local<v8::Context>* context,
    String16* objectGroup)
{
    std::unique_ptr<RemoteObjectId> remoteId;
    Response response = RemoteObjectId::parse(objectId, &remoteId);
    if (!response.isSuccess())
        return response;
    InjectedScript* injectedScript = nullptr;
    response = findInjectedScript(remoteId.get(), injectedScript);
    if (!response.isSuccess())
        return response;
    response = injectedScript->findObject(*remoteId, object);
    if (!response.isSuccess())
        return response;
    *context = injectedScript->context()->context();
    if (objectGroup)
        *objectGroup = injectedScript->objectGroupName(*remoteId);
    return Response::OK();
}

std::unique_ptr<protocol::Runtime::API::RemoteObject>
V8InspectorSessionImpl::wrapObject(v8::Local<v8::Context> context,
    v8::Local<v8::Value> value,
    const StringView& groupName,
    bool generatePreview)
{
    return wrapObject(context, value, toString16(groupName), generatePreview);
}

std::unique_ptr<protocol::Runtime::RemoteObject>
V8InspectorSessionImpl::wrapObject(v8::Local<v8::Context> context,
    v8::Local<v8::Value> value,
    const String16& groupName,
    bool generatePreview)
{
    InjectedScript* injectedScript = nullptr;
    findInjectedScript(InspectedContext::contextId(context), injectedScript);
    if (!injectedScript)
        return nullptr;
    std::unique_ptr<protocol::Runtime::RemoteObject> result;
    injectedScript->wrapObject(
        value, groupName,
        generatePreview ? WrapMode::kWithPreview : WrapMode::kNoPreview, &result);
    return result;
}

std::unique_ptr<protocol::Runtime::RemoteObject>
V8InspectorSessionImpl::wrapTable(v8::Local<v8::Context> context,
    v8::Local<v8::Object> table,
    v8::MaybeLocal<v8::Array> columns)
{
    InjectedScript* injectedScript = nullptr;
    findInjectedScript(InspectedContext::contextId(context), injectedScript);
    if (!injectedScript)
        return nullptr;
    return injectedScript->wrapTable(table, columns);
}

void V8InspectorSessionImpl::setCustomObjectFormatterEnabled(bool enabled)
{
    m_customObjectFormatterEnabled = enabled;
    int sessionId = m_sessionId;
    m_inspector->forEachContext(
        m_contextGroupId, [&enabled, &sessionId](InspectedContext* context) {
            InjectedScript* injectedScript = context->getInjectedScript(sessionId);
            if (injectedScript)
                injectedScript->setCustomObjectFormatterEnabled(enabled);
        });
}

void V8InspectorSessionImpl::reportAllContexts(V8RuntimeAgentImpl* agent)
{
    m_inspector->forEachContext(m_contextGroupId,
        [&agent](InspectedContext* context) {
            agent->reportExecutionContextCreated(context);
        });
}

void V8InspectorSessionImpl::dispatchProtocolMessage(
    const StringView& message)
{
    bool binary_protocol = message.is8Bit() && message.length() && message.characters8()[0] == 0xD8;
    if (binary_protocol) {
        use_binary_protocol_ = true;
        m_state->setBoolean("use_binary_protocol", true);
    }

    int callId;
    std::unique_ptr<protocol::Value> parsed_message;
    if (binary_protocol) {
        parsed_message = protocol::Value::parseBinary(
            message.characters8(), static_cast<unsigned>(message.length()));
    } else {
        parsed_message = protocol::StringUtil::parseJSON(message);
    }
    String16 method;
    if (m_dispatcher.parseCommand(parsed_message.get(), &callId, &method)) {
        // Pass empty string instead of the actual message to save on a conversion.
        // We're allowed to do so because fall-through is not implemented.
        m_dispatcher.dispatch(callId, method, std::move(parsed_message),
            protocol::ProtocolMessage());
    }
}

std::unique_ptr<StringBuffer> V8InspectorSessionImpl::stateJSON()
{
    String16 json = m_state->toJSONString();
    return StringBufferImpl::adopt(json);
}

std::vector<std::unique_ptr<protocol::Schema::API::Domain>>
V8InspectorSessionImpl::supportedDomains()
{
    std::vector<std::unique_ptr<protocol::Schema::Domain>> domains = supportedDomainsImpl();
    std::vector<std::unique_ptr<protocol::Schema::API::Domain>> result;
    for (size_t i = 0; i < domains.size(); ++i)
        result.push_back(std::move(domains[i]));
    return result;
}

std::vector<std::unique_ptr<protocol::Schema::Domain>>
V8InspectorSessionImpl::supportedDomainsImpl()
{
    std::vector<std::unique_ptr<protocol::Schema::Domain>> result;
    result.push_back(protocol::Schema::Domain::create()
                         .setName(protocol::Runtime::Metainfo::domainName)
                         .setVersion(protocol::Runtime::Metainfo::version)
                         .build());
    result.push_back(protocol::Schema::Domain::create()
                         .setName(protocol::Debugger::Metainfo::domainName)
                         .setVersion(protocol::Debugger::Metainfo::version)
                         .build());
    result.push_back(protocol::Schema::Domain::create()
                         .setName(protocol::Profiler::Metainfo::domainName)
                         .setVersion(protocol::Profiler::Metainfo::version)
                         .build());
    result.push_back(protocol::Schema::Domain::create()
                         .setName(protocol::HeapProfiler::Metainfo::domainName)
                         .setVersion(protocol::HeapProfiler::Metainfo::version)
                         .build());
    result.push_back(protocol::Schema::Domain::create()
                         .setName(protocol::Schema::Metainfo::domainName)
                         .setVersion(protocol::Schema::Metainfo::version)
                         .build());
    return result;
}

void V8InspectorSessionImpl::addInspectedObject(
    std::unique_ptr<V8InspectorSession::Inspectable> inspectable)
{
    m_inspectedObjects.insert(m_inspectedObjects.begin(), std::move(inspectable));
    if (m_inspectedObjects.size() > kInspectedObjectBufferSize)
        m_inspectedObjects.resize(kInspectedObjectBufferSize);
}

V8InspectorSession::Inspectable* V8InspectorSessionImpl::inspectedObject(
    unsigned num)
{
    if (num >= m_inspectedObjects.size())
        return nullptr;
    return m_inspectedObjects[num].get();
}

void V8InspectorSessionImpl::schedulePauseOnNextStatement(
    const StringView& breakReason, const StringView& breakDetails)
{
    m_debuggerAgent->schedulePauseOnNextStatement(
        toString16(breakReason),
        protocol::DictionaryValue::cast(
            protocol::StringUtil::parseJSON(breakDetails)));
}

void V8InspectorSessionImpl::cancelPauseOnNextStatement()
{
    m_debuggerAgent->cancelPauseOnNextStatement();
}

void V8InspectorSessionImpl::breakProgram(const StringView& breakReason,
    const StringView& breakDetails)
{
    m_debuggerAgent->breakProgram(
        toString16(breakReason),
        protocol::DictionaryValue::cast(
            protocol::StringUtil::parseJSON(breakDetails)));
}

void V8InspectorSessionImpl::setSkipAllPauses(bool skip)
{
    m_debuggerAgent->setSkipAllPauses(skip);
}

void V8InspectorSessionImpl::resume() { m_debuggerAgent->resume(); }

void V8InspectorSessionImpl::stepOver() { m_debuggerAgent->stepOver(); }

std::vector<std::unique_ptr<protocol::Debugger::API::SearchMatch>>
V8InspectorSessionImpl::searchInTextByLines(const StringView& text,
    const StringView& query,
    bool caseSensitive, bool isRegex)
{
    // TODO(dgozman): search may operate on StringView and avoid copying |text|.
    std::vector<std::unique_ptr<protocol::Debugger::SearchMatch>> matches = searchInTextByLinesImpl(this, toString16(text), toString16(query),
        caseSensitive, isRegex);
    std::vector<std::unique_ptr<protocol::Debugger::API::SearchMatch>> result;
    for (size_t i = 0; i < matches.size(); ++i)
        result.push_back(std::move(matches[i]));
    return result;
}

} // namespace v8_inspector
