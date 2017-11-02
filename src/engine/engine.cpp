#include "pch.h"

HWND g_hwndMainWindow;

CDX9EngineSettings g_DX9Settings;


//////////////////////////////////////////////////////////////////////////////
//
// Engine Implementation
//
//////////////////////////////////////////////////////////////////////////////

class EngineImpl : public PrivateEngine {
private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // types
    //
    //////////////////////////////////////////////////////////////////////////////

    typedef TList<PrivateSurface*>  SurfaceList;
    //typedef TList<DeviceDependant*> DeviceDependantList;
	TVector<Vector>   m_modes; //imago 6/24/09 7/1/09 added refresh rate

    //////////////////////////////////////////////////////////////////////////////
    //
    // members
    //
    //////////////////////////////////////////////////////////////////////////////

    //
    // State
    //

    bool                      m_bValid;
    bool                      m_bValidDevice;
	bool					  m_bChanged; //imago 7/7/09
    bool                      m_bAllowSecondary;
    bool                      m_bAllow3DAcceleration;
    bool                      m_b3DAccelerationImportant;
	bool					  m_bMipMapGenerationEnabled;
    DWORD                     m_dwBPP; // KGJV 32B - user choosen bpp or desktop bbp

	//
    //
    //

    TRef<PixelFormat>         m_ppf;
    HWND                      m_hwndClip;
    WinPoint                  m_pointPrimary;
    HWND                      m_hwndFocus;

	TRef<ModifiableWinPointValue>	m_sizeResolution;
	TRef<ModifiableBoolean>		m_bFullscreen;
	WinPoint					m_pointFullscreenCurrent;
//    TRef<PrivateSurface>      m_psurfaceBack;
    float                     m_gamma;

    //
    // Surface Cache
    //

    //DeviceDependantList       m_listDeviceDependant;
    SurfaceList               m_listSurfaces;
    SurfaceList               m_listDeviceFormatSurfaces;


    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

public:
    EngineImpl(bool bAllow3DAcceleration, bool bAllowSecondary, DWORD dwBPP, HWND hWindow) :
        m_sizeResolution(new ModifiableWinPointValue(WinPoint(
			int(CD3DDevice9::Get()->GetDeviceSetupParams()->sFullScreenMode.mode.Width), 
			int(CD3DDevice9::Get()->GetDeviceSetupParams()->sFullScreenMode.mode.Height)
		))),
        m_pointFullscreenCurrent(0, 0),
        m_bFullscreen(new ModifiableBoolean(false)),
        m_bAllow3DAcceleration(bAllow3DAcceleration),
        m_bAllowSecondary(bAllowSecondary),
        m_b3DAccelerationImportant(false),
        m_bValid(false),
        m_bValidDevice(false),
        m_hwndFocus(NULL),
        m_hwndClip(NULL),
        m_gamma(1.0f),
        m_dwBPP(dwBPP), // KGJV 32B
		m_bMipMapGenerationEnabled( false )
    {
		// Create the D3D device first up.
/*		m_pD3DDevice = CreateD3DDevice( hWindow );
	
		// Check it was ok.
		if( !m_pD3DDevice->IsValid() )
		{
            // !!! replace with a ZErrorHandler call - take from original ddraw code.
            ::MessageBox( NULL, "Unable to create Direct 3D Device.\n"
				                "Please check www.alleg.net for help.",
								"Initialization Error", MB_ICONEXCLAMATION | MB_OK );
            return;
		}

        // Get the primary device
/*       m_pdddevicePrimary = CreateDDDevice(this, m_bAllow3DAcceleration, hWindow );

        if (!m_pdddevicePrimary->IsValid()) {
            // !!! replace with a ZErrorHandler call

            ::MessageBox(
                NULL,
                "Unable to create primary DirectDraw Device.\n"
                "Please update your DirectX installation",
                "Initialization Error",
                MB_ICONEXCLAMATION | MB_OK
            );
            return;
        }

        //
        // Search for other devices with 3D support for fullscreen
        //

        DDCall(DirectDrawEnumerate(StaticDDDeviceCallback, this));

        //
        // Start on the primary device
        //

        m_pdddevice = m_pdddevicePrimary;*/

        //
        // Create a default pixel format
        //

        // KGJV 32B - set PixelFormat according to bpp - imago, force 32? would make things simpler...
        
/*        if (m_dwBPP == 0)
        {
            // fetch the desktop bpp
            DDSDescription ddsd;
            DDCall(m_pdddevicePrimary->GetDD()->GetDisplayMode(&ddsd));
            m_dwBPP = ddsd.ddpfPixelFormat.dwRGBBitCount;
            if (m_dwBPP != 32) m_dwBPP = 16; // fallback to 16 if desktop bpp isnt 32
        }*/
		if( ( CD3DDevice9::Get()->GetCurrentMode()->mode.Format == D3DFMT_A8B8G8R8 ) ||
			( CD3DDevice9::Get()->GetCurrentMode()->mode.Format == D3DFMT_A8R8G8B8 ) ||
			( CD3DDevice9::Get()->GetCurrentMode()->mode.Format == D3DFMT_X8B8G8R8 ) ||
			( CD3DDevice9::Get()->GetCurrentMode()->mode.Format == D3DFMT_X8R8G8B8 ) )
		{
			m_ppf = new PixelFormat(32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000); //Fix memory leak -Imago 8/2/09
			m_dwBPP = 32;
		}
		else
		{
			m_ppf = new PixelFormat(16, 0xf800, 0x07e0, 0x001f, 0x0000);
			m_dwBPP = 16;
		}
           

		//imago 6/24/09 - 6/29/09 - 7/1/09
#pragma warning(disable:4244)
		int i; int width; int height; int rate;
		for( i=0; i<CD3DDevice9::Get()->GetDeviceSetupParams()->iNumRes; i++ ) //8/2/09 fixed invalid memory access
		{
			width = CD3DDevice9::Get()->GetDeviceSetupParams()->pFullScreenResArray[i].iWidth;
			height = CD3DDevice9::Get()->GetDeviceSetupParams()->pFullScreenResArray[i].iHeight;
			rate = CD3DDevice9::Get()->GetDeviceSetupParams()->pFullScreenResArray[i].iFreq;
			//OutputDebugString("Found valid full screen rez: "+ZString(width)+"x"+ZString(height)+" @ "+ZString(rate));
			if (width == 640 && height == 480)
				m_modes.PushEnd((Vector(width,height,rate))); //VGA				
			if (width == 800 && height == 600)
				m_modes.PushEnd((Vector(width,height,rate))); //SVGA (default)	
			if (width == 1024 && height == 768)
				m_modes.PushEnd((Vector(width,height,rate))); //XGA
			if (width == 1280 && height == 1024)
				m_modes.PushEnd((Vector(width,height,rate))); //SXGA			
			if (width == 1366 && height == 768)
				m_modes.PushEnd((Vector(width,height,rate))); //WXGA (720p widescreen mode)
			if (width == 1400 && height == 1050)
				m_modes.PushEnd((Vector(width,height,rate))); //SXGA+
			if (width == 1440 && height == 900)
				m_modes.PushEnd((Vector(width,height,rate))); //WSXGA+ (widescreen)
			if (width == 1600 && height == 1200)
				m_modes.PushEnd((Vector(width,height,rate))); //UXGA
			if (width == 1680 && height == 1050)
				m_modes.PushEnd((Vector(width,height,rate))); //WSXGA+ (widescreen)
			if (width == 1920 && height == 1080)
				m_modes.PushEnd((Vector(width,height,rate))); //WUXGA (1080p widescreen mode)
		}
#pragma warning(default:4244)
    }

private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Destructor
    //
    //////////////////////////////////////////////////////////////////////////////

