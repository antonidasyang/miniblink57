/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bindings/core/v8/V8Initializer.h"

#include "bindings/core/v8/DOMWrapperWorld.h"
#include "bindings/core/v8/RejectedPromises.h"
#include "bindings/core/v8/RetainedDOMInfo.h"
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/ScriptWrappableVisitor.h"
#include "bindings/core/v8/SourceLocation.h"
#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8DOMException.h"
#include "bindings/core/v8/V8ErrorEvent.h"
#include "bindings/core/v8/V8ErrorHandler.h"
#include "bindings/core/v8/V8GCController.h"
#include "bindings/core/v8/V8IdleTaskRunner.h"
#include "bindings/core/v8/V8Location.h"
#include "bindings/core/v8/V8PerContextData.h"
#include "bindings/core/v8/V8PrivateProperty.h"
#include "bindings/core/v8/V8Window.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/core/v8/Modulator.h"
#include "core/dom/Document.h"
#include "core/fetch/AccessControlStatus.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/inspector/MainThreadDebugger.h"
#include "core/workers/WorkerGlobalScope.h"
#include "platform/EventDispatchForbiddenScope.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "public/platform/Platform.h"
#include "public/platform/WebScheduler.h"
#include "public/platform/WebThread.h"
#include "wtf/AddressSanitizer.h"
#include "wtf/PtrUtil.h"
#include "wtf/RefPtr.h"
#include "wtf/text/WTFString.h"
#include "wtf/typed_arrays/ArrayBufferContents.h"
#include <memory>
#include <v8-debug.h>
#include <v8-profiler.h>

namespace blink {

static Frame* findFrame(v8::Isolate* isolate,
    v8::Local<v8::Object> host,
    v8::Local<v8::Value> data)
{
    const WrapperTypeInfo* type = WrapperTypeInfo::unwrap(data);

    if (V8Window::wrapperTypeInfo.equals(type)) {
        v8::Local<v8::Object> windowWrapper = V8Window::findInstanceInPrototypeChain(host, isolate);
        if (windowWrapper.IsEmpty())
            return 0;
        return V8Window::toImpl(windowWrapper)->frame();
    }

    if (V8Location::wrapperTypeInfo.equals(type))
        return V8Location::toImpl(host)->frame();

    // This function can handle only those types listed above.
    ASSERT_NOT_REACHED();
    return 0;
}

static void reportFatalErrorInMainThread(const char* location,
    const char* message)
{
    int memoryUsageMB = Platform::current()->actualMemoryUsageMB();
    DVLOG(1) << "V8 error: " << message << " (" << location
             << ").  Current memory usage: " << memoryUsageMB << " MB";
    CRASH();
}

static void reportOOMErrorInMainThread(
    const char* location, 
#if V8_MAJOR_VERSION < 10
    bool isJsHeap
#else
    const v8::OOMDetails& details
#endif
)
{
#if V8_MAJOR_VERSION >= 10
    bool isJsHeap = details.is_heap_oom;
#endif
    int memoryUsageMB = Platform::current()->actualMemoryUsageMB();
    DVLOG(1) << "V8 " << (isJsHeap ? "javascript" : "process") << " OOM: ("
             << location << ").  Current memory usage: " << memoryUsageMB
             << " MB";
    OOM_CRASH();
}

static String extractMessageForConsole(v8::Isolate* isolate,
    v8::Local<v8::Value> data)
{
    if (V8DOMWrapper::isWrapper(isolate, data)) {
        v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(data);
        const WrapperTypeInfo* type = toWrapperTypeInfo(obj);
        if (V8DOMException::wrapperTypeInfo.isSubclass(type)) {
            DOMException* exception = V8DOMException::toImpl(obj);
            if (exception && !exception->messageForConsole().isEmpty())
                return exception->toStringForConsole();
        }
    }
    return emptyString();
}

namespace {
    MessageLevel MessageLevelFromNonFatalErrorLevel(int errorLevel)
    {
        MessageLevel level = ErrorMessageLevel;
        switch (errorLevel) {
        case v8::Isolate::kMessageLog:
            level = LogMessageLevel;
            break;
        case v8::Isolate::kMessageWarning:
            level = WarningMessageLevel;
            break;
        case v8::Isolate::kMessageDebug:
            level = DebugMessageLevel;
            break;
        case v8::Isolate::kMessageInfo:
            level = InfoMessageLevel;
            break;
        case v8::Isolate::kMessageError:
            level = InfoMessageLevel;
            break;
        default:
            NOTREACHED();
        }
        return level;
    }

