#include "enginewindow.h"

#include <token.h>

#include "VertexGenerator.h"
#include "EngineSettings.h"
#include "image.h"
#include "engineapp.h"
#include "LogFile.h"
#include "enginep.h"
#include "D3DDevice9.h"

#include "regkey.h"

bool g_bLuaDebug = false;

//////////////////////////////////////////////////////////////////////////////
//
// MenuCommandSink
//
//////////////////////////////////////////////////////////////////////////////

void EngineWindow::MenuCommandSink::OnMenuCommand(IMenuItem* pitem)
{
    m_pwindow->OnEngineWindowMenuCommand(pitem);
}

//////////////////////////////////////////////////////////////////////////////
//
// Static members
//
//////////////////////////////////////////////////////////////////////////////



//See also engine.cpp(180) if changing this list.
EngineWindow::ModeData EngineWindow::s_pmodes[] = //imago updated 6/29/09 NYI letterbox/strech non 4:3
    {
		ModeData(WinPoint(640, 480), false),
		ModeData(WinPoint(800, 600), false),
		ModeData(WinPoint(1024, 768), false),
		ModeData(WinPoint(1280, 1024), false),
		ModeData(WinPoint(1366, 768), false), //720p thanks to badpazzword 7/1/09
		ModeData(WinPoint(1400, 1050), false),		
		ModeData(WinPoint(1440, 900),  false),
		ModeData(WinPoint(1600, 1200), false),
		ModeData(WinPoint(1680, 1050), false),
		ModeData(WinPoint(1920, 1080), false),
        ModeData(WinPoint(1920, 1200), false),
        ModeData(WinPoint(2560, 1440), false)
    };

int EngineWindow::s_countModes = 11; //this is not the count, this number is the largest available index.

//////////////////////////////////////////////////////////////////////////////
//
// Keyboard Input Filter
//
//////////////////////////////////////////////////////////////////////////////

class EngineWindowKeyboardInput : public IKeyboardInput {
private:
    EngineWindow* m_pwindow;

public:
    EngineWindowKeyboardInput(EngineWindow* pwindow) :
        m_pwindow(pwindow)
    {
    }

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (ks.bDown && ks.bAlt) {
            switch(ks.vk) {
                case VK_F4:
                    m_pwindow->StartClose();
                    return true;

                case VK_RETURN:
                    m_pwindow->SetFullscreen(!m_pwindow->GetFullscreen());
                    return true;

                case 'F':
                    m_pwindow->ToggleShowFPS();
                    return true;

                #ifdef ICAP
                    case 'P':
                        m_pwindow->ToggleProfiling(-1);
                        return true;

                    case 'O':
                        m_pwindow->ToggleProfiling(1);
                        return true;
                #endif

                #ifdef _DEBUG
                    case VK_F9:
                        if (ks.bShift) {
                            ZError("Forced Assert");
                        }
                        return true;

                    case VK_F10:
                        if (ks.bShift) {
                            *(DWORD*)NULL = 0;
                        }
                        return true;
                #endif
            }
        }

        return false;
    }

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

EngineWindow::EngineWindow(	EngineConfigurationWrapper* pConfiguration,
							const ZString&		strCommandLine,
							const ZString&		strTitle,
							bool				bStartFullscreen,
							const WinRect&		rect,
							const WinPoint&		sizeMin,
							HMENU				hmenu
) :
				Window(NULL, rect, strTitle, ZString(), 0, hmenu),
				m_pengine(nullptr),
				m_pmodeler(nullptr),
				m_offsetWindowed(rect.Min()),
				m_bSizeable(true),
				m_bMinimized(false),
				m_bMovingWindow(false),
				m_pimageCursor(Image::GetEmpty()),
				m_bHideCursor(false),
				m_bCaptured(false),
				m_bHit(false),
				m_bInvalid(true),
				m_bActive(true),
				m_bShowCursor(true),
				m_bRestore(false),
				m_bMouseInside(false),
				m_bMoveOnHide(true),
				m_bWindowStateMinimised(false),
				m_bWindowStateRestored(false),
				m_bClickBreak(true), //Imago 7/10 #37
                m_pConfiguration(pConfiguration),
                m_pConfigurationUpdater(new ValueList(nullptr))
{
    GlobalConfigureLoggers(
        m_pConfiguration->GetDebugLogToOutput()->GetValue(),
        m_pConfiguration->GetDebugLogToFile()->GetValue()
    );

    g_bMDLLog = m_pConfiguration->GetDebugMdl()->GetValue();
    g_bWindowLog = m_pConfiguration->GetDebugWindow()->GetValue();
    g_bLuaDebug = m_pConfiguration->GetDebugLua()->GetValue();

    m_pConfigurationUpdater->PushEnd(new CallbackWhenChanged<bool>([this](bool bFullscreen) {
        SetFullscreen(bFullscreen);
    }, m_pConfiguration->GetGraphicsFullscreen()));

    m_pConfigurationUpdater->PushEnd(new CallbackWhenChanged<float, float>([this](float x, float y) {
        m_pengine->SetFullscreenSize(WinPoint((int)x, (int)y));
    }, m_pConfiguration->GetGraphicsResolutionX(), m_pConfiguration->GetGraphicsResolutionY()));

    m_pConfigurationUpdater->PushEnd(new CallbackWhenChanged<float>([this](float gamma) {
        m_pengine->SetGammaLevel(gamma);
    }, m_pConfiguration->GetGraphicsGamma()));

    m_pConfigurationUpdater->PushEnd(new CallbackWhenChanged<bool>([this](bool vsync) {
        m_pengine->SetVSync(vsync);
    }, m_pConfiguration->GetGraphicsUseVSync()));

    m_pConfigurationUpdater->PushEnd(new CallbackWhenChanged<bool>([this](bool aa) {
        m_pengine->SetAA(aa ? 1 : 0);
    }, m_pConfiguration->GetGraphicsUseAntiAliasing()));

    m_pcloseEventSource = new EventSourceImpl();
    m_pevaluateFrameEventSource = new TEvent<Time>::SourceImpl();

    //
    // Button Event Sink
    //

    m_pbuttonEventSink = new ButtonEvent::Delegate(this);

    //
    // mouse position
    //

    m_ppointMouse = new ModifiablePointValue(Point(0, 0));

    // Create the input engine
    //

    m_pinputEngine = CreateInputEngine(GetHWND());
    m_pinputEngine->SetFocus(true);

    //
    // Should we start fullscreen?
	CD3DDevice9 * pDev = CD3DDevice9::Get();

    m_pPreferredFullscreen = m_pConfiguration->GetGraphicsFullscreen();

    bool bStartFullScreen = m_pPreferredFullscreen->GetValue();
    ParseCommandLine(strCommandLine, bStartFullScreen);

    if (bStartFullScreen != m_pPreferredFullscreen->GetValue()) {
        m_pPreferredFullscreen->SetValue(bStartFullScreen);
    }

    // Get the mouse
    //

    m_pmouse = m_pinputEngine->GetMouse();
    m_pmouse->SetEnabled(bStartFullscreen);

    m_pmouse->GetEventSource()->AddSink(m_peventSink = new ButtonEvent::Delegate(this));

    //
    // Make the minimum window size
    //

    SetMinimumClientSize(sizeMin);

    // Filter all keyboard input
    m_pkeyboardInput = new EngineWindowKeyboardInput(this);
    AddKeyboardInputFilter(m_pkeyboardInput);

    // Initialize performance counters
    m_pszLabel[0]   = '\0';
    m_bFPS          = false;
    m_indexFPS      = 0;
    m_frameCount    = 0;
    m_frameTotal    = 0;
    m_timeLastFrame =
    m_timeLast      =
    m_timeCurrent   = 
    m_timeStart     = Time::Now();
    m_timeLastClick = 0;

    // menu
    m_pmenuCommandSink  = new MenuCommandSink(this);
	

    // Start the callback
    EnableIdleFunction();

	// Create the D3D device.
	HWND hWindow = GetHWND();

	CLogFile devLog( "DeviceCreation.log" );
	pDev->Initialise( &devLog );
	if( pDev->CreateD3D9( &devLog ) != D3D_OK )
	{
        ZAssert( false );
	}
	if( pDev->CreateDevice( hWindow, &devLog ) != D3D_OK )
	{
        ZAssert( false );
	}
	devLog.OutputString( "Device creation finished.\n" );

	// Initialise the various engine components.
	CVRAMManager::Get()->Initialise( );
	devLog.OutputString("CVRAMManager::Get()->Initialise( )\n");

	CVBIBManager::Get()->Initialise( );

	devLog.OutputString("CVBIBManager::Get()->Initialise( );\n");

	CVertexGenerator::Get()->Initialise( );

	devLog.OutputString("CVertexGenerator::Get()->Initialise( );\n");

	pDev->ResetDevice(bStartFullscreen == false);
}