    ~EngineImpl()
    {
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Terminate Dependants
    //
    //////////////////////////////////////////////////////////////////////////////

//    void ClearDependants()
//    {
///*        {
//            DeviceDependantList::Iterator iter(m_listDeviceDependant);
//
//            while (!iter.End()) {
//                iter.Value()->ClearDevice();
//                iter.Next();
//            }
//        }*/
//
//        {
//            SurfaceList::Iterator iter(m_listSurfaces);
//
//            while (!iter.End()) {
//                iter.Value()->ClearDevice();
//                iter.Next();
//            }
//        }
//    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Device Termination  this function apparently DOES NOTHING --imago 6/29/09
    //
    //////////////////////////////////////////////////////////////////////////////

    void TerminateDevice()
    {
        //ClearDependants();

 //       m_hwndClip     = NULL;
//        m_psurfaceBack = NULL;
//        m_pddClipper   = NULL;
//        m_pdds         = NULL;

//        m_pdddevice->FreeEverything();
//        m_pdddevice = NULL;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Termination
    //
    //////////////////////////////////////////////////////////////////////////////

    void TerminateEngine( bool bEngineAppTerminate /*=false*/)
    {
        //ClearDependants();

        m_hwndClip       = NULL;
//        m_psurfaceBack   = NULL;
		
		// Reset D3D device.
		if( bEngineAppTerminate == true )
		{
			CD3DDevice9::Get()->Shutdown();
		}
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Validation
    //
    //////////////////////////////////////////////////////////////////////////////

    bool IsValid()
    {
		return CD3DDevice9::Get()->IsDeviceValid();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetFocusWindow(Window* pwindow, bool bStartFullscreen)
    {
        //
        // This function can only be called once
        //

        ZAssert(m_hwndFocus == NULL && pwindow->GetHWND() != NULL);
        ZAssert(!m_bValid);
		//OutputDebugString("\n\nCalling SetFocusWindow() ONLY SUPPOSED TO HAPPEN ONCE!\n");

        m_hwndFocus			= pwindow->GetHWND();
		m_bFullscreen->SetValue(bStartFullscreen);
		g_hwndMainWindow	= m_hwndFocus;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Device Initialization
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateSurfacesPixelFormat()
    {
        //
        // Tell all the Device format surfaces their new pixel format
        //

        {
            SurfaceList::Iterator iterSurface(m_listDeviceFormatSurfaces);

            while (!iterSurface.End()) {
                iterSurface.Value()->SetPixelFormat(m_ppf);
                iterSurface.Next();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // SetGammaRamp
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetGammaRamp()
    {
		if( CD3DDevice9::Get()->IsDeviceValid() == true )
		{
            D3DGAMMARAMP gammaRamp;

            for (int index = 0; index < 256; index ++) 
			{
                float value  = (float)index / 255;
                float level  = pow(value, 1.0f / m_gamma);
                //float level  = (m_gamma - 1) + (1 - (m_gamma - 1)) * value;
                int   ilevel = MakeInt(level * 65535.0f);

                gammaRamp.red  [index] = ilevel;
                gammaRamp.green[index] = ilevel;
                gammaRamp.blue [index] = ilevel;
            };

            //
            // zero is always black
            //

            gammaRamp.red  [0] = 0;
            gammaRamp.green[0] = 0;
            gammaRamp.blue [0] = 0;

			CD3DDevice9::Get()->SetGammaRamp(0, D3DSGR_CALIBRATE, &gammaRamp);
        }
    }

    void SetGammaLevel(float value)
    {
        m_gamma = bound(value, 1.0f, 2.0f);
        SetGammaRamp();
    }

    float GetGammaLevel()
    {
        return m_gamma;
    }


    //////////////////////////////////////////////////////////////////////////////
    //
    // Create Primary Surface
    //
    //////////////////////////////////////////////////////////////////////////////

    bool CreatePrimarySurface()
    {
		// Update the device with a call to IDirect3DDevice9::Reset().
        return true;
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    // Switch to windowed
    //
    //////////////////////////////////////////////////////////////////////////////

    bool InitializeWindowed()
    {
        if (g_bWindowLog) {
            ZDebugOutput("InitializeWindowed\n");
        }

        //
        // If we were fullscreen go back to windowed mode
/*        if (m_pdddeviceFullscreen != NULL) 
		{
            if (g_bWindowLog) 
			{
                ZDebugOutput("SetCooperativeLevel(Normal)\n");
            }
            DDCall(m_pdddeviceFullscreen->GetDD()->SetCooperativeLevel(NULL, DDSCL_NORMAL));
            m_pdddeviceFullscreen = NULL;
        }*/

        //
        // Free up all the device specific objects
        //

        TerminateDevice();

        //
        // switch to the windowed device
		//
        m_pointFullscreenCurrent = WinPoint(0, 0);

        //
        // Get the primary surface
        //

        if (!CreatePrimarySurface()) {
            return false;
        }

        //
        // If the primary surface isn't 16bpp go to fullscreen automatically
        //

        // KGJV 32B : if game bpp != desktop bpp go fullscreen
//        if (m_ppf->PixelBits() != m_dwBPP) {
  //          m_bFullscreen = true;
    //        return false;
      //  }

        //
        // Update any device format surfaces
//        UpdateSurfacesPixelFormat();

		// Reset the device for windowed mode.

		CD3DDevice9::Get()->ResetDevice( true);

        if (g_bWindowLog) {
            ZDebugOutput("InitializeWindowed exiting\n");
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Create Fullscreen Surface
    //
    //////////////////////////////////////////////////////////////////////////////

/*    bool CreateFullscreenSurface(DDDevice* pdddevice, bool& bError)
    {
        bError = false;
        HRESULT hr;

        //
        // Create a double buffered surface
        //

        DDSDescription ddsd;

        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

        ddsd.ddsCaps.dwCaps =
              DDSCAPS_PRIMARYSURFACE
            | DDSCAPS_FLIP
            | DDSCAPS_COMPLEX
            | DDSCAPS_3DDEVICE;

        ddsd.dwBackBufferCount = 1;

        hr = pdddevice->GetDD()->CreateSurface(&ddsd, &m_pdds, NULL);

        if (hr == DDERR_OUTOFVIDEOMEMORY) {
            ZAssert(m_pdds == NULL);
            if (g_bWindowLog) {
                ZDebugOutput("Not enough memory for primary surface\n");
            }
            return false;
        }
        if (hr == DDERR_NOEXCLUSIVEMODE) {
            ZAssert(m_pdds == NULL);
            if (g_bWindowLog) {
                ZDebugOutput("Exclusive mode was lost\n");
            }
            bError = true;
            return false;
        }
        if (hr == DDERR_UNSUPPORTEDMODE) {
            ZAssert(m_pdds == NULL);
            if (g_bWindowLog) {
                ZDebugOutput("Unsupported mode\n");
            }
            bError = true;
            return false;
        }
        DDCall(hr);

        if (m_pdds == NULL) {
            return false;
        }

        //
        // Create a clipper for the surface
        //

        DDCall(pdddevice->GetDD()->CreateClipper(0, &m_pddClipper, NULL));
        DDCall(m_pdds->SetClipper(m_pddClipper));


        //
        // Update the gamma ramp
        //

        SetGammaRamp();

        //
        // Get the pixel format
        //

        DDCall(m_pdds->GetSurfaceDesc(&ddsd));

        m_ppf = GetPixelFormat(ddsd.GetPixelFormat());

        //
        // Create the ZBuffer
        TRef<IDirectDrawSurfaceX> pddsZBuffer;

        if (pdddevice->GetAllow3DAcceleration()) {
            ddsd.dwFlags         = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS | DDSD_PIXELFORMAT;
            ddsd.dwWidth         = ddsd.dwWidth;
            ddsd.dwHeight        = ddsd.dwHeight;
            ddsd.ddsCaps.dwCaps  = DDSCAPS_ZBUFFER;
            ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
            ddsd.ddpfPixelFormat = pdddevice->GetZBufferPixelFormat()->GetDDPF();

            hr = pdddevice->GetDD()->CreateSurface(&ddsd, &pddsZBuffer, NULL);

            if (hr == DDERR_OUTOFVIDEOMEMORY) {
                if (g_bWindowLog) {
                    ZDebugOutput("Not enough memory for ZBuffer\n");
                }
                m_pdds = NULL;
                return false;
            }
        
            if (hr == DDERR_NOZBUFFERHW) {
                if (g_bWindowLog) {
                    ZDebugOutput("Device doesn't support ZBuffers\n");
                }

                m_pdds = NULL;
                return false;
            }

            DDCall(hr);
        }*/

        //
        // Get the back buffer
/*        DDSCaps caps;
        caps.dwCaps = DDSCAPS_BACKBUFFER;

        TRef<IDirectDrawSurfaceX> pddsBack;
        DDCall(m_pdds->GetAttachedSurface(&caps, &pddsBack));

        if (pddsBack == NULL) {
            m_pdds = NULL;
            return false;
        }

        //
        // Attach the ZBuffer to the back buffer
        //

        if (pddsZBuffer) {
            if (FAILED(pddsBack->AddAttachedSurface(pddsZBuffer))) {
                m_pdds = NULL;
                return false;
            }
        }

        //
        // Create a surface wrapper
        //

        SurfaceType stype = SurfaceType2D() | SurfaceType3D() | SurfaceTypeZBuffer() | SurfaceTypeVideo();

        m_psurfaceBack =
            CreatePrivateSurface(
                this,
                CreateDDSurface(
                    pdddevice, 
                    pddsBack,
                    pddsZBuffer,
                    m_ppf,
                    NULL,
                    stype
                ),
                NULL
            );
        
        //
        // Fill the surface with black
        //

        m_psurfaceBack->FillSurface(Color::Black());
        Flip();
        m_psurfaceBack->FillSurface(Color::Black());

        return true;
    }*/

    //////////////////////////////////////////////////////////////////////////////
    //
    // Switch to a device at a certain resolution
    //
    //////////////////////////////////////////////////////////////////////////////

    bool SwitchToFullscreenDevice(int iRate, const WinPoint& size, bool& bError)  //imago is iRate ;-) 7/1/09
    {
        bError = false;

        if (g_bWindowLog) 
		{
            //ZDebugOutput( "SwitchToFullscreenDevice( " + pdddevice->GetName() + ", resolution: " + GetString(size) + ")\n" );
			ZDebugOutput( "SwitchToFullscreenDevice\n" );
        }

 /*       // If switching to a different device go to normal mode
        if (m_pdddeviceFullscreen != NULL && m_pdddeviceFullscreen != pdddevice) 
		{
            if (g_bWindowLog) 
			{
                ZDebugOutput("SetCooperativeLevel(" + pdddevice->GetName() + ", Normal)\n");
            }
            DDCall(m_pdddeviceFullscreen->GetDD()->SetCooperativeLevel(NULL, DDSCL_NORMAL));
            m_pdddeviceFullscreen = NULL;
        }*/

        //
        // Free up all the device specific objects
        //

        TerminateDevice();
/*      m_pdddevice = pdddevice;

        //
        // If this is a new fullscreen device go to exclusive mode
        //

        if (m_pdddeviceFullscreen != pdddevice) {
            if (g_bWindowLog) {
                ZDebugOutput("SetCooperativeLevel(" + pdddevice->GetName() + ", Exclusive)\n");
            }

            HRESULT hr =
                pdddevice->GetDD()->SetCooperativeLevel(
                    m_hwndFocus,
                      DDSCL_EXCLUSIVE
                    | DDSCL_FULLSCREEN
                );

            if (hr == DDERR_EXCLUSIVEMODEALREADYSET) {
                if (g_bWindowLog) {
                    ZDebugOutput("Can't set exclusive mode\n");
                }
                bError = true;
                return false;
            }

            DDCall(hr);

            m_pdddeviceFullscreen = pdddevice;
        }

        //
        // Switch resolutions
        //

        HRESULT hr =
            m_pdddevice->GetDD()->SetDisplayMode(
                size.X(),
                size.Y(),
                m_dwBPP, // KGJV 32B - set as parameter
                0,
                0
            );

        if (
               hr == DDERR_NOEXCLUSIVEMODE
            || hr == DDERR_UNSUPPORTED
        ) {
            if (g_bWindowLog) {
                ZDebugOutput("Error setting display mode\n");
            }
            bError = true;
            return false;
        }

        if (hr == DDERR_INVALIDMODE) {
            pdddevice->EliminateModes(size);
            if (g_bWindowLog) {
                ZDebugOutput("Invalid resolution\n");
            }
            return false;
        }

        DDCall(hr);

        //
        // Create the primary surface and back buffer
        //

        if (!CreateFullscreenSurface(pdddevice, bError)) {
            return false;
        }

        //
        // Everything worked.  Update any device format surfaces.
        //
        
        UpdateSurfacesPixelFormat();
		
*/

		// TBD: SET TRUE TO FALSE.
//		CD3DDevice9::ResetDevice( TRUE, size.X(), size.Y() );

		//imago added eliminate modes
		if (CD3DDevice9::Get()->ResetDevice( false, size.X(), size.Y(), g_DX9Settings.m_refreshrate ) != D3D_OK) {
			EliminateModes(Vector((float)size.X(),(float)size.Y(),(float)g_DX9Settings.m_refreshrate));
			if (g_bWindowLog) {
				ZDebugOutput("Invalid resolution\n");
			}
			//auto retry next mode untill end of list NYI
		}

        if (g_bWindowLog) {
            ZDebugOutput("SwitchToFullscreenDevice exiting\n");
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Switch to full screen
    //
    //////////////////////////////////////////////////////////////////////////////

    bool InitializeFullscreen(bool& bChanges)
    {
        if (g_bWindowLog) {
            ZDebugOutput("InitalizeFullscreen()\n");
        }

        //
        // Try the secondary device first
        //

//        DDDevice* pdddevice;

/*        if (
               m_bAllowSecondary 
            && m_bAllow3DAcceleration
            && m_b3DAccelerationImportant
            && m_pdddeviceSecondary != NULL
            && m_pdddeviceSecondary->GetAllow3DAcceleration()
        ) {
            pdddevice = m_pdddeviceSecondary;
        } else {
            pdddevice = m_pdddevicePrimary;
        }*/

        //
        // Don't do anything if we don't need to change the device
        // or resolution
        //

/*        if (  
               m_bValidDevice
            && m_pdddevice              == pdddevice 
            && m_pointFullscreenCurrent == m_pointFullscreen
        ) {
            ZDebugOutput("Device and resolution match\n");
            return true;
        }*/

		if( ( CD3DDevice9::Get()->IsDeviceValid() == true ) && 
			( m_pointFullscreenCurrent == m_sizeResolution->GetValue() ) )
		{
			return true;
		}

        //
        // Try different resolutions until we find one that actually works
        //

        bChanges = true;

        while (true) {
            //
            // Try the current resolution
            //

            bool bError;
//            if (SwitchToFullscreenDevice(pdddevice, m_pointFullscreen, bError)) {  imago 7/6/09
            if (SwitchToFullscreenDevice(g_DX9Settings.m_refreshrate, m_sizeResolution->GetValue(), bError)) {
                m_pointFullscreenCurrent = m_sizeResolution->GetValue();
                return true;
            }

            //
            // If there was an error just return
            //

            if (bError) {
                return false;
            }

            //
            // Didn't work goto to the next lower resolution
            //

/*            WinPoint pointNew = pdddevice->PreviousMode(m_pointFullscreen);

            if (pointNew == m_pointFullscreen) {
                if (g_bWindowLog) {
                    ZDebugOutput("No more valid resolutions\n");
                }
                return false;
            }

            m_pointFullscreen = pointNew;*/
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void DebugSetWindowed()
    {
//      if (m_pdddevice != NULL) {
//          m_pdddevice->GetDD()->SetCooperativeLevel(NULL, DDSCL_NORMAL);
//      }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Set Attributes
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetAllowSecondary(bool bAllowSecondary)
    {
        if (m_bAllowSecondary != bAllowSecondary) {
            m_bAllowSecondary = bAllowSecondary;
            m_bValid       = false;
            m_bValidDevice = false;
        }
    }

    void SetAllow3DAcceleration(bool bAllow3DAcceleration)
    {
        if (m_bAllow3DAcceleration != bAllow3DAcceleration) {
            m_bAllow3DAcceleration = bAllow3DAcceleration;
//            m_pdddevicePrimary->SetAllow3DAcceleration(m_bAllow3DAcceleration);
            m_bValid       = false;
            m_bValidDevice = false;
        }
    }

	void ForceReset()
	{
		//I do not know why this code is needed, I expect it to do some kind of reset. Since it was repeated a bunch of times I have collected calls to this code here.
		if (CD3DDevice9::Get()->IsInScene())
			CD3DDevice9::Get()->EndScene();
		CD3DDevice9::Get()->ClearScreen();
		CD3DDevice9::Get()->RenderFinished();
		CD3DDevice9::Get()->ClearScreen();
		CD3DDevice9::Get()->RenderFinished();
		WinPoint point = CD3DDevice9::Get()->GetCurrentResolution();
		CD3DDevice9::Get()->ResetDevice(CD3DDevice9::Get()->IsWindowed(), point.X(), point.Y(), g_DX9Settings.m_refreshrate);
	}

// yp Your_Persona August 2 2006 : MaxTextureSize Patch  //Imago 7/18/09 (DX9)
	void SetMaxTextureSize(int iMaxTextureSize)
	{
		if (g_DX9Settings.m_iMaxTextureSize != iMaxTextureSize)
		{
			if (iMaxTextureSize > 3) 
				iMaxTextureSize = 0;
			g_DX9Settings.m_iMaxTextureSize = iMaxTextureSize;

			ForceReset();
		}
	}

	// Imago 7/18/09
	void SetVSync(bool bVsync)
	{
		if (g_DX9Settings.m_bVSync != bVsync) {
			g_DX9Settings.m_bVSync = bVsync;

			ForceReset();
		}
	}

	void SetAA(DWORD dwAA)
	{
		if (g_DX9Settings.m_dwAA != dwAA)
		{
			switch(dwAA) {
			case 10:
			case 17:
			case 0:
				strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"No AA");
				g_DX9Settings.m_dwAA = 0;
				break;
			case 2:
			case 1:
				strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"2xFSAA");
				g_DX9Settings.m_dwAA = 2;
				break;
			case 3:
			case 4:
				strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"4xFSAA");
				g_DX9Settings.m_dwAA = 4;
				break;
			case 5:
			case 6:
				strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"6xFSAA");
				g_DX9Settings.m_dwAA = 6;
				break;
			case 7:
			case 8:
				strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"8xFSAA");
				g_DX9Settings.m_dwAA = 8;
				break;
			case 9:
			case 16:
				strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"16xFSAA");
				g_DX9Settings.m_dwAA = 16;
				break;
			default:
				g_DX9Settings.m_dwAA = 0;
				break;
			}

			//lets make sure we don't crash when we try changing AA settings
			LPDIRECT3D9 pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
			if (g_DX9Settings.m_dwAA) {
				if (pD3D9->CheckDeviceMultiSampleType(CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID, D3DDEVTYPE_HAL, 
					CD3DDevice9::Get()->GetCurrentMode()->mode.Format, CD3DDevice9::Get()->IsWindowed(), D3DMULTISAMPLE_2_SAMPLES, NULL) != D3D_OK) {
					g_DX9Settings.m_dwAA = 0;
					strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"No AA");
				}
			}
			if (g_DX9Settings.m_dwAA == 4) {
				if (pD3D9->CheckDeviceMultiSampleType(CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID, D3DDEVTYPE_HAL, 
					CD3DDevice9::Get()->GetCurrentMode()->mode.Format, CD3DDevice9::Get()->IsWindowed(), D3DMULTISAMPLE_4_SAMPLES, NULL) != D3D_OK) {
					g_DX9Settings.m_dwAA = 0;
					strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"No AA");
				}
			}
			if (g_DX9Settings.m_dwAA == 6) {
				if (pD3D9->CheckDeviceMultiSampleType(CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID, D3DDEVTYPE_HAL, 
					CD3DDevice9::Get()->GetCurrentMode()->mode.Format, CD3DDevice9::Get()->IsWindowed(), D3DMULTISAMPLE_6_SAMPLES, NULL) != D3D_OK) {
					g_DX9Settings.m_dwAA = 0;
					strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"No AA");
				}
			}
			if (g_DX9Settings.m_dwAA == 8) {
				if (pD3D9->CheckDeviceMultiSampleType(CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID, D3DDEVTYPE_HAL, 
					CD3DDevice9::Get()->GetCurrentMode()->mode.Format, CD3DDevice9::Get()->IsWindowed(), D3DMULTISAMPLE_8_SAMPLES, NULL) != D3D_OK) {
					g_DX9Settings.m_dwAA = 0;
					strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"No AA");
				}
			}
			if (g_DX9Settings.m_dwAA == 16) {
				if (pD3D9->CheckDeviceMultiSampleType(CD3DDevice9::Get()->GetDeviceSetupParams()->iAdapterID, D3DDEVTYPE_HAL, 
					CD3DDevice9::Get()->GetCurrentMode()->mode.Format, CD3DDevice9::Get()->IsWindowed(), D3DMULTISAMPLE_16_SAMPLES, NULL) != D3D_OK) {
					g_DX9Settings.m_dwAA = 0;
					strcpy_s(CD3DDevice9::Get()->GetDeviceSetupParams()->szAAType,64,"No AA");
				}
			}
			pD3D9->Release();

			ForceReset();
		}
	}

	void SetUsePack(bool bUsePack)
	{
		if (g_DX9Settings.mbUseTexturePackFiles != bUsePack)
		{
			g_DX9Settings.mbUseTexturePackFiles = bUsePack;

			ForceReset();
		}
	}

	void SetAutoGenMipMaps(bool bUseAutoGenMipMaps)
	{
		if (g_DX9Settings.m_bAutoGenMipmaps != bUseAutoGenMipMaps)
		{
			g_DX9Settings.m_bAutoGenMipmaps = bUseAutoGenMipMaps;
			//CD3DDevice9::Get()->GetDeviceSetupParams()->bAutoGenMipmap = bUseAutoGenMipMaps;

			ForceReset();
		}
	}
	//

	void SetEnableMipMapGeneration(bool bEnable)
	{
		// Store a local copy, reflect the setting in the vram manager.
		m_bMipMapGenerationEnabled = bEnable;
		CVRAMManager::Get()->SetEnableMipMapGeneration( bEnable );
	}

    void Set3DAccelerationImportant(bool b3DAccelerationImportant)
    {
        if (m_b3DAccelerationImportant != b3DAccelerationImportant) {
            m_b3DAccelerationImportant = b3DAccelerationImportant;
            m_bValid = false;
        }
    }

    void SetFullscreen(bool bFullscreen)
    {
        if (m_bFullscreen->GetValue() != bFullscreen) {
            m_bFullscreen->SetValue(bFullscreen);
            m_bValid       = false;
            m_bValidDevice = false;
        }
    }

    void SetFullscreenSize(const Vector& point)
    {
        if (g_bWindowLog) {
            ZDebugOutput("Engine::SetFullscreenSize(" + ZString(point.X()) + "x" + ZString(point.Y()) + " @ " + ZString(point.Z()) +")\n");
        }

		WinPoint next_resolution(int(point.X()), int(point.Y()));
        if (m_sizeResolution->GetValue() != next_resolution) {
			m_sizeResolution->SetValue(next_resolution);
            m_bValid          = false;
        }
		if (g_DX9Settings.m_refreshrate != int(point.Z())) {
			g_DX9Settings.m_refreshrate = int(point.Z());
			m_bValid = false;
		}

        if (g_bWindowLog) {
            ZDebugOutput("Engine::SetFullscreenSize() Exiting\n");
        }
    }


////////////////////////////////////////////////////////////////////////////////////////////////////
// Re-used full screen mode change functions
// Imago 6/24/09  7/1/09 enhanced to use vector and include Refresh Rate Hz
////////////////////////////////////////////////////////////////////////////////////////////////////

    Vector NextMode(const WinPoint& size)
    {
        int count = m_modes.GetCount();

        for(int index = 0; index < count; index++) {
            if (
                   m_modes[index].X() > size.X() 
                //|| m_modes[index].Y() > size.Y() // Imago - look at X only due to widescreens 7/2/09
            ) {
                return m_modes[index];
            }
        }

        return m_modes[count - 1];
    }

    Vector PreviousMode(const WinPoint& size)
    {
        int count = m_modes.GetCount();

        for(int index = count - 1 ; index > 0; index--) {
            if (
                   m_modes[index].X() < size.X() 
                //|| m_modes[index].Y() < size.Y() // Imago - look at X only due to widescreens 7/2/09
            ) {
                return m_modes[index];
            }
        }

        return m_modes[0];
    }

    void EliminateModes(const Vector& size)
    {
        int count = m_modes.GetCount();

        for(int index = 0; index < count; index++) {
            if (
                   m_modes[index].X() >= size.X() 
                //&& m_modes[index].Y() >= size.Y() // Imago - look at X only due to widescreens 7/2/09
            ) {
                m_modes.SetCount(index);
                return;
            }
        }
    }

    void ChangeFullscreenSize(bool bLarger)
    {
		//Imago restored 6/29/09
   		Vector whr; //changed w,h to w,h,r  (width, height, refreshrate)
        if (bLarger) {
			whr = NextMode(m_sizeResolution->GetValue());
        } else {
			whr = PreviousMode(m_sizeResolution->GetValue());
		}          
		m_bChanged = true; //7/7/09
		SetFullscreenSize(whr);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Get Attributes
    //
    //////////////////////////////////////////////////////////////////////////////

    bool GetAllowSecondary()  //REVIEW IMAGO REMOVE
    {
        return m_bAllowSecondary;
    }

    bool GetFullScreenChanged() //Imago added 7/7/09
    {
        return m_bChanged;
    }

    bool GetAllow3DAcceleration() //REVIEW IMAGO (RESTORE?)
    {
        return m_bAllow3DAcceleration;
    }

    bool Get3DAccelerationImportant()  //REVIEW IMAGO REMOVE
    {
        return m_b3DAccelerationImportant;
    }

	const TRef<ModifiableWinPointValue> GetResolutionSizeModifiable()
	{
		return m_sizeResolution;
	}

    const WinPoint GetFullscreenSize()
    {
        return m_sizeResolution->GetValue();
    }

    bool IsFullscreen()
    {
        return m_bFullscreen->GetValue() == true;
    }

    bool PrimaryHas3DAcceleration()  //REVIEW IMAGO (RESTORE?)
    {
        return true;
    }

    ZString GetDeviceName()
    {
		return CD3DDevice9::Get()->GetDeviceSetupString();
    }

    bool GetUsing3DAcceleration()  //REVIEW IMAGO (RESTORE?)
    {
		return true;
    }

    ZString GetPixelFormatName()
    {
        return ZString("Bits per pixel = ") + ZString((int)(GetPrimaryPixelFormat()->PixelBits()));
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //  
    //
    //////////////////////////////////////////////////////////////////////////////

    bool DeviceOK(bool& bChanges)
    {
        if (!m_bValid) 
		{
            if (m_bFullscreen->GetValue() == true) 
			{
                m_bValid = InitializeFullscreen(bChanges);
			} 
			else 
			{
                bChanges = true;
                m_bValid = InitializeWindowed();
            }

            m_bValidDevice = m_bValid;
        }

        return m_bValid;
    }

/*    bool IsDeviceReady(bool& bChanges)
    {
        bChanges = false;

		return DeviceOK(bChanges);


        //if (m_pdddevice) {
        //    HRESULT hr = m_pdddevice->TestCooperativeLevel();

        //    switch (hr) {
        //        case DD_OK:
        //            return DeviceOK(bChanges);

        //        case DDERR_NOEXCLUSIVEMODE:
        //            //
        //            // fullscreen but not active
        //            //

        //            m_bValidDevice = false;
        //            m_bValid       = false;
        //            break;

        //        case DDERR_EXCLUSIVEMODEALREADYSET:
        //            //
        //            // windowed somebody else is fullscreen
        //            //

        //            m_bValidDevice = false;
        //            m_bValid       = false;
        //            break;

        //        case DDERR_WRONGMODE:
        //            //
        //            // windowed the pixel depth has changed
        //            //

        //            m_pdddevicePrimary->Reset(NULL);
        //            m_bValidDevice = false;
        //            m_bValid       = false;

        //            return DeviceOK(bChanges);

        //        default:
        //            ZError("Unexpected result\n");
        //    }
        //}

        //return false;
    }*/


    bool IsDeviceReady(bool& bChanges)
    {
		CD3DDevice9 * pDev = CD3DDevice9::Get();
		if( pDev->IsDeviceValid() )
		{
			HRESULT hr = pDev->TestCooperativeLevel( );
			switch( hr )
			{
			case D3D_OK:
               return DeviceOK(bChanges);

			case D3DERR_DEVICELOST:
				// Device lost - for example, full screen window lost focus.
				// Sleep but carry on running. At some point we should hit a D3DERR_DEVICENOTRESET
				// return value in TestCooperativeLevel().
				m_bValidDevice = false;
				m_bValid       = false;
				break;

			case D3DERR_DEVICENOTRESET:
				hr = pDev->ResetDevice( pDev->IsWindowed() );
				if( hr == D3D_OK )
				{
					m_bValid = true;
					return DeviceOK(bChanges);
				}
				break;

			default:
				break;
			}
		}
		return false;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen updates
    //
    //////////////////////////////////////////////////////////////////////////////

    void Flip()
    {
/*        ZAssert(m_pdddeviceFullscreen);

        DDSurface* pddsurface; CastTo(pddsurface, m_psurfaceBack->GetVideoSurface());
        pddsurface->GetDDSX();

        HRESULT hr = m_pdds->Flip(NULL, DDFLIP_WAIT);
        if (
               hr == DDERR_NOEXCLUSIVEMODE
            || hr == DDERR_SURFACELOST

        ) {
            // These errors are ok if we are no longer active.
        } else {
            DDCall(hr);
        }
		m_pdddevice->GetD3DDevice()->GetD3DDeviceX()->Present( NULL, NULL, 0, NULL );*/
    }

    void BltToWindow(Window* pwindow, const WinPoint& point, Surface* psurface, const WinRect& rectSource)
    {
		_ASSERT( false );
/*        if (m_pdddeviceFullscreen == NULL) {
            if (m_hwndClip != pwindow->GetHWND()) {
                m_hwndClip = pwindow->GetHWND();
                DDCall(m_pddClipper->SetHWnd(0, m_hwndClip));

            }
            m_pointPrimary = pwindow->ClientToScreen(WinPoint(0, 0));

            WinRect
                rectTarget(
                    m_pointPrimary + point,
                    m_pointPrimary + point + rectSource.Size()
                );

            PrivateSurface* pprivateSurface; CastTo(pprivateSurface, psurface);
            DDSurface*      pddsurface;      CastTo(pddsurface, pprivateSurface->GetVideoSurface()); 
            HRESULT hr =
                m_pdds->Blt(
                    (RECT*)&rectTarget,
                    pddsurface->GetDDSX(),
                    (RECT*)&rectSource,
                    DDBLT_WAIT,
                    NULL
                );
        }*/
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Pixel Format cache
    //
    //////////////////////////////////////////////////////////////////////////////

    TVector<TRef<PixelFormat> > m_ppfs;

    TRef<PixelFormat> GetPixelFormat(const D3D9PixelFormat& ddpf)
    {
        int count = m_ppfs.GetCount();

        for(int index = 0; index < count; index++) {
            if (m_ppfs[index]->Equivalent(ddpf)) {
                return m_ppfs[index];
            }
        }

		TRef<PixelFormat> ppf = new PixelFormat(ddpf.dwRGBBitCount, 
												ddpf.dwRBitMask, 
												ddpf.dwGBitMask, 
												ddpf.dwBBitMask, 
												ddpf.dwRGBAlphaBitMask ); //Fix memory leak -Imago 8/2/09

        m_ppfs.PushEnd(ppf);

        return ppf;
    }

    TRef<PixelFormat> GetPixelFormat(
        int   bits,
        DWORD redMask,
        DWORD greenMask,
        DWORD blueMask,
        DWORD alphaMask
    ) {
        D3D9PixelFormat ddpf;

        ddpf.dwRGBBitCount     = bits;
        ddpf.dwRBitMask        = redMask;
        ddpf.dwGBitMask        = greenMask;
        ddpf.dwBBitMask        = blueMask;
        ddpf.dwRGBAlphaBitMask = alphaMask;

        return GetPixelFormat(ddpf);
    }

    PixelFormat* GetPrimaryPixelFormat()
    {
        return m_ppf;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Performance Counters
    //
    //////////////////////////////////////////////////////////////////////////////

	int GetTotalTextureMemory()     { return 0; } //m_pdddevice->GetTotalTextureMemory();     }
    int GetAvailableTextureMemory() { return 0; } //m_pdddevice->GetAvailableTextureMemory(); }
    int GetTotalVideoMemory()       { return 0; } //m_pdddevice->GetTotalVideoMemory();       }
    int GetAvailableVideoMemory()   { return 0; } //m_pdddevice->GetAvailableVideoMemory();   }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Surface Cache
    //
    //////////////////////////////////////////////////////////////////////////////

    //void AddDeviceDependant(DeviceDependant* pdeviceDependant)
    //{
    //    m_listDeviceDependant.PushFront(pdeviceDependant);
    //}

    //void RemoveDeviceDependant(DeviceDependant* pdeviceDependant)
    //{
    //    m_listDeviceDependant.Remove(pdeviceDependant);
    //}

    void RemovePrivateSurface(PrivateSurface* psurface)
    {
        //
        // Remove from the surface lists
        //

        m_listDeviceFormatSurfaces.Remove(psurface);
        m_listSurfaces.Remove(psurface);

        //
        // free up any device textures
        //

/*        DDSurface* pddsurface; CastTo(pddsurface, psurface->GetVideoSurfaceNoAlloc());

        if (pddsurface) {
            m_pdddevicePrimary->RemoveSurface(pddsurface);
            if (m_pdddeviceSecondary != NULL) {
                m_pdddeviceSecondary->RemoveSurface(pddsurface);
            }
        }*/
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // VideoSurface Constructors
    //
    //////////////////////////////////////////////////////////////////////////////

/*    TRef<DDSurface> CreateVideoSurface(	SurfaceType     stype,
										PixelFormat*    ppf,
										PrivatePalette* ppalette,
									const WinPoint&		size,
										int             pitch,
										BYTE*           pbits ) 
	{
		_ASSERT( false );
        if (stype.Test(SurfaceTypeVideo())) 
		{
            return CreateDDSurface( m_pdddevice, stype, m_ppf, NULL, size );
        } 
		else 
		{
            PrivatePalette* pprivatePalette; CastTo(pprivatePalette, ppalette);

            return CreateDDSurface( m_pdddevicePrimary, stype, ppf, ppalette, size, pitch, pbits );
        }
    }*/

    //////////////////////////////////////////////////////////////////////////////
    //
    // Surface Constructors
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<PrivateSurface> AddSurface(PrivateSurface* psurface, bool bDevice)
    {
        m_listSurfaces.PushEnd(psurface);

        if (bDevice) {
            m_listDeviceFormatSurfaces.PushEnd(psurface);
        }

        return psurface;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Create a surface with the specified pixel format
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Surface> CreateSurface(
        const WinPoint& size,
        PixelFormat*    ppf,
 //       Palette*        ppalette,
        SurfaceType     stype,
        SurfaceSite*    psite
) {
   //     PrivatePalette* pprivatePalette; CastTo(pprivatePalette, ppalette);

        return
            AddSurface(
                CreatePrivateSurface(
                    this,
                    ppf,
//                    pprivatePalette,
                    size,
                    stype,
                    psite
                ),
                false
            );
    }


    //////////////////////////////////////////////////////////////////////////////
	// CreateDummySurface()
	// Create a dummy surface which has valid dimensions, but no actual
	// VRAM resources associated with it.
    //////////////////////////////////////////////////////////////////////////////
	TRef<Surface> CreateDummySurface(	const WinPoint& size, 
								        SurfaceSite*    psite /*=NULL*/ )
	{
		TRef<PrivateSurface> psurface = CreatePrivateDummySurface( this, size, psite );
		return AddSurface( psurface, true );
	}


    //////////////////////////////////////////////////////////////////////////////
	// CreateRenderTargetSurface()
	// Create a render target surface.
    //////////////////////////////////////////////////////////////////////////////
	TRef<Surface> CreateRenderTargetSurface( const WinPoint& size, 
											 SurfaceSite*    psite /*=NULL*/ )
	{
		TRef<PrivateSurface> psurface = CreatePrivateRenderTargetSurface( this, size, psite );
		return AddSurface( psurface, true );
	}


    //////////////////////////////////////////////////////////////////////////////
    //
    // Create a device format surface
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Surface> CreateSurface(
        const WinPoint& size, 
        SurfaceType stype, 
        SurfaceSite* psite
    ) {
/*        if (stype.Test(SurfaceTypeVideo())) {
            TRef<DDSurface> pddsurface = 
                CreateDDSurface(
                    m_pdddevice,
                    stype,
                    m_ppf,
                    NULL,
                    size
                );

            if (pddsurface != NULL) {
                TRef<PrivateSurface> psurface = CreatePrivateSurface(this, pddsurface, psite);
                return AddSurface(psurface, true);
            }

            return NULL;
        } else {*/

		// Correct the pixel format if necessary.
		if( ( stype.Test( SurfaceTypeColorKey() ) == true ) &&
			( m_ppf->AlphaMask() == 0 ) )
		{
			if( m_ppf->PixelBytes() == 4 )
			{
				m_ppf = new PixelFormat(32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
			}
			else
			{
				if( CD3DDevice9::Get()->GetDevFlags()->bSupportsA1R5G6B6Format == true )
				{
					m_ppf = new PixelFormat(16, 0x7C00, 0x03e0, 0x001f, 0x8000);
				}
				else
				{
					m_ppf = new PixelFormat(32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
				}
			}
		}
        return 
            AddSurface(
                CreatePrivateSurface(
                    this,
                    m_ppf, 
//                       NULL,		// Remove palette.
                    size, 
                    stype, 
                    psite
                ), 
                true
            );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<PrivateSurface> CreateCompatibleSurface(
        PrivateSurface* psurface, 
        const WinPoint& size,
        SurfaceType     stype, 
        SurfaceSite*    psite
    ) {
//        PrivatePalette* pprivatePalette; CastTo(pprivatePalette, psurface->GetPalette());

		// Construct the pixel format, including any alpha due to color keying.
		PixelFormat * pixelFormat;

		if( psurface->HasColorKey() == false )
		{
			// No colour key, just a straight copy of the pixel format.
			pixelFormat = psurface->GetPixelFormat();
		}
		else
		{
			// For now we just handle two explicit cases. 16 bit with 1 bit alpha or full 32 bit.
			pixelFormat = psurface->GetPixelFormat();
			if( ( pixelFormat->PixelBytes() == 2 ) &&
				( CD3DDevice9::Get()->GetDevFlags()->bSupportsA1R5G6B6Format == true ) )
			{
				pixelFormat = new PixelFormat( D3DFMT_A1R5G5B5 );
			}
			else
			{
				pixelFormat = new PixelFormat( D3DFMT_A8R8G8B8 );
			}
		}
        return
            AddSurface(
                CreatePrivateSurface(
                    this,
                    pixelFormat,
//                    pprivatePalette,
                    size,
                    stype,
                    psite
                ),
                false
            );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Surface> CreateSurface(HBITMAP hbitmap)
    {
		_ASSERT( false );

		return AddSurface(NULL, false);
/*
        //
        // Get the bitmap size
        //

        BITMAP bm;
        ZVerify(::GetObject(hbitmap, sizeof(bm), &bm));

        //
        // Create a DC for the bitmap
        //

        HDC hdcBitmap = ::CreateCompatibleDC(NULL);
        ZAssert(hdcBitmap != NULL);
        HBITMAP hbitmapOld = (HBITMAP)::SelectObject(hdcBitmap, hbitmap);
        ZAssert(hbitmapOld != NULL);

        //
        // Create a surface whose pixel format matches the bitmap
        //

        D3D9PixelFormat ddpf;
        TRef<IDirectDrawPaletteX> pddpal;

		_ASSERT( false );
//        ZVerify(FillDDPF(ddpf, m_pdddevicePrimary->GetDD(), hdcBitmap, hbitmap, &pddpal));

        TRef<PrivatePalette> ppalette;
        if (pddpal) {
            ppalette = CreatePaletteImpl(pddpal);
        }

        //
        // Create the source surface
        //

        TRef<PrivateSurface> psurface =
            CreatePrivateSurface(
                this,
                GetPixelFormat(ddpf),
                ppalette,
                WinPoint(bm.bmWidth, bm.bmHeight),
                SurfaceType2D(),
                NULL
            );

        //
        // Copy the bitmap to the surface
        //

        psurface->BitBltFromDC(hdcBitmap);

        //
        // Release the DC we created
        //

        ZVerify(::SelectObject(hdcBitmap, hbitmapOld));
        ZVerify(::DeleteDC(hdcBitmap));

        return AddSurface(psurface, false);*/
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Surface> CreateSurface(    const WinPoint&		size,
									PixelFormat*		ppf,
									int					pitch,
									BYTE*				pdata,
									IObject*			pobjectMemory,
									const bool			bColorKey,
									const Color &		cColourKey,
									const ZString &		szTextureName, /*=""*/
									const bool			bSystemMemory ) 
	{
        return AddSurface(	CreatePrivateSurface(	this, 
													ppf, 
													size, 
													pitch, 
													pdata, 
													pobjectMemory,
													bColorKey,
													cColourKey,
													szTextureName,
													bSystemMemory),
							false );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////////////////
	TRef<Surface> CreateSurfaceD3DX(	const D3DXIMAGE_INFO *	pImageInfo,
										const WinPoint *		pTargetSize,
										IObject *				pobjectMemory,
										const bool				bColorKey,
										const Color &			cColorKey,
										const ZString &			szTextureName /*= ""*/,
										const bool				bSystemMemory /*= false*/ )
	{
		return AddSurface( CreatePrivateSurface(	this,
													pImageInfo,
													pTargetSize,
													pobjectMemory,
													bColorKey,
													cColorKey,
													szTextureName,
													bSystemMemory ),
							false );
	}
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

TRef<Engine> CreateEngine(bool bAllow3DAcceleration, bool bAllowSecondary, DWORD dwBPP, HWND hWindow)
{
    return new EngineImpl(bAllow3DAcceleration, bAllowSecondary, dwBPP, hWindow); //Fix memory leak -Imago 8/2/09
}