    const size_t kWasmWireBytesLimit = 1 << 12;

} // namespace

void V8Initializer::messageHandlerInMainThread(v8::Local<v8::Message> message, v8::Local<v8::Value> data)
{
    ASSERT(isMainThread());
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    if (isolate->GetEnteredContext().IsEmpty())
        return;

    // If called during context initialization, there will be no entered context.
    ScriptState* scriptState = ScriptState::current(isolate);
    if (!scriptState->contextIsValid())
        return;

    ExecutionContext* context = scriptState->getExecutionContext();
    std::unique_ptr<SourceLocation> locationToPrint = SourceLocation::fromMessage(isolate, message, context);

    context->addConsoleMessage(ConsoleMessage::create(
      JSMessageSource,
      MessageLevelFromNonFatalErrorLevel(message->ErrorLevel()),
      toCoreStringWithNullCheck(message->Get()), std::move(locationToPrint)));

    int errorLevel = message->ErrorLevel();
    if (errorLevel != v8::Isolate::kMessageError)
      return;

    std::unique_ptr<SourceLocation> location = SourceLocation::fromMessage(isolate, message, context);
    AccessControlStatus accessControlStatus = NotSharableCrossOrigin;
    if (message->IsOpaque())
        accessControlStatus = OpaqueResource;
    else if (message->IsSharedCrossOrigin())
        accessControlStatus = SharableCrossOrigin;

    ErrorEvent* event = ErrorEvent::create(toCoreStringWithNullCheck(message->Get()), std::move(location), &scriptState->world());

    String messageForConsole = extractMessageForConsole(isolate, data);
    if (!messageForConsole.isEmpty())
        event->setUnsanitizedMessage("Uncaught " + messageForConsole);

    V8ErrorHandler::storeExceptionOnErrorEventWrapper(scriptState, event, data, scriptState->context()->Global());
    context->dispatchErrorEvent(event, accessControlStatus);
}

namespace {

