
#include "pch.h"
// BUILD_DX9

////////////////////////////////////////////////////////////////////////////////////////////////////
// Allows user to select a valid video mode.
// 
// TBD:
// Add registry keys to store the video settings.
////////////////////////////////////////////////////////////////////////////////////////////////////

// Imago activated this, we choose suitable defaults and let pilots change options in-game
#define USE_DEFAULT_SETTINGS			// For PIX...

// NEEDS TO BE LANGUAGE DEPENDENT IF WE GET THAT FAR.
#include "..\Lang\USA\allegiance\Resource.h"

#include "DeviceModesDX9.h"

// kg - registry key is passed as parameter
// #define HKLM_3DSETTINGS ALLEGIANCE_REGISTRY_KEY_ROOT "\\3DSettings"


struct SAdditional3DRegistryData
{
	char szDeviceName[256];
	char szResolutionName[256];
	char szAASettingName[256];
};

int GetMaxRate(int index); //imago 7/27/09

////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ResPickerDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ProgressBarDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

////////////////////////////////////////////////////////////////////////////////////////////////////
void SetModeData( HWND hDlg, int iCurrentDevice );

////////////////////////////////////////////////////////////////////////////////////////////////////
void SetAAData( HWND hDlg, int iCurrentDevice, int iCurrentMode, bool bWindowed );

////////////////////////////////////////////////////////////////////////////////////////////////////
void SetGFXSettings( HWND hwndDlg, int iMaxTextureSize, bool bAutoGenMipmaps );

////////////////////////////////////////////////////////////////////////////////////////////////////
void SetDataSettings( HWND hwndDlg, bool bUseTexturePackFile );

////////////////////////////////////////////////////////////////////////////////////////////////////
int Read3DRegistrySettings( SAdditional3DRegistryData * pRegData, LPCSTR lpSubKey, CLogFile * pLogFile );

////////////////////////////////////////////////////////////////////////////////////////////////////
int Write3DRegistrySettings( LPCSTR lpSubKey );

////////////////////////////////////////////////////////////////////////////////////////////////////
bool SetupTexturePackFile( HINSTANCE hInstance, const char * szDataPath, char * szPackFileName );

////////////////////////////////////////////////////////////////////////////////////////////////////
struct SVideoSettingsData
{
	CD3DDeviceModeData *	pDevData;
	int						iCurrentDevice;
	int						iCurrentMode;
	int						iCurrentAASetting;

	// Selected.
	int						iNumResolutions;
	int						iSelectedResolution;
	CD3DDeviceModeData::SVideoResolution * pResolutionSet;

	D3DFORMAT				d3dBackBufferFormat;			
	D3DFORMAT				d3dDeviceFormat;
	HMONITOR				hSelectedMonitor;

	// Device settings.
	bool					bWindowed;
	bool					bWaitForVSync;
	D3DMULTISAMPLE_TYPE		multiSampleType;				// Antialiasing type.
	D3DTEXTUREFILTERTYPE	magFilter, minFilter, mipFilter;

	// Graphics settings.
	int						iMaxTextureSize;
	bool					bAutoGenMipmaps;

	// Data settings.
	bool					bUseTexturePackFile;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
SVideoSettingsData g_VideoSettings;



////////////////////////////////////////////////////////////////////////////////////////////////////
// PromptUserForVideoSettings()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool PromptUserForVideoSettings(bool bStartFullscreen, bool bRaise, int iAdapter, HINSTANCE hInstance, PathString & szArtPath, ZString lpSubKey )
{
	CLogFile logFile( "VideoSettings.log" );

	memset( &g_VideoSettings, 0, sizeof( SVideoSettingsData ) );

	// Obtain mode information for this system.
	g_VideoSettings.iCurrentDevice		= 0;			// Set to 1 for multi-monitor debugging.
	g_VideoSettings.iCurrentAASetting	= 0;
	g_VideoSettings.bWindowed			= !bStartFullscreen;
	g_VideoSettings.iCurrentMode		= g_VideoSettings.bWindowed;
	g_VideoSettings.bWaitForVSync		= false; //Spunky #265 backing out //Imago 7/10
	
// DEFAULT SETTINGS W/O DIALOG imago 6/29/09 - 7/2/09 - 7/19/09
//////////////////////////////////////////
	if (bRaise == false) {
		int iRetVal = 1;
		int idummy = 0;
    	int x = 800;
		int y = 600;

		g_VideoSettings.bWaitForVSync		= false;
		g_VideoSettings.bAutoGenMipmaps 	= false;
		g_VideoSettings.bUseTexturePackFile = false;
		g_VideoSettings.multiSampleType 	= D3DMULTISAMPLE_NONE;
		g_VideoSettings.iMaxTextureSize		= 0;

		//NYI have a texture filter toggle for D3DTEXF_ANISOTROPIC/LINEAR, check if D3DPTFILTERCAPS_*FANISOTROPIC is set
		g_VideoSettings.magFilter 			= D3DTEXF_LINEAR; 
		g_VideoSettings.minFilter 			= D3DTEXF_LINEAR;
		g_VideoSettings.mipFilter 			= D3DTEXF_LINEAR;

		D3DFORMAT bbf = D3DFMT_UNKNOWN;
		D3DFORMAT df = D3DFMT_UNKNOWN;
		
		HMONITOR hMon = MonitorFromPoint(Point(0,0), MONITOR_DEFAULTTOPRIMARY);
		HKEY hKey;
		//load preferences when not using dialog
		if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, KEY_READ, &hKey))
        {
            DWORD dwSize = 4;
            DWORD dwType = REG_DWORD;

            ::RegQueryValueEx(hKey, "CombatFullscreenXSize", NULL, &dwType, (BYTE*)&x, &dwSize);
			::RegQueryValueEx(hKey, "CombatFullscreenYSize", NULL, &dwType, (BYTE*)&y, &dwSize);
			::RegQueryValueEx(hKey, "UseAntialiasing", NULL, &dwType, (BYTE*)&g_DX9Settings.m_dwAA, &dwSize);
			::RegQueryValueEx(hKey, "UseAutoMipMaps", NULL, &dwType, (BYTE*)&g_VideoSettings.bAutoGenMipmaps, &dwSize);
			::RegQueryValueEx(hKey, "UseTexturePack", NULL, &dwType, (BYTE*)&g_VideoSettings.bUseTexturePackFile, &dwSize);
			::RegQueryValueEx(hKey, "UseVSync", NULL, &dwType, (BYTE*)&g_VideoSettings.bWaitForVSync, &dwSize);
			::RegQueryValueEx(hKey, "MaxTextureSize", NULL, &dwType, (BYTE*)&g_VideoSettings.iMaxTextureSize, &dwSize);
			//::RegQueryValueEx(hKey, "UseAnisotropic", NULL, &dwType, (BYTE*)&idummy, &dwSize); NYI
			//::RegQueryValueEx(hKey, "AAQuality", NULL, &dwType, (BYTE*)&idummy, &dwSize); NYI
            ::RegCloseKey(hKey);
        }
		x = (x != 0) ? x : 800; y = (y != 0) ? y : 600; //in case Windows7 wrote 0's before this bug was caught

