#if (defined ENABLE_WKE) && (ENABLE_WKE == 1)

#define BUILDING_wke

#include "content/browser/WebPage.h"

#include "wke/wkeWebView.h"
#include "wke/wkeJsBind.h"
#include "wke/wkeGlobalVar.h"

#include "content/web_impl_win/BlinkPlatformImpl.h"
#include "content/browser/WebFrameClientImpl.h"
#include "content/browser/PostTaskHelper.h"
#include "content/browser/TouchStruct.h"
#include "third_party/WebKit/public/platform/WebDragData.h"
#include "third_party/WebKit/public/platform/WebThread.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/platform/WebCachePolicy.h"
#include "third_party/WebKit/Source/web/WebViewImpl.h"
#include "third_party/WebKit/Source/web/WebSettingsImpl.h"
#include "third_party/WebKit/Source/platform/UserGestureIndicator.h"
#include "third_party/WebKit/Source/bindings/core/v8/ExceptionState.h"
#include "third_party/WebKit/Source/wtf/text/WTFStringUtil.h"
#include "third_party/WebKit/Source/wtf/text/qt4/mbchar.h"
#include "net/ActivatingObjCheck.h"
#include "net/WebURLLoaderManagerUtil.h"
#include "net/cookies/WebCookieJarCurlImpl.h"
#include "base/path_service.h"
#include "mbvip/core/mb.h"

#if defined(WIN32)
#undef  PURE
#define PURE = 0;
#define interface struct
#include <shlwapi.h>
#endif