    static RejectedPromises& rejectedPromisesOnMainThread()
    {
        ASSERT(isMainThread());
        DEFINE_STATIC_LOCAL(RefPtr<RejectedPromises>, rejectedPromises,
            (RejectedPromises::create()));
        return *rejectedPromises;
    }

} // namespace

void V8Initializer::reportRejectedPromisesOnMainThread()
{
    rejectedPromisesOnMainThread().processQueue();
}

static void promiseRejectHandler(v8::PromiseRejectMessage data,
    RejectedPromises& rejectedPromises,
    ScriptState* scriptState)
{
    if (data.GetEvent() == v8::kPromiseHandlerAddedAfterReject) {
        rejectedPromises.handlerAdded(data);
        return;
    }

    // Ignore reject/resolve after resolved.
#if V8_MAJOR_VERSION >= 7
    if (data.GetEvent() == v8::kPromiseRejectAfterResolved || data.GetEvent() == v8::kPromiseResolveAfterResolved)
        return;
#endif
    ASSERT(data.GetEvent() == v8::kPromiseRejectWithNoHandler);

    v8::Local<v8::Promise> promise = data.GetPromise();
    v8::Isolate* isolate = promise->GetIsolate();
    ExecutionContext* context = scriptState->getExecutionContext();

    v8::Local<v8::Value> exception = data.GetValue();
    if (V8DOMWrapper::isWrapper(isolate, exception)) {
        // Try to get the stack & location from a wrapped exception object (e.g.
        // DOMException).
        ASSERT(exception->IsObject());
        auto privateError = V8PrivateProperty::getDOMExceptionError(isolate);
        v8::Local<v8::Value> error = privateError.getOrUndefined(
            scriptState->context(), exception.As<v8::Object>());
        if (!error->IsUndefined())
            exception = error;
    }

    String errorMessage;
    AccessControlStatus corsStatus = NotSharableCrossOrigin;
    std::unique_ptr<SourceLocation> location;

    v8::Local<v8::Message> message = v8::Exception::CreateMessage(isolate, exception);
    if (!message.IsEmpty()) {
        // message->Get() can be empty here. https://crbug.com/450330
        errorMessage = toCoreStringWithNullCheck(message->Get());
        location = SourceLocation::fromMessage(isolate, message, context);
        if (message->IsSharedCrossOrigin())
            corsStatus = SharableCrossOrigin;
    } else {
        location = SourceLocation::create(context->url().getString(), 0, 0, nullptr);
    }

    String messageForConsole = extractMessageForConsole(isolate, data.GetValue());
    if (!messageForConsole.isEmpty())
        errorMessage = "Uncaught " + messageForConsole;

    rejectedPromises.rejectedWithNoHandler(scriptState, data, errorMessage,
        std::move(location), corsStatus);
}

static void promiseRejectHandlerInMainThread(v8::PromiseRejectMessage data)
{
    ASSERT(isMainThread());

    v8::Local<v8::Promise> promise = data.GetPromise();

    v8::Isolate* isolate = promise->GetIsolate();

    // TODO(ikilpatrick): Remove this check, extensions tests that use
    // extensions::ModuleSystemTest incorrectly don't have a valid script state.
    LocalDOMWindow* window = currentDOMWindow(isolate);
    if (!window || !window->isCurrentlyDisplayedInFrame())
        return;

    // Bail out if called during context initialization.
    ScriptState* scriptState = ScriptState::current(isolate);
    if (!scriptState->contextIsValid())
        return;

    promiseRejectHandler(data, rejectedPromisesOnMainThread(), scriptState);
}

static void promiseRejectHandlerInWorker(v8::PromiseRejectMessage data)
{
    v8::Local<v8::Promise> promise = data.GetPromise();

    // Bail out if called during context initialization.
    v8::Isolate* isolate = promise->GetIsolate();
    ScriptState* scriptState = ScriptState::current(isolate);
    if (!scriptState->contextIsValid())
        return;

    ExecutionContext* executionContext = scriptState->getExecutionContext();
    if (!executionContext)
        return;

    ASSERT(executionContext->isWorkerGlobalScope());
    WorkerOrWorkletScriptController* scriptController = toWorkerGlobalScope(executionContext)->scriptController();
    ASSERT(scriptController);

    promiseRejectHandler(data, *scriptController->getRejectedPromises(),
        scriptState);
}

static void failedAccessCheckCallbackInMainThread(v8::Local<v8::Object> host,
    v8::AccessType type,
    v8::Local<v8::Value> data)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    Frame* target = findFrame(isolate, host, data);
    // FIXME: We should modify V8 to pass in more contextual information (context,
    // property, and object).
    BindingSecurity::failedAccessCheckFor(isolate, target);
}

