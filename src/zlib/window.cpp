#include "window.h"
#include "zassert.h"
#include "ztime.h"

#ifdef ICAP
    #include "../../extern/icecap4/include/icecap.h"
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Handy painting function
//
/////////////////////////////////////////////////////////////////////////////

void FillSolidRect(HDC hdc, const WinRect& rect, COLORREF color)
{
    HBRUSH hbrush = CreateSolidBrush(color);
    ::FillRect(hdc, &rect, hbrush);
    DeleteObject(hbrush);
}

//////////////////////////////////////////////////////////////////////////////
//
// Window
//
//////////////////////////////////////////////////////////////////////////////

void Window::Construct()
{
    m_pfnWndProc    = DefWindowProc;
    m_bHit          = false;
    m_bShowMouse    = true;
    m_bMouseInside  = false;
    m_sizeMin       = WinPoint(0, 0);
    m_hcursor       = nullptr;
}

Window::Window():
    m_pwindowParent(nullptr),
    m_hmenu(nullptr),
    m_hwnd(nullptr),
    m_bSizeable(false),
    m_bHasMinimize(false),
    m_bHasMaximize(false),
    m_bHasSysMenu(false),
    m_bTopMost(false),
    m_nID(0)
{
    Construct();
}

Window::Window(
          Window*  pwindowParent,
    const WinRect& rect,
    const ZString& strTitle,
    const ZString& strClass,
          Style    style,
          HMENU    hmenu,
          StyleEX  styleEX
) :
    m_pwindowParent(pwindowParent),
    m_style(style),
    m_styleEX(styleEX),
    m_hmenu(hmenu),
    m_bSizeable(true),
    m_bHasSysMenu(false),
    m_bTopMost(false),
    m_bHasMinimize(true),
    m_bHasMaximize(true),
    m_lastPointMouse(0,0)
{
    Construct();

    if (m_pwindowParent) {
        m_style.Set(StyleChild());
    } else {
        m_style.Set(
            StylePopup() |
            StyleCaption() |
            StyleMaximizeBox() |
            StyleMinimizeBox() |
            StyleSysMenu() |
            StyleThickFrame()
        );
        
        // set a default arrow cursor for topmost windows
        m_hcursor = LoadCursor(nullptr, IDC_ARROW);
    }

    m_style.Set(StyleVisible() | StyleClipChildren() | StyleClipSiblings());

    m_rect = rect;
    AdjustWindowRect(&m_rect, m_style.GetWord(), m_hmenu != nullptr);

    if (strClass.IsEmpty()) {
        m_hwnd = ::CreateWindowEx(
            m_styleEX.GetWord(),
            TEXT(GetTopLevelWindowClassname()),
            strTitle,
            m_style.GetWord(),
			//Imago restored original impl in multimon & topmost effort 7/10
// BUILD_DX9
//            rect.XMin(), rect.YMin(),
//#else
            CW_USEDEFAULT, CW_USEDEFAULT,
// BUILD_DX9

            //m_rect.XMin(), m_rect.YMin(), 
            m_rect.XSize(), m_rect.YSize(),
            pwindowParent ? pwindowParent->GetHWND() : nullptr,
            m_hmenu,
            GetModuleHandle(nullptr),
            this
        );
    } else {
        m_hwnd = ::CreateWindowEx(
            m_styleEX.GetWord(),
            strClass,
            strTitle,
            m_style.GetWord(),

			//Imago restored original impl in multimon & topmost effort 7/10
// BUILD_DX9
 //           rect.XMin(), rect.YMin(),
//#else
            CW_USEDEFAULT, CW_USEDEFAULT,
// BUILD_DX9

            //m_rect.XMin(), m_rect.YMin(), 
            m_rect.XSize(), m_rect.YSize(),
            pwindowParent ? pwindowParent->GetHWND() : nullptr,
            m_hmenu,
            GetModuleHandle(nullptr),
            this
        );

        s_mapWindow.Set(m_hwnd, this);

        m_pfnWndProc = (WNDPROC)::GetWindowLong(m_hwnd, GWLx_WNDPROC); //x64 Imago 6/20/09
        ::SetWindowLong(m_hwnd, GWLx_WNDPROC, (uint32_t)Win32WndProc);  //x64 Imago 6/20/09
    }

    m_styleEX.SetWord(::GetWindowLong(m_hwnd, GWL_EXSTYLE));

    if (m_pwindowParent) {
        m_pwindowParent->AddChild(this);
    }

}

