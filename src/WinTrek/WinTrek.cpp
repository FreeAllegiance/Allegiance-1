#include "pch.h"

#include <shellapi.h>
#include "cmdview.h"
//#include "console.h"
#include "trekctrls.h"

#include "gamesite.h"

#include "badwords.h"
#include "slideshow.h"
#include "Training.h"
#include "CommandAcknowledgedCondition.h"
#include "SteamClans.h" // BT - STEAM

#include <Delayimp.h>   // For error handling & advanced features
//#include "..\\icqapi\\ICQAPIInterface.h"

class QuickChatNode : public IMDLObject {};

#include "quickchat.h"
#include <EngineSettings.h>
#include <paneimage.h>
#include <D3DDevice9.h>

#include "FileLoader.h"

#include "Configuration.h"

// Tell the linker that my DLL should be delay loaded
//#pragma comment(linker, "/DelayLoad:icqmapi.dll")

const float AnyViewMinRenderScreenSize = 1.0f;

const float c_dtFlashOn = 0.25f;
const float c_dtFlashOff = 0.25f;
const float c_dtFlashingDuration = 2.0f;

const int c_nCountdownMax = 1000000; // just a big number
const float c_nMinGain = -60;
const float c_nMinFFGain = 0; //Imago

// -Imago: manual AFK toggle flags for auto-AFK
extern bool g_bActivity = true;
extern bool g_bAFKToggled = false;




//////////////////////////////////////////////////////////////////////////////
//
// Stuff that was moved out of this file
//
//////////////////////////////////////////////////////////////////////////////

namespace SoundInit {
    void InitializeSoundTemplates(
        Modeler* pmodeler,
        TVector<TRef<ISoundTemplate> >& vSoundMap
    );
    void AddMembers(INameSpace* pns);
}

//////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//
//////////////////////////////////////////////////////////////////////////////

TRef<ZWriteFile> g_pzfFrameDump = NULL;
FrameDumpInfo    g_fdInfo;

const float s_fCommandViewDistanceMax = 20000.0f;
const float s_fCommandViewDistanceMin = 50.0f;
const float s_fCommandViewDistanceDefault = 6000.0f;
const float c_fCommandGridRadius = 7500.0f;

const float s_fCommandViewDisplacementMax = 10000.0f;

const float s_fExternalViewDistanceMax = 6000.0f;
const float s_fExternalViewDistanceMin = 100.0f;
const float s_fExteralViewDistanceDefault = 500.0f;

const float c_dtRejectQueuedCommand = 15.0f;

const float c_fVolumeDelta = 1;
const float c_fFFGainDelta = 100; //Imago 7/10 #187
const float c_fMouseSensDelta = 0.01f; //Imago 7/10 #187

const float g_hudBright = 0.85f;

//const float g_fJoystickDeadZoneNone = 0.0f; //imago added 7/1/09 removed 7/23/09 causes crashes /w joysticks
const float g_fJoystickDeadZoneSmallest = 0.04f; //imago added 7/13/09
const float g_fJoystickDeadZoneSmall = 0.1f;
const float g_fJoystickDeadZoneLarge = 0.3f;


float g_fJoystickDeadZone = g_fJoystickDeadZoneSmall;
float g_fInverseJoystickDeadZone = g_fJoystickDeadZone - 1.0f;

//////////////////////////////////////////////////////////////////////////////
//
// Joystick Helpers
//
//////////////////////////////////////////////////////////////////////////////

static const unsigned char buttonDown = 0x01;
static const unsigned char buttonChanged = 0x80;

struct  JoystickResults
{
    ControlData     controls;
    unsigned char   button1;
    unsigned char   button2;
    unsigned char   button3;
    unsigned char   button4;
    unsigned char   button5;
    unsigned char   button6;
    float           hat;  // low 7 bits: 0=center, 1-8=direction, high bit: 1=hat position changed
};

float   GetThrottle(ImodelIGC*  pmodel)
{
    assert (pmodel);
    if (trekClient.GetShip()->GetParentShip() == NULL)
    {
        float speed      = pmodel->GetVelocity().Length();
        float myMaxSpeed = trekClient.GetShip()->GetHullType()->GetMaxSpeed();

        return (speed >= myMaxSpeed)
               ? 1.0f
               : ((2.0f * speed / myMaxSpeed) - 1.0f);
    }
    else
        return 0.0f;
}

//////////////////////////////////////////////////////////////////////////////
//
// Misc Helpers
//
//////////////////////////////////////////////////////////////////////////////

//Imago 7/29/09
DWORD WINAPI DDVidCreateThreadProc( LPVOID param ) {
	
	//windowed 7/10 #112
	PlayVideoInfo * pData = (PlayVideoInfo*)param;
	DDVideo *DDVid = new DDVideo();
	bool bOk = true;
	bool bHide = false;
	HWND hwndFound = NULL;
	if (pData->bWindowed) {
		hwndFound=FindWindow(NULL, TrekWindow::GetWindowTitle()); // BT - 9/17 - Updated to dynamic value.
	} else {
		//this window will have our "intro" in it...
		hwndFound = ::CreateWindow("MS_ZLib_Window", "Intro", WS_VISIBLE|WS_POPUP, 0, 0,
			GetSystemMetrics(SM_CXFULLSCREEN),GetSystemMetrics(SM_CYFULLSCREEN),NULL, NULL,
			::GetModuleHandle(NULL), NULL);
		bHide = true;
	}
	
	DDVid->m_hWnd = hwndFound;	

	if( SUCCEEDED( DDVid->Play(pData->pathStr,pData->bWindowed))) //(WMV2 is good as most machines read it)
    {
		::ShowCursor(FALSE);
		
		while (DDVid->m_Running && bOk) //we can now do other stuff while playing
		{
			if (!DDVid->m_pVideo->IsPlaying() || GetAsyncKeyState(VK_ESCAPE) ||
				GetAsyncKeyState(VK_SPACE) || GetAsyncKeyState(VK_RETURN) ||
				GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_RBUTTON))
			{
				DDVid->m_Running = FALSE;
				DDVid->m_pVideo->Stop();
			}
			else {
				DDVid->m_pVideo->Draw(DDVid->m_lpDDSBack);
				if (pData->bWindowed) {
					bOk = DDVid->Flip(); //windowed #112 Imagooooo
				}
				else {
					DDVid->m_lpDDSPrimary->Flip(0, DDFLIP_WAIT);
				}
			}
		}


		::ShowCursor(TRUE);
		DDVid->DestroyDDVid();
	} else {
		DDVid->DestroyDirectDraw();
	}

	delete pData;

	if (bHide)
		::DestroyWindow(hwndFound);


	return 0;
}
//

TRef<IMessageBox> CreateMessageBox(
    const ZString& str,
    ButtonPane*    pbuttonIn,
    bool           fAddDefaultButton,
    bool           fAddCancelButton,
    float          paintDelay
) {
    TRef<ButtonPane> pbutton = pbuttonIn;

    debugf("Creating message box with text \"%s\"\n", (const char*)str);

    if (fAddDefaultButton) {
        assert(pbutton == NULL);

        TRef<Surface> psurfaceButton = GetModeler()->LoadImage("btnokbmp", true)->GetSurface();

        pbutton =
            CreateTrekButton(
                CreateButtonFacePane(psurfaceButton, ButtonNormal, 0, psurfaceButton->GetSize().X()),
                false,
                positiveButtonClickSound
            );
    }

    TRef<ButtonPane> pbuttonCancel;

    if (fAddCancelButton) {
        TRef<Surface> psurfaceButton = GetModeler()->LoadImage("btnabortbmp", true)->GetSurface();

        pbuttonCancel =
            CreateTrekButton(
                CreateButtonFacePane(psurfaceButton, ButtonNormal, 0, psurfaceButton->GetSize().X()),
                false,
                negativeButtonClickSound
            );
    }

    return CreateMessageBox(GetWindow(), GetModeler(), str, pbutton, false, pbuttonCancel, paintDelay);
}

ImodelIGC*  GetCurrentTarget(void)
{
    ImodelIGC*  pmodelTarget = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);

    if (pmodelTarget != NULL)
    {
        if (pmodelTarget->GetObjectType() == OT_ship)
            pmodelTarget = ((IshipIGC*)pmodelTarget)->GetSourceShip();

        if ((pmodelTarget == trekClient.GetShip()->GetSourceShip()) ||
            (pmodelTarget->GetHitTest() == NULL) ||
            (pmodelTarget->GetCluster() != trekClient.GetCluster()) ||
            !trekClient.GetShip()->CanSee(pmodelTarget))
        {
            pmodelTarget = NULL;
        }
    }

    return pmodelTarget;
}

int   GetSimilarTargetMask(ImodelIGC* pmodel)
{
    assert (pmodel);

    ObjectType  type = pmodel->GetObjectType();

    int tt = GetTypebits(type);
    if ((type == OT_ship) || (type == OT_station) || (type == OT_probe))
		tt |= ((trekClient.GetShip()->GetSide() == pmodel->GetSide()) || IsideIGC::AlliedSides(trekClient.GetShip()->GetSide(), pmodel->GetSide())) // #ALLY -was: == imago fixed 7/8/09
              ? c_ttFriendly
              : c_ttEnemy;
    else
        tt |= c_ttAllSides;

    return tt;
}

//This class wraps all the functionality for controlling a camera'a position, orientation, field of view & orientation
class   CameraControl
{
    public:
        CameraControl(void)
        :
            m_dtAnimate(0.0f),
            m_jiggle(0.0f),
            m_fov(s_fDefaultFOV),
            m_oldHeadYaw(0.0f)
        {
            m_orientationHead.Reset();
        }

        ~CameraControl(void)
        {
        }

        void    SetHeadOrientation(float    yaw)
        {
            if (yaw != m_oldHeadYaw)
            {
                m_oldHeadYaw = yaw;
                m_orientationHead.Reset();
                m_orientationHead.Yaw(-yaw);
            }
        }

        const Orientation&  GetHeadOrientation(void) const
        {
            return m_orientationHead;
        }

        void SetCameras(Camera* pcamera, Camera* pcameraPosters/*, Camera* pcameraTurret*/)
        {
            m_pcamera                = pcamera;
            m_pcameraPosters         = pcameraPosters;
            //m_pcameraTurret          = pcameraTurret;

            SetZClip(5.0f, 10000.0f);

            m_pcameraPosters->SetZClip(10, 10000);
            //m_pcameraTurret->SetZClip(0.5, 10000);

            m_pcameraTarget->SetZClip(1, 20);
            m_pcamera->SetFOV(s_fDefaultFOV);
            m_pcameraTarget->SetPosition(Vector(0, 0, 8.0f));
        }

        void    SetZClip(float  zMin, float zMax)
        {
            m_pcamera->SetZClip(zMin, zMax);
        }

        bool    Apply(float dt, bool bJiggle, bool bTarget)
        {
            bool    animatingF = false;
            if (m_dtAnimate > 0.0f)
            {
                float   tNext = m_tnowAnimate + dt;
                if (tNext >= m_dtAnimate)
                {
                    //but the animation has finished
                    if (m_bAnimatePosition)
                        m_position = m_position2;
                    m_orientation = m_quaternion2;
                    m_fov = m_fov2;

                    m_dtAnimate = 0.0f;
                }
                else
                {
                    float   interpolate = 0.5f - 0.5f * cos(pi * (tNext / m_dtAnimate));

                    if (m_bAnimatePosition)
                        m_position = (m_position1 * (1.0f - interpolate)) +
                                     (m_position2 * interpolate);
                    m_orientation = Slerp(m_quaternion1, m_quaternion2, interpolate);
                    m_fov = (m_fov1 * (1.0f - interpolate)) +
                            (m_fov2 * interpolate);

                    m_tnowAnimate = tNext;

                    animatingF = true;
                }
            }

            m_pcamera->SetPosition(m_position);

            //
            // Jiggle the camera orientation based on the amount of afterburner and any residual effects
            //
            const double jiggleHalfLife = 0.25f;
            m_jiggle *= (float)pow(jiggleHalfLife, (double)dt);

            float               fov = m_fov;
            Orientation         orientation;
            const Orientation*  porientation = &m_orientation;
            if (bJiggle)
            {
                IafterburnerIGC*    afterburner = (IafterburnerIGC*)(trekClient.GetShip()->GetSourceShip()->GetMountedPart(ET_Afterburner, 0));
                if (afterburner || m_jiggle > 1.0)
                {
                    float power = m_jiggle + (afterburner ? afterburner->GetPower() : 0);
                    if (power != 0.0f)
                    {
                        float amount = RadiansFromDegrees(0.5) * power;

                        orientation = m_orientation;

                        orientation.Yaw(random(-amount, amount));
                        orientation.Pitch(random(-amount, amount));

                        porientation = &orientation;
                    }
                }

                //Hack alert ... find the closest aleph and use it to provide an override for the fov
                IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();

                ImodelIGC*  pmodelAleph = FindTarget(pshipSource, c_ttWarp | c_ttNearest | c_ttAllSides);
                if (pmodelAleph)
                {
                    float   range = (pmodelAleph->GetPosition() - pshipSource->GetPosition()).Length() -
                                    trekClient.GetShip()->GetRadius();

                    float   radius = pmodelAleph->GetRadius();

                    if (range < radius)
                    {
                        fov = (range / radius) * s_fDefaultFOV;
                        if (fov < 0.1f)
                            fov = 0.1f;
                        else if (fov > m_fov)
                            fov = m_fov;
                    }
                }
            }

            Orientation viewOrientation = m_orientationHead * *porientation;

            m_pcamera->SetOrientation(viewOrientation);
            m_pcameraPosters->SetOrientation(viewOrientation);


            if (bTarget)
            {
                ImodelIGC* pmodelTarget = GetCurrentTarget();

                if (pmodelTarget != NULL)
                {
                    Orientation orientTarget = *porientation;

                    Vector  dp = pmodelTarget->GetPosition() - m_position;

                    static const Vector c_dpDefault(0.0f, 1.0f, 5.0f);
                    orientTarget.TurnTo(dp.LengthSquared() < 1.0f ? c_dpDefault : dp);
                    orientTarget.Invert();

                    m_porientTransformTarget->SetValue(Matrix(orientTarget, Vector(0, 0, 0), 1));
                    m_pwrapGeoTarget->SetGeo(m_pgeoTarget);
                }
                else
                {
                    m_pwrapGeoTarget->SetGeo(Geo::GetEmpty());
                }
            }

            m_pcamera->SetFOV(fov);
            m_pcameraPosters->SetFOV(fov);
            //m_pcameraTurret->SetFOV(m_fov);

            m_pcamera->Update();
            m_pcameraPosters->Update();
            //m_pcameraTurret->Update();

            return animatingF;
        }

        void    SetAnimation(float t)
        {
            if (t > 0.0f)
            {
                //Start a new animation
                m_position1 = m_position;
                m_quaternion1 = m_orientation;
                m_fov1 = m_fov;

                if (m_dtAnimate == 0.0f)
                {
                    //We were not animating ... so set default destinations for the things that animate
                    m_position2 = m_position1;
                    m_quaternion2 = m_quaternion1;
                    m_fov2 = m_fov1;
                }

                m_tnowAnimate = 0.0f;
                m_dtAnimate = t;

                m_bAnimatePosition = true;  //by default
            }
            else if ((t == 0.0f) && (m_dtAnimate != 0.0f))
            {
                //Turn animations off
                m_dtAnimate = 0.0f;

                m_position = m_position2;
                m_quaternion = m_quaternion2;
                m_fov = m_fov2;
            }
        }

        float   GetAnimation(void) const
        {
            return m_dtAnimate;
        }

        void    SetJiggle(float jiggle)
        {
            ZAssert(jiggle >= 0.0f);
            m_jiggle = jiggle;
        }

        void    SetToModel(ImodelIGC* pmodel)
        {
            assert (pmodel);
            SetPosition(pmodel->GetPosition());
            SetOrientation(pmodel->GetOrientation());
        }

        void    SetFOV(float   fov)
        {
            if (m_dtAnimate <= 0.0f)
            {
                //Snap to a position instantly
                m_fov = fov;
            }
            else
            {
                //Modify the endpoint of an existing animation
                m_fov2 = fov;
            }
        }

        void    SetPosition(const Vector&   position)
        {
            if ((m_dtAnimate <= 0.0f) || !m_bAnimatePosition)
            {
                //Snap to a position instantly
                m_position = position;
            }
            else
            {
                //Modify the endpoint of an existing animation
                m_position2 = position;
            }
        }

        void    SetOrientation(const Orientation&   orientation)
        {
            if (m_dtAnimate <= 0.0f)
            {
                m_orientation = orientation;
            }
            else
            {
                m_quaternion2 = orientation;
            }
        }

        float               GetFOV(void) const
        {
            return m_fov;
        }

        const Vector&       GetPosition(void) const
        {
            return m_position;
        }

        const Orientation&  GetOrientation(void) const
        {
            return m_orientation;
        }

        const Vector&       GetCameraPosition(void) const
        {
            if (m_pcamera)
            {
                return m_pcamera->GetPosition();
            }
            else
            {
                return m_position;
            }
        }

        const Orientation&  GetCameraOrientation(void) const
        {
            if (m_pcamera)
            {
                return m_pcamera->GetOrientation();
            }
            else
            {
                return m_orientation;
            }
        }

        void                SetAnimatePosition(bool bAnimatePosition)
        {
            m_bAnimatePosition = bAnimatePosition;
        }

    private:
        TRef<Camera>          m_pcamera;
        TRef<Camera>          m_pcameraPosters;
        //TRef<Camera>          m_pcameraTurret;

        Orientation     m_orientationHead;

        Vector          m_position;
        Orientation     m_orientation;
        Quaternion      m_quaternion;
        float           m_fov;

        float           m_dtAnimate;
        float           m_tnowAnimate;

        float           m_jiggle;

        Vector          m_position1;
        Vector          m_position2;

        Quaternion      m_quaternion1;
        Quaternion      m_quaternion2;

        float           m_fov1;
        float           m_fov2;

        float           m_oldHeadYaw;

        bool            m_bAnimatePosition;

    public:
        // some members are public

        //
        // Target HUD
        //

        TRef<Camera>           m_pcameraTarget;
        TRef<WrapGeo>          m_pwrapGeoTarget;
        TRef<Geo>              m_pgeoTarget;
        TRef<MatrixTransform>  m_porientTransformTarget;
};

// this class wraps the logic for tracking the listener's head position based
// on camera position
class CameraListener : public ISoundListener
{
private:
    // the camera to which this coresponds
    const CameraControl& m_camera;

public:

    // constructor
    CameraListener(CameraControl& camera) :
      m_camera(camera)
    {
    }

    //
    // ISoundListener
    //

    // get the "up" vector for the listener
    HRESULT GetUpDirection(Vector& vectUp)
    {
        vectUp = m_camera.GetCameraOrientation().GetUp();
        return S_OK;
    }


    //
    // ISoundPositionSource
    //

    // Gets the position of the sound in space
    HRESULT GetPosition(Vector& vectPosition)
    {
        vectPosition = m_camera.GetCameraPosition();
        return S_OK;
    }

    // Gets the velocity of the sound in space
    HRESULT GetVelocity(Vector& vectVelocity)
    {
        // REVIEW: the camera's velocity is not the same as the ships!
        vectVelocity = trekClient.GetShip()->GetCluster() ?
            trekClient.GetShip()->GetVelocity() : Vector(0,0,0);
        return S_OK;
    }

    // Gets the orientation of the sound in space, used for sound cones.
    HRESULT GetOrientation(Vector& vectOrientation)
    {
        vectOrientation = m_camera.GetCameraOrientation().GetForward();
        return S_OK;
    }

    // Returns S_OK if the position, velocity and orientation reported are
    // relative to the listener, S_FALSE otherwise.
    HRESULT IsListenerRelative()
    {
        return S_FALSE; // Of course
    }

    // Returns S_OK if this source is still playing the sound, S_FALSE
    // otherwise.  This might be false if a sound emitter is destroyed, for
    // example, in which case the sound might fade out.  Once it returns
    // S_FALSE once, it should never return S_OK again.
    HRESULT IsPlaying()
    {
        return S_OK;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Main Trek Window
//
//////////////////////////////////////////////////////////////////////////////

TRef<TrekWindow> g_ptrekWindow;
TRef<UpdatingConfiguration> g_pConfiguration = new UpdatingConfiguration(
    std::make_shared<FallbackConfigurationStore>(
        CreateJsonConfigurationStore(GetExecutablePath() + "\\config.json"),
        std::make_shared<RegistryConfigurationStore>(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT)
        )
);

TrekWindow* GetWindow()     { return g_ptrekWindow;               }
Engine*     GetEngine()     { return g_ptrekWindow->GetEngine();  }
Modeler*    GetModeler()    { return g_ptrekWindow->GetModeler(); }
UpdatingConfiguration* GetConfiguration() { return g_pConfiguration; }

/*
 * DelayLoadDllExceptionFilter
 *
   Purpose: Determine whether we need to halt if ICQ dll isn't found

   Side Effects: Handle exception if it's just ICQ not found, otherwise pass exception along
 */
LONG WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pep)
{
   // Assume we recognize this exception

   LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;

   // If this is a Delay-load problem, ExceptionInformation[0] points
   // to a DelayLoadInfo structure that has detailed error info

   PDelayLoadInfo pdli = PDelayLoadInfo(pep->ExceptionRecord->ExceptionInformation[0]);

   // we're not going to do anything if the dll isn't loaded, just make sure to pass on any error that's not ours.

   switch (pep->ExceptionRecord->ExceptionCode)
   {
        case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
            // The DLL module was not found at runtime

        case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
            // The DLL module was found but it doesn?t contain the function
            break;

        default:
            lDisposition = EXCEPTION_CONTINUE_SEARCH;  // We don?t recognize this exception
            break;
   }

   return lDisposition;
}

class TrekWindowImpl :
    public TrekWindow,
    public IIntegerEventSink,
    public ISubmenuEventSink,
    public IMenuCommandSink,
    public IClientEventSink,
    public ModelerSite,
    public TrekInputSite
{
public:
    const char*     m_pszCursor;

    //////////////////////////////////////////////////////////////////////////////
    //
    // SuperKeyboardInputFilter
    //
    //////////////////////////////////////////////////////////////////////////////

    class SuperKeyboardInputFilter : public IKeyboardInput {
    private:
        TrekWindowImpl* m_pwindow;

    public:
        SuperKeyboardInputFilter(TrekWindowImpl* pwindow) :
            m_pwindow(pwindow)
        {
        }

        bool OnChar(IInputProvider* pprovider, const KeyState& ks)
        {
            return false;
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
        {
			g_bActivity = true; // - Imago: Key press = activity
            return m_pwindow->OnSuperKeyFilter(pprovider, ks, fForceTranslate);
        }

        void SetFocusState(bool bFocus)
        {
        }
    };

    bool OnSuperKeyFilter(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (
               ks.bDown
            && m_pmenuQuickChat
            && ks.vk == VK_TAB
            && m_pconsoleImage != NULL
        ) {
            //
            // Grab the tab so we can change the chat target
            //

			// Imago NYI-  VK_ESC should close the chat compose window, saving any text
			//				in the chat entry box to a buffer to later redsiplay in the chat box
			//				as highlighted text.

            m_pconsoleImage->CycleChatTarget();

            return true;
        }

        return false;
    }

    TRef<SuperKeyboardInputFilter> m_psuperKeyboardInputFilter;

    //////////////////////////////////////////////////////////////////////////////
    //
    // KeyboardInputFilter
    //
    //////////////////////////////////////////////////////////////////////////////

    class KeyboardInputFilter : public IKeyboardInput {
    private:
        TrekWindowImpl* m_pwindow;

    public:
        KeyboardInputFilter(TrekWindowImpl* pwindow) :
            m_pwindow(pwindow)
        {
        }

        bool OnChar(IInputProvider* pprovider, const KeyState& ks)
        {
            return false;
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
        {
            return m_pwindow->OnKeyFilter(pprovider, ks, fForceTranslate);
        }

        void SetFocusState(bool bFocus)
        {
        }
    };

    bool OnKeyFilter(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (ks.bDown) {
            TrekKey tk = m_ptrekInput->TranslateKeyMessage(ks);

            if (TK_NoKeyMapping != tk) {
                return OnTrekKeyFilter(tk);

                /*!!!
                // hook to send this keypress to training missions
                if (Training::RecordKeyPress (tk))
                {
                    OnTrekKey(tk);
                    return true;
                }
                */
            }
        }

        return false;
    }


    TRef<KeyboardInputFilter> m_pkeyboardInputFilter;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Quick Chat
    //
    //////////////////////////////////////////////////////////////////////////////

    class QuickChatMenuCommandSink :
        public IMenuCommandSink,
        public ISubmenuEventSink
    {
    private:
        TrekWindowImpl*     m_pwindow;
        TRef<QuickChatMenu> m_pqcmenu;

    public:
        QuickChatMenuCommandSink(TrekWindowImpl* pwindow, QuickChatMenu* pqcmenu) :
            m_pwindow(pwindow),
            m_pqcmenu(pqcmenu)
        {
        }

        void OnMenuCommand(IMenuItem* pitem)
        {
            m_pwindow->OnQuickChatMenuCommand(m_pqcmenu, pitem);
        }

        void OnMenuSelect(IMenuItem* pitem)
        {
        }

        void OnMenuClose(IMenu* pmenu)
        {
            m_pwindow->OnQuickChatMenuClose(pmenu);
        }

        TRef<IPopup> GetSubMenu(IMenuItem* pitem)
        {
            return m_pwindow->OnQuickChatMenuCommand(m_pqcmenu, pitem);
        }
    };
    friend class QuickChatMenuCommandSink;

    TVector<TRef<SonicChat> > m_vsonicChats;
    TRef<IMenu>               m_pmenuQuickChat;
    TRef<QuickChatMenu>       m_pqcmenuMain;
    TRef<MDLType>             m_pmdlTypeMenu;
    ChatTarget                m_ctLobbyChat;
	ObjectID	              m_ctLobbyChatRecip;
    bool                      m_bQuitComposing;

    void OnQuickChatMenuClose(IMenu* pmenu)
    {
        if (pmenu == m_pmenuQuickChat) {
            m_pmenuQuickChat = NULL;

            if (m_pconsoleImage && m_bQuitComposing)
                m_pconsoleImage->QuitComposing();
        }
    }

    TRef<IPopup> OnQuickChatMenuCommand(QuickChatMenu* pqcmenu, IMenuItem* pmenuItem)
    {
        //
        // Find the QuickChatMenuItem
        //

        TRef<IObjectList> plist  = pqcmenu->GetItems();
        QuickChatMenuItem* pitem = (QuickChatMenuItem*)plist->GetNth(pmenuItem->GetID());

        //
        // Handle the Node
        //

        QuickChatNode* pnode = pitem->GetNode();

        if (pnode->GetType()->IsTypeOf(m_pmdlTypeMenu)) {
            //
            // It's a menu
            //

            return CreateQuickChatMenu((QuickChatMenu*)pnode);
        } else {
            //
            // It's a command
            //

            QuickChatCommand* pqcc = (QuickChatCommand*)pnode;

            Training::CommandAcknowledgedCondition::SetCommandAcknowledged (static_cast<char> (pqcc->GetCommandID ()));

            if (m_pconsoleImage)
            {
                m_pconsoleImage->SendQuickChat(
                    pqcc->GetSonicChat()->GetIndex(),
                    (int)pqcc->GetTargetType(),
                    (char)pqcc->GetCommandID(),
                    (short)pqcc->GetAbilityMask()
                );
            }
            else
            {
                // if the console is not around, send it to the lobby chat recipient. //Imago #8 7/10
                trekClient.SendChat(trekClient.GetShip(), m_ctLobbyChat,
                    m_ctLobbyChatRecip, pqcc->GetSonicChat()->GetIndex(), NULL);
            }

            m_pmenuQuickChat = NULL;

            //
            // Close the current menu
            //

            GetPopupContainer()->ClosePopup(m_pmenuQuickChat);

            return NULL;
        }
    }

    void OpenMainQuickChatMenu()
    {
        if (m_pconsoleImage)
            m_pconsoleImage->StartQuickChat(CHAT_NOSELECTION, NA, NULL);
        OpenQuickChatMenu(m_pqcmenuMain);
    }

    TRef<IMenu> CreateQuickChatMenu(QuickChatMenu* pqcmenu)
    {
        TRef<QuickChatMenuCommandSink> pmenuCommandSinkQuickChat = new QuickChatMenuCommandSink(this, pqcmenu);

        TRef<IMenu> pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                pmenuCommandSinkQuickChat
            );

        TRef<IObjectList> plist = pqcmenu->GetItems();

        plist->GetFirst();

        int index = 0;
        while (plist->GetCurrent() != NULL) {
            QuickChatMenuItem* pitem = (QuickChatMenuItem*)plist->GetCurrent();

            QuickChatNode* pnode = pitem->GetNode();

            if (pnode->GetType()->IsTypeOf(m_pmdlTypeMenu)) {
                pmenu->AddMenuItem(
                    index,
                    pitem->GetString(),
                    (char)pitem->GetChar(),
                    pmenuCommandSinkQuickChat
                );
            } else {
                pmenu->AddMenuItem(index, pitem->GetString(), (char)pitem->GetChar());
            }

            index++;
            plist->GetNext();
        }

        return pmenu;
    }

    void OpenQuickChatMenu(QuickChatMenu* pqcmenu)
    {
        m_pmenuQuickChat = CreateQuickChatMenu(pqcmenu);

        Point point = Point(10, GetEngine()->GetFullscreenSize().Y() - 10);
        OpenPopup(m_pmenuQuickChat, Rect(point, point));
    }

    void CloseQuickChatMenu()
    {
        if (m_pmenuQuickChat) {
            GetPopupContainer()->ClosePopup(m_pmenuQuickChat);
            m_pmenuQuickChat = NULL;
        }
    }

    void InitializeQuickChatMenu()
    {
        TRef<INameSpace>  pnsQuickChat = GetModeler()->GetNameSpace("quickchat");

        TRef<IObjectList> plist; CastTo(plist, pnsQuickChat->FindMember("sonicChats"));
        TLookup<SonicChat, DoSetIndex<SonicChat> >::Parse(plist, 0, m_vsonicChats);

        m_pqcmenuMain  = (QuickChatMenu*)pnsQuickChat->FindMember("mainMenu");
        m_pmdlTypeMenu = pnsQuickChat->FindType("QuickChatMenu");
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Sonic Chats
    //
    //////////////////////////////////////////////////////////////////////////////

    void PlaySonicChat(int id, int voice)
    {
        if ((id >= 0) && (id < m_vsonicChats.GetCount()))
        {
            SonicChat*           psonicChat  = m_vsonicChats[id];
            if (psonicChat)
            {
                TRef<IObjectList>    plistVoices = psonicChat->GetVoices();
                if (plistVoices)
                {
                    TRef<SonicChatVoice> pvoice      = (SonicChatVoice*)(plistVoices->GetNth(voice));

                    if (pvoice)
                        StartSound((ISoundTemplate*)(IObject*)pvoice->GetSound());
                    else
                        trekClient.PlaySoundEffect(id);
                }
            }
        }
    }

    ZString GetSonicChatText(int id, int voice)
    {
        if ((id >= 0) && (id < m_vsonicChats.GetCount()))
        {
            SonicChat*           psonicChat  = m_vsonicChats[id];
            if (psonicChat)
            {
                TRef<IObjectList>    plistVoices = psonicChat->GetVoices();
                if (plistVoices)
                {
                    TRef<SonicChatVoice> pvoice      = (SonicChatVoice*)(plistVoices->GetNth(voice));
                    if (pvoice)
                        return pvoice->GetString();
                }
            }
        }

        static ZString  unknown("Unknown chat");
        return unknown;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // member variables
    //
    //////////////////////////////////////////////////////////////////////////////

    unsigned long       m_frameID;
    Time                m_timeLastFrame;
    Time                m_timeLastDamage;
    float               m_fDeltaTime;
	// -Imago: Last activity timer
	Time				m_timeLastActivity;
	
    //Imago 7/10
    bool					m_bFFAutoCenter;
	TRef<ModifiableNumber>  m_pnumFFGain;
	//8/10
	int						m_iMouseAccel;
	int						m_iWheelDelay; //Spunky #282
	TRef<ModifiableNumber>  m_pnumMouseSens;
	//

    //
    // Rendering Toggles
    //

    bool m_bShowMeteors;
    bool m_bShowStations;
    bool m_bShowProjectiles;
    bool m_bShowAlephs;
    bool m_bShowShips;
    bool m_bBidirectionalLighting;

    TRef<ModifiableBoolean> m_pboolChatHistoryHUD;
    TRef<ModifiableBoolean> m_pboolCenterHUD;
    TRef<ModifiableBoolean> m_pboolTargetHUD;
    TRef<ModifiableNumber>  m_pnumberStyleHUD;
    TRef<ModifiableNumber>  m_pnumberIsGhost;
    TRef<ModifiableNumber>  m_pnumberFlash;
    TRef<ModifiableNumber>  m_pnumberTeamPaneCollapsed;
    TRef<WrapNumber>        m_pwrapNumberStyleHUD;

	// #294 - Turkey
	TRef<ModifiableNumber>		m_pnumberChatLinesDesired;
	TRef<ModifiableNumber>		m_pnumberChatLines;
	TRef<WrapNumber>			m_pwrapNumberChatLines;

    //
    // exports
    //

    TRef<ModifiableNumber>     m_pplayerThrottle;
    TRef<ModifiableColorValue> m_pcolorHUD;
    TRef<ModifiableColorValue> m_pcolorHUDshadows;
    TRef<ModifiableColorValue> m_pcolorTargetHUD;

    TRef<ModifiableNumber>     m_pnumSFXGain;
    TRef<ModifiableNumber>     m_pnumMusicGain;
    TRef<ModifiableNumber>     m_pnumVoiceOverGain;

    //
    // Screens
    //

    TRef<UiEngine>       m_pUiEngine;
    TRef<Image>          m_pimageScreen;
    TRef<Screen>         m_pscreen;
    ScreenID             m_screen;
	TRef<MatrixTransform2> m_pMatrixTransformScreen;

    //
    // Lighting
    //

    Color            m_color;
    Color            m_colorAlt;
    float            m_ambientLevel;
    float            m_ambientLevelBidirectional;

    //
    // The scene
    //

    TRef<Camera>           m_pcamera;
    TRef<Camera>           m_pcameraPosters;
    //TRef<Camera>           m_pcameraTurret;

    TRef<Viewport>         m_pviewport;
    TRef<Viewport>         m_pviewportPosters;
    //TRef<Viewport>         m_pviewportTurret;

    CameraControl          m_cameraControl;

    TRef<JoystickImage>    m_pjoystickImage;

    TRef<Geo>        m_pgeoDebris;
    TRef<WrapGeo>          m_pwrapGeoDebris;
	TRef<ModifiableNumber> m_debrisDensity; //LANS
    //TRef<Geo>              m_pgeoTurret;
    //TRef<MatrixTransform>  m_pmtTurret;
    TRef<WrapGeo>          m_pgeoScene;

    TRef<GeoImage>         m_pimageScene;
    TRef<StarImage>        m_pimageStars;
    TRef<GeoImage>         m_pimageEnvironment;
    //TRef<GeoImage>         m_pimageTurret;
    TRef<LensFlareImage>   m_pimageLensFlare;
    TRef<Image>            m_pimageBlack;
    TRef<WrapImage>        m_pwrapImageStars;
    TRef<WrapImage>        m_pwrapImageLensFlare;
    TRef<WrapImage>        m_pwrapImageScene;
    TRef<WrapImage>        m_pwrapImagePosters;
    TRef<WrapImage>        m_pwrapImageEnvironment;
    //TRef<WrapImage>        m_pwrapImageTurret;
    TRef<WrapImage>        m_pwrapImagePostersInside;
    TRef<WrapImage>        m_pwrapImageHudGroup;
    TRef<WrapImage>        m_pwrapImageComm;
    TRef<WrapImage>        m_pwrapImageRadar;
    TRef<WrapImage>        m_pwrapImageConsole;
    TRef<WrapImage>        m_pwrapImageHelp;
    TRef<WrapImage>        m_pwrapImageTop;
    TRef<ConsoleImage>     m_pconsoleImage;
    TRef<RadarImage>       m_pradarImage;
    TRef<MuzzleFlareImage> m_pmuzzleFlareImage;
    TRef<WrapImage>        m_pwrapImageBackdrop;
    TRef<GroupImage>       m_pgroupImage;
    TRef<GroupImage>       m_pgroupImage3D;
    TRef<GroupImage>       m_pgroupImageGame;
    TRef<GroupImage>       m_pgroupImageHUD;

    //
    // camera stuff
    //

    CameraMode          m_cm;
    CameraMode          m_cmOld;
    CameraMode          m_cmPreviousCommand;
    bool                m_bPreferChaseView;
    Time                m_timeOverrideStop;
    Time                m_timeRejectQueuedCommand;
    //Time                m_timeAutopilotWarning;
    bool                m_bUseOverridePosition;
    Vector              m_positionOverrideTarget;
    Vector              m_positionCommandView;
    Vector              m_displacementCommandView;
    TRef<ImissileIGC>   m_pmissileLast;
    Orientation         m_orientationExternalCamera;
    float               m_distanceExternalCamera;
    float               m_distanceCommandCamera;
    float               m_rollCommandCamera;
    bool                m_bEnableDisplacementCommandView;
    int                 m_suicideCount;
    Time                m_suicideTime;
    bool                m_bCommandGrid;
    bool                m_bTrackCommandView;
    int                 m_radarCockpit;
    int                 m_radarCommand;

    //
    // Pane stuff
    //

    ViewMode            m_viewmode;
    OverlayMask         m_voverlaymask[c_cViewModes];
    bool                m_bOverlaysChanged;
	bool				m_bShowSectorMapPane;
	bool				m_bShowInventoryPane;
    bool                m_bFreshInvestmentPane;


    //
    // chase view stuff
    //

    #define ARRAY_OF_SAMPLES_SIZE  4092 // BT - 8/17 - Was 128. Fixing sample under-run issues. Dx9 takes way more samples than Dx7 did, especially when zoomed out. 
    struct  TurnRateSample
    {
        float   fTurnRate[3];
        Time    time;
    };
    TurnRateSample      m_turnRateSamples[ARRAY_OF_SAMPLES_SIZE];
    int                 m_turnRateSampleIndex;

    //
    // LOD
    //

    TRef<ScrollPane>          m_pscrollPane;
    TRef<IIntegerEventSource> m_peventLOD;
    TRef<Image>               m_pimageLOD;
    TRef<WrapImage>           m_pwrapImageLOD;

    //
    // Menus
    //

    TRef<IMenu>                m_pmenu;
    TRef<IMenuCommandSink>     m_pmenuCommandSink;
    TRef<ISubmenuEventSink>    m_psubmenuEventSink;
    TRef<IIntegerEventSink>    m_pintegerEventSink;
    TRef<IKeyboardInput>       m_pkeyboardInput;
    TRef<IClientEventSink>     m_pClientEventSink;
    TRef<IClientEventSource>   m_pClientEventSource;

    TRef<IMenuItem>            m_pitemTogglePosters;
    TRef<IMenuItem>            m_pitemToggleDebris;
    TRef<IMenuItem>            m_pitemToggleStars;
    TRef<IMenuItem>            m_pitemToggleEnvironment;
    TRef<IMenuItem>			   m_pitemToggleUseOldUi;
    TRef<IMenuItem>			   m_pitemToggleShowJoystickIndicator;
    TRef<IMenuItem>            m_pitemToggleRoundRadar;
    TRef<IMenuItem>            m_pitemToggleLinearControls;
    TRef<IMenuItem>            m_pitemToggleLargeDeadZone;
    TRef<IMenuItem>            m_pitemToggleVirtualJoystick;
    TRef<IMenuItem>            m_pitemToggleFlipY;
    //Imago 7/10
    TRef<IMenuItem>            m_pitemToggleEnableFeedback; //moved around & fixed up 7/10 #187
	TRef<IMenuItem>            m_pitemToggleFFAutoCenter; 
	TRef<IMenuItem>            m_pitemToggleFFGainUp; 
	TRef<IMenuItem>            m_pitemToggleFFGainDown;
	// 8/10 #215
	TRef<IMenuItem>            m_pitemToggleMouseSensUp;
	TRef<IMenuItem>            m_pitemToggleMouseSensDown;
	TRef<IMenuItem>            m_pitemToggleMouseAccel;
	TRef<IMenuItem>            m_pitemToggleWheelDelay; //Spunky #282
	 //
    TRef<IMenuItem>            m_pitemToggleStrobes;
    TRef<IMenuItem>            m_pitemToggleTrails;
    TRef<IMenuItem>            m_pitemToggleBounds;
    TRef<IMenuItem>            m_pitemToggleTransparentObjects;
    TRef<IMenuItem>            m_pitemToggleSmoke;
    TRef<IMenuItem>            m_pitemToggleLensFlare;
    TRef<IMenuItem>            m_pitemToggleBidirectionalLighting;
    TRef<IMenuItem>            m_pitemToggleChatHistoryHUD;
    TRef<IMenuItem>            m_pitemToggleCenterHUD;
    TRef<IMenuItem>            m_pitemToggleTargetHUD;
    TRef<IMenuItem>            m_pitemStyleHUD;
    TRef<IMenuItem>            m_pitemToggleCensorChats;
    TRef<IMenuItem>            m_pitemToggleStickyChase;
    TRef<IMenuItem>            m_pitemFilterChatsToAll;
    TRef<IMenuItem>            m_pitemFilterQuickComms;
    TRef<IMenuItem>            m_pitemFilterLobbyChats;
	TRef<IMenuItem>			   m_pitemIncreaseChatLines;	// #294 - Turkey
	TRef<IMenuItem>			   m_pitemReduceChatLines;		// #294 - Turkey
    TRef<IMenuItem>            m_pitemSoundQuality;
    TRef<IMenuItem>            m_pitemToggleSoundHardware;
    TRef<IMenuItem>            m_pitemToggleDSound8Usage;
    TRef<IMenuItem>            m_pitemToggleMusic;
    TRef<IMenuItem>            m_pitemMusicVolumeUp;
    TRef<IMenuItem>            m_pitemMusicVolumeDown;
    TRef<IMenuItem>            m_pitemSFXVolumeUp;
    TRef<IMenuItem>            m_pitemSFXVolumeDown;
    TRef<IMenuItem>            m_pitemVoiceOverVolumeUp;
    TRef<IMenuItem>            m_pitemVoiceOverVolumeDown;
    TRef<IMenuItem>            m_pitemMaxTextureSize;		// yp Your_Persona August 2 2006 : MaxTextureSize Patch
    /* pkk May 6th: Disabled bandwidth patch
    TRef<IMenuItem>            m_pitemToggleBandwidth; // w0dk4 June 2007: Bandwith Patch*/
    TRef<IMenuItem>            m_pitemMuteFilter;			//TheBored 30-JUL-07: Filter Unknown Chat patch
    TRef<IMenuItem>            m_pitemFilterUnknownChats;	//TheBored 30-JUL-07: Filter Unknown Chat patch
	//imago added -- 6/29/09
	TRef<IMenuItem>            m_pitemMip;
	TRef<IMenuItem>            m_pitemPack;
	TRef<IMenuItem>            m_pitemAA;
	TRef<IMenuItem>            m_pitemVsync;

    bool                       m_bLensFlare;
    bool                       m_bMusic;
    bool                       m_bRoundRadar;

    bool                       m_bShowJoystickIndicator;

    //
    // CommandView
    //

    TRef<CommandGeo>    m_pCommandGeo;
    TRef<VisibleGeo>    m_pCommandVisibleGeo;

    //
    // UI stuff
    //

    int             m_nLastCountdown;
    TRef<IMessageBox> m_pmessageBoxLockdown;

	//
	//	ContextMenu data
	//
	PlayerInfo * contextPlayerInfo;

    //
    // Sound support
    //

    TRef<ISoundEngine>              m_pSoundEngine;
    TVector<TRef<ISoundTemplate> >  m_vSoundMap;
    ISoundEngine::Quality           m_soundquality;
    bool                            m_bEnableSoundHardware;
	bool                            m_bUseDSound8;
    TRef<ISoundMutex>               m_psoundmutexSal;
    TRef<ISoundMutex>               m_psoundmutexVO;

    //
    // VT members.
    //

    DWORD m_cVTVersion;
    HWND  m_hwndVTEdit;

    TRef<SimpleModifiableValue<bool>> m_pUseOldUi;

    //
    // Input
    //

    TRef<TrekInput> m_ptrekInput;
    bool            m_bEnableVirtualJoystick;
    bool            m_bFlipY;
    bool            m_bEnableFeedback;
    bool            m_bLinearControls;

    AsteroidAbilityBitMask          m_aabmInvest;
    AsteroidAbilityBitMask          m_aabmCommand;

public:

    bool      OnActivateApp(bool bActive) override {
        bool result = EngineWindow::OnActivateApp(bActive);
        UpdateMouseEnabled();
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Animated Image cache
    //
    //////////////////////////////////////////////////////////////////////////////

protected:
    TMap<ZString, TRef<AnimatedImage> > m_mapAnimatedImages;

public:
    TRef<AnimatedImage> LoadAnimatedImage(Number* ptime, const ZString& str)
    {
        TRef<AnimatedImage> pimage;

        if (m_mapAnimatedImages.Find(str, pimage)) {
            return new AnimatedImage(ptime, pimage);
        } else {
            TRef<Surface> psurface = GetModeler()->LoadSurface(str, true);

            if (psurface == NULL) {
                return NULL;
            }

            pimage = new AnimatedImage(ptime, psurface);

            m_mapAnimatedImages.Set(str, pimage);

            return pimage;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class GameStateCloseSink : public IEventSink {
    public:
        TrekWindowImpl*    m_pwindow;
        TRef<IEventSource> m_pevent;

        GameStateCloseSink(TrekWindowImpl* pwindow, IEventSource* pevent) :
            m_pwindow(pwindow),
            m_pevent(pevent)
        {
            m_pevent->AddSink(this);
        }

        void Terminate()
        {
            m_pevent->RemoveSink(this);
        }

        bool OnEvent(IEventSource* pevent)
        {
            m_pwindow->OnGameStateClose();
            return true;
        }
    };

    TRef<GameStateContainer> m_pgsc;
    TRef<GameStateCloseSink> m_pgameStateCloseSink;

    TRef<StringPane>         m_pstringPaneConquest;
    TRef<StringPane>         m_pstringPaneTerritory;
    TRef<StringPane>         m_pstringPaneDeathMatch;
    TRef<StringPane>         m_pstringPaneCountdown;
    TRef<StringPane>         m_pstringPaneProsperity;
    TRef<StringPane>         m_pstringPaneArtifacts;
    TRef<StringPane>         m_pstringPaneFlags;

    TRef<StringGridImage>    m_pstringGridImageConquest;
    TRef<StringGridImage>    m_pstringGridImageTerritory;
    TRef<StringGridImage>    m_pstringGridImageDeathMatch;
    TRef<StringGridImage>    m_pstringGridImageProsperity;
    TRef<StringGridImage>    m_pstringGridImageArtifacts;
    TRef<StringGridImage>    m_pstringGridImageFlags;

    void OnGameStateClose()
    {
        TurnOffOverlayFlags(ofGameState);
    }

    void InitializeGameStateContainer()
    {
        m_pgsc = m_pconsoleImage->GetGameStateContainer();
        m_pgameStateCloseSink =
            new GameStateCloseSink(
                this,
                m_pgsc->GetCloseEvent()
            );

        //
        // Add a game state tile for each winning condition
        // artifacts, deathmatch, prosperity, countdown
        //

        //
        // Conquest grid
        //

        const MissionParams* pmp = trekClient.m_pCoreIGC->GetMissionParams();

        if (pmp->IsConquestGame()) {
            m_pstringGridImageConquest = CreateStringGridImage(3, 2, m_pgsc->GetFont());

            TRef<Pane> ppaneGrid = new AnimatedImagePane(m_pstringGridImageConquest);

            m_pstringPaneConquest = m_pgsc->AddGameStateTile("wctitleconquestbmp", ppaneGrid);
            m_pstringPaneConquest->SetFont(TrekResources::SmallFont());
            m_pstringPaneConquest->SetTextColor(Color::White());

            if (pmp->GetConquestPercentage() == 100) {
                m_pstringPaneConquest->SetString("1st team to own all of the starbases wins");
            } else {
                m_pstringPaneConquest->SetString(
                    "1st team to own " + ZString(pmp->GetConquestPercentage()) + "% of the starbases wins"
                );
            }
        } else {
            m_pstringGridImageConquest = NULL;
        }

        if (pmp->IsTerritoryGame()) {
            m_pstringGridImageTerritory = CreateStringGridImage(3, 2, m_pgsc->GetFont());

            TRef<Pane> ppaneGrid = new AnimatedImagePane(m_pstringGridImageTerritory);

            m_pstringPaneTerritory = m_pgsc->AddGameStateTile("wctitleterritorybmp", ppaneGrid);  //NYI need new artwork
            m_pstringPaneTerritory->SetFont(TrekResources::SmallFont());
            m_pstringPaneTerritory->SetTextColor(Color::White());

            m_pstringPaneTerritory->SetString(
                "1st team to own " + ZString((50 + trekClient.m_pCoreIGC->GetClusters()->n() *
                                                   pmp->GetTerritoryPercentage()) / 100) + " sectors wins"
            );
        } else {
            m_pstringGridImageTerritory = NULL;
        }


        if (pmp->IsProsperityGame()) {
            m_pstringGridImageProsperity = CreateStringGridImage(3, 2, m_pgsc->GetFont());

            TRef<Pane> ppaneGrid = new AnimatedImagePane(m_pstringGridImageProsperity);

            m_pstringPaneProsperity = m_pgsc->AddGameStateTile("wctitleprosperitybmp", ppaneGrid);
            m_pstringPaneProsperity->SetFont(TrekResources::SmallFont());
            m_pstringPaneProsperity->SetTextColor(Color::White());
            m_pstringPaneProsperity->SetString(
                "First team to purchase reinforcements wins"
            );
        } else {
            m_pstringGridImageProsperity = NULL;
        }

        if (pmp->IsArtifactsGame()) {
            m_pstringGridImageArtifacts = CreateStringGridImage(3, 2, m_pgsc->GetFont());

            TRef<Pane> ppaneGrid = new AnimatedImagePane(m_pstringGridImageArtifacts);

            m_pstringPaneArtifacts = m_pgsc->AddGameStateTile("wctitleartifactsbmp", ppaneGrid);
            m_pstringPaneArtifacts->SetFont(TrekResources::SmallFont());
            m_pstringPaneArtifacts->SetTextColor(Color::White());
            m_pstringPaneArtifacts->SetString(
                "First team to own " + ZString(pmp->nGoalArtifactsCount) + " artifacts wins"
            );
        } else {
            m_pstringGridImageArtifacts = NULL;
        }

        if (pmp->IsFlagsGame()) {
            m_pstringGridImageFlags = CreateStringGridImage(3, 2, m_pgsc->GetFont());

            TRef<Pane> ppaneGrid = new AnimatedImagePane(m_pstringGridImageFlags);

            m_pstringPaneFlags = m_pgsc->AddGameStateTile("wctitlecapturebmp", ppaneGrid);
            m_pstringPaneFlags->SetFont(TrekResources::SmallFont());
            m_pstringPaneFlags->SetTextColor(Color::White());
            m_pstringPaneFlags->SetString(
                "First team to capture " + ZString(pmp->nGoalFlagsCount) + " flags wins"
            );
        } else {
            m_pstringGridImageFlags = NULL;
        }

        {
            m_pstringGridImageDeathMatch = CreateStringGridImage(5, 4, m_pgsc->GetFont());

            TRef<Pane> ppaneGrid = new AnimatedImagePane(m_pstringGridImageDeathMatch);

            m_pstringPaneDeathMatch = m_pgsc->AddGameStateTile(pmp->IsDeathMatchGame()
                                                               ? "wctitledeathmatchbmp"
                                                               : "wctitledeathsbmp",
                                                               ppaneGrid);
            m_pstringPaneDeathMatch->SetFont(TrekResources::SmallFont());
            m_pstringPaneDeathMatch->SetTextColor(Color::White());

            m_pstringPaneDeathMatch->SetString(pmp->IsDeathMatchGame()
                                               ? "1st team to kill " + ZString(pmp->GetDeathMatchKillLimit()) + " ships wins"
                                               : "");
        }
        UpdateGameStateContainer();
    }

    void TerminateGameStateContainer()
    {
        m_pgsc                = NULL;

        if (m_pgameStateCloseSink) {
            m_pgameStateCloseSink->Terminate();
            m_pgameStateCloseSink = NULL;
        }

        m_pstringPaneConquest   = NULL;
        m_pstringPaneTerritory  = NULL;
        m_pstringPaneDeathMatch = NULL;
        m_pstringPaneCountdown  = NULL;
        m_pstringPaneProsperity = NULL;
        m_pstringPaneArtifacts  = NULL;
        m_pstringPaneFlags  = NULL;

        m_pstringGridImageConquest   = NULL;
        m_pstringGridImageTerritory  = NULL;
        m_pstringGridImageDeathMatch = NULL;
        m_pstringGridImageProsperity = NULL;
        m_pstringGridImageArtifacts  = NULL;
        m_pstringGridImageFlags  = NULL;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class WinConditionInfo {
    public:
        virtual void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside) = 0;
        virtual void ClearLine(StringGridImage* pgrid, int row)                 = 0;
        virtual bool Greater(IsideIGC* pside1, IsideIGC* pside2)                = 0;
    };

    class WinConditionInfo3 : public WinConditionInfo {
    public:
        void ClearLine(StringGridImage* pgrid, int row)
        {
            pgrid->SetString(row, 2, ZString());
        }
    };

    class WinConditionInfo4 : public WinConditionInfo {
    public:
        void ClearLine(StringGridImage* pgrid, int row)
        {
            pgrid->SetString(row, 2, ZString());
            pgrid->SetString(row, 3, ZString());
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class ShipWinConditionInfo {
    public:
        virtual void FillLine(StringGridImage* pgrid, int row, IshipIGC* pship) {}
        virtual bool Greater(IshipIGC* pship1, IshipIGC* pship2)                { return false; }
        virtual bool AnyInfo()                                                  { return false; }
        virtual void ClearLine(StringGridImage* pgrid, int row)                 {}
    };

    static ShipWinConditionInfo s_shipWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class ConquestWinConditionInfo : public WinConditionInfo3 {
    public:
        void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside)
        {
            pgrid->SetString(
                row,
                2,
                ZString(pside->GetConquestPercent()) + "% owned"
            );
        }

        bool Greater(IsideIGC* pside1, IsideIGC* pside2)
        {
            return pside1->GetConquestPercent() > pside2->GetConquestPercent();
        }

    };

    static ConquestWinConditionInfo s_conquestWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class TerritoryWinConditionInfo : public WinConditionInfo3 {
    public:
        void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside)
        {
            unsigned char   c = pside->GetTerritoryCount();
            pgrid->SetString(
                row,
                2,
                ZString(c) + ((c == 1) ? " sector owned" : " sectors owned")
            );
        }

        bool Greater(IsideIGC* pside1, IsideIGC* pside2)
        {
            return pside1->GetTerritoryCount() > pside2->GetTerritoryCount();
        }

    };

    static TerritoryWinConditionInfo s_territoryWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class DeathMatchWinConditionInfo : public WinConditionInfo4 {
    public:
        void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside)
        {
            pgrid->SetString(row, 2, ZString(pside->GetKills() ) + " kills" );
            pgrid->SetString(row, 3, ZString(pside->GetEjections()) + " ejects");
            pgrid->SetString(row, 4, ZString(pside->GetDeaths()) + " deaths");
        }

        bool Greater(IsideIGC* pside1, IsideIGC* pside2)
        {
            if (pside1->GetKills() == pside2->GetKills()) {
                return pside1->GetDeaths() < pside2->GetDeaths();
            } else {
                return pside1->GetKills() > pside2->GetKills();
            }
        }
    };

    class DeathMatchShipWinConditionInfo : public ShipWinConditionInfo {
    public:
        void FillLine(StringGridImage* pgrid, int row, IshipIGC* pship)
        {
            pgrid->SetString(row, 2, ZString(pship->GetKills() ) + " kills" );
            pgrid->SetString(row, 3, ZString(pship->GetEjections()) + " ejects");
            pgrid->SetString(row, 4, ZString(pship->GetDeaths()) + " deaths");
        }
        virtual void ClearLine(StringGridImage* pgrid, int row)
        {
            pgrid->SetString(row, 2, ZString());
            pgrid->SetString(row, 3, ZString());
            pgrid->SetString(row, 4, ZString());
        }

        bool Greater(IshipIGC* pship1, IshipIGC* pship2)
        {
            if (pship1->GetKills() == pship2->GetKills()) {
                return pship1->GetDeaths() < pship2->GetDeaths();
            } else {
                return pship1->GetKills() > pship2->GetKills();
            }
        }

        bool AnyInfo() { return true; }
    };

    static DeathMatchWinConditionInfo     s_deathMatchWinConditionInfo;
    static DeathMatchShipWinConditionInfo s_deathMatchShipWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class ProsperityWinConditionInfo : public WinConditionInfo3 {
    public:
        void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside)
        {
            int prosperity = 0; //Actually get from the status of the prosperity project

            BucketLinkIGC* pbl = pside->GetBuckets()->last();
            if (pbl)
            {
                IbuyableIGC* pbuyable = pbl->data()->GetBuyable();
                if ((pbuyable->GetObjectType() == OT_development) &&
                    (pbuyable->GetObjectID() == c_didTeamMoney))
                {
                    prosperity = pbl->data()->GetPercentComplete();
                }
            }

            pgrid->SetString(
                row,
                2,
                ZString(prosperity) + "% invested"
            );
        }

        bool Greater(IsideIGC* pside1, IsideIGC* pside2)
        {
            return false;
            //return pside1->GetProsperityPercent() > pside2->GetProsperityPercent();
        }

    };

    static ProsperityWinConditionInfo s_prosperityWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class ArtifactsWinConditionInfo : public WinConditionInfo3 {
    public:
        void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside)
        {
            pgrid->SetString(
                row,
                2,
                ZString(pside->GetArtifacts()) + " owned"
            );
        }

        bool Greater(IsideIGC* pside1, IsideIGC* pside2)
        {
            return pside1->GetArtifacts() > pside2->GetArtifacts();
        }

    };

    static ArtifactsWinConditionInfo s_artifactsWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class FlagsWinConditionInfo : public WinConditionInfo3 {
    public:
        void FillLine(StringGridImage* pgrid, int row, IsideIGC* pside)
        {
            pgrid->SetString(
                row,
                2,
                ZString(pside->GetFlags()) + " captured"
            );
        }

        bool Greater(IsideIGC* pside1, IsideIGC* pside2)
        {
            return pside1->GetFlags() > pside2->GetFlags();
        }

    };

    static FlagsWinConditionInfo s_flagsWinConditionInfo;

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void FillWinCondition(
        WinConditionInfo&     info,
        ShipWinConditionInfo& shipInfo,
        StringGridImage*      pgrid
    ) {
        // top team
        // my team
        // top player
        // me

        static const ZString    c_rank[c_cSidesMax] = { "#1", "#2", "#3", "#4", "#5", "#6" };
        static const ZString    c_pound("#");

        IsideIGC* psideMe = trekClient.GetSide();
        IshipIGC* pshipMe = trekClient.GetShip();

        //
        // Figure out the top side and my rank
        //

        {
            IsideIGC* psideTop = NULL;
            int       myRank   = 1;

            for (SideLinkIGC* psl = trekClient.m_pCoreIGC->GetSides()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IsideIGC* pside = psl->data();

                if (pside != psideMe)
                {
                    if ((psideTop == NULL) || info.Greater(pside, psideTop))
                        psideTop = pside;

                    if (info.Greater(pside, psideMe))
                        myRank++;
                }
            }
            assert (psideTop != NULL);

            if (myRank == 1)
            {
                //Player side is the top rank
                pgrid->SetString(0, 0, c_rank[0]);
                pgrid->SetString(0, 1, CensorBadWords(psideMe->GetName()));
                pgrid->SetColor(0, psideMe->GetColor());
                info.FillLine(pgrid, 0, psideMe);

                pgrid->SetString(1, 0, c_rank[1]);
                pgrid->SetString(1, 1, CensorBadWords(psideTop->GetName()));
                pgrid->SetColor(1, psideTop->GetColor());
                info.FillLine(pgrid, 1, psideTop);
            }
            else
            {
                //Another side is the top rank
                pgrid->SetString(0, 0, c_rank[0]);
                pgrid->SetString(0, 1, CensorBadWords(psideTop->GetName()));
                pgrid->SetColor(0, psideTop->GetColor());
                info.FillLine(pgrid, 0, psideTop);

                pgrid->SetString(1, 0, c_rank[myRank - 1]);
                pgrid->SetString(1, 1, CensorBadWords(psideMe->GetName()));
                pgrid->SetColor(1, psideMe->GetColor());
                info.FillLine(pgrid, 1, psideMe);
            }
        }

        //
        // Figure out the top ship and my rank
        //

        if (shipInfo.AnyInfo())
        {
            IshipIGC* pshipTop = NULL;
            int       myRank   = 1;

            for (ShipLinkIGC* psl = trekClient.m_pCoreIGC->GetShips()->first();
                (psl != NULL);
                psl = psl->next())
            {
                IshipIGC* pship = psl->data();

                if ((pship != pshipMe) && (pship->GetPilotType() >= c_ptPlayer))
                {
                    if ((pshipTop == NULL) || shipInfo.Greater(pship, pshipTop))
                        pshipTop = pship;

                    if (shipInfo.Greater(pship, pshipMe))
                        myRank++;
                }
            }

            if (myRank == 1)
            {
                //Player side is the top rank
                pgrid->SetString(2, 0, c_rank[0]);
                pgrid->SetString(2, 1, pshipMe->GetName());
                pgrid->SetColor(2, psideMe->GetColor());
                shipInfo.FillLine(pgrid, 2, pshipMe);

                if (pshipTop)
                {
                    pgrid->SetString(3, 0, c_rank[1]);
                    pgrid->SetString(3, 1, pshipTop->GetName());
                    pgrid->SetColor(3, pshipTop->GetSide()->GetColor());
                    shipInfo.FillLine(pgrid, 3, pshipTop);
                }
                else
                {
                    pgrid->SetString(3, 0, ZString());
                    pgrid->SetString(3, 1, ZString());
                    shipInfo.ClearLine(pgrid, 3);
                }
            }
            else
            {
                assert (pshipTop);

                //Another side is the top rank
                pgrid->SetString(2, 0, c_rank[0]);
                pgrid->SetString(2, 1, pshipTop->GetName());
                pgrid->SetColor(2, pshipTop->GetSide()->GetColor());
                shipInfo.FillLine(pgrid, 2, pshipTop);

                pgrid->SetString(3, 0, c_pound + ZString(myRank));
                pgrid->SetString(3, 1, pshipMe->GetName());
                pgrid->SetColor(3, psideMe->GetColor());
                shipInfo.FillLine(pgrid, 3, pshipMe);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateGameStateContainer()
    {
        if (m_pgsc) {
            if (m_pstringGridImageConquest) {
                FillWinCondition(
                    s_conquestWinConditionInfo,
                    s_shipWinConditionInfo,
                    m_pstringGridImageConquest
                );
            }

            if (m_pstringGridImageTerritory) {
                FillWinCondition(
                    s_territoryWinConditionInfo,
                    s_shipWinConditionInfo,
                    m_pstringGridImageTerritory
                );
            }

            if (m_pstringGridImageDeathMatch) {
                FillWinCondition(
                    s_deathMatchWinConditionInfo,
                    s_deathMatchShipWinConditionInfo,
                    m_pstringGridImageDeathMatch
                );
            }

            if (m_pstringGridImageProsperity) {
                FillWinCondition(
                    s_prosperityWinConditionInfo,
                    s_shipWinConditionInfo,
                    m_pstringGridImageProsperity
                );
            }

            if (m_pstringGridImageArtifacts) {
                FillWinCondition(
                    s_artifactsWinConditionInfo,
                    s_shipWinConditionInfo,
                    m_pstringGridImageArtifacts
                );
            }
            if (m_pstringGridImageFlags) {
                FillWinCondition(
                    s_flagsWinConditionInfo,
                    s_shipWinConditionInfo,
                    m_pstringGridImageFlags
                );
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void InitializeSoundTemplates()
    {
        SoundInit::InitializeSoundTemplates(GetModeler(), m_vSoundMap);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    bool GetUI()
    {
        return m_screen != ScreenIDCombat;
    }

    void SetScreen(WrapImage* target, Screen* pscreen)
    {
        TRef<Image> pimageScreen = pscreen->GetImage();

        if (pimageScreen == NULL) {
            pimageScreen = Image::GetEmpty();
        }

        TRef<Pane> ppane = pscreen->GetPane();

        //
        // Add a caption
        //
        if (ppane) {
            SetCaption(CreateCaption(GetModeler(), ppane, this));
        }

        if (ppane == NULL) {
            ppane = new ImagePane(Image::GetEmpty());
        }

        //
        // Create the UI Window
        //

		m_pMatrixTransformScreen = new MatrixTransform2(Matrix2::GetIdentity());

        m_pimageScreen = new TransformImage(
            new GroupImage(
                CreatePaneImage(GetEngine(), false, ppane), 
                pimageScreen
            ),
			m_pMatrixTransformScreen
		);

		target->SetImage(m_pimageScreen);
		
        SetSizeable(true); // kg-: #226 always

        //
        // keep a reference to the screen to keep it alive
        //
        m_pscreen = pscreen;
    }

	void SetUiScreen(Screen * pscreen)
	{
		SetScreen(m_pwrapImageTop, pscreen);
	}

	void SetCombatScreen(Screen* pscreen)
	{
		SetScreen(m_pwrapImageBackdrop, pscreen);
	}

    ScreenID screen(void) const
    {
        return m_screen;
    }

    // review: this gets messy because it can be recursively called by a screen's
    // constructor.

    void screen(ScreenID s)
    {
        screen(s, -1);
    }

    void screen(ScreenID s, int idPlayer, const char * szPlayerName = NULL, const char * szOther = NULL)
    {

        static bool bSwitchingScreens = false;
        static ScreenID sNextScreen;
        ZString strPlayerName = szPlayerName ? szPlayerName : ""; // make copy of player name cause previous screen is going to be destroyed
        ZString strOther = szOther ? szOther : "";

        sNextScreen = s;

        // let the switch be handled later
        if (bSwitchingScreens)
            return;

        bSwitchingScreens = true;

		// kg- #226 main screen stuff here
        debugf("Switched to screen %d\n", s);
        if (s != m_screen)
        {

            if (m_screen == ScreenIDCombat) {
				// this used to also save the current resolution. Still keeping the other functions.
                GetConsoleImage()->OnSwitchViewMode();
            }

            SetHideCursorTimer(s == ScreenIDCombat);

            // destroy the old windows

            if (m_pimageScreen != NULL) {
                m_pwrapImageTop->SetImage(m_pgroupImageGame);
                m_pimageScreen = NULL;
                m_pscreen      = NULL;
                SetCaption(NULL);
            }

            if (s != ScreenIDCombat)
            {
                if (GetViewMode() == vmOverride)
                    EndOverrideCamera();
                else
                    SetCameraMode(cmCockpit);

                m_pwrapImageConsole->SetImage(Image::GetEmpty());
                m_pconsoleImage = NULL;
                TerminateGameStateContainer();
                trekClient.RequestViewCluster(NULL);
				m_pscreen = NULL;
				m_pimageScreen = NULL;
                m_viewmode = vmUI;
            }

            m_ptrekInput->SetFocus(s != ScreenIDCombat);
            RestoreCursor();

            // destroy any open popups
            if (!GetPopupContainer()->IsEmpty())
                GetPopupContainer()->ClosePopup(NULL);
            m_pmessageBox = NULL;
            m_pmessageBoxLockdown = NULL;

            // kill any capture
            m_pwrapImageTop->RemoveCapture();

            // create the new screen

            switch (s) {
                case ScreenIDCombat:
                {
                    SetFocus();
                    m_frameID = 0;
                    m_pconsoleImage = ConsoleImage::Create(GetEngine(), m_pviewport);
                    {
                        IsideIGC*   pside = trekClient.GetShip()->GetSide();
                        assert (pside);
                        assert (pside->GetObjectID() >= 0);
                        m_pconsoleImage->SetDisplayMDL(pside->GetCivilization()->GetHUDName());
                    }
                    m_pwrapImageConsole->SetImage(m_pconsoleImage);

                    ResetOverlayMask();

                    SetViewMode(trekClient.GetShip()->GetStation()
                        ? (trekClient.GetShip()->IsGhost() ? vmCommand : vmHangar)
                        : vmCombat, true);
                    PositionCommandView(NULL, 0.0f);

                    VTSetText("Screen=%d", ScreenIDCombat);

                    //
                    // Fill in the game state dialog
                    //

                    InitializeGameStateContainer();

                    //
                    // Bring up the game state pane by default
                    //

                    TurnOnOverlayFlags(ofGameState);
                    break;
                }

                case ScreenIDTeamScreen:
                    SetUiScreen(CreateTeamScreen(GetModeler()));
                    break;

                case ScreenIDGameScreen:
					SetUiScreen(CreateGameScreen(GetModeler()));
                    break;

                case ScreenIDGameOverScreen:
					SetUiScreen(CreateGameOverScreen(GetModeler()));
                    break;

                case ScreenIDCreateMission:
					SetUiScreen(CreateNewGameScreen(GetModeler()));
                    break;

                case ScreenIDIntroScreen:
					SetUiScreen(CreateIntroScreen(GetModeler(), *m_pUiEngine, m_pUseOldUi->GetValue()));
                    break;

				case ScreenIDSplashScreen:
					{
						//Imago 6/29/09 7/28/09 dont allow intro vid on nonprimary
						HMODULE hVidTest = ::LoadLibraryA("WMVDECOD.dll");
						HMODULE hAudTest = ::LoadLibraryA("wmadmod.dll");
						bool bWMP = (hVidTest && hAudTest) ? true : false;
						::FreeLibrary(hVidTest); ::FreeLibrary(hAudTest); 
						if (!CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID && bWMP) {					
							
							//dont' check for intro.avi, 
							// let the screen flash so they at least know this works
							DDVideo *DDVid = new DDVideo();
							if (m_pengine->IsFullscreen()) {
								CD3DDevice9::Get()->ResetDevice(true, 0, 0, 0);
							}

							bool bWindowCreated = false;

							// BT - 9/17 - Fixing the window frame that is shown around the movie when played in the intro screen.
							if (m_pengine->IsFullscreen() == false) {
								DDVid->m_hWnd = FindWindow(NULL, TrekWindow::GetWindowTitle());
							}
							else {
								//this window will have our "intro" in it...
								DDVid->m_hWnd = ::CreateWindow("MS_ZLib_Window", "Intro", WS_VISIBLE | WS_POPUP, 0, 0,
									GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN), NULL, NULL,
									::GetModuleHandle(NULL), NULL);

								bWindowCreated = true;
							}

							// BT - 9/17 - Replaced with the above.
							//DDVid->m_hWnd =  GetHWND();

							bool bOk = true;
							ZString pathStr = GetModeler()->GetArtPath() + "/intro_movie.avi"; //this can be any kind of AV file
							if(SUCCEEDED(DDVid->Play(pathStr,!m_pengine->IsFullscreen()))) //(Type WMV2 is good as most systems will play it)  
							{ 
								GetAsyncKeyState(VK_LBUTTON); GetAsyncKeyState(VK_RBUTTON);
								::ShowCursor(FALSE);
								while( DDVid->m_Running && bOk) //imago windooooow #112 7/10
								{
									if(!DDVid->m_pVideo->IsPlaying() || GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState(VK_SPACE) || 
										GetAsyncKeyState(VK_RETURN) || GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_RBUTTON))
									{
										DDVid->m_Running = FALSE;
										DDVid->m_pVideo->Stop();
									} else	{
										DDVid->m_pVideo->Draw(DDVid->m_lpDDSBack);
										if (m_pengine->IsFullscreen()) {
											DDVid->m_lpDDSPrimary->Flip(0,DDFLIP_WAIT);
										} else {
											bOk = DDVid->Flip();
										}
									}
								}

								::ShowCursor(TRUE);
								DDVid->DestroyDDVid();
								
							} else {
								DDVid->DestroyDirectDraw();
							}

							// BT - 9/17 - Clean up the movie window.
							if (bWindowCreated == true)
								::DestroyWindow(DDVid->m_hWnd);


							if (m_pengine->IsFullscreen()) {
								CD3DDevice9::Get()->ResetDevice(false);
							}
						}
						GetWindow()->screen(ScreenIDIntroScreen);
						SetUiScreen(CreateIntroScreen(GetModeler(), *m_pUiEngine, m_pUseOldUi->GetValue()));
	                    break;
					}


                case ScreenIDTrainScreen:
					SetUiScreen(CreateTrainingScreen(GetModeler()));
                    break;

                case ScreenIDZoneClubScreen:
					SetUiScreen(CreateZoneClubScreen(GetModeler(), GetTime()));
                    break;

                case ScreenIDSquadsScreen:
					SetUiScreen(CreateSquadsScreen(GetModeler(), szPlayerName ? PCC(strPlayerName) : NULL, idPlayer, szOther ? PCC(strOther) : NULL));
                    break;

                case ScreenIDGameStarting:
					SetUiScreen(CreateGameStartingScreen(GetModeler()));
                    break;

                case ScreenIDCharInfo:
                    if(idPlayer==-1)
                        idPlayer = trekClient.GetZoneClubID();
					SetUiScreen(CreateCharInfoScreen(GetModeler(), idPlayer));
                    break;

                case ScreenIDTrainSlideshow:
                {
                    extern  TRef<ModifiableNumber>  g_pnumberMissionNumber;
                    int     iMission = static_cast<int> (g_pnumberMissionNumber->GetValue ());
                    ZAssert ((iMission >= 1) && (iMission <= 8)); //TheBored 06-JUL-07: second condition must be (iMission <= (number of training missions))
                    char*   strNamespace[] =
                    {
                        "",
                        "",
                        "tm_2_basic_flight",
                        "tm_3_basic_weaponry",
                        "tm_4_enemy_engagement",
                        "tm_5_command_view",
                        "tm_6_practice_arena",
						"", //TheBored 06-JUL-07: Mish #7, blank because its never used
						"tm_8_nanite", //TheBored 06-JUL-07: Mish #8 pregame panels.
                    };

					SetUiScreen(CreateTrainingSlideshow (GetModeler (), strNamespace[iMission], iMission));
                    break;
                }

                case ScreenIDPostTrainSlideshow:
                {
                    extern  TRef<ModifiableNumber>  g_pnumberMissionNumber;
                    int     iMission = static_cast<int> (g_pnumberMissionNumber->GetValue ());
                    ZAssert ((iMission >= 1) && (iMission <= 10)); //TheBored 06-JUL-07: second condition must be (iMission <= (number of training missions))
                    char*   strNamespace[] =
                    {
                        "",
                        "tm_1_introduction",
                        "tm_2_basic_flight_post",
                        "tm_3_basic_weaponry_post",
                        "tm_4_enemy_engagement_post",
                        "tm_5_command_view_post",
                        "tm_6_practice_arena_post",
                        "", //TheBored 06-JUL-07: Mish #7, blank because its never used
                        "tm_8_nanite_post", //TheBored 06-JUL-07: Mish #8 postgame panels
                        "",
                        "tm_6_practice_arena_post" //05-NOV-17: FreeFlight pseudo training mission - use same post slides as tm6 (showing station instead of text in ui-stretch beta)
                    };
					SetUiScreen(CreatePostTrainingSlideshow (GetModeler (), strNamespace[iMission]));
                    break;
                }

                case ScreenIDZoneEvents:
					SetUiScreen(CreateZoneEventsScreen(GetModeler()));
                    break;

                case ScreenIDLeaderBoard:
					SetUiScreen(CreateLeaderBoardScreen(GetModeler(), strPlayerName));
                    break;

                default:
                    ZError("Bad screen id");
                    break;
            }

            m_screen = s;
        }

        bSwitchingScreens = false;

        if (s != sNextScreen)
            screen(sNextScreen, NULL);
    }

    void        CharInfoScreenForPlayer(int idZone)
    {
        screen(ScreenIDCharInfo, idZone);
    }

    void        SquadScreenForPlayer(const char * szName, int idZone, const char * szSquad)
    {
        screen(ScreenIDSquadsScreen, idZone, szName, szSquad);
    }

    void        LeaderBoardScreenForPlayer(const ZString & strCharacter)
    {
        screen(ScreenIDLeaderBoard, -1, PCC(strCharacter));
    }

    void OpenPopup(IPopup* ppopup, const Rect& rect)
    {
        GetPopupContainer()->OpenPopup(ppopup, rect);
        m_ptrekInput->SetFocus(false);
    }

    void OpenPopup(IPopup* ppopup, const Point& point)
    {
        GetPopupContainer()->OpenPopup(ppopup, point);
        m_ptrekInput->SetFocus(false);
    }

    bool IsProbablyNotForChat (int vk)
    {
        switch (vk)
        {
            case VK_PRIOR:
            case VK_NEXT:
            case VK_END:
            case VK_HOME:
            case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            case VK_SELECT:
            case VK_PRINT:
            case VK_EXECUTE:
            case VK_SNAPSHOT:
            case VK_INSERT:
            case VK_DELETE:
            case VK_NUMPAD0:
            case VK_NUMPAD1:
            case VK_NUMPAD2:
            case VK_NUMPAD3:
            case VK_NUMPAD4:
            case VK_NUMPAD5:
            case VK_NUMPAD6:
            case VK_NUMPAD7:
            case VK_NUMPAD8:
            case VK_NUMPAD9:
            case VK_MULTIPLY:
            case VK_ADD:
            case VK_SEPARATOR:
            case VK_SUBTRACT:
            case VK_DECIMAL:
            case VK_DIVIDE:
            case VK_F1:
            case VK_F2:
            case VK_F3:
            case VK_F4:
            case VK_F5:
            case VK_F6:
            case VK_F7:
            case VK_F8:
            case VK_F9:
            case VK_F10:
            case VK_F11:
            case VK_F12:
            case VK_F13:
            case VK_F14:
            case VK_F15:
            case VK_F16:
            case VK_F17:
            case VK_F18:
            case VK_F19:
            case VK_F20:
            case VK_F21:
            case VK_F22:
            case VK_F23:
            case VK_F24:
            case VK_NUMLOCK:
            case VK_SCROLL:
                return true;
            default:
                return false;
        }
    }

    bool IsInputEnabled(int vk)
    {
        return (m_screen != ScreenIDCombat) || IsProbablyNotForChat (vk) || !m_pconsoleImage->IsComposing ();
    }

	// BT - 9/17 - Made this a function to support chaining the opening microsoft splash with the longer classic
	// allegiance movie. 
	HANDLE PlayMovieClip(bool playMovies, bool isWindowed, ZString moviePath)
	{
		HANDLE hDDVidThread = 0;

		if (!g_bQuickstart && playMovies && !g_bReloaded &&
			::GetFileAttributes(moviePath) != INVALID_FILE_ATTRIBUTES &&
			!CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID) {
			//Imago only check for these if we have to 8/16/09
			HMODULE hVidTest = ::LoadLibraryA("WMVDECOD.dll");
			HMODULE hAudTest = ::LoadLibraryA("wmadmod.dll");
			bool bWMP = (hVidTest && hAudTest) ? true : false;
			::FreeLibrary(hVidTest); ::FreeLibrary(hAudTest);
			if (bWMP) {
				if (!CD3DDevice9::Get()->IsWindowed()) {
					::ShowWindow(GetHWND(), SW_HIDE);
				}

				//#112 windowed 7/10 Imago
				PlayVideoInfo * pData = new PlayVideoInfo;
				pData->pathStr = moviePath;
				pData->bWindowed = CD3DDevice9::Get()->IsWindowed();

				hDDVidThread = CreateThread(NULL, 0, DDVidCreateThreadProc, (void *)pData, THREAD_PRIORITY_HIGHEST, 0);
			}
		}

		return hDDVidThread;
	}

	TrekWindowImpl(
		EffectApp*     papp,
		const ZString& strCommandLine,
		// BUILD_DX9
		const ZString& strArtPath,
		// BUILD_DX9
		bool           bMovies
	) :
		TrekWindow(
			papp,
			strCommandLine,
			false, // BT - 10/17 - Set to always start windowed, then go full screen after game is initialized. Trying to find the source of the mystery "crash on launch" issues.
			WinRect(0 + CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetX,
				0 + CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetY,
				CD3DDevice9::Get()->GetCurrentMode()->mode.Width +
				CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetX,
				CD3DDevice9::Get()->GetCurrentMode()->mode.Height +
				CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetY),
			WinPoint(800, 600)
		),

		m_screen(ScreenIDSplashScreen),
		m_bShowMeteors(true),
		m_bShowStations(true),
		m_bShowProjectiles(true),
		m_bShowAlephs(true),
		m_bShowShips(true),
		m_bBidirectionalLighting(true),
		m_color(Color::White()),
		m_colorAlt(Color::White()),
		m_ambientLevel(0),
		m_ambientLevelBidirectional(0),
		m_frameID(0),
		m_timeLastFrame(Time::Now()),
		m_timeLastDamage(Time::Now()),
		m_cm(cmCockpit),
		m_cmOld(cmCockpit),
		m_timeRejectQueuedCommand(0),
		m_cmPreviousCommand(cmExternalCommandView34),
		m_bPreferChaseView(false),
		m_distanceExternalCamera(s_fExteralViewDistanceDefault),
		m_distanceCommandCamera(s_fCommandViewDistanceDefault),
		m_rollCommandCamera(0.0f),
		m_bEnableDisplacementCommandView(true),
		m_suicideCount(0),
		m_bLensFlare(true),
		m_bRoundRadar(false),
		m_bLinearControls(true),
		m_bMusic(false),
		m_bCommandGrid(false),
		m_radarCockpit(RadarImage::c_rlDefault),
		m_radarCommand(RadarImage::c_rlAll),
		m_viewmode(vmUI),
		m_bOverlaysChanged(false),
		m_pszCursor(AWF_CURSOR_DEFAULT),
		m_nLastCountdown(c_nCountdownMax),
		m_ctLobbyChat(CHAT_EVERYONE),
		m_bTrackCommandView(false),
		m_bQuitComposing(true),
		m_bEnableVirtualJoystick(false),
		m_bFlipY(false),
		m_bEnableFeedback(true),
		m_aabmInvest(0),
		m_aabmCommand(0),
		//Imago 7/10
		m_bFFAutoCenter(false),
		m_iMouseAccel(0), //#215
		m_bShowInventoryPane(true), // BT - 10/17 - Map and Sector Panes are now shown on launch and remember the pilots settings on last dock. 
		m_bShowSectorMapPane(true),  // BT - 10/17 - Map and Sector Panes are now shown on launch and remember the pilots settings on last dock. 
        m_pUseOldUi(nullptr),
        m_bShowJoystickIndicator(true)
    {
        HRESULT hr;

		debugf("Setting up TrekWindow\n");

		// DXHACKS - Could cause issues...
		// Move this call here, so that engine initialisation is performed *AFTER* we have a valid HWND.
		papp->Initialize(strCommandLine, GetHWND());
		
		m_pengine = papp->GetEngine();
		m_pmodeler = papp->GetModeler();

		debugf("Setting art path to: %s\n", (PCC) strArtPath);


        std::vector<ZString> vArtPaths;
        {
            //Go through the mod directory and add each directory in there

            std::string search_path = "./Mods/*";
            WIN32_FIND_DATA fd;
            HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != '.') {
                        vArtPaths.push_back(ZString("./Mods/") + fd.cFileName);
                    }
                } while (::FindNextFile(hFind, &fd));
                ::FindClose(hFind);
            }
        }

        //old and deprecated highres directory
        vArtPaths.push_back(strArtPath + "/Textures");

        //main artwork directory
        vArtPaths.push_back(strArtPath);

        // Now set the art path, performed after initialise, else Modeler isn't valid.
        auto pFileLoader = CreateSecureFileLoader(
            vArtPaths,
            strArtPath
        );
        GetModeler()->SetFileLoader(pFileLoader);

        GetModeler()->SetArtPath(strArtPath); //some functionality relies on the artpath

        UiEngine::SetGlobalFileLoader(pFileLoader);

        if (g_bLuaDebug) {
            UiEngine::m_stringLogPath = (std::string)"lua.log";
        }

		//Imago 6/29/09 7/28/09 now plays video in thread while load continues // BT - 9/17 - Refactored a bit.
		HANDLE hDDVidThread = NULL;

		// BT - 9/17 - If you want to re-add an intro movie, you can uncomment this code, but it was causing some people to crash,
		// and most didn't like having any intro at all. :(
		// To make a movie that is compatible with the movie player, use this ffmpeg command line: 
		// ffmpeg.exe -i intro_microsoft_original.avi -q:a 1 -q:v 1 -vcodec mpeg4 -acodec wmav2 intro_microsoft.avi
        // Rock: Converted to configuration setting
        if (m_pConfiguration->GetBool("Ui.ShowStartupCreditsMovie", false)->GetValue()) {
            ZString pathStr = GetModeler()->GetArtPath() + "/intro_microsoft.avi";
            hDDVidThread = PlayMovieClip(bMovies, CD3DDevice9::Get()->IsWindowed(), pathStr);
        }

		debugf("Reading FFGain, MouseSensitivity\n");

		m_pnumFFGain = new ModifiableNumber((float)LoadPreference("FFGain", 10000)); //Imago #187 
		m_pnumMouseSens = new ModifiableNumber(atof(LoadPreference("MouseSensitivity", "0.6")));

        m_pinputEngine->GetMouse()->SetSensitivity(m_pnumMouseSens->GetValue());

		debugf("TrekResources::Initialize() - Loading fonts.\n");

		// load the fonts
		TrekResources::Initialize(GetModeler());

		debugf("performing PostWindowCreationInit.\n");

		// Perform post window creation initialisation. Initialise the time value.
		PostWindowCreationInit( );
		InitialiseTime();

        if (!IsValid()) {
            return;
        }

		debugf("Setting up effects window.\n");

        SetEffectWindow(this);

		debugf("Setting up modeler site.\n");

        GetModeler()->SetSite(this);

        //  , global pointer to the one and only window
        //             should eventually be able to get rid of this

        ZAssert(g_ptrekWindow == NULL);
        g_ptrekWindow = this;

        //
        // App Icon
        //

        //SetIcon(LoadIcon(NULL, MAKEINTRESOURCE(0)));

        LPCTSTR pszRes = MAKEINTRESOURCE(10);
        HICON hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), pszRes, IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        SendMessage(WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIcon);

        HICON hIconSmall = (HICON)::LoadImage(GetModuleHandle(NULL), pszRes, IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        SendMessage(WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIconSmall);

        //
        // Event sink delegates
        //

		debugf("Creating event sinks. \n");

        m_pClientEventSink  = IClientEventSink::CreateDelegate(this);
        m_pintegerEventSink = IIntegerEventSink::CreateDelegate(this);
        m_pmenuCommandSink  = IMenuCommandSink::CreateDelegate(this);
        m_psubmenuEventSink = ISubmenuEventSink::CreateDelegate(this);

        //
        // advise us of client notifications
        //

		debugf("Getting event sources. \n");

        m_pClientEventSource = trekClient.GetClientEventSource();
        m_pClientEventSource->AddSink(m_pClientEventSink);

		debugf("Creating sound mutexes. \n");

        //
        // Initialize some of the basic sound stuff
        //

        ZSucceeded(CreateSoundMutex(m_psoundmutexSal));
        ZSucceeded(CreateSoundMutex(m_psoundmutexVO));

        //
        // Load our custom controls for .MDL screens
        //

        TRef<INameSpace> pnsSound = CreateSoundNameSpace(GetModeler(), GetModeler()->GetNameSpace("effect"));

        TRef<INameSpace> pnsGamePanes =
            GetModeler()->CreateNameSpace(
                "gamepanes",
                pnsSound
            );

        ::ExportPaneFactories(pnsGamePanes);
        pnsGamePanes->AddMember("ShowChatHistoryHUD", m_pboolChatHistoryHUD = new ModifiableBoolean(true));
        pnsGamePanes->AddMember("ShowCenterHUD", m_pboolCenterHUD = new ModifiableBoolean(true));
        pnsGamePanes->AddMember("ShowTargetHUD", m_pboolTargetHUD = new ModifiableBoolean(true));

        pnsGamePanes->AddMember(
            "StyleHUD",
            m_pwrapNumberStyleHUD = new WrapNumber(
                m_pnumberStyleHUD = new ModifiableNumber(0)
            )
        );

		// #294 - Turkey 
		m_pnumberChatLinesDesired = new ModifiableNumber(0);
		pnsGamePanes->AddMember("NumChatLines", m_pwrapNumberChatLines = new WrapNumber(m_pnumberChatLines = new ModifiableNumber(0)));

        pnsGamePanes->AddMember("Flash", m_pnumberFlash = new ModifiableNumber(0));
        pnsGamePanes->AddMember("TeamPaneCollapsed", m_pnumberTeamPaneCollapsed = new ModifiableNumber(0));
        pnsGamePanes->AddMember("IsGhost", m_pnumberIsGhost = new ModifiableNumber(0));

        pnsGamePanes->AddMember("playerThrottle", m_pplayerThrottle = new ModifiableNumber(0));
        pnsGamePanes->AddMember("hudColor",       m_pcolorHUD       = new ModifiableColorValue(Color::Black()));
        pnsGamePanes->AddMember("hudColorshadows",m_pcolorHUDshadows       = new ModifiableColorValue(Color::Black()));
        pnsGamePanes->AddMember("targetHudColor", m_pcolorTargetHUD = new ModifiableColorValue(Color::Black()));

        pnsGamePanes->AddMember("SFXGain", m_pnumSFXGain =
            new ModifiableNumber(-(float)LoadPreference("SFXGain", 8)));
        pnsGamePanes->AddMember("VoiceOverGain", m_pnumVoiceOverGain =
            new ModifiableNumber(-(float)LoadPreference("VoiceOverGain", 13)));
        pnsGamePanes->AddMember("MutexSal", m_psoundmutexSal);
        pnsGamePanes->AddMember("MutexVO", m_psoundmutexVO);

        SoundInit::AddMembers(pnsGamePanes);

        //
        // Target Image
        //

        m_cameraControl.m_pcameraTarget  = new Camera();
        m_cameraControl.m_pgeoTarget     = Geo::GetEmpty();
        m_cameraControl.m_pwrapGeoTarget = new WrapGeo(m_cameraControl.m_pgeoTarget),
        m_cameraControl.m_porientTransformTarget = new MatrixTransform(Matrix::GetIdentity());

        // initialize for chase view
        m_turnRateSampleIndex = 0;

        pnsGamePanes->AddMember(
            "targetGeo",
            new TransformGeo(
                m_cameraControl.m_pwrapGeoTarget,
                m_cameraControl.m_porientTransformTarget
            )
        );
        pnsGamePanes->AddMember("targetCamera", m_cameraControl.m_pcameraTarget);

        //
        // Keyboard input
        //

        m_psuperKeyboardInputFilter = new SuperKeyboardInputFilter(this);
        AddKeyboardInputFilter(m_psuperKeyboardInputFilter);

        //
        // Popup keyboard input
        //

        AddKeyboardInputFilter(GetPopupContainer());
        m_pkeyboardInput = IKeyboardInput::CreateDelegate(this);
        SetFocus(m_pkeyboardInput);

        //
        // Filter the keyboard input
        //

        m_pkeyboardInputFilter = new KeyboardInputFilter(this);
        AddKeyboardInputFilter(m_pkeyboardInputFilter);

        //
        // Create the virtual joystick image
        //

        m_pjoystickImage = CreateJoystickImage(1.0f / 512.0f);

        //
        // Initialize what's needed to show the splash screen
        //

        InitializeImages();
		
		if (hDDVidThread != NULL) { //imago 7/29/09 intro.avi
			if (!CD3DDevice9::Get()->IsWindowed()) {
				CD3DDevice9::Get()->ResetDevice(false,800,600,g_DX9Settings.m_refreshrate);
			}
		}

        //
        // initialize the sound engine (for the intro music if nothing else)
        //

        DWORD dwSoundInitStartTime = timeGetTime();

		m_bUseDSound8 = LoadPreference("UseDSound8", true);

        if (g_bEnableSound) {
            assert (m_pSoundEngine == NULL);
            hr = CreateSoundEngine(m_pSoundEngine, GetHWND(), m_bUseDSound8);
        }

        if (FAILED(hr) || !g_bEnableSound)
        {
            hr = CreateDummySoundEngine(m_pSoundEngine);
            ZAssert(SUCCEEDED(hr));
        }
        m_soundquality = (ISoundEngine::Quality)LoadPreference("SoundQuality",
            ISoundEngine::midQuality);
        ZSucceeded(m_pSoundEngine->SetQuality(m_soundquality));

        m_bEnableSoundHardware = LoadPreference("SoundHardwareAcceleration", false) != FALSE;
        ZSucceeded(m_pSoundEngine->EnableHardware(m_bEnableSoundHardware));

        ZSucceeded(m_pSoundEngine->SetListener(new CameraListener(m_cameraControl)));

        InitializeSoundTemplates();

        m_pUiEngine = UiEngine::Create(this, m_pengine, m_pSoundEngine, [this](std::string strWebsite) {
            this->ShowWebPage(strWebsite.c_str());
        });

        //
        // Load the Quick Chat Info
        //

        InitializeQuickChatMenu();

        debugf("Time initializing sound: %d ms\n", timeGetTime() - dwSoundInitStartTime);

        //
        // Load Input toggles
        //

        m_bEnableVirtualJoystick = (LoadPreference("EnableVirtualJoystick", 0) != 0);
        m_bFlipY                 = (LoadPreference("FlipY",                 0) != 0);
        m_bEnableFeedback        = (LoadPreference("EnableFeedback",        1) != 0);
        m_bFFAutoCenter			 = (LoadPreference("FFAutoCenter",			0) != 0); //Imago #187
		m_iMouseAccel			 = LoadPreference("MouseAcceleration",     0) % 3; // Imago #215 //#282 bugfix
		m_iWheelDelay			 = LoadPreference("WheelDelay",            2) % 5; //Spunky #282

        m_pUseOldUi = m_pConfiguration->GetBool("Ui.UseOldUi", true);

        m_bShowJoystickIndicator = (LoadPreference("ShowJoystickIndicator", 1) != 0);

        GetInputEngine()->GetMouse()->SetAccel(m_iMouseAccel);

// BUILD_DX9

        //
        // Create the combat camera
        //

        m_pcamera        = new Camera();
        m_pcameraPosters = new Camera();
        //m_pcameraTurret  = new Camera();

        m_cameraControl.SetCameras(
            m_pcamera,
            m_pcameraPosters/*,
            m_pcameraTurret*/
        );

        m_orientationExternalCamera.Reset();

        //
        // The viewports
        //

        m_pviewport        = new Viewport(m_pcamera,        GetRenderRectValue());
        m_pviewportPosters = new Viewport(m_pcameraPosters, GetRenderRectValue());
        //m_pviewportTurret  = new Viewport(m_pcameraTurret,  GetRenderRectValue());

        //
        // put some Debris into the scene
        //
		m_debrisDensity = new ModifiableNumber(atof(LoadPreference("Debris", "1.0"))); //variable debris - LANS
        m_pgeoDebris = CreateDebrisGeo(GetModeler(), GetTime(), m_pviewport, m_debrisDensity);

        //
        // Command View
        //

        m_pCommandVisibleGeo = new VisibleGeo(m_pCommandGeo = new CommandGeo(c_fCommandGridRadius, 0.0f, 40));
        m_pCommandVisibleGeo->SetVisible(false);

        //
        // The 3D Scene
        //

        m_pgeoScene = new WrapGeo(Geo::GetEmpty());
        m_pimageScene =
            new GeoImage(
                GroupGeo::Create(
                    m_pwrapGeoDebris = new WrapGeo(m_pgeoDebris),
                    m_pgeoScene,
                    m_pCommandVisibleGeo
                ),
                m_pviewport,
                true
            );

		//LANS - zero out debris if option is off
		if (m_debrisDensity->GetValue() == 0.0f) {
			m_pwrapGeoDebris->SetGeo(Geo::GetEmpty());
		}

        UpdateBidirectionalLighting();

        //
        // The environment sphere
        //

        m_pimageEnvironment = new GeoImage(Geo::GetEmpty(), m_pviewportPosters, false);

        //
        // A solid black image
        //

        m_pimageBlack = CreateColorImage(new ColorValue(Color::Black()));

        //
        // Turret
        //

        //m_pgeoTurret = Geo::GetEmpty();
        //m_pmtTurret = new MatrixTransform(Matrix::GetIdentity());

        //m_pimageTurret = new GeoImage(m_pgeoTurret, m_pviewportTurret, false);

        //
        // The background stars
        //

        m_pimageStars = StarImage::Create(m_pviewportPosters, 500);

        //
        // The Lens Flare
        //

        m_pimageLensFlare = CreateLensFlareImage(GetModeler(), m_pviewportPosters);

        //
        // The muzzle flare
        //

        m_pmuzzleFlareImage = CreateMuzzleFlareImage(GetModeler(), GetTime());

        //
        // The HUD
        //

        m_pradarImage = RadarImage::Create(GetModeler(), m_pviewport);

        m_pgroupImageHUD = new GroupImage();

        m_pgroupImageHUD->AddImage(CreateIndicatorImage(GetModeler(), m_pviewport, GetTime()));
        m_pgroupImageHUD->AddImage(m_pwrapImageRadar   = new WrapImage(m_pradarImage));
        //m_pgroupImageHUD->AddImage(m_pwrapImageTurret = new WrapImage(Image::GetEmpty()));

        //
        // Create an LODScrollBar
        //

        TRef<Pane> ppane = CreateScrollPane(WinPoint(128, 10), 100, 10, 1, 90, m_pscrollPane, m_peventLOD);
        m_peventLOD->AddSink(m_pintegerEventSink);

        m_pimageLOD =
            CreatePaneImage(
                GetEngine(),
                false,
                ppane
            );

        //
        // Wrap up the images that can be toggled
        //

        m_pwrapImageHudGroup    = new WrapImage(m_pgroupImageHUD);
        m_pwrapImageLensFlare   = new WrapImage(m_pimageLensFlare);
        m_pwrapImageScene       = new WrapImage(m_pimageScene);
        m_pwrapImageStars       = new WrapImage(m_pimageStars);
        m_pwrapImageEnvironment = new WrapImage(m_pimageEnvironment);
        m_pwrapImagePosters =
            new WrapImage(
                m_pwrapImagePostersInside = new WrapImage(
                    Image::GetEmpty()
                )
            );

        //
        // Group all of the image layers together, starting from the front.
        //

        m_pgroupImage3D = new GroupImage();

        m_pgroupImage3D->AddImage(m_pwrapImageHudGroup   );
        m_pgroupImage3D->AddImage(m_pmuzzleFlareImage    );
		//m_pgroupImage3D->AddImage(m_pwrapImageLensFlare  ); // Your_Persona: this line was moved down one line to move it up in the draw order.
        m_pgroupImage3D->AddImage(m_pwrapImageScene      );
		m_pgroupImage3D->AddImage(m_pwrapImageLensFlare  );// moved to here
        m_pgroupImage3D->AddImage(m_pwrapImagePosters    );
        m_pgroupImage3D->AddImage(m_pwrapImageStars      );

        m_pgroupImage3D->AddImage(m_pwrapImageEnvironment);

        m_pwrapImageConsole     = new WrapImage(Image::GetEmpty());
        m_pwrapImageBackdrop    = new WrapImage(m_pgroupImage3D);

        m_pgroupImageGame = new GroupImage();

        m_pgroupImageGame->AddImage(m_pwrapImageConsole);
        m_pgroupImageGame->AddImage(m_pwrapImageBackdrop);
        m_pgroupImageGame->AddImage(m_pimageBlack       );


        //
        // Initialize IGC
        //

        trekClient.Initialize(Time::Now());

        //
        // Load the sounds
        //

        m_ptrekInput = CreateTrekInput(GetModuleHandle(NULL), GetHWND(), GetInputEngine(), m_pjoystickImage);

        //
        // Initialize VT.
        //

        m_cVTVersion = 0;
        m_hwndVTEdit = ::CreateWindow(TEXT("EDIT"),
                                        TEXT("VTEdit"),
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        TrekWindow::GetHWND(),
                                        NULL,
                                        GetModuleHandle(NULL),
                                        NULL);

        //
        // Load saved settings
        //

        if (!LoadPreference("CensorChats", TRUE))
            ToggleCensorChats();
        if (LoadPreference ("PreferChaseView", FALSE))
            ToggleStickyChase ();
        if (!LoadPreference("FilterChatsToAll", TRUE))
            ToggleFilterChatsToAll();
        if (!LoadPreference("FilterQuickComms", TRUE))
            ToggleFilterQuickComms();
		if (!LoadPreference("FilterUnknownChats", TRUE))
            ToggleFilterUnknownChats(); //TheBored 30-JUL-07: Filter Unknown Chat patch
		if (!LoadPreference("LinearControlResponse", FALSE)) // BT - 8/17 Set to quadratic by default.
            ToggleLinearControls();
        if (!LoadPreference("Environment", TRUE) || IsWine())  //imago 9/19/09 force env in wine 8/16/09
            ToggleEnvironment();
        if (!LoadPreference("Posters", TRUE))
            TogglePosters();
        if (!LoadPreference("Stars", TRUE))
            ToggleStars();
        if (!LoadPreference("Strobes", TRUE))
            ToggleStrobes();
        if (!LoadPreference("Trails", TRUE))
            ToggleTrails();
        if (!LoadPreference("Bounds", TRUE))
            ToggleBounds();
        if (!LoadPreference("TransparentObjects", TRUE))
            ToggleTransparentObjects();
        SetSmoke (LoadPreference ("SmokeEffects", 3));
        if (!LoadPreference("Lens Flare", TRUE))
            ToggleLensFlare();
        if (!LoadPreference("BidirectionalLighting", TRUE))
            ToggleBidirectionalLighting();
        if (!LoadPreference("ChatHistory", TRUE))
            ToggleChatHistoryHUD();
        if (!LoadPreference("CenterHUD", TRUE))
            ToggleCenterHUD();
        if (!LoadPreference("TargetHUD", TRUE))
            ToggleTargetHUD();
        if (LoadPreference("SoftwareHUD", FALSE))  //All we need with two styles
            CycleStyleHUD();
		SetDeadzone(LoadPreference("DeadZone", 5)); //ToggleLargeDeadZone(); //Imago updated 7/8/09 // BT 8/17 - Small deadzone default.
		SetRadarLOD(LoadPreference("RadarLOD", 0)); //Imago updated 7/8/09 #24 (Gamma, VirtualJoystick, RadarLOD, ShowGrid)
		if (LoadPreference("ShowGrid", FALSE))
			ToggleShowGrid();
		SetGamma(LoadPreference("Gamma", "1.13"));
	    if (LoadPreference("VirtualJoystick", TRUE)) // BT - 10/17 - Enable virtual JS by default, not many people have joysticks now-a-days.
			ToggleVirtualJoystick();

		ToggleFilterLobbyChats(LoadPreference("FilterLobbyChats", 0)); //TheBored 25-JUN-07: Mute lobby chat patch // mmf 04/08 default this to 0

		// #294 - Turkey
		SetChatLines(LoadPreference("ChatLines", 10));

		/* pkk May 6th: Disabled bandwidth patch
		ToggleBandwidth(LoadPreference("Bandwidth",32)); // w0dk4 June 2007: Bandwith Patch - Increase default to max Imago 8/10*/

        //
        // Help
        //

        InitializeHelp();

        // initialize the bad words filters
        LoadBadWords ();

        m_pmissileLast = 0;

        //
        // intro.avi video moved up
        //
		TRef<Screen> introscr = CreateIntroScreen(GetModeler(), *m_pUiEngine, m_pUseOldUi->GetValue());
		SetUiScreen(introscr);
        m_screen = ScreenIDIntroScreen;
        RestoreCursor();

        // if the startup credits are running, wait.
        if (hDDVidThread != NULL) {
            WaitForSingleObject(hDDVidThread, INFINITE);
            CloseHandle(hDDVidThread);
        }

        auto pShowIntroMovie = m_pConfiguration->GetBool("Ui.ShowStartupIntroMovie", false);
        if (pShowIntroMovie->GetValue()) {
            //only show on first run
            pShowIntroMovie->SetValue(false);

            ZString pathMovieStr = "";

            // To make a movie that is compatible with the movie player, use this ffmpeg command line: 
            // ffmpeg.exe -i intro_microsoft_original.avi -q:a 1 -q:v 1 -vcodec mpeg4 -acodec wmav2 intro_microsoft.avi
            pathMovieStr = GetModeler()->GetArtPath() + "/intro_movie.avi";

            hDDVidThread = PlayMovieClip(bMovies, CD3DDevice9::Get()->IsWindowed(), pathMovieStr);

            if (hDDVidThread != NULL)
            {
                WaitForSingleObject(hDDVidThread, INFINITE);
                CloseHandle(hDDVidThread);
            }

            if (!CD3DDevice9::Get()->IsWindowed()) {
                ::ShowWindow(GetHWND(), SW_SHOWMAXIMIZED);
            }
        }
    }

    void InitializeImages()
    {
        m_pwrapImageTop = new WrapImage(Image::GetEmpty());
        m_pwrapImageLOD = new WrapImage(Image::GetEmpty());
		m_pwrapImageHelp = new WrapImage(Image::GetEmpty());

        m_pgroupImage = new GroupImage();
        m_pgroupImage->AddImage(m_pjoystickImage);
        m_pgroupImage->AddImage(m_pwrapImageLOD);
        m_pgroupImage->AddImage(GetPopupContainer()->GetImage());
        m_pgroupImage->AddImage(m_pwrapImageHelp);
        m_pgroupImage->AddImage(m_pwrapImageTop);
        SetImage(m_pgroupImage);
    }

    void SetCursor(const char* pszCursor)
    {
        if (m_pszCursor != pszCursor)       //Assume they are all static strings that are combined so pointers are all we need
        {
            m_pszCursor = pszCursor;

            if (GetCursorImage() != Image::GetEmpty())
                RestoreCursor();
        }
    }

    void RestoreCursor()
    {
        TRef<Image> pimageCursor;
        CastTo(pimageCursor, (Value*)GetModeler()->GetNameSpace("cursor")->FindMember(m_pszCursor));
        assert(pimageCursor);
        SetCursorImage(pimageCursor);
    }

    void SetWaitCursor()
    {
        TRef<Image> pimageCursor;
        CastTo(pimageCursor, (Value*)GetModeler()->GetNameSpace("cursor")->FindMember(AWF_CURSOR_WAIT));
        assert(pimageCursor);
        SetCursorImage(pimageCursor);
    }

    void SavePreference(const ZString& szName, DWORD dwValue)
    {
        GetConfiguration()->SetIntValue(std::string(szName), (int)dwValue);
    }

    DWORD LoadPreference(const ZString& szName, DWORD dwDefault)
    {
        return (DWORD)GetConfiguration()->GetIntValue(std::string(szName), (int)dwDefault);
    }

    void SavePreference(const ZString& szName, const ZString& strValue)
    {
        GetConfiguration()->SetStringValue(std::string(szName), std::string(strValue));
    }

    ZString LoadPreference(const ZString& szName, const ZString& strDefault)
    {
        return GetConfiguration()->GetStringValue(std::string(szName), std::string(strDefault)).c_str();
    }

	bool IsWine() { //Imago 8/17/09
		HKEY hKey;
		if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_READ, &hKey)) {
			::RegCloseKey(hKey);
			return true;
		}
		return false;
	}

    void Terminate()
    {
		SetGamma(ZString(GetEngine()->GetGammaLevel())); //imago 7/8/09 #24

        //
        // Terminate VT.
        //
        if (NULL != m_hwndVTEdit)
        {
            if (TRUE == ::DestroyWindow(m_hwndVTEdit))
                m_hwndVTEdit = NULL;
        }

        // close all popups (a potential circular reference)
        if (!GetPopupContainer()->IsEmpty())
            GetPopupContainer()->ClosePopup(NULL);
        m_pmessageBox = NULL;
        m_pmessageBoxLockdown= NULL;

        m_pClientEventSource->RemoveSink(m_pClientEventSink);

        RemoveKeyboardInputFilter(GetPopupContainer());
        RemoveKeyboardInputFilter(m_pkeyboardInputFilter);

        trekClient.Terminate();

		//imago removed for Visual Studio 2008 Express users (lacks ATL/COM) - we're not using TM7 anyways 6/22/09
        // clean up after the training mission if we need to
        //extern  void KillTrainingStandaloneGame (void);
        //KillTrainingStandaloneGame ();

        m_mapAnimatedImages.SetEmpty();

        TerminateGameStateContainer();
        m_pwrapImageTop->SetImage(Image::GetEmpty());
        m_pimageScreen     = NULL;
        m_pscreen          = NULL;
        SetCaption(NULL);

		// BT - 10/17 - Fixing 8982261	211206	allegiance.exe	allegiance.exe	tvector.h	362	13	0	Win32 StructuredException at 0058C1DA : UNKNOWN	2017-10-08 14:33:29	0x0018C1DA	10	UNKNOWN
		// Not sure why we need to set the image to empty when the window is closing down anyway. 
		// This causes a crash on some machines as the underlying TVector is already gone when this part of the code
		// is reached. 
		if (m_screen == ScreenIDCombat)
		{
			SetImage(Image::GetEmpty());
			m_pwrapImageConsole->SetImage(Image::GetEmpty());
			m_pwrapImageTop->SetImage(Image::GetEmpty());
		}

        m_pgeoScene          = NULL;
        m_pcamera            = NULL;
        m_pcameraPosters     = NULL;
        //m_pcameraTurret      = NULL;
        m_pimageScene        = NULL;
        m_pgeoDebris         = NULL;
        m_pimageStars        = NULL;
        m_pimageEnvironment  = NULL;
        m_pimageBlack        = NULL;
        //m_pimageTurret       = NULL;
        m_pimageLensFlare    = NULL;
        m_pwrapImageHudGroup = NULL;
        m_pwrapImageComm     = NULL;
        m_pwrapImageRadar    = NULL;
        m_pwrapImageConsole  = NULL;
        m_pgroupImageHUD     = NULL;
        m_pconsoleImage      = NULL;
        m_pradarImage        = NULL;
        m_pgroupImage        = NULL;
        m_pgroupImage3D      = NULL;
        m_pconsoleImage = NULL;

        m_vSoundMap.SetEmpty();
        m_vsonicChats.SetEmpty();
        m_pqcmenuMain = NULL;
        m_psoundmutexSal->Reset();
        m_psoundmutexVO->Reset();

        m_pUiEngine = NULL;
        m_pSoundEngine = NULL;
        m_ptrekInput = NULL;
    }

    void OnClose()
    {
        Terminate();
        EngineWindow::OnClose();
    }

    void CloseMessageBox ()
    {
        m_pmessageBox = NULL;
    }

    void DoClose()
    {
        PostMessage(WM_CLOSE);
    }

    TRef<IMessageBox> m_pmessageBox;

    void StartClose()
    {
		DoClose();
    }

    bool OnEvent(IIntegerEventSource* pevent, int value)
    {
        if (pevent == m_peventLOD) {
            ThingGeo::SetLODBias(((float)value) / 100);
        }
        return true;
    }

    void DoQuitMission()
    {
        if (Training::IsTraining ())
        {
            Training::EndMission ();
            Training::SetSkipPostSlideshow ();
        }
        else if (Slideshow::IsInSlideShow ())
        {
            screen (ScreenIDTrainScreen);
        }
        else
        {
            trekClient.QuitMission();

            SetWaitCursor();
            TRef<IMessageBox> pmsgBox = CreateMessageBox("Quitting team...", NULL, false, false, 1.0f);
            GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
    }

    class QuitMissionSink : public IIntegerEventSink {
    public:
        TrekWindowImpl* m_pwindow;

        QuitMissionSink(TrekWindowImpl* pwindow) :
            m_pwindow(pwindow)
        {
        }

        bool OnEvent(IIntegerEventSource* pevent, int value)
        {
            if (value == IDOK) {
                m_pwindow->DoQuitMission();
            }
            return false;
        }
    };

    void StartQuitMission()
    {
        TRef<IMessageBox> pmessageBox;
        if (Training::IsTraining() || Slideshow::IsInSlideShow())
            pmessageBox = CreateMessageBox("Quit the Current Training Mission?", NULL, true, true);
        else
            pmessageBox = CreateMessageBox("Quit the Current Game?", NULL, true, true);
        pmessageBox->GetEventSource()->AddSink(new QuitMissionSink(this));
        GetPopupContainer()->OpenPopup(pmessageBox, false);
    }

    TrekInput*  GetInput(void)
    {
        return m_ptrekInput;
    }

    void SetLightDirection(const Vector& direction)
    {
        m_pimageLensFlare->SetLightDirection(direction);
        m_pimageScene->SetLightDirection(direction);
    }

    void SetCluster(IclusterIGC*   pcluster)
    {
        if (pcluster)
        {
            m_pCommandGeo->SetCluster(pcluster);
            m_pgeoScene->SetGeo(pcluster->GetClusterSite()->GetGroupScene());
            m_pwrapImagePostersInside->SetImage(pcluster->GetClusterSite()->GetPosterImage());
            m_pimageEnvironment->SetGeo(
                CreateCopyModeGeo(
                    pcluster->GetClusterSite()->GetEnvironmentGeo()
                )
            );
            m_pimageStars->SetCount(pcluster->GetStarSeed(), pcluster->GetNstars());

            m_color = pcluster->GetLightColor();
            m_colorAlt = pcluster->GetLightColorAlt();
            m_ambientLevel = pcluster->GetAmbientLevel();
            m_ambientLevelBidirectional = pcluster->GetBidirectionalAmbientLevel();

            SetLightDirection(pcluster->GetLightDirection());
            UpdateBidirectionalLighting();

            // show the 3D view
            m_pwrapImageBackdrop->SetImage(m_pgroupImage3D);

            // if they switched away from command view before the cluster could be displayed...
            if (trekClient.GetViewCluster() && (m_viewmode == vmHangar || m_viewmode == vmLoadout))
            {
                // put them back in command mode
                SetViewMode(vmCommand);
            }
        }

        m_pnumberIsGhost->SetValue(trekClient.GetShip()->IsGhost() ? 1.0f : 0.0f);
    }

    void UpdateBackdropCentering()
    {
        //kg- #226 - todo -factorize with above code
        if (m_pimageScreen)
        {
            // center the pane on the screen
            const Rect& rectScreen = GetScreenRectValue()->GetValue();
            Point sizeScreenBeforeScaling;
            if (m_pscreen->GetPane())
            {
                const WinPoint& sizePane = m_pscreen->GetPane()->GetSize();
                sizeScreenBeforeScaling = Point(sizePane.X(), sizePane.Y());
            }
            else if (m_pscreen->GetImage()) {
                sizeScreenBeforeScaling = m_pscreen->GetImage()->GetBounds().GetRect().Size();
            }

			float scale;
			scale = std::min(rectScreen.XSize() / sizeScreenBeforeScaling.X(), rectScreen.YSize() / sizeScreenBeforeScaling.Y());

			Point pointTranslate;
			pointTranslate = Point(
				0.5 * (rectScreen.XSize() - sizeScreenBeforeScaling.X() * scale),
				0.5 * (rectScreen.YSize() - sizeScreenBeforeScaling.Y() * scale)
			);

			Matrix2 matrix = Matrix2::GetIdentity();
			matrix.SetScale(Point(scale, scale));
			matrix.Translate(pointTranslate);

			m_pMatrixTransformScreen->SetValue(matrix);
		}
	}

	void contextAcceptPlayer()
	{
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmPosAck, C, POSITIONACK)
        END_PFM_CREATE
        pfmPosAck->shipID = contextPlayerInfo->ShipID();
        pfmPosAck->fAccepted = true;
        pfmPosAck->iSide = trekClient.GetSideID();
	}

	void contextRejectPlayer()
	{
		// if we are booting a player who is already on our team...
       if (contextPlayerInfo->SideID() == trekClient.GetSideID())
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmQuitSide, CS, QUIT_SIDE)
            END_PFM_CREATE
            pfmQuitSide->shipID = contextPlayerInfo->ShipID();
            pfmQuitSide->reason = QSR_LeaderBooted;
        }
        else
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmPosAck, C, POSITIONACK)
            END_PFM_CREATE
            pfmPosAck->shipID = contextPlayerInfo->ShipID();
            pfmPosAck->fAccepted = false;
            pfmPosAck->iSide = trekClient.GetSideID();
        }
	}

	void contextMakeLeader()
	{
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmSetTeamLeader, CS, SET_TEAM_LEADER)
        END_PFM_CREATE
        pfmSetTeamLeader->sideID = trekClient.GetSideID();
        pfmSetTeamLeader->shipID = contextPlayerInfo->ShipID();
	}

	void contextMute()
	{
		contextPlayerInfo->SetMute(!contextPlayerInfo->GetMute());
	}

	// BT - STEAM - Enable moderators to ban players by context menu.
	void contextKickPlayer()
	{
		char szMessageParam[CB_ZTS];
		lstrcpy(szMessageParam, "You have been moved to NOAT by an administrator.");
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
		BEGIN_PFM_CREATE(trekClient.m_fm, pfmQuitSide, CS, QUIT_SIDE)
			FM_VAR_PARM(szMessageParam, CB_ZTS)
			END_PFM_CREATE
			pfmQuitSide->shipID = contextPlayerInfo->ShipID();
		pfmQuitSide->reason = QSR_SwitchingSides;
	}

	// BT - STEAM - Enable moderators to ban players by context menu.
	void contextBanPlayer()
	{
		char szMessageParam[CB_ZTS];
		lstrcpy(szMessageParam, "You have been banned from this game an administrator.");
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
		BEGIN_PFM_CREATE(trekClient.m_fm, pfmQuitSide, CS, QUIT_MISSION)
			FM_VAR_PARM(szMessageParam, CB_ZTS)
			END_PFM_CREATE
			pfmQuitSide->shipID = contextPlayerInfo->ShipID();
		pfmQuitSide->reason = QSR_AdminBooted;
	}
	
	void contextDockDrone() //Xynth #48 8/2010
	{
		//Spunky #250 #257
		bool docked=contextPlayerInfo->LastSeenState() == c_ssDocked; //GetStation() doesn't work here
		if (docked && !contextPlayerInfo->GetShip()->GetStayDocked() || !docked) 
			contextPlayerInfo->GetShip()->SetStayDocked(true);
 		else
			contextPlayerInfo->GetShip()->SetStayDocked(false);
	
		if (trekClient.m_fm.IsConnected())
		{
			trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
			BEGIN_PFM_CREATE(trekClient.m_fm, pfmOC, CS, ORDER_CHANGE)
			END_PFM_CREATE

			pfmOC->shipID = contextPlayerInfo->GetShip()->GetObjectID();			
			pfmOC->command = c_cmdAccepted;
			if (docked && contextPlayerInfo->GetShip()->GetStayDocked())
				pfmOC->commandID = c_cidDoNothing;
			else 
				pfmOC->commandID = c_cidGoto; 
			pfmOC->objectType = OT_invalid;
			pfmOC->objectID = NA;
		}
	}

	//Xynth #197 8/2010
	void contextChat()
	{
		GetWindow()->GetConsoleImage()->GetConsoleData()->PickShip(contextPlayerInfo->GetShip());
	}

    //////////////////////////////////////////////////////////////////////////////
    //
    // ModelerSite
    //
    //////////////////////////////////////////////////////////////////////////////

    void Error(const ZString& str)
    {
        SetFullscreen(false);
        MessageBox(str, "Error", MB_OK);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    Camera*          GetCamera()                    { return m_pcamera;                         }
    Orientation      GetCameraOrientation()         { return m_cameraControl.GetHeadOrientation() * m_cameraControl.GetOrientation();    }
    void             ResetCameraFOV()               { m_cameraControl.SetFOV(s_fDefaultFOV);    }
    Camera*          GetCameraPosters()             { return m_pcameraPosters;                  }
    ConsoleImage*    GetConsoleImage()              { return m_pconsoleImage;                   }
    Viewport*        GetViewport()                  { return m_pviewport;                       }
    Viewport*        GetPosterViewport()            { return m_pviewportPosters;                }
    int              GetRadarMode(void) const       { return int(m_pradarImage->GetRadarLOD()); }
    void             SetRadarMode(int radarMode)
    {
        m_pradarImage->SetRadarLOD (static_cast<RadarImage::RadarLOD> (radarMode));
        m_radarCockpit = radarMode;
    }

    AsteroidAbilityBitMask  GetInvestAsteroid(void) const { return m_aabmInvest; }
    void                    SetInvestAsteroid(AsteroidAbilityBitMask aabm)  { m_aabmInvest = aabm; }

    AsteroidAbilityBitMask  GetCommandAsteroid(void) const { return m_aabmCommand; }
    void                    SetCommandAsteroid(AsteroidAbilityBitMask aabm)  { m_aabmCommand = aabm; }

    bool             GetRoundRadarMode(void) const  { return m_bRoundRadar;                     }
    CameraMode       GetCameraMode(void) const      { return m_cm;                              }

    bool             GetShowJoystickIndicator(void) const { return m_bShowJoystickIndicator; }

    /*
    void             TurretChange(void)
    {
        Mount   turret = trekClient.GetShip()->GetTurretID();

        if (turret == NA)
        {
            m_pwrapImageTurret->SetImage(Image::GetEmpty());
            m_pgeoTurret = Geo::GetEmpty();
        }
        else
        {
            IshipIGC*   pshipParent = trekClient.GetShip()->GetParentShip();
            assert (pshipParent);

            float   minDot = pshipParent->GetHullType()->GetHardpointData(turret).minDot;
            if (minDot < -0.75f)
            {
                m_pwrapImageTurret->SetImage(Image::GetEmpty());
                m_pgeoTurret = Geo::GetEmpty();
            }
            else
            {
                m_pgeoTurret = CreateWireSphereGeo(pshipParent->GetHullType()->GetHardpointData(turret).minDot, 32);
            }
        }
        m_pimageTurret =
            new GeoImage(new TransformGeo(m_pgeoTurret, m_pmtTurret),
                         m_pviewportTurret, false);
    }
    */

    bool IsValid()
    {
        return EngineWindow::IsValid();
    }

    void ShowWebPage(const char* szURL)
    {
		//
		// WLP - 2005 removed line below - this overrides all web pages to alleg.net for now
		//  the compare doesn't work anyway
		//   if (szURL[0] == '\0')
		if (szURL[0] == '\0')
			szURL = "https://www.freeallegiance.org/forums"; // BT - 9/17 updated dead link to point to forums instead.

        ShellExecute(NULL, NULL, szURL, NULL, NULL, SW_SHOWNORMAL);
    }

    #define idmConfigure         3
    #define idmView              5
    #define idmOptions           6
    #define idmHelp              7
    #define idmExitGame          8
    #define idmEngineOptions     9
    #define idmSoundOptions      10
    #define idmGameOptions       11
    #define idmExitApp           12
    #define idmGameDetails       13
	#define idmVersion           14 // TE: Added Version menu
	#define idmTags				 15 // BT - STEAM - Let the user select thier group tag.

    #define idmChannelN          101
    #define idmChannelShow       102
    #define idmChannelHide       103
    #define idmChannelRename     104

    #define idmCommSend          201
    #define idmCommMuzzle        202
    #define idmCommUnMuzzle      203

    #define idmEquipmentN        301
    #define idmEquipmentMount    302
    #define idmEquipmentDismount 303
    #define idmEquipmentDrop     304

    #define idmWeaponN           401
    #define idmWeaponMount       402
    #define idmWeaponDismount    403
    #define idmWeaponDrop        404

    #define idmCMGameState       501
    #define idmCMCombat          502
    #define idmCMCommand         503
    #define idmCMSector          504
    #define idmCMLoadout         505
    #define idmCMTeamPane        506

    #define idmSound                       601
    #define idmTogglePosters               602
    #define idmToggleDebris                603
    #define idmToggleStars                 604
    #define idmToggleStrobes               605
    #define idmToggleLensFlare             606
    #define idmToggleTrails                609
    #define idmToggleEnvironment           611
    #define idmToggleBidirectionalLighting 612
    #define idmToggleChatHistoryHUD        614
    #define idmToggleCenterHUD             615
    #define idmToggleTargetHUD             616
    #define idmStyleHUD                    618
    #define idmToggleCensorChats           619
    #define idmToggleLinearControls        620
    #define idmToggleSmoke                 621
    #define idmToggleBounds                622
    #define idmToggleTransparentObjects    623
    #define idmFilterChatsToAll            624
    #define idmFilterQuickComms            625
    #define idmFilterLobbyChats            626
    #define idmToggleLargeDeadZone         627
    #define idmToggleVirtualJoystick       628
    #define idmToggleFlipY                 629
    #define idmToggleStickyChase           630
    #define idmToggleEnableFeedback        631
    #define idmMaxTextureSize              632 // yp Your_Persona August 2 2006 : MaxTextureSize Patch
    #define idmPings                       633 // w0dk4 player-pings feature
    /* pkk May 6th: Disabled bandwidth patch
    #define idmBandwidth		       634 // w0dk4 June 2007: Bandwith Patch */
    #define	idmMuteFilterOptions		 635 //TheBored 30-JUL-07: Filter Unknown Chat patch
    #define idmFilterUnknownChats		 636 //TheBored 30-JUL-07: Filter Unknown Chat patch
	#define idmScrollbar				 637 // #294 - Turkey
	#define idmIncreaseChatLines		 638 // #294 - Turkey
	#define idmReduceChatLines			 639 // #294 - Turkey
	#define idmCycleTimestamp			 640 // #294 - Turkey

    #define idmResetSound           701
    #define idmSoundQuality         702
    #define idmSoundHardware        703
    #define idmToggleMusic          704
    #define idmMusicVolumeUp        705
    #define idmMusicVolumeDown      706
    #define idmSFXVolumeUp          707
    #define idmSFXVolumeDown        708
    #define idmVoiceOverVolumeUp    709
    #define idmVoiceOverVolumeDown  710
	#define idmUseDSound8           711

	#define idmContextAcceptPlayer	801
	#define idmContextRejectPlayer	802
	#define idmContextMakeLeader	803
	#define idmContextMutePlayer	804

	//imago 6/30/09
	#define idmDeviceOptions		805
	#define idmAA					806
	#define idmMip					807
	#define idmPack					808
	#define idmVsync				810
	
    //Imago 7/10
    #define idmFFOptions            811
    #define idmFFGainUp             812
    #define idmFFGainDown           813
    #define idmFFAutoCenter         814

	
	//Xynth #48 8/2010
	#define idmContextDockDrone		815
	//Xynth #197 8/2010
	#define idmContextChat			816

	//Imago 8/10 #215
	#define idmMouseOptions		817
	#define idmMouseSensUp		818
	#define idmMouseSensDown	819
	#define idmMouseAccel		820
	#define idmWheelDelay		821 //Spunky #282

    #define idmOldUi	        823

    #define idmShowJoystickIndicator 824

	// BT - STEAM
	#define idmCallsignTag0		900
	#define idmCallsignTag1		901
	#define idmCallsignTag2		902
	#define idmCallsignTag3		903
	#define idmCallsignTag4		904
	#define idmCallsignTag5		905
	#define idmCallsignTag6		906
	#define idmCallsignTag7		907
	#define idmCallsignTag8		908
	#define idmCallsignTag9		909
	#define idmCallsignTagNone	910 
	
	// BT - STEAM
	#define idmToken0			920
	#define idmToken1			921
	#define idmToken2			922
	#define idmToken3			923
	#define idmToken4			924

	// BT - STEAM - New player context menu options
	#define idmContextKickPlayer	1000
	#define idmContextBanPlayer		1001
	
	/* SR: TakeScreenShot() grabs an image of the screen and saves it as a 24-bit
	 * bitmap. Filename is determined by the user's local time.
	 * Author: Stain_Rat
	 */
	void TakeScreenShot()
	{
		// BT - STEAM - When the user attempts to take a screen shot inside of Alleg, trigger the steam overlay to do it instead.
		SteamScreenshots()->TriggerScreenshot();
		return; 

		////capturing screen size this way (instead of using a native GDI call) will create
		////windowed screen shots which look like they were taken with the game running
		////in full screen mode.
		//Point currentResolution = GetRenderRectValue()->GetValue().Size();
		//int screenX = currentResolution.X();
		//int screenY = currentResolution.Y();

		//HDC hDesktopDC = GetDC();
		//HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
		//void* DIBBitValues;
		//BITMAPINFO bmpInfo = {0};

		////build up the BITMAPINFOHEADER
		//bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		//bmpInfo.bmiHeader.biWidth = screenX;
		//bmpInfo.bmiHeader.biHeight = screenY;
		//bmpInfo.bmiHeader.biPlanes = 1;
		//bmpInfo.bmiHeader.biBitCount = 24;
		//bmpInfo.bmiHeader.biCompression = BI_RGB;

		//HBITMAP hCaptureBitmap = CreateDIBSection(hDesktopDC, &bmpInfo, DIB_RGB_COLORS, &DIBBitValues, NULL, NULL);
		//SelectObject(hCaptureDC, hCaptureBitmap);
		//BitBlt(hCaptureDC, 0, 0, screenX, screenY, hDesktopDC, 0, 0, SRCCOPY);

		////populates bmpInfo.bmiHeader.biSizeImage with the actual size
		//GetDIBits(hDesktopDC, hCaptureBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

		////build up the BITMAPFILEHEADER
		//BITMAPFILEHEADER bmpFileHeader = {0};
		//bmpFileHeader.bfType = 'MB';
		//bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpInfo.bmiHeader.biSizeImage;
		//bmpFileHeader.bfReserved1 = 0;
		//bmpFileHeader.bfReserved2 = 0;
		//bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		////create a filename for our screenshot using the local time
		//SYSTEMTIME sysTimeForName;
		//GetLocalTime(&sysTimeForName);
		//ZString scrnShotName = sysTimeForName.wYear;
		//scrnShotName += "-";
		//scrnShotName += sysTimeForName.wMonth;
		//scrnShotName += "-";
		//scrnShotName += sysTimeForName.wDay;
		//scrnShotName += "_";
		//scrnShotName += sysTimeForName.wHour;
		//scrnShotName += ".";
		//scrnShotName += sysTimeForName.wMinute;
		//scrnShotName += ".";
		//scrnShotName += sysTimeForName.wSecond;
		//scrnShotName += ".";
		//scrnShotName += sysTimeForName.wMilliseconds;
		//scrnShotName += ".bmp";

		//FILE* outputFile;
		//outputFile = fopen(scrnShotName,"wb");
		////write the BITMAPFILEHEADER, BITMAPINFOHEADER, and bimap bit values to create the *.bmp
		//fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, outputFile);
		//fwrite(&bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, outputFile);
		//fwrite(DIBBitValues, bmpInfo.bmiHeader.biSizeImage, 1, outputFile);
		//fclose(outputFile);

		//ReleaseDC(hDesktopDC);
		//DeleteDC(hCaptureDC);
		//DeleteObject(hCaptureBitmap);
	}

	// BT - STEAM - TODO Move these to where the other globals are hiding?
	CallsignTagInfo m_currentCallsignTag;
#ifdef STEAM_APP_ID
	SteamClans m_availableSteamClans;
#endif

    void ShowMainMenu()
    {
        m_pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                m_pmenuCommandSink
            );

		// mmf format octal version (defined in SlmVer.h) to decimal YY.MM.DD
		int YY,MM,DD;
		ZString dR;
		{
			ZString dVer, dYY, dMM, dDD;
			dVer = ZVersionInfo().GetStringValue("FileVersion").Right(6);
			dR = ZVersionInfo().GetProductVersionString().Middle(3,1); //Imago good till R10
			dDD = dVer.Right(2);
			dMM = dVer.Middle(2,2);
			dYY = dVer.Middle(0,2);

			YY = atoi(dYY); 	
			YY = (YY/10)*8+(YY%10);
			
			MM = atoi(dMM); 	
			MM = (MM/10)*8+(MM%10);
			
			DD = atoi(dDD); 	
			DD = (DD/10)*8+(DD%10);
		}

		// TE: Add version menu, mmf changed format, zero pad YY, that will last us 3 more years and saves an if
		// TheBored 05-APR-2010: Removed leading 0 from year, hooray 2010!
		// mmf added ifs to zero pad MM and DD
		if (MM<10 && DD<10) m_pmenu->AddMenuItem(0, "FAZ R"+dR+" Build # " + ZString(YY) + ".0" + ZString(MM) + ".0" + ZString(DD));
		if (MM<10 && DD>9)  m_pmenu->AddMenuItem(0, "FAZ R"+dR+" Build # " + ZString(YY) + ".0" + ZString(MM) + "." + ZString(DD));
		if (MM>9 && DD<10)  m_pmenu->AddMenuItem(0, "FAZ R"+dR+" Build # " + ZString(YY) + "." + ZString(MM) + ".0" + ZString(DD));
		if (MM>9 && DD>9)   m_pmenu->AddMenuItem(0, "FAZ R"+dR+" Build # " + ZString(YY) + "." + ZString(MM) + "." + ZString(DD));
		//#62 Imago 7/10
		if (trekClient.m_pMissionInfo) {
			ZString zVer = UTL::GetServerVersion(trekClient.m_pMissionInfo->GetCookie());
			if (zVer.IsEmpty() == false) {
				m_pmenu->AddMenuItem(0, PCC("Server v" + zVer));
			}
		}
		//AEM, redesigned ESC menu 7/6/07
		// mmf 10/07 swapped position of S and G
		m_pmenu->AddMenuItem(0               , "");
		m_pmenu->AddMenuItem(0				 , "HELP");
		m_pmenu->AddMenuItem(0               , "--------------------------");
		m_pmenu->AddMenuItem(idmHelp         , "Manual & Quick Reference"            , 'H'                     );
		m_pmenu->AddMenuItem(0               , "");
		m_pmenu->AddMenuItem(0               , "OPTIONS");
		m_pmenu->AddMenuItem(0               , "--------------------------");
        m_pmenu->AddMenuItem(idmEngineOptions, "Graphics Device" , 'D', m_psubmenuEventSink);
        m_pmenu->AddMenuItem(idmOptions      , "Graphics", 'O', m_psubmenuEventSink);
        m_pmenu->AddMenuItem(idmGameOptions  , "Game",     'G', m_psubmenuEventSink);
		m_pmenu->AddMenuItem(idmSoundOptions , "Sound"   , 'S', m_psubmenuEventSink);

		
		// BT - STEAM - Let the user select their steam call sign from a list of options.
#ifdef STEAM_APP_ID
		if (m_availableSteamClans.GetAvailableCallsignTags()->GetCount() > 0)
		{
			ZString menuOption = "Squad Tags";
			if (m_currentCallsignTag.m_steamGroupID > 0)
			{
				ZString currentName = trekClient.GetSavedCharacterName();
				ZString renderedName = m_currentCallsignTag.Render(currentName);
				menuOption = ZString("Squad Tags (") + renderedName + ")   ";
			}

			if (m_screen == ScreenIDSplashScreen || m_screen == ScreenIDIntroScreen || m_screen == ScreenIDZoneClubScreen)
			{

				m_pmenu->AddMenuItem(0, "");
				m_pmenu->AddMenuItem(0, "Only Available Before");
				m_pmenu->AddMenuItem(0, "Connecting to the Lobby");
				m_pmenu->AddMenuItem(0, "--------------------------");
				m_pmenu->AddMenuItem(idmTags, menuOption, 'T', m_psubmenuEventSink);

				
				ZString tokens = m_currentCallsignTag.GetAvailableTokens();
				if (tokens.GetLength() > 0)
				{
					for (int i = 0; i < tokens.GetLength(); i++)
					{
						if (tokens[i] == m_currentCallsignTag.m_callsignToken[0])
							continue;

						m_pmenu->AddMenuItem(idmToken0 + i, "Add Officer Token: ", tokens[i]);
					}
				}
			}
		}

		// BT - STEAM - END.
#endif

        if (trekClient.MyMission() != NULL) {
			m_pmenu->AddMenuItem(0               , "");
			m_pmenu->AddMenuItem(0               , "INFORMATION");
			m_pmenu->AddMenuItem(0               , "--------------------------");
            m_pmenu->AddMenuItem(idmGameDetails, "Game Details",   'I');
			m_pmenu->AddMenuItem(idmPings  ,     "Player Pings",   'P');	// w0dk4 player-pings feature
		}

        m_pmenu->AddMenuItem(0               , "");
		m_pmenu->AddMenuItem(0               , "QUIT");
		m_pmenu->AddMenuItem(0               , "--------------------------");
        if ((trekClient.MyMission() != NULL) || Slideshow::IsInSlideShow ())
            m_pmenu->AddMenuItem(idmExitGame , "Mission"    , 'Q'                 );
        m_pmenu->AddMenuItem(idmExitApp      , "Allegiance" , 'X'                     );

        OpenPopup(m_pmenu, Point(10, 10));
    }

	// BT - Steam
	bool IsPlayerSteamModerator()
	{
		// The MSAlleg Steam Group ID.
		CSteamID moderatorGroupID = ((uint64)103582791460031578);
		int clanCount = SteamFriends()->GetClanCount();
		bool isModerator = false;
		for (int i = 0; i < clanCount; i++)
		{
			if (SteamFriends()->GetClanByIndex(i) == moderatorGroupID)
			{
				isModerator = true;
				break;
			}
		}

		return isModerator;
	}

	// YP: Add this for the rightclick lobby patch
	void ShowPlayerContextMenu(PlayerInfo * playerInfo)
	{
		contextPlayerInfo = playerInfo;

		char str1[30];
		char str2[30];
		char str3[30];	sprintf(str3, "Make Leader  ",playerInfo->CharacterName());
		char str4[30];	sprintf(str4, playerInfo->GetMute() == false ?"Mute         " :"UnMute       ",playerInfo->CharacterName());

        bool bEnableAccept = false;
        bool bEnableReject = false;
        bool bEnableMakeLeader = false;
		bool bEnableMute = false;
			if (playerInfo->SideID() == trekClient.GetSideID())
			{
				if(trekClient.GetPlayerInfo()->IsTeamLeader())
				{
					sprintf(str2,"Boot       ",playerInfo->CharacterName());
					bEnableReject = playerInfo->ShipID() != trekClient.GetShipID()
						&& (!trekClient.MyMission()->GetMissionParams().bLockTeamSettings
							|| playerInfo->SideID() != trekClient.GetSideID());

					bEnableMakeLeader = playerInfo->SideID() == trekClient.GetSideID()
						&& playerInfo->ShipID() != trekClient.GetShipID();


				}
			}
			else
			{
				if(trekClient.GetPlayerInfo()->IsTeamLeader())
				{
					sprintf(str1,"Accept       ",playerInfo->CharacterName());
					bEnableAccept = trekClient.MyMission()->SideAvailablePositions(trekClient.GetSideID()) > 0
						&& trekClient.MyMission()->FindRequest(trekClient.GetSideID(), playerInfo->ShipID());

					sprintf(str2,"Reject       ",playerInfo->CharacterName());
					bEnableReject = playerInfo->ShipID() != trekClient.GetShipID()
							&& trekClient.MyMission()->FindRequest(trekClient.GetSideID(), playerInfo->ShipID())
							&& (!trekClient.MyMission()->GetMissionParams().bLockTeamSettings
								|| playerInfo->SideID() != trekClient.GetSideID());
				}
			}

			// we always want to be able to mute :)
			bEnableMute = playerInfo != trekClient.GetPlayerInfo() ? true : false ;

		m_pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                m_pmenuCommandSink
            );


        if(bEnableAccept)		m_pmenu->AddMenuItem(idmContextAcceptPlayer , str1 , 'A');
        if(bEnableReject)		m_pmenu->AddMenuItem(idmContextRejectPlayer , str2 , 'R');
        if(bEnableMakeLeader)	m_pmenu->AddMenuItem(idmContextMakeLeader , str3 , 'L');
        if(bEnableMute)			m_pmenu->AddMenuItem(idmContextMutePlayer  , str4 , playerInfo->GetMute() == false ?'M' :'U');		

		// BT - STEAM - Enable moderators to ban players by context menu.
		if (IsPlayerSteamModerator() && playerInfo->IsHuman())
		{
			m_pmenu->AddMenuItem(idmContextKickPlayer, "Kick To NOAT", 'K');
			m_pmenu->AddMenuItem(idmContextBanPlayer, "Ban From Game", 'B');
		}
		
		Point popupPosition = GetMousePosition();


		TRef<Pane> ppane = m_pmenu->GetPane();
		ppane->UpdateLayout();
		Point p = Point::Cast(ppane->GetSize());

		popupPosition.SetY(popupPosition.Y() - p.Y());
        OpenPopup(m_pmenu,	popupPosition);
	}

	//Xynth #48 8/2010 Add for player pane right click (initially miner dock)
	void ShowPlayerPaneContextMenu(PlayerInfo * playerInfo)
	{
		contextPlayerInfo = playerInfo;

		char str1[30];
		char str2[30];
		char str3[30];
		char str4[30];
		
		//Xynth #48 8/2010 Add Dock menu item
		bool bEnableDock = false;
		bool bEnableChat = false;
		bool bEnableReject = false;
		bool bEnableAccept = false;

		if (playerInfo->SideID() == trekClient.GetSideID())
		{
			// pkk #211 08/05/2010 Allow all drones stay docked Xynth Only commander can staydock and launch
			if (playerInfo->GetShip()->GetPilotType() < c_ptPlayer && trekClient.GetPlayerInfo()->IsTeamLeader())
			{
				if (playerInfo->GetShip()->GetPilotType() != c_ptCarrier)
				{ 					 
					if ((playerInfo->LastSeenState() == c_ssDocked || playerInfo->LastSeenState() == NULL) && playerInfo->GetShip()->GetStayDocked()) //Spunky #250					
						sprintf(str1,"Launch  ");
					else
						sprintf(str1,"Dock    ");
					bEnableDock  = true;

				}				
				else  //carrier
				{					
					if (!(playerInfo->LastSeenState() == c_ssDocked || playerInfo->LastSeenState() == NULL))
					{
						sprintf(str1,"Dock/Flee ");  //if no shipyard, carriers go to nearest base players can launch from
						bEnableDock  = true;  //no launch, since carriers don't have a default command
					}
				}
				}
			}

		bEnableChat = (playerInfo->ShipID() != trekClient.GetShipID()) &&
					   (playerInfo->GetShip()->GetPilotType() == c_ptPlayer);
		sprintf(str2,"Chat       ");

		if (playerInfo->SideID() != trekClient.GetSideID())		
		{
			if(trekClient.GetPlayerInfo()->IsTeamLeader())
			{
				sprintf(str3,"Accept       ");
				bEnableAccept = trekClient.MyMission()->SideAvailablePositions(trekClient.GetSideID()) > 0
					&& trekClient.MyMission()->FindRequest(trekClient.GetSideID(), playerInfo->ShipID());

				sprintf(str4,"Reject       ");
				bEnableReject = playerInfo->ShipID() != trekClient.GetShipID()
					&& trekClient.MyMission()->FindRequest(trekClient.GetSideID(), playerInfo->ShipID())
					&& (!trekClient.MyMission()->GetMissionParams().bLockTeamSettings
					|| playerInfo->SideID() != trekClient.GetSideID());
			}
		}

		bool bSteamModerator = IsPlayerSteamModerator();

		if (bEnableDock || bEnableChat ||bEnableReject || bEnableAccept || bSteamModerator)  //Xynth #205 8/2010 Will need || for any other menu options.  
						  //The point is don't create the menu if nothing is on it
		{

			m_pmenu =
				CreateMenu(
				GetModeler(),
				TrekResources::SmallFont(),
				m_pmenuCommandSink
				);


			if(bEnableDock)			m_pmenu->AddMenuItem(idmContextDockDrone , str1 , 'D'); //Xynth #48 8/2010
			if(bEnableChat)			m_pmenu->AddMenuItem(idmContextChat , str2 , 'C'); //Xynth #197 8/2010
			if(bEnableAccept)		m_pmenu->AddMenuItem(idmContextAcceptPlayer , str3 , 'A');
			if(bEnableReject)		m_pmenu->AddMenuItem(idmContextRejectPlayer , str4 , 'R');

			// BT - STEAM - Enable moderators to ban players by context menu.
			if (bSteamModerator == true && bEnableDock == false && playerInfo->IsHuman())
			{
				m_pmenu->AddMenuItem(idmContextKickPlayer, "Kick To NOAT", 'K');
				m_pmenu->AddMenuItem(idmContextBanPlayer, "Ban From Game", 'B');
			}

			Point popupPosition = GetMousePosition();


			TRef<Pane> ppane = m_pmenu->GetPane();
			ppane->UpdateLayout();
			Point p = Point::Cast(ppane->GetSize());

			popupPosition.SetY(popupPosition.Y() - p.Y());
			OpenPopup(m_pmenu,	popupPosition);
		}
	}

    void ShowOptionsMenu()
    {
        m_pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                m_pmenuCommandSink
            );

        m_pmenu->AddMenuItem(idmEngineOptions, "Graphics Device" , 'D', m_psubmenuEventSink);
        m_pmenu->AddMenuItem(idmOptions      , "Graphics Options", 'O', m_psubmenuEventSink);
        m_pmenu->AddMenuItem(idmSoundOptions , "Sound Options"   , 'S', m_psubmenuEventSink);
        m_pmenu->AddMenuItem(idmGameOptions  , "Game Options" ,    'G', m_psubmenuEventSink);

        OpenPopup(m_pmenu, Point(10, 10));
    }


	// BT - STEAM 
#ifdef STEAM_APP_ID
	void AddAvailablePlayerTagsToMenu(TRef<IMenu> pmenu)
	{
		pmenu->AddMenuItem(0, "Squad Tags");
		pmenu->AddMenuItem(0, "--------------------------");

		pmenu->AddMenuItem(idmCallsignTagNone, "<None>", 'X');

		// Allow up to 30 tags to be shown.
		// Backing this out for now, forgot to add additional idmCallsignTag* slots. 
		for (int i = 0; i < m_availableSteamClans.GetAvailableCallsignTags()->GetCount() && i < 10; i++)
		{
			CallsignTagInfo item = m_availableSteamClans.GetAvailableCallsignTags()->Get(i);
			pmenu->AddMenuItem(idmCallsignTag0 + i, item.m_callsignTag, 48 + item.m_index); // 48 = ASCII code for '0'.
		}
	}

	// BT - STEAM
	void SetPlayerCallsign(int playerCallsignIndex)
	{
		for (int i = 0; i < m_availableSteamClans.GetAvailableCallsignTags()->GetCount(); i++)
		{
			CallsignTagInfo callsignTagInfo = m_availableSteamClans.GetAvailableCallsignTags()->Get(i);

			if (callsignTagInfo.m_index == playerCallsignIndex - idmCallsignTag0)
			{
				m_currentCallsignTag.SetSteamGroupID(callsignTagInfo.m_steamGroupID, callsignTagInfo.m_callsignTag);
				break;
			}
		}
	}

	// BT - STEAM
	void UnsetPlayerCallsign()
	{
		m_currentCallsignTag.SetSteamGroupID(0, "");
	}

	// BT - STEAM
	void SetPlayerToken(int playerTokenIndex)
	{
		ZString tokens = m_currentCallsignTag.GetAvailableTokens();
		m_currentCallsignTag.SetToken(tokens.Middle(idmToken0 - playerTokenIndex, 1));
	}
#endif

    TRef<IPopup> GetSubMenu(IMenuItem* pitem)
    {
        TRef<IMenu> pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                m_pmenuCommandSink
            );

        switch (pitem->GetID()) {
            case idmEngineOptions:
                return GetEngineMenu(TrekResources::SmallFont());

            case idmOptions:
                		       		 				 pmenu->AddMenuItem(idmDeviceOptions,					"Advanced Options",				  'A', m_psubmenuEventSink);
                m_pitemToggleEnvironment           = pmenu->AddMenuItem(idmToggleEnvironment,           GetEnvironmentMenuString()          , 'E');
                m_pitemTogglePosters               = pmenu->AddMenuItem(idmTogglePosters,               GetPostersMenuString()              , 'P');
                m_pitemToggleDebris                = pmenu->AddMenuItem(idmToggleDebris,                GetDebrisMenuString()               , 'D');
                m_pitemToggleStars                 = pmenu->AddMenuItem(idmToggleStars,                 GetStarsMenuString()                , 'S');
                m_pitemToggleStrobes               = pmenu->AddMenuItem(idmToggleStrobes,               GetStrobesMenuString()              , 'R');
                m_pitemToggleTrails                = pmenu->AddMenuItem(idmToggleTrails,                GetTrailsMenuString()               , 'T');
                #ifdef _DEBUG
                    m_pitemToggleBounds                = pmenu->AddMenuItem(idmToggleBounds,                GetBoundsMenuString()               , 'N');
                    m_pitemToggleTransparentObjects    = pmenu->AddMenuItem(idmToggleTransparentObjects,    GetTransparentObjectsMenuString()   , 'O');
                #endif
                m_pitemToggleSmoke                 = pmenu->AddMenuItem(idmToggleSmoke,                 GetSmokeMenuString()                ,	'L'); //was same as posters - Imago 8/8/09
                m_pitemToggleLensFlare             = pmenu->AddMenuItem(idmToggleLensFlare,             GetLensFlareMenuString()            ,	'F');
                m_pitemToggleBidirectionalLighting = pmenu->AddMenuItem(idmToggleBidirectionalLighting, GetBidirectionalLightingMenuString(),	'B');
                m_pitemStyleHUD                    = pmenu->AddMenuItem(idmStyleHUD,                    GetStyleHUDMenuString()             ,	'H'); //Imago 6/30/09 adjust new dx9 settings in game

                m_pitemToggleUseOldUi     = pmenu->AddMenuItem(idmOldUi, GetOldUiMenuString(), 'G');

                m_pitemToggleShowJoystickIndicator = pmenu->AddMenuItem(idmShowJoystickIndicator, GetShowJoystickIndicatorMenuString(), 'J');
 				
				break;

            case idmGameOptions:
                m_pitemMuteFilter		           = pmenu->AddMenuItem(idmMuteFilterOptions,					"Mute/Filter",						'M', m_psubmenuEventSink); //TheBored 30-JUL-07: Filter Unknown Chat patch
                m_pitemToggleStickyChase           = pmenu->AddMenuItem(idmToggleStickyChase,           GetStickyChaseMenuString (),        'K');
                m_pitemToggleLinearControls        = pmenu->AddMenuItem(idmToggleLinearControls,        GetLinearControlsMenuString(),      'L');
                m_pitemToggleLargeDeadZone         = pmenu->AddMenuItem(idmToggleLargeDeadZone,         GetDeadzoneMenuString(),       'Z'); //imago updated 7/8/09
                m_pitemToggleVirtualJoystick       = pmenu->AddMenuItem(idmToggleVirtualJoystick,       GetVirtualJoystickMenuString(),     'J');
                m_pitemToggleFlipY                 = pmenu->AddMenuItem(idmToggleFlipY,                 GetFlipYMenuString(),               'Y');
                //m_pitemToggleEnableFeedback        = pmenu->AddMenuItem(idmToggleEnableFeedback,        GetEnableFeedbackMenuString(),      'E'); //imago sunk 7/10
													 pmenu->AddMenuItem(idmFFOptions,					"Force Feedback",				  'E', m_psubmenuEventSink);
													 pmenu->AddMenuItem(idmMouseOptions,				"Mouse Options",				  'Q', m_psubmenuEventSink);
					
				/* pkk May 6th: Disabled bandwidth patch // w0dk4 June 2007: Bandwith Patch
				m_pitemToggleBandwidth			   = pmenu->AddMenuItem(idmBandwidth,					GetBandwidthMenuString(),		    'B');*/
                                                     pmenu->AddMenuItem(idmConfigure           ,        "Map Keys and Controls"      ,      'C');
                break;

            case idmSoundOptions:
                #ifdef _DEBUG
                pmenu->AddMenuItem(idmResetSound, "Reset Sound", 'R');
                #endif
                m_pitemSoundQuality         = pmenu->AddMenuItem(idmSoundQuality, GetSoundQualityMenuString());
                m_pitemToggleSoundHardware  = pmenu->AddMenuItem(idmSoundHardware, GetSoundHardwareMenuString());
				m_pitemToggleDSound8Usage   = pmenu->AddMenuItem(idmUseDSound8, GetDSound8EnabledString());
                m_pitemSFXVolumeUp          = pmenu->AddMenuItem(idmSFXVolumeUp,
                    GetGainMenuString("Sound Effect", m_pnumSFXGain->GetValue(), c_fVolumeDelta), 'S');
                m_pitemSFXVolumeDown        = pmenu->AddMenuItem(idmSFXVolumeDown,
                    GetGainMenuString("Sound Effect", m_pnumSFXGain->GetValue(), -c_fVolumeDelta), 'A');
                m_pitemVoiceOverVolumeUp    = pmenu->AddMenuItem(idmVoiceOverVolumeUp,
                    GetGainMenuString("Voice Over", m_pnumVoiceOverGain->GetValue(), c_fVolumeDelta), 'V');
                m_pitemVoiceOverVolumeDown  = pmenu->AddMenuItem(idmVoiceOverVolumeDown,
                    GetGainMenuString("Voice Over", m_pnumVoiceOverGain->GetValue(), -c_fVolumeDelta), 'C');
                break;

				// BT - STEAM
#ifdef STEAM_APP_ID
			case idmTags:
				AddAvailablePlayerTagsToMenu(pmenu);
				break;
#endif
			//TheBored 30-JUL-07: Filter Unknown Chat patch
			case idmMuteFilterOptions:
                m_pitemToggleCensorChats           = pmenu->AddMenuItem(idmToggleCensorChats,           GetCensorChatsMenuString(),         'D');
				m_pitemFilterChatsToAll            = pmenu->AddMenuItem(idmFilterChatsToAll,            GetFilterChatsToAllMenuString(),    'A');
                m_pitemFilterQuickComms            = pmenu->AddMenuItem(idmFilterQuickComms,            GetFilterQuickCommsMenuString(),    'V');
				m_pitemFilterUnknownChats          = pmenu->AddMenuItem(idmFilterUnknownChats,          GetFilterUnknownChatsString(),      'U');
                m_pitemFilterLobbyChats            = pmenu->AddMenuItem(idmFilterLobbyChats,            GetFilterLobbyChatsMenuString(),    'L');
				
				// #294 - Turkey
				m_pitemIncreaseChatLines		   = pmenu->AddMenuItem(idmIncreaseChatLines,			GetIncreaseChatLinesMenuString(),	'I');
				m_pitemReduceChatLines			   = pmenu->AddMenuItem(idmReduceChatLines,				GetReduceChatLinesMenuString(),		'R');
				break;
			//End TB 30-JUL-07
			//imago 6/30/09: new graphics options dx9, removed vsync 7/10
			case idmDeviceOptions:
				m_pitemAA				= pmenu->AddMenuItem(idmAA   			  , GetAAString()                                       , 'A');
			    //m_pitemMip				= pmenu->AddMenuItem(idmMip    			  , GetMipString()                                      , 'M'); // BT - Disable MipMaps for now
				m_pitemVsync			= pmenu->AddMenuItem(idmVsync  			  , GetVsyncString()                                    , 'V'); //Spunky #265 backing out //Imago 7/10
				// yp Your_Persona August 2 2006 : MaxTextureSize Patch
				m_pitemMaxTextureSize	= pmenu->AddMenuItem(idmMaxTextureSize,     GetMaxTextureSizeMenuString(),    					  'X');
				break;

			//Imago 7/10 #187
			case idmFFOptions:
				m_pitemToggleEnableFeedback         = pmenu->AddMenuItem(idmToggleEnableFeedback  , GetEnableFeedbackMenuString(),      'E'); //imago sunk 7/10
				m_pitemToggleFFGainUp				= pmenu->AddMenuItem(idmFFGainUp			  , GetFFGainMenuString(m_pnumFFGain->GetValue(), c_fFFGainDelta)   , 'U');
				m_pitemToggleFFGainDown				= pmenu->AddMenuItem(idmFFGainDown			  , GetFFGainMenuString(m_pnumFFGain->GetValue(), -c_fFFGainDelta)  , 'D');
			    m_pitemToggleFFAutoCenter			= pmenu->AddMenuItem(idmFFAutoCenter		  , GetFFAutoCenterMenuString()                                     , 'C');
				break;

			//Imago 8/10 #215
			case idmMouseOptions:
                                 pmenu->AddMenuItem(0                     , "------------------------------------------------"     );
                                 pmenu->AddMenuItem(0                     , "Options are only valid when in fullscreen"     );
                                 pmenu->AddMenuItem(0                     , "------------------------------------------------"     );
				m_pitemToggleMouseAccel		        = pmenu->AddMenuItem(idmMouseAccel			  , GetMouseAccelMenuString()										, 'A');
				m_pitemToggleWheelDelay				= pmenu->AddMenuItem(idmWheelDelay			  ,	GetWheelDelayMenuString(), 'W'); //Spunky #282
				m_pitemToggleMouseSensUp			= pmenu->AddMenuItem(idmMouseSensUp			  , GetMouseSensMenuString(m_pnumMouseSens->GetValue(), c_fMouseSensDelta)		, 'U');
				m_pitemToggleMouseSensDown			= pmenu->AddMenuItem(idmMouseSensDown		  , GetMouseSensMenuString(m_pnumMouseSens->GetValue(), -c_fMouseSensDelta)		, 'D');
				break;



        }

        return pmenu;
    }

    void ToggleDebris()
    {
		//LANS - allow off/low/medium/high debris settings
		//lower numbers = more debris
		if (m_debrisDensity->GetValue() == 1.5f) { //low -> medium
			m_debrisDensity->SetValue(1.0f);
			SavePreference("Debris", "1.0");
		}
		else if (m_debrisDensity->GetValue() == 1.0f) { //medium -> high
			m_debrisDensity->SetValue(0.8f);
			SavePreference("Debris", "0.8");
		}
		else if (m_debrisDensity->GetValue() == 0.8f) { //high -> off
			m_debrisDensity->SetValue(0.0f);
			m_pwrapGeoDebris->SetGeo(Geo::GetEmpty());
			SavePreference("Debris", "0");
		}
		else { //off -> low
			m_debrisDensity->SetValue(1.5f);
			m_pwrapGeoDebris->SetGeo(m_pgeoDebris);
			SavePreference("Debris", "1.5");
		}
		if (m_pitemToggleDebris != NULL) {
			m_pitemToggleDebris->SetString(GetDebrisMenuString());
		}
    }

    void ToggleEnvironment()
    {
		if (m_pwrapImageEnvironment->GetImage() == m_pimageEnvironment || IsWine()) { //Imago 8/17/09
            m_pwrapImageEnvironment->SetImage(Image::GetEmpty());
            SavePreference("Environment", FALSE);
        } else {
			m_pwrapImageEnvironment->SetImage(m_pimageEnvironment);
            SavePreference("Environment", TRUE);
        }

        if (m_pitemToggleEnvironment != NULL) {
            m_pitemToggleEnvironment->SetString(GetEnvironmentMenuString());
        }
    }

    void ToggleCensorChats()
    {
        if (CensorDisplay ())
            SavePreference("CensorChats", FALSE);
        else
            SavePreference("CensorChats", TRUE);

        ToggleCensorDisplay ();

        if (m_pitemToggleCensorChats != NULL) {
            m_pitemToggleCensorChats->SetString(GetCensorChatsMenuString());
        }
    }

    void ToggleStickyChase ()
    {
        if (m_bPreferChaseView)
        {
            m_bPreferChaseView = false;
            SavePreference("PreferChaseView", FALSE);
        }
        else
        {
            m_bPreferChaseView = true;
            SavePreference ("PreferChaseView", TRUE);
        }

        if (m_pitemToggleStickyChase != NULL)
            m_pitemToggleStickyChase->SetString (GetStickyChaseMenuString ());
    }

    void ToggleFilterChatsToAll()
    {
        if (trekClient.FilterChatsToAll())
        {
            trekClient.FilterChatsToAll(false);
            SavePreference("FilterChatsToAll", FALSE);
        }
        else
        {
            trekClient.FilterChatsToAll(true);
            SavePreference("FilterChatsToAll", TRUE);
        }

        if (m_pitemFilterChatsToAll != NULL) {
            m_pitemFilterChatsToAll->SetString(GetFilterChatsToAllMenuString());
        }
    }

    void ToggleFilterQuickComms()
    {
        if (trekClient.FilterQuickComms())
        {
            trekClient.FilterQuickComms(false);
            SavePreference("FilterQuickComms", FALSE);
        }
        else
        {
            trekClient.FilterQuickComms(true);
            SavePreference("FilterQuickComms", TRUE);
        }

        if (m_pitemFilterQuickComms != NULL) {
            m_pitemFilterQuickComms->SetString(GetFilterQuickCommsMenuString());
        }
    }
	//TheBored 30-JUL-07: Filter Unknown Chat patch
    void ToggleFilterUnknownChats()
    {
        if (trekClient.FilterUnknownChats())
        {
            trekClient.FilterUnknownChats(false);
            SavePreference("FilterUnknownChats", FALSE);
        }
        else
        {
            trekClient.FilterUnknownChats(true);
            SavePreference("FilterUnknownChats", TRUE);
        }

        if (m_pitemFilterUnknownChats != NULL) {
            m_pitemFilterUnknownChats->SetString(GetFilterUnknownChatsString());
        }
    }
	//End TB 30-JUL-07
	//TheBored 25-JUN-07: Altered function allowing for 3 options. 1 = Filter Lobby, 2 = Filter Lobby and PMs, 3 = Dont Filter Anything
    void ToggleFilterLobbyChats(DWORD dwLobbyChatSetting)
    {
		if (dwLobbyChatSetting > 3){dwLobbyChatSetting = 1;}
		switch (dwLobbyChatSetting)
		{
			case 1:
				trekClient.FilterLobbyChats(dwLobbyChatSetting);
				SavePreference("FilterLobbyChats", dwLobbyChatSetting);
				break;
			case 2:
				trekClient.FilterLobbyChats(dwLobbyChatSetting);
				SavePreference("FilterLobbyChats", dwLobbyChatSetting);
			case 3:
				trekClient.FilterLobbyChats(dwLobbyChatSetting);
				SavePreference("FilterLobbyChats", dwLobbyChatSetting);
		}
        if (m_pitemFilterLobbyChats != NULL) {
            m_pitemFilterLobbyChats->SetString(GetFilterLobbyChatsMenuString());
        }
    }

	// #294 - Turkey 8/13
	void IncreaseChatLines()
	{
		int lines = (int)m_pnumberChatLinesDesired->GetValue() + 1;

		if (SetChatLines(lines))
		{
			m_pitemIncreaseChatLines->SetString(GetIncreaseChatLinesMenuString());
			m_pitemReduceChatLines->SetString(GetReduceChatLinesMenuString());

			if (m_pchatListPane)
			{
				if (GetViewMode() == vmLoadout)
				{
					m_pchatListPane->SetChatLines(std::min(lines, 6));
					m_pnumberChatLines->SetValue(std::min(lines, 6));
				}
				else if (GetViewMode() <= vmOverride)
				{
					m_pchatListPane->SetChatLines(lines);
					m_pnumberChatLines->SetValue(lines);
				}
			}

			SavePreference("ChatLines", lines);
		}
	}

	void ReduceChatLines()
	{
		int lines = (int)m_pnumberChatLinesDesired->GetValue() - 1;

		if (SetChatLines(lines))
		{
			m_pitemIncreaseChatLines->SetString(GetIncreaseChatLinesMenuString());
			m_pitemReduceChatLines->SetString(GetReduceChatLinesMenuString());

			if (m_pchatListPane)
			{
				if (GetViewMode() == vmLoadout)
				{
					m_pchatListPane->SetChatLines(std::min(lines, 6));
					m_pnumberChatLines->SetValue(std::min(lines, 6));
				}
				else if (GetViewMode() <= vmOverride)
				{
					m_pchatListPane->SetChatLines(lines);
					m_pnumberChatLines->SetValue(lines);
				}
			}


			SavePreference("ChatLines", lines);
		}
	}



	// end #294


	//End TB 25-JUN-07
    void ToggleLinearControls()
    {
        if (m_bLinearControls)
        {
            m_bLinearControls = false;
            SavePreference("LinearControlResponse", FALSE);
        }
        else
        {
            m_bLinearControls = true;
            SavePreference("LinearControlResponse", TRUE);
        }

        if (m_pitemToggleLinearControls != NULL) {
            m_pitemToggleLinearControls->SetString(GetLinearControlsMenuString());
        }
    }

    void ToggleStars()
    {
        if (m_pwrapImageStars->GetImage() == Image::GetEmpty()) {
            m_pwrapImageStars->SetImage(m_pimageStars);
            SavePreference("Stars", TRUE);
        } else {
            m_pwrapImageStars->SetImage(Image::GetEmpty());
            SavePreference("Stars", FALSE);
        }

        if (m_pitemToggleStars != NULL) {
            m_pitemToggleStars->SetString(GetStarsMenuString());
        }
    }

    void TogglePosters()
    {
        if (m_pwrapImagePosters->GetImage() == Image::GetEmpty()) {
			m_pwrapImagePosters->SetImage(m_pwrapImagePostersInside);
			SavePreference("Posters", TRUE);
        } else {
            m_pwrapImagePosters->SetImage(Image::GetEmpty());
            SavePreference("Posters", FALSE);
        }

        if (m_pitemTogglePosters != NULL) {
            m_pitemTogglePosters->SetString(GetPostersMenuString());
        }
    }

    void ToggleLensFlare()
    {
        m_bLensFlare = !m_bLensFlare;

        SavePreference("Lens Flare", (DWORD)m_bLensFlare);

        if (!CommandCamera(m_cm)) {
            if (m_bLensFlare) {
                m_pwrapImageLensFlare->SetImage(m_pimageLensFlare);
            } else {
                m_pwrapImageLensFlare->SetImage(Image::GetEmpty());
            }
        }

        if (m_pitemToggleLensFlare != NULL) {
            m_pitemToggleLensFlare->SetString(GetLensFlareMenuString());
        }
    }

    void UpdateBidirectionalLighting()
    {
        if (m_bBidirectionalLighting) {
            m_pimageScene->SetLight(m_color, m_colorAlt);
            m_pimageScene->SetAmbientLevel(m_ambientLevelBidirectional);
        } else {
            m_pimageScene->SetLight(m_color);
            m_pimageScene->SetAmbientLevel(m_ambientLevel);
        }
    }

    void ToggleBidirectionalLighting()
    {
        m_bBidirectionalLighting = !m_bBidirectionalLighting;

        SavePreference("BidirectionalLighting", (DWORD)m_bBidirectionalLighting);

        UpdateBidirectionalLighting();

        if (m_pitemToggleBidirectionalLighting != NULL) {
            m_pitemToggleBidirectionalLighting->SetString(GetBidirectionalLightingMenuString());
        }
    }

	// yp Your_Persona August 2 2006 : MaxTextureSize Patch  //Imago OBSOLOETE REMOVE REVIEW 7/20/09
	void ToggleMaxTextureSize(DWORD dwNewMaxSize)
	{
		if(dwNewMaxSize > 3){dwNewMaxSize =0;}
        trekClient.MaxTextureSize(dwNewMaxSize); //? Imago REVIEW we use g_DX9Settings.m_iMaxTextureSize now

		g_DX9Settings.m_iMaxTextureSize = dwNewMaxSize;

		GetEngine()->SetMaxTextureSize(trekClient.MaxTextureSize());
        SavePreference("MaxTextureSize", trekClient.MaxTextureSize());

        if (m_pitemMaxTextureSize != NULL) {
            m_pitemMaxTextureSize->SetString(GetMaxTextureSizeMenuString());
        }
	}

	/* pkk May 6th: Disabled bandwidth patch
	// w0dk4 June 2007: Bandwith Patch
	void ToggleBandwidth(unsigned int iBandwidth)
	{
		if(iBandwidth > 32){iBandwidth = 2;}
		trekClient.MaxBandwidth(iBandwidth);

		SavePreference("Bandwidth", (DWORD)trekClient.MaxBandwidth());

        if (m_pitemToggleBandwidth != NULL) {
            m_pitemToggleBandwidth->SetString(GetBandwidthMenuString());
        }
	}*/

    void ToggleStrobes()
    {
        ThingGeo::SetShowLights(!ThingGeo::GetShowLights());

        SavePreference("Strobes", (DWORD)ThingGeo::GetShowLights());

        if (m_pitemToggleStrobes != NULL) {
            m_pitemToggleStrobes->SetString(GetStrobesMenuString());
        }
    }

    void ToggleTrails()
    {
        ThingGeo::SetShowTrails(!ThingGeo::GetShowTrails());

        SavePreference("Trails", (DWORD)ThingGeo::GetShowTrails());

        if (m_pitemToggleTrails != NULL) {
            m_pitemToggleTrails->SetString(GetTrailsMenuString());
        }
    }

    void ToggleBounds()
    {
        ThingGeo::SetShowBounds(!ThingGeo::GetShowBounds());

        SavePreference("Bounds", (DWORD)ThingGeo::GetShowBounds());

        if (m_pitemToggleBounds != NULL) {
            m_pitemToggleBounds->SetString(GetBoundsMenuString());
        }
    }

    void ToggleTransparentObjects()
    {
        ThingGeo::SetTransparentObjects(!ThingGeo::GetTransparentObjects());

        SavePreference("TransparentObjects", (DWORD)ThingGeo::GetTransparentObjects());

        if (m_pitemToggleTransparentObjects != NULL) {
            m_pitemToggleTransparentObjects->SetString(GetTransparentObjectsMenuString());
        }
    }

    void SetSmoke (DWORD value)
    {
        if (value == 2) { //imago 8/16/09
			ThingGeo::SetPerformance(true);
            ThingGeo::SetShowSmoke (1);
        } else {
            ThingGeo::SetShowSmoke (int (value));
           }
    }

    void ToggleSmoke()
    {
        int     iSmoke = ThingGeo::GetShowSmoke();
        if (iSmoke == 0) 
            iSmoke = 2;
        else if (iSmoke == 2) 
            iSmoke = 0;
        bool bPerformance = false;
        switch (iSmoke)
        {
            case 2: //Imago 8/16/09
                bPerformance = true;
                iSmoke = 1;
                break;
            case 0:
                iSmoke = 1;
                break;
            case 1:
                iSmoke = 3;
                break;
            case 3:
                iSmoke = 5;
                break;
            default:
                iSmoke = 0;
        }
        
		ThingGeo::SetPerformance(bPerformance);

        ThingGeo::SetShowSmoke(iSmoke);
        SavePreference("SmokeEffects", (DWORD) (bPerformance) ? 2 : iSmoke);

        if (m_pitemToggleSmoke != NULL) {
            m_pitemToggleSmoke->SetString(GetSmokeMenuString());
        }
    }

    void ToggleChatHistoryHUD()
    {
        m_pboolChatHistoryHUD->SetValue(!m_pboolChatHistoryHUD->GetValue());
        //SavePreference("ChatHistory", (DWORD)m_pboolChatHistoryHUD->GetValue());

        if (m_pitemToggleChatHistoryHUD != NULL)
            m_pitemToggleChatHistoryHUD->SetString(GetChatHistoryHUDMenuString());
    }

    void ToggleCenterHUD()
    {
        m_pboolCenterHUD->SetValue(!m_pboolCenterHUD->GetValue());
        SavePreference("CenterHUD", (DWORD)m_pboolCenterHUD->GetValue());

        if (m_pitemToggleCenterHUD != NULL)
            m_pitemToggleCenterHUD->SetString(GetCenterHUDMenuString());
    }

    void ToggleTargetHUD()
    {
        m_pboolTargetHUD->SetValue(!m_pboolTargetHUD->GetValue());
        SavePreference("TargetHUD", (DWORD)m_pboolTargetHUD->GetValue());

        if (m_pitemToggleTargetHUD != NULL)
            m_pitemToggleTargetHUD->SetString(GetTargetHUDMenuString());
    }

    //Something of a misnomer since there are only two styles but this may change
	//Andon: Changed to support up to 5 styles
    void CycleStyleHUD()
    {
        int style = (int(m_pnumberStyleHUD->GetValue()) + 1) % 5;
        m_pnumberStyleHUD->SetValue(float(style));

        SavePreference("SoftwareHUD", (DWORD)style);

        if (m_pitemStyleHUD != NULL)
            m_pitemStyleHUD->SetString(GetStyleHUDMenuString());
    }

	//Imago 7/8/09 7/13/09
    void SetDeadzone(DWORD value)
    {
        switch (value)
        {
            case 0:
			case 31:
				//g_fJoystickDeadZone = g_fJoystickDeadZoneNone;  removed 7/23/09 causes crash imago
				//break;
			case 4:
			case 1:
				g_fJoystickDeadZone = g_fJoystickDeadZoneSmallest;
				break;
			case 10:
			case 5:
				g_fJoystickDeadZone = g_fJoystickDeadZoneSmall;
				break;
			case 30:
			case 11:
				g_fJoystickDeadZone = g_fJoystickDeadZoneLarge;
				break;
            default:
                g_fJoystickDeadZone = g_fJoystickDeadZoneLarge;
        }
		g_fInverseJoystickDeadZone = (g_fJoystickDeadZone == 0) ? 0 : g_fJoystickDeadZone - 1.0f;
        SavePreference("DeadZone", (DWORD) value);
        if (m_pitemToggleLargeDeadZone != NULL) {
           m_pitemToggleLargeDeadZone->SetString(GetDeadzoneMenuString());
        }
    }

    void ToggleVirtualJoystick()
    {
        m_bEnableVirtualJoystick = !m_bEnableVirtualJoystick;

        SavePreference("VirtualJoystick", m_bEnableVirtualJoystick);

        if (m_pitemToggleVirtualJoystick != NULL) {
            m_pitemToggleVirtualJoystick->SetString(GetVirtualJoystickMenuString());
        }
    }

    class CloseNotificationSink : public IIntegerEventSink {
    public:
        TrekWindowImpl* m_pwindow;

        CloseNotificationSink(TrekWindowImpl* pwindow) :
            m_pwindow(pwindow)
        {
        }

        bool OnEvent(IIntegerEventSource* pevent, int value)
        {
            m_pwindow->CloseMessageBox();
            return false;
        }
    };

    void ToggleOldUi()
    {
        m_pUseOldUi->SetValue(!m_pUseOldUi->GetValue());

        if (m_pitemToggleUseOldUi != NULL) {
            m_pitemToggleUseOldUi->SetString(GetOldUiMenuString());
        }

        m_pmessageBox = CreateMessageBox("Enabling or Disabling the old UI will require you to restart Allegiance.", NULL, true, false);
        m_pmessageBox->GetEventSource()->AddSink(new CloseNotificationSink(this));
        GetWindow()->GetPopupContainer()->OpenPopup(m_pmessageBox, false);

    }

    void ToggleShowJoystickIndicator()
    {
        if (m_bShowJoystickIndicator)
        {
            m_bShowJoystickIndicator = false;
            SavePreference("ShowJoystickIndicator", FALSE);
        }
        else
        {
            m_bShowJoystickIndicator = true;
            SavePreference("ShowJoystickIndicator", TRUE);
        }

        if (m_pitemToggleShowJoystickIndicator != NULL) {
            m_pitemToggleShowJoystickIndicator->SetString(GetShowJoystickIndicatorMenuString());
        }
    }

	//Imago 7/8/09 #24
    void ToggleShowGrid()
    {
		//only do this when loading, save is in the cycle
		m_bCommandGrid = !m_bCommandGrid;
    }

    void SetRadarLOD(DWORD value)
    {
		//only do this when loading, save is in the cycle
		m_radarCockpit = m_radarCommand = value;
    }

	//Imago 7/8/09 #24
    void SetGamma(ZString& value)
    {
       //we save only when loading non-default or terminate()
		GetEngine()->SetGammaLevel(atof(ZString(value)));
        SavePreference("Gamma", value);
    }

	// #294 - Turkey returns true if the requested value was within range (1-10), false otherwise
	bool SetChatLines(DWORD value)
	{
		bool bInRange = false;
		if (value >= 1)
		{
			if (value > 10) m_pnumberChatLinesDesired->SetValue(10.0f);
			else
			{
				m_pnumberChatLinesDesired->SetValue((float)value);
				bInRange = true;
			}
		}
		else
		{
			m_pnumberChatLinesDesired->SetValue(1.0f);
		}

		m_pnumberChatLines->SetValue(m_pnumberChatLinesDesired->GetValue());

		return bInRange;
	}


    void ToggleFlipY()
    {
        m_bFlipY = !m_bFlipY;

        SavePreference("FlipY", m_bFlipY);

        if (m_pitemToggleFlipY != NULL) {
            m_pitemToggleFlipY->SetString(GetFlipYMenuString());
        }
    }

    void ToggleEnableFeedback()
    {
        m_bEnableFeedback = !m_bEnableFeedback;

        SavePreference("EnableFeedback", m_bEnableFeedback);

        if (m_pitemToggleEnableFeedback != NULL) {
            m_pitemToggleEnableFeedback->SetString(GetEnableFeedbackMenuString());
        }
    }
	
	//Imago 7/10
    void ToggleEnableFFAutoCenter()
    {
		if (GetInputEngine() == NULL || GetInputEngine()->GetJoystick(0) == NULL)
			return;

        m_bFFAutoCenter = !m_bFFAutoCenter;

        SavePreference("FFAutoCenter", m_bFFAutoCenter);

        if (m_pitemToggleFFAutoCenter != NULL) {
            m_pitemToggleFFAutoCenter->SetString(GetFFAutoCenterMenuString());
        }
		GetInputEngine()->GetJoystick(0)->SetRanges();
    }

    void RenderSizeChanged(bool bSmaller)
    {
        EngineWindow::RenderSizeChanged(bSmaller);
        if (bSmaller && GetFullscreen()) {
            m_pwrapNumberStyleHUD->SetWrappedValue(new Number(1.0f));
        } else {
            m_pwrapNumberStyleHUD->SetWrappedValue(m_pnumberStyleHUD);
        }
    }

    // Reloads the sound mdls.
    // this is a hack added to make art's life easier
    void ResetSound()
    {
        // reload the sound-related MDLs
        GetModeler()->UnloadNameSpace("quickchat");
        GetModeler()->UnloadNameSpace("sounddef");
        InitializeQuickChatMenu();
        InitializeSoundTemplates();

        // reset the SFX
        trekClient.ResetSound();
    }

    void SwitchSoundQuality()
    {
        switch (m_soundquality)
        {
        case ISoundEngine::minQuality:
            m_soundquality = ISoundEngine::midQuality;
            break;

        case ISoundEngine::midQuality:
            m_soundquality = ISoundEngine::maxQuality;
            break;

        case ISoundEngine::maxQuality:
            m_soundquality = ISoundEngine::minQuality;
            break;

        default:
            ZAssert(false);
            m_soundquality = ISoundEngine::minQuality;
            break;
        }

        ZSucceeded(m_pSoundEngine->SetQuality(m_soundquality));
        SavePreference("SoundQuality", (DWORD)m_soundquality);

        if (m_pitemSoundQuality != NULL)
            m_pitemSoundQuality->SetString(GetSoundQualityMenuString());
    };

	//Imago #215 8/10
    void SwitchMouseAccel()
    {
        switch (m_iMouseAccel)
        {
        case 0:
            m_iMouseAccel = 1;
            break;

        case 1:
            m_iMouseAccel = 2;
            break;

        case 2:
            m_iMouseAccel = 0;
            break;

        default:
            ZAssert(false);
            m_iMouseAccel = 0;
            break;
        }

		GetInputEngine()->GetMouse()->SetAccel(m_iMouseAccel);
		SavePreference("MouseAcceleration", (DWORD)m_iMouseAccel);

        if (m_pitemToggleMouseAccel != NULL)
            m_pitemToggleMouseAccel->SetString(GetMouseAccelMenuString());
    };

    void SwitchWheelDelay() //Spunky #282
    {
		if (++m_iWheelDelay == 5)
			m_iWheelDelay = 0;

        SavePreference("WheelDelay", (DWORD)m_iWheelDelay);

		if (m_pitemToggleWheelDelay != NULL)
            m_pitemToggleWheelDelay->SetString(GetWheelDelayMenuString());
	};

    void ToggleSoundHardware()
    {
        m_bEnableSoundHardware = !m_bEnableSoundHardware;

        SavePreference("SoundHardwareAcceleration", (DWORD)m_bEnableSoundHardware);

        ZSucceeded(m_pSoundEngine->EnableHardware(m_bEnableSoundHardware));

        if (m_pitemToggleSoundHardware != NULL)
            m_pitemToggleSoundHardware->SetString(GetSoundHardwareMenuString());
    }

	void ToggleUseDSound8()
	{
		m_bUseDSound8 = !m_bUseDSound8;

		SavePreference("UseDSound8", (DWORD)m_bUseDSound8);

		if(m_pitemToggleDSound8Usage != NULL)
			m_pitemToggleDSound8Usage->SetString(GetDSound8EnabledString());
	}

    void AdjustSFXVolume(float fDelta)
    {
        float fNewValue = std::min(0.0f, std::max(c_nMinGain, m_pnumSFXGain->GetValue() + fDelta));
        m_pnumSFXGain->SetValue(fNewValue);

        SavePreference("SFXGain", (DWORD)-fNewValue);

        if (m_pitemSFXVolumeUp != NULL)
        {
            m_pitemSFXVolumeUp->SetString(
                GetGainMenuString("Sound Effect", m_pnumSFXGain->GetValue(), c_fVolumeDelta));
        }
        if (m_pitemSFXVolumeDown != NULL)
        {
            m_pitemSFXVolumeDown->SetString(
                GetGainMenuString("Sound Effect", m_pnumSFXGain->GetValue(), -c_fVolumeDelta));
        }
    }

    void AdjustVoiceOverVolume(float fDelta)
    {
        float fNewValue = std::min(0.0f, std::max(c_nMinGain, m_pnumVoiceOverGain->GetValue() + fDelta));
        m_pnumVoiceOverGain->SetValue(fNewValue);

        SavePreference("VoiceOverGain", (DWORD)-fNewValue);

        if (m_pitemVoiceOverVolumeUp != NULL)
        {
            m_pitemVoiceOverVolumeUp->SetString(
                GetGainMenuString("Voice Over", m_pnumVoiceOverGain->GetValue(), c_fVolumeDelta));
        }
        if (m_pitemVoiceOverVolumeDown != NULL)
        {
            m_pitemVoiceOverVolumeDown->SetString(
                GetGainMenuString("Voice Over", m_pnumVoiceOverGain->GetValue(), -c_fVolumeDelta));
        }
    }

	//Imago 7/10 #187
    void AdjustFFGain(float fDelta)
    {
        float fNewValue = std::min(10000.0f, std::max(c_nMinFFGain, m_pnumFFGain->GetValue() + fDelta));
        m_pnumFFGain->SetValue(fNewValue);

        SavePreference("FFGain", fNewValue);

        if (m_pitemToggleFFGainUp != NULL)
        {
            m_pitemToggleFFGainUp->SetString(
                GetFFGainMenuString(m_pnumFFGain->GetValue(), c_fFFGainDelta));
        }
        if (m_pitemToggleFFGainDown != NULL)
        {
            m_pitemToggleFFGainDown->SetString(
                GetFFGainMenuString(m_pnumFFGain->GetValue(), -c_fFFGainDelta));
        }

		if (GetInputEngine() != NULL && GetInputEngine()->GetJoystick(0) != NULL)
			GetInputEngine()->GetJoystick(0)->SetRanges();
    }

    void AdjustMouseSens(float fDelta)
    {
        float fNewValue = std::min(2.0f, std::max(0.1f, m_pnumMouseSens->GetValue() + fDelta));
        m_pnumMouseSens->SetValue(fNewValue);

        SavePreference("MouseSensitivity", ZString(fNewValue));

        if (m_pitemToggleMouseSensUp != NULL)
        {
            m_pitemToggleMouseSensUp->SetString(
                GetMouseSensMenuString(m_pnumMouseSens->GetValue(), c_fMouseSensDelta));
        }
        if (m_pitemToggleMouseSensDown != NULL)
        {
            m_pitemToggleMouseSensDown->SetString(
                GetMouseSensMenuString(m_pnumMouseSens->GetValue(), -c_fMouseSensDelta));
        }
		
		GetInputEngine()->GetMouse()->SetSensitivity(fNewValue);
    }
	//Imago

    ZString GetPostersMenuString()
    {
        return (m_pwrapImagePosters->GetImage() != Image::GetEmpty()) ? "Posters On " : "Posters Off ";
    }

    ZString GetDebrisMenuString()
    {
		//LANS - multiple debris options
		static const ZString	c_strLow("Debris Low");
		static const ZString	c_strMed("Debris Medium");
		static const ZString	c_strHigh("Debris High");
		static const ZString	c_strOff("Debris Off");

		if (m_debrisDensity->GetValue() == 0.8f) {
			return c_strHigh;
		}
		else if (m_debrisDensity->GetValue() == 1.0f) {
			return c_strMed;
		}
		else if (m_debrisDensity->GetValue() == 1.5f) {
			return c_strLow;
		}
		else {
			return c_strOff;
		}
        //return (m_pwrapGeoDebris->GetGeo() != Geo::GetEmpty())   ? "Debris On " : "Debris Off ";
    }

    ZString GetEnvironmentMenuString()
    {
        return (m_pwrapImageEnvironment->GetImage() == m_pimageEnvironment) ? "Environment On " : "Environment Off ";
    }

// yp Your_Persona August 2 2006 : MaxTextureSize Patch //Imago 7/18/09
	ZString GetMaxTextureSizeMenuString()
    {
		int i = 0;
		int j = 2;

		i = 8 + g_DX9Settings.m_iMaxTextureSize;

		j = pow((float)j,(float)i);
        return "Max Texture Size ("  + ZString( j)  + ") ";
    }

	/* pkk May 6th: Disabled bandwidth patch
	// w0dk4 June 2007: Bandwith Patch
	ZString GetBandwidthMenuString()
	{
		if(trekClient.MaxBandwidth() == 2)
			return "Bandwidth: Dial-Up (33k)";
		if(trekClient.MaxBandwidth() == 4)
			return "Bandwidth: Dial-Up (56k)";
		if(trekClient.MaxBandwidth() == 8)
			return "Bandwidth: Broadband (128k)";
		if(trekClient.MaxBandwidth() == 16)
			return "Bandwidth: Broadband (512k)";
		if(trekClient.MaxBandwidth() == 32)
			return "Bandwidth: Broadband (>1mbit)";

		return "Error";
	}*/


	// Imago #215 8/10
    ZString GetMouseSensMenuString(float fCurrentSens, float fDelta)
    {
        ZString strResult = ((fDelta > 0) ? "Raise " : "Lower ") + ZString("Mouse Sensitivity ");
        if (fCurrentSens >= 2 && fDelta > 0)
        {
            strResult += "(highest)";
        }
        else if (fCurrentSens <= 0.1f && fDelta < 0)
        {
            strResult += "(lowest)";
        }
        else
        {
			float value = (std::min(2.0f, std::max(0.1f, fCurrentSens + fDelta))) * 100;
			char szValue[4] = {'\0'};
			sprintf(szValue,"%.0f",value);
            strResult += "to " + ZString(szValue) + " %";
        }

        return strResult;
    }

	ZString GetMouseAccelMenuString()
	{
		if(m_iMouseAccel == 0)
			return "Mouse Acceleration: Off";
		if(m_iMouseAccel == 1)
			return "Mouse Acceleration: Low (2x)";
		if(m_iMouseAccel == 2)
			return "Mouse Acceleration: High (4x)";

		return "Error";
	}

	//Spunky #282
	ZString GetWheelDelayMenuString()
	{
		static const ZString str[] = {"Max", "High", "Med", "Min", "None"};
		if (m_iWheelDelay >= 0 && m_iWheelDelay < 5)
			return "Wheel & Keyboard Throttle Delay: " + str[m_iWheelDelay];
		return "Error";
	}

	//Imago 7/10
    ZString GetFFAutoCenterMenuString()
    {
        return (m_bFFAutoCenter) ? "Auto Center On" : "Auto Center Off ";
    }
	//

    ZString GetRoundRadarMenuString()
    {
        return (m_bRoundRadar) ? "Round Radar" : "Square Radar";
    }

    ZString GetCensorChatsMenuString()
    {
        return (CensorDisplay ()) ? "Censor Display" : "Don't Censor Display";
    }

    ZString GetStickyChaseMenuString ()
    {
        return m_bPreferChaseView ? "Default To Chase View" : "Default To Cockpit View";
    }

    ZString GetFilterChatsToAllMenuString()
    {
        return trekClient.FilterChatsToAll() ? "Filter Chats Sent To All" : "Don't Filter Chats Sent To All";
    }

    ZString GetFilterQuickCommsMenuString()
    {
        return trekClient.FilterQuickComms() ? "Filter Voice Commands" : "Don't Filter Voice Commands";
    }

	//TheBored 30-JUL-07: Filter Unknown Chat patch
    ZString GetFilterUnknownChatsString()
    {
        return trekClient.FilterUnknownChats() ? "Filter Unknown Chats" : "Don't Filter Unknown Chats";
    }

    ZString GetFilterLobbyChatsMenuString()
    {
		//TheBored 25-JUN-07: Added a new option so the user can choose if PMs are filtered.
        switch (trekClient.FilterLobbyChats())
        {
            case 1:
                return "Filter Chats Sent From Lobby";
                break;
            case 2:
                return "Filter Chats Sent From Lobby Including PMs";
                break;
            case 3:
                return "Don't Filter Chats Sent From Lobby";
                break;
            default:
                return "Default Case"; //TB: Shouldn't happen, but left in for testing.
                break;
        }
    }

	// #294 - Turkey
	ZString GetIncreaseChatLinesMenuString()
	{
		if (m_pnumberChatLinesDesired->GetValue() > 9.9f) return "Chat Lines At Maximum";
		return "Increase To " + ZString((int)m_pnumberChatLinesDesired->GetValue() + 1) + " Chat Lines";
	}

	// #294 - Turkey
	ZString GetReduceChatLinesMenuString()
	{
		if (m_pnumberChatLinesDesired->GetValue() < 1.1f) return "Chat Lines At Minimum";
		return "Reduce To " + ZString((int)m_pnumberChatLinesDesired->GetValue() - 1) + " Chat Lines";
	}

    ZString GetLinearControlsMenuString()
    {
        return (m_bLinearControls) ? "Linear Joystick Control Response" : "Quadratic Joystick Control Response";
    }

    ZString GetStarsMenuString()
    {
        return (m_pwrapImageStars->GetImage() != Image::GetEmpty()) ? "Stars On " : "Stars Off ";
    }

    ZString GetStrobesMenuString()
    {
        return ThingGeo::GetShowLights() ? "Strobes On " : "Strobes Off ";
    }

    ZString GetTrailsMenuString()
    {
        return ThingGeo::GetShowTrails() ? "Trails On " : "Trails Off ";
    }

    ZString GetBoundsMenuString()
    {
        return ThingGeo::GetShowBounds() ? "Bounds On " : "Bounds Off ";
    }

    ZString GetTransparentObjectsMenuString()
    {
        return ThingGeo::GetTransparentObjects() ? "TransparentObjects On " : "TransparentObjects Off ";
    }

    ZString GetLensFlareMenuString()
    {
        return (m_bLensFlare) ? "Lens Flare On " : "Lens Flare Off ";
    }

    ZString GetSmokeMenuString()
    {
        switch (ThingGeo::GetShowSmoke())
        {
            case 0:
                return ZString ("Particles Off");
                break;
            case 2: //Imago 8/16/09
                return ZString ("Particles On - Performance");
                break;
            case 1:
                return ZString ("Particles On - Low Quality");
                break;
            case 3:
                return ZString ("Particles On - Medium Quality");
                break;
            default:
                return ZString ("Particles On - High Quality");
                break;
        }
    }

    ZString GetSoundQualityMenuString()
    {
        switch (m_soundquality)
        {
        case ISoundEngine::minQuality:
            return "Sound Quality: Low";

        case ISoundEngine::midQuality:
            return "Sound Quality: Default";

        case ISoundEngine::maxQuality:
            return "Sound Quality: High";

        default:
            ZAssert(false);
            return "<bug>";
        }
    }

    ZString GetSoundHardwareMenuString()
    {
        return m_bEnableSoundHardware ? "Sound Card Acceleration On" : "Sound Card Acceleration Off";
    }

	// mdv new sound8 or old sound engine
	// mmf changed wording
	ZString GetDSound8EnabledString()
	{
		return m_bUseDSound8 ? "DirectSound (Restart Reqd.): New" : "DirectSound (Restart Reqd.): Old";
	}

    ZString GetBidirectionalLightingMenuString()
    {
        return (m_bBidirectionalLighting) ? "Bidirectional Lighting On " : "Bidirectional Lighting Off ";
    }

    ZString GetChatHistoryHUDMenuString()
    {
        return (m_pboolChatHistoryHUD->GetValue()) ? "Chat History On " : "Chat History Off ";
    }

    ZString GetCenterHUDMenuString()
    {
        return (m_pboolCenterHUD->GetValue()) ? "Center HUD On " : "Center HUD Off ";
    }

    ZString GetTargetHUDMenuString()
    {
        return (m_pboolTargetHUD->GetValue()) ? "Target HUD On " : "Target HUD Off ";
    }

    //Andon: Expanding the number of HUD style switches available
	const ZString& GetStyleHUDMenuString()
    {
        static const ZString    c_strNormal("Style: Normal");
        static const ZString    c_strSoftware("Style: Software");
		static const ZString    c_strCust1("Style: Custom Hud 1");//Add in the first custom one
		static const ZString    c_strCust2("Style: Custom Hud 2");//Add in the second custom one
		static const ZString    c_strCust3("Style: Custom Hud 3");//Add in the third custom one
		static const ZString    c_strOops("Style: Error"); //Just in case I goofed

		if (m_pnumberStyleHUD->GetValue() == 0)
		{
			return c_strNormal;
		}
		else if (m_pnumberStyleHUD->GetValue() == 1)
		{
			return c_strSoftware;
		}
		else if (m_pnumberStyleHUD->GetValue() == 2)
		{
			return c_strCust1;
		}
		else if (m_pnumberStyleHUD->GetValue() == 3)
		{
			return c_strCust2;
		}
		else
		{
			return c_strCust3;
		}
		//Andon: The old version, was simply True/False
		//return (m_pnumberStyleHUD->GetValue()) ? c_strSoftware : c_strNormal;
    }

    const ZString& GetDeadzoneMenuString()
    {
		static const ZString    strLarge = "Large dead zone";
		//static const ZString    strNone = "No dead zone";
		static const ZString    strSmall = "Small dead zone";
		static const ZString    strSmallest = "Smallest dead zone";
		static const ZString    strInvalid = "Invalid dead zone";
		int     iDZ = int(g_fJoystickDeadZone * 100);
        switch (iDZ)
        {
           // case 0:
			//	return strNone;
			case 4:
				return strSmallest;
			case 10:
				return strSmall;
			case 30:
				return strLarge;
            default:
				return strInvalid;
        }
    }

    ZString GetVirtualJoystickMenuString()
    {
        return (m_bEnableVirtualJoystick ? "Use Mouse As Virtual Joystick: On " : "Use Mouse As Virtual Joystick: Off ");
    }

    ZString GetFlipYMenuString()
    {
        return (m_bFlipY ? "Y Axis Flipped " : "Y Axis Not Flipped ");
    }

    ZString GetOldUiMenuString()
    {
        return "Use old UI: " + ZString(m_pUseOldUi->GetValue() ? "On" : "Off");
    }

    ZString GetShowJoystickIndicatorMenuString()
    {
        return "Joystick Indicator: " + ZString(m_bShowJoystickIndicator ? "On" : "Off");
    }

    ZString GetEnableFeedbackMenuString()
    {
        return (m_bEnableFeedback ? "Force Feedback Enabled " : "Force Feedback Disabled ");
    }

    ZString GetGainMenuString(const ZString& strSoundType, float fCurrentGain, float fDelta)
    {
        ZString strResult = ((fDelta > 0) ? "Raise " : "Lower ") + strSoundType + " Volume ";
        if (fCurrentGain >= 0 && fDelta > 0)
        {
            strResult += "(maxed)";
        }
        else if (fCurrentGain <= c_nMinGain && fDelta < 0)
        {
            strResult += "(off)";
        }
        else
        {
            strResult += "to " + ZString(std::min(0.0f, std::max(c_nMinGain, fCurrentGain + fDelta))) + " dB";
        }

        return strResult;
    }

	//Imago 7/10
    ZString GetFFGainMenuString(float fCurrentGain, float fDelta)
    {
        ZString strResult = ((fDelta > 0) ? "Raise " : "Lower ") + ZString("Force Feedback Gain ");
        if (fCurrentGain >= 10000 && fDelta > 0)
        {
            strResult += "(maxed)";
        }
        else if (fCurrentGain <= c_nMinFFGain && fDelta < 0)
        {
            strResult += "(off)";
        }
        else
        {
            strResult += "to " + ZString(std::min(10000.0f, std::max(c_nMinFFGain, fCurrentGain + fDelta) / 100)) + " %";
        }

        return strResult;
    }

	//imago WIP 6/30/09 7/18/09

	ZString GetAAString()
	{
		return "Antialiasing (" + ZString(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType) + ")";
	}
	ZString GetMipString()
	{
		ZString strResult = (CD3DDevice9::Get()->GetDeviceSetupParams()->bAutoGenMipmap) ? "Yes" : "No";
		return "Auto Mipmap (" + strResult + ")";
	}
	ZString GetVsyncString()
	{
		ZString strResult = (CD3DDevice9::Get()->GetDeviceSetupParams()->bWaitForVSync) ? "On" : "Off";
		return "Vertical Sync (" + strResult + ")";
	}

    void DoInputConfigure()
    {
        CloseMenu();
        TRef<IPopup> ppopup =
            m_ptrekInput->CreateInputMapPopup(
                GetModeler(),
                TrekResources::SmallFont(),
                GetTime()
            );

        GetPopupContainer()->OpenPopup(ppopup);
    }

    void CloseMenu()
    {
        GetPopupContainer()->ClosePopup(m_pmenu);
        m_pmenu = NULL;
    }


    void OnGameState()
    {
        GetPopupContainer()->OpenPopup(
            CreateMissionParametersPopup(GetModeler())
        );
    }

	// w0dk4 player-pings feature
	void ShowPlayerPings()
    {
		const ShipListIGC* ships = trekClient.m_pCoreIGC->GetShips();

		ZString str1;
		str1 += "<Color|yellow><Font|medBoldVerdana>Connection Info of Players<Font|smallFont><Color|white><p><p>";
		ZString str2;
		PlayerInfo* pPlayerInfo;
		ShipID shipID;

		int iAveragePing = 0;
		int iAverageLoss = 0;
		int iShips = 0;

		for (ShipLinkIGC*   l = ships->first();
                     (l != NULL);
                     l = l->next())
        {
            IshipIGC*  s = l->data();

			unsigned int m_ping;
			unsigned int m_loss;

			if ((pPlayerInfo = (PlayerInfo*)s->GetPrivateData()) && (pPlayerInfo == trekClient.GetPlayerInfo())){
				pPlayerInfo->GetConnectionData(&m_ping,&m_loss);

				if(m_ping > 1000)
					m_ping = 999;
				if(m_ping < 10)
					m_ping = 10;

				if(m_loss > 100)
					m_loss = 99;

				iAveragePing += m_ping;
				iAverageLoss += m_loss;
				iShips++;

				ZString formatPing;
				if(m_ping < 150)
					formatPing = "<Color|green>";
				else if(m_ping < 300)
					formatPing = "<Color|yellow>";
				else
					formatPing = "<Color|red>";

				ZString formatLoss;
				if(m_loss < 5)
					formatLoss = "<Color|green>";
				else if(m_loss < 15)
					formatLoss = "<Color|yellow>";
				else
					formatLoss = "<Color|red>";

				str1 += "<Color|cyan>Your Connection Info<Color|white><p>Ping: " + formatPing + ZString((int)m_ping) + "ms<Color|white>	Packet Loss: " + formatLoss + ZString((int)m_loss) + "%<Color|white><p><p>";

			} else if ((pPlayerInfo = (PlayerInfo*)s->GetPrivateData()) && pPlayerInfo->IsHuman()) {
				pPlayerInfo->GetConnectionData(&m_ping,&m_loss);

				if(m_ping > 1000)
					m_ping = 999;
				if(m_ping < 10)
					m_ping = 10;

				if(m_loss > 100)
					m_loss = 99;

				iAveragePing += m_ping;
				iAverageLoss += m_loss;
				iShips++;

				ZString formatPing;
				if(m_ping < 150)
					formatPing = "<Color|green>";
				else if(m_ping < 300)
					formatPing = "<Color|yellow>";
				else
					formatPing = "<Color|red>";

				ZString formatLoss;
				if(m_loss < 5)
					formatLoss = "<Color|green>";
				else if(m_loss < 15)
					formatLoss = "<Color|yellow>";
				else
					formatLoss = "<Color|red>";

				str2 += "Ping: " + formatPing + ((m_ping < 100) ? ZString("0") : ZString("")) + ZString((int)m_ping)
					+ "ms<Color|white>	Packet Loss: " + formatLoss + ((m_loss < 10) ? ZString("0") : ZString("")) + ZString((int)m_loss) + "%<Color|white>		Name: <Color|cyan>" + ZString(pPlayerInfo->CharacterName()) + "<Color|white><p>";
			}
		}

		if(iShips > 0) {
			iAveragePing = iAveragePing/iShips;
			iAverageLoss = iAverageLoss/iShips;

			ZString formatPing;
				if(iAveragePing < 150)
					formatPing = "<Color|green>";
				else if(iAveragePing < 300)
					formatPing = "<Color|yellow>";
				else
					formatPing = "<Color|red>";

				ZString formatLoss;
				if(iAverageLoss < 5)
					formatLoss = "<Color|green>";
				else if(iAverageLoss < 15)
					formatLoss = "<Color|yellow>";
				else
					formatLoss = "<Color|red>";

			str2 += "<p>Average Ping: " + formatPing + ZString(iAveragePing) + "ms<Color|white>	Average Packet Loss: " + formatLoss + ZString(iAverageLoss) + "%";
		}

        GetPopupContainer()->OpenPopup(
			CreateMMLPopup(
            GetModeler(),
            (str1+str2),
            true)
			);
    }
    // end w0dk4 player-pings feature
    void OnMenuCommand(IMenuItem* pitem)
    {
        switch (pitem->GetID()) {
            case idmConfigure:
                DoInputConfigure();
                break;

            case idmHelp:
                CloseMenu();
                OnHelp(true);
                break;

            case idmGameDetails:
                CloseMenu();
                OnGameState();
                break;

			// w0dk4 player-pings feature
			case idmPings:
				{
					CloseMenu();
					if (trekClient.m_fm.IsConnected()){
						trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
						BEGIN_PFM_CREATE(trekClient.m_fm, pfmPingDataReq, C, REQPINGDATA)
						END_PFM_CREATE
					}
				}
				break;

            case idmExitGame:
                CloseMenu();
                StartQuitMission();
                break;

            case idmExitApp:
                CloseMenu();
                StartClose();
                break;

            case idmTogglePosters:
                TogglePosters();
                break;

            case idmToggleDebris:
                ToggleDebris();
                break;

            case idmToggleStars:
                ToggleStars();
                break;

            case idmToggleEnvironment:
                ToggleEnvironment();
                break;

            case idmOldUi:
                ToggleOldUi();
                break;

            case idmShowJoystickIndicator:
                ToggleShowJoystickIndicator();
                break;

			/* pkk May 6th: Disabled bandwidth patch
			// w0dk4 June 2007: Bandwith Patch
			case idmBandwidth:
				ToggleBandwidth(trekClient.MaxBandwidth()*2);
				if(trekClient.m_fLoggedOn) {
					// send change to server if connected
					trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
					BEGIN_PFM_CREATE(trekClient.m_fm, pfmBandwidth, C, BANDWIDTH)
					END_PFM_CREATE
					pfmBandwidth->value = trekClient.MaxBandwidth();
				}
                break;*/

			//Imago 7/18/09
			// yp Your_Persona August 2 2006 : MaxTextureSize Patch
            case idmMaxTextureSize:
				//ToggleMaxTextureSize(trekClient.MaxTextureSize()+1); Obsolete REMOVE REVIEW, extra, unneeded functions
				GetEngine()->SetMaxTextureSize(g_DX9Settings.m_iMaxTextureSize + 1);
				SavePreference("MaxTextureSize", g_DX9Settings.m_iMaxTextureSize);
				if (m_pitemMaxTextureSize != NULL) {
					m_pitemMaxTextureSize->SetString(GetMaxTextureSizeMenuString());
				}
				break;

			case idmAA:
				GetEngine()->SetAA(g_DX9Settings.m_dwAA + 1);
				SavePreference("UseAntialiasing", g_DX9Settings.m_dwAA);
				if (m_pitemAA != NULL) {
					m_pitemAA->SetString(GetAAString());
				}
				break;
			case idmMip:
				GetEngine()->SetAutoGenMipMaps(!g_DX9Settings.m_bAutoGenMipmaps);
				SavePreference("UseAutoMipMaps", g_DX9Settings.m_bAutoGenMipmaps);
				if (m_pitemMip != NULL) {
					m_pitemMip->SetString(GetMipString());
				}
				break;

			case idmVsync:
				//only does anything if the device is fullscreen...but we'll let them push it anyways.
				GetEngine()->SetVSync(!g_DX9Settings.m_bVSync);
				SavePreference("UseVSync", g_DX9Settings.m_bVSync);
				if (m_pitemVsync != NULL) {
					m_pitemVsync->SetString(GetVsyncString());
				}
				break;

            case idmToggleCensorChats:
                ToggleCensorChats ();
                break;

            case idmToggleStickyChase:
                ToggleStickyChase ();
                break;

            case idmFilterChatsToAll:
                ToggleFilterChatsToAll();
                break;

            case idmFilterQuickComms:
                ToggleFilterQuickComms();
                break;

			//TheBored 30-JUL-07: Filter Unknown Chat patch
			case idmFilterUnknownChats:
                ToggleFilterUnknownChats();
                break;

            case idmFilterLobbyChats:
                //TheBored 25-JUN-07: Lobby filter change.
				ToggleFilterLobbyChats(trekClient.FilterLobbyChats() + 1);
                break;

				// #294 - Turkey
			case idmIncreaseChatLines:
				IncreaseChatLines();
				break;

				// #294 - Turkey
			case idmReduceChatLines:
				ReduceChatLines();
				break;

            case idmToggleLinearControls:
                ToggleLinearControls ();
                break;

            case idmToggleLargeDeadZone:
                SetDeadzone( (g_fJoystickDeadZone * 100) + 1 ); //Imago 7/8/09 //ToggleLargeDeadZone(); 7/13/09
                break;

            case idmToggleVirtualJoystick:
                ToggleVirtualJoystick();
                break;

            case idmToggleFlipY:
                ToggleFlipY();
                break;

            case idmToggleEnableFeedback:
                ToggleEnableFeedback();
                break;

            case idmToggleStrobes:
                ToggleStrobes();
                break;

            case idmToggleTrails:
                ToggleTrails();
                break;

            case idmToggleBounds:
                ToggleBounds();
                break;

            case idmToggleTransparentObjects:
                ToggleTransparentObjects();
                break;

            case idmToggleSmoke:
                ToggleSmoke();
                break;

            case idmToggleLensFlare:
                ToggleLensFlare();
                break;

            case idmToggleBidirectionalLighting:
                ToggleBidirectionalLighting();
                break;

            case idmToggleChatHistoryHUD:
                ToggleChatHistoryHUD();     //Not persisted
                break;

            case idmToggleCenterHUD:
                ToggleCenterHUD();
                break;

            case idmToggleTargetHUD:
                ToggleTargetHUD();
                break;

            case idmStyleHUD:
                CycleStyleHUD();
                break;

            case idmResetSound:
                ResetSound();
                break;

            case idmSoundQuality:
                SwitchSoundQuality();
                break;

            case idmSoundHardware:
                ToggleSoundHardware();
                break;

			case idmUseDSound8:
				ToggleUseDSound8();
				break;

            case idmSFXVolumeUp:
                AdjustSFXVolume(c_fVolumeDelta);
                break;

            case idmSFXVolumeDown:
                AdjustSFXVolume(-c_fVolumeDelta);
                break;

            case idmVoiceOverVolumeUp:
                AdjustVoiceOverVolume(c_fVolumeDelta);
                break;

            case idmVoiceOverVolumeDown:
                AdjustVoiceOverVolume(-c_fVolumeDelta);
                break;
			// YP: Add cases for rightclick lobby patch
			case idmContextAcceptPlayer:
				contextAcceptPlayer();	CloseMenu();
				break;

			case idmContextRejectPlayer:
				contextRejectPlayer();	CloseMenu();
				break;

			case idmContextMakeLeader:
				contextMakeLeader();	CloseMenu();
				break;

			case idmContextMutePlayer:
				contextMute();			CloseMenu();
				break;

				// BT - STEAM - Enable moderators to ban players by context menu.
			case idmContextKickPlayer:
				contextKickPlayer();			CloseMenu();
				break;

				// BT - STEAM - Enable moderators to ban players by context menu.
			case idmContextBanPlayer:
				contextBanPlayer();				CloseMenu();
				break;

			//Xynth #48 8/2010
			case idmContextDockDrone:
				contextDockDrone();		CloseMenu();
				break;
			//Xynth #197 8/2010
			case idmContextChat:
				contextChat();		CloseMenu();
				break;	

			//Imago #187 7/10
			case idmFFAutoCenter:
				ToggleEnableFFAutoCenter();
				break;

			case idmFFGainUp:
				AdjustFFGain(c_fFFGainDelta);
				break;

			case idmFFGainDown:
				AdjustFFGain(-c_fFFGainDelta);
				break;

			// #215 8/10
			case idmMouseAccel:
				SwitchMouseAccel();
				break;

			//Spunky #282
			case idmWheelDelay: 
				SwitchWheelDelay();
				break;

			case idmMouseSensUp:
				AdjustMouseSens(c_fMouseSensDelta);
				break;

			case idmMouseSensDown:
				AdjustMouseSens(-c_fMouseSensDelta);
				break;
			// End Imago

#ifdef STEAM_APP_ID
			// BT - STEAM
			case idmCallsignTag0:
			case idmCallsignTag1:
			case idmCallsignTag2:
			case idmCallsignTag3:
			case idmCallsignTag4:
			case idmCallsignTag5:
			case idmCallsignTag6:
			case idmCallsignTag7:
			case idmCallsignTag8:
			case idmCallsignTag9:
				SetPlayerCallsign(pitem->GetID());
				CloseMenu();
				break;

				// BT - STEAM
			case idmCallsignTagNone:
				UnsetPlayerCallsign();
				CloseMenu();
				break;

				// BT - STEAM
			case idmToken0:
			case idmToken1:
			case idmToken2:
			case idmToken3:
			case idmToken4:
				SetPlayerToken(pitem->GetID());
				CloseMenu();
				break;
#endif
        }
    }

    void SetJiggle(float jiggle)
    {
        m_cameraControl.SetJiggle(jiggle);
    }

    void OverrideCamera(Time now, ImodelIGC* pmodelTarget, bool bOverridePosition)
    {
        m_viewmode = vmOverride;
        m_pwrapImageBackdrop->SetImage(trekClient.GetCluster() ? m_pgroupImage3D : Image::GetEmpty());
		m_pscreen = NULL;
		m_pimageScreen = NULL;

        SetCameraMode(cmExternalOverride);

        m_cameraControl.SetAnimatePosition(false);
        m_cameraControl.SetAnimation(0.0f);
        m_cameraControl.SetFOV(s_fDefaultFOV);

        // always use the upright coordinate system as a starting point here
        //m_cameraControl.SetOrientation (trekClient.GetShip()->GetSourceShip()->GetOrientation());
        Orientation     orthogonal (Vector (1.0f, 0.0f, 0.0f), Vector (0.0f, 0.0f, 1.0f));
        m_cameraControl.SetOrientation (orthogonal);

        // this controls how long the launch animation lasts
        m_timeOverrideStop = now + (bOverridePosition ? 5.0f : 3.0f);

        m_bUseOverridePosition = bOverridePosition;

        if (bOverridePosition)
        {
            IshipIGC*       pshipSource = trekClient.GetShip()->GetSourceShip();
            IclusterIGC*    pcluster = pshipSource->GetCluster();
            float           offset = pshipSource->GetRadius() * 2.0f;

            const Vector*   pPositionTarget;
            if (pmodelTarget && (pmodelTarget->GetCluster() == pcluster))
            {
                pPositionTarget = &(pmodelTarget->GetPosition());
                if (pmodelTarget->GetObjectType() == OT_ship)
                    SetTarget(pmodelTarget, c_cidNone);
            }
            else
            {
                static Vector   positionBfr;
                positionBfr = pshipSource->GetPosition() + pshipSource->GetOrientation().GetBackward() * offset;
                pPositionTarget = &positionBfr;
            }

            trekClient.GetShip()->SetCluster(NULL);
            trekClient.SetViewCluster(pcluster);
            TrackCamera(pshipSource->GetPosition(), *pPositionTarget,
                        offset * 3.0f);
        }
        else
        {
            m_positionOverrideTarget = pmodelTarget->GetPosition();
        }

        UpdateOverlayFlags();
    }

    void EndOverrideCamera(void)
    {
        if (m_bUseOverridePosition)
            SetTarget(NULL, c_cidNone);

        // end the overide camera's special status
        m_cm = cmExternal;

        if (trekClient.flyingF())
        {
            SetViewMode(vmCombat);
        }
        else if (screen() == ScreenIDCombat)
        {
            if (Training::IsTraining ())
            {
                // training missions will usually just put the ship back where
                // it was and proceed. However, this call can signal us that
                // the mission doesn't want to resume for some reason, so
                // we stay in override mode if that happens. The mission *should*
                // terminate when this happens, so it shouldn't last forever.
                if (!Training::RestoreShip ())
                {
                    m_cm = cmExternalOverride;
                    m_timeOverrideStop = m_timeOverrideStop + 10.0f;
                }
            }
            else
            {
                SetViewMode(trekClient.GetShip()->IsGhost() ? vmCommand : vmHangar);
                PositionCommandView(NULL, 0.0f);
                StartSound(personalJumpSound);
                assert (trekClient.GetShip()->IsGhost() || trekClient.GetViewCluster() == NULL);
            }
        }
    }

    void SetCameraMode(CameraMode cm)
    {
        // Ignore the set display mode.
        if (m_cm == cmExternalOverride)
            return;

        // allow us to use the external chase as our default instead of the
        // cockpit. This keeps you from having to hit F11 every time you
        // launch or switch to another view. It's not fully tested, so I
        // added a registry key switch. Set "PreferChaseView" in the registry
        // to enable this feature.
        if ((cm == cmCockpit) && m_bPreferChaseView)
            cm = cmExternalChase;

        if (m_cm != cm)
        {
            m_cmOld = m_cm;
            m_cm = cm;

            if (!CommandCamera(cm))
            {
                m_cameraControl.SetAnimation (InternalCamera(cm) ? 1.0f : 2.0f);
                if (m_bLensFlare)
                    m_pwrapImageLensFlare->SetImage(m_pimageLensFlare);
            }

            // Any camera change restores the FOV
            m_cameraControl.SetFOV(s_fDefaultFOV);

            // adjust the display of damage effects for the camera view
            ThingSitePrivate* pSite; CastTo(pSite, trekClient.GetShip ()->GetSourceShip()->GetThingSite());
            TRef<ThingGeo> pThing = pSite ? pSite->GetThingGeo() : NULL;

            // turn things on or off based on internal or external camera modes
            if (InternalCamera(cm))
            {
                if (InternalCamera(m_cmOld))
                {
                    // going from internal view to internal view
                    //Never animate the change in position
                    m_cameraControl.SetAnimatePosition(false);
                }
                if (pThing)
                    pThing->SetShowDamage (false);
            }
            else if (ExternalCamera (cm))
            {
                if (pThing)
                    pThing->SetShowDamage (true);
            }

            UpdateOverlayFlags();

			// BT - 10/17 - Map and Sector Panes are now shown on launch and remember the pilots settings on last dock. 
			if (m_bShowInventoryPane == true)
				TurnOnOverlayFlags(ofInventory);

			// BT - 10/17 - Map and Sector Panes are now shown on launch and remember the pilots settings on last dock. 
			if(m_bShowSectorMapPane == true)
				TurnOnOverlayFlags(ofSectorPane);

            // REVIEW: why not expose this in MDL?
			if (GetOverlayFlags() & (ofInCockpit | ofInChase))
				m_pradarImage->SetRadarLOD((RadarImage::RadarLOD)m_radarCockpit);
            else if (GetOverlayFlags() & (ofInFlightCommand | ofInStationCommand))
                m_pradarImage->SetRadarLOD((RadarImage::RadarLOD)m_radarCommand);
            else
                m_pradarImage->SetRadarLOD(RadarImage::c_rlNone);

        }
    }

    ViewMode    GetViewMode(void) const
    {
        return m_viewmode;
    }

    void SetViewMode(ViewMode vm, bool bForce = false)
    {
        ZDebugOutput("SetViewMode(" + ZString((int)vm) + ")\n");

        //
        // Ignore the set display mode.
        //

        if (m_cm == cmExternalOverride)
            return;

        //
        // Switch the image
        //

        if (vm != m_viewmode)
        {
            debugf("Changing view mode from %d to %d\n", m_viewmode, vm);

            m_viewmode = vm;

            GetConsoleImage()->OnSwitchViewMode();

            // destroy any open popups
            if (!GetPopupContainer()->IsEmpty())
                GetPopupContainer()->ClosePopup(NULL);
            m_pmessageBox = NULL;
            m_pmessageBoxLockdown = NULL;
            GetWindow()->RestoreCursor();

            // kill any mouse capture
            m_pwrapImageTop->RemoveCapture();

			// yp - Your_Persona buttons get stuck patch. aug-03-2006
			// clear the keyboard buttons.
			m_ptrekInput->ClearButtonStates(); //Possibly not needed any more with server side fix.

			// #294 - Turkey / #361 Use different number of chatlines for the loadout screen cos there's less space
			if (m_pchatListPane)
			{
				int lines = m_pnumberChatLinesDesired->GetValue();

				if (vm == vmLoadout)
				{
					m_pchatListPane->SetChatLines(std::min(lines, 6));
					m_pnumberChatLines->SetValue(std::min(lines, 6));
				}
				else if (vm <= vmOverride)
				{
					m_pchatListPane->SetChatLines(lines);
					m_pnumberChatLines->SetValue(lines);
				}
			}

            switch (vm)
            {
            case vmHangar:
				SetCombatScreen(CreateHangarScreen(GetModeler(), "hangar"));

                // reset the current selected cluster
                trekClient.RequestViewCluster(NULL);

                UpdateOverlayFlags();
                SetCameraMode(cmCockpit);
                m_pradarImage->SetRadarLOD(RadarImage::c_rlNone);
                break;

            case vmLoadout:
				SetCombatScreen(CreateLoadout(GetModeler(), GetWindow()->GetTime()));

                // reset the current selected cluster
                trekClient.RequestViewCluster(NULL);

                UpdateOverlayFlags();
                SetCameraMode(cmCockpit);
                m_pradarImage->SetRadarLOD(RadarImage::c_rlNone);
                break;

            case vmCommand:
                SetCameraMode(m_cmPreviousCommand);
                m_pwrapImageBackdrop->SetImage(trekClient.GetCluster() ? m_pgroupImage3D : Image::GetEmpty());
                m_pscreen = NULL;
                m_pimageScreen = NULL;
                break;

            case vmCombat:
                assert (trekClient.GetViewCluster() == NULL);
                SetCameraMode(cmCockpit);
                m_pwrapImageBackdrop->SetImage(trekClient.GetCluster() ? m_pgroupImage3D : Image::GetEmpty());
				m_pscreen = NULL;
				m_pimageScreen = NULL;
                break;

            case vmOverride:
                assert(false); // must call OverrideCamera()
                break;

            case vmUI:
                assert(false); // must call screen()
                break;

            default:
                assert(false);
                break;
            }

            TurnOffOverlayFlags(c_omBanishablePanes);
            UpdateOverlayFlags();

            //
            // if we are transitioning from the override camera or UI stuff,
            // we may be locked down but not displaying the message box.
            //

            if (trekClient.IsLockedDown())
                StartLockDown(trekClient.GetLockDownReason());
        }
    }

    void EnableDisplacementCommandView(bool bEnable)
    {
        m_bEnableDisplacementCommandView = bEnable;
    }

    OverlayMask GetOverlayFlags() const
    {
        return m_voverlaymask[m_viewmode];
    }

    void SetOverlayFlags(OverlayMask om)
    {
        m_voverlaymask[m_viewmode] = om;
        if (m_pconsoleImage)
            m_pconsoleImage->SetOverlayFlags(om);

        if (om & ofTeam)
            m_pnumberTeamPaneCollapsed->SetValue(1.0f);
        else if (om & ofExpandedTeam)
            m_pnumberTeamPaneCollapsed->SetValue(0.0f);

        if ((om & ofInvestment) == 0)
            SetInvestAsteroid(0);

        m_bOverlaysChanged = true;
    }

    // the normal set & clear were ambiguous

    void TurnOnOverlayFlags(OverlayMask om)
    {
        SetOverlayFlags(m_voverlaymask[m_viewmode] | om);
    }

    void TurnOffOverlayFlags(OverlayMask om)
    {
        SetOverlayFlags(m_voverlaymask[m_viewmode] & ~om);
    }

    void ToggleOverlayFlags(OverlayMask om)
    {
        SetOverlayFlags(m_voverlaymask[m_viewmode] ^ om);
    }

    void ResetOverlayMask()
    {
        for (int i = 0; i < c_cViewModes; i++)
        {
            m_voverlaymask[i] = 0;
        }
        m_voverlaymask[vmCombat] = ofSectorPane;
        m_voverlaymask[vmCommand] = ofSectorPane;
    }

    void CopyOverlayFlags(OverlayMask om, bool bSet)
    {
        if (bSet)
            TurnOnOverlayFlags(om);
        else
            TurnOffOverlayFlags(om);
    }

    void UpdateOverlayFlags()
    {
        bool bInCommand = GetViewMode() == vmCommand;
        bool bInFlight = trekClient.GetShip()->GetCluster() != NULL;

        CopyOverlayFlags(ofInHangar, GetViewMode() == vmHangar);
        CopyOverlayFlags(ofInLoadout, GetViewMode() == vmLoadout);
        CopyOverlayFlags(ofInFlightCommand, bInFlight && bInCommand);
        CopyOverlayFlags(ofInStationCommand, !bInFlight && bInCommand);
        CopyOverlayFlags(ofInFlight, bInFlight);
        CopyOverlayFlags(ofInCockpit, bInFlight && GetCameraMode() == cmCockpit);
        CopyOverlayFlags(ofInChase, bInFlight && ((GetCameraMode() == cmExternalChase) || ((GetCameraMode() == cmExternalMissile) && (m_pmissileLast == 0))));
        CopyOverlayFlags(ofCommandPane, bInCommand && !(GetOverlayFlags() & c_omBanishablePanes));
        CopyOverlayFlags(ofGloatCam, GetCameraMode() == cmExternalOverride);
    }


    void    TrackCamera(const Vector&       position1,
                        const Vector&       position2,
                        float               length)
    {
        assert ((position2 - position1).LengthSquared() > 0.0f);
        Vector      axis = (position2 - position1).Normalize();
        // GetOrthogonalVector does not return consistent results, which
        // causes an unsightly jump when tracking the camera. I replaced it with
        // some code that is biased toward the camera orientation when building a
        // coordinate system. In an animation, this should choose an initial
        // condition, then use that as a start point for the next frame. That
        // should always give smooth results. BSW 2/2/2000
        //Vector  side = axis.GetOrthogonalVector();
        Orientation orientation = m_cameraControl.GetOrientation();
        Vector      up = orientation.GetUp ();
        float       dot_product = up * axis;
        if (fabsf (dot_product) > 0.95f)
        {
            up = orientation.GetRight ();
            dot_product = up * axis;
            if (fabsf (dot_product) > 0.95f)
                up = orientation.GetRight ();
            assert (up * axis < 0.95f);
        }
        Vector      side = CrossProduct (axis, up);

        m_cameraControl.SetPosition(position1 - axis * (length * 2.0f) + side * (length * 0.25f));

        orientation.TurnTo(axis);
        m_cameraControl.SetOrientation(orientation);
    }

    void ComputeExternalChase (IshipIGC* pShip, Orientation& orientationBfr, Vector& positionBfr, float& offset)
    {
        // get the ship orientation and position
        const Orientation&  shipOrientation = pShip->GetOrientation ();
        const Vector&       shipPosition = pShip->GetPosition();

        // This part is for dealing with the zoomable camera
        float   fZoomDelta = s_fExternalViewDistanceMax - s_fExternalViewDistanceMin;
        offset = (offset - s_fExternalViewDistanceMin) / (fZoomDelta * 0.01);  // maps this into the range 0.0 ... 100.0
        float   fDelayTime = 0.05f + (offset * 0.00333);

        // find the sample index to interpolate
        int     iSampleIndex = ((m_turnRateSampleIndex - 1) + ARRAY_OF_SAMPLES_SIZE) % ARRAY_OF_SAMPLES_SIZE;

		// BT - 8/17 Fixing DX9 hang when in chase mode, and the camera is zoomed all the way out, this would cause a hang withe Dx9 engine. 
		// The experiance is still bad (it's jumping all over the place), but at  least it doesn't hang. The Dx7 engine 
		// works smoothly here, and the ship itself turns a lot nicer under DX7. 
		// I believe that if you can figure out why this is different between the two engines, you will have solved the issue that was 
		// introduced with the Dx9 conversion that is causing some players to report aiming issues. 
		// The issue with the Dx9 version is that the m_turnRateSamples[iSampleIndex].time + fDelayTime) > m_turnRateSamples[m_turnRateSampleIndex].time is always true. 
		// In the Dx7 engine version, this is NOT always true. 
		// TODO: Resolve difference between Dx7 and Dx9 versions of the engines. 
        while ((m_turnRateSamples[iSampleIndex].time + fDelayTime) > m_turnRateSamples[m_turnRateSampleIndex].time && iSampleIndex != 0)
            iSampleIndex = ((iSampleIndex - 1) + ARRAY_OF_SAMPLES_SIZE) % ARRAY_OF_SAMPLES_SIZE;

        // find the amount to interpolate
        float   fDeltaTime = m_turnRateSamples[m_turnRateSampleIndex].time - (m_turnRateSamples[iSampleIndex].time + fDelayTime);
        int     iNextSampleIndex = (iSampleIndex + 1) % ARRAY_OF_SAMPLES_SIZE;
        float   fIntervalSize = m_turnRateSamples[iNextSampleIndex].time - m_turnRateSamples[iSampleIndex].time;
        float   fInterpolant = 1.0f - (fDeltaTime / fIntervalSize);

        // compute the angular displacements
        float   fYawDisplacement = 0.0f;
        float   fPitchDisplacement = 0.0f;
        float   fRollDisplacement = 0.0f;

        float   fPreviousYawRate = Interpolate (m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisYaw], m_turnRateSamples[iSampleIndex].fTurnRate[c_axisYaw], fInterpolant);
        float   fPreviousPitchRate = Interpolate (m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisPitch], m_turnRateSamples[iSampleIndex].fTurnRate[c_axisPitch], fInterpolant);
        float   fPreviousRollRate = Interpolate (m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisRoll], m_turnRateSamples[iSampleIndex].fTurnRate[c_axisRoll], fInterpolant);
        float   fTimeInterval = fInterpolant * fIntervalSize * 0.5f;

        while (iNextSampleIndex != m_turnRateSampleIndex)
        {
            fYawDisplacement += (fPreviousYawRate + m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisYaw]) * fTimeInterval;
            fPreviousYawRate = m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisYaw];

            fPitchDisplacement += (fPreviousPitchRate + m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisPitch]) * fTimeInterval;
            fPreviousPitchRate = m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisPitch];

            fRollDisplacement += (fPreviousRollRate + m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisRoll]) * fTimeInterval;
            fPreviousRollRate = m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisRoll];

            Time    oldTime = m_turnRateSamples[iNextSampleIndex].time;
            iNextSampleIndex = (iNextSampleIndex + 1) % ARRAY_OF_SAMPLES_SIZE;
            fTimeInterval = (m_turnRateSamples[iNextSampleIndex].time - oldTime) * 0.5f;
        }
        fYawDisplacement += (fPreviousYawRate + m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisYaw]) *  fTimeInterval;
        fPitchDisplacement += (fPreviousPitchRate + m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisPitch]) *  fTimeInterval;
        fRollDisplacement += (fPreviousRollRate + m_turnRateSamples[iNextSampleIndex].fTurnRate[c_axisRoll]) *  fTimeInterval;

        // Compute orientation matrix from angular displacements
        orientationBfr = shipOrientation;
        orientationBfr.Roll (-fRollDisplacement);
        orientationBfr.Pitch (fPitchDisplacement); // not negated, because it is already backwards for some reason
        orientationBfr.Yaw (-fYawDisplacement);

        // compute vector scaling quantities
        float   fRadius = pShip->GetHullType()->GetLength();
        float   fMaxVelocity = pShip->GetHullType()->GetMaxSpeed ();
        Vector  velocityVector = pShip->GetVelocity ();
        float   fVelocity = velocityVector.Length ();
        float   fFractionMaxVelocity = fVelocity / fMaxVelocity;
        if (fVelocity > 0.0f)
        {
            // this computes a proper multiplier for the scalar regardless of what
            // direction the ship is travelling with respect to its orientation
            velocityVector /= fVelocity;
            float   fVelocityScalar = velocityVector * shipOrientation.GetForward ();
            fVelocity *= (fVelocityScalar < 0.0f) ? (fVelocityScalar * 0.5f) : fVelocityScalar;
        }
        float   fFractionMaxForwardVelocity = fVelocity / fMaxVelocity;                     // range 0..1

        // compute vector offset quantities
        float   fCurrentVelocityOffset = fRadius * -0.5f * fFractionMaxVelocity;            // half the radius times the fraction of max velocity
        float   fOldForwardVelocityOffset = fRadius * fFractionMaxForwardVelocity;          // range of the radius * (0 .. 1)
        float   fOldUpOffset = fRadius * 0.3333f;                                           // one quarter of the radius
        float   fCurrentOrientationOffset = offset * 0.5f;                                  // range 0 .. 50
        float   fOldOrientationOffset = (offset + 100.0f) * 0.5f;                           // range 50 .. 100

        // compute orientations taking into account the POV (the joystick hat control)
        const Orientation&  headOrientation = m_cameraControl.GetHeadOrientation();
        Orientation         currentLookOrientation = headOrientation * shipOrientation;
        Orientation         oldLookOrientation = headOrientation * orientationBfr;

        // We compute a new location for the camera by combining an offset
        // along the old forward vector, an offset along the new forward
        // vector, an offset along the actual velocity vector, and a bit
        // of up offset so you can see what's on the other side of the ship.
        positionBfr = shipPosition
            + (orientationBfr.GetUp ()                  * fOldUpOffset)
            + (orientationBfr.GetBackward ()            * fOldForwardVelocityOffset)
            + (velocityVector                           * fCurrentVelocityOffset)
            + (currentLookOrientation.GetBackward ()    * fCurrentOrientationOffset)
            + (oldLookOrientation.GetBackward ()        * fOldOrientationOffset)
            ;

        // don't let the camera mode add an additional offset later
        offset = 0.0f;
    }

    void UpdateCamera(float dt)
    {
        if (trekClient.GetCluster())
        {
            bool        bJiggle = false;

            assert (trekClient.GetShip()->GetBaseHullType() || trekClient.GetShip()->GetParentShip());

            IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();

            const Orientation&  shipOrientation = pshipSource->GetOrientation();
            const Orientation*  pMyOrientation;
            //Orientation         orientationBfr;

            float               myRadius = pshipSource->GetRadius();
            const Vector&       myPosition = pshipSource->GetPosition();

            // fill in samples for chase view
            m_turnRateSampleIndex = (m_turnRateSampleIndex + 1) % ARRAY_OF_SAMPLES_SIZE;
            m_turnRateSamples[m_turnRateSampleIndex].fTurnRate[c_axisYaw] = pshipSource->GetCurrentTurnRate (c_axisYaw);
            m_turnRateSamples[m_turnRateSampleIndex].fTurnRate[c_axisPitch] = pshipSource->GetCurrentTurnRate (c_axisPitch);
            m_turnRateSamples[m_turnRateSampleIndex].fTurnRate[c_axisRoll] = pshipSource->GetCurrentTurnRate (c_axisRoll);
            m_turnRateSamples[m_turnRateSampleIndex].time = trekClient.GetCore()->GetLastUpdate ();

            if (InternalCamera(m_cm))
            {
                bJiggle = true;

                const Vector*  poffset;
                if (pshipSource != trekClient.GetShip())
                {
                    assert (trekClient.GetShip()->GetParentShip() != NULL);
                    Mount   turretID = trekClient.GetShip()->GetTurretID();
                    if (turretID != NA)
                    {
                        //Turret orientation always mirrors the orientation of the source ship
                        const Orientation&  clientOrientation = trekClient.GetShip()->GetOrientation();

                        /*
                        if (m_cameraControl.GetAnimation() == 0.0f)
                        {
                            Orientation oTurret = pshipSource->GetHullType()->GetWeaponOrientation(turretID) *
                                                  shipOrientation;
                            m_pmtTurret->SetValue(Matrix(oTurret, Vector::GetZero(), 1.0f));

                            //m_pwrapImageTurret->SetImage(m_pimageTurret);
                            //m_pcameraTurret->SetOrientation(m_cameraControl.GetHeadOrientation() * clientOrientation);
                        }
                        else
                        {
                            //m_pwrapImageTurret->SetImage(Image::GetEmpty());
                        }
                        */

                        /* NYI gun orientation depends on parent ship orientation
                        Orientation oTurret = pshipSource->GetHullType()->GetWeaponOrientation(turretID) *
                                              shipOrientation;

                        orientationBfr = clientOrientation * oTurret;
                        pMyOrientation = &orientationBfr;
                        */
                        pMyOrientation = &clientOrientation;
                        poffset = &(pshipSource->GetHullType()->GetWeaponPosition(turretID));
                    }
                    else
                    {
                        pMyOrientation = &shipOrientation;
                        poffset = &(pshipSource->GetHullType()->GetCockpit());
                    }
                }
                else
                {
                    pMyOrientation = &shipOrientation;

                    poffset = &(pshipSource->GetHullType()->GetCockpit());
                    /*if (m_cameraControl.GetAnimation() == 0.0f)
                    {
                        m_pcameraTurret->SetOrientation(m_cameraControl.GetHeadOrientation());
                    }
                    */
                }

                Vector p = myPosition + *poffset * shipOrientation;

                m_cameraControl.SetPosition(p);      //Do not override the existing animation

                m_cameraControl.SetOrientation(*pMyOrientation);
            }
            else
            {
                //m_pwrapImageTurret->SetImage(Image::GetEmpty());

                if (CommandCamera(m_cm))
                {
                    UpdateCommandView();
                }
                else
                {
                    assert (ExternalCamera(m_cm));

                    Orientation         orientationBfr;
                    Vector              positionBfr;

                    const Orientation*  pOrientation;
                    const Vector*       pPosition;
                    float               offset = m_distanceExternalCamera;

                    switch (m_cm)
                    {
                        case cmExternalTarget:
                        {
                            ImodelIGC*  pmodelTarget = GetCurrentTarget();
                            if (pmodelTarget)
                            {
                                {
                                    float   fMin = 5.0f + 2.0f * pmodelTarget->GetRadius();
                                    if (offset < fMin)
                                        offset = m_distanceExternalCamera = fMin;
                                }
                                pPosition = &(pmodelTarget->GetPosition());
                                pOrientation = &m_orientationExternalCamera;
                                break;
                            }
                        }
                        //No break

                        case cmExternal:
                        {
                            {
                                float   fMin = 5.0f + 2.0f * pshipSource->GetRadius();
                                if (offset < fMin)
                                    offset = m_distanceExternalCamera = fMin;
                            }

                            pPosition = &myPosition;
                            pOrientation = &m_orientationExternalCamera;
                        }
                        break;

                        case cmExternalChase:
                        {
                            ComputeExternalChase (pshipSource, orientationBfr, positionBfr, offset);
                            pOrientation = &orientationBfr;
                            pPosition = &(positionBfr);
                        }
                        break;

                        case cmExternalFlyBy:
                        {
                            //Position the camera so that both the target and the player are in view
                            //Basic algorithm:
                            //Aim the camera at the midpoint, back far enough so both are in view
                            ImodelIGC* pmodelTarget = GetCurrentTarget();
                            if (pmodelTarget != NULL)
                            {
                                const Vector&   targetPosition = pmodelTarget->GetPosition();

                                pPosition = &positionBfr;
                                pOrientation = &m_orientationExternalCamera;

                                positionBfr = (0.5f * (targetPosition + myPosition));

                                const Vector& backward = pOrientation->GetBackward();

                                //Now how far back to we have to move the camera to get both in view
                                float   dTarget;
                                {
                                    Vector  delta = targetPosition - positionBfr;

                                    //Find the distance the target is along the forward vector
                                    float   dBackward = delta * backward;

                                    //Find the distance the target is perpendicular to the forward vector
                                    float   dSide = (float)sqrt(delta.LengthSquared() + dBackward * dBackward);

                                    dTarget = dSide * 3.0f + dBackward;  //5.0 == fudge factor based on field of view
                                }

                                float   dMe;
                                {
                                    Vector  delta = myPosition - positionBfr;
                                    float   dBackward = delta * backward;
                                    float   dSide = (float)sqrt(delta.LengthSquared() + dBackward * dBackward);
                                    dMe = dSide * 3.0f + dBackward;
                                }

                                offset = (1.25f * ((dTarget > dMe) ? dTarget : dMe));
                            }
                            else
                            {
                                //Degenerate case ... treat as external chase
                                pPosition = &myPosition;
                                pOrientation = &m_orientationExternalCamera;
                            }
                        }
                        break;

                        case cmExternalOrbit:
                        {
                            m_orientationExternalCamera.Pitch(dt * 0.35f);
                            m_orientationExternalCamera.Yaw(dt * 0.2f);
                            m_orientationExternalCamera.Roll(dt * 0.5f);

                            pPosition = &myPosition;
                            pOrientation = &m_orientationExternalCamera;
                            offset = 5.0f + 2.5f * pshipSource->GetHullType()->GetLength();
                        }
                        break;

                        case cmExternalOverride:
                        {
                            if (!m_bUseOverridePosition)
                                TrackCamera(myPosition, m_positionOverrideTarget,
                                            pshipSource->GetRadius() * 3.0f);

                            offset = -1.0f;
                        }
                        break;

                        case cmExternalStation:
                        {
                            ImodelIGC*  myTarget = GetCurrentTarget();
                            if ((myTarget != NULL) &&
                                (myTarget != pshipSource) &&
                                (pshipSource->GetCluster() == myTarget->GetCluster()) &&
                                (myTarget->GetHitTest() != NULL) &&
                                pshipSource->CanSee(myTarget))
                            {
                                pPosition = &myPosition;
                                pOrientation = &m_orientationExternalCamera;
                                offset = 5.0f + pshipSource->GetRadius() * 2.5f;
                                TrackCamera (myPosition, myTarget->GetPosition(), offset);
                                offset = -1.0f;
                            }
                            else
                            {
                                // no target, or it wasn't visible...
                                // so use external chase view
                                ComputeExternalChase (pshipSource, orientationBfr, positionBfr, offset);
                                pOrientation = &orientationBfr;
                                pPosition = &(positionBfr);

                                // and should we switch back to cockpit?
                            }
                        }
                        break;

                        case cmExternalMissile:
                        {
                            ImissileIGC*    pmissile = pshipSource->GetLastMissileFired();
                            if (pmissile != m_pmissileLast)
                            {
                                m_cmOld = cmExternalMissile;    //Hack: prevent anything weird on the animated transition to the missile
                                m_cameraControl.SetOrientation (pshipSource->GetOrientation ());
                                m_cameraControl.SetAnimation(1.0f); //Force an animation to the new missile/ship position
                                m_pmissileLast = pmissile;
                                UpdateOverlayFlags ();
                            }
                            if (pmissile)
                            {
                                ImodelIGC*  pTarget = pmissile->GetTarget ();
                                if (pTarget && pTarget->GetMission ())
                                    // if the missile has a valid target, we use the tracking camera
                                    TrackCamera (pmissile->GetPosition (), pTarget->GetPosition(), pmissile->GetRadius () * 3.0f);
                                else
                                    // otherwise track the missile and the ship it was fired from
                                    TrackCamera (pmissile->GetPosition (), pshipSource->GetPosition(), pmissile->GetRadius () * 3.0f);
                                offset = -1.0f;
                            }
                            else
                            {
                                // No missile ... treat as a chase view
                                ComputeExternalChase (pshipSource, orientationBfr, positionBfr, offset);
                                pOrientation = &orientationBfr;
                                pPosition = &(positionBfr);

                                // and check to see if we are out of missiles, just for the heck of it
                                // and switch back to cockpit mode if we are?
                            }
                        }
                        break;

                        default:
                        {
                            //Some unsupprted mode ... treat as external
                            pPosition = &myPosition;
                            pOrientation = &m_orientationExternalCamera;
                        }
                    }

                    if (offset >= 0.0f)
                    {

                        m_cameraControl.SetPosition(offset == 0.0f
                                                    ? *pPosition
                                                    : (*pPosition + offset * pOrientation->GetBackward()));
                        m_cameraControl.SetOrientation(*pOrientation);
                    }
                }
            }

            bool    bAnimating = m_cameraControl.Apply(dt, bJiggle, m_pboolTargetHUD->GetValue());

            m_pCommandVisibleGeo->SetVisible(m_bCommandGrid || CommandCamera(m_cm) ||
                                             (bAnimating && CommandCamera(m_cmOld)));

            {
                bool    v = !(InternalCamera(m_cm) &&
                              ((!bAnimating) || InternalCamera(m_cmOld)));

                pshipSource->SetVisibleF(v);
            }

            m_pmuzzleFlareImage->SetVisible(!bAnimating);
        }
    }

    void AddMuzzleFlare(const Vector& vecDirection, float duration)
    {
        /*  , doesn't work
        if (vecDirection.LengthSquared() != 0) {
            Point pointNDC;

            if (m_pcamera->TransformEyeToNDC(vecDirection, pointNDC)) {
                m_pmuzzleFlareImage->AddFlare(pointNDC, duration);
            }
        }
        */
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    ZString GetFPSString(float fps, float mspf, Context* pcontext)
    {
        ZString strPosition;

        if (g_pzfFrameDump && g_pzfFrameDump->IsValid())
        {
            ZString str = ZString((int)mspf) + "," + ZString((int)fps) + ",";

            int i;
            for (i=0; i<7; i++)
                str += ZString(g_fdInfo.objects_drawn[i]) + ",";

            for (i=0; i<7; i++)
                str += ZString(g_fdInfo.objects_insector[i]) + ",";

            str += ZString(pcontext->GetPerformanceCounter(CounterTriangles)) + ",";
            str += ZString(pcontext->GetPerformanceCounter(CounterDrawStrings)) + ",";
            str += ZString(pcontext->GetPerformanceCounter(CounterDrawStringChars)) + "\n";

            g_pzfFrameDump->Write(str);
        }

        if (trekClient.flyingF()) {
            const Vector& myPosition = trekClient.GetShip()->GetPosition();
            strPosition =
                  " pos("
                + ZString((int)myPosition.X()) + ","
                + ZString((int)myPosition.Y()) + ","
                + ZString((int)myPosition.Z()) + ")";
        } else {
            strPosition = " pos: (Dead)";
        }

        /*
        ZString str;

        str += (m_pwrapGeoDebris->GetGeo()          != Geo::GetEmpty())   ? "D" : "d";
        str += (m_pwrapImageStars->GetImage()       != Image::GetEmpty()) ? "S" : "s";
        str += (m_pwrapImagePosters->GetImage()     != Image::GetEmpty()) ? "N" : "n";
        str += (m_pwrapImageScene->GetImage()       != Image::GetEmpty()) ? "3" : "*";
        str += (m_pwrapImageHudGroup->GetImage()    != Image::GetEmpty()) ? "H" : "h";
        str += m_bShowMeteors                                             ? "M" : "m";
        str += m_bShowStations                                            ? "S" : "s";
        str += m_bShowProjectiles                                         ? "P" : "p";
        str += m_bShowAlephs                                              ? "A" : "a";
        str += m_bShowShips                                               ? "F" : "f";
        str += " ";
        str += ThingGeo::GetShowLights()                                  ? "S" : "s";
        str += (m_pwrapImageLensFlare->GetImage()   != Image::GetEmpty()) ? "F" : "f";
        str += ThingGeo::GetShowTrails()                                  ? "T" : "t";
        str += (m_pwrapImageEnvironment->GetImage() != Image::GetEmpty()) ? "E" : "s";
        */

        int nNumPlayingSoundBuffers;
        ZSucceeded(m_pSoundEngine->GetNumPlayingVirtualBuffers(nNumPlayingSoundBuffers));
        char szRemoteAddress[64];
        if (trekClient.m_fm.IsConnected())
          trekClient.m_fm.GetIPAddress(*trekClient.m_fm.GetServerConnection(), szRemoteAddress);
        else
          lstrcpy(szRemoteAddress, "N/A");

        return
              "TC:" + ZString(ThingGeo::GetTrashCount())
            + " CC:" + ZString(ThingGeo::GetCrashCount())
            + " Server: " + ZString(szRemoteAddress)
            + " fov: " + ((m_pcamera != NULL) ? ZString(DegreesFromRadians(m_pcamera->GetFOV())) : ZString())
            + strPosition
            + " lag: " + ZString((long) trekClient.m_serverLag)
            + " ping: " + ZString(Time::Now() - trekClient.m_timeLastPing)
            + " sync: " + ZString(int(100.0f * trekClient.m_sync))
            + " snds: " + ZString(nNumPlayingSoundBuffers);

        //return m_ptrekInput->GetFPSString();
    }

    void StartTraining (int iMissionIndex)
    {
        if (Training::StartMission (iMissionIndex))
        {
            screen(ScreenIDCombat);

            if (m_cm == cmExternalOverride)
                EndOverrideCamera();

            Training::SetupShipAndCamera ();
            m_cameraControl.SetAnimatePosition(false);
            m_cameraControl.SetAnimation(0.0f);

            // set the ship status
            ShipStatus  status;
            status.SetHullID (static_cast<HullID> (trekClient.GetShip ()->GetHullType ()->GetObjectID ()));
            status.SetSectorID (Training::GetStartSectorID ());
            status.SetState (c_ssFlying);
            status.SetDetected (false);
            trekClient.GetPlayerInfo ()->SetShipStatus (status);
        }
    }

    void OrientCommandView(void)
    {
        assert (CommandCamera(m_cm));

        Orientation o;
        if (m_cm == cmExternalCommandView34)
        {
            static const Vector  forward(0.0f, sqrt2 / 2.0f, -sqrt2 / 2.0f);    //Remember negated in set
            static const Vector  up(0.0f, sqrt2 / 2.0f, sqrt2 / 2.0f);
            o.Set(forward, up);
        }
        else
        {
            assert (m_cm == cmExternalCommandViewTD);

            o.Reset();
        }
        o.PostRoll(m_rollCommandCamera);
        m_cameraControl.SetOrientation(o);
    }

    static void  GetDefaultPosition(Vector*    pposition, float*   pradiusMin)
    {
        IstationIGC*    pstation = trekClient.GetShip()->GetStation();
        if (pstation && pstation->GetCluster() == trekClient.GetCluster())
        {
            *pposition = pstation->GetPosition();
            *pradiusMin = pstation->GetRadius() * 2.0f;
        }
        else
        {
            IshipIGC*   pship = trekClient.GetShip()->GetSourceShip();
            assert (pship);

            *pposition = pship->GetPosition();
            *pradiusMin = 5.0f + pship->GetRadius() * 2.0f;
        }
    }

    void PositionCommandView(const Vector*  pposition, float dt)
    {
        if (CommandCamera(m_cm))
        {
            m_cameraControl.SetHeadOrientation(0.0f);
            m_cameraControl.SetAnimation(dt);

            OrientCommandView();

            m_displacementCommandView = Vector::GetZero();

            if (pposition)
            {
                m_positionCommandView = *pposition;
                m_distanceCommandCamera = s_fCommandViewDistanceMax / 2.0f;
            }
            else
            {
                float   fMin;
                GetDefaultPosition(&m_positionCommandView, &fMin);
                if (m_distanceCommandCamera < fMin)
                    m_distanceCommandCamera = fMin;
            }
        }
    }

	// mdvalley: the func return type has to be explicit now.
    virtual void UpdateCommandView(void)
    {
        assert (CommandCamera(m_cm));

        IclusterIGC*    pcluster = trekClient.GetCluster();
        assert (pcluster);

        if (!m_pconsoleImage->DrawSelectionBox())
        {
            float   fMin = s_fCommandViewDistanceMin;
            Vector  positionTarget;
            ImodelIGC*  pmodelTarget = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);
            if (pmodelTarget != NULL)
            {
                if (m_bTrackCommandView)
                {
                    if (pmodelTarget->GetObjectType() == OT_ship)
                        pmodelTarget = ((IshipIGC*)pmodelTarget)->GetSourceShip();

                    if ((pmodelTarget->GetCluster() == pcluster) &&
                        trekClient.GetShip()->CanSee(pmodelTarget))
                    {
                        positionTarget = pmodelTarget->GetPosition();
                        fMin = 5.0f + pmodelTarget->GetRadius() * 2.0f;

                        if (m_distanceCommandCamera < fMin)
                            m_distanceCommandCamera = fMin;

                        m_positionCommandView = positionTarget +
                                                m_displacementCommandView;
                    }
                }
            }
            /*
            else
            {
                GetDefaultPosition(&positionTarget, &fMin);
                if (m_distanceCommandCamera < fMin)
                    m_distanceCommandCamera = fMin;

                m_positionCommandView = positionTarget +
                                        m_displacementCommandView;
            }
            */

            {
                //No matter what ... never allow displacement over twice the normal max
                float   l2 = m_positionCommandView.LengthSquared();
                if (l2 > 4.0f * s_fCommandViewDisplacementMax * s_fCommandViewDisplacementMax)
                    m_positionCommandView *= 2.0f * s_fCommandViewDisplacementMax / float(sqrt(l2));
            }
        }

        const Orientation&  myOrientation = m_cameraControl.GetOrientation();
        m_cameraControl.SetPosition(m_positionCommandView + (myOrientation.GetBackward() * m_distanceCommandCamera));
    }

    virtual void ChangeChatMessage(void)
    {
    }

    /*
    void OnLoadoutChanged(IpartIGC* ppart, LoadoutChange lc)
    {
        if (lc == c_lcHullChange)
        {
            IshipIGC*     pship     = trekClient.GetShip()->GetSourceShip();
            if (pship->GetCluster() != NULL)
            {
                // Change in flight ... update the display mdl
                // NYI assume we'll only get this when being placed in a life pod

                if (m_pconsoleImage)
                    m_pconsoleImage->SetDisplayMDL(pship->GetHullType()->GetPilotHUDName());
            }
        }
        else if (lc == c_lcTurretChange)
        {
            //player switched positions
            IhullTypeIGC*   pht = trekClient.GetShip()->GetSourceShip()->GetBaseHullType();

            const char* pszDisplayMDL;
            Mount   turretID = trekClient.GetShip()->GetTurretID();
            if (turretID == NA)
            {
                pszDisplayMDL = pht->GetObserverHUDName();
            }
            else
            {
                pszDisplayMDL = pht->GetHardpointData(turretID).hudName;
            }
            m_pconsoleImage->SetDisplayMDL(pszDisplayMDL);
        }
    }
    */

    void OnPaint(HDC hdc, const WinRect& rect)
    {
    }

    float   GetDeltaTime (void) const
    {
        return m_fDeltaTime;
    }

    void UpdateMouseEnabled() {
        bool bEnable =
            m_bEnableVirtualJoystick
            && m_bActive
            && GetPopupContainer()->IsEmpty()
            && trekClient.flyingF()
            && ((m_viewmode == vmCombat) || (m_viewmode == vmOverride))
            && ((m_voverlaymask[m_viewmode] & (c_omBanishablePanes & ~ofInvestment)) == 0)
            && (!m_bFreshInvestmentPane || (m_voverlaymask[m_viewmode] & ofInvestment) == 0);

        //enabling mouse means that we listen to the mouse manually and ignore window events
        m_pmouse->SetEnabled((bEnable || m_pengine->IsFullscreen()));
        m_pjoystickImage->SetEnabled(bEnable, bEnable);
        SetMoveOnHide(!bEnable);
        ShowCursor(!bEnable);
    }

    void EvaluateFrame(Time time)
    {
        static bool bFirstFrame = true;

        if (bFirstFrame)
        {
            m_ptrekInput->SetInputSite(this); //Imago 8/15/09

            // allow the splash screen to draw itself before we do
            // any other initialization
            bFirstFrame = false;
            if (m_screen != ScreenIDCombat)
                return;
        }

        float dtime = m_fDeltaTime = (float)(time - m_timeLastFrame);
        ZAssert(dtime >= 0);

        //
        // Turn on the virtual joystick if the right conditions are met
        //

        UpdateMouseEnabled();

        //
        // Give the current screen a chance to do something on a per frame basis
        //

        if (m_pscreen) {
            m_pscreen->OnFrame();
        }
        CheckCountdownSound();

        // Handle AutoDownload as needed, if we're not downloading, do nothing
        if (trekClient.m_pAutoDownload)
        {
            if(GetWindow()->GetFullscreen())
                trekClient.HandleAutoDownload(50); // give smaller time slice to allow for mouse to update
            else
                trekClient.HandleAutoDownload(500); // since the mouse is hardware in not full screen, the graphics engine doesn't need much CPU
        }


        // receive network messages
        trekClient.m_lastUpdate  = m_timeLastFrame;
        trekClient.m_now         = time;
        if (FAILED(trekClient.ReceiveMessages())) {
			debugf("*!* frame# %d dropped (net)\n",m_frameID);
			return;     //bug out
		}
            
            // (CRC) Too much spew: ShouldBe ((!trekClient.m_serverOffsetValidF) || (fabs(Time::Now() - trekClient.m_timeLastPing) < 30.0f));

        // Update the world
		DoTrekUpdate(m_timeLastFrame, time, dtime, (m_frameID > 1));

        // Update sounds
        m_pSoundEngine->Update();
        trekClient.UpdateAmbientSounds(DWORD(dtime * 1000));

        // Update the HUD Graphics
        if ((m_cm == cmExternalOverride) && (m_timeOverrideStop < time))
        {
            // Back to a normal camera mode

            EndOverrideCamera();
        }
        if (m_pconsoleImage != NULL && m_bOverlaysChanged)
        {
            UpdateOverlayFlags();
            m_bOverlaysChanged = false;
        }

        // if this is not a training mission, the commands time out
        if (m_timeRejectQueuedCommand < time)
            RejectQueuedCommand(false);

        IclusterIGC*    pcluster = trekClient.GetCluster();

        if (pcluster && m_screen == ScreenIDCombat)
        {
            assert (m_screen == ScreenIDCombat);
            m_frameID++;

            Rect rectImage = m_pviewport->GetViewRect()->GetValue();

            //
            // Move the camera
            //

            UpdateCamera(dtime);

            //
            // Update the screen positions and thrust values for all of the ships
            // (and, while we are here, see if we can see it)
            //

            const ModelListIGC* models = pcluster->GetPickableModels();
            assert (models);

            double   angleToPixels = double(rectImage.Size().X()) / double(m_cameraControl.GetFOV());

            {
                Point pointCenter = rectImage.Center();
                rectImage.Offset(-pointCenter);

                rectImage.Expand(-c_flBorderSide);
            }
            for (int i=0; i<7; i++)
            {
                g_fdInfo.objects_drawn[i] = 0;
                g_fdInfo.objects_insector[i] = 0;
            }

            const Vector&   positionCamera = m_cameraControl.GetPosition();

            IshipIGC*   pshipParent = trekClient.GetShip()->GetParentShip();
            IshipIGC*   pshipSource = pshipParent ? pshipParent : trekClient.GetShip();

            // store the screen size for later use
            #if 1
            float           fScreenSize = (rectImage.YMax() - rectImage.YMin()) * 0.48f;    //NYI same constant as in radarimage.cpp
            #else
            float           fScreenXSize = rectImage.XMax() - rectImage.XMin(),
                            fScreenYSize = rectImage.YMax() - rectImage.YMin(),
                            fScreenSize = sqrtf ((fScreenXSize * fScreenXSize) + (fScreenYSize * fScreenYSize)) * 0.5f;
            #endif

            ImodelIGC*  pmodelTarget = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);

            //
            // Update the target geo
            //
            {
                SetTargetGeo(pmodelTarget);
                assert (m_pconsoleImage);
                m_pconsoleImage->GetConsoleData()->SetTarget(pmodelTarget);
            }

            IsideIGC*   psideMe;
            bool        bAnyEnemyShips;
            if (pmodelTarget)
                bAnyEnemyShips = true;
            else
            {
                bAnyEnemyShips = false;
                psideMe = trekClient.GetSide();
            }

            float   zMin;
            float   zMax;

            const Vector*   pcameraPosition;
            const Vector*   pcameraBackward;
            if (CommandCamera(m_cm))
            {
                pcameraPosition = &(m_cameraControl.GetPosition());
                pcameraBackward = &(m_cameraControl.GetOrientation().GetBackward());

                //Set the initial zMin & Max to whatever is needed to contain the command grid
                //the grid is a disk of radius r in the XY plane
                //Phi is the angle between the z-axis and the camera vector
                double  sinPhi = pcameraBackward->z;         //Convenient
                double  cosTheta2 = 1.0 - sinPhi * sinPhi;
                float r = cosTheta2 > 0.0f ? float(c_fCommandGridRadius * sqrt(cosTheta2)) : 0.0f;

                float   d = (*pcameraPosition * *pcameraBackward);
                zMin = d-r;
                zMax = d+r;
            }

            for (ModelLinkIGC*  l = models->first(); (l != NULL); l = l->next())
            {
                ImodelIGC*        pmodel = l->data();

                // check to see if this is a ship
                if (pmodel->GetObjectType () == OT_ship)
                {
                    // it is a ship, let's check to see if it is approaching an aleph
                    IshipIGC* pship; CastTo(pship, pmodel);
                    ImodelIGC*  pmodelAleph = FindTarget (pship, c_ttWarp | c_ttNearest | c_ttAllSides);
                    if (pmodelAleph)
                    {
                        Vector  vecDelta = pmodelAleph->GetPosition() - pship->GetPosition();
                        float   fRange = vecDelta.Length ();
                        float   fRelativeVelocity = pship->GetVelocity () * (vecDelta / fRange);
                        if (fRelativeVelocity > 0.0f)
                            pship->GetThingSite ()->SetTimeUntilAleph (fRange / fRelativeVelocity);
                        else
                            pship->GetThingSite ()->SetTimeUntilAleph (-1.0f);
                    }
                }

                // do all other stuff
                if (((pmodel != trekClient.GetShip()) &&
                     (pmodel != pshipParent)) || pmodel->GetVisibleF())
                {
                    ObjectType        type   = pmodel->GetObjectType();
                    if ((type != OT_ship) || (((IshipIGC*)pmodel)->GetParentShip() == NULL))
                    {
                        ThingSitePrivate* psite; CastTo(psite, pmodel->GetThingSite());

                        unsigned char     renderMask = c_ucRenderAll;

                        //
                        // Update screen positions for ships
                        //
                        {
                            float           fScreenRadius;
                            float           distanceToEdge;

                            Point           pointScreenPosition;
                            unsigned char   ucRadarState;

                            const Vector&   positionModel = pmodel->GetPosition();
                            float           radius = pmodel->GetRadius();

                            if (CommandCamera(m_cm))
                            {
                                float   d = (*pcameraPosition - positionModel) * *pcameraBackward;

                                float   z1 = d - radius;
                                if (z1 < zMin)
                                    zMin = z1;
                                float   z2 = d + radius;
                                if (z2 > zMax)
                                    zMax = z2;
                            }

                            {
                                if (m_pcamera->TransformLocalToImage(positionModel, pointScreenPosition))
                                {
                                    pointScreenPosition = rectImage.TransformNDCToImage(pointScreenPosition);

                                    //
                                    // Blip is in front of ship
                                    //
                                    if (m_bRoundRadar)
                                    {
                                        // lock the blip to a circle inscribed in the view rectangle
                                        float   fDistanceSquared = (pointScreenPosition.X () * pointScreenPosition.X ()) + (pointScreenPosition.Y () * pointScreenPosition.Y ());
                                        if (fDistanceSquared >= (fScreenSize * fScreenSize))
                                        {
                                            ucRadarState = c_ucRadarOffScreen;
                                        }
                                        else
                                        {
                                            ucRadarState = c_ucRadarOnScreenLarge;

                                            //Calculate this here, since we have it in a convenient form
                                            distanceToEdge = fScreenSize - sqrtf(fDistanceSquared);
                                        }
                                    }
                                    else
                                    {
                                        // lock the blip to the screen rectangle
                                        ucRadarState = rectImage.Inside(pointScreenPosition) ? c_ucRadarOnScreenLarge : c_ucRadarOffScreen;
                                    }
                                }
                                else
                                    ucRadarState = c_ucRadarOffScreen;
                            }

                            if (ucRadarState == c_ucRadarOffScreen)
                            {
                                fScreenRadius = CmdViewMaxIconScreenSize;
                                distanceToEdge = 0.0f;

                                // Direction from eye to object

                                Vector vecDirection = m_pcamera->TransformLocalToEye(positionModel);
                                if (m_bRoundRadar)
                                {
                                    //No gutter for now ... icons are pinned to the circle
                                    float fLength = sqrtf(vecDirection.X() * vecDirection.X() +
                                                          vecDirection.Y() * vecDirection.Y());

                                    float   f = fScreenSize / fLength;
                                    pointScreenPosition.SetX(vecDirection.X() * f);
                                    pointScreenPosition.SetY(vecDirection.Y() * f);
                                }
                                else
                                {
                                    //
                                    // Get the components
                                    //

                                    float x = vecDirection.X();
                                    float y = vecDirection.Y();

                                    if (x == 0.0f)
                                    {
                                        y = (y <= 0.0f) ? rectImage.YMin() : rectImage.YMax();
                                    }
                                    else if (y == 0.0f)
                                    {
                                        x = (x <= 0.0f) ? rectImage.XMin() : rectImage.XMax();
                                    }
                                    else
                                    {
                                        float   tX = ((x > 0.0f) ? rectImage.XMax() : rectImage.XMin()) / x;
                                        float   tY = ((y > 0.0f) ? rectImage.YMax() : rectImage.YMin()) / y;
                                        float   tMin = (tX < tY) ? tX : tY;

                                        x *= tMin;
                                        y *= tMin;
                                    }

                                    //No gutter for now ... icons are pinned to the edge of the screen
                                    pointScreenPosition.SetX(x);
                                    pointScreenPosition.SetY(y);
                                }
                            }
                            else
                            {
                                double  sinAlpha = radius / (positionModel - positionCamera).Length();
                                fScreenRadius = (sinAlpha < 1.0)
                                                ? float(angleToPixels * asin(sinAlpha))      //hack ... 480/(50*pi/180) * 2asin(r/d)
                                                : 640.0f;
                                if (fScreenRadius < CmdViewMaxIconScreenSize)
                                {
                                    if ((fScreenRadius < AnyViewMinRenderScreenSize) ||
                                         CommandCamera(m_cm))
                                    {
                                        ucRadarState = c_ucRadarOnScreenSmall;
                                        renderMask = c_ucRenderTrail;
                                    }

                                    fScreenRadius = CmdViewMaxIconScreenSize;
                                }

                                //distanceToEdge is calculated above for round radars
                                if (!m_bRoundRadar)
                                {
                                    distanceToEdge = (pointScreenPosition.X() >= 0.0f)
                                                     ? (rectImage.XMax() - pointScreenPosition.X())
                                                     : (pointScreenPosition.X() - rectImage.XMin());
                                    {
                                        float   d = (pointScreenPosition.Y() >= 0.0f)
                                                    ? (rectImage.YMax() - pointScreenPosition.Y())
                                                    : (pointScreenPosition.Y() - rectImage.YMin());
                                        if (d < distanceToEdge)
                                            distanceToEdge = d;
                                    }
                                }
                                assert (distanceToEdge >= 0.0f);
                            }
                            g_fdInfo.objects_insector[type]++;

                            psite->UpdateScreenPosition(
                                pointScreenPosition,
                                fScreenRadius,
                                distanceToEdge,
                                ucRadarState
                            );
                        }

                        bool    visibleF = pshipSource->CanSee(pmodel);
                        bool    bSetVisible;
                        if (type == OT_ship)
                        {
                            IshipIGC* pship; CastTo(pship, pmodel);

                            int         stateM = pship->GetStateM();
                            if (stateM & miningMaskIGC)
                            {
                                ImodelIGC*  pmodel = pship->GetCommandTarget(c_cmdAccepted);
                                if (pmodel && (pmodel->GetObjectType() == OT_asteroid))
                                {
                                    float   minedOre = dtime * pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMiningRate);

                                    ((IasteroidIGC*)pmodel)->MineOre(minedOre);
                                }
                            }

                            if (!bAnyEnemyShips)
                            {
                                //We haven't spotted any enemy ships yet ... see if this is one.
                                bAnyEnemyShips = ( (pship->GetSide() != psideMe) && !psideMe->AlliedSides(psideMe,pship->GetSide()) ); //#ALLY -imago 7/3/09
                            }

                            bSetVisible = (pmodel != trekClient.GetShip()) && (pmodel != pshipParent);

                            TRef<ThingGeo> pthing = psite->GetThingGeo();

                            if (visibleF && pthing)
                            {
                                IafterburnerIGC*    afterburner = (IafterburnerIGC*)(pship->GetMountedPart(ET_Afterburner, 0));

                                if (afterburner)
                                    pthing->SetAfterburnerThrust(pship->GetOrientation().GetBackward(),
                                                                 afterburner->GetPower());
                                else
                                    pthing->SetAfterburnerThrust(Vector::GetZero(),
                                                                 0.0f);

                                pthing->SetThrust(0.5f * (pship->GetControls().jsValues[3] + 1));

								// mdvalley: Uneyed miners no bolties.
								if ((stateM & miningMaskIGC) && trekClient.GetShip()->CanSee(pship))
                                    psite->ActivateBolt();
                                else
                                    psite->DeactivateBolt();
                            }
                            else
                                pthing->SetAfterburnerThrust(Vector::GetZero(), 0.0f);
                        }
                        else
                            bSetVisible = true;

                        //Is the model visible?
                        if (bSetVisible)
                        {
                            switch (type)
                            {
                                case OT_asteroid:
                                    visibleF &=  m_bShowMeteors;
                                break;

                                case OT_station:
                                    visibleF &= m_bShowStations;
                                break;

                                case OT_projectile:
                                    visibleF &= m_bShowProjectiles;
                                break;

                                case OT_warp:
                                    visibleF &= m_bShowAlephs;
                                break;

                                case OT_ship:
                                    visibleF &= m_bShowShips;
                            }

                            pmodel->SetVisibleF(visibleF);
                        }
                        pmodel->SetRender(renderMask);
                    }
                }
                else
                {
                    //Player's ship in a first person view
                    IshipIGC* pship; CastTo(pship, pmodel);
                    ThingSitePrivate* psite; CastTo(psite, pmodel->GetThingSite());
                    TRef<ThingGeo> pthing = psite->GetThingGeo();

                    if (pthing)
                    {
                        IafterburnerIGC*    afterburner = (IafterburnerIGC*)(pship->GetMountedPart(ET_Afterburner, 0));

                        if (afterburner)
                            pthing->SetAfterburnerThrust(pship->GetOrientation().GetBackward(),
                                                         afterburner->GetPower());
                        else
                            pthing->SetAfterburnerThrust(Vector::GetZero(),
                                                         0.0f);
                    }
                }
            }

            if (CommandCamera(m_cm))
                m_cameraControl.SetZClip(zMin < 5.0f ? 5.0f : zMin,
                                         zMax);
            else
                m_cameraControl.SetZClip(5.0f, 10000.0f);

            if ((pmodelTarget == NULL) && bAnyEnemyShips)
            {
                //We have no target and there are enemy ships ... select an appropriate ship
                const Vector&   position = pshipSource->GetPosition();

                ImodelIGC*  m = FindTarget(pshipSource,
                                           (c_ttShip | c_ttEnemy | c_ttNearest),
                                           NULL,
                                           pcluster,
                                           &position, NULL, c_habmRescue);

                if (m == NULL)
                {
                    m = FindTarget(pshipSource,
                                   (c_ttShip | c_ttEnemy | c_ttNearest),
                                   NULL,
                                   pcluster,
                                   &position, NULL, c_habmMiner | c_habmBuilder);

                    if (m == NULL)
                        m = FindTarget(pshipSource,
                                       (c_ttShip | c_ttEnemy | c_ttNearest),
                                       NULL,
                                       pcluster,
                                       &position);
                }

                if (m)
                    SetTarget(m, c_cidDefault);
            }

            m_pnumberFlash->SetValue(GetFlash(time) ? 1.0f : 0.0f);
        }

        if (m_pconsoleImage)
            m_pconsoleImage->Update(time);

        m_timeLastFrame = time;

        //
        // Center the hangar bitmap if we are in the hangar
        //

        UpdateBackdropCentering();

        //
        // Update the countdown timer
        //

        UpdateGameStateContainer();
        if (m_pgsc)
        {
            const MissionParams*    pmp = trekClient.m_pCoreIGC->GetMissionParams();
            float                   dt = time - pmp->timeStart;

            if (pmp->IsCountdownGame())
            {
                //Countdown timer
                m_pgsc->SetTimeRemaining(int(pmp->GetCountDownTime() - dt));
            }
            else
            {
                //Countup timer
                m_pgsc->SetTimeElapsed(int(dt));
            }
        }
    }

    void OnEnterMission (void)
    {
        if (!(Training::IsTraining () || (Training::GetTrainingMissionID () == Training::c_TM_7_Live)))
        {
            Training::StartMission (0);

            SetCluster(trekClient.GetShip()->GetCluster());
            screen(ScreenIDCombat);

            IstationIGC*    pstation = trekClient.GetShip()->GetStation();
            if (pstation)
                trekClient.ReplaceLoadout(pstation);
        }
    }

    void PlayFFEffect(ForceEffectID effectID, LONG lDirection = 0)
    {
        if (m_bEnableFeedback) {
            m_ptrekInput->PlayFFEffect(effectID, lDirection);
        }
    }

    void DoTrekUpdate(Time  lastUpdate,
                      Time  now,
                      float dt,
                      bool  activeControlsF)
	{
		// BT - STEAM
		SteamAPI_RunCallbacks();

		//Spunky #76 - only update throttle if enough time elapsed
		static const float delay[] = {0.066f, 0.033f, 0.022f, 0.016f, 0.0f}; //#282
		static float TimeSinceThrottleUpdate;
		static bool bUpdateThrottle = true;

		// - Imago: Only set AFK from inactivity when logged on
		if (trekClient.m_fLoggedOn) {
			Time timeLastMouseMove;
			timeLastMouseMove = GetMouseActivity();
			if (g_bActivity || timeLastMouseMove.clock() >= m_timeLastActivity.clock()) {
				m_timeLastActivity = now;
				g_bActivity = false;
				if (!g_bAFKToggled && trekClient.GetPlayerInfo() && !trekClient.GetPlayerInfo ()->IsReady()) {
					trekClient.GetPlayerInfo ()->SetReady(true);
					trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
					BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, PLAYER_READY)
					END_PFM_CREATE
					pfmReady->fReady = true;
					pfmReady->shipID = trekClient.GetShipID();
					//trekClient.SendChat(trekClient.GetShip(), CHAT_EVERYONE, NA, NA, "I'm back from being AFK!");
				}
			} else {
				int inactive_threshold; // mmf added this so those in NOAT go afk quicker
				if (trekClient.GetSideID() == SIDE_TEAMLOBBY) inactive_threshold = 90000;
				else inactive_threshold = 180000;
				if (now.clock() - m_timeLastActivity.clock() > inactive_threshold) {
					if (!g_bAFKToggled && trekClient.GetPlayerInfo() && trekClient.GetPlayerInfo ()->IsReady()) {
						trekClient.GetPlayerInfo ()->SetReady(false);
						trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
						BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, PLAYER_READY)
						END_PFM_CREATE
						pfmReady->fReady = false;
						pfmReady->shipID = trekClient.GetShipID();
						//trekClient.SendChat(trekClient.GetShip(), CHAT_EVERYONE, NA, NA, "I've been AFK for 3 minutes!");
					}
				}
			}
		}

        if (trekClient.GetCluster() && GetWindow()->screen() == ScreenIDCombat)
		{
            //For now, leave joystick specific code here.
            //Only process joystick if we have focus
            if (GetFocus())
            {
                bool bAllowKeyboardMovement = !m_pconsoleImage->IsComposing() && GetPopupContainer()->IsEmpty();
                bool bNoCameraControl = NoCameraControl(m_cm);

                if (trekClient.flyingF())
                {
                    //
                    // Handle any joystick button commands
                    //

                    m_ptrekInput->GetButtonTrekKeys(this);

                    //Process joystick input for the player
                    //Carry over persistent bits
                    int buttonsM = trekClient.GetShip()->GetStateM() & (coastButtonIGC | cloakActiveIGC);

                    //Controls default to what the ship has now
                    JoystickResults js;
                    js.controls.jsValues[c_axisYaw] = js.controls.jsValues[c_axisPitch] = js.controls.jsValues[c_axisRoll] = 0.0f;
                    js.controls.jsValues[c_axisThrottle] = trekClient.GetShip()->GetControls().jsValues[c_axisThrottle];

                    bool fAutoPilot = trekClient.autoPilot();
                    if (!GetUI() && activeControlsF)
                    {
                        //Spunky #76 #282
						if (!bUpdateThrottle)
							if ((TimeSinceThrottleUpdate += dt) > delay[m_iWheelDelay])
							{
								bUpdateThrottle = true;
								TimeSinceThrottleUpdate = 0;
							}

						bool    bControlsInUse = SenseJoystick(&js, bNoCameraControl, bAllowKeyboardMovement, bUpdateThrottle);
                        int     oldButtonsM = buttonsM;

                        if (!CommandCamera(m_cm)) {
                            if (m_ptrekInput->IsTrekKeyDown(TK_ThrustLeft, bAllowKeyboardMovement))
                                buttonsM |= leftButtonIGC;
                            if (m_ptrekInput->IsTrekKeyDown(TK_ThrustRight, bAllowKeyboardMovement))
                                buttonsM |= rightButtonIGC;
                            if (m_ptrekInput->IsTrekKeyDown(TK_ThrustUp, bAllowKeyboardMovement))
                                buttonsM |= upButtonIGC;
                            if (m_ptrekInput->IsTrekKeyDown(TK_ThrustDown, bAllowKeyboardMovement))
                                buttonsM |= downButtonIGC;
                        }
                        if (m_ptrekInput->IsTrekKeyDown(TK_ThrustForward, bAllowKeyboardMovement))
                            buttonsM |= forwardButtonIGC;
                        if (m_ptrekInput->IsTrekKeyDown(TK_ThrustBackward, bAllowKeyboardMovement))
                            buttonsM |= backwardButtonIGC;
                        if (m_ptrekInput->IsTrekKeyDown(TK_FireBooster, bAllowKeyboardMovement))
                            buttonsM |= afterburnerButtonIGC;

                        if (fAutoPilot)
                        {
                            if (oldButtonsM != buttonsM)
                                bControlsInUse = true;
                            else
                            {
                                bControlsInUse = bControlsInUse ||
                                                 js.button1 || js.button2 || js.button3 || js.button4 || js.button5 || js.button6;
                            }

                            if (trekClient.bInitTrekJoyStick)
                            {
                                trekClient.trekJoyStick[0] = js.controls.jsValues[0];
                                trekClient.trekJoyStick[1] = js.controls.jsValues[1];
                                trekClient.trekJoyStick[2] = js.controls.jsValues[2];
                                trekClient.bInitTrekJoyStick = false;
                            }
                            else
                            {
                                const float c_fAutopilotDisengage = g_fJoystickDeadZone * 2.0f;
                                bControlsInUse = bControlsInUse ||
                                                 (js.controls.jsValues[c_axisYaw] - trekClient.trekJoyStick[c_axisYaw] < -c_fAutopilotDisengage) ||
                                                 (js.controls.jsValues[c_axisYaw] - trekClient.trekJoyStick[c_axisYaw] >  c_fAutopilotDisengage) ||
                                                 (js.controls.jsValues[c_axisPitch] - trekClient.trekJoyStick[c_axisPitch] < -c_fAutopilotDisengage) ||
                                                 (js.controls.jsValues[c_axisPitch] - trekClient.trekJoyStick[c_axisPitch] >  c_fAutopilotDisengage) ||
                                                 (js.controls.jsValues[c_axisRoll] - trekClient.trekJoyStick[c_axisRoll] < -c_fAutopilotDisengage) ||
                                                 (js.controls.jsValues[c_axisRoll] - trekClient.trekJoyStick[c_axisRoll] >  c_fAutopilotDisengage);
                            }

                            if (bControlsInUse)
                            {
                                /*
                                if (now >= m_timeAutopilotWarning)
                                {
                                    m_timeAutopilotWarning = now + 5.0f;
                                    trekClient.PostText(true, "Hit P to disengage the autopilot");
                                }
                                */

                                trekClient.SetAutoPilot(false);
                                SwitchToJoyThrottle();
                                fAutoPilot = false;
                                trekClient.PlaySoundEffect(salAutopilotDisengageSound);
								g_bActivity = true; // Imago: Joystick movment while Autopiloting = active!
                            }
                        } else //Imago: Joystick movment while not Autopiloting = active!
                        {
							if (oldButtonsM != buttonsM)
								bControlsInUse = true;
							else
							{
								bControlsInUse = bControlsInUse ||
												 js.button1 || js.button2 || js.button3 || js.button4 || js.button5 || js.button6;
							}
							bControlsInUse = bControlsInUse ||
								(js.controls.jsValues[c_axisYaw] - trekClient.trekJoyStick[c_axisYaw] < -g_fJoystickDeadZone) ||
								(js.controls.jsValues[c_axisYaw] - trekClient.trekJoyStick[c_axisYaw] >  g_fJoystickDeadZone) ||
								(js.controls.jsValues[c_axisPitch] - trekClient.trekJoyStick[c_axisPitch] < -g_fJoystickDeadZone) ||
								(js.controls.jsValues[c_axisPitch] - trekClient.trekJoyStick[c_axisPitch] >  g_fJoystickDeadZone) ||
								(js.controls.jsValues[c_axisRoll] - trekClient.trekJoyStick[c_axisRoll] < -g_fJoystickDeadZone) ||
								(js.controls.jsValues[c_axisRoll] - trekClient.trekJoyStick[c_axisRoll] >  g_fJoystickDeadZone);

							if (bControlsInUse) g_bActivity = true;
						}
                    }
                    else
                    {
                        js.controls.Reset();
                        js.hat = 0;
                        js.button1 = js.button2 = js.button3 = js.button4 = js.button5 = js.button6 = 0x00;
                    }

                    IshipIGC*   pshipParent = trekClient.GetShip()->GetParentShip();
                    if (fAutoPilot && (pshipParent == NULL))
                    {
                        js.controls = trekClient.GetShip()->GetControls();
                        // use the controls decided by autoPilot.Update()
                        buttonsM = trekClient.GetShip()->GetStateM();

                        // we want to poll the joystick, and ignore everything but the hat.
                    }
                    else // autoPilot doesn't need these
                    {
                        // Pay attention to the joystick buttons.
                        buttonsM |= (trekClient.m_selectedWeapon << selectedWeaponShiftIGC);
                        if (js.button1 & buttonDown)
                            buttonsM |= trekClient.fGroupFire
                                        ? allWeaponsIGC
                                        : oneWeaponIGC;

                        if (js.button2 & buttonDown)
                            buttonsM |= missileFireIGC;

                        if (js.button3 & buttonDown)
                            buttonsM |= afterburnerButtonIGC;
                    }

                    if (m_ptrekInput->IsTrekKeyDown(TK_DropMine, bAllowKeyboardMovement))
                        buttonsM |= mineFireIGC;
                    else
                        buttonsM &= ~mineFireIGC;

                    if (m_ptrekInput->IsTrekKeyDown(TK_DropChaff, bAllowKeyboardMovement))
                        buttonsM |= chaffFireIGC;
                    else
                        buttonsM &= ~chaffFireIGC;

                    // training mission hook
                    Training::ApproveControls (js.controls);
                    buttonsM = Training::ApproveActions (buttonsM);
                    // end training mission hook

                    if (m_cm == cmCockpit)
					{
                        if (trekClient.GetShip()->GetTurretID() != NA)
                        {
                            float   fov = (s_fMinFOV + 0.5f * (s_fMaxFOV - s_fMinFOV)) +
                                          (0.5f * (s_fMaxFOV - s_fMinFOV)) * js.controls.jsValues[c_axisThrottle];
                            m_cameraControl.SetFOV(fov);
                        }
                        else
						{
                            float   fov = m_cameraControl.GetFOV();
                            if (m_ptrekInput->IsTrekKeyDown(TK_ZoomIn, true))
                            {
                                fov -= dt;
                                if (fov < s_fMinFOV)
                                    fov = s_fMinFOV;

                                m_cameraControl.SetFOV(fov);
                            }
                            else if (m_ptrekInput->IsTrekKeyDown(TK_ZoomOut, true))
                            {
                                fov += dt;
                                if (fov > s_fMaxFOV)
                                    fov = s_fMaxFOV;

                                m_cameraControl.SetFOV(fov);
                            }

							// BT 3/13/2016 - Wasp's slew rate fix. 
							if (trekClient.GetShip()->GetBaseHullType() != NULL)
							{
								//What is the maximum desired rate of turn for this field of view?
								//Use the same calculation as for turrets.
								//Keep in sync with wintrek.cpp's FOV by throttle
								static const float  c_minRate = RadiansFromDegrees(7.5f);
								static const float  c_maxRate = RadiansFromDegrees(75.0f);
								float   maxSlewRate = c_minRate +
									(c_maxRate - c_minRate) * fov / s_fMaxFOV;

								const IhullTypeIGC* pht = trekClient.GetShip()->GetHullType();
								{
									float               pitch = pht->GetMaxTurnRate(c_axisPitch);

									if (pitch > maxSlewRate)
										js.controls.jsValues[c_axisPitch] *= maxSlewRate / pitch;
								}
								{
									float               yaw = pht->GetMaxTurnRate(c_axisYaw);

									if (yaw > maxSlewRate)
										js.controls.jsValues[c_axisYaw] *= maxSlewRate / yaw;
								}
							}
                        }
                    }


                    trekClient.GetShip()->SetControls(js.controls);
                    trekClient.GetShip()->SetStateBits(buttonsMaskIGC | weaponsMaskIGC | selectedWeaponMaskIGC |
                                                       missileFireIGC | mineFireIGC | chaffFireIGC, buttonsM);

					if ((m_cm == cmCockpit) || (m_cm == cmExternalChase)) { 
						if (!m_ptrekInput->IsTrekKeyDown(TK_ViewRearLeft,bAllowKeyboardMovement) //TheRock 31-7-2009 Allow keyboard users to look around
							&& !m_ptrekInput->IsTrekKeyDown(TK_ViewRearRight,bAllowKeyboardMovement) 
							&& !m_ptrekInput->IsTrekKeyDown(TK_ViewFrontLeft,bAllowKeyboardMovement)
							&& !m_ptrekInput->IsTrekKeyDown(TK_ViewFrontRight,bAllowKeyboardMovement)
							&& !m_ptrekInput->IsTrekKeyDown(TK_ViewRear,bAllowKeyboardMovement)
							&& !m_ptrekInput->IsTrekKeyDown(TK_ViewLeft,bAllowKeyboardMovement)
							&& !m_ptrekInput->IsTrekKeyDown(TK_ViewRight,bAllowKeyboardMovement))

							m_cameraControl.SetHeadOrientation(js.hat);
					}
                    else
                        m_cameraControl.SetHeadOrientation(0.0f);


                    m_pplayerThrottle->SetValue(pshipParent
                                                ? 0.0f
                                                : ((buttonsM & (leftButtonIGC | rightButtonIGC | upButtonIGC | downButtonIGC |
                                                                forwardButtonIGC | backwardButtonIGC | afterburnerButtonIGC))
                                                   ? 1.0f
                                                   : (js.controls.jsValues[c_axisThrottle] * 0.5f + 0.5f)));
                }

                if (CommandCamera(m_cm) && m_bEnableDisplacementCommandView && !m_pconsoleImage->DrawSelectionBox())
                {
                    float delta = dt * m_distanceCommandCamera;

                    float   dRight = 0.0f;
                    float   dUp = 0.0f;

                    Point point;
                    bool  bInside;
                    Rect  rectImage = GetScreenRectValue()->GetValue();

                    if (GetEngine()->IsFullscreen()) {
                        point = GetMousePosition();
                        point.SetY(GetScreenRectValue()->GetValue().YMax() - 1 - point.Y());
                        bInside = rectImage.Inside(point);
                        point = point - rectImage.Min();
                    } else {
                        WinPoint   xy;
                        bInside = false;
                        if (GetCursorPos(&xy)) {
                            point   = Point::Cast(ScreenToClient(xy));
                            bInside = rectImage.Inside(point);
                            point   = point - rectImage.Min();
                        }
                    }

                    if (bInside) {
                        static const float gutter = 8.0f;
                        static const float maxGutter = 12.0f;

                        if (point.X() < gutter) {
                            dRight = delta * (point.X() - gutter) / maxGutter;
                        } else {
                            float x = rectImage.XSize() - point.X();
                            if (x < gutter) {
                                dRight = delta * (gutter - x) / maxGutter;
                            }
                        }

                        if (point.Y() < gutter) {
                            dUp = delta * (point.Y() - gutter) / maxGutter;
                        } else {
                            float y = rectImage.YSize() - point.Y();
                            if (y < gutter) {
                                dUp = delta * (gutter - y) / maxGutter;
                            }
                        }
                    }

                    // Cheat ... horizontal is always parallel to the plane

                    if (m_ptrekInput->IsTrekKeyDown(TK_YawLeft, bAllowKeyboardMovement))
                    {
                        dRight -= delta;
                    }
                    else if (m_ptrekInput->IsTrekKeyDown(TK_YawRight, bAllowKeyboardMovement))
                    {
                        dRight += delta;
                    }

                    if (m_ptrekInput->IsTrekKeyDown(TK_PitchUp, bAllowKeyboardMovement))
                    {
                        dUp -= delta;
                    }
                    else if (m_ptrekInput->IsTrekKeyDown(TK_PitchDown, bAllowKeyboardMovement))
                    {
                        dUp += delta;
                    }

                    if (m_ptrekInput->IsTrekKeyDown(TK_ZoomOut, bAllowKeyboardMovement))
                    {
                        m_distanceCommandCamera += delta * 2.0f;
                        if (m_distanceCommandCamera > s_fCommandViewDistanceMax)
                            m_distanceCommandCamera = s_fCommandViewDistanceMax;
                    }
                    else if (m_ptrekInput->IsTrekKeyDown(TK_ZoomIn, bAllowKeyboardMovement))
                    {
                        m_distanceCommandCamera -= delta * 2.0f;

                        if (m_distanceCommandCamera < s_fCommandViewDistanceMin)
                            m_distanceCommandCamera = s_fCommandViewDistanceMin;
                    }

                    if (m_ptrekInput->IsTrekKeyDown(TK_RollRight, bAllowKeyboardMovement))
                    {
                        m_rollCommandCamera += dt;

                        if (m_rollCommandCamera >= 2.0f * pi)
                            m_rollCommandCamera -= 2.0f * pi;

                        OrientCommandView();
                    }
                    else if (m_ptrekInput->IsTrekKeyDown(TK_RollLeft, bAllowKeyboardMovement))
                    {
                        m_rollCommandCamera -= dt;

                        if (m_rollCommandCamera < 0.0f)
                            m_rollCommandCamera += 2.0f * pi;

                        OrientCommandView();
                    }

                    if (m_ptrekInput->IsTrekKeyDown(TK_ThrustRight, bAllowKeyboardMovement))
                        dRight += delta / 2;
                    if (m_ptrekInput->IsTrekKeyDown(TK_ThrustLeft, bAllowKeyboardMovement))
                        dRight -= delta / 2;
                    if (m_ptrekInput->IsTrekKeyDown(TK_ThrustUp, bAllowKeyboardMovement))
                        dUp -= delta / 2; // Something is off here - should be called dDown. This is what happens if you don't just stick with top/left as 0/0 as it's meant to be done. Also costs performance above.
                    if (m_ptrekInput->IsTrekKeyDown(TK_ThrustDown, bAllowKeyboardMovement))
                        dUp += delta / 2;

                    if (dRight)
                    {
                        Vector  v = m_cameraControl.GetOrientation().GetRight() * dRight;
                        m_displacementCommandView += v;
                        m_positionCommandView += v;
                    }

                    if (dUp)
                    {
                        Vector  up = m_cameraControl.GetOrientation().GetUp();
                        up.z = 0.0f;

                        float   l = (float)sqrt(up.x * up.x + up.y * up.y);
                        assert (l != 0.0f);


                        Vector  v = up * (dUp / l);
                        m_displacementCommandView -= v;
                        m_positionCommandView -= v;
                    }
                }
                else if (!bNoCameraControl)
                {
                    if (m_ptrekInput->IsTrekKeyDown(TK_PitchUp, bAllowKeyboardMovement))
                        m_orientationExternalCamera.Pitch(dt);
                    else  if (m_ptrekInput->IsTrekKeyDown(TK_PitchDown, bAllowKeyboardMovement))
                        m_orientationExternalCamera.Pitch(-dt);

                    if (m_ptrekInput->IsTrekKeyDown(TK_YawLeft, bAllowKeyboardMovement))
                        m_orientationExternalCamera.Yaw(dt);
                    else  if (m_ptrekInput->IsTrekKeyDown(TK_YawRight, bAllowKeyboardMovement))
                        m_orientationExternalCamera.Yaw(-dt);

                    if (m_ptrekInput->IsTrekKeyDown(TK_RollLeft, bAllowKeyboardMovement))
                        m_orientationExternalCamera.Roll(dt);
                    else  if (m_ptrekInput->IsTrekKeyDown(TK_RollRight, bAllowKeyboardMovement))
                        m_orientationExternalCamera.Roll(-dt);

                    if (m_ptrekInput->IsTrekKeyDown(TK_ZoomOut, bAllowKeyboardMovement))
                    {
                        m_distanceExternalCamera += dt * m_distanceExternalCamera;
                        if (m_distanceExternalCamera > s_fExternalViewDistanceMax)
                            m_distanceExternalCamera = s_fExternalViewDistanceMax;
                    }
                    else if (m_ptrekInput->IsTrekKeyDown(TK_ZoomIn, bAllowKeyboardMovement))
                    {
                        m_distanceExternalCamera -= dt * m_distanceExternalCamera;

                        if (m_distanceExternalCamera < s_fExternalViewDistanceMin)
                            m_distanceExternalCamera = s_fExternalViewDistanceMin;
                    }
                }
                else if (m_cm == cmExternalChase)
                {
                    if (m_ptrekInput->IsTrekKeyDown(TK_ZoomOut, bAllowKeyboardMovement))
                    {
                        m_distanceExternalCamera += dt * m_distanceExternalCamera;
                        if (m_distanceExternalCamera > s_fExternalViewDistanceMax)
                            m_distanceExternalCamera = s_fExternalViewDistanceMax;
                    }
                    else if (m_ptrekInput->IsTrekKeyDown(TK_ZoomIn, bAllowKeyboardMovement))
                    {
                        m_distanceExternalCamera -= dt * m_distanceExternalCamera;

                        if (m_distanceExternalCamera < s_fExternalViewDistanceMin)
                            m_distanceExternalCamera = s_fExternalViewDistanceMin;
                    }
                }
            }

            trekClient.SendUpdate(lastUpdate);

            IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();

            //Change sounds for warning state as appropriate
            {
                WarningMask wm = trekClient.GetShip()->GetWarningMask();
                if (trekClient.wmOld != wm)
                {
                    if ((wm & c_wmOutOfBounds) && ((trekClient.wmOld & c_wmOutOfBounds) == 0))
                    {
                        trekClient.PlaySoundEffect(salBoundryExceededSound);
                    }

                    if ((wm & c_wmCrowdedSector) && ((trekClient.wmOld & c_wmCrowdedSector) == 0))
                    {
                        trekClient.PlaySoundEffect(salSectorCrowdedSound);
                    }

                    trekClient.wmOld = wm;
                }
            }

            //NYI this should only be done once or on a side change
            const float alpha = 0.75f;

            IsideIGC* pside = trekClient.GetShip()->GetSide();
            assert (pside);
            Color   colorHUD = pside->GetColor();
            colorHUD = colorHUD * g_hudBright;
            colorHUD.SetAlpha(alpha);
            m_pcolorHUD->SetValue(colorHUD);

            const float alphaless = 0.55f;

            Color   colorHUDshadows = Color(45, 36, 59);
            colorHUDshadows = colorHUDshadows * 0.65f;
            colorHUDshadows.SetAlpha(alphaless);
            m_pcolorHUDshadows->SetValue(colorHUDshadows);
		} else {

			// //-Imago 7/13/09 we're not actually in a sector playing the game...
			// this is the right time & place place to rest our CPU. We can also give it more of a break now.
			if (!GetFullscreen())
				Sleep(5);
			else
				Sleep(1);
		}
        //
        // Handle sending network messages
        //
        trekClient.CheckServerLag(Time::Now());
        if (trekClient.m_fm.IsConnected())
            trekClient.SendMessages();
        else {
            //------------------------------------------------------------------------------
            // Interception point for handling training missions
            // =================================================
            // This call is to be made when we are running a training mission to check to
            // see if mission goals have been accomplished, and prevent the player from
            // running off into space wherever appropriate. The function called will
            // maintain the state, and provide guidance and restrictions as applicable.
            //------------------------------------------------------------------------------

            Training::HandleMission();

            //------------------------------------------------------------------------------
            // End interception for training missions
            //------------------------------------------------------------------------------
        }

        trekClient.m_pCoreIGC->Update(now);

        /*
        //
        // check for the clock bug.
        //
        static bool bWarnedAboutClockBug = false;

        if (!bWarnedAboutClockBug && trekClient.m_sync < 0.30)
        {
            bWarnedAboutClockBug = true;
            TRef<IMessageBox> pmsgBox = CreateMessageBox("You are experiencing the clock bug.");
            OpenPopup(pmsgBox);
        }
        */
	}

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& bForceTranslate)
    {
        if (IsInputEnabled (ks.vk)) {
            m_ptrekInput->SetFocus(true);
            TrekKey tk = m_ptrekInput->HandleKeyMessage(ks);

            if (g_bLuaDebug && ks.bDown && ks.bAlt) {
                switch (ks.vk) {
                case VK_F1:
                    m_pUiEngine->TriggerReload();
                    return true;
                }
            }
            if (TK_NoKeyMapping != tk) {
                if (ks.bDown) {

                    // hook to send this keypress to training missions
                    if (Training::RecordKeyPress (tk))
                    {
                        if (
                               trekClient.IsInGame()
                            && ((GetViewMode() != vmOverride) || (tk == TK_StartChat))
                            && !trekClient.IsLockedDown()
                        ) {
                            OnTrekKey(tk);
                        }

                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        if (!GetUI()) {
            return m_pconsoleImage->OnChar(ks);
        } else {
            return false;
        }
    }

    void SetTargetGeo(ImodelIGC* pmodel)
    {
        const float alpha = 0.5f;
        Color colorTargetHUD(alpha, alpha, alpha, alpha);

        m_cameraControl.m_pgeoTarget = Geo::GetEmpty();

        if (pmodel != NULL)
        {
            if (pmodel->GetObjectType() == OT_ship)
                pmodel = ((IshipIGC*)pmodel)->GetSourceShip();

            if ((pmodel != trekClient.GetShip()->GetSourceShip()) &&
                (pmodel->GetHitTest() != NULL) &&
                (pmodel->GetCluster() == trekClient.GetCluster()) &&
                trekClient.GetShip()->CanSee(pmodel))
            {
                ThingSite* pthingSite = pmodel->GetThingSite();

                if (pthingSite)
                {
                    TRef<ThingGeo>  pthing;
                    CastTo(pthing, pthingSite->GetThingGeo());
                    if (pthing)
                        m_cameraControl.m_pgeoTarget = pthing->GetTargetGeo();
                }
            }

            IsideIGC* pside = pmodel->GetSide();
            if (pside)
            {
                colorTargetHUD = pside->GetColor();
                colorTargetHUD = colorTargetHUD * g_hudBright;
                colorTargetHUD.SetAlpha(alpha);
            }
        }

        m_pcolorTargetHUD->SetValue(colorTargetHUD);
    }

    void SetTarget(ImodelIGC* pmodel, CommandID cid)
    {
        if (pmodel && trekClient.GetShip()->GetCommandTarget(c_cmdCurrent) != pmodel)
        {
            StartSound(newTargetSound);
        }

        if (cid == c_cidDefault)
            cid = pmodel ? trekClient.GetShip()->GetDefaultOrder(pmodel) : c_cidNone;

        trekClient.GetShip()->SetCommand(c_cmdCurrent, pmodel, cid);

        if (trekClient.m_fm.IsConnected())
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmOC, CS, ORDER_CHANGE)
            END_PFM_CREATE

            //Don't bother to set ship ID
            pfmOC->command = c_cmdCurrent;
            pfmOC->commandID = cid;
            if (pmodel)
            {
                pfmOC->objectType = pmodel->GetObjectType();
                pfmOC->objectID = pmodel->GetObjectID();
            }
            else
            {
                pfmOC->objectType = OT_invalid;
                pfmOC->objectID = NA;
            }
        }

        if (CommandCamera(m_cm))
        {
            if (m_cameraControl.GetAnimation() < 1.0f)
                m_cameraControl.SetAnimation(1.0f);

            if (pmodel && !m_bTrackCommandView)
            {
                if (pmodel->GetObjectType() == OT_ship)
                    pmodel = ((IshipIGC*)pmodel)->GetSourceShip();

                if ((pmodel->GetCluster() == trekClient.GetCluster()) &&
                    trekClient.GetShip()->CanSee(pmodel))
                {
                    m_positionCommandView = pmodel->GetPosition();
                }
            }

            m_displacementCommandView = Vector::GetZero();
        }

    }
    void SetAccepted(ImodelIGC* pmodel, CommandID cid)
    {
        if ((cid != c_cidNone) && m_pconsoleImage)
        {
            trekClient.PostText(false, "Command accepted.");
            trekClient.PostText(true, "");
        }

        trekClient.GetShip()->SetCommand(c_cmdAccepted, pmodel, cid);
        /*
        if (trekClient.autoPilot())
        {
            if (pmodel)
                SetTarget(pmodel, cid);
            else
            {
                trekClient.SetAutoPilot(false);
                SwitchToJoyThrottle();
                trekClient.PlaySoundEffect(salAutopilotDisengageSound);
            }
        }
        */

        if (trekClient.m_fm.IsConnected())
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmOC, CS, ORDER_CHANGE)
            END_PFM_CREATE

            //Don't bother to set ship ID
            pfmOC->command = c_cmdAccepted;
            pfmOC->commandID = cid;
            if (pmodel)
            {
                pfmOC->objectType = pmodel->GetObjectType();
                pfmOC->objectID = pmodel->GetObjectID();
            }
            else
            {
                pfmOC->objectType = OT_invalid;
                pfmOC->objectID = NA;
            }
        }
    }

    void    SetTimeDamaged(Time now)
    {
        if (now <= m_timeLastDamage + c_dtFlashingDuration)
        {
            //We were already hit and flashing. Fudge the time hit to the most recent boundary between
            //on and off
            float   dt = now - m_timeLastDamage;
            float   n = float(floor(dt / (c_dtFlashOn + c_dtFlashOff)));

            m_timeLastDamage += n * (c_dtFlashOn + c_dtFlashOff);
        }
        else
            m_timeLastDamage = now; //Start flashing now
    }

    bool    GetFlash(Time now)
    {
        bool    bFlash;

        if (now <= m_timeLastDamage + c_dtFlashingDuration)
        {
            //We were already hit and flashing. Fudge the time hit to the most recent boundary between
            //on and off
            float   dt = now - m_timeLastDamage;
            float   n = float(floor(dt / (c_dtFlashOn + c_dtFlashOff)));

            Time    timeStartFlash = m_timeLastDamage + n * (c_dtFlashOn + c_dtFlashOff);
            bFlash = (now <= (timeStartFlash + c_dtFlashOn));
        }
        else
            bFlash = false;

        return bFlash;
    }

    void    TrekFindTarget(int              ttMask,
                           TrekKey          tk,
                           TrekKey          tkNearest,
                           TrekKey          tkPrevious,
                           AbilityBitMask   abm = 0)
    {
        assert ((ttMask & c_ttFront) == 0);
		if (tk == tkNearest)
            ttMask |= c_ttNearest;
		else if (tk == tkPrevious) 
            ttMask |= c_ttPrevious;

        const Vector*   pposition;
        if (trekClient.GetViewCluster())
        {
            IstationIGC*    pstation = trekClient.GetShip()->GetStation();
            if (pstation && (pstation->GetCluster() == trekClient.GetViewCluster()))
                pposition = &(pstation->GetPosition());
            else
                pposition = &(Vector::GetZero());
        }
		else
			pposition = &(trekClient.GetShip()->GetPosition());

		ImodelIGC*  m = NULL; //Imago 7/31/09 ALLY 

		if (trekClient.GetSide()->GetAllies() != NA) { 

			//only target our sides bases unless in a pod
			if (tkNearest == TK_TargetFriendlyBaseNearest) {

				//determine if we're in a pod or not
				bool bLifepod = false;
				IhullTypeIGC*   pht = trekClient.GetShip()->GetSourceShip()->GetBaseHullType();
				if (pht) {
					HullAbilityBitMask  habm = pht->GetCapabilities();
					if (habm & c_habmLifepod)
						bLifepod = true;
				}

				if (!bLifepod)
					m = FindTarget(trekClient.GetShip()->GetSourceShip(), ttMask, trekClient.GetShip()->GetCommandTarget(c_cmdCurrent),
						trekClient.GetCluster(), pposition, NULL, abm, 0x7fffffff, 0); // false = don't search allied bases
				else
					m = FindTarget(trekClient.GetShip()->GetSourceShip(), ttMask, trekClient.GetShip()->GetCommandTarget(c_cmdCurrent),
						trekClient.GetCluster(), pposition, NULL, abm); // true = (default) search allied bases/rescue probes

			//targets allied bases only
			} else if (tkNearest == TK_TargetAlliedBaseNearest) {
				m = FindTarget(trekClient.GetShip()->GetSourceShip(), ttMask, trekClient.GetShip()->GetCommandTarget(c_cmdCurrent),
					trekClient.GetCluster(), pposition, NULL, abm, 0x7fffffff, 2); // 2 = (hack) search allied bases/rescue probes only

			} else {
				//not after a friendly/allied base, do our regular thing (default)
				m = FindTarget(trekClient.GetShip()->GetSourceShip(), ttMask, trekClient.GetShip()->GetCommandTarget(c_cmdCurrent),
					trekClient.GetCluster(), pposition, NULL, abm);
			}

		} else { // team has no allies, do our regular thing

			m = FindTarget(trekClient.GetShip()->GetSourceShip(),
				ttMask,
				trekClient.GetShip()->GetCommandTarget(c_cmdCurrent),
				trekClient.GetCluster(),
				pposition, NULL, abm);
		}

		SetTarget(m, c_cidDefault);
    }

    void ToggleWeapon(Mount mount)
    {
        IshipIGC* pship = trekClient.GetShip();

        if (pship->GetParentShip() == NULL && (m_viewmode != vmHangar && m_viewmode != vmLoadout)) //#207 - Imago 8/10
        {
            Mount nHardpoints = pship->GetHullType()->GetMaxFixedWeapons();
            IweaponIGC* pweapon = (IweaponIGC*)pship->GetMountedPart(ET_Weapon, mount);

            if (pweapon && mount < nHardpoints)
            {
                int stateM = trekClient.GetShip()->GetStateM();
                Mount mountSelected = (stateM & selectedWeaponMaskIGC) >> selectedWeaponShiftIGC;

                if (trekClient.fGroupFire
                    || pweapon->GetMountID() != mountSelected)
                {
                    // set the client to single fire with this weapon
                    trekClient.SetSelectedWeapon(mount);
                    trekClient.fGroupFire = false;
                    trekClient.PlaySoundEffect(ungroupWeaponsSound, trekClient.GetShip());
                }
                else
                {
                    // set the client back to group fire
                    trekClient.fGroupFire = true;
                    trekClient.PlaySoundEffect(groupWeaponsSound, trekClient.GetShip());
                }
            }
        }
    }

    int FindPartIndex(IpartIGC* ppart)
    {
        const PartLinkIGC* ppartLink = trekClient.GetShip()->GetParts()->first();
        int   index                  = 0;

        while (ppartLink->data() != ppart)
        {
            index++;
            ppartLink = ppartLink->next();

            assert (ppartLink);
        }

        return index;
    }

    void SwapPart(EquipmentType et, Mount mount)
    {
        IshipIGC* pship = trekClient.GetShip();

        if ((pship->GetParentShip() == NULL) && trekClient.flyingF())
        {
            PartMask maxPartMask = pship->GetHullType()->GetPartMask(et, mount);
            IpartIGC*   ppartOld = pship->GetMountedPart(et, mount);

            if (trekClient.SelectCargoPartOfType(et, maxPartMask, ppartOld ? ppartOld->GetPartType() : NULL))
            {
                Mount       mountNew = trekClient.GetSelectedCargoMount();
                IpartIGC*   ppartNew = pship->GetMountedPart(et, mountNew);

                if (ppartOld || ppartNew)
                {
                    if (ppartOld)
                        trekClient.SwapPart(ppartOld, mountNew);
                    else if (ppartNew)
                        trekClient.SwapPart(ppartNew, mount);

                    if (ppartNew)
                    {
                        trekClient.PlaySoundEffect(mountSound, trekClient.GetShip());
                    }
                    else if (ppartOld)
                    {
                        trekClient.PlaySoundEffect(unmountSound, trekClient.GetShip());
                    }

                    // set the inventory slot to the next item of that type
                    trekClient.NextCargoPart();
                    trekClient.SelectCargoPartOfType(et, maxPartMask, ppartNew ? ppartNew->GetPartType() : NULL);

                    trekClient.GetCore()->GetIgcSite()->LoadoutChangeEvent(pship, trekClient.GetCargoPart(), c_lcCargoSelectionChanged);
                }
            }
            else
            {
                trekClient.PlaySoundEffect(salCargoFullSound, trekClient.GetShip());
            }
        }
    }

    void SwapWeapon(Mount mount)
    {
        IshipIGC* pship = trekClient.GetShip();

        if ((pship->GetParentShip() == NULL) && trekClient.flyingF())
        {
            Mount nHardpoints = pship->GetHullType()->GetMaxFixedWeapons();

            if (mount < nHardpoints)
            {
                SwapPart(ET_Weapon, mount);
            }
        }
    }

    void SwapTurret(int id)
    {
        ZAssert(id >= 0);

        IshipIGC* pship = trekClient.GetShip();

        if ((pship->GetParentShip() == NULL) && trekClient.flyingF())
        {
            Mount nFixedHardpoints = pship->GetHullType()->GetMaxFixedWeapons();
            int nTurrets = pship->GetHullType()->GetMaxWeapons() - nFixedHardpoints;

            if (id < nTurrets)
            {
                SwapPart(ET_Weapon, id + nFixedHardpoints);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // HUD Style exposure
    //
    //////////////////////////////////////////////////////////////////////////////

    float   GetHUDStyle (void) const
    {
        return m_pnumberStyleHUD->GetValue ();
    }

    void    SetHUDStyle (float newStyle)
    {
        m_pnumberStyleHUD->SetValue (newStyle);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Help stuff
    //
    //////////////////////////////////////////////////////////////////////////////

    class HelpPosition :
        public PointValue,
        public IEventSink
    {
    private:
        bool  m_bOn;
        float m_timeStart;
        float m_valueStart;
        float m_value;
        Point m_positionOff;
        Point m_positionOn;

        float GetTime() { return Number::Cast(GetChild(0))->GetValue() * 2.0f; }

    public:
        HelpPosition(Number* ptime, IEventSource* peventSource) :
            PointValue(ptime),
            m_bOn(false),
            m_timeStart(ptime->GetValue()),
            m_valueStart(0),
            m_value(0),
            m_positionOn(30, 30),
            m_positionOff(-1300, 30)
        {
            peventSource->AddSink(this);
        }

        bool OnEvent(IEventSource* pevent)
        {
            m_bOn        = false;
            m_timeStart  = GetTime();
            m_valueStart = m_value;
            return true;
        }

        void Toggle(bool bOn)
        {
            m_bOn        = bOn || !m_bOn;
            m_timeStart  = GetTime();
            m_valueStart = m_value;
        }

        void Evaluate()
        {
            if (m_bOn) {
                m_value = std::min(1.0f, m_valueStart + (GetTime() - m_timeStart));
            } else {
                m_value = std::max(0.0f, m_valueStart - (GetTime() - m_timeStart));
            }

            GetValueInternal() = Interpolate(m_positionOff, m_positionOn, m_value);
        }
    };

    TRef<HelpPane>     m_phelp;
    TRef<HelpPosition> m_phelpPosition;

    class PagePaneIncluderImpl : public PagePaneIncluder {
    public:
        TRef<ZFile> Include(const ZString& str)
        {
            return NULL;
        }
    };

    ZString GetVersionString()
    {
    }

    void InitializeHelp()
    {
        m_phelp = CreateHelpPane(GetModeler(), "hlpStart", new PagePaneIncluderImpl());

        m_phelp->SetString("ver", ZVersionInfo().GetProductVersionString());

        m_phelpPosition = new HelpPosition(GetTime(), m_phelp->GetEventSourceClose());

		GetModeler()->SetColorKeyHint( true );

        m_pwrapImageHelp->SetImage(
            new TransformImage(
                CreatePaneImage(
                    GetEngine(),
                    true,
                    m_phelp
                ),
                new TranslateTransform2(m_phelpPosition)
            )
        );

		GetModeler()->SetColorKeyHint( false );
	}

    void OnHelp(bool bOn)
    {
        trekClient.PlaySoundEffect(positiveButtonClickSound);
        m_phelpPosition->Toggle(bOn);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnTrekKeyFilter(TrekKey tk)
    {
        if (m_screen != ScreenIDSplashScreen) {
            switch(tk) {
				// SR: added ability to take screenshot
				case TK_ScrnShot:
					TakeScreenShot();
					return true;

				// Toggle virtual joystick during launch animation
				case TK_ToggleMouse:
					if (trekClient.IsInGame() &&
					GetViewMode() == vmOverride &&
					!trekClient.IsLockedDown() && 
                    (m_pconsoleImage == NULL || !m_pconsoleImage->IsComposing())) {
						m_bEnableVirtualJoystick = !m_bEnableVirtualJoystick;
						if(m_bEnableVirtualJoystick) m_ptrekInput->ClearButtonStates();//#56
						return true;
					}
					return false;

                case TK_Help:
                    OnHelp(false);
                    return true;

                case TK_MainMenu:
                    if (
                           (
                               !trekClient.IsInGame()
                            || (GetViewMode() != vmOverride)
                           )
                        && !trekClient.IsLockedDown()
                        && (
                               m_pconsoleImage == NULL
                            || !m_pconsoleImage->IsComposing()
                           )
                    ) {
                        ShowMainMenu();
                        return true;
                    }

                    return false;

                case TK_ChatPageUp:
                    if (m_pchatListPane != NULL) {
                        m_pchatListPane->PageUp();
                    }
                    return true;

                case TK_ChatPageDown:
                    if (m_pchatListPane != NULL) {
                        m_pchatListPane->PageDown();
                    }
                    return true;

                case TK_QuickChatMenu:
                    if (
                           trekClient.MyMission()
                        && (!trekClient.IsInGame() || (GetViewMode() != vmOverride))
                        && !trekClient.IsLockedDown()
                        && (m_screen == ScreenIDGameOverScreen
                            || m_screen == ScreenIDTeamScreen
                            || m_screen == ScreenIDCombat)
                    ) {
                        OpenMainQuickChatMenu();
                    }
                    return true;
            }
        }

        return false;
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////
    void SetViewToCombatMode (CameraMode newCameraMode)
    {
        // turn off command view overlays
        TurnOffOverlayFlags(c_omBanishablePanes);

        if (GetViewMode () == vmCombat)
        {
            // if we aren't already facing ahead, face ahead
            m_cameraControl.SetHeadOrientation(0.0f);

            // set the desired camera mode
            SetCameraMode (newCameraMode);
        }
        else if (trekClient.flyingF())
        {
            // set the mode to combat
            SetViewMode (vmCombat);

            // set the desired camera mode
            SetCameraMode (newCameraMode);
        }
        else
        {
            // switch to the hangar mode
            SetViewMode(trekClient.GetShip()->IsGhost() ? vmCommand : vmHangar);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnTrekKey(TrekKey tk)
    {
        bool suicideF = false;

        switch(tk)
        {
            
            case TK_ChatPageUp: // Imago uncommented for mouse wheel 8/14/09
                if (m_pchatListPane != NULL && !m_ptrekInput->IsTrekKeyDown(TK_ChatPageUp, true)) {
                    m_pchatListPane->PageUp();
                }
                break;

            case TK_ChatPageDown: // Imago uncommented for mouse wheel 8/14/09
                if (m_pchatListPane != NULL && !m_ptrekInput->IsTrekKeyDown(TK_ChatPageDown, true)) {
                    m_pchatListPane->PageDown();
                }
                break;
            /*
            case TK_QuickChatMenu:
                if (
                       trekClient.MyMission()
                    && (!trekClient.IsInGame() || (GetViewMode() != vmOverride))
                    && !trekClient.IsLockedDown()
                ) {
                    OpenMainQuickChatMenu();
                }
                break;
            */

            case TK_ToggleMouse:
            {
                if (m_voverlaymask[m_viewmode] & (c_omBanishablePanes & ~ofInvestment)) {
                    TurnOffOverlayFlags(c_omBanishablePanes & ~ofInvestment);
                    m_bEnableVirtualJoystick = true;
                }
                else if (m_bFreshInvestmentPane && (m_voverlaymask[m_viewmode] & ofInvestment))
                    m_bFreshInvestmentPane = false;
                else
                    m_bEnableVirtualJoystick = !m_bEnableVirtualJoystick;
            }
            break;

            case TK_TrackCommandView:
            {
                m_bTrackCommandView = !m_bTrackCommandView;
                trekClient.PostText(false, m_bTrackCommandView ? "Command view tracking enabled" : "Command view tracking disabled");
            }
            break;

            case TK_StartChat:
            {
                m_pconsoleImage->StartChat();
                m_ptrekInput->SetFocus(false);
            }
            break;

            case TK_ToggleLODSlider:
            {
                if (m_pwrapImageLOD->GetImage() == Image::GetEmpty()) {
                    m_pwrapImageLOD->SetImage(m_pimageLOD);
                } else {
                    m_pwrapImageLOD->SetImage(Image::GetEmpty());
                }
            }
            break;

            case TK_LockVector:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                {
                    trekClient.GetShip()->SetStateBits(coastButtonIGC, Training::ApproveActions (trekClient.GetShip()->GetStateM() ^ coastButtonIGC));
                    trekClient.PlaySoundEffect(vectorLockSound);
                }
            }
            break;

            case TK_ToggleCloak:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                    trekClient.GetShip()->SetStateBits(cloakActiveIGC, Training::ApproveActions (trekClient.GetShip()->GetStateM() ^ cloakActiveIGC));
            }
            break;

            case TK_Ripcord:
            {
                
                    if ((trekClient.GetShip()->GetParentShip() == NULL) &&
                        trekClient.GetShip()->GetCluster())
                    {
                        if ((trekClient.GetShip()->GetFlag() != NA) ||
                            trekClient.GetShip()->GetBaseHullType()->HasCapability(c_habmNoRipcord))
                        {
                            trekClient.PlayNotificationSound(salShipCantRipcordSound, trekClient.GetShip());
                        }
                        else
                        {
                            IclusterIGC*    pcluster = NULL;
                            if (!trekClient.GetShip()->fRipcordActive())
                            {
                                ImodelIGC*  pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);

                                if (!pmodel)
                                    pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdAccepted);

                                if (pmodel)
                                    pcluster = trekClient.GetCluster(trekClient.GetShip(), pmodel);

                                if (!pcluster)
                                    pcluster = trekClient.GetShip()->GetCluster();
                                assert (pcluster);
                            }
                            trekClient.RequestRipcord(trekClient.GetShip(), pcluster);
                        }
                    }
            }
            break;

            case TK_PrevWeapon:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                    trekClient.PreviousWeapon();
            }
            break;

            case TK_NextWeapon:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                    trekClient.NextWeapon();
            }
            break;

            case TK_ToggleGroupFire:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                    trekClient.fGroupFire = !trekClient.fGroupFire;

                if (trekClient.fGroupFire)
                    trekClient.PlaySoundEffect(groupWeaponsSound, trekClient.GetShip());
                else
                    trekClient.PlaySoundEffect(ungroupWeaponsSound, trekClient.GetShip());
            }
            break;

            case TK_SwapWeapon1:
            {
                SwapWeapon(0);
            }
            break;

            case TK_SwapWeapon2:
            {
                SwapWeapon(1);
            }
            break;

            case TK_SwapWeapon3:
            {
                SwapWeapon(2);
            }
            break;

            case TK_SwapWeapon4:
            {
                SwapWeapon(3);
            }
            break;

            case TK_SwapTurret1:
            {
                SwapTurret(0);
            }
            break;

            case TK_SwapTurret2:
            {
                SwapTurret(1);
            }
            break;

            case TK_SwapTurret3:
            {
                SwapTurret(2);
            }
            break;

            case TK_SwapTurret4:
            {
                SwapTurret(3);
            }
            break;

            case TK_PromoteTurret1:
            case TK_PromoteTurret2:
            case TK_PromoteTurret3:
            case TK_PromoteTurret4:
            {
                IhullTypeIGC*   pht = trekClient.GetShip()->GetBaseHullType();
                if (pht)
                {
                    Mount   turret = Mount(tk - TK_PromoteTurret1) + pht->GetMaxFixedWeapons();
                    if (turret < pht->GetMaxWeapons())
                    {
                        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
                        BEGIN_PFM_CREATE(trekClient.m_fm, pfmPromote, C, PROMOTE)
                        END_PFM_CREATE;

                        pfmPromote->mountidPromoted = turret;
                    }
                }
            }
            break;

            case TK_SwapMissile:
            {
                SwapPart(ET_Magazine, 0);
            }
            break;

            case TK_SwapChaff:
            {
                SwapPart(ET_ChaffLauncher, 0);
            }
            break;

            case TK_SwapShield:
            {
                SwapPart(ET_Shield, 0);
            }
            break;

            case TK_SwapCloak:
            {
                SwapPart(ET_Cloak, 0);
            }
            break;

            case TK_SwapAfterburner:
            {
                SwapPart(ET_Afterburner, 0);
            }
            break;

            case TK_SwapMine:
            {
                SwapPart(ET_Dispenser, 0);
            }
            break;

            case TK_NextCargo:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                {
                    trekClient.NextCargoPart();
                    trekClient.GetCore()->GetIgcSite()->LoadoutChangeEvent(trekClient.GetShip(), trekClient.GetCargoPart(), c_lcCargoSelectionChanged);
                }
            }
            break;

            case TK_ToggleWeapon1:
            {
                ToggleWeapon(0);
            }
            break;

            case TK_ToggleWeapon2:
            {
                ToggleWeapon(1);
            }
            break;

            case TK_ToggleWeapon3:
            {
                ToggleWeapon(2);
            }
            break;

            case TK_ToggleWeapon4:
            {
                ToggleWeapon(3);
            }
            break;

            case TK_Reload:
            {
                if (trekClient.GetShip()->GetParentShip() == NULL)
                {
                    int stateM = trekClient.GetShip()->GetStateM();

                    if ((stateM & (oneWeaponIGC | allWeaponsIGC)) == 0)
                        trekClient.Reload(trekClient.GetShip(), NULL, ET_Weapon);

                    if ((stateM & afterburnerButtonIGC) == 0)
                        trekClient.Reload(trekClient.GetShip(), NULL, ET_Afterburner);

                    if ((stateM & missileFireIGC) == 0)
                        trekClient.Reload(trekClient.GetShip(), (IlauncherIGC*)(trekClient.GetShip()->GetMountedPart(ET_Magazine, 0)), ET_Magazine);
                    if ((stateM & mineFireIGC) == 0)
                        trekClient.Reload(trekClient.GetShip(), (IlauncherIGC*)(trekClient.GetShip()->GetMountedPart(ET_Dispenser, 0)), ET_Dispenser);
                    if ((stateM & chaffFireIGC) == 0)
                        trekClient.Reload(trekClient.GetShip(), (IlauncherIGC*)(trekClient.GetShip()->GetMountedPart(ET_ChaffLauncher, 0)), ET_ChaffLauncher);

                    trekClient.PlayNotificationSound(salReloadingSound, trekClient.GetShip());
                }
            }
            break;

            case TK_DropCargo:
            {
                if (trekClient.flyingF() && (trekClient.GetShip()->GetParentShip() == NULL))
                {
                    IpartIGC* ppart = trekClient.GetCargoPart();

                    if (ppart)
                    {
                        // to keep from changing the order of things in the
                        // inventory pane, always drop the last instance of
                        // this part type.
                        IpartTypeIGC *ppartType = ppart->GetPartType();

                        for (Mount mount = -c_maxCargo; mount < 0; ++mount)
                        {
                            IpartIGC *ppartPrev = trekClient.GetShip()->GetMountedPart(NA, mount);
                            IpartTypeIGC *ppartTypePrev = ppartPrev ? ppartPrev->GetPartType() : NULL;

                            if (ppartType == ppartTypePrev)
                            {
                                trekClient.DropPart(ppartPrev);
                                trekClient.PlaySoundEffect(dropSound, trekClient.GetShip());
                                break;
                            }
                        }
                    }
                    else
                    {
                        trekClient.NextCargoPart();
                    }
                    trekClient.GetCore()->GetIgcSite()->LoadoutChangeEvent(trekClient.GetShip(), trekClient.GetCargoPart(), c_lcCargoSelectionChanged);
                }
            }
            break;

            case TK_ViewRearLeft:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi * 1.25f);
                }
            }
            break;

            case TK_ViewRearRight:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi * 0.75f);
                }
            }
            break;

            case TK_ViewFrontLeft:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi * 1.75f);
                }
            }
            break;

            case TK_ViewFrontRight:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi * 0.25f);
                }
            }
            break;

            case TK_ViewRear:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi);
                }
            }
            break;

            case TK_ViewLeft:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi * 1.5f);
                }
            }
            break;

            case TK_ViewRight:
            {
                if ((m_cm == cmCockpit) || (m_cm == cmExternalChase))
                {
                    m_cameraControl.SetHeadOrientation(pi * 0.5f);
                }
            }
            break;

            case TK_ViewExternal:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternal) ? cmExternal : cmCockpit);
            }
            break;

            case TK_ViewExternalOrbit:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternalOrbit) ? cmExternalOrbit : cmCockpit);
            }
            break;

            case TK_ViewExternalStation:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternalStation) ? cmExternalStation : cmCockpit);
            }
            break;

            case TK_ViewFlyby:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternalFlyBy) ? cmExternalFlyBy : cmCockpit);
            }
            break;

            case TK_ViewMissile:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternalMissile) ? cmExternalMissile : cmCockpit);
            }
            break;

            case TK_ViewTarget:
            {
                if (trekClient.flyingF())
                {
                    if (GetCameraMode() != cmExternalTarget)
                        SetCameraMode(cmExternalTarget);
                    else
                        SetCameraMode(cmCockpit);
                }
            }
            break;

            case TK_ViewChase:
            {
                SetViewToCombatMode((GetCameraMode() != cmExternalChase) ? cmExternalChase : cmCockpit);
/*
                if (m_bUsePreferChaseView)
                {
                    // here we want to treat this as a sticky key press, not a toggle
                    m_bPreferChaseView = true;
                    SetViewToCombatMode (cmExternalChase);
                }
                else
                {
                    // here we just toggle the views
                    SetViewToCombatMode ((GetCameraMode () != cmExternalChase) ? cmExternalChase : cmCockpit);
                }
*/
            }
            break;
            /*
            // This camera mode is not actually handled by the viewing code
            case TK_ViewTurret:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternalTurret) ? cmExternalTurret : cmCockpit);
            }
            break;
            */

            case TK_ViewBase:
            {
                SetViewToCombatMode ((GetCameraMode () != cmExternalBase) ? cmExternalBase : cmCockpit);
            }
            break;

            case TK_ConModeGameState:
            {
                if (GetViewMode() != vmOverride && trekClient.m_fm.IsConnected())
                {
                    TurnOffOverlayFlags(c_omBanishablePanes & ~ofGameState);
                    ToggleOverlayFlags(ofGameState);
                }
            }
            break;

            case TK_ConModeNav:
            case TK_ViewSector:
            {
				
                if (GetViewMode() != vmOverride) {
                    ToggleOverlayFlags(ofSectorPane);

					// BT - 10/17 - Map and Sector Panes are now shown on launch and remember the pilots settings on last dock. 
					m_bShowSectorMapPane = GetOverlayFlags() & ofSectorPane == ofSectorPane;
                }
            }
            break;

            case TK_ViewCommand:
            case TK_ConModeCommand:
            {

                if (CommandCamera(m_cm))
                {
                    if (GetOverlayFlags() & c_omBanishablePanes) 
                        TurnOffOverlayFlags(c_omBanishablePanes);

                    if (!trekClient.GetShip()->IsGhost()) {
                        if (trekClient.flyingF()) {
                            SetViewMode(vmCombat);
                        } else {
                            SetViewMode(vmHangar);
                        }
                    }
                }
                else
                {
                    //Turn off overlay flags, as overlays get closed anyway. Without, their closing anim shows on closing command view.
                    if (GetOverlayFlags() & c_omBanishablePanes)
                        TurnOffOverlayFlags(c_omBanishablePanes);

                    if ((trekClient.GetViewCluster() == NULL) && !trekClient.flyingF())
                    {
                        IstationIGC*    pstation = trekClient.GetShip()->GetStation();
                        assert (pstation);

                        IclusterIGC*    pcluster = pstation->GetCluster();
                        assert (pcluster);
                        trekClient.RequestViewCluster(pcluster);
                        SetViewMode(vmCommand);
                    }
                    else
                    {
                        SetViewMode(vmCommand);
                        PositionCommandView(NULL, 2.0f);
                    }
                }
            }
            break;

            case TK_ToggleCommand:
            {
                if (m_cmPreviousCommand == cmExternalCommandViewTD)
                {
                    m_cmPreviousCommand = cmExternalCommandView34;
                }
                else if (m_cmPreviousCommand == cmExternalCommandView34)
                {
                    m_cmPreviousCommand = cmExternalCommandViewTD;
                }

                if (CommandCamera(m_cm))
                {
                    SetCameraMode(m_cmPreviousCommand);
                    PositionCommandView(NULL, 2.0f);
                }
            }
            break;

            case TK_CycleRadar: //imago updated 7/8/09
            {
                extern const char* c_szRadarLODs[];
				DWORD value = 0;
                if (m_cm == cmCockpit && GetViewMode() == vmCombat)
                {
                    value = m_radarCockpit = (m_radarCockpit + 1) % (int(RadarImage::c_rlMax) + 1);
                    m_pradarImage->SetRadarLOD((RadarImage::RadarLOD)m_radarCockpit);

                    trekClient.PostText(false, "%s", c_szRadarLODs[m_radarCockpit]);
                }
                else if (GetViewMode() == vmCommand)
                {
                    value = m_radarCommand = (m_radarCommand + 1) % (int(RadarImage::c_rlMax) + 1);
                    m_pradarImage->SetRadarLOD((RadarImage::RadarLOD)m_radarCommand);

                    trekClient.PostText(false, "%s", c_szRadarLODs[m_radarCommand]);
                }
				SavePreference("RadarLOD", value); //imago 7/8/09
            }
            break;

            case TK_ConModeCombat:
            {
                if (GetCameraMode() != cmCockpit)
                {
                    // save the old default state
                    bool    bOldPreference = m_bPreferChaseView;

                    // if we are using the prefer chase view option, then make
                    // chase view unsticky
                    m_bPreferChaseView = false;

                    // always set the view mode to cockpit
                    SetViewToCombatMode (cmCockpit);

                    // restore the old default state
                    m_bPreferChaseView = bOldPreference;
                }
                else
                {
                    SetViewToCombatMode(cmCockpit);
                }
            }
            break;

            case TK_ConModeInventory:
            {
                if (GetViewMode() == vmOverride) {
                        // do nothing
                } else if (trekClient.GetShip()->IsGhost()) {
                    TurnOffOverlayFlags(c_omBanishablePanes);
                } else if (trekClient.GetShip()->GetStation() != NULL) {
                    if (GetViewMode() != vmLoadout)
                        SetViewMode(vmLoadout);
                    else if (GetOverlayFlags() & c_omBanishablePanes)
                        TurnOffOverlayFlags(c_omBanishablePanes);
                    else
                        SetViewMode(vmHangar);
                }
				else
				{
					ToggleOverlayFlags(ofInventory);

					// BT - 10/17 - Map and Sector Panes are now shown on launch and remember the pilots settings on last dock. 
					m_bShowInventoryPane = GetOverlayFlags() & ofSectorPane == ofSectorPane;
				}
            }
            break;

            case TK_ConModeInvest:
            {

                if (GetViewMode() != vmOverride && trekClient.m_fm.IsConnected()
                    && trekClient.MyMission()->AllowDevelopments())
                {
                    TurnOffOverlayFlags(c_omBanishablePanes & ~ofInvestment);
                    ToggleOverlayFlags(ofInvestment);
                    m_bFreshInvestmentPane = (m_voverlaymask[m_viewmode] & ofInvestment);
                }
            }
            break;

            case TK_ConModeTeam:
            {

                TurnOffOverlayFlags(c_omBanishablePanes & ~(ofTeam | ofExpandedTeam));
                if (GetViewMode() != vmOverride)
                {
                    if (GetOverlayFlags() & ofTeam)
                    {
                        TurnOnOverlayFlags(ofExpandedTeam);
                        TurnOffOverlayFlags(ofTeam);
                    }
                    else if (GetOverlayFlags() & ofExpandedTeam)
                    {
                        TurnOffOverlayFlags(ofExpandedTeam);
                    }
                    else
                    {
                        TurnOnOverlayFlags(ofExpandedTeam);
                    }
                }
            }
            break;

            case TK_ConModeMiniTeam:
            {

                TurnOffOverlayFlags(c_omBanishablePanes & ~(ofTeam | ofExpandedTeam));
                if ((GetViewMode() != vmOverride) && trekClient.m_fm.IsConnected())
                {
                    if (GetOverlayFlags() & ofTeam)
                    {
                        TurnOffOverlayFlags(ofTeam);
                    }
                    else if (GetOverlayFlags() & ofExpandedTeam)
                    {
                        TurnOnOverlayFlags(ofTeam);
                        TurnOffOverlayFlags(ofExpandedTeam);
                    }
                    else
                    {
                        TurnOnOverlayFlags(ofTeam);
                    }
                    UpdateOverlayFlags();
                }
            }
            break;

            case TK_ConModeTeleport:
            {

                TurnOffOverlayFlags(c_omBanishablePanes & ~ofTeleportPane);
                if (trekClient.GetShip()->GetStation() && !trekClient.GetShip()->IsGhost())
                {
                    SetViewMode(vmHangar);

                    if (GetOverlayFlags() & ofTeleportPane)
                        TurnOffOverlayFlags(ofTeleportPane);
                    else
                        TurnOnOverlayFlags(ofTeleportPane);
                }
            }
            break;

            case TK_TargetHostile:  // target the thing that has done the most damage to me recently
            {
                DamageTrack*    pdt = trekClient.GetShip()->GetDamageTrack();
                assert (pdt);
                int n = pdt->GetDamageBuckets()->n();
                if (n > 0)
                {
                    DamageBucketLink* pdbl = pdt->GetDamageBuckets()->first();
                    assert (pdbl);

                    ImodelIGC*   pmodel = pdbl->data()->model();
                    if ((n > 1) && (trekClient.GetShip()->GetCommandTarget(c_cmdCurrent) == pmodel))
                    {
                        assert (pdbl->next());
                        pmodel = pdbl->next()->data()->model();
                    }

                    SetTarget(pmodel, c_cidDefault);
                }
                else
                {
                    TrekFindTarget((c_ttShip | c_ttEnemy | c_ttNearest), tk, TK_TargetEnemyNearest, TK_TargetEnemyPrev, c_habmRescue | c_habmMiner | c_habmBuilder);
                }
            }
            break;

            case TK_TargetCenter:   // target the ship closest to the reticle
            {
                if (trekClient.flyingF())
                {
                    const Orientation*  porientation = &(trekClient.GetShip()->GetOrientation());
                    IshipIGC*           pship = trekClient.GetShip()->GetParentShip();

                    if (pship)
                    {
                        Mount   turretID = trekClient.GetShip()->GetTurretID();
                        if (turretID == NA)
                            porientation = &(pship->GetOrientation());
                    }
                    else
                        pship = trekClient.GetShip();

                    ImodelIGC*  m = FindTarget(pship,
                                               (c_ttFront | c_ttAllTypes | c_ttAllSides),
                                               trekClient.GetShip()->GetCommandTarget(c_cmdCurrent),
                                               NULL, NULL,
                                               porientation);

                    SetTarget(m, c_cidDefault);
                }
            }
            break;

            case TK_TargetEnemy:
            case TK_TargetEnemyNearest:
            case TK_TargetEnemyPrev:
            {
                TrekFindTarget((c_ttShip | c_ttEnemy), tk, TK_TargetEnemyNearest, TK_TargetEnemyPrev, c_habmRescue | c_habmMiner | c_habmBuilder);
            }
            break;

            case TK_TargetEnemyBomber:
            case TK_TargetEnemyBomberNearest:
            case TK_TargetEnemyBomberPrev:
            {
                TrekFindTarget((c_ttShip | c_ttEnemy), tk, TK_TargetEnemyBomberNearest, TK_TargetEnemyBomberPrev, c_habmThreatToStation);
            }
            break;

            case TK_TargetEnemyFighter:
            case TK_TargetEnemyFighterNearest:
            case TK_TargetEnemyFighterPrev:
            {
                TrekFindTarget((c_ttShip | c_ttEnemy), tk, TK_TargetEnemyFighterNearest, TK_TargetEnemyFighterPrev, c_habmFighter);
            }
            break;

            case TK_TargetEnemyMiner:
            case TK_TargetEnemyMinerNearest:
            case TK_TargetEnemyMinerPrev:
            {
                TrekFindTarget((c_ttShip | c_ttEnemy), tk, TK_TargetEnemyMinerNearest, TK_TargetEnemyMinerPrev, c_habmMiner);
            }
            break;

            case TK_TargetEnemyConstructor:
            case TK_TargetEnemyConstructorNearest:
            case TK_TargetEnemyConstructorPrev:
            {
                TrekFindTarget((c_ttShip | c_ttEnemy), tk, TK_TargetEnemyConstructorNearest, TK_TargetEnemyConstructorPrev, c_habmBuilder);
            }
            break;

            case TK_TargetEnemyBase:
            case TK_TargetEnemyBaseNearest:
            case TK_TargetEnemyBasePrev:
            {
                TrekFindTarget((c_ttStation | c_ttEnemy | c_ttAnyCluster), tk, TK_TargetEnemyBaseNearest, TK_TargetEnemyBasePrev, ~c_sabmPedestal);
            }
            break;

            case TK_TargetEnemyMajorBase:
            case TK_TargetEnemyMajorBaseNearest:
            case TK_TargetEnemyMajorBasePrev:
            {
                TrekFindTarget((c_ttStation | c_ttEnemy | c_ttAnyCluster), tk, TK_TargetEnemyMajorBaseNearest, TK_TargetEnemyMajorBasePrev, c_sabmFlag);
            }
            break;

            case TK_TargetFriendlyBase:
            case TK_TargetFriendlyBaseNearest:
            case TK_TargetFriendlyBasePrev:
            {
                int             ttMask = (c_ttStation | c_ttFriendly | c_ttAnyCluster);
                AbilityBitMask  abm = c_sabmLand;

                {
                    IhullTypeIGC*   pht = trekClient.GetShip()->GetSourceShip()->GetBaseHullType();
                    if (pht)
                    {
                        HullAbilityBitMask  habm = pht->GetCapabilities();

                        if (habm & c_habmLifepod)
                        {
                            abm = c_sabmRescue | c_sabmRescueAny | c_sabmLand;
                            ttMask |= c_ttProbe;
                        }
                        else if ((habm & c_habmFighter) == 0)
                            abm = c_sabmCapLand;
                    }
                }

                TrekFindTarget(ttMask, tk, TK_TargetFriendlyBaseNearest, TK_TargetFriendlyBasePrev,
                               abm);
            }
            break;

            case TK_TargetAlliedBase:
            case TK_TargetAlliedBaseNearest:
            case TK_TargetAlliedBasePrev:
            {
                int             ttMask = (c_ttStation | c_ttFriendly | c_ttAnyCluster);
                AbilityBitMask  abm = c_sabmLand;

                {
                    IhullTypeIGC*   pht = trekClient.GetShip()->GetSourceShip()->GetBaseHullType();
                    if (pht)
                    {
                        HullAbilityBitMask  habm = pht->GetCapabilities();

                        if (habm & c_habmLifepod)
                        {
                            abm = c_sabmRescue | c_sabmRescueAny | c_sabmLand;
                            ttMask |= c_ttProbe;
                        }
                        else if ((habm & c_habmFighter) == 0)
                            abm = c_sabmCapLand;
                    }
                }

                TrekFindTarget(ttMask, tk, TK_TargetAlliedBaseNearest, TK_TargetAlliedBasePrev,
                               abm);
            }
            break;

            case TK_TargetFriendlyMajorBase:
            case TK_TargetFriendlyMajorBaseNearest:
            case TK_TargetFriendlyMajorBasePrev:
            {
                TrekFindTarget((c_ttStation | c_ttFriendly | c_ttAnyCluster), tk, TK_TargetFriendlyMajorBaseNearest, TK_TargetFriendlyMajorBasePrev, c_sabmFlag);
            }
            break;

            case TK_Target:
            case TK_TargetNearest:
            case TK_TargetPrev:
            {
                ImodelIGC*  pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);

                TrekFindTarget(pmodel && (pmodel != trekClient.GetShip())
                               ? GetSimilarTargetMask(pmodel)
                               : (c_ttShipTypes | c_ttEnemy),
                               tk, TK_TargetNearest, TK_TargetPrev);
            }
            break;

            case TK_TargetFriendly:
            case TK_TargetFriendlyNearest:
            case TK_TargetFriendlyPrev:
            {
                TrekFindTarget((c_ttShip | c_ttFriendly), tk, TK_TargetFriendlyNearest, TK_TargetFriendlyPrev);
            }
            break;

            case TK_TargetFriendlyBomber:
            case TK_TargetFriendlyBomberNearest:
            case TK_TargetFriendlyBomberPrev:
            {
                TrekFindTarget((c_ttShip | c_ttFriendly), tk, TK_TargetFriendlyBomberNearest, TK_TargetFriendlyBomberPrev, c_habmThreatToStation);
            }
            break;

            case TK_TargetFriendlyFighter:
            case TK_TargetFriendlyFighterNearest:
            case TK_TargetFriendlyFighterPrev:
            {
                TrekFindTarget((c_ttShip | c_ttFriendly), tk, TK_TargetFriendlyFighterNearest, TK_TargetFriendlyFighterPrev, c_habmFighter);
            }
            break;

            case TK_TargetFriendlyMiner:
            case TK_TargetFriendlyMinerNearest:
            case TK_TargetFriendlyMinerPrev:
            {
                TrekFindTarget((c_ttShip | c_ttFriendly), tk, TK_TargetFriendlyMinerNearest, TK_TargetFriendlyMinerPrev, c_habmMiner);
            }
            break;

            case TK_TargetFriendlyConstructor:
            case TK_TargetFriendlyConstructorNearest:
            case TK_TargetFriendlyConstructorPrev:
            {
                TrekFindTarget((c_ttShip | c_ttFriendly), tk, TK_TargetFriendlyConstructorNearest, TK_TargetFriendlyConstructorPrev, c_habmBuilder);
            }
            break;

            case TK_TargetFriendlyLifepod:
            case TK_TargetFriendlyLifepodNearest:
            case TK_TargetFriendlyLifepodPrev:
            {
                TrekFindTarget((c_ttShip | c_ttFriendly), tk, TK_TargetFriendlyLifepodNearest, TK_TargetFriendlyLifepodPrev, c_habmLifepod);
            }
            break;

            case TK_TargetArtifact:
            case TK_TargetArtifactNearest:
            case TK_TargetArtifactPrev:
            {
                TrekFindTarget((c_ttTreasure | c_ttNeutral), tk, TK_TargetArtifactNearest, TK_TargetArtifactPrev, 1);
            }
            break;

            case TK_TargetFlag:
            case TK_TargetFlagNearest:
            case TK_TargetFlagPrev:
            {
                TrekFindTarget((c_ttTreasure | c_ttEnemy), tk, TK_TargetFlagNearest, TK_TargetFlagPrev, 1);
            }
            break;

            case TK_TargetWarp:
            case TK_TargetWarpNearest:
            case TK_TargetWarpPrev:
            {
                TrekFindTarget((c_ttWarp | c_ttNeutral), tk, TK_TargetWarpNearest, TK_TargetWarpPrev);
            }
            break;

            case TK_ToggleAutoPilot:
            {
                if (trekClient.autoPilot())
                {
                    trekClient.SetAutoPilot(false);
                    SwitchToJoyThrottle();
                    trekClient.PlaySoundEffect(salAutopilotDisengageSound);
                }
                else if ((trekClient.GetShip()->GetParentShip() == NULL) && (trekClient.GetShip()->GetStation() == NULL))
                {
                    //m_timeAutopilotWarning = Time::Now();
                    trekClient.SetAutoPilot(true);
                    trekClient.bInitTrekJoyStick = true;
                    trekClient.PlaySoundEffect(salAutopilotEngageSound);
                }
            }
            break;

            case TK_VoteYes:
            {
                trekClient.Vote(true);
            }
            break;

            case TK_VoteNo:
            {
                trekClient.Vote(false);
            }
            break;

            case TK_ExecuteCommand:
            case TK_AcceptCommand:     // queued command becomes accepted command
            {
                AcceptQueuedCommand(tk == TK_ExecuteCommand);
            }
            break;

            case TK_RejectCommand:
            {
                RejectQueuedCommand(true, true);
            }
            break;

            case TK_ClearCommand:     // clear accepted command
            {
                SetAccepted(NULL, c_cidNone);
            }
            break;

            case TK_TargetCommand:
            {
                SetTarget(trekClient.GetShip()->GetCommandTarget(c_cmdAccepted),
                          trekClient.GetShip()->GetCommandID(c_cmdAccepted));
            }
            break;

            case TK_LODUp:
            {
                m_pscrollPane->PageUp();
            }
            break;

            case TK_LODDown:
            {
                m_pscrollPane->PageDown();
            }
            break;

            case TK_Suicide:
            {
                suicideF = true;
            }
            break;

            case TK_ToggleGrid:
            {
                m_bCommandGrid = !m_bCommandGrid;
				SavePreference("ShowGrid", m_bCommandGrid); //imago 7/8/09 #24
            }
            break;

            case TK_TargetSelf:
            {
                SetTarget(trekClient.GetShip(), c_cidNone);
            }
            break;

            case TK_TargetNothing:
            {
                SetTarget(NULL, c_cidNone);
            }
            break;

            case TK_OccupyNextTurret:
            {
                IshipIGC* pship = trekClient.GetShip()->GetParentShip();

                if (pship)
                {
                    trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
                    BEGIN_PFM_CREATE(trekClient.m_fm, pfmBoardShip, C, BOARD_SHIP)
                    END_PFM_CREATE;

                    pfmBoardShip->sidParent = pship->GetObjectID();
                }
            }
            break;

            //begin imago 8/14/09 mouse wheel
			
            case TK_ZoomOut:
            case TK_ZoomIn:
            {
                if (!m_ptrekInput->IsTrekKeyDown(TK_ZoomOut, true) && !m_ptrekInput->IsTrekKeyDown(TK_ZoomIn, true)) {
                    float dt = 0.1f;
                    if (CommandCamera(m_cm) && !m_pconsoleImage->DrawSelectionBox()) {
                        float delta = dt * m_distanceCommandCamera;
                        if (tk == TK_ZoomIn) {
                            m_distanceCommandCamera -= delta * 2.0f;
                            if (m_distanceCommandCamera < s_fCommandViewDistanceMin)
                                m_distanceCommandCamera = s_fCommandViewDistanceMin;
                        } else {
                            m_distanceCommandCamera += delta * 2.0f;
                            if (m_distanceCommandCamera > s_fCommandViewDistanceMax)
                                m_distanceCommandCamera = s_fCommandViewDistanceMax;
                        }
                    } else if (m_cm == cmExternalChase || !NoCameraControl(m_cm)) {
                        if (tk == TK_ZoomIn) {
                            m_distanceExternalCamera -= dt * m_distanceExternalCamera;
                            if (m_distanceExternalCamera < s_fExternalViewDistanceMin)
                                m_distanceExternalCamera = s_fExternalViewDistanceMin;
                        } else {
                            m_distanceExternalCamera += dt * m_distanceExternalCamera;
                            if (m_distanceExternalCamera > s_fExternalViewDistanceMax)
                                m_distanceExternalCamera = s_fExternalViewDistanceMax;
                        }
                    } 
					else if (m_cm == cmCockpit) {
                        float   fov = m_cameraControl.GetFOV();
                        if (tk == TK_ZoomIn) {
                            fov -= dt;
                            if (fov < s_fMinFOV)
                                fov = s_fMinFOV;
                            m_cameraControl.SetFOV(fov);
                        } else {
                            fov += dt;
                            if (fov > s_fMaxFOV)
                                fov = s_fMaxFOV;
                            m_cameraControl.SetFOV(fov);
                        }
                    }
					
                }
            }
            break;
			
            case TK_ThrottleUp:
            {
                if (trekClient.flyingF() && trekClient.GetShip() && !m_ptrekInput->IsTrekKeyDown(TK_ThrottleUp, true)) {
					trekClient.trekThrottle = (trekClient.trekThrottle < 0.8f) ? (trekClient.trekThrottle + 0.2f) : 1.0f;  //Imago matched orig values below - was 0.7 - 0.3 6/10 - cleaned up 7/10
					trekClient.joyThrottle = false; //#116 7/10 Imago
				}
            }
            break;

            case TK_ThrottleDown:
            {
                if (trekClient.flyingF() && trekClient.GetShip() && !m_ptrekInput->IsTrekKeyDown(TK_ThrottleDown, true)) {
					trekClient.trekThrottle = (trekClient.trekThrottle > -0.8f) ? (trekClient.trekThrottle - 0.2f) : -1.0f; //Imago matched orig values below - was 0.7 - 0.3 6/10 - cleaned up 7/10
					trekClient.joyThrottle = false; //#116 7/10 Imago
				}
            }
            break;
			
            // end imago

            case TK_DebugTest1:
            case TK_DebugTest2:
            case TK_DebugTest3:
            case TK_DebugTest4:
            case TK_DebugTest5:
            case TK_DebugTest6:
            case TK_DebugTest7:
            case TK_DebugTest8:
            case TK_DebugTest9:
            {
                // do nothing
            }
            break;

            default:
                return;
        }

        if (suicideF)
        {
            // 3 seconds to hit the sequence of keys
            if (m_suicideCount++ == 0)
                m_suicideTime = Time::Now();
            else if (Time::Now().clock() - m_suicideTime.clock() > 3000)
            {
                assert (m_suicideCount != 0);
                m_suicideCount = 0;
            }
            else if (m_suicideCount == 3)
            {
                m_suicideCount = 0;

                if (trekClient.m_fm.IsConnected())
                {
                    trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
                    BEGIN_PFM_CREATE(trekClient.m_fm, pfm, C, SUICIDE)
                    END_PFM_CREATE
                }
                else
                    trekClient.KillShipEvent(Time::Now(), trekClient.GetShip(), NULL, 1.0f, trekClient.GetShip()->GetPosition(), Vector::GetZero());
            }
        }
        else
        {
            m_suicideCount = 0;
        }
    }

    void    RejectQueuedCommand(bool    bClear = true, bool bNegative = false)
    {
        if (trekClient.GetLastMoneyRequest() != 0)
        {
            trekClient.SetLastMoneyRequest(0);

            IshipIGC*   pshipSender = trekClient.GetLastSender();
            if (pshipSender)
            {
                if (bNegative)
                {
                    if (pshipSender != trekClient.GetShip())
                    {
                        trekClient.SendChat(trekClient.GetShip(), CHAT_INDIVIDUAL, pshipSender->GetObjectID(),
                                            voNegativeSound, "Negative.");
                    }
                }
                trekClient.SetLastSender(NULL);
            }
        }
        else if (trekClient.GetShip()->GetCommandID(c_cmdQueued) != c_cidNone)
        {
            if (bClear)
                trekClient.GetShip()->SetCommand(c_cmdQueued, NULL, c_cidNone);

            IshipIGC*   pshipSender = trekClient.GetLastSender();
            if (pshipSender)
            {
                if (bNegative)
                {
                    if (pshipSender != trekClient.GetShip())
                    {
                        trekClient.SendChat(trekClient.GetShip(), CHAT_INDIVIDUAL, pshipSender->GetObjectID(),
                                            voNegativeSound, "Negative.");
                    }
                }
                trekClient.SetLastSender(NULL);
            }
        }

        m_timeRejectQueuedCommand = trekClient.m_now + (3600.0f * 24.0f * 20.0f);   //20 days in the future
    }


    void    AcceptQueuedCommand(bool    bExecute)
    {
        IshipIGC*   pshipSender = trekClient.GetLastSender();

        Money   request = trekClient.GetLastMoneyRequest();
        if (request != 0)
        {
            if (pshipSender && (pshipSender != trekClient.GetShip()))
            {
                Money   m = trekClient.GetMoney();
                if (request > m)
                    request = m;

                if (request > 0)
                {
                    PlayerInfo* pplayerSender = trekClient.FindPlayer(pshipSender->GetObjectID());
                    if (pplayerSender)
                        trekClient.DonateMoney(pplayerSender, request);
                }

                trekClient.SetLastMoneyRequest(0);
            }
        }
        else
        {
            CommandID   cid = trekClient.GetShip()->GetCommandID(c_cmdQueued);
            if (cid != c_cidNone)
            {
                ImodelIGC*  pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdQueued);
                if (pmodel && trekClient.GetShip()->LegalCommand(cid, pmodel))
                {
                    trekClient.PlaySoundEffect(acceptCommandSound);
                    if (cid == c_cidJoin)
                    {
                        if (pmodel->GetObjectType() == OT_ship)
                            trekClient.SetWing(((IshipIGC*)pmodel)->GetWingID());
                    }
                    else
                    {
                        if ((cid == c_cidPickup) &&
                            (pmodel->GetObjectType() == OT_ship) &&
                            trekClient.GetShip()->GetBaseHullType()->HasCapability(c_habmRescue))
                        {
                            //Tell the other person to pick me up as well
                            trekClient.SendChat(trekClient.GetShip(),
                                                CHAT_INDIVIDUAL,
                                                pmodel->GetObjectID(),
                                                NA,
                                                NULL,
                                                c_cidPickup,
                                                OT_ship,
                                                trekClient.GetShipID(),
                                                trekClient.GetShip());
                        }

                        SetTarget(pmodel, cid);
                        SetAccepted(pmodel, cid);

                        if (bExecute &&
                            (trekClient.GetShip()->GetStation() == NULL) &&
                            (trekClient.GetShip()->GetParentShip() == NULL) &&
                            trekClient.GetCluster(trekClient.GetShip(), pmodel))
                        {
                            trekClient.SetAutoPilot(true);
                            trekClient.bInitTrekJoyStick = true;
                            trekClient.PlaySoundEffect(salAutopilotEngageSound);
                        }
                    }

                    trekClient.GetShip()->SetCommand(c_cmdQueued, NULL, c_cidNone);
                }
            }
        }


        if (pshipSender)
        {
            if (pshipSender != trekClient.GetShip())
                trekClient.SendChat(trekClient.GetShip(), CHAT_INDIVIDUAL, pshipSender->GetObjectID(),
                                    voAffirmativeSound, "Affirmative.");
            trekClient.SetLastSender(NULL);
        }

        m_timeRejectQueuedCommand = trekClient.m_now + (3600.0f * 24.0f * 20.0f);   //20 days in the future
    }

    void    SetQueuedCommand(IshipIGC*  pshipSender,
                             CommandID  cid,
                             ImodelIGC* pmodelTarget)
    {
        if (pshipSender != trekClient.GetLastSender())
            RejectQueuedCommand();

        if (!Training::IsTraining ())
            m_timeRejectQueuedCommand = trekClient.m_now + c_dtRejectQueuedCommand;

        trekClient.SetLastSender(pshipSender);
        trekClient.SetLastMoneyRequest(0);
        trekClient.GetShip()->SetCommand(c_cmdQueued, pmodelTarget, cid);
    }

    void    SetQueuedCommand(IshipIGC*  pshipSender,
                             Money      money,
                             HullID     hid)
    {
        if (pshipSender != trekClient.GetLastSender())
            RejectQueuedCommand();

        m_timeRejectQueuedCommand = trekClient.m_now + c_dtRejectQueuedCommand;

        trekClient.SetLastSender(pshipSender);
        trekClient.SetLastMoneyRequest(money);
        trekClient.GetShip()->SetCommand(c_cmdQueued, NULL, c_cidNone);

        if ((pshipSender != trekClient.GetShip()) && GetWindow()->GetConsoleImage())
        {
            static const ZString c_str1(" has requested $");
            static const ZString c_str2(" to buy a ");
			static const ZString c_str3("  Press the [Insert] key to approve it.");

            assert (pshipSender);
            IhullTypeIGC*   pht = trekClient.m_pCoreIGC->GetHullType(hid);
            assert (pht);

            trekClient.PostText(true, "%s",
                                (const char*)((pshipSender->GetName() + c_str1) +
                                              (ZString(money) + c_str2) +
                                              (pht->GetName() + c_str3)));
        }
    }

    TRef<ChatListPane> m_pchatListPane;

    void SetChatListPane(ChatListPane* pchatListPane)
    {
        m_pchatListPane = pchatListPane;
    }

    void SetLobbyChatTarget(ChatTarget ct, ObjectID recip = NA)
    {
        m_ctLobbyChat = ct;
	//#8 Imago 7/10 added recip
		m_ctLobbyChatRecip = recip; 
    }
    ObjectID GetLobbyChatRecip()
    {
        return m_ctLobbyChatRecip;
    }
	//

    ChatTarget GetLobbyChatTarget()
    {
        return m_ctLobbyChat;
    }


    float MapJoystick(float value)
    {
        if (value > g_fJoystickDeadZone)
            value = (value - g_fJoystickDeadZone) / g_fInverseJoystickDeadZone;
        else if (value < -g_fJoystickDeadZone)
            value = (value + g_fJoystickDeadZone) / g_fInverseJoystickDeadZone;
        else
            value = 0.0f;

        if (!m_bLinearControls)
            value *= fabsf (value);

        return value;
    }

    void SwitchToJoyThrottle(void)
    {
        if (m_ptrekInput->IsAxisValid(3))
        {
            trekClient.joyThrottle = true;
        }
        else
        {
            // if the ship is currently thrusting in the same direction as the ship orientation (not actively slowing down)
            if (trekClient.GetShip()->GetEngineVector() * trekClient.GetShip()->GetOrientation().GetForward() > 0)
            {
                trekClient.trekThrottle = 1.0f;
            }
            else
            {
                trekClient.trekThrottle = -1.0f;
            }
        }
    }

    bool SenseJoystick(JoystickResults* js, bool bInternalCamera, bool bReadKeyboard, bool &bHandleThrottle) //Spunky #76
    {
        bool bThrottleChange = false;

        //
        // Read the joystick
        //

        if (m_pjoystickImage->GetJoystickEnabled()) {
            js->controls.jsValues[c_axisYaw  ] = m_pjoystickImage->GetValue(0)->GetValue();
            js->controls.jsValues[c_axisPitch] = m_pjoystickImage->GetValue(1)->GetValue();
            js->controls.jsValues[c_axisRoll ] = 0;
        } else {
            if (m_ptrekInput->IsTrekKeyDown(TK_RollModifier, bReadKeyboard)) {
                js->controls.jsValues[c_axisYaw ] = 0;
                js->controls.jsValues[c_axisRoll] = MapJoystick(m_ptrekInput->GetAxis(0));
            } else {
                js->controls.jsValues[c_axisYaw ] = MapJoystick(m_ptrekInput->GetAxis(0));
                js->controls.jsValues[c_axisRoll] = MapJoystick(m_ptrekInput->GetAxis(2));
            }

            js->controls.jsValues[c_axisPitch   ] =  MapJoystick(m_ptrekInput->GetAxis(1));
        }

        js->controls.jsValues[c_axisThrottle] = -m_ptrekInput->GetAxis(3);
        js->hat                               =  m_ptrekInput->GetAxis(4) * pi;

        //
        // Flip the y axis if requested
        //

        if (m_bFlipY) {
            js->controls.jsValues[c_axisPitch] = -js->controls.jsValues[c_axisPitch];
        }

        //
        // If the throttle system needs to be initialized, we do that. Otherwise, if the joystick
        // throttle has changed significantly, then we start using the joystick throttle.
        //

        if (trekClient.bInitTrekThrottle)
        {
            // see Training::StartMission for initialization of trekClient.trekThrottle
            // it is 1.0f for on-line play, and -1.0f for off-line.
            trekClient.joyThrottle = false;
            trekClient.fOldJoyThrottle = js->controls.jsValues[c_axisThrottle];
            trekClient.bInitTrekThrottle = false;
        }
        else if (fabs(js->controls.jsValues[c_axisThrottle] - trekClient.fOldJoyThrottle) > g_fJoystickDeadZone)
        {
            bThrottleChange = true;
            trekClient.joyThrottle = true;
        }

        //
        // Read buttons and keyboard
        //

        bool newButton1 = m_ptrekInput->IsTrekKeyDown(TK_FireWeapon , bReadKeyboard);
        bool newButton2 = m_ptrekInput->IsTrekKeyDown(TK_FireMissile, bReadKeyboard);
        bool newButton3 = m_ptrekInput->IsTrekKeyDown(TK_FireBooster, bReadKeyboard);
        //bool newButton4 = m_ptrekInput->GetButton(3); // !!! was next weapon
        //bool newButton5 = m_ptrekInput->GetButton(4); // !!! was vector lock
        bool newButton6 = m_ptrekInput->IsTrekKeyDown(TK_MatchSpeed , bReadKeyboard);

        if (bInternalCamera)
        {
            // roll left/right

            if (m_ptrekInput->IsTrekKeyDown(TK_RollRight, bReadKeyboard)) {
                js->controls.jsValues[c_axisRoll] = -1.0f;
            } else if (m_ptrekInput->IsTrekKeyDown(TK_RollLeft, bReadKeyboard)) {
                js->controls.jsValues[c_axisRoll] = 1.0f;
            }

            // yaw left/right

            if (m_ptrekInput->IsTrekKeyDown(TK_RollModifier, bReadKeyboard)) {
                if (m_ptrekInput->IsTrekKeyDown(TK_YawRight, bReadKeyboard)) {
                    js->controls.jsValues[c_axisRoll] = -1.0f;
                } else if (m_ptrekInput->IsTrekKeyDown(TK_YawLeft, bReadKeyboard)) {
                    js->controls.jsValues[c_axisRoll] = 1.0f;
                }
            } else {
                if (m_ptrekInput->IsTrekKeyDown(TK_YawRight, bReadKeyboard)) {
                    js->controls.jsValues[c_axisYaw] = -1.0f;
                } else if (m_ptrekInput->IsTrekKeyDown(TK_YawLeft, bReadKeyboard)) {
                    js->controls.jsValues[c_axisYaw] = 1.0f;
                }
            }

            // up/down == pitch

            if (m_ptrekInput->IsTrekKeyDown(TK_PitchDown, bReadKeyboard)) {
                js->controls.jsValues[c_axisPitch] = -1.0f;
            } else if (m_ptrekInput->IsTrekKeyDown(TK_PitchUp, bReadKeyboard)) {
                js->controls.jsValues[c_axisPitch] = 1.0f;
            }

            if (bHandleThrottle && m_ptrekInput->IsTrekKeyDown(TK_ThrottleUp, bReadKeyboard)) //Spunky #76
            {
                trekClient.trekThrottle =
                      (trekClient.trekThrottle < 0.8f)
                    ? (trekClient.trekThrottle + 0.2f)
                    : 1.0f;
                trekClient.joyThrottle = false;
                bThrottleChange = true;
				bHandleThrottle=false;
            }
            else if (bHandleThrottle && m_ptrekInput->IsTrekKeyDown(TK_ThrottleDown, bReadKeyboard)) //Spunky #76
            {
                trekClient.trekThrottle =
                      (trekClient.trekThrottle > -0.8f)
                    ? (trekClient.trekThrottle - 0.2f)
                    : -1.0f;
                trekClient.joyThrottle = false;
                bThrottleChange = true;
				bHandleThrottle=false;
            }
        }

        if (m_ptrekInput->IsTrekKeyDown(TK_ThrottleZero, bReadKeyboard))
        {
            trekClient.trekThrottle = -1.0f;
            trekClient.joyThrottle = false;
            bThrottleChange = true;
        }
        else if (m_ptrekInput->IsTrekKeyDown(TK_Throttle33, bReadKeyboard))
        {
            trekClient.trekThrottle = -0.3f;
            trekClient.joyThrottle = false;
            bThrottleChange = true;
        }
        else if (m_ptrekInput->IsTrekKeyDown(TK_Throttle66, bReadKeyboard))
        {
            trekClient.trekThrottle = 0.3f;
            trekClient.joyThrottle = false;
            bThrottleChange = true;
        }
        else if (m_ptrekInput->IsTrekKeyDown(TK_ThrottleFull, bReadKeyboard))
        {
            trekClient.trekThrottle = 1.0f;
            trekClient.joyThrottle = false;
            bThrottleChange = true;
        }

        //
        // Handle match speed
        //

        if (newButton6)
        {
            ImodelIGC* pmodel = GetCurrentTarget();
            if (pmodel)
            {
                trekClient.trekThrottle = GetThrottle(pmodel);
                trekClient.joyThrottle = false;
                bThrottleChange = true;
            }
        }

        if (trekClient.joyThrottle)
        {
            trekClient.fOldJoyThrottle = js->controls.jsValues[c_axisThrottle];
        }
        else
        {
            js->controls.jsValues[c_axisThrottle] = trekClient.trekThrottle;
        }

        // Process joystick buttons, keeping track of whether the button has changed
        // since the last poll.

        static bool oldButton1 = false;
        static bool oldButton2 = false;
        static bool oldButton3 = false;
        //static bool oldButton4 = false;
        //static bool oldButton5 = false;
        static bool oldButton6 = false;


        js->button1 = (newButton1 ? buttonDown : 0x00) | (newButton1 != oldButton1 ? buttonChanged : 0x00);
        js->button2 = (newButton2 ? buttonDown : 0x00) | (newButton2 != oldButton2 ? buttonChanged : 0x00);
        js->button3 = (newButton3 ? buttonDown : 0x00) | (newButton3 != oldButton3 ? buttonChanged : 0x00);
        //!!!js->button4 = (newButton4 ? buttonDown : 0x00) | (newButton4 != oldButton4 ? buttonChanged : 0x00);
        //!!!js->button5 = (newButton5 ? buttonDown : 0x00) | (newButton5 != oldButton5 ? buttonChanged : 0x00);
        js->button4 = 0;
        js->button5 = 0;
        js->button6 = (newButton6 ? buttonDown : 0x00) | (newButton6 != oldButton6 ? buttonChanged : 0x00);

        oldButton1 = newButton1;
        oldButton2 = newButton2;
        oldButton3 = newButton3;
        //oldButton4 = newButton4;
        //oldButton5 = newButton5;
        oldButton6 = newButton6;

        return bThrottleChange;
    }

    HRESULT HandleMsg(FEDMESSAGE* pfm, Time lastUpdate, Time now)
    {
        HRESULT hr = S_OK;  //handled by default

        switch(pfm->fmid)
        {
            case FM_S_LOGONACK:
            {
                CASTPFM(pfmLogonAck, S, LOGONACK, pfm);

                if (!pfmLogonAck->fValidated)
                    hr = S_FALSE; // don't do anything mission control will handle
            }
            break;

            case FM_S_GAME_OVER:
            {
                CASTPFM(pfmGameOver, S, GAME_OVER, pfm);
                trekClient.SetGameoverInfo(pfmGameOver);

                if (trekClient.GetSideID() != SIDE_TEAMLOBBY)
                    screen(ScreenIDGameOverScreen);
            }
            break;

            case FM_S_GAME_OVER_PLAYERS:
            {
                CASTPFM(pfmGameoverPlayers, S, GAME_OVER_PLAYERS, pfm);
                int nCount = pfmGameoverPlayers->cbrgPlayerInfo / sizeof(PlayerEndgameInfo);
                PlayerEndgameInfo* vPlayerEndgameInfo
                    = (PlayerEndgameInfo*)FM_VAR_REF(pfmGameoverPlayers, rgPlayerInfo);
                trekClient.AddGameoverPlayers(vPlayerEndgameInfo, nCount);
            }
            break;

            case FM_S_ICQ_CHAT_ACK:
            {
                CASTPFM(pfmICQChat, S, ICQ_CHAT_ACK, pfm);
                __try
                {
                  static bool fICQInit = false;
                  if (!fICQInit)
                  {
                    //ICQAPICall_SetLicenseKey("Microsoft", "hankukkk", "EDB699A39FE5BAE8");
                    fICQInit = true;
                  }
                  //ICQAPICall_SendMessage(pfmICQChat->icqid, FM_VAR_REF(pfmICQChat, Message));
                  //ICQAPICall_SendExternal(pfmICQChat->icqid, "", FM_VAR_REF(pfmICQChat, Message), true);
                }
                __except(DelayLoadDllExceptionFilter(GetExceptionInformation()))
                {
                  // nothing to do here
                }
            }
            break;

			// w0dk4 player-pings feature
			case FM_S_PINGDATA:
			{
				CASTPFM(pfmPingData, S, PINGDATA, pfm);
				if(pfmPingData->shipID != -1){
					PlayerInfo* playerInfo = trekClient.FindPlayer(pfmPingData->shipID);
					if(playerInfo)
						playerInfo->SetConnectionData(pfmPingData->ping,pfmPingData->loss);

				} else {
					if (pfmPingData->ping == 1)
						TrekWindowImpl::ShowPlayerPings();
				}
			}
			break;

            case FM_S_TREASURE_SETS:
            {
                // NYI
            }
            break;

            default:
            {
                hr = S_FALSE;
            }
        }

        return hr;
    }

    void TerminateModelEvent(ImodelIGC* model)
    {
        ObjectType    type = model->GetObjectType();
        if (type == OT_ship)
        {
            //A ship died ... do we need to do anything special?
            if (model == trekClient.GetShip())
            {
                //m_pconsoleImage->SetConsoleMode(c_cmNone);
            }
        }
    }


    void OnDelPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
    {
        if (pPlayerInfo->ShipID() == trekClient.GetShipID() && trekClient.GetSideID() == SIDE_TEAMLOBBY)
        {
            m_nLastCountdown = c_nCountdownMax;
        }
    }

    virtual void OnMissionCountdown(MissionInfo* pMissionDef)
    {
        m_nLastCountdown = c_nCountdownMax;
        trekClient.PlaySoundEffect(countdownStartingSound);
    }

    void CheckCountdownSound()
    {
        if (trekClient.MyMission()
            && (trekClient.MyMission()->GetStage() == STAGE_STARTING
                || (trekClient.MyMission()->GetStage() == STAGE_STARTED
                    && trekClient.MyMission()->GetMissionParams().IsCountdownGame())))
        {
            // note: have the timer lag by 1 second to give users the familiar countdown feel
            int nTimeLeft;

            if (trekClient.MyMission()->GetStage() == STAGE_STARTING)
                nTimeLeft = std::max(0, int(trekClient.MyMission()->GetMissionParams().timeStart - Time::Now()) + 1);
            else
                nTimeLeft = std::max(0, int(
                    trekClient.MyMission()->GetMissionParams().GetCountDownTime()
                        - (Time::Now() - trekClient.MyMission()->GetMissionParams().timeStart) + 1));

            if (nTimeLeft != m_nLastCountdown)
            {
                m_nLastCountdown = nTimeLeft;

                switch (m_nLastCountdown)
                {
                case 10 * 60:
                    trekClient.PlaySoundEffect(countdown10minSound);
                    break;

                case 5 * 60:
                    trekClient.PlaySoundEffect(countdown5minSound);
                    break;

                case 1 * 60:
                    trekClient.PlaySoundEffect(countdown1minSound);
                    break;

                case 30:
                    trekClient.PlaySoundEffect(countdown30Sound);
                    break;

                case 10:
                    trekClient.PlaySoundEffect(countdown10Sound);
                    break;

                case 9:
                    trekClient.PlaySoundEffect(countdown9Sound);
                    break;

                case 8:
                    trekClient.PlaySoundEffect(countdown8Sound);
                    break;

                case 7:
                    trekClient.PlaySoundEffect(countdown7Sound);
                    break;

                case 6:
                    trekClient.PlaySoundEffect(countdown6Sound);
                    break;

                case 5:
                    trekClient.PlaySoundEffect(countdown5Sound);
                    break;

                case 4:
                    trekClient.PlaySoundEffect(countdown4Sound);
                    break;

                case 3:
                    trekClient.PlaySoundEffect(countdown3Sound);
                    break;

                case 2:
                    trekClient.PlaySoundEffect(countdown2Sound);
                    break;

                case 1:
                    trekClient.PlaySoundEffect(countdown1Sound);
                    break;
                }
            }
        }
    }

    void OnChatMessageChange()
    {
        this->ChangeChatMessage();
    }

    virtual bool OnOK(int result)
    {
        assert(result == IDOK);
        return false;
    }

    //
    // VT functions.
    //

    VOID VTSetText(LPSTR szFormat, ...)
    {
        TCHAR szText[1024];
        va_list vaList;
        INT cchText;

        if (NULL != m_hwndVTEdit)
        {
            cchText = sprintf(szText, "Version=%d,", ++m_cVTVersion);
            va_start(vaList, szFormat);
            vsprintf(szText + cchText, szFormat, vaList);
            va_end(vaList);

            ::SetWindowText(m_hwndVTEdit, szText);
        }
    }

    void  SoundEngineUpdate (void)
    {
        ZSucceeded(m_pSoundEngine->Update ());
    }

    TRef<ISoundInstance>  StartSound(ISoundTemplate* ptemplate, ISoundPositionSource* psource = NULL)
    {
        if (ptemplate)
        {
            TRef<ISoundInstance> pSoundInstance;

            if (ZSucceeded(ptemplate->CreateSound(pSoundInstance, m_pSoundEngine->GetBufferSource(), psource)))
            {
                return pSoundInstance;
            }
        }

        {
            // may avoid some art crashes in retail builds
            return NULL;
        }
    };

    TRef<ISoundInstance>  StartSound(SoundID soundId, ISoundPositionSource* psource = NULL)
    {
        if (soundId < 0 || soundId > m_vSoundMap.GetCount())
        {
            // may avoid some art crashes in retail builds
            ZAssert(false);
            return NULL;
        }

        return StartSound(m_vSoundMap[soundId], psource);
    };

    void      StartLockDown(const ZString& strReason)
    {
        ZAssert(m_pmessageBoxLockdown == NULL || trekClient.IsLockedDown());

        if (m_pmessageBoxLockdown)
            GetPopupContainer()->ClosePopup(m_pmessageBoxLockdown);

        if ((GetViewMode() != vmOverride) && (GetViewMode() != vmUI))
        {
            m_pmessageBoxLockdown = CreateMessageBox(strReason, NULL, false, false, 1.0f);
            GetWindow()->SetWaitCursor();
            GetPopupContainer()->OpenPopup(m_pmessageBoxLockdown, false);
        }
    }

    void      EndLockDown()
    {
        if (!trekClient.IsLockedDown())
        {
            GetPopupContainer()->ClosePopup(m_pmessageBoxLockdown);
            GetWindow()->RestoreCursor();
            m_pmessageBoxLockdown = NULL;
        }
    }
};

TrekWindowImpl::ShipWinConditionInfo           TrekWindowImpl::s_shipWinConditionInfo;
TrekWindowImpl::DeathMatchWinConditionInfo     TrekWindowImpl::s_deathMatchWinConditionInfo;
TrekWindowImpl::DeathMatchShipWinConditionInfo TrekWindowImpl::s_deathMatchShipWinConditionInfo;
TrekWindowImpl::ConquestWinConditionInfo       TrekWindowImpl::s_conquestWinConditionInfo;
TrekWindowImpl::TerritoryWinConditionInfo      TrekWindowImpl::s_territoryWinConditionInfo;
TrekWindowImpl::ProsperityWinConditionInfo     TrekWindowImpl::s_prosperityWinConditionInfo;
TrekWindowImpl::ArtifactsWinConditionInfo      TrekWindowImpl::s_artifactsWinConditionInfo;
TrekWindowImpl::FlagsWinConditionInfo          TrekWindowImpl::s_flagsWinConditionInfo;

TRef<TrekWindow> TrekWindow::Create(
    EffectApp*     papp,
    const ZString& strCommandLine,
// BUILD_DX9
	const ZString& strArtPath,					// Added for DX9 build, due to reordered startup.
// BUILD_DX9
    bool           bMovies
) {
    return
        new TrekWindowImpl(
            papp,
            strCommandLine,
// BUILD_DX9
			strArtPath,
// BUILD_DX9
            bMovies
        );
}

// forward squad message to squad screen
void ForwardSquadMessage(FEDMESSAGE *pSquadMessage);

void WinTrekClient::ForwardSquadMessage(FEDMESSAGE * pSquadMessage)
{
    ::ForwardSquadMessage(pSquadMessage);
}

// forward CharacterInfo message to CharacterInfo screen
void ForwardCharInfoMessage(FEDMESSAGE *pCharInfoMessage);

void WinTrekClient::ForwardCharInfoMessage(FEDMESSAGE * pCharInfoMessage)
{
    ::ForwardCharInfoMessage(pCharInfoMessage);
}

// forward LeaderBoard message to the LeaderBoard screen
void ForwardLeaderBoardMessage(FEDMESSAGE *pCharInfoMessage);

void WinTrekClient::ForwardLeaderBoardMessage(FEDMESSAGE * pLeaderBoardMessage)
{
    ::ForwardLeaderBoardMessage(pLeaderBoardMessage);
}