namespace wke {

CWebView::CWebView(COLORREF color)
    : m_hWnd(NULL)
    , m_transparent(false)
    , m_width(0)
    , m_height(0)
    , m_zoomFactor(1)
    , m_awake(true)
    , m_title("", 0, true)
    , m_cookie("", 0, true)
    , m_name("miniblink", 0, true)
    , m_url("", 0, true)
    , m_isCokieEnabled(true)
    , m_isCreatedDevTools(false)
{
    m_id = net::ActivatingObjCheck::inst()->genId();
    net::ActivatingObjCheck::inst()->add(m_id);
    g_liveWebViews.insert(this);

    content::BlinkPlatformImpl* platform = (content::BlinkPlatformImpl*)blink::Platform::current();
    m_zoomFactor = platform->getZoom();

    _initPage(color);
    _initHandler();
    _initMemoryDC();

    m_settings.size = 0;
    //m_webPage->wkeHandler().isWke = true;    
}

CWebView::~CWebView()
{
    if (!net::ActivatingObjCheck::inst()->isActivating(m_id))
        return;
    net::ActivatingObjCheck::inst()->remove(m_id);
    g_liveWebViews.erase(this);
    m_webPage->close();
}

bool CWebView::create()
{
    return true;
}

void CWebView::destroy()
{
    delete this;
}

bool CWebView::isValid()
{
    return m_webPage->isValid();
}

void CWebView::shutdown()
{
    std::set<wkeWebView>::iterator it = wke::g_liveWebViews.begin();
    while (it != wke::g_liveWebViews.end()) {
        wkeWebView webView = *it;
        HWND hWnd = wkeGetHostHWND(webView);
        webView->setWillDestroy();
        delete webView;

        it = wke::g_liveWebViews.begin();
    }
}

void CWebView::setWillDestroy()
{
    m_webPage->setWillDestroy();
}

const utf8* CWebView::name() const
{
    return m_name.string();
}

const WCHAR* CWebView::nameW() const
{
    return m_name.stringW();
}

void CWebView::setName(const utf8* name)
{
    m_name.setString(name);
}

void CWebView::setName(const WCHAR* name)
{
#if defined(OS_WIN)
    m_name.setString(name);
#else
    DebugBreak();
#endif
}

bool CWebView::isTransparent() const
{
    return m_transparent;
}

void CWebView::setTransparent(bool transparent)
{
    if (m_transparent == transparent)
        return;

    m_transparent = transparent;

    m_webPage->setTransparent(transparent);
}

void CWebView::loadPostURL(const utf8* inUrl, const char * poastData, int nLen )
{
    blink::KURL url(blink::ParsedURLString, inUrl);
    if (!url.isValid())
        url.setProtocol("http:");

    if (!url.isValid())
        return;

    if (blink::protocolIsJavaScript(url)) {
        //m_mainFrame->script()->executeIfJavaScriptURL(url);
        return;
    }

    blink::WebURLRequest request(url);
    request.setCachePolicy(blink::WebCachePolicy::UseProtocolCachePolicy);
    request.setHTTPMethod(blink::WebString::fromUTF8("POST"));

    blink::WebHTTPBody body;
    body.initialize();
    body.appendData(blink::WebData(poastData, nLen));
    request.setHTTPBody(body);
    m_webPage->loadRequest(content::WebPage::kMainFrameId, request, false);
}

void CWebView::loadPostURL(const WCHAR* inUrl,const char * poastData, int nLen)
{
#if defined(OS_WIN)
    loadPostURL(String(inUrl).utf8().data(), poastData,nLen);
#endif
    DebugBreak();
}

static bool checkIsFileUrl(const utf8* inUrl)
{
    if (nullptr != strstr(inUrl, "http:"))
        return false;
    if (nullptr != strstr(inUrl, "https:"))
        return false;
    if (nullptr != strstr(inUrl, "file:"))
        return true;
    return false;
}

static bool trimPathBody(const utf8* inUrl, int length, bool isFile, std::vector<char>* out)
{
    out->clear();

    const char* fileBody = inUrl;
    int fileBodyLength = length;

    if (fileBodyLength <= 3)
        return false;
#if defined(WIN32) 
    int len = 0;
    if (fileBody[1] != ':' && isFile) { // xxx.htm
        Vector<WCHAR> filenameBuffer;
        filenameBuffer.resize(MAX_PATH + 3);
        ::GetCurrentDirectoryW(MAX_PATH, filenameBuffer.data());

        //int pathLength = base::c16len(filenameBuffer.data());
        int pathLength = 0;
        while (*(filenameBuffer.data() + pathLength)) {
            ++pathLength;
        }

        if (pathLength <= 1 || pathLength > MAX_PATH)
            return false;

        if (filenameBuffer[pathLength - 1] != L'\\') {
            filenameBuffer[pathLength] = L'\\';
            filenameBuffer[pathLength + 1] = L'\0';
            pathLength += 1;
        }

        WTF::WCharToMByte(filenameBuffer.data(), pathLength, out, CP_UTF8);
        len = out->size();
    } else { // c:/xxx.htm

    }

    out->resize(len + fileBodyLength);
    strncpy(&out->at(0) + len, fileBody, fileBodyLength);
    out->push_back('\0');
    return true;

#else
    base::FilePath fullpath;
    base::FilePath file(fileBody);
    if (fileBody[0] != '/' && isFile) { // xxx.htm
        base::PathService::Get(base::DIR_MODULE, &fullpath);        
        fullpath = fullpath.Append(file);
    } else {
        fullpath = file;
    }
    std::string path = fullpath.AsUTF8Unsafe();
    printf("trimPathBody: %s\n", path.c_str());

    out->resize(path.size());
    strncpy(out->data(), path.c_str(), path.size());
    out->push_back('\0');
    return true;
#endif
}

static bool trimPath(const utf8* inUrl, bool isFile, bool* isViewSource, std::vector<char>* out)
{
    // linux下，file:///是绝对路径，/开头也是绝对路径
    int length = strlen(inUrl);
#if defined(WIN32) 
    const char* fileHead = "file:///";
#else
    const char* fileHead = "file://";
#endif
    int fileHeadLength = strlen(fileHead);
    const char* fileBody = inUrl;
    int fileBodyLength = length;

    const char* viewSourceHead = "view-source:";
    int viewSourceHeadLength = strlen(viewSourceHead);

    if (length < 3)
        return false;

    if (length > viewSourceHeadLength) {
        if (0 == strncmp(inUrl, viewSourceHead, viewSourceHeadLength)) {
            *isViewSource = true;
            out->clear();
            out->resize(length - viewSourceHeadLength);
            strncpy(&out->at(0), inUrl + viewSourceHeadLength, length - viewSourceHeadLength);
            out->push_back('\0');
            return true;
        }
    }

    if (length > fileHeadLength) {
        if (0 == strncmp(inUrl, fileHead, fileHeadLength)) { // file:///xxx.htm  file:///c:/xxx.htm
            fileBody = inUrl + fileHeadLength;
            fileBodyLength = length - fileHeadLength;
            if (fileBodyLength < 3)
                return false;

            return trimPathBody(fileBody, fileBodyLength, isFile, out);
        } else {
            return trimPathBody(fileBody, fileBodyLength, isFile, out); // xxx.htm  c:/xxx.htm
        }
    } else
        return trimPathBody(fileBody, fileBodyLength, isFile, out); // xxx.htm  c:/xxx.htm
}

void CWebView::_loadURL(const utf8* inUrl, bool isFile)
{
    std::vector<char> inUrlBuf;
    bool isViewSource = false;
    if (!trimPath(inUrl, isFile, &isViewSource, &inUrlBuf))
        return;

    //cexer 必须调用String::fromUTF8显示构造第二个参数，否则String::String会把inUrl当作latin1处理。
    blink::KURL url(blink::ParsedURLString, &inUrlBuf[0]);
    if (!url.isValid())
        url.setProtocol("http:");

    if (!url.isValid())
        return;

    if (blink::protocolIsJavaScript(url)) {
        //m_mainFrame->script()->executeIfJavaScriptURL(url);
        return;
    }

    printf("CWebView::_loadURL: [%s] [%s] [%s]\n", inUrl, &inUrlBuf[0], url.getUTF8String().utf8().data());

    m_url = &inUrlBuf[0];
    blink::WebURLRequest request(url);
    request.setCachePolicy(blink::WebCachePolicy::UseProtocolCachePolicy);
    request.setHTTPMethod(blink::WebString::fromUTF8("GET"));
    m_webPage->loadRequest(content::WebPage::kMainFrameId, request, isViewSource);
}

void CWebView::loadURL(const utf8* inUrl)
{
    _loadURL(inUrl, checkIsFileUrl(inUrl));
}

void CWebView::loadURL(const WCHAR* url)
{
#if defined(OS_WIN)
    loadURL(String(url).utf8().data());
#endif
    DebugBreak();
}

static String createMemoryUrl()
{
#if defined(OS_WIN)
    std::vector<WCHAR> path;
    path.resize(MAX_PATH + 1);
    memset(&path[0], 0, sizeof(WCHAR) * (MAX_PATH + 1));
    ::GetModuleFileNameW(nullptr, &path[0], MAX_PATH);
    ::PathRemoveFileSpecW(&path[0]);

    String result(&path[0]);
    result = WTF::ensureUTF16String(result);
    result.replace(u16("\\"), u16("/"));
    result.insert(u16("file:///"), 0);
    result.append(String::format("/_miniblink__data_%u.htm", GetTickCount()));

    return WTF::ensureStringToUTF8String(result);
#else
    DebugBreak();
    return String();
#endif
}

void CWebView::loadHTML(const utf8* html)
{
    loadHtmlWithBaseUrl(html, nullptr);
}

void CWebView::loadHtmlWithBaseUrl(const utf8* html, const utf8* baseUrl)
{
    size_t length = strlen(html);
    if (0 == length)
        return;
    String url = createMemoryUrl(); // String::format("MemoryURL://data.com/%d", GetTickCount());

    blink::KURL kurl(blink::ParsedURLString, url);
    blink::KURL kbaseUrl;
    if (baseUrl)
        kbaseUrl = blink::KURL(blink::ParsedURLString, baseUrl);
    if (!kbaseUrl.isValid())
        kbaseUrl = kurl;

    m_webPage->loadHTMLString(content::WebPage::kMainFrameId, blink::WebData(html, length), kbaseUrl, kbaseUrl, true);
}

void CWebView::loadHTML(const WCHAR* html)
{
#if defined(OS_WIN)
    size_t length = wcslen(html);
    if (0 == length)
        return;
    String htmlUTF8((UChar*)html, length);
    Vector<char> htmlUTF8Buf = WTF::ensureStringToUTF8(htmlUTF8, false);

    String url = createMemoryUrl(); // String::format("MemoryURL://data.com/%d", GetTickCount());
    m_webPage->loadHTMLString(content::WebPage::kMainFrameId, blink::WebData(htmlUTF8Buf.data(), htmlUTF8Buf.size()), blink::KURL(blink::ParsedURLString, url), blink::WebURL(), true);
#else
    DebugBreak();
#endif
}

void CWebView::loadFile(const utf8* filename)
{
    if (!filename)
        return;
    size_t length = strlen(filename);
    if (length < 4)
        return;

    //String filenameUTF8(filename, length);
    //loadFile(ensureUTF16UChar(filenameUTF8, true).data());
    _loadURL(filename, true);
}

void CWebView::loadFile(const WCHAR* filename)
{
#if defined(OS_WIN)
    if (!filename)
        return;
    size_t length = wcslen(filename);
    if (length < 4)
        return;

    String filenameA(filename, length);
    _loadURL(WTF::ensureStringToUTF8(filenameA, true).data(), true);
#else
    DebugBreak();
#endif
}

const utf8* CWebView::url() const
{
    return m_url.string();
}

void CWebView::onUrlChanged(const wkeString url)
{
    m_url = *url;
}

bool CWebView::isLoading() const
{
    content::WebFrameClientImpl* frameClient = m_webPage->webFrameClientImpl();
    return frameClient->isLoading();
}

bool CWebView::isLoadingSucceeded() const
{
    content::WebFrameClientImpl* frameClient = m_webPage->webFrameClientImpl();
    bool b = frameClient->isLoaded();
    return b;
}

bool CWebView::isLoadingFailed() const
{
    content::WebFrameClientImpl* frameClient = m_webPage->webFrameClientImpl();
    bool b = frameClient->isLoadFailed();
    return b;
}

bool CWebView::isLoadingCompleted() const
{
    bool b = isLoadingSucceeded() || isLoadingFailed();
    return b;
}

bool CWebView::isDocumentReady() const
{
    content::WebFrameClientImpl* frameClient = m_webPage->webFrameClientImpl();
    return frameClient->isDocumentReady();
}

void CWebView::setUserAgent(const utf8 * useragent)
{
    content::BlinkPlatformImpl* platform = (content::BlinkPlatformImpl*)blink::Platform::current();
    platform->setUserAgent((char *)useragent);
}

void CWebView::setUserAgent(const WCHAR * useragent)
{
#if defined(OS_WIN)
    setUserAgent(String(useragent).utf8().data());
#else
    DebugBreak();
#endif
}

void CWebView::stopLoading()
{
    m_webPage->mainFrame()->stopLoading();
}

void CWebView::reload()
{
    net::WebURLLoaderManager::sharedInstance()->cancelAllJobsOfWebview(m_id);
    m_webPage->mainFrame()->reload(blink::WebFrameLoadType::Reload);
}

void CWebView::goToOffset(int offset)
{
    m_webPage->goToOffset(offset);
}

void CWebView::goToIndex(int index)
{
    m_webPage->goToIndex(index);
}

const utf8* CWebView::title()
{
    content::WebFrameClientImpl* frameClient = m_webPage->webFrameClientImpl();
    m_title = frameClient->title();

    return m_title.string();
}

const WCHAR* CWebView::titleW()
{
#if defined(OS_WIN)
    content::WebFrameClientImpl* frameClient = m_webPage->webFrameClientImpl();
    m_title = frameClient->title();

    return m_title.stringW();
#else
    DebugBreak();
    return nullptr;
#endif
}

void CWebView::resize(int w, int h)
{
    m_webPage->fireResizeEvent(m_webPage->getHWND(), WM_SIZE, 0, MAKELONG(w, h));
}

int CWebView::width() const 
{ 
    return m_webPage->viewportSize().width();
}

int CWebView::height() const 
{ 
    return m_webPage->viewportSize().height();
}

int CWebView::contentsWidth() const
{
    return contentWidth();
}

int CWebView::contentsHeight() const
{
    return contentHeight();
}

int CWebView::contentWidth() const
{
    return m_webPage->mainFrame()->contentsSize().width;
}

int CWebView::contentHeight() const
{
    return m_webPage->mainFrame()->contentsSize().height;
}

void CWebView::setDirty(bool dirty)
{
    m_webPage->setNeedsCommit();
}

bool CWebView::isDirty() const
{
    return m_webPage->isDrawDirty();
}

void CWebView::addDirtyArea(int x, int y, int w, int h)
{
    if (w > 0 && h > 0) {
        m_dirtyArea.unite(blink::IntRect(x, y, w, h));
        setDirty(true);
    }
}

void CWebView::layoutIfNeeded()
{
    m_webPage->fireTimerEvent();
}

void CWebView::repaintIfNeeded()
{
    m_webPage->fireTimerEvent();

    RECT rc = { m_dirtyArea.x(), m_dirtyArea.y(), m_dirtyArea.maxX(), m_dirtyArea.maxY() };
    m_webPage->firePaintEvent(nullptr, WM_PAINT, (WPARAM)&rc, 0);

    m_dirtyArea = blink::IntRect();
}

HDC CWebView::viewDC()
{
    return m_webPage->viewDC();
}

void CWebView::releaseHdc()
{
    m_webPage->releaseHdc();
}

HWND CWebView::windowHandle() const
{
    return m_hWnd;
}

void CWebView::setHandle(HWND wnd)
{
    m_hWnd = wnd;
    m_webPage->setHWND(wnd);
}

void CWebView::setHandleOffset(int x, int y)
{
    blink::IntPoint offset(x, y);
    m_webPage->setHwndRenderOffset(offset);
}

void CWebView::setDragDropEnable(bool b)
{
    m_webPage->setDragDropEnable(b);
}

void CWebView::setTouchSimulateEnabled(bool b)
{
    m_webPage->setTouchSimulateEnabled(b);
}

void CWebView::setSystemTouchEnabled(bool b)
{
    m_webPage->setSystemTouchEnabled(b);
}

void CWebView::setViewSettings(const wkeViewSettings* settings)
{
    if (!settings)
        return;

    m_settings = *settings;

    if (m_webPage)
        m_webPage->setBackgroundColor(m_settings.bgColor);
}

void CWebView::paint(void* bits, int pitch)
{
    if (m_webPage->needsCommit())
        m_webPage->fireTimerEvent();
    m_webPage->paintToBit(bits, pitch);
}

void CWebView::paint(void* bits, int bufWid, int bufHei, int xDst, int yDst, int w, int h, int xSrc, int ySrc, bool bCopyAlpha)
{
//     if (m_dirty)
//         repaintIfNeeded();
// 
//     if(xSrc + w > m_width) w = m_width - xSrc;
//     if(ySrc + h > m_height) h = m_height -ySrc;
//     
//     if(xDst + w > bufWid) w =bufWid - xDst;
//     if(yDst + h > bufHei) h = bufHei - yDst;
//     
//     int pitchDst = bufWid*4;
//     int pitchSrc = m_width*4;
//     
//     unsigned char* src = (unsigned char*)m_pixels; 
//     unsigned char* dst = (unsigned char*)bits; 
//     src += pitchSrc*ySrc + xSrc*4;
//     dst += yDst*pitchDst + xDst*4;
//     
//     if(bCopyAlpha) {
//         for(int j = 0; j< h; j++) {
//             memcpy(dst,src,w*4);
//             dst += pitchDst;
//             src += pitchSrc;
//         }
//     } else {
//         for(int j = 0; j< h; j++) {
//             for(int i=0;i<w;i++) {
//                 memcpy(dst,src,3);
//                 dst += 4;
//                 src += 4;
//             }
//             dst += (bufWid - w)*4;
//             src += (m_width - w)*4;
//         }
//     }
}

bool CWebView::canGoBack() const
{
    //return page()->backForwardList()->backItem() && !page()->defersLoading();
    return m_webPage->canGoBack();
}

bool CWebView::goBack()
{
    //return page()->goBack();
    m_webPage->goBack();
    return true;
}

bool CWebView::canGoForward() const
{
    //return page()->backForwardList()->forwardItem() && !page()->defersLoading();
    return m_webPage->canGoForward();
}

bool CWebView::goForward()
{
    //return page()->goForward();
    m_webPage->goForward();
    return true;
}

void CWebView::navigateAtIndex(int index)
{
    m_webPage->goToIndex(index);
}

int CWebView::getNavigateIndex() const
{
    return m_webPage->getNavigateIndex();
}

void CWebView::editorSelectAll()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("SelectAll");
}