		lpSubKey += ZString("\\3DSettings");
		g_VideoSettings.pDevData			= new CD3DDeviceModeData( 800, 600 , &logFile);	// Mininum ENGINE width/height allowed.
		g_VideoSettings.pDevData->GetResolutionDetails(iAdapter,0,&idummy,&idummy,&g_DX9Settings.m_refreshrate,&bbf,&df,&hMon); //imago use this function!
		// If default device has no available modes, can't run the game.
		if( g_VideoSettings.pDevData->GetTotalResolutionCount( 0 ) == 0 )
		{
			logFile.OutputString( "Primary device has no modes that support Allegiance.\n" );
			MessageBox( NULL, "Primary device has no modes that support Allegiance.",
							"Error", MB_OK );
			return false;
		}
		g_VideoSettings.iCurrentDevice		= iAdapter;  // -adapter <n>     
		g_VideoSettings.d3dBackBufferFormat = (!bbf || bbf == D3DFMT_UNKNOWN) ? D3DFMT_X8R8G8B8 : bbf;
		g_VideoSettings.d3dDeviceFormat		= (!df || df == D3DFMT_UNKNOWN) ? D3DFMT_X8R8G8B8 : df;
		g_VideoSettings.hSelectedMonitor	= hMon; //use primary monitor on speicfied adapter


		//NYI multimon: zlib/window is causing issues, moving our window incorrectly when
		//the multimon layout is not Left to Right or Top to Bottom
		//	(i.e.  [1] [2] works fine,  [2] [1] does not
		//  [1]				[2]
		//	[2] works fine,	[1] does not)

		//NYI -monitor <n> for "Multi-Head" adapters

		// Find the optimal mode index using the highest, sane refresh rate 7/27/09 (Imago, Sgt_Baker)
		int iBestMode = 0;
		int maxrate = GetMaxRate(iAdapter);

		int iModeCount = g_VideoSettings.pDevData->GetResolutionCount(iAdapter,g_VideoSettings.d3dDeviceFormat);
		for( int i=0; i<iModeCount; i++ ) {
			int myx, myy, myrate;
			D3DFORMAT mybbf, mydf; 
			HMONITOR mymon;
			g_VideoSettings.pDevData->GetResolutionDetails(iAdapter,i,&myx,&myy,&myrate,&mybbf,&mydf,&mymon);
			if (g_VideoSettings.d3dDeviceFormat == mydf && g_VideoSettings.d3dBackBufferFormat == mybbf) {
				if (x == myx && y == myy) {
					if (myrate >= g_DX9Settings.m_refreshrate && myrate <= maxrate)
						iBestMode = i;
				}
			}
		}
		_ASSERT(iModeCount != 0);
		//imago build the adapter res array for in-game switching
		g_VideoSettings.pDevData->GetRelatedResolutions(
											iAdapter,
											iBestMode,
											&g_VideoSettings.iNumResolutions,
											&g_VideoSettings.iCurrentMode,
											&g_VideoSettings.pResolutionSet );
		g_VideoSettings.iCurrentMode = iBestMode;

        logFile.OutputStringV("\n\nFOUND RESOLUTIONS (MODE %i) (MAXRATE: %i):\n",iBestMode,maxrate);
		for (int i=0;i<g_VideoSettings.iNumResolutions;i++) {
            logFile.OutputStringV("\t%i) %ix%i @ %i\n",i,g_VideoSettings.pResolutionSet[i].iWidth,g_VideoSettings.pResolutionSet[i].iHeight,
				g_VideoSettings.pResolutionSet[i].iFreq);
			if (g_VideoSettings.pResolutionSet[i].iWidth == x && g_VideoSettings.pResolutionSet[i].iHeight == y)
				g_DX9Settings.m_refreshrate = g_VideoSettings.pResolutionSet[i].iFreq;
		}
        logFile.OutputStringV("CURRENT IN-GAME RESOLUTION: %ix%i @ %i\n",x,y,g_DX9Settings.m_refreshrate);