#if V8_MAJOR_VERSION < 10
static bool codeGenerationCheckCallbackInMainThread(v8::Local<v8::Context> context, v8::Local<v8::String> source)
{
    if (ExecutionContext* executionContext = toExecutionContext(context)) {
        if (ContentSecurityPolicy* policy = toDocument(executionContext)->contentSecurityPolicy())
            return policy->allowEval(ScriptState::from(context), ContentSecurityPolicy::SendReport, ContentSecurityPolicy::WillThrowException);
    }
    return false;
}
#else
static v8::ModifyCodeGenerationFromStringsResult
codeGenerationCheckCallbackInMainThread(v8::Local<v8::Context> context, v8::Local<v8::Value> source, bool is_code_like)
{
    v8::ModifyCodeGenerationFromStringsResult result;
    if (ExecutionContext* executionContext = toExecutionContext(context)) {
        if (ContentSecurityPolicy* policy = toDocument(executionContext)->contentSecurityPolicy())
            result.codegen_allowed = policy->allowEval(ScriptState::from(context), ContentSecurityPolicy::SendReport, ContentSecurityPolicy::WillThrowException);
    }
    return result;
}
#endif

static bool allowWasmCompileCallbackInMainThread(v8::Isolate* isolate,
    v8::Local<v8::Value> source,
    bool asPromise)
{
    // We allow async compilation irrespective of buffer size.
    if (asPromise)
        return true;
    if (source->IsArrayBuffer() && v8::Local<v8::ArrayBuffer>::Cast(source)->ByteLength() > kWasmWireBytesLimit) {
        return false;
    }
    if (source->IsArrayBufferView() && v8::Local<v8::ArrayBufferView>::Cast(source)->ByteLength() > kWasmWireBytesLimit) {
        return false;
    }
    return true;
}

// static bool allowWasmInstantiateCallbackInMainThread(
//     v8::Isolate* isolate,
//     v8::Local<v8::Value> source,
//     v8::MaybeLocal<v8::Value> ffi,
//     bool asPromise)
// {
//     // Async cases are allowed, regardless of the size of the
//     // wire bytes. Note that, for instantiation, we use the wire
//     // bytes size as a proxy for instantiation time. We may
//     // consider using the size of the ffi (nr of properties)
//     // instead, or, even more directly, number of imports.
//     if (asPromise)
//         return true;
//     // If it's not a promise, the source should be a wasm module
//     DCHECK(source->IsWebAssemblyCompiledModule());
//     v8::Local<v8::WasmCompiledModule> module = v8::Local<v8::WasmCompiledModule>::Cast(source);
//     if (static_cast<size_t>(module->GetWasmWireBytes()->Length()) > kWasmWireBytesLimit) {
//         return false;
//     }
//     return true;
// }

#if V8_MAJOR_VERSION >= 7
#if V8_MAJOR_VERSION >= 7 && V8_MAJOR_VERSION < 10
static v8::MaybeLocal<v8::Promise> hostImportModuleDynamically(
    v8::Local<v8::Context> context,
    v8::Local<v8::ScriptOrModule> v8Referrer,
    v8::Local<v8::String> v8Specifier)
#else
v8::MaybeLocal<v8::Promise> hostImportModuleDynamically(
    v8::Local<v8::Context> context, v8::Local<v8::Data> host_defined_options,
    v8::Local<v8::Value> v8ReferrerUrl, v8::Local<v8::String> v8Specifier,
    v8::Local<v8::FixedArray> importAssertions)
#endif
{
    v8::EscapableHandleScope handleScope(context->GetIsolate());
    v8::Context::Scope scope(context);
    ScriptState* scriptState = ScriptState::from(context);
    ScriptPromiseResolver* resolver = ScriptPromiseResolver::create(scriptState);
    ScriptPromise promise = resolver->promise();

    Modulator* modulator = Modulator::from(context);
    if (!modulator) {
        resolver->reject();
        return v8::Local<v8::Promise>::Cast(promise.v8Value());
    }

    String specifier = toCoreStringWithNullCheck(v8Specifier);
#if V8_MAJOR_VERSION >= 7 && V8_MAJOR_VERSION < 10
    v8::Local<v8::Value> v8ReferrerUrl = v8Referrer->GetResourceName();
#endif
    KURL referrerUrl;
    if (v8ReferrerUrl->IsString()) {
        String referrerResourceUrlStr = toCoreString(v8::Local<v8::String>::Cast(v8ReferrerUrl));
        if (!referrerResourceUrlStr.isEmpty())
            referrerUrl = KURL(ParsedURLString, referrerResourceUrlStr);
    }

    //ReferrerScriptInfo referrer_info = ReferrerScriptInfo::FromV8HostDefinedOptions(context, v8_referrer->GetHostDefinedOptions());
    modulator->resolveDynamically(scriptState, specifier, referrerUrl, /*referrer_info, */resolver);
    return v8::Local<v8::Promise>::Cast(promise.v8Value());

}