void CWebView::editorUnSelect()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Unselect");
}

void CWebView::editorCopy()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Copy");
}

void CWebView::editorCut()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Cut");
}

void CWebView::editorPaste()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Paste");
}

void CWebView::editorDelete()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Delete");
}

void CWebView::editorUndo()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Undo");
}

void CWebView::editorRedo()
{
    ((blink::WebLocalFrame*)(m_webPage->mainFrame()))->executeCommand("Redo");
}

void CWebView::setCookieEnabled(bool enable)
{
    if (!m_webPage || !m_webPage->webViewImpl())
        return;

    blink::WebSettingsImpl* settings = m_webPage->webViewImpl()->settingsImpl();
    if (!settings)
        return;

    settings->setCookieEnabled(enable);
    m_isCokieEnabled = enable;
}

const WCHAR* CWebView::cookieW()
{
#if defined(OS_WIN)
    cookie();
    return m_cookie.stringW();
#else
    DebugBreak();
    return nullptr;
#endif
}

const utf8* CWebView::cookie()
{
    if (!m_webPage->mainFrame())
        return "";
    blink::WebDocument webDocument = m_webPage->mainFrame()->document();
    if (webDocument.isNull())
        return "";

    net::WebCookieJarImpl* cookieJar = getCookieJar();
    if (!cookieJar)
        return "";

    const blink::Document* doc = webDocument.constUnwrap<blink::Document>();
    m_cookie = cookieJar->getCookiesForSession(KURL(), doc->cookieURL(), true);

    return m_cookie.string();
}