BOOL Window::Create(
          Window*  pwindowParent,
    const WinRect& rect,
          LPCSTR   szTitle,
          LPCSTR   szClass,
          Style    style,
          HMENU    hmenu,
          UINT     nID,
          StyleEX  styleEX
) {
    Construct();

    m_style.Set(style);
    m_styleEX.Set(styleEX);
    m_style.Set(StyleVisible() | StyleClipChildren() | StyleClipSiblings());
    m_rect = rect;
    m_pwindowParent = pwindowParent;
    m_hmenu = hmenu;
    m_nID = nID;

    if (!m_pwindowParent) {
        // set a default arrow cursor for topmost windows
        m_hcursor = LoadCursor(nullptr, IDC_ARROW);
    }
    
    m_hwnd = ::CreateWindowEx(
            styleEX.GetWord(),
            szClass ? szClass : "Window",
            szTitle,
            m_style.GetWord(),
            m_rect.left, m_rect.top,
            m_rect.XSize(), m_rect.YSize(),
            pwindowParent ? pwindowParent->GetHWND() : nullptr,
            hmenu ? hmenu : (HMENU) nID,
            GetModuleHandle(nullptr),
            this);
    
    s_mapWindow.Set(m_hwnd, this);
    m_pfnWndProc = (WNDPROC)::GetWindowLong(m_hwnd, GWLx_WNDPROC); //x64 Imago 6/20/09

    if ((WNDPROC)m_pfnWndProc != (WNDPROC)Win32WndProc) {
        ::SetWindowLong(m_hwnd, GWLx_WNDPROC, (uint32_t)Win32WndProc); //x64 Imago 6/20/09
    } else {
        m_pfnWndProc = DefWindowProc;
    }

    m_styleEX.SetWord(::GetWindowLong(m_hwnd, GWL_EXSTYLE));
    ::GetClientRect(GetHWND(), &m_rectClient);

    return TRUE;
}

Window::~Window()
{
    DestroyWindow(m_hwnd);
    s_mapWindow.Remove(m_hwnd);
    EnableIdleFunction(false);
}

//////////////////////////////////////////////////////////////////////////////
//
// Sizing
//
//////////////////////////////////////////////////////////////////////////////

void Window::CalcStyle()
{
    if (m_bTopMost) {
        m_styleEX.Set(StyleEXTopMost());
    } else {
        m_styleEX.Clear(StyleEXTopMost());
    }

    if (m_bSizeable) {
        m_style.Clear(StyleBorder());
        m_style.Set(StyleThickFrame());
    } else {
        m_style.Clear(StyleThickFrame());
        m_style.Set(StyleBorder());
    }

    if (m_bHasMaximize) {
        m_style.Set(StyleMaximizeBox());
    } else {
        m_style.Clear(StyleMaximizeBox());
    }

    if (m_bHasMinimize) {
        m_style.Set(StyleMinimizeBox());
    } else {
        m_style.Clear(StyleMinimizeBox());
    }

    if (m_bHasMinimize || m_bHasMaximize || m_bSizeable) {
        m_style.Set(StyleCaption());
    } else {
        m_style.Clear(StyleCaption());
    }

    if (m_bHasSysMenu) {
        m_style.Set(StyleSysMenu());
    } else {
        m_style.Clear(StyleSysMenu());
    }

    ::SetWindowLong(m_hwnd, GWL_STYLE,   m_style.GetWord());
    ::SetWindowLong(m_hwnd, GWL_EXSTYLE, m_styleEX.GetWord());
}