EngineWindow::~EngineWindow()
{
}

void EngineWindow::SetEngine(Engine* pengine) {
    debugf("EngineWindow: Setting engine");

    m_pengine = pengine;

    // Tell the engine we are the window
    GetEngine()->SetFocusWindow(this, m_pPreferredFullscreen->GetValue());

    //
    // These rects track the size of the window
    //
    m_prectValueScreen = new ModifiableRectValue(GetClientRect());
    m_prectValueRender = new ModifiableRectValue(Rect(0, 0, 640, 480));
    m_pwrapRectValueRender = new WrapRectValue(m_prectValueScreen);
    m_modeIndex = s_countModes;

    m_ptranslateTransform = new TranslateTransform2(m_ppointMouse);
    m_ptransformImageCursor = new TransformImage(Image::GetEmpty(), m_ptranslateTransform);
    m_pwrapImage = new WrapImage(Image::GetEmpty());
    m_pgroupImage = new GroupImage(CreateUndetectableImage(m_ptransformImageCursor), m_pwrapImage);
}

void EngineWindow::SetModeler(Modeler* pmodeler) {
    debugf("EngineWindow: Setting modeler");

    m_pmodeler = pmodeler;

    m_pfontFPS = GetModeler()->GetNameSpace("model")->FindFont("defaultFont");
}

void EngineWindow::StartClose()
{
    PostMessage(WM_CLOSE);
}

bool EngineWindow::IsValid()
{
    return m_pengine->IsValid();
}

void EngineWindow::OnClose()
{
    m_pcloseEventSource->Trigger();

    RemoveKeyboardInputFilter(m_pkeyboardInput);

    m_pgroupImage           = NULL;
    m_pwrapImage            = NULL;
    m_ptransformImageCursor = NULL;
    m_ptranslateTransform   = NULL;

    m_pmodeler->Terminate();
    m_pmodeler = NULL;

    m_pengine->TerminateEngine();

    Window::OnClose();
}

// 
bool g_bMDLLog    = false;
bool g_bWindowLog = false;