bool CWebView::isCookieEnabled() const
{
    return m_isCokieEnabled;
}

void CWebView::setMediaVolume(float volume)
{
    //page()->setMediaVolume(volume);
}

float CWebView::mediaVolume() const
{
    //return page()->mediaVolume();
    return 0;
}

bool CWebView::fireMouseEvent(unsigned int message, int x, int y, unsigned int flags)
{
    if (message == WM_CANCELMODE) {
        //mainFrame()->eventHandler()->lostMouseCapture();
        return true;
    }

    // Create our event.
    // On WM_MOUSELEAVE we need to create a mouseout event, so we force the position
    // of the event to be at (MINSHORT, MINSHORT).
    if (message == WM_MOUSELEAVE) {
        x = MINSHORT;
        y = MINSHORT;
    }
    
    BOOL handled = TRUE;
    WPARAM wParam = 0;
    LPARAM lParam = MAKELPARAM(x, y);
    if (flags & WKE_CONTROL)
        wParam |= MK_CONTROL;
    if (flags & WKE_SHIFT)
        wParam |= MK_SHIFT;

    if (flags & WKE_LBUTTON)
        wParam |= MK_LBUTTON;
    if (flags & WKE_MBUTTON)
        wParam |= MK_MBUTTON;
    if (flags & WKE_RBUTTON)
        wParam |= MK_RBUTTON;
    m_webPage->fireMouseEvent(m_webPage->getHWND(), message, wParam, lParam, &handled);
    return handled;
}

bool CWebView::fireContextMenuEvent(int x, int y, unsigned int flags)
{
//     page()->contextMenuController()->clearContextMenu();
// 
//     if (x == -1 && y == -1)
//     {   
//         blink::Frame* focusedFrame = page()->focusController()->focusedOrMainFrame();
//         return focusedFrame->eventHandler()->sendContextMenuEventForKey();
//     }
// 
//     blink::IntPoint pos(x, y);
//     blink::IntPoint globalPos(x, y);
// 
//     blink::MouseButton button = messageToButtonType(WM_RBUTTONUP, flags);
//     blink::MouseEventType eventType = messageToEventType(WM_RBUTTONUP);
// 
//     bool shift = flags & WKE_SHIFT;
//     bool ctrl = flags & WKE_CONTROL;
//     bool alt = GetKeyState(VK_MENU) & 0x8000;
//     bool meta = alt;
//     double timestamp = ::GetTickCount()*0.001;
// 
//     int clickCount = 0;
//     blink::PlatformMouseEvent mouseEvent(pos, globalPos, button, eventType, clickCount, shift, ctrl, alt, meta, timestamp);
// 
//     blink::IntPoint documentPoint(mainFrame()->view()->windowToContents(pos));
//     blink::HitTestResult result = mainFrame()->eventHandler()->hitTestResultAtPoint(documentPoint, false);
//     blink::Frame* targetFrame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document()->frame() : page()->focusController()->focusedOrMainFrame();
// 
//     targetFrame->view()->setCursor(blink::pointerCursor());
//     return targetFrame->eventHandler()->sendContextMenuEvent(mouseEvent);
    return true;
}

bool CWebView::fireMouseWheelEvent(int x, int y, int wheelDelta, unsigned int flags)
{
    BOOL handled = TRUE;
    WPARAM wParam = 0;

    POINT screenPoint = { x, y };
    ::ClientToScreen(m_webPage->getHWND(), &screenPoint);
    LPARAM lParam = MAKELPARAM(screenPoint.x, screenPoint.y);
    if (flags & WKE_CONTROL)
        wParam |= MK_CONTROL;
    if (flags & WKE_SHIFT)
        wParam |= MK_SHIFT;

    if (flags & WKE_LBUTTON)
        wParam |= MK_LBUTTON;
    if (flags & WKE_MBUTTON)
        wParam |= MK_MBUTTON;
    if (flags & WKE_RBUTTON)
        wParam |= MK_RBUTTON;

    wParam = MAKEWPARAM(wParam, wheelDelta);

    m_webPage->fireWheelEvent(m_webPage->getHWND(), WM_MOUSEWHEEL, wParam, lParam);
    return handled;
}

bool CWebView::fireMouseWheelEventOnUiThread(int x, int y, int wheelDelta, unsigned int flags)
{
    BOOL handled = TRUE;
    WPARAM wParam = 0;

    POINT screenPoint = { x, y };
    ::ClientToScreen(m_webPage->getHWND(), &screenPoint);
    LPARAM lParam = MAKELPARAM(screenPoint.x, screenPoint.y);
    if (flags & WKE_CONTROL)
        wParam |= MK_CONTROL;
    if (flags & WKE_SHIFT)
        wParam |= MK_SHIFT;

    if (flags & WKE_LBUTTON)
        wParam |= MK_LBUTTON;
    if (flags & WKE_MBUTTON)
        wParam |= MK_MBUTTON;
    if (flags & WKE_RBUTTON)
        wParam |= MK_RBUTTON;

    wParam = MAKEWPARAM(wParam, wheelDelta);

    return m_webPage->fireWheelEventOnUiThread(m_webPage->getHWND(), WM_MOUSEWHEEL, wParam, lParam) == 0 ? false : true;
}

bool CWebView::fireKeyUpEvent(unsigned int virtualKeyCode, unsigned int flags, bool systemKey)
{
    WPARAM wParam = virtualKeyCode;
    LPARAM lParam = 0;

    if (flags & WKE_REPEAT)
        lParam |= ((KF_REPEAT) >> 16);
    if (flags & WKE_EXTENDED)
        lParam |= ((KF_EXTENDED) >> 16);

    m_webPage->fireKeyUpEvent(m_webPage->getHWND(), WM_KEYUP, wParam, lParam);
    return true;
}

bool CWebView::fireKeyDownEvent(unsigned int virtualKeyCode, unsigned int flags, bool systemKey)
{
    WPARAM wParam = virtualKeyCode;
    LPARAM lParam = flags;

//     if (flags & WKE_REPEAT)
//         lParam |= ((KF_REPEAT) >> 16);
//     if (flags & WKE_EXTENDED)
//         lParam |= ((KF_EXTENDED) >> 16);

    m_webPage->fireKeyDownEvent(m_webPage->getHWND(), WM_KEYDOWN, wParam, lParam);
    return true;
}