// https://html.spec.whatwg.org/C/#hostgetimportmetaproperties
static void hostGetImportMetaProperties(v8::Local<v8::Context> context,
    v8::Local<v8::Module> module,
    v8::Local<v8::Object> meta) {
    //ScriptState* scriptState = ScriptState::from(context);
    v8::Isolate* isolate = context->GetIsolate();
    v8::HandleScope handle_scope(isolate);

    Modulator* modulator = Modulator::from(context);
    if (!modulator)
        return;

    // TODO(shivanisha): Can a valid source url be passed to the constructor.
    //ModuleImportMeta host_meta = modulator->HostGetImportMetaProperties(ModuleRecord(isolate, module, KURL()));
    ModuleRecord* moduleRecord = modulator->getModuleRecordById(module->GetIdentityHash());
    if (!moduleRecord)
        return;

    // 3. Return <<Record { [[Key]]: "url", [[Value]]: urlString }>>. [spec text]
    v8::Local<v8::String> urlKey = v8String(isolate, "url");
    v8::Local<v8::String> urlValue = v8String(isolate, /*host_meta.Url()*/moduleRecord->url().getUTF8String());
    meta->CreateDataProperty(context, urlKey, urlValue).ToChecked();
}
#endif

static void initializeV8Common(v8::Isolate* isolate)
{
    isolate->AddGCPrologueCallback(V8GCController::gcPrologue);
    isolate->AddGCEpilogueCallback(V8GCController::gcEpilogue);
    if (RuntimeEnabledFeatures::traceWrappablesEnabled()) {
        std::unique_ptr<ScriptWrappableVisitor> visitor(
            new ScriptWrappableVisitor(isolate));
        V8PerIsolateData::from(isolate)->setScriptWrappableVisitor(
            std::move(visitor));
        isolate->SetEmbedderHeapTracer(
            V8PerIsolateData::from(isolate)->scriptWrappableVisitor());
    }

#if V8_MAJOR_VERSION >= 7
    isolate->SetHostImportModuleDynamicallyCallback(hostImportModuleDynamically);
    isolate->SetHostInitializeImportMetaObjectCallback(hostGetImportMetaProperties);
#endif
#if V8_MAJOR_VERSION <= 7
    v8::Debug::SetLiveEditEnabled(isolate, false);
#endif

    isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kScoped);
}

namespace {

    class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
        // Allocate() methods return null to signal allocation failure to V8, which
        // should respond by throwing a RangeError, per
        // http://www.ecma-international.org/ecma-262/6.0/#sec-createbytedatablock.
        void* Allocate(size_t size) override
        {
            void* data;
            WTF::ArrayBufferContents::allocateMemoryOrNull(size, WTF::ArrayBufferContents::ZeroInitialize, data);
            return data;
        }

        void* AllocateUninitialized(size_t size) override
        {
            void* data;
            WTF::ArrayBufferContents::allocateMemoryOrNull(size, WTF::ArrayBufferContents::DontInitialize, data);
            return data;
        }

        void Free(void* data, size_t size) override
        {
            WTF::ArrayBufferContents::freeMemory(data, size);
        }
    };

} // namespace