void Window::SetTopMost(bool bTopMost)
{
    ZAssert(m_pwindowParent == nullptr);

    //if (m_bTopMost != bTopMost) {
        m_bTopMost = bTopMost;
        CalcStyle();
    //}
}

void Window::SetSizeable(bool bSizeable)
{
    ZAssert(m_pwindowParent == nullptr);

    if (m_bSizeable != bSizeable) {
        m_bSizeable = bSizeable;
        CalcStyle();
    }
}

void Window::SetHasMinimize(bool bHasMinimize)
{
    if (m_bHasMinimize != bHasMinimize) {
        m_bHasMinimize = bHasMinimize;
        CalcStyle();
    }
}

void Window::SetHasMaximize(bool bHasMaximize)
{
    if (m_bHasMaximize != bHasMaximize) {
        m_bHasMaximize = bHasMaximize;
        CalcStyle();
    }
}

void Window::SetHasSysMenu(bool bSysMenu)
{
    if (m_bHasSysMenu != bSysMenu) {
        m_bHasSysMenu = bSysMenu;
        CalcStyle();
    }
}

void Window::UpdateRect()
{
	BOOL bRetVal;
    bRetVal = ::GetWindowRect(m_hwnd, &m_rect);
    ZAssert( bRetVal != FALSE );
	bRetVal = ::GetClientRect(m_hwnd, &m_rectClient);
    ZAssert( bRetVal != FALSE );

    WinPoint pointOffset = ClientToScreen(WinPoint(0, 0));

    if (m_pwindowParent) {
        pointOffset = m_pwindowParent->ScreenToClient(pointOffset);
    }

    m_rectClient.Offset(pointOffset);

    if (m_pwindowParent) {
        m_pwindowParent->ChildRectChanged(this);
    }
}

void Window::ChildRectChanged(Window* pchild)
{
}

void Window::RectChanged()
{
}

void Window::SetRect(const WinRect& rect)
{
    debugf("Window::SetRect rect=%s", (const char*)rect.GetString());
    MoveWindow(m_hwnd, rect.XMin(), rect.YMin(), rect.XSize(), rect.YSize(), true);
}

void Window::SetClientRect(const WinRect& rectClient)
{
    debugf("Window::SetClientRect rect=%s", (const char*) rectClient.GetString());
    WinRect rect = rectClient;
    AdjustWindowRect(&rect, m_style.GetWord(), m_hmenu != nullptr);
    SetRect(rect);
}