bool CWebView::fireKeyPressEvent(unsigned int charCode, unsigned int flags, bool systemKey)
{
    WPARAM wParam = charCode;
    LPARAM lParam = flags;

//     if (flags & WKE_REPEAT)
//         lParam |= ((KF_REPEAT) >> 16);
//     if (flags & WKE_EXTENDED)
//         lParam |= ((KF_EXTENDED) >> 16);

    if (systemKey) // 这个systemKey以前没有，现在为了支持flash中文，改用做区分WM_IME_CHAR消息
        m_webPage->fireKeyPressEvent(m_webPage->getHWND(), WM_IME_CHAR, wParam, lParam);
    else
        m_webPage->fireKeyPressEvent(m_webPage->getHWND(), WM_CHAR, wParam, lParam);
    return true;
}

bool CWebView::fireWindowsMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    BOOL handle = FALSE;
    LRESULT webResult = 0;

    if (WM_SETCURSOR == message) {
        webResult = m_webPage->fireCursorEvent(hWnd, message, wParam, lParam, &handle);
    } else if (WM_TOUCH == message) {
        m_webPage->fireTouchEvent(hWnd, message, wParam, lParam);
        *result = 0;
        handle = TRUE;
    }

    if (result)
        *result = webResult;
    return handle;
}

void CWebView::setFocus()
{
    m_webPage->fireSetFocusEvent(m_webPage->getHWND(), WM_SETFOCUS, 0, 0);
}

void CWebView::killFocus()
{
    m_webPage->fireKillFocusEvent(m_webPage->getHWND(), WM_KILLFOCUS, 0, 0);
}

wkeRect CWebView::getCaret()
{
    return caretRect();
}

wkeRect CWebView::caretRect()
{
    wkeRect rect;
    rect.x = rect.y = 0;
    rect.w = rect.h = 0;
    if (!m_webPage)
        return rect;

    IntRect caret = m_webPage->caretRect();

    rect.x = caret.x();
    rect.y = caret.y();
    rect.w = caret.width();
    rect.h = caret.height();

    return rect;
}

int64_t CWebView::wkeWebFrameHandleToFrameId(content::WebPage* page, wkeWebFrameHandle frameId)
{
    return (int64_t)frameId + page->getFirstFrameId() - 1;
}

wkeWebFrameHandle CWebView::frameIdTowkeWebFrameHandle(content::WebPage* page, int64_t frameId)
{
    return (wkeWebFrameHandle)(frameId - page->getFirstFrameId() + 1);
}

static jsValue runJsImpl(blink::WebFrame* mainFrame, String* codeString, bool isInClosure)
{
    //blink::UserGestureIndicator gestureIndicator(blink::DefinitelyProcessingUserGesture);
    RefPtr<blink::UserGestureToken> userGestureToken = blink::UserGestureIndicator::currentToken();

    if (codeString->startsWith("javascript:", WTF::TextCaseASCIIInsensitive))
        codeString->remove(0, sizeof("javascript:") - 1);
    if (isInClosure) {
        codeString->insert("(function(){", 0);
        codeString->append("})();");
    }
    blink::WebScriptSource code(*codeString, KURL(ParsedURLString, "CWebView::runJS"));
    blink::Frame* coreFrame = blink::toCoreFrame(mainFrame);
    if (!mainFrame || !coreFrame || !coreFrame->isLocalFrame())
        return jsUndefined();

    blink::LocalFrame* localFrame = blink::toLocalFrame(coreFrame);
    v8::HandleScope handleScope(blink::toIsolate(localFrame));
    v8::Local<v8::Context> context = mainFrame->mainWorldScriptContext();
    v8::Context::Scope contextScope(context);

    wke::AutoAllowRecordJsExceptionInfo autoAllowRecordJsExceptionInfo;
    v8::Local<v8::Value> result = mainFrame->executeScriptAndReturnValue(code);
    return v8ValueToJsValue(context, result);
}

jsValue CWebView::runJS(const WCHAR* script)
{
#if defined(OS_WIN)
    if (!script)
        return jsUndefined();

    String codeString(script);
    return runJsImpl(m_webPage->mainFrame(), &codeString, true);
#else
    DebugBreak();
    return 0;
#endif
}

jsValue CWebView::runJS(const utf8* script)
{
    if (!script)
        return jsUndefined();

    String codeString = String::fromUTF8(script);
    return runJsImpl(m_webPage->mainFrame(), &codeString, true);
}

jsValue CWebView::runJsInFrame(wkeWebFrameHandle frameId, const utf8* script, bool isInClosure)
{
    if (!m_webPage)
        return jsUndefined();
    blink::WebFrame* webFrame = m_webPage->getWebFrameFromFrameId(wkeWebFrameHandleToFrameId(m_webPage, frameId));
    if (!webFrame)
        return jsUndefined();
   
    String codeString = String::fromUTF8(script);
    return runJsImpl(webFrame, &codeString, isInClosure);
}

jsExecState CWebView::globalExec()
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    return wke::createTempExecStateByV8Context(m_webPage->mainFrame()->mainWorldScriptContext());
}

jsExecState CWebView::globalExecByFrame(wkeWebFrameHandle frameId)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    blink::WebFrame* webFrame = m_webPage->getWebFrameFromFrameId(wkeWebFrameHandleToFrameId(m_webPage, frameId));
    if (!webFrame)
        return nullptr;
    return wke::createTempExecStateByV8Context(webFrame->mainWorldScriptContext());
}

void CWebView::sleep()
{
    m_awake = false;
//     page()->setCanStartMedia(false);
//     page()->willMoveOffscreen();
}

void CWebView::wake()
{
    m_awake = true;
//     page()->didMoveOnscreen();
//     page()->setCanStartMedia(true);
}

bool CWebView::isAwake() const
{
    return m_awake;
}

void CWebView::setZoomFactor(float factor)
{
    //m_mainFrame->setPageZoomFactor(factor);
    if (!m_webPage)
        return;

    WebViewImpl* view = m_webPage->webViewImpl();
    if (view) {
        view->setZoomFactorOverride(factor);
        m_zoomFactor = factor;
    }
}

float CWebView::zoomFactor() const
{
    //return m_mainFrame->pageZoomFactor();
    return m_zoomFactor;
}

void CWebView::setEditable(bool editable)
{
//     if (page()->isEditable() != editable) {
//         page()->setEditable(editable);
//         page()->setTabKeyCyclesThroughElements(!editable);
//         if (editable)
//             m_mainFrame->editor()->applyEditingStyleToBodyElement();
//     }
}

void CWebView::onTitleChanged(wkeTitleChangedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().titleChangedCallback = callback;
    m_webPage->wkeHandler().titleChangedCallbackParam = callbackParam;
}

void CWebView::onMouseOverUrlChanged(wkeTitleChangedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().mouseOverUrlChangedCallback = callback;
    m_webPage->wkeHandler().mouseOverUrlChangedCallbackParam = callbackParam;
}

void CWebView::onURLChanged(wkeURLChangedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().urlChangedCallback = callback;
    m_webPage->wkeHandler().urlChangedCallbackParam = callbackParam;
}

void CWebView::onURLChanged2(wkeURLChangedCallback2 callback, void* callbackParam)
{
    m_webPage->wkeHandler().urlChangedCallback2 = callback;
    m_webPage->wkeHandler().urlChangedCallback2Param = callbackParam;
}