static void adjustAmountOfExternalAllocatedMemory(int64_t diff)
{
#if DCHECK_IS_ON()
    DEFINE_THREAD_SAFE_STATIC_LOCAL(int64_t, processTotal, new int64_t(0));
    DEFINE_THREAD_SAFE_STATIC_LOCAL(RecursiveMutex, mutex, new RecursiveMutex);
    {
        MutexLocker locker(mutex);

        processTotal += diff;
        DCHECK_GE(processTotal, 0) << "total amount = " << processTotal
                                   << ", diff = " << diff;
    }
#endif

    v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(diff);
}

void V8Initializer::initializeMainThread()
{
    ASSERT(isMainThread());

    WTF::ArrayBufferContents::initialize(adjustAmountOfExternalAllocatedMemory);

    //DEFINE_STATIC_LOCAL(ArrayBufferAllocator, arrayBufferAllocator, ());
    //   auto v8ExtrasMode = RuntimeEnabledFeatures::experimentalV8ExtrasEnabled()
    //                           ? gin::IsolateHolder::kStableAndExperimentalV8Extras
    //                           : gin::IsolateHolder::kStableV8Extras;
    printf("V8Initializer::initializeMainThread\n");
    //gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode, /*v8ExtrasMode,*/ &arrayBufferAllocator);

    // NOTE: Some threads (namely utility threads) don't have a scheduler.
    WebScheduler* scheduler = Platform::current()->currentThread()->scheduler();
    // When timer task runner is used for PerIsolateData, GC tasks are getting
    // throttled and memory usage goes up. For now we're using loading task queue
    // to prevent this.
    // TODO(altimin): Consider switching to timerTaskRunner here.
    v8::Isolate* isolate = V8PerIsolateData::initialize(
        scheduler ? scheduler->loadingTaskRunner()
                  : Platform::current()->currentThread()->getWebTaskRunner());

    initializeV8Common(isolate);

    isolate->SetOOMErrorHandler(reportOOMErrorInMainThread);
    isolate->SetFatalErrorHandler(reportFatalErrorInMainThread);
    isolate->AddMessageListenerWithErrorLevel(
        messageHandlerInMainThread,
        v8::Isolate::kMessageError | v8::Isolate::kMessageWarning | v8::Isolate::kMessageInfo | v8::Isolate::kMessageDebug | v8::Isolate::kMessageLog);
    isolate->SetFailedAccessCheckCallbackFunction(failedAccessCheckCallbackInMainThread);
#if V8_MAJOR_VERSION <= 7
    isolate->SetAllowCodeGenerationFromStringsCallback(codeGenerationCheckCallbackInMainThread);
#else
    isolate->SetModifyCodeGenerationFromStringsCallback(codeGenerationCheckCallbackInMainThread);
#endif

    //   isolate->SetAllowWasmCompileCallback(allowWasmCompileCallbackInMainThread);
    //   isolate->SetAllowWasmInstantiateCallback(allowWasmInstantiateCallbackInMainThread);
    if (RuntimeEnabledFeatures::v8IdleTasksEnabled()) {
        V8PerIsolateData::enableIdleTasks(
            isolate, WTF::makeUnique<V8IdleTaskRunner>(scheduler));
    }

    isolate->SetPromiseRejectCallback(promiseRejectHandlerInMainThread);

#if V8_MAJOR_VERSION < 10
    if (v8::HeapProfiler* profiler = isolate->GetHeapProfiler())
        profiler->SetWrapperClassInfoProvider(
            WrapperTypeInfo::NodeClassId, &RetainedDOMInfo::createRetainedDOMInfo);
#endif
    ASSERT(ThreadState::mainThreadState());
    ThreadState::mainThreadState()->addInterruptor(
        WTF::makeUnique<V8IsolateInterruptor>(isolate));
    ThreadState::mainThreadState()->registerTraceDOMWrappers(
        isolate, V8GCController::traceDOMWrappers,
        ScriptWrappableVisitor::invalidateDeadObjectsInMarkingDeque,
        ScriptWrappableVisitor::performCleanup);

    V8PerIsolateData::from(isolate)->setThreadDebugger(
        WTF::makeUnique<MainThreadDebugger>(isolate));
}

