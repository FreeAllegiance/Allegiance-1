#include "D3DDevice9.h"

#include <zassert.h>

#include "EngineSettings.h"
#include "LogFile.h"
#include "VBIBManager.h"
#include "VRAMManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class implemented as a singleton (mSingleInstance).
// Use static function Get() to get access to the single instance of this class.

CD3DDevice9 CD3DDevice9::mSingleInstance;

////////////////////////////////////////////////////////////////////////////////////////////////////
// CD3DDevice9()
// Constructor. Note: should not be called explicitly. This class uses a single static
// instance of itself.
////////////////////////////////////////////////////////////////////////////////////////////////////
CD3DDevice9::CD3DDevice9()
{
	memset( &m_sD3DDev9, 0, sizeof( SD3DDevice9State ) );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
CD3DDevice9::~CD3DDevice9()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialise()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::Initialise( CLogFile * pLogFile )
{
    ZAssert( m_sD3DDev9.bInitialised == false );

	// Should only be called once, but on the off chance it isn't, we don't want to destroy
	// valid interfaces.
	if( m_sD3DDev9.bInitialised == false )
	{
		memset( &m_sD3DDev9, 0, sizeof( CD3DDevice9::SD3DDevice9State ) );
		m_sD3DDev9.bInitialised = true;
	}

	pLogFile->OutputString( "CD3DDevice9 initialised.\n" );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Shutdown()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::Shutdown( )
{
	if( m_sD3DDev9.bInitialised == true )
	{
		// Clear out a bunch of state. Anything assigned to the device will keep a reference
		// until released. D3D9 won't shut down properly until all references released.
		ResetReferencedResources( );
		
		CVBIBManager::Get()->Shutdown( );
		CVRAMManager::Get()->Shutdown( );

		if( m_sD3DDev9.pBackBufferDepthStencilSurface != NULL )
		{
			m_sD3DDev9.pBackBufferDepthStencilSurface->Release();
			m_sD3DDev9.pBackBufferDepthStencilSurface = NULL;
		}
		if( m_sD3DDev9.pRTDepthStencilSurface != NULL )
		{
			m_sD3DDev9.pRTDepthStencilSurface->Release();
			m_sD3DDev9.pRTDepthStencilSurface = NULL;
		}

		m_sD3DDev9.pD3DDevice->Release();
		m_sD3DDev9.pD3DDevice = NULL;
		m_sD3DDev9.pD3D9->Release( );
		m_sD3DDev9.pD3D9 = NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ResetReferencedResources()
// Clear out a bunch of state. Anything assigned to the device will keep a reference
// until released. D3D9 won't shut down properly until all references released.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::ResetReferencedResources( )
{
	if( m_sD3DDev9.bInitialised == true )
	{
		// Clear out a bunch of state. Anything assigned to the device will keep a reference
		// until released. D3D9 won't shut down properly until all references released.
		m_sD3DDev9.pD3DDevice->SetFVF( 0 );  //Fix memory leak -Imago 8/2/09
		m_sD3DDev9.pD3DDevice->SetIndices( NULL );
		m_sD3DDev9.pD3DDevice->SetStreamSource( 0, NULL, 0, 0 );
		m_sD3DDev9.pD3DDevice->SetPixelShader( NULL );
		m_sD3DDev9.pD3DDevice->SetVertexDeclaration( NULL );
		m_sD3DDev9.pD3DDevice->SetVertexShader( NULL );
		m_sD3DDev9.pD3DDevice->EvictManagedResources( );
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseD3D9()
// Initialise the D3D9 interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::CreateD3D9( CLogFile * pLogFile )
{
	// Create the D3D9 interface.
	m_sD3DDev9.pD3D9 = Direct3DCreate9( D3D_SDK_VERSION ); //Fix memory leak -Imago 8/2/09
	HMODULE hRast = LoadLibrary("rgb9rast.dll");
	if (hRast == 0) {
		hRast = LoadLibrary("rgb9rast_1.dll");
		if (hRast == 0) {
			hRast = LoadLibrary("rgb9rast_2.dll");
		}
	}
	if(hRast != 0) {
            FARPROC D3D9GetSWInfo = GetProcAddress( hRast, "D3D9GetSWInfo");
#ifdef __GNUC__
            HRESULT hr = m_sD3DDev9.pD3D9->RegisterSoftwareDevice((void*)D3D9GetSWInfo);
#else
            HRESULT hr = m_sD3DDev9.pD3D9->RegisterSoftwareDevice(D3D9GetSWInfo);
#endif
			if (hr == D3D_OK) {
				pLogFile->OutputString( "DX registered the SW Rasterizer.\n" );
			} else {
				pLogFile->OutputString( "DX did not register the SW Rasterizer!\n" );
			}
	} else {
		pLogFile->OutputString( "SW Rasterizer failed to load.\n" );
	}
	
    ZAssert( m_sD3DDev9.pD3D9 != NULL );
	if( m_sD3DDev9.pD3D9 == NULL )
	{
		pLogFile->OutputString( "Direct3DCreate9 failed.\n" );
		return E_FAIL;
	}

	pLogFile->OutputString( "Direct3DCreate9 succeeded.\n" );
	return D3D_OK;
}


// BT - 10/17 - Building a last chance device creation. We're gonna create SOMETHING dammit.
D3DFORMAT CD3DDevice9::GetValidBackBufferFormat(class CLogFile * pLogFile)
{
	D3DFORMAT BackBufferFormats[] = {
		D3DFMT_A2R10G10B10, D3DFMT_X8R8G8B8, 
		D3DFMT_X1R5G5B5, D3DFMT_R5G6B5, // Windowed and fullscreen
		D3DFMT_A8R8G8B8, D3DFMT_A1R5G5B5, // Windowed only
		(D3DFORMAT)0 // Terminator.
	};

	D3DFORMAT *pFormatList = BackBufferFormats;

	while (*pFormatList)
	{
		//CheckDeviceType() is used to verify that a Device can support a particular display mode.
		HRESULT hr = m_sD3DDev9.pD3D9->CheckDeviceType(D3DADAPTER_DEFAULT, //Test the primary display device, this is
														//necessary because systems can have add-on cards
														//or multi-monitor setups
			D3DDEVTYPE_HAL,  //This states that we want support for this mode
							 //in hardware rather than emulated in software
			*pFormatList,   //The is the primary (viewable) buffer format
			*pFormatList,   //This is the back (drawable) buffer format
			m_sDevSetupParams.bRunWindowed);   //Is this windowed mode? 

		if (SUCCEEDED(hr)) {
			pLogFile->OutputStringV("GetValidBackBufferFormat: Found format: %x\n", *pFormatList);
			return *pFormatList;
		}

		pFormatList++;
	}

	pLogFile->OutputString("GetValidBackBufferFormat: No valid format found!\n");

	return *pFormatList;
}

	// BT - 10/17 - Building a last chance device creation. We're gonna create SOMETHING dammit.
	// https://www.gamedev.net/forums/topic/135961-what-format-should-i-set-autodepthstencilformat-to/
D3DFORMAT CD3DDevice9::GetValidDepthStencilFormat(D3DFORMAT backbufferFormat, class CLogFile * pLogFile)
{
	D3DFORMAT DepthSetencilFormatList[] = {
		D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D15S1, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D16,  // 32bit stencil formats first...
		D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D15S1, // Then 32bit non-stencil formats...
		D3DFMT_D15S1, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D32, D3DFMT_D24X8, // Then 16bit stencil formats...
		D3DFMT_D16, D3DFMT_D15S1, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, // Finally 16bit non-stencil formats.
		(D3DFORMAT)0 // Terminator.
	};

	D3DFORMAT *pFormatList = DepthSetencilFormatList;

	while (*pFormatList)
	{
		// Does this depth format exist on this card, and can it be used in conjunction with the specified rendertarget format?

		HRESULT hr = m_sD3DDev9.pD3D9->CheckDeviceFormat(m_sDevSetupParams.iAdapterID,
			D3DDEVTYPE_HAL,
			backbufferFormat,
			D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE,
			*pFormatList);

		if (SUCCEEDED(hr))
		{
			break;

			/*
			if (SUCCEEDED(m_sD3DDev9.pD3D9->CheckDepthStencilMatch(m_sDevSetupParams.iAdapterID,
				g_Engine3D->m_pCurrentCaps->DeviceType,
				g_pDefaultEngineWindow->m_d3dpp.BackBufferFormat,
				surface,
				*pFormatList)))
				break;*/
		}

		pFormatList++;
	}

	if (*pFormatList)
		pLogFile->OutputStringV("GetValidDepthStencilFormat: Found valid depth stencil format: %x\n", *pFormatList);
	else
		pLogFile->OutputString("GetValidDepthStencilFormat: No valid depth stencil format found!\n");


	return *pFormatList;
}



// BT - 10/17 - Building a last chance device creation. We're gonna create SOMETHING dammit.
HRESULT CD3DDevice9::LastChanceCreateDevice(HWND hParentWindow, class CLogFile * pLogFile)
{
	HRESULT hr;

	D3DFORMAT backbufferFormat = GetValidBackBufferFormat(pLogFile);
	D3DFORMAT depthStencilFormat = GetValidDepthStencilFormat(backbufferFormat, pLogFile);

	memset(&m_sD3DDev9.d3dPresParams, 0, sizeof(D3DPRESENT_PARAMETERS));
	m_sD3DDev9.d3dPresParams.BackBufferWidth = m_sD3DDev9.pCurrentMode->mode.Width;
	m_sD3DDev9.d3dPresParams.BackBufferHeight = m_sD3DDev9.pCurrentMode->mode.Height;
	m_sD3DDev9.d3dPresParams.BackBufferFormat = backbufferFormat;
	m_sD3DDev9.d3dPresParams.BackBufferCount = 1;
	m_sD3DDev9.d3dPresParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_sD3DDev9.d3dPresParams.MultiSampleQuality = 0;
	m_sD3DDev9.d3dPresParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_sD3DDev9.d3dPresParams.hDeviceWindow = hParentWindow;
	m_sD3DDev9.d3dPresParams.Windowed = m_sDevSetupParams.bRunWindowed;
	m_sD3DDev9.d3dPresParams.EnableAutoDepthStencil = TRUE;
	m_sD3DDev9.d3dPresParams.AutoDepthStencilFormat = depthStencilFormat;
	m_sD3DDev9.d3dPresParams.Flags = 0;
	m_sD3DDev9.d3dPresParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	m_sD3DDev9.d3dPresParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	hr = m_sD3DDev9.pD3D9->CreateDevice(m_sDevSetupParams.iAdapterID,
		D3DDEVTYPE_HAL, 
		hParentWindow,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&m_sD3DDev9.d3dPresParams,
		&m_sD3DDev9.pD3DDevice);

	if (SUCCEEDED(hr))
	{
		m_sD3DDev9.bHardwareVP = true;
		m_sD3DDev9.bPureDevice = false;
		pLogFile->OutputString("LastChanceCreateDevice: HWVP device created.\n");
	}
	else
	{
		hr = m_sD3DDev9.pD3D9->CreateDevice(m_sDevSetupParams.iAdapterID,
			D3DDEVTYPE_HAL,
			hParentWindow,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&m_sD3DDev9.d3dPresParams,
			&m_sD3DDev9.pD3DDevice);
	}

	if (SUCCEEDED(hr))
	{
		m_sD3DDev9.bHardwareVP = false;
		m_sD3DDev9.bPureDevice = false;
		pLogFile->OutputString("LastChanceCreateDevice: SWVP device created.\n");
	}
	else
	{
		pLogFile->OutputString("LastChanceCreateDevice: Failed!\n");
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseDevice()
// Create a new D3D9 device, given a window.
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::CreateDevice( HWND hParentWindow, CLogFile * pLogFile )
{
	DWORD dwCreationFlags;
	HRESULT hr;

	if( m_sDevSetupParams.bRunWindowed == true )
	{
		m_sD3DDev9.pCurrentMode = &m_sDevSetupParams.sWindowedMode;
	}
	else
	{
		m_sD3DDev9.pCurrentMode = &m_sDevSetupParams.sFullScreenMode;
	}

	m_sD3DDev9.hParentWindow = hParentWindow;

	// Create a new 3D device.
	memset( &m_sD3DDev9.d3dPresParams, 0, sizeof( D3DPRESENT_PARAMETERS ) );
	m_sD3DDev9.d3dPresParams.AutoDepthStencilFormat		= m_sD3DDev9.pCurrentMode->fmtDepthStencil;
	m_sD3DDev9.d3dPresParams.EnableAutoDepthStencil		= TRUE;
	m_sD3DDev9.d3dPresParams.BackBufferCount			= 1;
	m_sD3DDev9.d3dPresParams.hDeviceWindow				= hParentWindow;
	m_sD3DDev9.d3dPresParams.Windowed					= m_sDevSetupParams.bRunWindowed;
	m_sD3DDev9.d3dPresParams.BackBufferFormat			= m_sD3DDev9.pCurrentMode->mode.Format;
	m_sD3DDev9.d3dPresParams.BackBufferWidth			= m_sD3DDev9.pCurrentMode->mode.Width;
	m_sD3DDev9.d3dPresParams.BackBufferHeight			= m_sD3DDev9.pCurrentMode->mode.Height;
	if (m_sD3DDev9.pCurrentMode->d3dMultiSampleSetting == D3DMULTISAMPLE_NONE) {
		m_sD3DDev9.d3dPresParams.SwapEffect = D3DSWAPEFFECT_FLIP;
		m_sD3DDev9.d3dPresParams.Flags	= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL | D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; //Imago 7/12/09 enabled
	} else {
		m_sD3DDev9.d3dPresParams.SwapEffect	= D3DSWAPEFFECT_DISCARD;
		//m_sD3DDev9.d3dPresParams.Flags	= D3DPRESENTFLAG_DEVICECLIP; //Imago 7/12/09 enabled
	}

	if( m_sDevSetupParams.bRunWindowed == true )
	{
		m_sD3DDev9.d3dPresParams.FullScreen_RefreshRateInHz	= 0;
	}
	else
	{
		m_sD3DDev9.d3dPresParams.FullScreen_RefreshRateInHz	= g_DX9Settings.m_refreshrate; //m_sD3DDev9.pCurrentMode->mode.RefreshRate;
	}

	if( m_sDevSetupParams.bWaitForVSync == true )
	{
		m_sD3DDev9.d3dPresParams.PresentationInterval		= D3DPRESENT_INTERVAL_ONE;
	}
	else
	{
		m_sD3DDev9.d3dPresParams.PresentationInterval		= D3DPRESENT_INTERVAL_IMMEDIATE; //Spunky #265 backing out //imago 6/10 bWaitForVSync rendered useless
	}

	//imago 7/1/09 NYI test for multisample maskable optons (CSAA, etc) and set accordingly
	m_sD3DDev9.d3dPresParams.MultiSampleQuality				= 0; //<-- here --^
	m_sD3DDev9.d3dPresParams.MultiSampleType				= m_sD3DDev9.pCurrentMode->d3dMultiSampleSetting;

	// KG-
	// to allow NVidia PerfHUD 
	D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;
	bool bForceSWVP = false; //Imago 7/28/09

// - NVidia PerfHUD specific
	D3DADAPTER_IDENTIFIER9 Identifier;
	hr = m_sD3DDev9.pD3D9->GetAdapterIdentifier(m_sDevSetupParams.iAdapterID,0,&Identifier);
	if (strstr(Identifier.Description,"PerfHUD") != 0)
	{
		DeviceType=D3DDEVTYPE_REF;
		pLogFile->OutputString("PerfHUD detected, switching to REF type\n");
	}
// - end of NVidia PerfHUD specific

	dwCreationFlags = D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING;

	// ATI Radeon 9600 (RV350) specific, find out why the TMeshGeo DrawTriangles are failing completley, 
	// I gather this is (related to) the Intel 8xx issue.  --Imago 7/28/09
	if (Identifier.VendorId == 0x1002 && Identifier.DeviceId == 0x4151 ) {
		dwCreationFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		bForceSWVP = true;
	}

    // SiS 661FX/M661FX/760/741/M760/M741 specific --Imago 11/30/09
	if (Identifier.VendorId == 0x1039 && Identifier.DeviceId == 0x6330 ) {
		dwCreationFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		bForceSWVP = true;
	}

	if (m_sD3DDev9.d3dPresParams.BackBufferWidth == 0) {
		//uhhh, try this hack Imago 7/10
		m_sD3DDev9.d3dPresParams.BackBufferWidth = 800;
		m_sD3DDev9.d3dPresParams.BackBufferHeight = 600;
		m_sD3DDev9.d3dPresParams.BackBufferFormat = D3DFMT_R5G6B5;
		m_sD3DDev9.d3dPresParams.AutoDepthStencilFormat = D3DFMT_D24X8;
	}

	// BT - Do not ship this one, this is just for Xynth to test!
	//hr = LastChanceCreateDevice(hParentWindow, pLogFile);


	hr = m_sD3DDev9.pD3D9->CreateDevice(	m_sDevSetupParams.iAdapterID,
											DeviceType, //D3DDEVTYPE_HAL, changed for NVidia PerfHUD
											hParentWindow,
											dwCreationFlags,
											&m_sD3DDev9.d3dPresParams,
											&m_sD3DDev9.pD3DDevice );  //Fix memory leak -Imago 8/2/09




	// Did we create a valid device?
	// 29.07.08 - Courtesy of Imago, it appears that some device drivers (such as Intel Integrated chipset)
	// don't return D3DERR_NOTAVAILABLE when they don't support certain device types. In the event that 
	// the CreateDevice call fails, we'll not bother checking the return value and just step down the 
	// creation chain.
	if( hr != D3D_OK )
	{
		pLogFile->OutputStringV( "Pure HWVP device creation failed: 0x%08x.\n", hr );
		
		//if( hr == D3DERR_NOTAVAILABLE )
		{
			// No, try a non-pure device.
			dwCreationFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
			hr = m_sD3DDev9.pD3D9->CreateDevice(	m_sDevSetupParams.iAdapterID,
												DeviceType, //D3DDEVTYPE_HAL, changed for NVidia PerfHUD,
												hParentWindow,
												dwCreationFlags,
												&m_sD3DDev9.d3dPresParams,
												&m_sD3DDev9.pD3DDevice );
			if( hr != D3D_OK )
			{
				pLogFile->OutputStringV( "HWVP device creation failed: 0x%08x.\n", hr );
				
				// Still no joy, try a software vp device.
				//if( hr == D3DERR_NOTAVAILABLE )
				{
					dwCreationFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
					hr = m_sD3DDev9.pD3D9->CreateDevice(	m_sDevSetupParams.iAdapterID,
														DeviceType, //D3DDEVTYPE_HAL, changed for NVidia PerfHUD,
														hParentWindow,
														dwCreationFlags,
														&m_sD3DDev9.d3dPresParams,
														&m_sD3DDev9.pD3DDevice );
					if( hr == D3D_OK )
					{
						m_sD3DDev9.bHardwareVP = false;
						m_sD3DDev9.bPureDevice = false;
						pLogFile->OutputString( "SWVP device created.\n" );
					}
					else
					{
						pLogFile->OutputStringV( "SWVP device creation failed: 0x%08x.\n", hr );
						hr = m_sD3DDev9.pD3D9->CreateDevice(	m_sDevSetupParams.iAdapterID,
														D3DDEVTYPE_SW, //D3DDEVTYPE_HAL, changed for NVidia PerfHUD,
														hParentWindow,
														dwCreationFlags,
														&m_sD3DDev9.d3dPresParams,
														&m_sD3DDev9.pD3DDevice );
						if( hr == D3D_OK )
						{
							m_sD3DDev9.bHardwareVP = false;
							m_sD3DDev9.bPureDevice = false;
							pLogFile->OutputString( "D3DDEVTYPE_SW device created.\n" );
						}
					}
				}
			}
			else
			{
				m_sD3DDev9.bHardwareVP = true;
				m_sD3DDev9.bPureDevice = false;
				pLogFile->OutputString( "HWVP device created.\n" );
			}
		}
	}
	else
	{
		if (bForceSWVP) {
			m_sD3DDev9.bHardwareVP = false;
			m_sD3DDev9.bPureDevice = false;
			pLogFile->OutputString( "Forced SWVP device created.\n" );
		} else {
			// Valid pure hw device.
			m_sD3DDev9.bHardwareVP = true;
			m_sD3DDev9.bPureDevice = true;
			pLogFile->OutputString( "Pure HWVP device created.\n" );
		}
	}

	if( hr != D3D_OK )
	{
		hr = LastChanceCreateDevice(hParentWindow , pLogFile);

		if (FAILED(hr))
		{

			if (::MessageBox(NULL, "Allegiance couldn't create a valid directX device. We are actively trying to solve this issue, but we need your help. If you encounter this error, please hop onto the Steam community hub, and let us know you saw this message. Would you like to be taken there now?", "Fatal Error", MB_ICONERROR | MB_OKCANCEL) == IDOK)
				ShellExecute(NULL, NULL, "http://steamcommunity.com/app/700480/discussions/2/", NULL, NULL, SW_SHOWNORMAL);

			// BT - 10/17 - if the D3D device couldn't be created, not much point continuing. 
			(*(int*)0) = 0; // Force exception here.

			return hr;
		}
	}
 
	if( hr == D3D_OK )
	{
		hr = m_sD3DDev9.pD3DDevice->GetDeviceCaps( &m_sD3DDev9.sD3DDevCaps );
        ZAssert( hr == D3D_OK );
		hr = m_sD3DDev9.pD3D9->GetAdapterIdentifier(	m_sDevSetupParams.iAdapterID,
													0,
													&m_sD3DDev9.d3dAdapterID );
        ZAssert( hr == D3D_OK );
	}
	
	// Recreate the AA depth stencil buffer, if required.
	CreateAADepthStencilBuffer();

	ClearScreen();
	RenderFinished();
	ClearScreen();
	RenderFinished();

	// Store state.
	m_sD3DDev9.bIsWindowed = m_sDevSetupParams.bRunWindowed;

	//try this hack Imago 7/10
	if (m_sD3DDev9.pCurrentMode->mode.Format == D3DFMT_UNKNOWN) {
		m_sD3DDev9.pCurrentMode->mode.Format = D3DFMT_R5G6B5;
	}

	// Get flags and caps.
	HRESULT hTemp = m_sD3DDev9.pD3D9->CheckDeviceFormat(m_sDevSetupParams.iAdapterID, //Imago was D3DADAPTER_DEFAULT 7/28/09
														D3DDEVTYPE_HAL,
														m_sD3DDev9.pCurrentMode->mode.Format,
														0,
														D3DRTYPE_TEXTURE,
														D3DFMT_A1R5G5B5 );

	if( hTemp == D3D_OK) // BT - Reverting this, it was causing downstream issues. && (m_sD3DDev9.pCurrentMode->mode.Format == D3DFMT_X1R5G5B5 || m_sD3DDev9.pCurrentMode->mode.Format == D3DFMT_R5G6B5 || m_sD3DDev9.pCurrentMode->mode.Format == D3DFMT_A1R5G5B5))
	{
		pLogFile->OutputString( "wasit wiDevice supports A1R5G5B5 format.\n" );
		m_sD3DDev9.sFormatFlags.bSupportsA1R5G6B6Format = true;
	} else {
		m_sD3DDev9.sFormatFlags.bSupportsA1R5G6B6Format = false;
	}

	

	// Auto gen mipmaps flag - include user setting.
	if( ( ( m_sD3DDev9.sD3DDevCaps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP ) != 0 ) &&
		( m_sDevSetupParams.bAutoGenMipmap == true ) )
	{
		pLogFile->OutputString( "Device can auto generate mipmaps.\n" );
		m_sD3DDev9.sFormatFlags.bCanAutoGenMipMaps = true;
		m_sDevSetupParams.bAutoGenMipmap = true;
	} else {
		m_sD3DDev9.sFormatFlags.bCanAutoGenMipMaps = false; //Imago 7/10 #41
		m_sDevSetupParams.bAutoGenMipmap = false;
	}

	// Initialise the caches.
	InitialiseDeviceStateCache( &m_sD3DDev9.sDeviceStateCache );
	InitialiseTransformCache( &m_sD3DDev9.sTransformCache );
	InitialiseMaterialCache( &m_sD3DDev9.sMaterialCache );
	InitialiseLightCache( &m_sD3DDev9.sLightCache );

	// Setup stats.
#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[eD9S_InitialTexMem] = (int)m_sD3DDev9.pD3DDevice->GetAvailableTextureMem();

	if( m_sD3DDev9.bPureDevice == true )
	{
		sprintf_s( m_sDevSetupParams.szDevType, 64, "Pure HWVP" );
	}
	else if( m_sD3DDev9.bHardwareVP == true )
	{
		sprintf_s( m_sDevSetupParams.szDevType, 64, "HWVP" );
	}
	else
	{
		sprintf_s( m_sDevSetupParams.szDevType, 64, "SWVP" );
	}

	sprintf_s( m_sD3DDev9.pszDevSetupString, 256,
		"%s [%s, %s]", m_sD3DDev9.d3dAdapterID.Description,
		m_sDevSetupParams.szDevType,
		m_sDevSetupParams.szAAType );
#endif // EnablePerformanceCounters

	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// UpdateCurrentMode()
// Called after initial settings have been made. Sets up the current pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::UpdateCurrentMode( )
{
	if( m_sDevSetupParams.bRunWindowed == true )
	{
		m_sD3DDev9.pCurrentMode = &m_sDevSetupParams.sWindowedMode;
	}
	else
	{
		m_sD3DDev9.pCurrentMode = &m_sDevSetupParams.sFullScreenMode;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateAADepthStencilBuffer()
// If needed create the additional non-aa depthstencil buffer for render targets.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::CreateAADepthStencilBuffer()
{
	if( ( m_sDevSetupParams.bAntiAliased == true ) &&
		( m_sD3DDev9.d3dPresParams.MultiSampleType != D3DMULTISAMPLE_NONE ) )
	{
		// Store the back buffer depth stencil surface.
		HRESULT hr = m_sD3DDev9.pD3DDevice->GetDepthStencilSurface( &m_sD3DDev9.pBackBufferDepthStencilSurface );
        ZAssert( hr == D3D_OK );
		
		// If these fire, need to look at how we create these.  Imago commented out, only supporting FSAA at this time
        ZAssert( m_sDevSetupParams.sFullScreenMode.mode.Width >= m_sDevSetupParams.sWindowedMode.mode.Width );
        ZAssert( m_sDevSetupParams.sFullScreenMode.mode.Height >= m_sDevSetupParams.sWindowedMode.mode.Height );
        ZAssert( m_sDevSetupParams.sFullScreenMode.fmtDepthStencil == m_sDevSetupParams.sWindowedMode.fmtDepthStencil );

		// Create the additional render target depthstencil surface.
		hr = m_sD3DDev9.pD3DDevice->CreateDepthStencilSurface(
			m_sDevSetupParams.sFullScreenMode.mode.Width,
			m_sDevSetupParams.sFullScreenMode.mode.Height,
			m_sDevSetupParams.sFullScreenMode.fmtDepthStencil,
			D3DMULTISAMPLE_NONE,
			0,
			TRUE,
			&m_sD3DDev9.pRTDepthStencilSurface,
			NULL );
        ZAssert( hr == D3D_OK );
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// ResetDevice()
// Reset the device, switching from full screen to windowed.
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::ResetDevice(	bool	bWindowed, 
									DWORD	dwWidth /*=0*/, 
									DWORD	dwHeight /*=0*/, 
									int 	iRate) // =60 imago added iRate 7/1/09
{
	HRESULT hr				= D3D_OK;
	bool bResetRequired		= false;

	if( bWindowed == true )
	{
		m_sD3DDev9.pCurrentMode = &m_sDevSetupParams.sWindowedMode;
	}
	else
	{
		m_sD3DDev9.pCurrentMode = &m_sDevSetupParams.sFullScreenMode;
	}

	if( (BOOL)bWindowed != m_sD3DDev9.d3dPresParams.Windowed )
	{
		m_sD3DDev9.d3dPresParams.Windowed = bWindowed;
		m_sD3DDev9.bIsWindowed = bWindowed;
		bResetRequired = true;
	}

	// If both zero, we want to force a reset. Used to recover from lost devices.
	if( ( dwWidth == 0 ) || ( dwHeight == 0 ) )
	{
		bResetRequired = true;
		if( bWindowed == true )
		{
            //Imago - removed 1/10
            //ZAssert( ( m_sD3DDev9.dwCurrentWindowedWidth != 0 ) && ( m_sD3DDev9.dwCurrentWindowedHeight != 0 ) );
			dwWidth = m_sD3DDev9.dwCurrentWindowedWidth;
			dwHeight = m_sD3DDev9.dwCurrentWindowedHeight; 
		}
		else
		{   // Imago Removed - 1/10
            //ZAssert( ( m_sD3DDev9.dwCurrentFullscreenWidth != 0 ) && ( m_sD3DDev9.dwCurrentFullscreenHeight != 0 ) );
			dwWidth = m_sD3DDev9.dwCurrentFullscreenWidth;
			dwHeight = m_sD3DDev9.dwCurrentFullscreenHeight;
		}
	}

	// Configure the back buffer width setting.  Imago changed on 6/20/09 to look at height as well.
	if( bWindowed == true )
	{
		if( m_sD3DDev9.d3dPresParams.BackBufferWidth != dwWidth || m_sD3DDev9.d3dPresParams.BackBufferHeight != dwHeight)
		{
			m_sD3DDev9.d3dPresParams.BackBufferWidth = dwWidth;
			m_sD3DDev9.d3dPresParams.BackBufferHeight = dwHeight;
			bResetRequired = true;
		}
		m_sD3DDev9.d3dPresParams.FullScreen_RefreshRateInHz = 0;		// Must set to 0 for windowed.
		m_sD3DDev9.dwCurrentWindowedWidth = dwWidth;
		m_sD3DDev9.dwCurrentWindowedHeight = dwHeight;
	}
	else
	{
		if( m_sD3DDev9.d3dPresParams.BackBufferWidth != dwWidth || m_sD3DDev9.d3dPresParams.BackBufferHeight != dwHeight)
		{
			m_sD3DDev9.d3dPresParams.BackBufferWidth = dwWidth;
			m_sD3DDev9.d3dPresParams.BackBufferHeight = dwHeight;
			m_sD3DDev9.d3dPresParams.FullScreen_RefreshRateInHz = iRate; //m_sD3DDev9.pCurrentMode->mode.RefreshRate; //imago made dynamic 7/1/09
			bResetRequired = true;
		}
		m_sD3DDev9.dwCurrentFullscreenWidth = dwWidth;
		m_sD3DDev9.dwCurrentFullscreenHeight = dwHeight;

	}

    {
        char szBuffer[256];
        sprintf_s(szBuffer, 256, "CD3DDevice9: switching to %s, size %d x %d @ %dHz\n", +bWindowed ? "windowed" : "fullscreen",
            m_sD3DDev9.d3dPresParams.BackBufferWidth,
            m_sD3DDev9.d3dPresParams.BackBufferHeight,
            m_sD3DDev9.d3dPresParams.FullScreen_RefreshRateInHz);
        g_pDebugLogger->Log(szBuffer);
    }

	if( bResetRequired == true || //mode changes
		g_DX9Settings.m_dwAA != (DWORD)m_sD3DDev9.pCurrentMode->d3dMultiSampleSetting || //any multisample changes
		g_DX9Settings.m_bAutoGenMipmaps != m_sDevSetupParams.bAutoGenMipmap || //any mip map changes
		g_DX9Settings.m_bVSync != m_sDevSetupParams.bWaitForVSync || //Spunky #265 backing out //Imago 7/10
		g_DX9Settings.m_iMaxTextureSize != (int)m_sDevSetupParams.maxTextureSize) //any texture size changes

	{
		// Prepare the d3dPresParams.
		m_sD3DDev9.d3dPresParams.BackBufferFormat	= m_sD3DDev9.pCurrentMode->mode.Format;
		m_sD3DDev9.d3dPresParams.MultiSampleType = (D3DMULTISAMPLE_TYPE)g_DX9Settings.m_dwAA;
		m_sD3DDev9.d3dPresParams.AutoDepthStencilFormat = m_sD3DDev9.pCurrentMode->fmtDepthStencil;

		ResetReferencedResources( );

		// Currently, render targets and dynamic buffers reside in the the default pool.
		CVRAMManager::Get()->EvictDefaultPoolResources( );
		CVBIBManager::Get()->EvictDefaultPoolResources( );

		if( m_sDevSetupParams.bAntiAliased == true )
		{
			// BT - 9/17 - Prevent crash on reset device with AA enabled.
			if (m_sD3DDev9.pRTDepthStencilSurface != NULL)
			{
				m_sD3DDev9.pRTDepthStencilSurface->Release();
				m_sD3DDev9.pRTDepthStencilSurface = NULL;
			}

			// BT - 9/17 - Prevent crash on reset device with AA enabled.
			if (m_sD3DDev9.pBackBufferDepthStencilSurface != NULL)
			{
				// We also stored a pointer to the main back buffer.
				m_sD3DDev9.pBackBufferDepthStencilSurface->Release();
				m_sD3DDev9.pBackBufferDepthStencilSurface = NULL;
			}
		}

		//Imago 7/19/09
		if (g_DX9Settings.m_dwAA) {
			m_sDevSetupParams.bAntiAliased = true;
			m_sD3DDev9.pCurrentMode->d3dMultiSampleSetting = (D3DMULTISAMPLE_TYPE)g_DX9Settings.m_dwAA;
			UpdateCurrentMode();
		} else {
			m_sDevSetupParams.bAntiAliased = false;
			m_sD3DDev9.pCurrentMode->d3dMultiSampleSetting = D3DMULTISAMPLE_NONE;
			UpdateCurrentMode();
		}

		if (m_sDevSetupParams.bAntiAliased == false) {
			m_sD3DDev9.d3dPresParams.SwapEffect = D3DSWAPEFFECT_FLIP;
			m_sD3DDev9.d3dPresParams.Flags	= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL | D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; //Imago 7/12/09 enabled  8/10 removed Clip #227
		} else {
			m_sD3DDev9.d3dPresParams.SwapEffect	= D3DSWAPEFFECT_DISCARD;
			//m_sD3DDev9.d3dPresParams.Flags	= D3DPRESENTFLAG_DEVICECLIP; //Imago 7/12/09 enabled 8/10 disabled #227
			m_sD3DDev9.d3dPresParams.Flags = 0; //Spunky #252
		}

		//imago 7/18/09
		if (g_DX9Settings.m_bVSync) {
			m_sDevSetupParams.bWaitForVSync = true;
			m_sD3DDev9.d3dPresParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		} else {
			m_sDevSetupParams.bWaitForVSync = false;
			m_sD3DDev9.d3dPresParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; //Spunky #265 backing out //rendered useless 6/10 Imago
		}

		//Imago added m_sD3DDev9.sFormatFlags.bCanAutoGenMipMaps just to be sure 7/10 #41
		if ((m_sD3DDev9.sD3DDevCaps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP ) && g_DX9Settings.m_bAutoGenMipmaps)
		{
			CVRAMManager::Get()->SetEnableMipMapGeneration(true);
			m_sDevSetupParams.bAutoGenMipmap = true;
			m_sD3DDev9.sFormatFlags.bCanAutoGenMipMaps = true; 
		} else {
			CVRAMManager::Get()->SetEnableMipMapGeneration(false);
			m_sDevSetupParams.bAutoGenMipmap = false;
			m_sD3DDev9.sFormatFlags.bCanAutoGenMipMaps = false; 
		}

		m_sDevSetupParams.dwMaxTextureSize = 256 << g_DX9Settings.m_iMaxTextureSize;
		m_sDevSetupParams.maxTextureSize = (CD3DDevice9::EMaxTextureSize) g_DX9Settings.m_iMaxTextureSize;
		//

		// Perform the reset.
		hr = m_sD3DDev9.pD3DDevice->Reset( &m_sD3DDev9.d3dPresParams ); //imago changed 6/29/09 to fall thru  //Fix memory leak -Imago 8/2/09

		if(hr == D3D_OK ) {

			// Initialise the caches.
			InitialiseDeviceStateCache( &m_sD3DDev9.sDeviceStateCache );
			InitialiseTransformCache( &m_sD3DDev9.sTransformCache );
			InitialiseMaterialCache( &m_sD3DDev9.sMaterialCache );
			InitialiseLightCache( &m_sD3DDev9.sLightCache );

			// Recreate the AA depth stencil buffer, if required.
			CreateAADepthStencilBuffer( );

			// UIpdate this Imago 7/19/09
			sprintf_s( m_sD3DDev9.pszDevSetupString, 256,
				"%s [%s, %s]", m_sD3DDev9.d3dAdapterID.Description,
				m_sDevSetupParams.szDevType,
				m_sDevSetupParams.szAAType );
		}
	}

	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// ClearScreen()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::ClearScreen( )
{
	HRESULT hr = D3D_OK;

	hr = m_sD3DDev9.pD3DDevice->Clear(	0, 
										NULL, 
										D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
										0xFF000000,
										1.0f,
										0 );
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// TestCooperativeLevel()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::TestCooperativeLevel( )
{
	return m_sD3DDev9.pD3DDevice->TestCooperativeLevel();
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderFinished()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::RenderFinished( )
{
	HRESULT hr;
	hr = m_sD3DDev9.pD3DDevice->Present( NULL, NULL, NULL, NULL );  //Fix memory leak -Imago 8/2/09
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCurrentResolution()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
WinPoint CD3DDevice9::GetCurrentResolution( )
{
	return WinPoint( m_sD3DDev9.d3dPresParams.BackBufferWidth, m_sD3DDev9.d3dPresParams.BackBufferHeight );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// IsDeviceValid()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CD3DDevice9::IsDeviceValid( )
{
	return m_sD3DDev9.bInitialised && ( m_sD3DDev9.pD3DDevice != NULL );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// IsInScene()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CD3DDevice9::IsInScene( )
{
	return m_sD3DDev9.bInScene;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// IsWindowed()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CD3DDevice9::IsWindowed( )
{
	return m_sD3DDev9.bIsWindowed;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetBackBufferDepthStencil()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::SetBackBufferDepthStencil( )
{
    ZAssert( m_sDevSetupParams.bAntiAliased == true );
    ZAssert( m_sD3DDev9.pBackBufferDepthStencilSurface != NULL );

	return m_sD3DDev9.pD3DDevice->SetDepthStencilSurface( m_sD3DDev9.pBackBufferDepthStencilSurface );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetRTDepthStencil()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::SetRTDepthStencil( )
{
    ZAssert( m_sDevSetupParams.bAntiAliased == true );
    ZAssert( m_sD3DDev9.pRTDepthStencilSurface != NULL );

	return m_sD3DDev9.pD3DDevice->SetDepthStencilSurface( m_sD3DDev9.pRTDepthStencilSurface);
}

// BT - 10/17 - If the D3D device becomes null, re-create it to get it up and rolling again.
const LPDIRECT3DDEVICE9 CD3DDevice9::Device()
{ 
	if (m_sD3DDev9.pD3DDevice == nullptr)
	{
		CLogFile logfile("D3DDevice9_Reinitialize.log");

		for (int i = 0; i < 10 && m_sD3DDev9.pD3DDevice == nullptr; i++)
		{
			CreateDevice(m_sD3DDev9.hParentWindow, &logfile);
			Sleep(100);
		}

		// If the window couldn't be created in the current mode, try to create it windowed instead.
		for (int i = 0; i < 10 && m_sD3DDev9.pD3DDevice == nullptr; i++)
		{
			m_sD3DDev9.bIsWindowed = true;

			CreateDevice(m_sD3DDev9.hParentWindow, &logfile);
			Sleep(100);
		}
		
		logfile.CloseLogFile();
	}

	return m_sD3DDev9.pD3DDevice; 
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// STATE CACHING ROUTINES
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseDeviceStateCache()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseDeviceStateCache( SD3D9DeviceStateCache * pCache )
{
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

	DWORD i;

	// Reset the render states.
	InitialiseRenderStateCache( pCache->pRenderState );

	// Reset the texture stage states.
	for( i=0; i<dwMaxTextureStages; i++ )
	{
		InitialiseTextureStageCache( i, pCache->pTextureStageStates[i] );
	}

	// Reset the sampler states.
	pCache->dwNumSamplers = m_sD3DDev9.sD3DDevCaps.MaxSimultaneousTextures;
	pCache->pSamplerState = new DWORD* [ pCache->dwNumSamplers ]; //Fix memory leak -Imago 8/2/09

	for( i=0; i<pCache->dwNumSamplers; i++ )
	{
		pCache->pSamplerState[i] = new DWORD[ dwMaxSamplerStatesAllowed ]; //Fix memory leak -Imago 8/2/09
		InitialiseSamplerStateCache( i, pCache->pSamplerState[i] );
	}

	// Reset remaining cached values.
	pCache->dwFVF = 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
#define RESET_STATE_CACHE_VALUE( state, value ) pRenderStateCache[state] = value;\
	m_sD3DDev9.pD3DDevice->SetRenderState( state, value );

////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseRenderStateCache()
// Clear out a state cache and reset to defaults.
// Default values taken from Oct '06 SDK documentation.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseRenderStateCache( DWORD * pRenderStateCache )
{
	float fOne = 1.0f;
	float fSixtyFour = 64.0f;
	memset( pRenderStateCache, 0xFF, dwMaxRenderStatesAllowed * sizeof( DWORD ) );

	RESET_STATE_CACHE_VALUE( D3DRS_ZENABLE, D3DZB_TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_FILLMODE, D3DFILL_SOLID );
	RESET_STATE_CACHE_VALUE( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	RESET_STATE_CACHE_VALUE( D3DRS_ZWRITEENABLE, TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_ALPHATESTENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_LASTPIXEL, TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_SRCBLEND, D3DBLEND_ONE );
	RESET_STATE_CACHE_VALUE( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	RESET_STATE_CACHE_VALUE( D3DRS_CULLMODE, D3DCULL_CCW );
	RESET_STATE_CACHE_VALUE( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	RESET_STATE_CACHE_VALUE( D3DRS_ALPHAREF, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_ALPHAFUNC, D3DCMP_ALWAYS );
	RESET_STATE_CACHE_VALUE( D3DRS_DITHERENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_ALPHABLENDENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_FOGENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_SPECULARENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_FOGCOLOR, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	RESET_STATE_CACHE_VALUE( D3DRS_FOGSTART, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_FOGEND, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_FOGDENSITY, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_RANGEFOGENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILREF, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILMASK, 0xFFFFFFFF );
	RESET_STATE_CACHE_VALUE( D3DRS_STENCILWRITEMASK, 0xFFFFFFFF );
	RESET_STATE_CACHE_VALUE( D3DRS_TEXTUREFACTOR, 0xFFFFFFFF );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP0, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP1, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP2, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP3, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP4, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP5, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP6, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP7, 0 );
    if (m_sD3DDev9.sD3DDevCaps.MaxVertexW < 2) { //8/8/09 11/09 Imago
        RESET_STATE_CACHE_VALUE( D3DRS_CLIPPING, TRUE );
    } else {
	    RESET_STATE_CACHE_VALUE( D3DRS_CLIPPING, FALSE );
    }
	RESET_STATE_CACHE_VALUE( D3DRS_LIGHTING, TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_AMBIENT, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_FOGVERTEXMODE, D3DFOG_NONE );
	RESET_STATE_CACHE_VALUE( D3DRS_COLORVERTEX, TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_LOCALVIEWER, TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_NORMALIZENORMALS, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );
	RESET_STATE_CACHE_VALUE( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2 );
	RESET_STATE_CACHE_VALUE( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL );
	RESET_STATE_CACHE_VALUE( D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );
	RESET_STATE_CACHE_VALUE( D3DRS_VERTEXBLEND, D3DVBF_DISABLE );
	RESET_STATE_CACHE_VALUE( D3DRS_CLIPPLANEENABLE, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSIZE, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSIZE_MIN, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSPRITEENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSCALEENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSCALE_A, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSCALE_B, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSCALE_C, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
	RESET_STATE_CACHE_VALUE( D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF );
	RESET_STATE_CACHE_VALUE( D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE );
	RESET_STATE_CACHE_VALUE( D3DRS_DEBUGMONITORTOKEN, D3DDMT_ENABLE );
	RESET_STATE_CACHE_VALUE( D3DRS_POINTSIZE_MAX, *((DWORD*) (&fSixtyFour)));
	RESET_STATE_CACHE_VALUE( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_COLORWRITEENABLE, 0x0000000F );
	RESET_STATE_CACHE_VALUE( D3DRS_TWEENFACTOR, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_BLENDOP, D3DBLENDOP_ADD );
	RESET_STATE_CACHE_VALUE( D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC );
	RESET_STATE_CACHE_VALUE( D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR );
	RESET_STATE_CACHE_VALUE( D3DRS_SCISSORTESTENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_SLOPESCALEDEPTHBIAS, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_ANTIALIASEDLINEENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_MINTESSELLATIONLEVEL, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_MAXTESSELLATIONLEVEL, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_ADAPTIVETESS_X, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_ADAPTIVETESS_Y, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_ADAPTIVETESS_Z, *((DWORD*) (&fOne)));
	RESET_STATE_CACHE_VALUE( D3DRS_ADAPTIVETESS_W, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_ENABLEADAPTIVETESSELLATION, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_TWOSIDEDSTENCILMODE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP );
	RESET_STATE_CACHE_VALUE( D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP );
	RESET_STATE_CACHE_VALUE( D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP );
	RESET_STATE_CACHE_VALUE( D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS );
	RESET_STATE_CACHE_VALUE( D3DRS_COLORWRITEENABLE1, 0x0000000F );
	RESET_STATE_CACHE_VALUE( D3DRS_COLORWRITEENABLE2, 0x0000000F );
	RESET_STATE_CACHE_VALUE( D3DRS_COLORWRITEENABLE3, 0x0000000F );
	RESET_STATE_CACHE_VALUE( D3DRS_BLENDFACTOR, 0xFFFFFFFF );
	RESET_STATE_CACHE_VALUE( D3DRS_SRGBWRITEENABLE, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_DEPTHBIAS, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP8, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP9, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP10, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP11, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP12, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP13, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP14, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_WRAP15, 0 );
	RESET_STATE_CACHE_VALUE( D3DRS_SEPARATEALPHABLENDENABLE, FALSE );
	RESET_STATE_CACHE_VALUE( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );
	RESET_STATE_CACHE_VALUE( D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO );
	RESET_STATE_CACHE_VALUE( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
#define RESET_TEXTURE_STATE_CACHE_VALUE( stage, state, value ) pTextureStateCache[state] = value;\
	m_sD3DDev9.pD3DDevice->SetTextureStageState( stage, state, value );

////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseTextureStageCache()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseTextureStageCache(DWORD dwStageIndex, DWORD *pTextureStateCache)
{
	memset( pTextureStateCache, 0xFF, dwMaxTextureStageStatesAllowed * sizeof( DWORD ) );

	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_COLOROP, (dwStageIndex == 0) ? D3DTOP_MODULATE : D3DTOP_DISABLE );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_COLORARG2, D3DTA_CURRENT );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_ALPHAOP, (dwStageIndex == 0) ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_BUMPENVMAT00, 0 );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_BUMPENVMAT01, 0 );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_BUMPENVMAT10, 0 );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_BUMPENVMAT11, 0 );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_TEXCOORDINDEX, dwStageIndex );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_BUMPENVLSCALE, 0 );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_BUMPENVLOFFSET, 0 );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_COLORARG0, D3DTA_CURRENT );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_ALPHAARG0, D3DTA_CURRENT );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_RESULTARG, D3DTA_CURRENT );
	RESET_TEXTURE_STATE_CACHE_VALUE( dwStageIndex, D3DTSS_CONSTANT, D3DTA_CURRENT );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
#define RESET_SAMPLER_STATE_CACHE_VALUE( sampler, state, value ) pSamplerStateCache[state] = value;\
	m_sD3DDev9.pD3DDevice->SetSamplerState( sampler, state, value );

////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseSamplerStateCache()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseSamplerStateCache( DWORD dwSamplerIndex, DWORD * pSamplerStateCache )
{
	memset( pSamplerStateCache, 0xFF, dwMaxSamplerStatesAllowed * sizeof( DWORD ) );

	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_BORDERCOLOR, 0 );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_MINFILTER, D3DTEXF_POINT );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_MIPMAPLODBIAS, 0 );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_MAXMIPLEVEL, 0 );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_MAXANISOTROPY, 1 );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_SRGBTEXTURE, 0 );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_ELEMENTINDEX, 0 );
	RESET_SAMPLER_STATE_CACHE_VALUE( dwSamplerIndex, D3DSAMP_DMAPOFFSET, 0 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseTransformCache()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseTransformCache( SD3D9TransformCache * pCache )
{
	// No need to clear, already reset.
//	memset( pCache, 0, sizeof( SD3D9TransformCache ) );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseMaterialCache()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseMaterialCache( SD3D9MaterialCache * pCache )
{
	// No need to clear already reset.
//	memset( pCache, 0, sizeof( SD3D9MaterialCache ) );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// InitialiseLightCache()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::InitialiseLightCache( SD3D9LightCache * pCache )
{
	pCache->dwNumLights = m_sD3DDev9.sD3DDevCaps.MaxActiveLights;

	if( pCache->dwNumLights == 0xFFFFFFFF )
	{
		// Software VP supports any number of lights, so returns -1. We'll limit it to 8,
		// even though we don't use that many.
        ZAssert( m_sD3DDev9.bHardwareVP == false );
		pCache->dwNumLights = 8;
	}

	pCache->pLightsActive = new BOOL[ pCache->dwNumLights ]; //Fix memory leak -Imago 8/2/09
	pCache->pLights = new D3DLIGHT9[ pCache->dwNumLights ]; //Fix memory leak -Imago 8/2/09

	memset( pCache->pLightsActive, 0, pCache->dwNumLights * sizeof( BOOL ) );
	memset( pCache->pLights, 0, pCache->dwNumLights * sizeof( D3DLIGHT9 ) );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Cache access function.
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// SetRenderState()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetRenderState( D3DRENDERSTATETYPE State, DWORD Value )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( State < dwMaxRenderStatesAllowed );
	if( m_sD3DDev9.sDeviceStateCache.pRenderState[ State ] == Value )
	{
		return D3D_OK;
	}
#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumStateChanges ] ++;
#endif // EnablePerformanceCounters

	m_sD3DDev9.sDeviceStateCache.pRenderState[ State ] = Value;
	return m_sD3DDev9.pD3DDevice->SetRenderState( State, Value );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetRenderState()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetRenderState( D3DRENDERSTATETYPE State, DWORD * pValue )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( State < dwMaxRenderStatesAllowed );
	*pValue = m_sD3DDev9.sDeviceStateCache.pRenderState[ State ];
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTextureStageState()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( Stage < dwMaxTextureStages );
    ZAssert( Type < dwMaxTextureStageStatesAllowed );
	if( m_sD3DDev9.sDeviceStateCache.pTextureStageStates[Stage][Type] == Value )
	{
		return D3D_OK;
	}
#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumStateChanges ] ++;
#endif // EnablePerformanceCounters

	m_sD3DDev9.sDeviceStateCache.pTextureStageStates[Stage][Type] = Value;
	return m_sD3DDev9.pD3DDevice->SetTextureStageState( Stage, Type, Value );
}

	

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetTextureStageState()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::GetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD * pValue )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( Stage < dwMaxTextureStages );
    ZAssert( Type < dwMaxTextureStageStatesAllowed );
	*pValue = m_sD3DDev9.sDeviceStateCache.pTextureStageStates[Stage][Type];
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetSamplerState()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( Sampler < m_sD3DDev9.sDeviceStateCache.dwNumSamplers );
    ZAssert( Type < dwMaxSamplerStatesAllowed );
	
	if( m_sD3DDev9.sDeviceStateCache.pSamplerState[Sampler][Type] == Value )
	{
		return D3D_OK;
	}
#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumStateChanges ] ++;
#endif // EnablePerformanceCounters

	m_sD3DDev9.sDeviceStateCache.pSamplerState[Sampler][Type] = Value;
	return m_sD3DDev9.pD3DDevice->SetSamplerState( Sampler, Type, Value );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetSamplerState()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD * pValue )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( Sampler < m_sD3DDev9.sDeviceStateCache.dwNumSamplers );
    ZAssert( Type < dwMaxSamplerStatesAllowed );

	*pValue = m_sD3DDev9.sDeviceStateCache.pSamplerState[Sampler][Type];
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTransform()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::SetTransform( D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX * pMatrix )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert(	( State == D3DTS_PROJECTION ) ||
				( State == D3DTS_VIEW ) ||
				( State == D3DTS_WORLD ) );

	HRESULT hr;
	switch( State )
	{
	case D3DTS_PROJECTION:
		memcpy( &m_sD3DDev9.sTransformCache.matProjection, pMatrix, sizeof( D3DMATRIX ) );
		hr = m_sD3DDev9.pD3DDevice->SetTransform( D3DTS_PROJECTION, pMatrix );
		break;

	case D3DTS_VIEW:
		memcpy( &m_sD3DDev9.sTransformCache.matView, pMatrix, sizeof( D3DMATRIX ) );
		hr = m_sD3DDev9.pD3DDevice->SetTransform( D3DTS_VIEW, pMatrix );
		break;

	case D3DTS_WORLD:
		memcpy( &m_sD3DDev9.sTransformCache.matWorld, pMatrix, sizeof( D3DMATRIX ) );
		hr = m_sD3DDev9.pD3DDevice->SetTransform( D3DTS_WORLD, pMatrix );
		break;

	default:
        ZAssert( false );
		hr = D3DERR_INVALIDCALL;
	}
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetTransform()
// 
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetTransform( D3DTRANSFORMSTATETYPE State, D3DMATRIX * pMatrix )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert(	( State == D3DTS_PROJECTION ) ||
				( State == D3DTS_VIEW ) ||
				( State == D3DTS_WORLD ) );
	HRESULT hr = D3D_OK;
	switch( State )
	{
	case D3DTS_PROJECTION:
		memcpy( pMatrix, &m_sD3DDev9.sTransformCache.matProjection, sizeof( D3DMATRIX ) );
		break;

	case D3DTS_VIEW:
		memcpy( pMatrix, &m_sD3DDev9.sTransformCache.matView, sizeof( D3DMATRIX ) );
		break;

	case D3DTS_WORLD:
		memcpy( pMatrix, &m_sD3DDev9.sTransformCache.matWorld, sizeof( D3DMATRIX ) );
		break;

	default:
        ZAssert( false );
		hr = D3DERR_INVALIDCALL;
	}
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetMaterial()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetMaterial( CONST D3DMATERIAL9 * pMaterial )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( pMaterial != NULL );

	memcpy( &m_sD3DDev9.sMaterialCache.currentMaterial, pMaterial, sizeof( D3DMATERIAL9 ) );
	return m_sD3DDev9.pD3DDevice->SetMaterial( pMaterial );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetMaterial()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetMaterial( D3DMATERIAL9 * pMaterial )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( pMaterial != NULL );
	memcpy( pMaterial, &m_sD3DDev9.sMaterialCache.currentMaterial, sizeof( D3DMATERIAL9 ) );
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetLight()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::SetLight( DWORD Index, CONST D3DLIGHT9 * pLight )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( pLight != NULL );
    ZAssert( Index < m_sD3DDev9.sLightCache.dwNumLights );

	memcpy( &m_sD3DDev9.sLightCache.pLights[ Index ], pLight, sizeof( D3DLIGHT9 ) );
	return m_sD3DDev9.pD3DDevice->SetLight( Index, pLight );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetLight()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::GetLight( DWORD Index, D3DLIGHT9 * pLight )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( pLight != NULL );
    ZAssert( Index < m_sD3DDev9.sLightCache.dwNumLights );

	memcpy( pLight, &m_sD3DDev9.sLightCache.pLights[ Index ], sizeof( D3DLIGHT9 ) );
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// LightEnable()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::LightEnable( DWORD Index, BOOL bEnable )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( Index < m_sD3DDev9.sLightCache.dwNumLights );

	HRESULT hr = D3D_OK;
	if( m_sD3DDev9.sLightCache.pLightsActive[ Index ] != bEnable )
	{
		m_sD3DDev9.sLightCache.pLightsActive[ Index ] = bEnable;
		hr = m_sD3DDev9.pD3DDevice->LightEnable( Index, bEnable );
	}
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetLightEnable()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetLightEnable( DWORD Index, BOOL * pEnable )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
    ZAssert( Index < m_sD3DDev9.sLightCache.dwNumLights );

	*pEnable = m_sD3DDev9.sLightCache.pLightsActive[Index];
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetFVF()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetFVF( DWORD FVF )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	
	if( m_sD3DDev9.sDeviceStateCache.dwFVF == FVF )
	{
		return D3D_OK;
	}

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumShaderChanges ] ++;
#endif // EnablePerformanceCounters

	m_sD3DDev9.sDeviceStateCache.dwFVF = FVF;
	return m_sD3DDev9.pD3DDevice->SetFVF( FVF );  //Fix memory leak -Imago 8/2/09
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetFVF()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetFVF( DWORD * pFVF )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

	*pFVF = m_sD3DDev9.sDeviceStateCache.dwFVF;
	return D3D_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTexture()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetTexture( DWORD Sampler, LPDIRECT3DBASETEXTURE9 pTexture )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumTextureChanges ] ++;
#endif // EnablePerformanceCounters

	return m_sD3DDev9.pD3DDevice->SetTexture( Sampler, pTexture );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetTexture()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetTexture( DWORD Sampler, LPDIRECT3DBASETEXTURE9 * ppTexture )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->GetTexture( Sampler, ppTexture );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetViewport()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetViewport( CONST D3DVIEWPORT9 * pViewport )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->SetViewport( pViewport );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetViewport()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetViewport( D3DVIEWPORT9 * pViewport )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->GetViewport( pViewport );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetIndices()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetIndices( LPDIRECT3DINDEXBUFFER9 pIndexData )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->SetIndices( pIndexData );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetIndices()
// 
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetIndices( LPDIRECT3DINDEXBUFFER9 * ppIndexData )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->GetIndices( ppIndexData );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetStreamSource()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::SetStreamSource( UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->SetStreamSource( StreamNumber, pStreamData, OffsetInBytes, Stride );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetStreamSource()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::GetStreamSource( UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 * ppStreamData, UINT * pOffsetInBytes, UINT * pStride )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->GetStreamSource( StreamNumber, ppStreamData, pOffsetInBytes, pStride );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// BeginScene()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::BeginScene()
{
	HRESULT hr;

    ZAssert( m_sD3DDev9.bInScene == false );
	hr = m_sD3DDev9.pD3DDevice->BeginScene();

	if( hr == D3D_OK )
	{
		m_sD3DDev9.bInScene = true;
	}

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_CurrentTexMem ] = (int)m_sD3DDev9.pD3DDevice->GetAvailableTextureMem();
#endif // EnablePerformanceCounters


    ZAssert( hr == D3D_OK );
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// EndScene()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::EndScene()
{
	HRESULT hr;

    ZAssert( m_sD3DDev9.bInScene == true );
	hr = m_sD3DDev9.pD3DDevice->EndScene();
	
	if( hr == D3D_OK )
	{
		m_sD3DDev9.bInScene = false;
	}

    ZAssert( hr == D3D_OK );
	return hr;
}

	
	
////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawPrimitive()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::DrawPrimitive(	D3DPRIMITIVETYPE Type,
									UINT StartVertex,
									UINT PrimitiveCount )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumDrawPrims ] ++;
	m_pPerformanceCounters[ eD9S_NumPrimsRendered ] += (int)PrimitiveCount;
#endif // EnablePerformanceCounters

	return m_sD3DDev9.pD3DDevice->DrawPrimitive( Type, StartVertex, PrimitiveCount );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawIndexedPrimitive()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::DrawIndexedPrimitive(	D3DPRIMITIVETYPE Type, 
											INT BaseVertexIndex, 
											UINT MinIndex, 
											UINT NumVertices, 
											UINT StartIndex, 
											UINT PrimitiveCount )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumDrawPrims ] ++;
	m_pPerformanceCounters[ eD9S_NumPrimsRendered ] += (int)PrimitiveCount;
#endif // EnablePerformanceCounters

	return m_sD3DDev9.pD3DDevice->DrawIndexedPrimitive( 
				Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawIndexedPrimitiveUP()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CD3DDevice9::DrawIndexedPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, 
												UINT MinVertexIndex, 
												UINT NumVertices, 
												UINT PrimitiveCount, 
												CONST void * pIndexData, 
												D3DFORMAT IndexDataFormat, 
												CONST void * pVertexStreamZeroData,
												UINT VertexStreamZeroStride )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumDrawPrims ] ++;
	m_pPerformanceCounters[ eD9S_NumPrimsRendered ] += (int)PrimitiveCount;
#endif // EnablePerformanceCounters

	return m_sD3DDev9.pD3DDevice->DrawIndexedPrimitiveUP( 
				PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, 
				pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride );  //Fix memory leak -Imago 8/2/09
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawIndexedPrimitive()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::DrawPrimitiveUP(	D3DPRIMITIVETYPE PrimitiveType, 
										UINT PrimitiveCount,
										CONST void * pVertexStreamZeroData,
										UINT VertexStreamZeroStride )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );

#ifdef EnablePerformanceCounters
	m_pPerformanceCounters[ eD9S_NumDrawPrims ] ++;
	m_pPerformanceCounters[ eD9S_NumPrimsRendered ] += (int)PrimitiveCount;
#endif // EnablePerformanceCounters

	return m_sD3DDev9.pD3DDevice->DrawPrimitiveUP( PrimitiveType, PrimitiveCount, 
				pVertexStreamZeroData, VertexStreamZeroStride );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// ColorFill()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::ColorFill( LPDIRECT3DSURFACE9 pSurface, CONST RECT * pRect, D3DCOLOR color )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->ColorFill( pSurface, pRect, color );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetGammaRamp()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CD3DDevice9::Clear( DWORD Count, CONST D3DRECT * pRects, DWORD Flags, 
							D3DCOLOR Color, float Z, DWORD Stencil )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	return m_sD3DDev9.pD3DDevice->Clear( Count, pRects, Flags, Color, Z, Stencil );
}

	
	
////////////////////////////////////////////////////////////////////////////////////////////////////
// SetGammaRamp()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::SetGammaRamp( UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP * pRamp )
{
    ZAssert( m_sD3DDev9.bInitialised == true );
    ZAssert( m_sD3DDev9.pD3DDevice != NULL );
	m_sD3DDev9.pD3DDevice->SetGammaRamp( iSwapChain, Flags, pRamp );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Performance statistics gathering.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef EnablePerformanceCounters

////////////////////////////////////////////////////////////////////////////////////////////////////
// ResetPerformanceCounters()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDevice9::ResetPerformanceCounters()
{
	m_pPerformanceCounters[ eD9S_NumDrawPrims ] = 0;
	m_pPerformanceCounters[ eD9S_NumPrimsRendered ] = 0;
	m_pPerformanceCounters[ eD9S_NumStateChanges ] = 0;
	m_pPerformanceCounters[ eD9S_NumTextureChanges ] = 0;
	m_pPerformanceCounters[ eD9S_NumShaderChanges ] = 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetPerformanceCounter()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CD3DDevice9::GetPerformanceCounter(EDevice9Stat stat)
{
	return m_pPerformanceCounters[ stat ];
}

#endif // EnablePerformanceCounters