void CWebView::onPaintUpdated(wkePaintUpdatedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().paintUpdatedCallback = callback;
    m_webPage->wkeHandler().paintUpdatedCallbackParam = callbackParam;
}

void CWebView::onPaintBitUpdated(wkePaintBitUpdatedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().paintBitUpdatedCallback = callback;
    m_webPage->wkeHandler().paintBitUpdatedCallbackParam = callbackParam;
}

void CWebView::onCaretChanged(wkeCaretChangedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().caretChangedCallback = callback;
    m_webPage->wkeHandler().caretChangedCallbackParam = callbackParam;
}

void CWebView::onAlertBox(wkeAlertBoxCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().alertBoxCallback = callback;
    m_webPage->wkeHandler().alertBoxCallbackParam = callbackParam;
}

void CWebView::onConfirmBox(wkeConfirmBoxCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().confirmBoxCallback = callback;
    m_webPage->wkeHandler().confirmBoxCallbackParam = callbackParam;
}

void CWebView::onPromptBox(wkePromptBoxCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().promptBoxCallback = callback;
    m_webPage->wkeHandler().promptBoxCallbackParam = callbackParam;
}

static LRESULT CALLBACK hideWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

static HWND createHideWnd()
{
    static HWND s_hWnd = nullptr;
    if (s_hWnd) {
        return s_hWnd;
    }
    const LPCWSTR kWindowClassName = u16("MiniBlinkAlertWindowClass");

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = hideWindowWndProc;
    wcex.hInstance = ::GetModuleHandleW(NULL);
    wcex.lpszClassName = kWindowClassName;
    ::RegisterClassExW(&wcex);

    s_hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,        // window ex-style
        kWindowClassName,    // window class name
        u16("HideTopMostWnd"), // window caption
        WS_POPUP & (~WS_CAPTION) & (~WS_SYSMENU) & (~WS_SIZEBOX), // window style
        1,              // initial x position
        1,              // initial y position
        1,          // initial x size
        1,         // initial y size
        NULL,         // parent window handle
        NULL,           // window menu handle
        GetModuleHandleW(NULL),           // program instance handle
        NULL);         // creation parameters

    return s_hWnd;
}

void WKE_CALL_TYPE defaultRunAlertBox(wkeWebView webView, void* param, const wkeString msg)
{
#if defined(OS_WIN)
    HWND hWnd = createHideWnd();
    ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ::SetForegroundWindow(hWnd);
    ::ShowWindow(hWnd, SW_SHOW);

    const int maxShowLength = 500;
    Vector<WCHAR> msgBuf;
    const WCHAR* msgString = wkeGetStringW(msg);
    if (wcslen(msgString) > maxShowLength) {
        msgBuf.resize(maxShowLength);
        memcpy(msgBuf.data(), msgString, maxShowLength * sizeof(WCHAR));
        msgBuf[maxShowLength - 1] = L'\0';
        msgBuf[maxShowLength - 2] = L'.';
        msgBuf[maxShowLength - 3] = L'.';
        msgBuf[maxShowLength - 4] = L'.';
        msgBuf[maxShowLength - 5] = L'.';
        msgBuf[maxShowLength - 6] = L'.';
        msgBuf[maxShowLength - 7] = L'.';
        msgString = msgBuf.data();
    }
    MessageBoxW(hWnd, msgString, u16("Miniblink"), MB_OK);

    ::ShowWindow(hWnd, SW_HIDE);

    hWnd = wkeGetHostHWND(webView);
    ::SetFocus(hWnd);
#else
    DebugBreak();
#endif
}

bool WKE_CALL_TYPE defaultRunConfirmBox(wkeWebView webView, void* param, const wkeString msg)
{
#if defined(OS_WIN)
    int result = MessageBoxW(NULL, wkeGetStringW(msg), u16("wke"), MB_OKCANCEL);
    return result == IDOK;
#else
    DebugBreak();
    return false;
#endif
}

bool WKE_CALL_TYPE defaultRunPromptBox(wkeWebView webView, void* param, const wkeString msg, const wkeString defaultResult, wkeString result)
{
    return false;
}

void CWebView::_initHandler()
{
    m_webPage->wkeHandler().alertBoxCallback = defaultRunAlertBox;
    m_webPage->wkeHandler().confirmBoxCallback = defaultRunConfirmBox;
    m_webPage->wkeHandler().promptBoxCallback = defaultRunPromptBox;
}

void CWebView::_initPage(COLORREF color)
{
    m_webPage = new content::WebPage(nullptr);
    m_webPage->initWkeWebView(this);
    m_webPage->init(nullptr, color);

//     blink::Page::PageClients pageClients;
//     pageClients.chromeClient = new ChromeClient(this);
//     pageClients.contextMenuClient = new ContextMenuClient;
//     pageClients.inspectorClient = new InspectorClient;
//     pageClients.editorClient = new EditorClient;
//     pageClients.dragClient = new DragClient;
// 
//     m_page = adoptPtr(new blink::Page(pageClients));
//     blink::Settings* settings = m_webPage->settings();
//     settings->setMinimumFontSize(0);
//     settings->setMinimumLogicalFontSize(9);
//     settings->setDefaultFontSize(16);
//     settings->setDefaultFixedFontSize(13);
//     settings->setJavaScriptEnabled(true);
//     settings->setPluginsEnabled(true);
//     settings->setLoadsImagesAutomatically(true);
//     settings->setDefaultTextEncodingName(icuwin_getDefaultEncoding());
//     
//     settings->setStandardFontFamily("Times New Roman");
//     settings->setFixedFontFamily("Courier New");
//     settings->setSerifFontFamily("Times New Roman");
//     settings->setSansSerifFontFamily("Arial");
//     settings->setCursiveFontFamily("Comic Sans MS");
//     settings->setFantasyFontFamily("Times New Roman");
//     settings->setPictographFontFamily("Times New Roman");
// 
//     settings->setAllowUniversalAccessFromFileURLs(true);
//     settings->setAllowFileAccessFromFileURLs(true);
//     settings->setJavaScriptCanAccessClipboard(true);
//     settings->setShouldPrintBackgrounds(true);
//     settings->setTextAreasAreResizable(true);
//     settings->setLocalStorageEnabled(true);
// 
//     UChar dir[256];
//     GetCurrentDirectory(256, dir);
//     wcscat(dir, u16("\\localStorage"));
//     settings->setLocalStorageDatabasePath(dir);
//     blink::DatabaseTracker::tracker().setDatabaseDirectoryPath(dir);
// 
//     FrameLoaderClient* loader = new FrameLoaderClient(this, m_page.get());
//     m_mainFrame = blink::Frame::create(m_page.get(), NULL, loader).get();
//     loader->setFrame(m_mainFrame);
//     m_mainFrame->init();
// 
//     page()->focusController()->setActive(true);
}

void CWebView::_initMemoryDC()
{
    //m_hdc = adoptPtr(::CreateCompatibleDC(0));
}

void CWebView::onNavigation(wkeNavigationCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().navigationCallback = callback;
    m_webPage->wkeHandler().navigationCallbackParam = callbackParam;
}