		//lets make extra sure we don't crash when we autoload AA settings
		LPDIRECT3D9 pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
		if (g_DX9Settings.m_dwAA >= 1 && g_DX9Settings.m_dwAA < 4) // #113 7/10 Imago
			if (pD3D9->CheckDeviceMultiSampleType(iAdapter, D3DDEVTYPE_HAL, g_VideoSettings.d3dBackBufferFormat, 
				g_VideoSettings.bWindowed, D3DMULTISAMPLE_2_SAMPLES, NULL) == D3D_OK) {
				g_VideoSettings.iCurrentAASetting = 1;
                g_DX9Settings.m_dwAA = 2;
			} else {
				g_DX9Settings.m_dwAA = 0;
			}
		if (g_DX9Settings.m_dwAA >= 4)
			if (pD3D9->CheckDeviceMultiSampleType(iAdapter, D3DDEVTYPE_HAL, g_VideoSettings.d3dBackBufferFormat, 
				g_VideoSettings.bWindowed, D3DMULTISAMPLE_4_SAMPLES, NULL) == D3D_OK) {
				g_VideoSettings.iCurrentAASetting = 2;
                g_DX9Settings.m_dwAA = 4;
			} else {
				g_VideoSettings.iCurrentAASetting = 1;
				g_DX9Settings.m_dwAA = 2;
			}
		if (g_DX9Settings.m_dwAA >= 6)
			if (pD3D9->CheckDeviceMultiSampleType(iAdapter, D3DDEVTYPE_HAL, g_VideoSettings.d3dBackBufferFormat, 
				g_VideoSettings.bWindowed, D3DMULTISAMPLE_6_SAMPLES, NULL) == D3D_OK) {
				g_VideoSettings.iCurrentAASetting = 3;
                g_DX9Settings.m_dwAA = 6;
			} else {
				g_VideoSettings.iCurrentAASetting = 2;
				g_DX9Settings.m_dwAA = 4;
			}
		if (g_DX9Settings.m_dwAA >= 8)
			if (pD3D9->CheckDeviceMultiSampleType(iAdapter, D3DDEVTYPE_HAL, g_VideoSettings.d3dBackBufferFormat, 
				g_VideoSettings.bWindowed, D3DMULTISAMPLE_8_SAMPLES, NULL) == D3D_OK) {
				g_VideoSettings.iCurrentAASetting = 4;
                g_DX9Settings.m_dwAA = 8;
			} else {
				g_VideoSettings.iCurrentAASetting = 3;
				g_DX9Settings.m_dwAA = 6;
			}
		if (g_DX9Settings.m_dwAA >= 16)
			if (pD3D9->CheckDeviceMultiSampleType(iAdapter, D3DDEVTYPE_HAL, g_VideoSettings.d3dBackBufferFormat, 
				g_VideoSettings.bWindowed, D3DMULTISAMPLE_16_SAMPLES, NULL) == D3D_OK) {
				g_VideoSettings.iCurrentAASetting = 5;
                g_DX9Settings.m_dwAA = 16;
			} else {
				g_VideoSettings.iCurrentAASetting = 4;
				g_DX9Settings.m_dwAA = 8;
			}
		pD3D9->Release();

//////////////////////////////////////////
	} else { // BEGIN USE_DIALOG_SETTINGS
		lpSubKey += ZString("\\3DSettings");
		SAdditional3DRegistryData sExtraRegData;

		g_VideoSettings.pDevData = new CD3DDeviceModeData( 800, 600, &logFile );	// Mininum width/height allowed.

		// If default device has no available modes, can't run the game.
		if( g_VideoSettings.pDevData->GetTotalResolutionCount( 0 ) == 0 )
		{
			logFile.OutputString( "Primary device has no modes that support Allegiance.\n" );
			MessageBox( NULL, "Primary device has no modes that support Allegiance.",
							"Error", MB_OK );
			return false;
		}

		int iRetVal;
		bool bValid = true;

		// Load in the registry settings.
		iRetVal = Read3DRegistrySettings( &sExtraRegData, lpSubKey, &logFile );
		if( iRetVal == 1 )
		{
			// Values were loaded from the registry key, validate them.
			// Loaded settings: iCurrentDevice, szDeviceName, iCurrentMode, szResolutionName
			// bWindowed, bWaitForVSync, szAASettingName, iCurrentAASetting, magFilter, 
			// minFilter, mipFilter.
			// Assumption, if current device, device name, current mode and resolution name
			// are valid, assume the rest of the settings must be ok.
			if( ( g_VideoSettings.iCurrentDevice >= 0 ) &&
				( g_VideoSettings.iCurrentDevice < g_VideoSettings.pDevData->GetDeviceCount() ) )
			{
				char szBuffer[256];
				g_VideoSettings.pDevData->GetDeviceNameByIndex( 
							g_VideoSettings.iCurrentDevice,
							szBuffer,
							256 );
				if( strcmp( sExtraRegData.szDeviceName, szBuffer ) != 0 )
				{
					bValid = false;
				}
				else if(	( g_VideoSettings.iCurrentMode >= 0 ) &&
							( g_VideoSettings.iCurrentMode < 
									g_VideoSettings.pDevData->GetTotalResolutionCount( 
											g_VideoSettings.iCurrentDevice ) ) )
				{
					g_VideoSettings.pDevData->GetResolutionStringByIndex( 
						g_VideoSettings.iCurrentDevice,
						g_VideoSettings.iCurrentMode, 
						szBuffer, 256 );
					if( strcmp( sExtraRegData.szResolutionName, szBuffer ) != 0 )
					{
						bValid = false;
					}
				}
			}
			else
			{
				bValid = false;
			}
		}

		if( bValid == false )
		{
			CD3DDeviceModeData * pTemp = g_VideoSettings.pDevData;
			memset( &g_VideoSettings, 0, sizeof( SVideoSettingsData ) );
			g_VideoSettings.pDevData = pTemp;
		}

		// Create Dialog box, then populate with video settings.
		iRetVal = DialogBox(	hInstance, 
								MAKEINTRESOURCE( IDD_RESPICKER ), 
								GetDesktopWindow(), 
								ResPickerDialogProc );
		if( iRetVal == IDCANCEL ) 
		{
			// User selected quit.
			return false;
		}
	} // END USE_DIALOG_SETTINGS