void EngineWindow::ParseCommandLine(const ZString& strCommandLine, bool& bStartFullscreen)
{
    PCC pcc = strCommandLine;
    CommandLineToken token(pcc, strCommandLine.GetLength());

    while (token.MoreTokens()) {
        ZString str;

        if (token.IsMinus(str)) {
            if (str == "windowed") {
                bStartFullscreen = false;
            } else if (str == "fullscreen") {
                bStartFullscreen = true;
			}
        } else {
            token.IsString(str);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// InitialiseTime()
//
//////////////////////////////////////////////////////////////////////////////
void EngineWindow::InitialiseTime()
{
    // get time
    TRef<INameSpace> pnsModel = m_pmodeler->GetNameSpace("model");
    CastTo(m_pnumberTime, pnsModel->FindMember("time"));
}

//////////////////////////////////////////////////////////////////////////////
//
// Private Attribute Functions
//
//////////////////////////////////////////////////////////////////////////////

void EngineWindow::UpdateSurfacePointer()
{
	//Imago 6/26/09

    CD3DDevice9 * pDev = CD3DDevice9::Get();

    if (!m_pengine->IsFullscreen()) 
	{
		WinPoint size = GetClientRect().Size();

		if (size.X() == 0) {
		    size.SetX(1);
		}

		if (size.Y() == 0) {
		    size.SetY(1);
		}

		m_psurface = m_pengine->CreateDummySurface( size, NULL );
		ZAssert(m_psurface != NULL && m_psurface->IsValid());
		if(pDev->IsDeviceValid()) {
			//imago 7/6/09
			SetWindowPos(GetHWND(), HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
			DDCall(pDev->ResetDevice(true,size.X(),size.Y()));
		}

	} else {
		WinPoint point = m_pengine->GetFullscreenSize();
		m_psurface = m_pengine->CreateDummySurface(point, NULL );
		ZAssert(m_psurface != NULL && m_psurface->IsValid());
		if(pDev->IsDeviceValid()) {
			//Imago 7/28/09
            SetWindowPos(GetHWND(),HWND_TOP,0,0,point.X(),point.Y(),0);
			DDCall(pDev->ResetDevice(false,point.X(),point.Y(),g_DX9Settings.m_refreshrate));
			if (pDev->GetDeviceSetupParams()->iAdapterID) {
				    ::ShowWindow(GetHWND(),SW_MINIMIZE);	 //imago 7/7/09 (workaround for one of the multimon bugs)
				    ::ShowWindow(GetHWND(),SW_MAXIMIZE);
			}
		}
	}
}

void EngineWindow::UpdateWindowStyle()
{
    if (m_pengine->IsFullscreen()) {
        SetHasMinimize(false);
        SetHasMaximize(false);
        SetHasSysMenu(false);
        Window::SetSizeable(false);
        SetTopMost(true);

        //
        // Size the window to cover the entire desktop
        // Win32 doesn't recognize the style change unless we resize the window
        // or use SetWindowPos

        WinRect rect = GetRect();

        SetClientRect(rect);
        SetPosition(WinPoint(0, 0));
    } else {
        WinPoint size = m_pengine->GetFullscreenSize();

        // get the currently selected monitor's resolution
        CD3DDevice9 * pDev = CD3DDevice9::Get();
        RECT rectWindow = pDev->GetDeviceSetupParams()->monitorInfo.rcMonitor;
        LONG screenWidth = rectWindow.right - rectWindow.left;
        LONG screenHeight = rectWindow.bottom - rectWindow.top;

        //windowed, but we do not fit with the selected resolution, switch to borderless
        bool bMakeBorderless = screenWidth <= size.X() && screenHeight <= size.Y();

        //set window properties
        m_bMovingWindow = true;

        SetHasMinimize(!bMakeBorderless);
        SetHasMaximize(!bMakeBorderless);
        SetHasSysMenu(!bMakeBorderless);

        Window::SetSizeable(!bMakeBorderless && m_bSizeable);

        //make sure we are on top of everything if we are borderless
        SetTopMost(bMakeBorderless);

        // Win32 doesn't recognize the style change unless we make a call to SetWindowPos
        SetPosition(m_offsetWindowed);
        SetClientSize(size);
        m_bMovingWindow = false;
    }

    //
    // Enable DInput if we are fullscreen
    //

    m_pmouse->SetEnabled(m_bActive && m_pengine->IsFullscreen());
}

void EngineWindow::UpdateRectValues()
{
    if (g_bWindowLog) {
        ZDebugOutput("EngineWindow::UpdateRectValues()\n");
    }

    //
    // The screen rect
    //

    if (m_pengine->IsFullscreen()) {
        WinRect 
            rect(
                WinPoint(0, 0),
                m_pengine->GetFullscreenSize()
            );

        if (g_bWindowLog) {
            ZDebugOutput("  Fullscreen: " + GetString(0, rect) + "\n");
        }

        m_prectValueScreen->SetValue(rect);
        m_pmouse->SetClipRect(rect);
    } else {
		WinRect rect(WinPoint(0, 0), m_pengine->GetFullscreenSize());
        if (g_bWindowLog) {
            ZDebugOutput("  Windowed: " + GetString(0, rect) + "\n");
        }

        m_prectValueScreen->SetValue(rect);
        m_pmouse->SetClipRect(rect);
    }

    //
    // The render rect
    //

    if (m_pengine->IsFullscreen() && m_modeIndex < s_countModes) {
        WinPoint size = GetFullscreenSize();

        float dx = ((float)size.X() - s_pmodes[m_modeIndex].m_size.X()) / 2.0f;
        float dy = ((float)size.Y() - s_pmodes[m_modeIndex].m_size.Y()) / 2.0f;

        m_prectValueRender->SetValue(
            Rect(
                dx,
                dy,
                (float)size.X() - dx,
                (float)size.Y() - dy
            )
        );

        m_pwrapRectValueRender->SetWrappedValue(m_prectValueRender);
    } else {
        m_pwrapRectValueRender->SetWrappedValue(m_prectValueScreen);
    }
}

void EngineWindow::SetMoveOnHide(bool bMoveOnHide)
{
    m_bMoveOnHide = bMoveOnHide;
}

void EngineWindow::UpdateCursor()
{
    bool bGameCursor;
    
    if (m_pengine->IsFullscreen()) {
        bGameCursor = true;
    } else if (m_bCaptured) {
        bGameCursor = m_prectValueScreen->GetValue().Inside(m_ppointMouse->GetValue());
    } else {
        bGameCursor = m_bMouseInside;
    }

    if (bGameCursor) {
        bool bTimeOut = 
               m_bHideCursor
            && m_timeCurrent - m_timeLastMouseMove > 2.0f;

        if (
               m_pimageCursor != NULL 
            && m_bShowCursor
            && (!bTimeOut)
        ) {
            m_ptransformImageCursor->SetImage(m_pimageCursor);
            s_cursorIsHidden = false;
        } else {
            /*
            if (m_bMoveOnHide) {
                HandleMouseMessage(0, Point(0, 0));
            }
            */
            m_ptransformImageCursor->SetImage(Image::GetEmpty());
            s_cursorIsHidden = true;
        }
        ShowMouse(false);
    } else {
        m_ptransformImageCursor->SetImage(Image::GetEmpty());
        ShowMouse(m_bShowCursor);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Public methods
//
//////////////////////////////////////////////////////////////////////////////

RectValue* EngineWindow::GetScreenRectValue()
{
    return m_prectValueScreen;
}

RectValue* EngineWindow::GetRenderRectValue()
{
    return m_pwrapRectValueRender;
}

void EngineWindow::SetFullscreen(bool bFullscreen)
{
    if (m_pPreferredFullscreen->GetValue() != bFullscreen) {
        m_pPreferredFullscreen->SetValue(bFullscreen);
    }

    if (m_pengine->IsFullscreen() != bFullscreen) {
        //
        // Make sure we don't recurse
        //

        m_bMovingWindow = true;

        //
        // Release the backbuffer
        //

//        m_psurface = NULL;

        //
        // Switch modes
        //

        m_pengine->SetFullscreen(bFullscreen);

        //
        // Tell the caption
        //

        if (m_pcaption) {
            m_pcaption->SetFullscreen(bFullscreen);
        }

        //
        // Enable DirectInput mouse?
        //

        m_pmouse->SetEnabled(m_pengine->IsFullscreen() && m_bActive);
        m_pmouse->SetPosition(m_prectValueScreen->GetValue().Center());

        //
        // Done, start listening to window sizing notifications
        //

        m_bMovingWindow = false;
    }
}

bool EngineWindow::OnWindowPosChanging(WINDOWPOS* pwp)
{	
	/*
	char szBuffer[256];
	sprintf( szBuffer, "ONWNDPOSCHANGE: X:%d  y:%d flags:%d,   StateMinimized:%d StateRestored:%d\n", pwp->x, pwp->y, pwp->flags,
		m_bWindowStateMinimised, m_bWindowStateRestored );
	OutputDebugString( szBuffer );
	*/

	//NYI TTHIS BREAKS MULTIMON 7/29/09

    if ((pwp->x != 0 && pwp->y !=0) && GetFullscreen()) { //imago fixed crash 7/6/09
        pwp->x = 0;
        pwp->y = 0;
    } 
	else 
	{
		// For some reason, when restoring a minimised window, it gets a position of
		// -32000, -32000.
        pwp->x = std::max( 0, pwp->x );
        pwp->y = std::max( 0, pwp->y );

        if (!m_bMovingWindow) 
		{
            return Window::OnWindowPosChanging(pwp);
        }
    }

    return true;
}

void EngineWindow::Invalidate()
{
    m_bInvalid = true;
}

void EngineWindow::RectChanged()
{
	ZDebugOutput("EngineWindow::RectChanged() moving="+ZString(m_bMovingWindow) + " WindowRect=" + GetRect().GetString() + " ClientRect=" + GetClientRect().GetString());
    if (
           (!m_bMovingWindow)
        && (m_pengine && !m_pengine->IsFullscreen())
    ) {
        WinPoint size = GetClientRect().Size();

        if (
               (size           != WinPoint(0, 0))
            && (m_pengine->GetFullscreenSize() != size          )
        ) {
            SetFullscreenSize(Vector(size.X(), size.Y(), g_DX9Settings.m_refreshrate));
            Invalidate();
        }

        if (m_offsetWindowed != GetRect().Min()) {
            m_offsetWindowed = GetRect().Min();
        }
    }
}

void EngineWindow::SetSizeable(bool bSizeable)
{
    if (m_bSizeable != bSizeable) {
        m_bSizeable = bSizeable;

        if (m_pitemHigherResolution) {
            m_pitemHigherResolution->SetEnabled(m_bSizeable);
            m_pitemLowerResolution->SetEnabled(m_bSizeable);
        }

        Invalidate();
    }
}

WinPoint EngineWindow::GetSize()
{
    if (m_pengine->IsFullscreen()) {
        return GetFullscreenSize();
    } else {
        return GetWindowedSize();
    }
}

WinPoint EngineWindow::GetWindowedSize()
{
    return m_pengine->GetFullscreenSize();
}

WinPoint EngineWindow::GetFullscreenSize()
{
    return m_pengine->GetFullscreenSize();
}

void EngineWindow::SetFullscreenSize(const Vector& size)
{
    m_pengine->SetFullscreenSize(size);

    RenderSizeChanged(true);
}

void EngineWindow::ChangeFullscreenSize(bool bLarger)
{
    if (m_bSizeable) 
	{
        WinPoint size = GetFullscreenSize();

        if (size == WinPoint(640, 480)) 
		{
            if (bLarger) 
			{
                if (m_modeIndex < s_countModes) 
				{
                    m_modeIndex++;
                } 
				else 
				{
                    m_pengine->ChangeFullscreenSize(bLarger);
                }
            } 
			else 
			{
                if (m_modeIndex > 0) 
				{
                    m_modeIndex--;
                }
            }
        } 
		else 
		{
            m_pengine->ChangeFullscreenSize(bLarger);
        }

        Invalidate();
		// Imago 6/29/09 

        RenderSizeChanged(	( size == WinPoint ( 640, 480 ) ) && 
							( m_modeIndex < s_countModes ) );
    }
}

void EngineWindow::SetImage(Image* pimage)
{
    m_pwrapImage->SetImage(pimage);
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

#define idmHigherResolution    1
#define idmLowerResolution     2
#define idmAllow3DAcceleration 3
#define idmAllowSecondary      4
#define idmBrightnessUp        5
#define idmBrightnessDown      6

TRef<IPopup> EngineWindow::GetEngineMenu(IEngineFont* pfont)
{
    TRef<IMenu> pmenu =
        CreateMenu(
            GetModeler(),
            pfont,
            m_pmenuCommandSink
        );

                                 pmenu->AddMenuItem(idmBrightnessUp       , "Brightness Up"                                   , 'U');
                                 pmenu->AddMenuItem(idmBrightnessDown     , "Brightness Down"                                 , 'D');
                                 pmenu->AddMenuItem(0                     , "------------------------------------------------"     );
    m_pitemHigherResolution    = pmenu->AddMenuItem(idmHigherResolution   , "Higher Resolution"                               , 'H');
    m_pitemLowerResolution     = pmenu->AddMenuItem(idmLowerResolution    , "Lower Resolution"                                , 'L');
								 pmenu->AddMenuItem(0                     , "------------------------------------------------"     );
                                 pmenu->AddMenuItem(0                     , "Current device state                            ", 'C');
                                 pmenu->AddMenuItem(0	                  , "------------------------------------------------"     );
    m_pitemDevice              = pmenu->AddMenuItem(0                     , GetDeviceString()                                      );
//    m_pitemRenderer            = pmenu->AddMenuItem(0                     , GetRendererString()                                    );
    m_pitemResolution          = pmenu->AddMenuItem(0                     , GetResolutionString()                                  );
    m_pitemRendering           = pmenu->AddMenuItem(0                     , GetRenderingString()                                   );
    m_pitemBPP                 = pmenu->AddMenuItem(0                     , GetPixelFormatString()                                 ); // KGJV 32B

    return pmenu;
}


ZString EngineWindow::GetResolutionString()
{
    Point size = GetScreenRectValue()->GetValue().Size();

    return "Resolution: " + ZString(size.X()) + " x " + ZString(size.Y());
}

ZString EngineWindow::GetRenderingString()
{
    Point size = GetRenderRectValue()->GetValue().Size();

    return "Rendering: " + ZString(size.X()) + " x " + ZString(size.Y());
}

// KGJV 32B
ZString EngineWindow::GetPixelFormatString()
{
    return GetEngine()->GetPixelFormatName();
}
ZString EngineWindow::GetRendererString()
{
    return 
          (
              (
                   m_psurface == NULL 
                || m_psurface->GetSurfaceType().Test(SurfaceTypeVideo())
               )
          )
        ? "Using hardware rendering"
        : "Using software rendering";
}

ZString EngineWindow::GetDeviceString()
{
    return "Device: " + GetEngine()->GetDeviceName();
}

void EngineWindow::UpdateMenuStrings()
{
    if (m_pitemDevice) {
        m_pitemDevice             ->SetString(GetDeviceString()             );
//        m_pitemRenderer           ->SetString(GetRendererString()           );
        m_pitemResolution         ->SetString(GetResolutionString()         );
        m_pitemRendering          ->SetString(GetRenderingString()          );
        m_pitemBPP                ->SetString(GetPixelFormatString()        );
    }
}

void EngineWindow::OnEngineWindowMenuCommand(IMenuItem* pitem)
{
    switch (pitem->GetID()) 
	{
		// DISABLE THE higher/lower resolution option - to be reinstated at some point.
		//Imago reinstated 6/26/09
		case idmHigherResolution:
            ChangeFullscreenSize(true);
            break;

        case idmLowerResolution:
            ChangeFullscreenSize(false);
            break;
    }
}

ZString EngineWindow::GetFPSString(float fps, float mspf, Context* pcontext)
{
    return ZString();
}

void EngineWindow::UpdatePerformanceCounters(Context* pcontext, Time timeCurrent)
{
    m_frameTotal++;
    if (m_bFPS) {
        if (m_frameCount == -1) {
//            pcontext->ResetPerformanceCounters();
			CD3DDevice9::Get()->ResetPerformanceCounters();
            m_frameCount    = 0;
            m_timeLastFrame = timeCurrent;
        }

        m_frameCount++;
		if (m_timeCurrent - m_timeLastFrame > 1.0) 
		{
			float fOneOverFrameCount = 1.0f / m_frameCount;
			float fNumPrims = (float)CD3DDevice9::Get()->GetPerformanceCounter( CD3DDevice9::eD9S_NumPrimsRendered );
			float fPrimsPerFrame = fNumPrims * fOneOverFrameCount;
			float fPrimsPerSecond = fNumPrims / (timeCurrent - m_timeLastFrame);
			float fDPsPerFrame = (float) CD3DDevice9::Get()->GetPerformanceCounter( CD3DDevice9::eD9S_NumDrawPrims );
			float fStateChangesPerFrame = (float) CD3DDevice9::Get()->GetPerformanceCounter( CD3DDevice9::eD9S_NumStateChanges );
			float fTextureChangesPerFrame = (float) CD3DDevice9::Get()->GetPerformanceCounter( CD3DDevice9::eD9S_NumTextureChanges );
			float fShaderChangesPerFrame = (float) CD3DDevice9::Get()->GetPerformanceCounter( CD3DDevice9::eD9S_NumShaderChanges );
			fDPsPerFrame *= fOneOverFrameCount;
			fStateChangesPerFrame *= fOneOverFrameCount;
			fTextureChangesPerFrame *= fOneOverFrameCount;
			fShaderChangesPerFrame *= fOneOverFrameCount;

			double fps  = m_frameCount / (timeCurrent - m_timeLastFrame);
			double mspf = 1000.0 * (timeCurrent - m_timeLastFrame) / m_frameCount;

			if (m_indexFPS == 0) 
			{
				// Old performance string:
				// mspf, fps, tmem/maxtmem, vmem/maxvmem, tris/frame, tris/sec, points/frame, gamma
				//
				// New performance string:
				// fps, available tmem, num prims, num dp calls,
				// num state changes, num tex changes, num shader changes.

				m_strPerformance1 =
#ifdef ICAP
					ZString("ICAP: ")
#elif defined(_DEBUG)
					ZString("Debug: ")
#else
					ZString("Retail: ")
#endif
//					+ ZString(CD3DDevice9::GetDeviceSetupString())
					+ " mspf: " + ZString(MakeInt(mspf))
					+ " fps: " + ZString(MakeInt(fps))
//					+ " tmem: (" + ZString(CD3DDevice9::GetPerformanceCounter( CD3DDevice9::eD9S_CurrentTexMem ) )
//					+ "of " + ZString(CD3DDevice9::GetPerformanceCounter( CD3DDevice9::eD9S_InitialTexMem ) )
//					+ ") ppf: " + ZString(MakeInt(fPrimsPerFrame))
					+ " ppf: " + ZString(MakeInt(fPrimsPerFrame))
					+ " pps: " + ZString(MakeInt(fPrimsPerSecond))
					+ " #dp: " + ZString(MakeInt(fDPsPerFrame))
					+ " #sc: " + ZString(MakeInt(fStateChangesPerFrame))
					+ " #tx: " + ZString(MakeInt(fTextureChangesPerFrame))
					+ " #shc: " + ZString(MakeInt(fShaderChangesPerFrame));
				
				m_strPerformance2 = GetFPSString((float)fps, (float)mspf, pcontext);
			} 
			else 
			{
				m_strPerformance1 = ZString(MakeInt(mspf)) + "/" + ZString(MakeInt(fps));
			}

            m_frameCount    = 0;
            m_timeLastFrame = timeCurrent;
			CD3DDevice9::Get()->ResetPerformanceCounters();
        }
    }
}

void EngineWindow::OutputPerformanceCounters()
{
   if (m_bFPS) {
        ZString strOut = m_strPerformance1 + " " + m_strPerformance2 + "\r\n";
        OutputDebugStringA(strOut);
    }
}

void EngineWindow::RenderPerformanceCounters(Surface* psurface)
{
    if (m_bFPS) {
		
		// Disable AA.
		CD3DDevice9::Get()->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE );

        int ysize = m_pfontFPS->GetHeight();
        Color color(1, 0, 0);

        psurface->DrawString(m_pfontFPS, color, WinPoint(1, 1 + 0 * ysize), m_strPerformance1);

        if (m_indexFPS == 0) {
            psurface->DrawString(m_pfontFPS, color, WinPoint(1, 1 + 1 * ysize), m_strPerformance2);
            psurface->DrawString(m_pfontFPS, color, WinPoint(1, 1 + 2 * ysize), "Frame " + ZString(m_frameTotal));
        }
		
		// Reenable AA.
		CD3DDevice9::Get()->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
	}

    #ifdef ICAP
        if (IsProfiling()) {
            psurface->DrawString(m_pfontFPS, Color::White(), WinPoint(1, 1), "Profiling");
        }
    #endif
}

void EngineWindow::SetHideCursorTimer(bool bHideCursor)
{
    m_bHideCursor = bHideCursor;
}

void EngineWindow::UpdateFrame()
{
    m_timeCurrent = Time::Now();
    if (m_pnumberTime) {
        m_pnumberTime->SetValue(m_timeCurrent - m_timeStart);
    }

    m_pevaluateFrameEventSource->Trigger(m_timeCurrent);

    m_pgroupImage->Update();
}


// 205 - main background texture index.
static int iDebugTexCount = 1;

bool EngineWindow::RenderFrame()
{
    PrivateEngine* pengine; CastTo(pengine, m_pengine);
    TRef<Surface> psurface;

	HRESULT hr = CD3DDevice9::Get()->BeginScene();
    ZAssert( hr == D3D_OK );
    ZAssert( m_psurface != NULL );

	TRef<Context> pcontext = m_psurface->GetContext();
	if( pcontext ) 
	{
//		pcontext->BeginRendering();

		ZStartTrace("---------Begin Frame---------------");

		const Rect& rect = m_pwrapRectValueRender->GetValue();
		pcontext->Clip(rect);

        pcontext->SetBlendMode(BlendModeSourceAlpha);

		m_pgroupImage->Render( pcontext );
		UpdatePerformanceCounters(pcontext, m_timeCurrent);
		m_psurface->ReleaseContext(pcontext);
		RenderPerformanceCounters(m_psurface);
	}
	hr = CD3DDevice9::Get()->EndScene();

	if( hr == D3D_OK )
	{
		return true;
	}
    return false;
}

void EngineWindow::OnPaint(HDC hdc, const WinRect& rect)
{
    /* !!!
    if (m_psurface) {
        UpdateFrame();
        if (RenderFrame()) {
            //hdc = GetDC();
            m_psurface->BitBltToDC(hdc);
            //ReleaseDC(hdc);
            return;
        }
    }
    */

 //   ZVerify(::FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH)));
}

bool EngineWindow::ShouldDrawFrame()
{
    if (m_pengine->IsFullscreen()) {
        return true;
    } else {
        return !m_bMinimized;
    }
}

void EngineWindow::DoIdle()
{
    //
    // Update the input values
    //

    UpdateInput();

    //
    // Switch fullscreen state if requested
    //
    m_pConfigurationUpdater->Update();

    m_pConfiguration->Update();

	//Imago 7/10 #37 - Added a "clicker breaker outter", a dirty trick to get Win 5+ to give up the mouse?
    if (m_bRestore || (m_bWindowStateMinimised && !m_bClickBreak && m_pengine->IsFullscreen())) {
        
		if (!m_bWindowStateMinimised) {
			m_bRestore = false;
			SetFullscreen(false);
			::SetCursorPos(0,0);
		}
		INPUT Inputs[2];
		ZeroMemory(Inputs,sizeof(INPUT)*2);
		Inputs[0].type = INPUT_MOUSE;
		Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		Inputs[1] = Inputs[0];
		Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(2,Inputs,sizeof(INPUT));
		m_bClickBreak = true;
    }
  
    //
    // Is the device ready
    //

    bool bChanges = false;
    if (m_pengine && m_pengine->IsDeviceReady(bChanges))
	{
        if (bChanges || m_bInvalid) 
		{
			m_bInvalid = false;

			UpdateWindowStyle();
			UpdateRectValues();
			UpdateMenuStrings();
			UpdateSurfacePointer();
        }

        //
        // Evaluation
        //

        UpdateFrame();

        //
        // Rendering
        if( ShouldDrawFrame() ) 
		{
			CD3DDevice9::Get()->ClearScreen( );

			UpdateCursor();

			// Render, and if successful, display.
			if( RenderFrame() ) 
			{
				CD3DDevice9::Get()->RenderFinished( );
				return;
			}
        }
    } else {
        //
        // We don't have access to the device just update the frame
        //

        UpdateFrame();
    }

    //
    // we are flipping, but
    //      we are minimized
    //      we couldn't get the surface
    //   or we couldn't get the context
    // so sleep for a while so we don't eat up too much processor time
    //

	if (m_bMinimized && !bChanges && !m_bInvalid) //Imago doubled when we we know we're minimized 7/13/09
    	::Sleep(60);
	else
		::Sleep(30);

}

void EngineWindow::SetShowFPS(bool bFPS, const char* pszLabel)
{
    if (pszLabel)
        strncpy_s(m_pszLabel, 20, pszLabel, sizeof(m_pszLabel) - 1);
    else
        m_pszLabel[0] = '\0';

    m_bFPS       = bFPS;
    m_indexFPS   = 0;
    m_frameCount = -1;
}

void EngineWindow::ToggleShowFPS()
{
    if (m_bFPS) {
        if (m_indexFPS == 0) {
            m_indexFPS = 1;
        } else {
            m_bFPS     = false;
            m_indexFPS = 0;
        }
    } else {
        m_bFPS = true;
    }
}

bool EngineWindow::OnActivate(UINT nState, bool bMinimized)
{
    m_bMinimized = bMinimized;
    return Window::OnActivate(nState, bMinimized);
}

bool EngineWindow::OnActivateApp(bool bActive)
{
    if (g_bWindowLog) {
        ZDebugOutput(
              ZString("OnActivateApp: was ")
            + (m_bActive ? "active" : "inactive")
            + ", becoming "
            + (bActive ? "active" : "inactive")
            + "\n"
        );
    }

    if (m_bActive != bActive) {
        m_bActive = bActive;
        m_pinputEngine->SetFocus(m_bActive);
		
		if( m_bActive == true )
		{
			SetActiveWindow( GetHWND() );
        }
    }

    if (g_bWindowLog) {
        ZDebugOutput("OnActivateApp exiting\n");
    }

    return Window::OnActivateApp(bActive);
}

void EngineWindow::SetCursorImage(Image* pimage)
{
    if (m_pimageCursor != pimage) {
        m_pimageCursor = pimage;
    }
}

Image* EngineWindow::GetCursorImage(void) const
{
    return m_pimageCursor;
}

bool EngineWindow::OnSysCommand(UINT uCmdType, const WinPoint &point)
{
    switch (uCmdType) {
        case SC_KEYMENU:
            //
            // don't let the ALT key open the system menu
            //
            return true;

        case SC_MAXIMIZE:
			m_bWindowStateMinimised = false;
			m_bWindowStateRestored = false;
            SetFullscreen(true);
			m_bInvalid = true; //imago 7/6/09
			m_bMovingWindow = true; //Imago 7/10 
            return true;

		case SC_MINIMIZE:
			m_bWindowStateMinimised = true;
			m_bInvalid = true;
			m_bMovingWindow = true;
			m_bClickBreak = false; //Imago 7/10 #37
			break;

		case SC_RESTORE:
			if( m_bWindowStateMinimised )
			{
				m_bWindowStateMinimised = false;
				m_bWindowStateRestored = true;
				m_bInvalid = true; //imago 7/6/09
				m_bMovingWindow = true;
			}
			break;

		case SC_MOVE: //Imago 7/10
			if (GetAsyncKeyState(VK_LBUTTON) < 0) {
				m_bMovingWindow = true;
			}
			break;

        case SC_CLOSE:
            StartClose();
            return true;

		default:
			m_bMovingWindow = false;
			break;
    }

    return Window::OnSysCommand(uCmdType, point);
}

//////////////////////////////////////////////////////////////////////////////
//
// IInputProvider
//
//////////////////////////////////////////////////////////////////////////////

bool EngineWindow::IsDoubleClick()
{
	// BT - 8/17 - Double click fix. #234 - [url="https://github.com/kgersen/Allegiance/issues/234"]https://github.com/kgersen/Allegiance/issues/234[/url]
	return (fabs(fabs(m_timeCurrent.clock()) - fabs(m_timeLastClick.clock())) < 250);

	// return (m_timeCurrent < (m_timeLastClick + 0.25f));
}

void EngineWindow::SetCursorPos(const Point& point)
{
    if (m_pmouse->IsEnabled()) {
        m_pmouse->SetPosition(point);
        //HandleMouseMessage(WM_MOUSEMOVE, point);
    } else {
        //If disabled, send a new mouse position to the window.
        Window::SetCursorPos(point);
    }
}

void EngineWindow::ShowCursor(bool bShow)
{
    m_bShowCursor = bShow;
}

//////////////////////////////////////////////////////////////////////////////
//
// Mouse stuff
//
//////////////////////////////////////////////////////////////////////////////

int     EngineWindow::s_forceHitTestCount = 0;
bool    EngineWindow::s_cursorIsHidden = false;

void EngineWindow::DoHitTest()
{
    if (!s_cursorIsHidden)
    {
        Window::DoHitTest();
        s_forceHitTestCount = 2;
    }
}

void EngineWindow::HandleMouseMessage(UINT message, const Point& point, UINT nFlags)
{
    if (m_pgroupImage != NULL) {
        //
        // Make sure these objects don't go away until we are done
        //

        TRef<Image>  pimage = m_pgroupImage;
        TRef<Window> pthis  = this;

        //
        // Handle mouse move messages
        //

        switch (message) {
            case WM_MOUSEHOVER:
			case WM_NCMOUSEHOVER: //Imago 7/10
            case 0: // 0 == WM_MOUSEENTER
                //pimage->MouseEnter(this, point);
                m_bMouseInside = true;
                break;

            case WM_MOUSELEAVE:
			case WM_NCMOUSELEAVE: //Imago 7/10
                //pimage->MouseLeave(this);
                m_bMouseInside = false;
                break;

            case WM_MOUSEMOVE:
                m_timeLastMouseMove = m_timeCurrent;
                m_timeLastClick     = 0;
                m_ppointMouse->SetValue(point);

                if (m_pengine->IsFullscreen()) {
                    m_ptransformImageCursor->SetImage(m_pimageCursor ? (Image*)m_pimageCursor : Image::GetEmpty());
                }
                break;
        }
    
        //
        // Is the mouse hitting the image?
        //

        MouseResult mouseResult;

            while (true) {
                mouseResult = pimage->HitTest(this, point, m_bCaptured);

                if (!mouseResult.Test(MouseResultRelease())) {
                    break;
                } 

                pimage->RemoveCapture();
                ReleaseMouse();
                m_bCaptured = false;
            }

        bool bHit = m_bMouseInside && mouseResult.Test(MouseResultHit());

        //
        // Call MouseMove, MouseLeave or MouseEnter
        //

        if (m_bHit != bHit) {
            if (m_bHit) {
                pimage->MouseLeave(this);
            }

            m_bHit = bHit;

            if (m_bHit) {
                pimage->MouseEnter(this, point);
            }
        } else if (m_bHit) {
            pimage->MouseMove(this, point, false, true);
        }


        //
        // Handle button messages
        //

        switch (message) {
            case WM_LBUTTONDOWN: 
                mouseResult = pimage->Button(this, point, 0, m_bCaptured, m_bHit, true );
                m_timeLastClick = m_timeCurrent;
                break;

            case WM_LBUTTONUP:   
                mouseResult = pimage->Button(this, point, 0, m_bCaptured, m_bHit, false);
                break;

            case WM_RBUTTONDOWN: 
                mouseResult = pimage->Button(this, point, 1, m_bCaptured, m_bHit, true );
                break;

            case WM_RBUTTONUP:   
                mouseResult = pimage->Button(this, point, 1, m_bCaptured, m_bHit, false);
                break;

            case WM_MBUTTONDOWN: 
                mouseResult = pimage->Button(this, point, 2, m_bCaptured, m_bHit, true );
                break;

            case WM_MBUTTONUP:   
                mouseResult = pimage->Button(this, point, 2, m_bCaptured, m_bHit, false);
                break;

            case WM_MOUSEWHEEL:  //imago 8/13/09
                if (nFlags >2) {
                    if (GET_WHEEL_DELTA_WPARAM(nFlags) < 0) {
                        mouseResult = pimage->Button(this,point, 8, m_bCaptured, m_bHit, true );
                        if (!m_pmouse->IsEnabled())
                            mouseResult = pimage->Button(this,point, 8, m_bCaptured, m_bHit, false );
                    } else {
                        mouseResult = pimage->Button(this, point, 9, m_bCaptured, m_bHit, true );
                        if (!m_pmouse->IsEnabled())
                            mouseResult = pimage->Button(this, point, 9, m_bCaptured, m_bHit, false );
                    }
                } else if (nFlags == 1) {
                    mouseResult = pimage->Button(this,point, 8, m_bCaptured, m_bHit, false );
                } else if (nFlags == 0) {
                    mouseResult = pimage->Button(this,point, 9, m_bCaptured, m_bHit, false );
                }
                break;

		    case WM_XBUTTONDOWN: //imago 8/15/09
                ZDebugOutput("WM_XBUTTONDOWN: " + ZString(2+GET_XBUTTON_WPARAM(nFlags)) + "\n");
                mouseResult = pimage->Button(this, point, 2+GET_XBUTTON_WPARAM(nFlags), m_bCaptured, m_bHit, true );
                break;

		    case WM_XBUTTONUP:
                ZDebugOutput("WM_XBUTTONUP: " + ZString(2+GET_XBUTTON_WPARAM(nFlags)) + "\n");
                mouseResult = pimage->Button(this, point, 2+GET_XBUTTON_WPARAM(nFlags), m_bCaptured, m_bHit, false );
                break;
        }

        if (mouseResult.Test(MouseResultRelease())) {
            pimage->RemoveCapture();
            ReleaseMouse();
            m_bCaptured = false;
        } else if (mouseResult.Test(MouseResultCapture())) {
            CaptureMouse();
            m_bCaptured = true;
        }

    }
}

bool EngineWindow::OnMouseMessage(UINT message, UINT nFlags, const WinPoint& point)
{
    if (!m_pmouse->IsEnabled()) {
        //we are not ignoring window mouse events
        HandleMouseMessage(message, Point::Cast(point), nFlags);
    }
    
    return true;
}

bool EngineWindow::OnEvent(ButtonEvent::Source* pevent, ButtonEventData be)
{
    //
    // button state change
    //

    if (be.GetButton() == 0) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_LBUTTONDOWN, m_pmouse->GetPosition());
        } else {
            HandleMouseMessage(WM_LBUTTONUP,   m_pmouse->GetPosition());
        }
    } else if (be.GetButton() == 1) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_RBUTTONDOWN, m_pmouse->GetPosition());
        } else {
            HandleMouseMessage(WM_RBUTTONUP,   m_pmouse->GetPosition());
        }
    } else if (be.GetButton() == 2) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_MBUTTONDOWN, m_pmouse->GetPosition());
        } else {
            HandleMouseMessage(WM_MBUTTONUP,   m_pmouse->GetPosition());
        }

    //Imago 8/15/09
    } else if (be.GetButton() == 3) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_XBUTTONDOWN, m_pmouse->GetPosition(), MAKEWPARAM(0,1));
        } else {
            HandleMouseMessage(WM_XBUTTONUP,   m_pmouse->GetPosition(), MAKEWPARAM(0,1));
        }
    } else if (be.GetButton() == 4) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_XBUTTONDOWN, m_pmouse->GetPosition(), MAKEWPARAM(0,2));
        } else {
            HandleMouseMessage(WM_XBUTTONUP,   m_pmouse->GetPosition(), MAKEWPARAM(0,2));
        }
    } else if (be.GetButton() == 5) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_XBUTTONDOWN, m_pmouse->GetPosition(), MAKEWPARAM(0,3));
        } else {
            HandleMouseMessage(WM_XBUTTONUP,   m_pmouse->GetPosition(), MAKEWPARAM(0,3));
        }
    } else if (be.GetButton() == 6) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_XBUTTONDOWN, m_pmouse->GetPosition(), MAKEWPARAM(0,4));
        } else {
            HandleMouseMessage(WM_XBUTTONUP,   m_pmouse->GetPosition(), MAKEWPARAM(0,4));
        }
    } else if (be.GetButton() == 7) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_XBUTTONDOWN, m_pmouse->GetPosition(), MAKEWPARAM(0,5));
        } else {
            HandleMouseMessage(WM_XBUTTONUP,   m_pmouse->GetPosition(), MAKEWPARAM(0,5));
        }

    } else if (be.GetButton() == 8) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_MOUSEWHEEL, m_pmouse->GetPosition(), -WHEEL_DELTA);
        } else {
            HandleMouseMessage(WM_MOUSEWHEEL, m_pmouse->GetPosition(), 1);
        }
    } else if (be.GetButton() == 9) {
        if (be.IsDown()) {
            HandleMouseMessage(WM_MOUSEWHEEL, m_pmouse->GetPosition(), WHEEL_DELTA);
        } else {
            HandleMouseMessage(WM_MOUSEWHEEL, m_pmouse->GetPosition(), 0);
        }
    }

    return true;
}