void CWebView::onCreateView(wkeCreateViewCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().createViewCallback = callback;
    m_webPage->wkeHandler().createViewCallbackParam = callbackParam;
}

void CWebView::onLoadingFinish(wkeLoadingFinishCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().loadingFinishCallback = callback;
    m_webPage->wkeHandler().loadingFinishCallbackParam = callbackParam;
}

void CWebView::onDownload(wkeDownloadCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().downloadCallback = callback;
    m_webPage->wkeHandler().downloadCallbackParam = callbackParam;
}

void CWebView::onDownload2(wkeDownload2Callback callback, void* callbackParam)
{
    m_webPage->wkeHandler().download2Callback = callback;
    m_webPage->wkeHandler().download2CallbackParam = callbackParam;
}

void CWebView::onNetResponse(wkeNetResponseCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().netResponseCallback = callback;
    m_webPage->wkeHandler().netResponseCallbackParam = callbackParam;
}

void CWebView::onConsole(wkeConsoleCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().consoleCallback = callback;
    m_webPage->wkeHandler().consoleCallbackParam = callbackParam;
}

void CWebView::onCallUiThread(wkeCallUiThread callback, void* callbackParam)
{
    m_webPage->wkeHandler().callUiThreadCallback = callback;
    m_webPage->wkeHandler().callUiThreadCallbackParam = callbackParam;
}

void CWebView::onDocumentReady(wkeDocumentReadyCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().documentReadyCallback = callback;
    m_webPage->wkeHandler().documentReadyCallbackParam = callbackParam;
}

void CWebView::onDocumentReady2(wkeDocumentReady2Callback callback, void* callbackParam)
{
    m_webPage->wkeHandler().documentReady2Callback = callback;
    m_webPage->wkeHandler().documentReady2CallbackParam = callbackParam;
}

void CWebView::onLoadUrlBegin(wkeLoadUrlBeginCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().loadUrlBeginCallback = callback;
    m_webPage->wkeHandler().loadUrlBeginCallbackParam = callbackParam;
}

void CWebView::onLoadUrlEnd(wkeLoadUrlEndCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().loadUrlEndCallback = callback;
    m_webPage->wkeHandler().loadUrlEndCallbackParam = callbackParam;
}

void CWebView::onLoadUrlFail(wkeLoadUrlFailCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().loadUrlFailCallback = callback;
    m_webPage->wkeHandler().loadUrlFailCallbackParam = callbackParam;
}

void CWebView::onDidCreateScriptContext(wkeDidCreateScriptContextCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().didCreateScriptContextCallback = callback;
    m_webPage->wkeHandler().didCreateScriptContextCallbackParam = callbackParam;
}

void CWebView::onWillReleaseScriptContext(wkeWillReleaseScriptContextCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().willReleaseScriptContextCallback = callback;
    m_webPage->wkeHandler().willReleaseScriptContextCallbackParam = callbackParam;
}

void CWebView::onOtherLoad(wkeOnOtherLoadCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().otherLoadCallback = callback;
    m_webPage->wkeHandler().otherLoadCallbackParam = callbackParam;
}

void CWebView::onContextMenuItemClick(wkeOnContextMenuItemClickCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().contextMenuItemClickCallback = callback;
    m_webPage->wkeHandler().contextMenuItemClickCallbackParam = callbackParam;
}

void CWebView::onDraggableRegionsChanged(wkeDraggableRegionsChangedCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().draggableRegionsChangedCallback = callback;
    m_webPage->wkeHandler().draggableRegionsChangedCallbackParam = callbackParam;
}

void CWebView::onStartDragging(wkeStartDraggingCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().startDraggingCallback = callback;
    m_webPage->wkeHandler().startDraggingCallbackParam = callbackParam;
}

void CWebView::onPrint(wkeOnPrintCallback callback, void* callbackParam)
{
    m_webPage->wkeHandler().printCallback = callback;
    m_webPage->wkeHandler().printCallbackParam = callbackParam;
}

void CWebView::setClientHandler(const wkeClientHandler* handler)
{
    m_webPage->wkeSetClientHandler((void*)handler);
}

const wkeClientHandler* CWebView::getClientHandler() const
{
    return (const wkeClientHandler *)m_webPage->wkeClientHandler();
}

CWebViewHandler* CWebView::getWkeHandler() const
{
    if (!m_webPage)
        return nullptr;

    return &m_webPage->wkeHandler();
}

void CWebView::setUserKeyValue(const char* key, void* value)
{
    if (!key)
        return;
    m_userKeyValues[key] = value;
}

void* CWebView::getUserKeyValue(const char* key)
{
    if (!key)
        return nullptr;
    std::map<std::string, void*>::const_iterator it = m_userKeyValues.find(key);
    if (m_userKeyValues.end() == it)
        return nullptr;
    return it->second;
}

int CWebView::getCursorInfoType()
{
    return m_webPage->getCursorInfoType();
}

void CWebView::setCursorInfoType(int type)
{
    m_webPage->setCursorInfoType(type);
}

void CWebView::setDragFiles(const POINT* clintPos, const POINT* screenPos, wkeString files[], int filesCount)
{
    DebugBreak();
//     blink::WebPoint clientPoint(clintPos->x, clintPos->y);
//     blink::WebPoint screenPoint(screenPos->x, screenPos->y);
// 
//     blink::WebDragData webDragData;
//     webDragData.initialize();
// 
//     for (int i = 0; i < filesCount; ++i) {
//         WTF::String file = WTF::String::fromUTF8(files[i]->string());
//         file.insert("file:///", 0);
//     
//         blink::WebDragData::Item it;
//         it.storageType = blink::WebDragData::Item::StorageTypeFileSystemFile;
//         it.fileSystemURL = blink::KURL(blink::ParsedURLString, file);
//         webDragData.addItem(it);
//     }
// 
//     blink::WebViewImpl* webView = m_webPage->webViewImpl();
//     webView->dragTargetDragEnter(webDragData, clientPoint, screenPoint, blink::WebDragOperationEvery, 0);
//     webView->dragTargetDragOver(clientPoint, screenPoint, blink::WebDragOperationEvery, 0);
//     webView->dragTargetDrop(clientPoint, screenPoint, 0);
}

void CWebView::setNetInterface(const char* netInterface)
{
    m_netInterface = netInterface;
}

void CWebView::setProxyInfo(const String& host, unsigned long port, net::ProxyType type, const String& username, const String& password)
{
    m_proxyType = type;

    if (!host.length()) {
        m_proxy = emptyString();
    } else {
        String userPass;
        if (username.length() || password.length())
            userPass = username + ":" + password + "@";

        m_proxy = String("http://") + userPass + host + ":" + String::number(port);
    }
}

class ShowDevToolsTaskObserver : public blink::WebThread::TaskObserver {
public:
    ShowDevToolsTaskObserver(CWebView* parent, const std::string& url, wkeOnShowDevtoolsCallback callback, void* param)
    {
        m_parent = parent;
        m_url = url;
        m_callback = callback;
        m_param = param;

        m_memoryBMP = nullptr;
        m_memoryDC = nullptr;
        ::InitializeCriticalSection(&m_memoryCanvasLock);
    }
    virtual ~ShowDevToolsTaskObserver()
    {
        ::DeleteCriticalSection(&m_memoryCanvasLock);
    }

