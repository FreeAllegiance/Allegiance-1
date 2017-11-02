#ifndef _Window_h_
#define _Window_h_

#include "event.h"
#include "input.h"
#include "rect.h"
#include "tmap.h"
#include "winstyles.h"

#include "commctrl.h"

// x64: GWL is GWLP in x64 SDK Imago 6/20/09
#if defined (_WIN64) 
	#define GWLx_WNDPROC GWLP_WNDPROC
#else
	#define GWLx_WNDPROC GWL_WNDPROC
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Window
//
//////////////////////////////////////////////////////////////////////////////

#ifndef ID_APP_ICON
    #define ID_APP_ICON IDI_APPLICATION
#endif

__declspec(dllexport) void FillSolidRect(HDC hdc, const WinRect& rect, COLORREF color);

    typedef BOOL (WINAPI *PFNTrackMouseEvent)(TRACKMOUSEEVENT* tme);

//////////////////////////////////////////////////////////////////////////////
//
// Window
//
//////////////////////////////////////////////////////////////////////////////


class Window :
    public IInputProvider,
    public ITimerEventSource
{
private:
    HWND     m_hwnd;
    HMENU    m_hmenu;
    HCURSOR  m_hcursor;
    Style    m_style;
    StyleEX  m_styleEX;
    WinRect  m_rectClient;
    WinRect  m_rect;
    WinPoint m_sizeMin;
    WNDPROC  m_pfnWndProc;
    UINT     m_nID;
    bool     m_bSizeable;
    bool     m_bTopMost;
    bool     m_bHasMinimize;
    bool     m_bHasMaximize;
    bool     m_bHasSysMenu;
    bool     m_bHit;
    bool     m_bShowMouse;
    bool     m_bMouseInside;
    WinPoint m_lastPointMouse;

    Window* m_pwindowParent;

    TList<TRef<Window> > m_listChildren;

    TRef<IKeyboardInput> m_pkeyboardInputFocus;

    //
    // Profiling
    //

    #ifdef ICAP
        static int  s_countProfile;
        static bool s_bProfileStarted;
    #endif

    void UpdateRect();
    void CalcStyle();

    static PFNTrackMouseEvent s_pfnTrackMouseEvent;
    static TList<TRef<IKeyboardInput> > m_listKeyboardInputFilters;

    static TMap<HWND, Window* > s_mapWindow;
    static uint32_t CALLBACK Win32WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void Construct();

	void ClipOrCenterRectToMonitor(LPRECT prc, UINT flags);
	void ClipOrCenterWindowToMonitor( const WinRect& rect, UINT flags );

protected:
    uint32_t OriginalWndProc(UINT, WPARAM, LPARAM);
    static void DoHitTest();

public:
    //
    // Profiling
    //

    #ifdef ICAP
        static void ToggleProfiling(int count);
        static bool IsProfiling();
    #endif

    //
    // static interfaces
    //

    static HRESULT StaticInitialize();
    static HRESULT StaticTerminate();
    static HRESULT MessageLoop();
    static void SetContinuousIdle(bool b);
    static Window* WindowFromHWND(HWND hwnd);
    static void AddKeyboardInputFilter(IKeyboardInput* pkeyboardInput);
    static void RemoveKeyboardInputFilter(IKeyboardInput* pkeyboardInput);
	static void RemoveAllKeyboardInputFilters(); // BT - 10/17 - Fixing crash when allegiance is exited from the new game screen.
    static LPCTSTR GetTopLevelWindowClassname() { return TEXT("MS_ZLib_Window"); };
	
    //
    // ITimerEventSource
    //

    void AddSink(IEventSink* psink);
    void AddSink(IEventSink* psink, float when);
    void RemoveSink(IEventSink* psink);

    //
    // IInputProvider
    //

    ITimerEventSource* GetTimer();
    bool IsFocus(IKeyboardInput* pinput);
    IKeyboardInput* GetFocus();
    void SetFocus(IKeyboardInput* pinput);
    void RemoveFocus(IKeyboardInput* pinput);
    void SetCursorPos(const Point& point);
    void ShowCursor(bool bShow);

    //
    // Hierachy
    //

    void AddChild(Window* pchild);
    void RemoveChild(Window* pchild);
    void RemoveAllChildren();

    Window* GetParent() { return m_pwindowParent; }
    const Window* GetParent() const { return m_pwindowParent; }

    //
    // Window message handlers
    //

    virtual uint32_t WndProc(UINT, WPARAM, LPARAM);

    virtual void ChildRectChanged(Window* pchild);
    virtual void RectChanged();

    virtual void OnDestroy();
    virtual void OnClose();
    virtual void OnPaint(HDC hdc, const WinRect& rect);

    virtual bool OnVScroll(UINT nSBCode, UINT nPos, HWND hwnd);
    virtual bool OnHScroll(UINT nSBCode, UINT nPos, HWND hwnd);
    virtual bool OnTimer();
    virtual bool OnCommand(WPARAM wParam, LPARAM lParam);
    virtual bool OnSysCommand(UINT uCmdType, const WinPoint& point);
    virtual bool OnActivate(UINT nState, bool bMinimized);
    virtual bool OnActivateApp(bool bActive);
    virtual bool OnChar(const KeyState& ks);
    virtual bool OnKey(const KeyState& ks);
    virtual bool OnMouseMessage(UINT message, UINT nFlags, const WinPoint& point);
	virtual bool OnWindowPosChanging(WINDOWPOS* pwp);

    //
    // constructors
    //

    Window(
              Window*  pwindowParent,
        const WinRect& rect          = WinRect(0, 0, -1, -1),
        const ZString& strTitle      = ZString(),
        const ZString& strClass      = ZString(),
              Style    style         = Style(),
              HMENU    hmenu         = NULL,
              StyleEX  styleEX       = StyleEX()
    );

    BOOL Create(
            Window*     pwindowParent,
            const WinRect& rect = WinRect(0, 0, -1, -1),
            LPCSTR      szTitle = NULL,
            LPCSTR      szClass = NULL,
            Style       style   = Style(),
            HMENU       hmenu   = 0,
            UINT        nID     = 0,
            StyleEX     styleEX = StyleEX()

    );

    Window();  //constructor for dialogs
    virtual ~Window();

    //
    // Accessors
    //

          HWND     GetHWND()       const { return m_hwnd;       }
    const WinRect& GetRect()       const { return m_rect;       }
    const WinRect& GetClientRect() const { return m_rectClient; }
          HMENU    GetMenu()       const { return m_hmenu;      }
          HCURSOR  GetCursor()     const;
          ZString  GetText()       const;

          bool     IsSizeable()    const { return m_bSizeable;     }
          bool     HasMinimize()   const { return m_bHasMinimize; }
          bool     HasMaximize()   const { return m_bHasMaximize; }
          bool     HasSysMenu()    const { return m_bHasSysMenu;  }

    //
    // Attributes
    //

    void SetRect(const WinRect& rect);
    void SetClientRect(const WinRect& rect);
    void SetPosition(const WinPoint& point);
    void SetSize(const WinPoint& point);
    void SetClientSize(const WinPoint& point);
    void SetMinimumClientSize(const WinPoint& point);

    void SetSizeable(bool bSizable);
    void SetTopMost(bool bTopMost);
    void SetHasMinimize(bool bMinimize);
    void SetHasMaximize(bool bMaximize);
    void SetHasSysMenu(bool bSysMenu);

    void SetCursor(HCURSOR hcursor);

    //
    // Idle processing
    //

    void EnableIdleFunction(bool bEnable = true);
    virtual void DoIdle();

    //
    // window function wrappers
    //

	void ShowMouse(bool bShow);
    void CaptureMouse();
    void ReleaseMouse();
    void SetCursorPos(const WinPoint& point);

    BOOL InvalidateRgn(HRGN hrgn = NULL, bool bErase = true)
    {
        return ::InvalidateRgn(m_hwnd, hrgn, bErase);
    }

    BOOL RedrawWindow(HRGN hrgn = NULL, UINT flags = RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW)
    {
        return ::RedrawWindow(m_hwnd, NULL, hrgn, flags);
    }

    BOOL RedrawWindow(const WinRect& rect, UINT flags = RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW)
    {
        return ::RedrawWindow(m_hwnd, &(RECT)rect, NULL, flags);
    }

    int MessageBox(const ZString& strText, const ZString& strCaption, UINT nType)
    {
        return ::MessageBox(m_hwnd, strText, strCaption, nType);
    }

    HMENU GetSystemMenu(BOOL bRevert = false)
    { 
        return ::GetSystemMenu(m_hwnd, bRevert); 
    }

    void DrawMenuBar()
    {
        ::DrawMenuBar(m_hwnd);
    }

    BOOL SetWindowText(LPCTSTR lpszString)
    {
        return ::SetWindowText(m_hwnd, lpszString);
    }

    BOOL ShowWindow(int nCmdShow)
    {
        return ::ShowWindow(m_hwnd, nCmdShow);
    }

    int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE)
    {
        return ::SetScrollPos(m_hwnd, nBar, nPos, bRedraw);
    }

    BOOL SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE)
    {
        return ::SetScrollInfo(m_hwnd, nBar, lpScrollInfo, bRedraw);
    }

    WinPoint ClientToScreen(const WinPoint& point)
    {
        POINT pt = point;
        ::ClientToScreen(m_hwnd, &pt);
        return WinPoint(pt.x, pt.y);
    }

    WinPoint ScreenToClient(const WinPoint& point)
    {
        POINT pt = point;
        ::ScreenToClient(m_hwnd, &pt);
        return WinPoint(pt.x, pt.y);
    }

    void SetFocus()
    {
        ::SetFocus(m_hwnd);
    }

    void SetForeground()
    {
        ::SetForegroundWindow(m_hwnd);
    }

    LRESULT SendMessage( UINT message, WPARAM wParam = 0, LPARAM lParam = 0 )
    {
        return ::SendMessage(m_hwnd, message, wParam, lParam);
    }

    LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0 )
    {
        return ::SendDlgItemMessage(m_hwnd, nID, message, wParam, lParam);
    }

    BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        return ::PostMessage(m_hwnd, message, wParam, lParam);
    }

    BOOL TrackPopupMenu(HMENU hmenu, UINT nFlags, int x, int y, LPCRECT lpRect = NULL )
    {
        return ::TrackPopupMenu(hmenu, nFlags, x, y, 0, m_hwnd, lpRect);
    }

    HDC GetDC()
    {
        return ::GetDC(m_hwnd);
    }

    void ReleaseDC(HDC hdc)
    {
        ::ReleaseDC(m_hwnd, hdc);
    }

    /*
    HRESULT RegisterDragDrop(IDropTarget* ptarget)
    {
        return ::RegisterDragDrop(m_hwnd, ptarget);
    }

    HRESULT RevokeDragDrop()
    {
        return ::RevokeDragDrop(m_hwnd);
    }
    */

    //
    // Window message wrappers
    //

    void SetFont(HFONT hfont)
    {
        ::SendMessage(m_hwnd, WM_SETFONT, (WPARAM)hfont, false);
    }
};

#endif