void EngineWindow::UpdateInput()
{
    m_pinputEngine->Update();

    //
    // Update the mouse position
    //

    if (m_pmouse->IsEnabled()) {
        // we have to manually fire mouse move events
        if (m_ppointMouse->GetValue() != m_pmouse->GetPosition() || (s_forceHitTestCount >> 0)) {
            if (s_forceHitTestCount > 0) {
                s_forceHitTestCount--;
            }
            HandleMouseMessage(WM_MOUSEMOVE, m_pmouse->GetPosition());
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// IScreenSiteMethods
//
//////////////////////////////////////////////////////////////////////////////

void EngineWindow::SetCaption(ICaption* pcaption)
{
    m_pcaption = pcaption;
    if (m_pcaption) {
        m_pcaption->SetFullscreen(GetFullscreen());
    }
}

void EngineWindow::OnCaptionMinimize()
{
    PostMessage(WM_SYSCOMMAND, SC_MINIMIZE);
}

void EngineWindow::OnCaptionMaximize()
{
    PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE);
}

void EngineWindow::OnCaptionFullscreen()
{
    SetFullscreen(true);
}

void EngineWindow::OnCaptionRestore()
{
    if (GetFullscreen()) {
        m_bRestore = true;
    } else {
        PostMessage(WM_SYSCOMMAND, SC_RESTORE);
    }
}

void EngineWindow::OnCaptionClose()
{
    StartClose();
}