    struct WrapInfo {
        ShowDevToolsTaskObserver* self;
        int id;
        HWND hWnd;
        CWebView* parent;
    };

    static void WKE_CALL_TYPE handleDevToolsWebViewDestroy(wkeWebView webWindow, void* param)
    {
        WrapInfo* wrapInfo = (WrapInfo*)param;
        wrapInfo->parent->m_isCreatedDevTools = false;
        delete wrapInfo;
    }

    static void WKE_CALL_TYPE onPaintUpdated(wkeWebView wkeWebview, void* param, const HDC hdc, int x, int y, int cx, int cy)
    {
        WrapInfo* wrap = (WrapInfo*)param;
        bool b = net::ActivatingObjCheck::inst()->isActivatingLocked(wrap->id);
        if (!b) {
            net::ActivatingObjCheck::inst()->unlock();
            return;
        }

        wrap->self->onPrePaintUpdatedInCompositeThread(wrap->id, wrap->hWnd, hdc, x, y, cx, cy);
        net::ActivatingObjCheck::inst()->unlock();
    }

    void onPrePaintUpdatedInCompositeThread(int id, HWND hWnd, const HDC hdc, int x, int y, int cx, int cy)
    {
        ::EnterCriticalSection(&m_memoryCanvasLock);
        onPaintUpdatedInCompositeThread(hWnd, hdc, x, y, cx, cy);
        ::LeaveCriticalSection(&m_memoryCanvasLock);

        ShowDevToolsTaskObserver* self = this;
        content::postTaskToMainThread(FROM_HERE, [self, id, hWnd, x, y, cx, cy] {
            bool b = net::ActivatingObjCheck::inst()->isActivating(id);
            if (!b)
                return;

            self->onPaintUpdatedInUiThread(hWnd, x, y, cx, cy);
        });        
    }

    void onPaintUpdatedInCompositeThread(HWND hWnd, const HDC hdc, int x, int y, int cx, int cy)
    {
        RECT rc = { 0 };
        ::GetWindowRect(hWnd, &rc);
        SIZE clientSize = { rc.right - rc.left, rc.bottom - rc.top };
        if (0 == clientSize.cx * clientSize.cy)
            return;

        if (!m_memoryDC)
            m_memoryDC = ::CreateCompatibleDC(nullptr);

        if ((!m_memoryBMP)
            && (clientSize.cx * clientSize.cy != 0)
            && (clientSize.cx == cx && clientSize.cy == cy && 0 == x && 0 == y)) {

            if (m_memoryBMP)
                ::DeleteObject((HGDIOBJ)m_memoryBMP);
            m_memoryBMP = ::CreateCompatibleBitmap(hdc, clientSize.cx, clientSize.cy);
            ::SelectObject(m_memoryDC, m_memoryBMP);
        }

        if (m_memoryDC)
            ::BitBlt(m_memoryDC, x, y, cx, cy, hdc, x, y, SRCCOPY);
    }

    void onPaintUpdatedInUiThread(HWND hWnd, int x, int y, int cx, int cy)
    {
        ::EnterCriticalSection(&m_memoryCanvasLock);
        if (hWnd) {
            HDC hdcScreen = ::GetDC(hWnd);
            ::BitBlt(hdcScreen, x, y, cx, cy, m_memoryDC, x, y, SRCCOPY);
            ::ReleaseDC(hWnd, hdcScreen);
        }
        ::LeaveCriticalSection(&m_memoryCanvasLock);

        RECT rc = { x, y, x + cx, y + cy };
        ::InvalidateRect(hWnd, &rc, false);
    }

    virtual void willProcessTask() override
    {
        
    }
    virtual void didProcessTask() override
    {
        OutputDebugStringA("Devtools willProcessTask\n");

        wkeWebView devToolsWebView = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, nullptr, 200, 200, 800, 600);
        m_parent->m_devToolsWebView = devToolsWebView;

        content::WebPage::connetDevTools(devToolsWebView->webPage(), m_parent->webPage());

        WrapInfo* wrapInfo = new WrapInfo();
        wrapInfo->hWnd = wkeGetHostHWND(devToolsWebView);
        wrapInfo->self = this;
        wrapInfo->parent = m_parent;

        wrapInfo->id = wkeGetWebviewId(devToolsWebView);

        wkeLoadURL(devToolsWebView, m_url.c_str());
        wkeShowWindow(devToolsWebView, TRUE);
        wkeOnWindowDestroy(devToolsWebView, handleDevToolsWebViewDestroy, (void*)wrapInfo);
        wkeOnPaintUpdated(devToolsWebView, onPaintUpdated, (void*)wrapInfo);
        wkeSetWindowTitle(devToolsWebView, "Miniblink Devtools");
        wkeSetZoomFactor(devToolsWebView, m_parent->zoomFactor());
        wkeSetDragDropEnable(devToolsWebView, false);
        blink::Platform::current()->currentThread()->removeTaskObserver(this);

        if (m_callback)
            m_callback(devToolsWebView, m_param);
    }

private:
    CWebView* m_parent;
    std::string m_url;
    wkeOnShowDevtoolsCallback m_callback;
    void* m_param;

    CRITICAL_SECTION m_memoryCanvasLock;
    HBITMAP m_memoryBMP;
    HDC m_memoryDC;
};

void CWebView::showDevTools(const utf8* url, wkeOnShowDevtoolsCallback callback, void* param)
{
    if (m_isCreatedDevTools)
        return;
    m_isCreatedDevTools = true;
    //blink::Platform::current()->currentThread()->addTaskObserver(new ShowDevToolsTaskObserver(this, url, callback, param));
    DebugBreak();
}

net::WebCookieJarImpl* CWebView::getCookieJar()
{
    if (!m_webPage)
        return nullptr;

    return m_webPage->getCookieJar();
}

CURLSH* CWebView::getCurlShareHandle()
{
    CURLSH* curlsh = nullptr;
    if (m_webPage && m_webPage->getPageNetExtraData()) {
        curlsh = m_webPage->getPageNetExtraData()->getCurlShareHandle();
        if (curlsh)
            return curlsh;
    }

    curlsh = net::WebURLLoaderManager::sharedInstance()->getCurlShareHandle();
    return curlsh;
}

std::string CWebView::getCookieJarPath()
{
    std::string cookiesData;
    if (m_webPage && m_webPage->getPageNetExtraData() && m_webPage->getPageNetExtraData()->getCookieJar()) {
        cookiesData = m_webPage->getPageNetExtraData()->getCookieJarFullPath();
        return cookiesData;
    }

    net::WebURLLoaderManager* manager = net::WebURLLoaderManager::sharedInstance();
    if (!manager)
        return "";

    cookiesData = manager->getShareCookieJar()->getCookieJarFullPath();
    return cookiesData;
}

void CWebView::setCookieJarFullPath(const utf8* path)
{
    if (m_webPage)
        m_webPage->setCookieJarFullPath(path);
}

void CWebView::setLocalStorageFullPath(const utf8* path)
{
    if (m_webPage)
        m_webPage->setLocalStorageFullPath(path);
}

} // namespace wke

#endif