//////////////////////////////////////////

	logFile.OutputString("\nUser selected values:\n");
	logFile.OutputStringV( "AD %d   MON 0x%08x   WIN %d   VSYNC %d\n", 
		g_VideoSettings.iCurrentDevice,
		g_VideoSettings.hSelectedMonitor,
		g_VideoSettings.bWindowed,
		g_VideoSettings.bWaitForVSync );

	// Set the static setup params.
	CD3DDevice9 * pDev = CD3DDevice9::Get();
	CD3DDevice9::SD3DDeviceSetupParams * pParams = pDev->GetDeviceSetupParams();

	pParams->iAdapterID			= g_VideoSettings.iCurrentDevice;
	pParams->hMonitor			= g_VideoSettings.hSelectedMonitor;
	pParams->monitorInfo.cbSize	= sizeof( MONITORINFOEX );
	pParams->bRunWindowed		= g_VideoSettings.bWindowed;
	pParams->bWaitForVSync		= g_VideoSettings.bWaitForVSync;
	g_VideoSettings.pDevData->GetModeParams(	pParams,
												g_VideoSettings.iCurrentDevice,
												g_VideoSettings.iCurrentMode,
												g_VideoSettings.iCurrentAASetting,
												&logFile );

	//this is where we override Refresh Rate, AA and Texture Filter (NYI) settings when not using the dialog (Imago)
	if (!bRaise)
		pParams->sFullScreenMode.mode.RefreshRate = g_DX9Settings.m_refreshrate; //refresh rate hack 7/2/09
	//

	
	if( GetMonitorInfo( g_VideoSettings.hSelectedMonitor, &pParams->monitorInfo ) == FALSE )
	{
		// Failed to get valid monitor data.
		logFile.OutputString( "ERROR: failed to get valid monitor info.\n" );
		return false;
	}

	// Offset for windowed mode. - (multimon NYI)
	pParams->iWindowOffsetX	= pParams->monitorInfo.rcMonitor.left;
	pParams->iWindowOffsetY	= pParams->monitorInfo.rcMonitor.top;
	logFile.OutputStringV( "Window offset: %d  %d\n", pParams->iWindowOffsetX,
								pParams->iWindowOffsetY );

	// Copy over the resolution array.
	pParams->iNumRes = g_VideoSettings.iNumResolutions;
	pParams->iCurrentRes = g_VideoSettings.iSelectedResolution;
	pParams->pFullScreenResArray = new CD3DDevice9::SD3DVideoResolution[ g_VideoSettings.iNumResolutions ];

	int i;
	for( i=0; i<pParams->iNumRes; i++ )
	{
		pParams->pFullScreenResArray[i].iWidth = g_VideoSettings.pResolutionSet[i].iWidth;
		pParams->pFullScreenResArray[i].iHeight = g_VideoSettings.pResolutionSet[i].iHeight;
		pParams->pFullScreenResArray[i].iFreq = g_VideoSettings.pResolutionSet[i].iFreq;
	}

	// Store filter details. 
	g_VideoSettings.minFilter = pParams->d3dMinFilter;
	g_VideoSettings.magFilter = pParams->d3dMagFilter;
	g_VideoSettings.mipFilter = pParams->d3dMipFilter;

	// Graphics settings.
	pParams->maxTextureSize = (CD3DDevice9::EMaxTextureSize) g_VideoSettings.iMaxTextureSize;
	pParams->bAutoGenMipmap = g_VideoSettings.bAutoGenMipmaps;
	pParams->dwMaxTextureSize = 256 << g_VideoSettings.iMaxTextureSize;

	// Data settings go into the engine settings object.
	g_DX9Settings.mbUseTexturePackFiles = g_VideoSettings.bUseTexturePackFile;
	g_DX9Settings.m_bVSync = g_VideoSettings.bWaitForVSync; //imago 7/18/09
	g_DX9Settings.m_iMaxTextureSize = g_VideoSettings.iMaxTextureSize; //imago 7/18/09
	g_DX9Settings.m_bAutoGenMipmaps = g_VideoSettings.bAutoGenMipmaps; //imago 7/18/09


	// Update the registry settings.
	Write3DRegistrySettings( lpSubKey);

	// Now handle pack file stuff.
	if( g_DX9Settings.mbUseTexturePackFiles == true )
	{
		SetupTexturePackFile( hInstance, &szArtPath[0], "CommonTextures" );
	}

	// Tidy up.
	if( g_VideoSettings.pDevData != NULL )
	{
		delete g_VideoSettings.pDevData;
		g_VideoSettings.pDevData = NULL;
	}
	return true;			// Success.
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetModeData()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetModeData( HWND hDlg, int iCurrentDevice )
{
	char szBuffer[256];
	HWND hControl;
	int i, iCount;

	// Get handle of drop down menu.
	hControl = GetDlgItem( hDlg, IDC_RESDROPDOWN );

	// Reset list contents.
	SendMessage( hControl, CB_RESETCONTENT, 0, 0 );

	iCount = g_VideoSettings.pDevData->GetTotalResolutionCount( iCurrentDevice );
	for( i=0; i<iCount; i++ )
	{
		g_VideoSettings.pDevData->GetResolutionStringByIndex( iCurrentDevice, i, szBuffer, 256 );
		SendMessage( hControl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR) szBuffer ) );
	}
	SendMessage( hControl, CB_SETCURSEL, g_VideoSettings.iCurrentMode, 0 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetAAData()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetAAData( HWND hDlg, int iCurrentDevice, int iCurrentMode, bool bWindowed )
{
	char szBuffer[256];
	HWND hControl;
	int i, iCount;

	// Get handle of drop down menu.
	hControl = GetDlgItem( hDlg, IDC_ANTIALIAS );

	// Reset list contents.
	SendMessage( hControl, CB_RESETCONTENT, 0, 0 );

	iCount = g_VideoSettings.pDevData->GetNumAASettings(
		iCurrentDevice,
		iCurrentMode,
		bWindowed );

	for( i=0; i<iCount; i++ )
	{
		g_VideoSettings.pDevData->GetAASettingString( 
			iCurrentDevice, iCurrentMode, bWindowed, i, szBuffer, 256 );
		SendMessage( hControl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR) szBuffer ) );
	}

	// Clamp the selection value.
	if( g_VideoSettings.iCurrentAASetting > iCount )
	{
		g_VideoSettings.iCurrentAASetting = iCount - 1;
	}
	SendMessage( hControl, CB_SETCURSEL, g_VideoSettings.iCurrentAASetting, 0 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetGFXSettings()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetGFXSettings( HWND hwndDlg, int iMaxTextureSize, bool bAutoGenMipmaps )
{
	int i;
	HWND hControl;

	// Get handle of drop down menu.
	hControl = GetDlgItem( hwndDlg, IDC_MAXTEXTURESIZE );

	for( i=0; i<CD3DDevice9::eMTS_NumTextureSizes; i++ )
	{
		SendMessage( hControl, CB_ADDSTRING, 0, 
			reinterpret_cast<LPARAM>((LPCTSTR) g_VideoSettings.pDevData->GetMaxTextureString( i ) ) );
	}
	SendMessage( hControl, CB_SETCURSEL, iMaxTextureSize, 0 );

	hControl = GetDlgItem( hwndDlg, IDC_AUTOGENMIPMAPS );
	SendMessage( hControl, BM_SETCHECK, (bAutoGenMipmaps == true ) ? BST_CHECKED : BST_UNCHECKED, 0 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// SetDataSettings()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetDataSettings( HWND hwndDlg, bool bUseTexturePackFile )
{
	HWND hControl;

	// Set data params.
	hControl = GetDlgItem( hwndDlg, IDC_USETEXTUREPACK );
	SendMessage( hControl, BM_SETCHECK, (bUseTexturePackFile == true ) ? BST_CHECKED : BST_UNCHECKED, 0 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetModeData()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool GetModeData( HWND hDlg, bool bWriteToRegistry )
{
	HWND	hControl;
	int		iCurrentMode, iSelWidth, iSelHeight, iSelFreq;
	bool	bWindowed, bWaitForVSync;

	// Get handle of drop down menu and get selected mode.
	hControl		= GetDlgItem( hDlg, IDC_RESDROPDOWN );
	iCurrentMode	= SendMessage( hControl, CB_GETCURSEL, 0, 0 );

	// Get check box values.
	hControl		= GetDlgItem( hDlg, IDC_WINDOWED );
	bWindowed		= ( SendMessage( hControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) ? true : false;
	hControl		= GetDlgItem( hDlg, IDC_VSYNC );
	bWaitForVSync	= ( SendMessage( hControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) ? true : false;

	// Validate the current settings.
	if(	g_VideoSettings.pDevData->ValidateSettings(	g_VideoSettings.iCurrentDevice,
													iCurrentMode,
													bWindowed ) == 0 )
	{
		// Get the rest of the settings from the device data holder.
		g_VideoSettings.iCurrentMode	= iCurrentMode;
		g_VideoSettings.bWindowed		= bWindowed;
		g_VideoSettings.bWaitForVSync	= bWaitForVSync;

		// For the specified device and mode, retrieve the resolution settings.
		g_VideoSettings.pDevData->GetResolutionDetails(		g_VideoSettings.iCurrentDevice,
															g_VideoSettings.iCurrentMode,
															&iSelWidth,
															&iSelHeight,
															&iSelFreq,
															&g_VideoSettings.d3dBackBufferFormat,
															&g_VideoSettings.d3dDeviceFormat,
															&g_VideoSettings.hSelectedMonitor );

		// Now get the rest of the full screen resolutions we'll be supporting.
		// Returned value includes the selected resolution.
		g_VideoSettings.pDevData->GetRelatedResolutions(
										g_VideoSettings.iCurrentDevice,
										g_VideoSettings.iCurrentMode,
										&g_VideoSettings.iNumResolutions,
										&g_VideoSettings.iSelectedResolution,
										&g_VideoSettings.pResolutionSet );

		// Get the graphics settings.
		hControl = GetDlgItem( hDlg, IDC_AUTOGENMIPMAPS );
		g_VideoSettings.bAutoGenMipmaps = ( SendMessage( hControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) ? true : false;
		hControl = GetDlgItem( hDlg, IDC_MAXTEXTURESIZE );
		g_VideoSettings.iMaxTextureSize = SendMessage( hControl, CB_GETCURSEL, 0, 0 );

		// Get the data settings.
		hControl = GetDlgItem( hDlg, IDC_USETEXTUREPACK );
		g_VideoSettings.bUseTexturePackFile = ( SendMessage( hControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) ? true : false;

		return true;
	}
	return false;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// DialogProc()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ResPickerDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	HWND hControl;
	switch( uMsg )
	{
	case WM_INITDIALOG:
		{
			// Populate the controls.
			char szBuffer[256];
			int i;
			
			// Populate the device drop down.
			hControl = GetDlgItem( hwndDlg, IDC_DEVDROPDOWN );
			for( i=0; i<g_VideoSettings.pDevData->GetDeviceCount(); i++ )
			{
				g_VideoSettings.pDevData->GetDeviceNameByIndex( i, szBuffer, 256 );
				SendMessage( hControl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR) szBuffer ) );
			}
			
			// Make the first option, the selected one. Once reg key stuff set up, make the last used device the selected one.
			if( g_VideoSettings.pDevData->GetDeviceCount() > 0 )
			{
				SendMessage( hControl, CB_SETCURSEL, g_VideoSettings.iCurrentDevice, 0 );
			
				// Add display modes for current selection.
				SetModeData( hwndDlg, g_VideoSettings.iCurrentDevice );

				// Add AA settings.
				SetAAData( hwndDlg, g_VideoSettings.iCurrentDevice, 
					g_VideoSettings.iCurrentMode, g_VideoSettings.bWindowed );

				// Set the windowed flag.
				hControl = GetDlgItem( hwndDlg, IDC_WINDOWED );
				SendMessage( hControl, BM_SETCHECK, (
					g_VideoSettings.bWindowed == true ) ? BST_CHECKED : BST_UNCHECKED, 0 );

				// Set the vertical sync flag.
				hControl = GetDlgItem( hwndDlg, IDC_VSYNC );
				SendMessage( hControl, BM_SETCHECK, (
					g_VideoSettings.bWaitForVSync == true ) ? BST_CHECKED : BST_UNCHECKED, 0 );

				// Set graphics settings.
				SetGFXSettings( hwndDlg, g_VideoSettings.iMaxTextureSize, g_VideoSettings.bAutoGenMipmaps );

				// Set data settings.
				SetDataSettings( hwndDlg, g_VideoSettings.bUseTexturePackFile );
			}

			// Centre the dialog box in the middle of the desktop.
			RECT dlgRect, desktopRect;
			int iNewXPos, iNewYPos;
			GetWindowRect( hwndDlg, &dlgRect );
			GetWindowRect( GetDesktopWindow(), &desktopRect );
			iNewXPos = (( desktopRect.right - desktopRect.left ) / 2 ) - (( dlgRect.right - dlgRect.left ) / 2 );
			iNewYPos = (( desktopRect.bottom - desktopRect.top ) / 2 ) - (( dlgRect.bottom - dlgRect.top ) / 2 );

			SetWindowPos( hwndDlg, HWND_TOP, iNewXPos, iNewYPos, 0, 0, SWP_NOSIZE );
		}
		break;

    case WM_COMMAND: 
        switch (LOWORD(wParam)) 
        { 
            case IDOK: 
				// Update parameters.
				if( GetModeData( hwndDlg, true ) == true )
				{
	                EndDialog(hwndDlg, wParam); 
					return TRUE;
				}
				MessageBox( hwndDlg, "Selected mode could not be validated.", "Invalid mode selection", MB_OK );
				return FALSE;			// Do nothing.

            case IDCANCEL: 
				GetModeData( hwndDlg, false );
                EndDialog(hwndDlg, wParam); 
                return TRUE;

        }
		switch ( HIWORD ( wParam ) )
		{
		case CBN_SELCHANGE:
			// Change of device?
			hControl = GetDlgItem( hwndDlg, IDC_DEVDROPDOWN );
			if( LOWORD(wParam) == GetDlgCtrlID( hControl ) )
			{
				g_VideoSettings.iCurrentDevice = SendMessage( hControl, CB_GETCURSEL, 0, 0 );
				_ASSERT( g_VideoSettings.iCurrentDevice != CB_ERR );

				// Set the mode data for the new device.
				SetModeData( hwndDlg, g_VideoSettings.iCurrentDevice );
				return FALSE;
			}
			// Change of resolution?
			hControl = GetDlgItem( hwndDlg, IDC_RESDROPDOWN );
			if( LOWORD(wParam) == GetDlgCtrlID( hControl ) )
			{
				g_VideoSettings.iCurrentMode = SendMessage( hControl, CB_GETCURSEL, 0, 0 );
				_ASSERT( g_VideoSettings.iCurrentMode != CB_ERR );

				SetAAData(	hwndDlg,
							g_VideoSettings.iCurrentDevice,
							g_VideoSettings.iCurrentMode,
							g_VideoSettings.bWindowed );
				return FALSE;
			}
			// Change of FSAA setting?
			hControl = GetDlgItem( hwndDlg, IDC_ANTIALIAS );
			if( LOWORD(wParam) == GetDlgCtrlID( hControl ) )
			{
				g_VideoSettings.iCurrentAASetting = SendMessage( hControl, CB_GETCURSEL, 0, 0 );
				_ASSERT( g_VideoSettings.iCurrentAASetting != CB_ERR );
				return FALSE;
			}
			return FALSE;

		case BN_CLICKED:
			hControl = GetDlgItem( hwndDlg, IDC_WINDOWED );
			if( LOWORD(wParam) == GetDlgCtrlID( hControl ) )
			{
				// Update the state.
				if( SendMessage( hControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
				{
					g_VideoSettings.bWindowed = true;
				}	
				else
				{
					g_VideoSettings.bWindowed = false;
				}
				SetAAData(	hwndDlg,
							g_VideoSettings.iCurrentDevice,
							g_VideoSettings.iCurrentMode,
							g_VideoSettings.bWindowed );
			}
			break;
		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Read3DRegistrySettings()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int Read3DRegistrySettings( SAdditional3DRegistryData * pRegData, LPCSTR lpSubKey, CLogFile * pLogFile )
{
	HKEY hKey;
	int iRetVal = 0;
	DWORD dwDataSize, dwBoolValue;
	if( ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER,
											lpSubKey,
											0,
											KEY_READ,
											&hKey ) )
	{
		pLogFile->OutputString("\nReading previous user selected values from registry:\n");

		// Load the values.
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "DeviceIndex", 0, 0, 
			(LPBYTE) &g_VideoSettings.iCurrentDevice, &dwDataSize );
		dwDataSize = 256;
		::RegQueryValueEx( hKey, "DeviceName", 0, 0,
			(LPBYTE) pRegData->szDeviceName, &dwDataSize );

		pLogFile->OutputStringV( "Device index %d:  %s\n", g_VideoSettings.iCurrentDevice, pRegData->szDeviceName );

		dwDataSize = 4;
		::RegQueryValueEx( hKey, "ModeIndex", 0, 0, 
			(LPBYTE) &g_VideoSettings.iCurrentMode, &dwDataSize );
		dwDataSize = 256;
		::RegQueryValueEx( hKey, "ModeName", 0, 0,
			(LPBYTE) pRegData->szResolutionName, &dwDataSize );
		
		pLogFile->OutputStringV( "Mode index %d:  %s\n", g_VideoSettings.iCurrentMode, pRegData->szResolutionName );

		dwDataSize = 4;
		::RegQueryValueEx( hKey, "Windowed", 0, 0, 
			(LPBYTE) &dwBoolValue, &dwDataSize );
		g_VideoSettings.bWindowed = ( dwBoolValue == 0 ) ? false : true;
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "WaitForVSync", 0, 0, 
			(LPBYTE) &dwBoolValue, &dwDataSize );
		g_VideoSettings.bWaitForVSync = ( dwBoolValue == 0 ) ? false : true;

		pLogFile->OutputStringV( "WIN: %d   VSYNC: %d\n", g_VideoSettings.bWindowed, g_VideoSettings.bWaitForVSync );

		dwDataSize = 256;
		::RegQueryValueEx( hKey, "AAName", 0, 0,
			(LPBYTE) pRegData->szAASettingName, &dwDataSize );
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "AAIndex", 0, 0, 
			(LPBYTE) &g_VideoSettings.iCurrentAASetting, &dwDataSize );

		pLogFile->OutputStringV( "AA %d:  %s\n", g_VideoSettings.iCurrentAASetting, pRegData->szAASettingName );

		dwDataSize = 4;
		::RegQueryValueEx( hKey, "MagFilter", 0, 0, 
			(LPBYTE) &g_VideoSettings.magFilter, &dwDataSize );
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "MinFilter", 0, 0, 
			(LPBYTE) &g_VideoSettings.minFilter, &dwDataSize );
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "MipFilter", 0, 0, 
			(LPBYTE) &g_VideoSettings.mipFilter, &dwDataSize );

		pLogFile->OutputStringV( "MAG: %d   MIN: %d   MIP: %d\n", 
			g_VideoSettings.magFilter, g_VideoSettings.minFilter, g_VideoSettings.mipFilter );

		dwDataSize = 4;
		::RegQueryValueEx( hKey, "MaxTextureSize", 0, 0, 
			(LPBYTE) &g_VideoSettings.iMaxTextureSize, &dwDataSize );
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "AutoGenMipmaps", 0, 0, 
			(LPBYTE) &dwBoolValue, &dwDataSize );
		g_VideoSettings.bAutoGenMipmaps = ( dwBoolValue == 0 ) ? false : true;
		dwDataSize = 4;
		::RegQueryValueEx( hKey, "UseTexturePackFile", 0, 0, 
			(LPBYTE) &dwBoolValue, &dwDataSize );
		g_VideoSettings.bUseTexturePackFile = ( dwBoolValue == 0 ) ? false : true;

		pLogFile->OutputStringV( "MTS: %d   AGMIP: %d   PACK: %d\n", 
			g_VideoSettings.iMaxTextureSize, g_VideoSettings.bAutoGenMipmaps, g_VideoSettings.bUseTexturePackFile );

		iRetVal = 1;		// Read existing key.
		RegCloseKey(hKey);
	}
	return iRetVal;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Write3DRegistrySettings()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int Write3DRegistrySettings( LPCSTR lpSubKey )
{
	HKEY hKey;
	int iRetVal = 0;
	DWORD dwDisposition;
	if( ERROR_SUCCESS == ::RegCreateKeyEx(HKEY_CURRENT_USER,
											lpSubKey,
											0, 
											"",
											REG_OPTION_NON_VOLATILE,
											KEY_READ | KEY_WRITE,
											NULL,
											&hKey,
											&dwDisposition ) )
	{
		char szBuffer[256];
		DWORD	dwDeviceIndex = (DWORD) g_VideoSettings.iCurrentDevice,
				dwWindowed = g_VideoSettings.bWindowed ? 1 : 0,
				dwVSync = g_VideoSettings.bWaitForVSync ? 1 : 0,
				dwAutoGenMipmaps = g_VideoSettings.bAutoGenMipmaps ? 1 : 0,
				dwUseTexturePack = g_VideoSettings.bUseTexturePackFile ? 1 : 0;
	
		g_VideoSettings.pDevData->GetDeviceNameByIndex( (int) dwDeviceIndex, szBuffer, 256 );
		::RegSetValueEx(	hKey, "DeviceIndex", 0, REG_DWORD,
							(const BYTE*) &dwDeviceIndex, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "DeviceName", 0, REG_SZ,
							(const BYTE*) szBuffer, strlen( szBuffer ) + 1 );
		::RegSetValueEx(	hKey, "ModeIndex", 0, REG_DWORD,
							(const BYTE*) &g_VideoSettings.iCurrentMode, sizeof( DWORD ) );
		g_VideoSettings.pDevData->GetResolutionStringByIndex( (int) dwDeviceIndex, 
																g_VideoSettings.iCurrentMode,
																szBuffer, 256 );
		::RegSetValueEx(	hKey, "ModeName", 0, REG_SZ,
							(const BYTE*) szBuffer, strlen( szBuffer ) + 1 );
		::RegSetValueEx(	hKey, "Windowed", 0, REG_DWORD,
							(const BYTE*) &dwWindowed, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "WaitForVSync", 0, REG_DWORD,
							(const BYTE*) &dwVSync, sizeof( DWORD ) );

		g_VideoSettings.pDevData->GetAASettingString(	(int) dwDeviceIndex, 
														g_VideoSettings.iCurrentMode,
														g_VideoSettings.bWindowed,
														g_VideoSettings.iCurrentAASetting,
														szBuffer, 
														256 );
		::RegSetValueEx(	hKey, "AAName", 0, REG_SZ,
							(const BYTE*) szBuffer, strlen( szBuffer ) + 1 );
		::RegSetValueEx(	hKey, "AAIndex", 0, REG_DWORD,
							(const BYTE*) &g_VideoSettings.iCurrentAASetting, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "MagFilter", 0, REG_DWORD,
							(const BYTE*) &g_VideoSettings.magFilter, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "MinFilter", 0, REG_DWORD,
							(const BYTE*) &g_VideoSettings.minFilter, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "MipFilter", 0, REG_DWORD,
							(const BYTE*) &g_VideoSettings.mipFilter, sizeof( DWORD ) );

		::RegSetValueEx(	hKey, "MaxTextureSize", 0, REG_DWORD, 
							(const BYTE*)  &g_VideoSettings.iMaxTextureSize, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "AutoGenMipmaps", 0, REG_DWORD, 
							(const BYTE*) &dwAutoGenMipmaps, sizeof( DWORD ) );
		::RegSetValueEx(	hKey, "UseTexturePackFile", 0, REG_DWORD, 
							(const BYTE*) &dwUseTexturePack, sizeof( DWORD ) );

		RegCloseKey(hKey);
	}
	return iRetVal;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Messy bunch of globals...
HANDLE hPackThread = INVALID_HANDLE_VALUE;
HWND hProgressCtrl = NULL, hDialog = NULL;
CDX9PackFile * gPackFile = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PackCreateCallback()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void PackCreateCallback( int iCurrentFileIndex, int iMaxFileIndex )
{
	if( ( iCurrentFileIndex == -1 ) &&
		( iMaxFileIndex == -1 ) )
	{
		SendMessage( hDialog, WM_COMMAND, MAKEWPARAM( IDOK, 0 ), 0 );
	}
	else
	{
		SendMessage( hProgressCtrl, PBM_SETRANGE, 0, (LPARAM) MAKELPARAM (0, iMaxFileIndex) );
		SendMessage( hProgressCtrl, PBM_SETPOS, iCurrentFileIndex, 0 );
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// PackCreateThreadProc()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI PackCreateThreadProc( LPVOID param )
{
	CDX9PackFile * pPackFile = (CDX9PackFile*) param;
	pPackFile->Create( PackCreateCallback );
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// SetupTexturePackFile()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool SetupTexturePackFile( HINSTANCE hInstance, const char * szDataPath, char * szPackFileName )
{
	bool bRetVal = true;
	int iRetVal;
	CDX9PackFile textures( szDataPath, szPackFileName );
	gPackFile = &textures;

	DWORD dwThreadID;
	if( textures.Exists() == false )
	{
		hPackThread = CreateThread(	NULL,
									0,
									PackCreateThreadProc,
									&textures,
									CREATE_SUSPENDED,
									&dwThreadID );
		_ASSERT( hPackThread != INVALID_HANDLE_VALUE );

		// Create Dialog box, then populate with video settings.
		iRetVal = DialogBox(	hInstance, 
								MAKEINTRESOURCE( IDD_PROGRESSBAR ), 
								GetDesktopWindow(), 
								ProgressBarDialogProc );
		if( iRetVal == IDCANCEL ) 
		{
			// User selected quit.
			return false;
		}
	}
	return bRetVal;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// DialogProc()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ProgressBarDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_INITDIALOG:
		{
			hDialog = hwndDlg;
			hProgressCtrl = GetDlgItem( hwndDlg, IDC_PROGRESS1 );
			ResumeThread( hPackThread );
		}
		break;

    case WM_COMMAND: 
        switch (LOWORD(wParam)) 
        { 
        case IDCANCEL: 
			gPackFile->SetUserCancelled( true );
			while( gPackFile->GetPackFileFinished() == false )
			{
				Sleep( 30 );		// Give the pack file a chance to finish up.
			}
            EndDialog(hwndDlg, wParam); 
            return TRUE;

		case IDOK:
			while( gPackFile->GetPackFileFinished() == false )
			{
				Sleep( 30 );		// Give the pack file a chance to finish up.
			}
            EndDialog(hwndDlg, wParam); 
            return TRUE;
        }
	}
	return FALSE;
}


//Imago - 7/28/09 this function will return the adapter's Windows desktop refresh rate setting
				// used to set the maximum refresh rate and avoid any PnP issues (thanks Sgt_Baker)
int GetMaxRate(int index = 0)
{
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(DISPLAY_DEVICE);

	if (!EnumDisplayDevices(NULL, index, &dd, 0))
    {
		debugf("1: EnumDisplayDevices failed:%d\n", GetLastError());
        return 60;
    }
 
    DISPLAY_DEVICE monitor;
    monitor.cb = sizeof(DISPLAY_DEVICE);
    if (!EnumDisplayDevices(dd.DeviceName, 0, &monitor, 0))
    {
		debugf("2: EnumDisplayDevices failed:%d\n", GetLastError());
		if (!EnumDisplayDevices(NULL, 0, &monitor, 0))	{
			debugf("22: EnumDisplayDevices failed:%d\n", GetLastError());
			return 60;
		}
	}
 
    DEVMODE dm;
    dm.dmSize = sizeof(DEVMODE);
 
    if (!EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
    {
		debugf("3: EnumDisplaySettings failed:%d\n", GetLastError());
        return 60;
    }
 
	debugf("Device string: %s\n", dd.DeviceString);
    debugf("Monitor ID: %s\n", monitor.DeviceID);
	debugf("Monitor name: %s\n", monitor.DeviceName);
	debugf("Monitor string: %s\n", monitor.DeviceString);
    debugf("Refresh rate, in hertz: %d\n", dm.dmDisplayFrequency);
    debugf("Color depth: %d\n", dm.dmBitsPerPel);
    debugf("Screen resolution, in pixels: %d x %d\n", 
        dm.dmPelsWidth, dm.dmPelsHeight);

	return dm.dmDisplayFrequency;
}


// BUILD_DX9