void Window::SetPosition(const WinPoint& point)
{
    debugf("Window::SetPosition point=%s", (const char*)point.GetString());
    SetWindowPos(m_hwnd, nullptr, point.X(), point.Y(), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void Window::SetSize(const WinPoint& point)
{
    debugf("Window::SetSize point=%s", (const char*)point.GetString());
    SetWindowPos(m_hwnd, nullptr, 0, 0, point.X(), point.Y(), SWP_NOMOVE | SWP_NOZORDER);
}

void Window::SetClientSize(const WinPoint& point)
{
    debugf("Window::SetClientSize point=%s", (const char*)point.GetString());
    WinRect rect = GetClientRect();
    rect.SetSize(point);
    SetClientRect(rect);
}

void Window::SetMinimumClientSize(const WinPoint& point)
{
    WinRect rect(WinPoint(0, 0), point);
    AdjustWindowRect(&rect, m_style.GetWord(), m_hmenu != nullptr);

    m_sizeMin = rect.Size();
}

//////////////////////////////////////////////////////////////////////////////
//
// Children
//
//////////////////////////////////////////////////////////////////////////////

void Window::AddChild(Window* pchild)
{
    ZAssert(pchild != nullptr);
    m_listChildren.PushFront(pchild);
}

void Window::RemoveChild(Window* pchild)
{
    ZAssert(m_listChildren.Find(pchild));
    m_listChildren.Remove(pchild);
}

void Window::RemoveAllChildren()
{
    m_listChildren.SetEmpty();
}

//////////////////////////////////////////////////////////////////////////////
//
// Input
//
//////////////////////////////////////////////////////////////////////////////

TList<TRef<IKeyboardInput> > g_listKeyboardInputFilters;

void Window::AddKeyboardInputFilter(IKeyboardInput* pkeyboardInput)
{
	g_listKeyboardInputFilters.PushEnd(pkeyboardInput);
}

void Window::RemoveKeyboardInputFilter(IKeyboardInput* pkeyboardInput)
{
    g_listKeyboardInputFilters.Remove(pkeyboardInput);
}

// BT - 10/17 - Fixing crash when allegiance is exited from the new game screen.
void Window::RemoveAllKeyboardInputFilters()
{
	g_listKeyboardInputFilters.SetEmpty();
}

//////////////////////////////////////////////////////////////////////////////
//
// Timers
//
//////////////////////////////////////////////////////////////////////////////

class Timer : public IObjectSingle {
public:
    Time               m_when;
    float              m_delta;
    TRef<IEventSink>   m_psink;

    Timer(IEventSink* psink, float delta, Time when) :
        m_psink(psink),
        m_delta(delta),
        m_when(when)
    {
    }

    bool Trigger(Time now)
    {
        return m_psink->OnEvent(nullptr);
    }
};

class TimerCompare {
public:
    bool operator () (const Timer* ptimer1, const Timer* ptimer2)
    {
        return ptimer1->m_when > ptimer2->m_when;
    }
};

class TimerEquals {
public:
    bool operator () (const Timer* ptimer1, const Timer* ptimer2)
    {
        return ptimer1->m_psink == ptimer2->m_psink;
    }
};

typedef TList<TRef<Timer>, TimerEquals, TimerCompare> TimerList;
TimerList g_listTimers;

void CallTimers()
{
    Time now = Time::Now();

    while (g_listTimers.GetCount() != 0) {
        TRef<Timer> ptimer = g_listTimers.GetFront();

        if (now < ptimer->m_when) {
            break;
        }

        g_listTimers.PopFront();

        if (ptimer->Trigger(now)) {
            g_listTimers.InsertSorted(
                new Timer(
                    ptimer->m_psink,
                    ptimer->m_delta,
                    Time::Now() + ptimer->m_delta
                )
            );
        }
    }
}

void Window::AddSink(IEventSink* psink)
{
    AddSink(psink, 1.0f);
}

void Window::AddSink(IEventSink* psink, float delta)
{
    g_listTimers.InsertSorted(new Timer(psink, delta, Time::Now() + delta));
}

void Window::RemoveSink(IEventSink* psink)
{
    g_listTimers.Remove(new Timer(psink, 0, 0));
}

ITimerEventSource* Window::GetTimer()
{
    return this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Keyboard
//
//////////////////////////////////////////////////////////////////////////////

bool Window::OnKey(const KeyState& ks)
{
    bool bTranslate = false;

    if (m_pkeyboardInputFocus) {
        if (m_pkeyboardInputFocus->OnKey(this, ks, bTranslate) && !bTranslate) {
            return true;
        }
    }

    return false;
}

bool Window::OnChar(const KeyState& ks)
{
    if (m_pkeyboardInputFocus) {
        if (m_pkeyboardInputFocus->OnChar(this, ks)) {
            return true;
        }
    }

    return false;
}

bool Window::IsFocus(IKeyboardInput* pinput)
{
    return m_pkeyboardInputFocus == pinput;
}

IKeyboardInput* Window::GetFocus()
{
    return m_pkeyboardInputFocus;
}

void Window::SetFocus(IKeyboardInput* pinput)
{
    if (m_pkeyboardInputFocus != nullptr) {
        m_pkeyboardInputFocus->SetFocusState(false);
    }
    m_pkeyboardInputFocus = pinput;
    m_pkeyboardInputFocus->SetFocusState(true);
}

void Window::RemoveFocus(IKeyboardInput* pinput)
{
    ZAssert(m_pkeyboardInputFocus == pinput);
    m_pkeyboardInputFocus->SetFocusState(false);
    m_pkeyboardInputFocus = nullptr;
}

//////////////////////////////////////////////////////////////////////////////
//
// Mouse
//
//////////////////////////////////////////////////////////////////////////////

PFNTrackMouseEvent Window::s_pfnTrackMouseEvent;

bool Window::OnMouseMessage(UINT message, UINT nFlags, const WinPoint& pointArg)
{
    return true;
}

void Window::SetCursorPos(const Point& point)
{
    WinPoint 
        pointScreen(
            (int)point.X(),
            m_rectClient.YSize() - 1 - (int)point.Y()
        );

    SetCursorPos(pointScreen);
}

//////////////////////////////////////////////////////////////////////////////
//
// The hierarchy of images changed so fake a mouse move to redo hit testing
//
//////////////////////////////////////////////////////////////////////////////

void Window::DoHitTest()
{
    POINT point;

    ::GetCursorPos(&point);
    ::SetCursorPos(point.x, point.y);
}

//////////////////////////////////////////////////////////////////////////////
//
// Windows APIs
//
//////////////////////////////////////////////////////////////////////////////

void Window::SetCursorPos(const WinPoint& point)
{
    WinPoint pointScreen = ClientToScreen(point);
    ::SetCursorPos(pointScreen.X(), pointScreen.Y());
}

ZString Window::GetText() const
{
    int length = GetWindowTextLength(m_hwnd) + 1;

    if (length == 0) {
        return ZString();
    } else {
        char* pch = new char[length];
        GetWindowText(m_hwnd, pch, length);
        ZString str(pch);
        delete[] pch;
        return str;
    }
}

HCURSOR Window::GetCursor() const
{
    if (m_hcursor != nullptr) {
        return m_hcursor;
    } else {
        // inherit our cursor from our parent window

        if (GetParent() != nullptr) {
            return GetParent()->GetCursor();
        } else {
            return nullptr;
        }
    }
}

void Window::ShowMouse(bool bShow)
{
    if (m_bShowMouse != bShow) {
        m_bShowMouse = bShow;
        ::ShowCursor(m_bShowMouse);
    }
}

void Window::CaptureMouse()
{
    ::SetCapture(m_hwnd);
}

void Window::ReleaseMouse()
{
    ::ReleaseCapture();
}

void Window::ShowCursor(bool bShow)
{
    ZUnimplemented();
}

bool Window::OnVScroll(UINT nSBCode, UINT nPos, HWND hwnd)
{
    return false;
}

bool Window::OnHScroll(UINT nSBCode, UINT nPos, HWND hwnd)
{
    return false;
}

void Window::OnPaint(HDC hdc, const WinRect& rect)
{
}

bool Window::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return false;
}

bool Window::OnSysCommand(UINT uCmdType, const WinPoint& point)
{
    return false;
}

bool Window::OnActivate(UINT nState, bool bMinimized)
{
    return false;
}

bool Window::OnActivateApp(bool bActive)
{
    return false;
}

bool Window::OnTimer()
{
    return false;
}

uint32_t Window::OriginalWndProc(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    return CallWindowProc(m_pfnWndProc, m_hwnd, message, wParam, lParam);
}

WinPoint MakePoint(LPARAM lParam)
{
    uint16_t x = LOWORD(lParam);
    uint16_t y = HIWORD(lParam);

    int fx = *(short int*)(&x);
    int fy = *(short int*)(&y);

    return WinPoint(fx, fy);
}

void Window::OnClose()
{
    ::PostQuitMessage(0);
}

void Window::OnDestroy()
{
}

bool Window::OnWindowPosChanging(WINDOWPOS* pwp)
{
    if ((pwp->flags & SWP_NOSIZE) == 0) {
        if (pwp->cx < m_sizeMin.X()) {
            pwp->cx = m_sizeMin.X();
            if (pwp->x != m_rect.XMin()) {
                pwp->x = m_rect.XMax() - m_sizeMin.X();
            }
        }
        if (pwp->cy < m_sizeMin.Y()) {
            pwp->cy = m_sizeMin.Y();
            if (pwp->y != m_rect.YMin()) {
                pwp->y = m_rect.YMax() - m_sizeMin.Y();
            }
        }
    }

    return true;
}


void Window::SetCursor(HCURSOR hcursor)
{
    POINT pntCursorPos;
    HWND hwndBelowCursor;

    m_hcursor = hcursor;

    // send an update cursor message to the window under the cursor
    if (GetCursorPos(&pntCursorPos)) {
        hwndBelowCursor = WindowFromPoint(pntCursorPos);

        if (hwndBelowCursor) {
            // REVIEW: ideally we'd send a WM_NCHITTEST message to figure out 
            // which cursor to set, but if the app below the cursor was frozen
            // that could freeze us too.  
            ::PostMessage(hwndBelowCursor, WM_SETCURSOR, (unsigned)hwndBelowCursor, 
                MAKELONG(HTCLIENT, WM_MOUSEMOVE));
        }
    }
}

uint32_t Window::WndProc(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    switch (message) {
        case WM_CLOSE:
            OnClose();
            return 0;

        case WM_DESTROY:
            OnDestroy();
            return 0;

        case WM_PAINT:
            PAINTSTRUCT paint;
            BeginPaint(GetHWND(), &paint);
            OnPaint(paint.hdc, paint.rcPaint);
            EndPaint(GetHWND(), &paint);
            return 0;

        case WM_MOVE:
        case WM_SIZE:
            UpdateRect();
            RectChanged();
            break;

        case WM_TIMER:
            if (OnTimer())
                return 0;
            break;

        case WM_HSCROLL:
            if (OnHScroll((int)LOWORD(wParam), (short int) HIWORD(wParam), (HWND)lParam))
                return 0;
            break;

        case WM_VSCROLL:
            if (OnVScroll((int)LOWORD(wParam), (short int) HIWORD(wParam), (HWND)lParam))
                return 0;
            break;

        case WM_COMMAND:
            if (OnCommand(wParam, lParam))
                return 0;
            break;

        case WM_SYSCOMMAND:
            if (OnSysCommand(wParam & 0xFFF0, MakePoint(lParam)))
                return 0;
            break;

        case WM_ACTIVATE:
            if (OnActivate(LOWORD(wParam), HIWORD(wParam) != 0))
                return 0;
            break;

        case WM_ACTIVATEAPP:
            if (OnActivateApp(wParam != 0))
                return 0;
            break;

        case WM_CHAR:
            {
                KeyState ks;
                ks.vk = wParam;
                ks.bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                ks.bControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                ks.countRepeat = LOWORD(lParam);

                return !OnChar(ks);
            }
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            {
                KeyState ks;
                ks.vk = wParam;
                ks.bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                ks.bControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                ks.bAlt = (message == WM_SYSKEYUP || message == WM_SYSKEYDOWN) 
                    && (lParam & (1 << 29)); // See help for WM_SYSKEYDOWN
                ks.bDown = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
                ks.countRepeat = LOWORD(lParam);

                return !OnKey(ks);
            }
            break;
		
		case WM_MOUSEWHEEL: //imago 8/13/09
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: 
        case WM_MOUSEHOVER: // imago --^
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSELEAVE:
		case WM_NCMOUSEHOVER: //Imago 7/10
		case WM_NCMOUSELEAVE: //<--^
            {
                WinPoint pointMouse;
                if (message != WM_MOUSEWHEEL) {
                    WinPoint point(MakePoint(lParam));
                    pointMouse = WinPoint(point.X(),m_rectClient.YSize() - 1 - point.Y());
                    m_lastPointMouse = pointMouse;
                } else {
                    pointMouse = m_lastPointMouse;
                }

                //
                // Handle mouse leave
                //

                if (message == WM_MOUSELEAVE || message == WM_NCMOUSELEAVE) {
                    if (m_bMouseInside) {
                        m_bMouseInside = false;
                        OnMouseMessage(WM_MOUSELEAVE, 0, pointMouse);
                    }
                } else {
                    //
                    // Handle mouse enter
                    //

                    if (!m_bMouseInside && (message != WM_NCMOUSEHOVER && message != WM_MOUSEHOVER && message != 0)) {
                        m_bMouseInside = true;
                        OnMouseMessage(0, 0, pointMouse);

                        if (s_pfnTrackMouseEvent) {
                            TRACKMOUSEEVENT tme;

                            tme.cbSize      = sizeof(TRACKMOUSEEVENT);
                            tme.dwFlags     = TME_LEAVE;
                            tme.hwndTrack   = m_hwnd;
                            tme.dwHoverTime = HOVER_DEFAULT;

                            ZVerify(s_pfnTrackMouseEvent(&tme));
                        }
                    }

                    //
                    // Handle the mouse message
                    //

                    uint32_t ret = 
                          OnMouseMessage(message, wParam, pointMouse)
                        ? 0
                        : 1;
                }

                return true;
            }
            break;

        case WM_SETCURSOR:
            //
            // use the custom cursor for the client area
            //

            if (LOWORD(lParam) == HTCLIENT) {
                ::SetCursor(nullptr);
            } else {
                ::SetCursor(GetCursor());
            }

            break;

        case WM_WINDOWPOSCHANGING:
            if (OnWindowPosChanging((LPWINDOWPOS)lParam)) 
                return 0;
            break;
    }

    return m_pfnWndProc(m_hwnd, message, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////
//
// Idle processing
//
//////////////////////////////////////////////////////////////////////////////

TRef<TList<Window*> > g_plistIdle;

void Window::DoIdle()
{
}

void Window::EnableIdleFunction(bool bEnable)
{
    if (g_plistIdle) {
        if (bEnable) {
            ZAssert(!g_plistIdle->Find(this));
            g_plistIdle->PushFront(this);
        } else {
            g_plistIdle->Remove(this);
        }
    }
}

void CallIdleFunctions()
{
    TList<Window*>::Iterator iter(*g_plistIdle);

    while (!iter.End()) {
        iter.Value()->DoIdle();
        iter.Next();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Win32 code
//
//////////////////////////////////////////////////////////////////////////////

TMap<HWND, Window* > Window::s_mapWindow;

uint32_t CALLBACK Window::Win32WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    Window* pwindow;

    if (message == WM_CREATE && 
            nullptr != (pwindow = (Window*)(((CREATESTRUCT *)lParam)->lpCreateParams))) {
        pwindow->m_hwnd = hwnd;
        s_mapWindow.Set(hwnd, pwindow);
    } else {
        if (!s_mapWindow.Find(hwnd, pwindow)) {
            pwindow = nullptr;
        }
    }

    if (pwindow) {
        return pwindow->WndProc(message, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

HRESULT Window::StaticInitialize()
{
    g_plistIdle = new TList<Window*>;

    WNDCLASS wc;

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)Win32WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 4;
    wc.hInstance     = GetModuleHandle(nullptr);
    wc.hIcon         = LoadIcon(nullptr, ID_APP_ICON);
    wc.hCursor       = nullptr;
#ifdef DEBUG
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); //imago test 7/7/09 (NYI final multimon issue, see enginewindow.cpp(386))
#else
	wc.hbrBackground = nullptr; //imago test
#endif
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = GetTopLevelWindowClassname();

    RegisterClass(&wc);

    //
    // See if TrackMouseEvent exists
    //
    s_pfnTrackMouseEvent = 
        (PFNTrackMouseEvent)GetProcAddress(
            GetModuleHandle("user32"),
            "TrackMouseEvent"
        );
    return S_OK;
}

HRESULT Window::StaticTerminate()
{
	// BT - 10/17 - Fixing crash when allegiance is exited from the new game screen.
	RemoveAllKeyboardInputFilters();

    g_plistIdle = nullptr;
    return S_OK;
}

Window* Window::WindowFromHWND(HWND hwnd)
{
    Window* pwindow = nullptr;
    if (!s_mapWindow.Find(hwnd, pwindow))
        pwindow = nullptr;
    return pwindow;
}

bool g_bContinuousIdle = true;

void Window::SetContinuousIdle(bool b)
{
    g_bContinuousIdle = b;
}

#ifdef ICAP
    int  Window::s_countProfile    = 0;
    bool Window::s_bProfileStarted = false;

    void Window::ToggleProfiling(int count)
    {
        if (count == -1) {
            if (s_countProfile == 0) {
                s_countProfile = -1;
            } else {
                s_countProfile = 0;
            }
        } else {
            s_countProfile = count;
        }
    }

    bool Window::IsProfiling()
    {
        return s_bProfileStarted;
    }
#endif

HRESULT Window::MessageLoop()
{
    MSG msg;

    while (true) {
        //
        // Turn on Profiling
        //

        #ifdef ICAP
            if (s_bProfileStarted) {
                if (s_countProfile == 0) {
                    MarkProfile(2); 
                    StopProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
                    s_bProfileStarted = false;
                }
            } else {
                if (s_countProfile != 0) {
                    StartProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
                    MarkProfile(1); 
                    s_bProfileStarted = true;
                }

                if (s_countProfile > 0) {
                    s_countProfile--;
                }
            }
        #endif

        //
        // Call Timers and Idle functions
        //

        CallTimers();
        CallIdleFunctions();

        //
        // Handles Win32 messages
        //

        bool bAnyMessage = true;
        if (g_bContinuousIdle) {
            bAnyMessage = ::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0;
        } else {
            ::GetMessage(&msg, nullptr, 0, 0);
        }

        if (bAnyMessage) {
            do {
                if (msg.message == WM_QUIT) {
                    s_mapWindow.SetEmpty();
                    return S_OK;
                }

                switch (msg.message) {
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                        {
                            KeyState ks;
                            ks.vk = msg.wParam;
                            ks.bAlt = (msg.message == WM_SYSKEYUP || msg.message == WM_SYSKEYDOWN) 
                                && (msg.lParam & (1 << 29)); // See help for WM_SYSKEYDOWN
                            ks.bShift = (GetKeyState(VK_SHIFT) & 0x8000) !=0;
                            ks.bControl = (GetKeyState(VK_CONTROL) & 0x8000) !=0;
                            ks.bDown = (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || ks.vk == 44); // special exception for PrintScreen key
                            ks.countRepeat = LOWORD(msg.lParam);

							bool fHandled = false;
                            bool fForceTranslate = false;

                            TList<TRef<IKeyboardInput> >::Iterator iter(g_listKeyboardInputFilters);
                            while (!iter.End() && !fHandled) {
                                fHandled = iter.Value()->OnKey(nullptr, ks, fForceTranslate);
                                iter.Next();
                            }

                            if (!fHandled) {
                                fHandled = (::DispatchMessage(&msg) == 0);
                            }

                            if (!fHandled || fForceTranslate) {
                                ::TranslateMessage(&msg);
                            }
                        }
                        break;

                    case WM_CHAR:
                        {
                            KeyState ks;
                            ks.vk = msg.wParam;
                            ks.bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                            ks.bControl = (GetKeyState(VK_CONTROL) & 0x8000) !=0;
                            ks.countRepeat = LOWORD(msg.lParam);

                            bool fHandled = false;

                            TList<TRef<IKeyboardInput> >::Iterator iter(g_listKeyboardInputFilters);
                            while (!iter.End() && !fHandled) {
                                fHandled = iter.Value()->OnChar(nullptr, ks);
                                iter.Next();
                            }

                            if (fHandled) {
                                break;
                            }
                        }

                        //
                        // intentional fallthrough
                        //

                    default:
                        ::DispatchMessage(&msg);
                        break;
                } 
            } while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE));
        }
    }
}
