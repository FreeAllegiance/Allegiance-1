#ifndef _enginewindow_h_
#define _enginewindow_h_

//////////////////////////////////////////////////////////////////////////////
//
// A window with an associated engine object
//
//////////////////////////////////////////////////////////////////////////////

class EngineWindow :
    public Window,
    public IKeyboardInput,
    public ICaptionSite,
    public ButtonEvent::Sink
{
protected:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Types
    //
    //////////////////////////////////////////////////////////////////////////////

    class ModeData {
    public:
        WinPoint m_size;
        bool     m_bStretch;

        ModeData(const WinPoint& size, bool bStretch) :
            m_size(size),
            m_bStretch(bStretch)
        {
        }

        ModeData(const ModeData& data) :
            m_size(data.m_size),
            m_bStretch(data.m_bStretch)
        {
        }
    };

    class MenuCommandSink : public IMenuCommandSink {
    private:
        EngineWindow* m_pwindow;

    public:    
        MenuCommandSink(EngineWindow* pwindow) :
            m_pwindow(pwindow)
        {
        }

        void OnMenuCommand(IMenuItem* pitem);
    };

    friend class MenuCommandSink;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Static Members
    //
    //////////////////////////////////////////////////////////////////////////////

    static ModeData s_pmodes[];
    static int      s_countModes;
    static int      s_forceHitTestCount;
    static bool     s_cursorIsHidden;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Members
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Engine>               m_pengine;
    TRef<Modeler>              m_pmodeler;
    TRef<InputEngine>          m_pinputEngine;
    TRef<ButtonEvent::Sink>    m_pbuttonEventSink;
    TRef<MouseInputStream>     m_pmouse;
    TRef<ModifiablePointValue> m_ppointMouse;
	EngineApp *					m_pEngineApp;

    TRef<Surface>              m_psurface;
    TRef<ICaption>             m_pcaption;
    WinRect                    m_rectWindowed;

    TRef<IKeyboardInput>       m_pkeyboardInput;

    TRef<GroupImage>           m_pgroupImage;
    TRef<WrapImage>            m_pwrapImage;
    TRef<TransformImage>       m_ptransformImageCursor;
    TRef<TranslateTransform2>  m_ptranslateTransform;
    TRef<Image>                m_pimageCursor;
    TRef<IPopupContainer>      m_ppopupContainer;

    WinPoint                   m_offsetWindowed;

    bool                       m_bMovingWindow;
    bool                       m_bSizeable;
    bool                       m_bMinimized;
    bool                       m_bHideCursor;
    bool                       m_bCaptured;
    bool                       m_bHit;
    bool                       m_bInvalid;
    bool                       m_bActive;
    bool                       m_bShowCursor;
    bool                       m_bRestore;
    bool                       m_bMouseInside;
    bool                       m_bMoveOnHide;
	bool						m_bStartFullScreen;
	bool						m_bWindowStateMinimised;
	bool						m_bWindowStateRestored;
	bool						m_bClickBreak;

    int                        m_modeIndex;

    TRef<ModifiableRectValue>  m_prectValueScreen;
    TRef<ModifiableRectValue>  m_prectValueRender;
    TRef<WrapRectValue>        m_pwrapRectValueRender;

    TRef<ModifiableNumber>     m_pnumberTime;
    Time                       m_timeStart;
    Time                       m_timeLast;
    Time                       m_timeLastFrame;
    Time                       m_timeCurrent;
    Time                       m_timeLastMouseMove;
    Time                       m_timeLastClick;

    //
    // Input
    //

    TRef<ButtonEvent::Sink>    m_peventSink;

    //
    // menu
    //

    TRef<IMenuCommandSink>     m_pmenuCommandSink;
    TRef<IMenuItem>            m_pitemDevice;
//    TRef<IMenuItem>            m_pitemRenderer;
    TRef<IMenuItem>            m_pitemResolution;
    TRef<IMenuItem>            m_pitemRendering;
    TRef<IMenuItem>            m_pitemBPP; // KGJV 32B
  //  TRef<IMenuItem>            m_pitemAllowSecondary;
//    TRef<IMenuItem>            m_pitemAllow3DAcceleration;
    TRef<IMenuItem>            m_pitemHigherResolution;
    TRef<IMenuItem>            m_pitemLowerResolution;

    //
    // Performance
    //

    char                       m_pszLabel[20];
    bool                       m_bFPS;
    int                        m_indexFPS;
    float                      m_frameCount;
    int                        m_frameTotal;
    ZString                    m_strPerformance1;
    ZString                    m_strPerformance2;
    TRef<IEngineFont>          m_pfontFPS;

    double                     m_triangles;
    double                     m_tpf;
    double                     m_ppf;  
    double                     m_tps;  
    double                     m_fps;  
    double                     m_mspf; 

    //////////////////////////////////////////////////////////////////////////////
    //
    // Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    //
    // Performance
    //

    void UpdatePerformanceCounters(Context* pcontext, Time timeCurrent);
    void RenderPerformanceCounters(Surface* psurface);

    //
    // Implementation methods
    //

    void UpdateRectValues();
    void UpdateWindowStyle();
    void UpdateSurfacePointer();
    void UpdateCursor();
    
    void UpdateInput();
    void HandleMouseMessage(UINT message, const Point& point, UINT nFlags = NULL);

    void ParseCommandLine(const ZString& strCommandLine, bool& bStartFullscreen);
    void DoIdle();
    bool ShouldDrawFrame();
    bool RenderFrame();
    void UpdateFrame();
    void Invalidate();

    //
    // menu
    //

    void    OnEngineWindowMenuCommand(IMenuItem* pitem);
    ZString GetRendererString();
    ZString GetDeviceString();
    ZString GetResolutionString();
    ZString GetRenderingString();
    ZString GetPixelFormatString(); // KGJV 32B
    ZString GetAllow3DAccelerationString();
    ZString GetAllowSecondaryString();
    void    UpdateMenuStrings();

public:
    EngineWindow(
              EngineApp*   papp,
        const ZString&     strCommandLine,
        const ZString&     strTitle         = ZString(),
              bool         bStartFullscreen = false,
        const WinRect&     rect             = WinRect(0, 0, -1, -1),
        const WinPoint&    sizeMin          = WinPoint(1, 1),
              HMENU        hmenu            = NULL
    );

    ~EngineWindow();

    //
    // Static Functions
    //

    static void DoHitTest();

    //
    // EngineWindow methods
    //

	// Added so that we could reorganise the device creation order.
	void			InitialiseTime();
	void			PostWindowCreationInit();

    Number*          GetTime()           { return m_pnumberTime;             }
    Engine*          GetEngine()         { return m_pengine;                 }
    Modeler*         GetModeler()        { return m_pmodeler;                }
    bool             GetFullscreen()     { return m_pengine->IsFullscreen(); }
    bool             GetShowFPS()        { return m_bFPS;                    }
    IPopupContainer* GetPopupContainer() { return m_ppopupContainer;         }
    InputEngine*     GetInputEngine()    { return m_pinputEngine;            }
    const Point&     GetMousePosition()  { return m_ppointMouse->GetValue(); }
    ModifiablePointValue* GetMousePositionModifiable() { return m_ppointMouse; }
	Time&		   	 GetMouseActivity()  { return m_timeLastMouseMove;		 } //Imago: Added to adjust AFK status from mouse movment
    bool             GetActive()         { return m_bActive;                 }

    TRef<IPopup> GetEngineMenu(IEngineFont* pfont);

    RectValue* GetScreenRectValue();
    RectValue* GetRenderRectValue();

    void SetFullscreen(bool bFullscreen);
    void SetSizeable(bool bSizeable);
    void SetWindowedSize(const WinPoint& point);
    void SetFullscreenSize(const Vector& point);
    void ChangeFullscreenSize(bool bLarger);
    void Set3DAccelerationImportant(bool b3DAccelerationImportant);
    void SetMouseEnabled(bool bEnable);

    WinPoint GetSize();
    WinPoint GetWindowedSize();
    WinPoint GetFullscreenSize();

    void OutputPerformanceCounters();
    void SetImage(Image* pimage);
    void SetCursorImage(Image* pimage);
    Image* GetCursorImage(void) const;
    void SetCaption(ICaption* pcaption);
    void SetHideCursorTimer(bool bHideCursor);

    void SetShowFPS(bool bFPS, const char* pszLabel = NULL);
    void ToggleShowFPS();
    void SetMoveOnHide(bool bMoveOnHide);

    //
    // App exit
    //

    virtual void StartClose();

    //
    // subclass overrides
    //

    virtual ZString GetFPSString(float dtime, float mspf, Context* pcontext);

    virtual void EvaluateFrame(Time time) {}
    virtual void RenderSizeChanged(bool bSmaller) {}

    //
    // ICaptionSite methods
    //

    void OnCaptionMinimize();
    void OnCaptionMaximize();
    void OnCaptionFullscreen();
    void OnCaptionRestore();
    void OnCaptionClose();

    //
    // ButtonEvent::Sink methods
    //

    bool OnEvent(ButtonEvent::Source* pevent, ButtonEventData data);

    //
    // IInputProvider methods
    //

    void SetCursorPos(const Point& point);
    bool IsDoubleClick();
    void ShowCursor(bool bShow);

    //
    // Window methods
    //

    void RectChanged();
    void OnClose();
    bool OnActivate(UINT nState, bool bMinimized);
    bool OnActivateApp(bool bActive);
    bool OnMouseMessage(UINT message, UINT nFlags, const WinPoint& point);
    bool OnSysCommand(UINT uCmdType, const WinPoint &point);
    void OnPaint(HDC hdc, const WinRect& rect);
    bool OnWindowPosChanging(WINDOWPOS* pwp);

    //
    // IObject methods
    //

    bool IsValid();
};

#endif