void V8Initializer::shutdownMainThread()
{
    ASSERT(isMainThread());
    v8::Isolate* isolate = V8PerIsolateData::mainThreadIsolate();
    V8PerIsolateData::willBeDestroyed(isolate);
    V8PerIsolateData::destroy(isolate);
}

static void reportFatalErrorInWorker(const char* location,
    const char* message)
{
    // FIXME: We temporarily deal with V8 internal error situations such as
    // out-of-memory by crashing the worker.
    CRASH();
}

static void messageHandlerInWorker(v8::Local<v8::Message> message,
    v8::Local<v8::Value> data)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    V8PerIsolateData* perIsolateData = V8PerIsolateData::from(isolate);

    // During the frame teardown, there may not be a valid context.
    ScriptState* scriptState = ScriptState::current(isolate);
    if (!scriptState->contextIsValid())
        return;

    // Exceptions that occur in error handler should be ignored since in that case
    // WorkerGlobalScope::dispatchErrorEvent will send the exception to the worker
    // object.
    if (perIsolateData->isReportingException())
        return;

    perIsolateData->setReportingException(true);

    ExecutionContext* context = scriptState->getExecutionContext();
    std::unique_ptr<SourceLocation> location = SourceLocation::fromMessage(isolate, message, context);

    if (message->ErrorLevel() != v8::Isolate::kMessageError) {
        context->addConsoleMessage(ConsoleMessage::create(
            JSMessageSource,
            MessageLevelFromNonFatalErrorLevel(message->ErrorLevel()),
            toCoreStringWithNullCheck(message->Get()), std::move(location)));
        return;
    }

    ErrorEvent* event = ErrorEvent::create(toCoreStringWithNullCheck(message->Get()),
        std::move(location), &scriptState->world());

    AccessControlStatus corsStatus = message->IsSharedCrossOrigin()
        ? SharableCrossOrigin
        : NotSharableCrossOrigin;

    // If execution termination has been triggered as part of constructing
    // the error event from the v8::Message, quietly leave.
    if (!isolate->IsExecutionTerminating()) {
        V8ErrorHandler::storeExceptionOnErrorEventWrapper(
            scriptState, event, data, scriptState->context()->Global());
        scriptState->getExecutionContext()->dispatchErrorEvent(event, corsStatus);
    }

    perIsolateData->setReportingException(false);
}

// Stack size for workers is limited to 500KB because default stack size for
// secondary threads is 512KB on Mac OS X. See GetDefaultThreadStackSize() in
// base/threading/platform_thread_mac.mm for details.
static const int kWorkerMaxStackSize = 500 * 1024;

// This function uses a local stack variable to determine the isolate's stack
// limit. AddressSanitizer may relocate that local variable to a fake stack,
// which may lead to problems during JavaScript execution.  Therefore we disable
// AddressSanitizer for V8Initializer::initializeWorker().
NO_SANITIZE_ADDRESS
void V8Initializer::initializeWorker(v8::Isolate* isolate)
{
    initializeV8Common(isolate);

    isolate->AddMessageListenerWithErrorLevel(
        messageHandlerInWorker,
        v8::Isolate::kMessageError | v8::Isolate::kMessageWarning | v8::Isolate::kMessageInfo | v8::Isolate::kMessageDebug | v8::Isolate::kMessageLog);
    isolate->SetFatalErrorHandler(reportFatalErrorInWorker);

    uint32_t here;
    isolate->SetStackLimit(reinterpret_cast<uintptr_t>(&here) - kWorkerMaxStackSize);
    isolate->SetPromiseRejectCallback(promiseRejectHandlerInWorker);
}

} // namespace blink
