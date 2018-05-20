#include "DeviceModesDX9.h"

#include <zassert.h>

#include "LogFile.h"

#define OUTPUT_DEVICE_DETAILS

BOOL CALLBACK enumMonitorProc(HMONITOR hMonitor, HDC, LPRECT rect, LPARAM lParam) {
	HMONITOR *hm = (HMONITOR*) lParam;
	*hm = hMonitor;
	return true;
}

HMONITOR getPrimaryMonitor() {
	HMONITOR hm;
	EnumDisplayMonitors(NULL, NULL, enumMonitorProc, (LONG) &hm);
	return hm;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Buffer formats, in order of preference.
static D3DFORMAT pSupportedModes[] = 
{
	D3DFMT_X8R8G8B8,
	D3DFMT_R5G6B5,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Depth formats, sorted in order of preference.
static D3DFORMAT pSupportedDepthStencil[] = 
{
	D3DFMT_D24X8,
	D3DFMT_D16
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3DMULTISAMPLE_TYPE pMultiSampleTypes[] =
{
	D3DMULTISAMPLE_NONE,
	D3DMULTISAMPLE_2_SAMPLES,
	D3DMULTISAMPLE_4_SAMPLES,
	D3DMULTISAMPLE_6_SAMPLES,
	D3DMULTISAMPLE_8_SAMPLES,
	D3DMULTISAMPLE_16_SAMPLES
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static DWORD pTextureMinFilterTypes[] = 
{
	D3DPTFILTERCAPS_MINFPOINT,
	D3DPTFILTERCAPS_MINFLINEAR,
	D3DPTFILTERCAPS_MINFANISOTROPIC,
	D3DPTFILTERCAPS_MINFPYRAMIDALQUAD,
	D3DPTFILTERCAPS_MINFGAUSSIANQUAD
 };

////////////////////////////////////////////////////////////////////////////////////////////////////
static DWORD pTextureMagFilterTypes[] = 
{
	D3DPTFILTERCAPS_MAGFPOINT,
	D3DPTFILTERCAPS_MAGFLINEAR,
	D3DPTFILTERCAPS_MAGFANISOTROPIC,
	D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD,
	D3DPTFILTERCAPS_MAGFGAUSSIANQUAD
 };

////////////////////////////////////////////////////////////////////////////////////////////////////
static DWORD pTextureMipFilterTypes[] = 
{
	D3DPTFILTERCAPS_MIPFPOINT,
	D3DPTFILTERCAPS_MIPFLINEAR,
};

static D3DTEXTUREFILTERTYPE pTextureFilters[] =
{
	D3DTEXF_POINT,
	D3DTEXF_LINEAR,
	D3DTEXF_ANISOTROPIC,
	D3DTEXF_PYRAMIDALQUAD,
	D3DTEXF_GAUSSIANQUAD
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static const char * pszAAStrings[] =
{
	"No antialiasing",
	"2x full-scene antialiasing",
	"4x full-scene antialiasing",
	"6x full-scene antialiasing",
	"8x full-scene antialiasing"
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static const char * pszShortAAStrings[] =
{
	"No AA",
	"2xFSAA",
	"4xFSAA",
	"6xFSAA",
	"8xFSAA"
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static const char * pszMaxTextureSizeStrings[ CD3DDevice9::eMTS_NumTextureSizes ] =
{
	"256x256",
	"512x512",
	"1024x1024",
	"2048x2048"
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor()
// Generates all the data in one swoop, if possible.
////////////////////////////////////////////////////////////////////////////////////////////////////
CD3DDeviceModeData::CD3DDeviceModeData( int iMinWidth, int iMinHeight, class CLogFile * pLogFile )
{
	int i;

	m_iAdapterCount = 0;
	m_pAdapterArray = NULL;
	m_pD3D9			= NULL;
	m_bDataValid	= false;
	m_iMinWidth		= iMinWidth;
	m_iMinHeight	= iMinHeight;
	m_pLogFile		= pLogFile;

	// Create D3D9 interface.
	m_pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
	if( m_pD3D9 == NULL )
	{
		return;
	}

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
            HRESULT hr = m_pD3D9->RegisterSoftwareDevice((void*)D3D9GetSWInfo);
#else
			HRESULT hr = m_pD3D9->RegisterSoftwareDevice(D3D9GetSWInfo);
#endif
			if (hr == D3D_OK) {
				m_pLogFile->OutputString( "DX registered the SW Rasterizer.\n" );
			} else {
				pLogFile->OutputString( "DX did not register the SW Rasterizer!\n" );
			}
	} else {
		m_pLogFile->OutputString( "SW Rasterizer failed to load.\n" );
	}
	m_pLogFile->OutputString( "Created D3D9 interface.\n" );

	m_iAdapterCount = m_pD3D9->GetAdapterCount( );
	m_pAdapterArray = new SAdapter[ m_iAdapterCount ];
	memset( m_pAdapterArray, 0, m_iAdapterCount * sizeof( SAdapter ) );

	m_pLogFile->OutputStringV( "Found %d adapters.\n", m_iAdapterCount );

	// Grab data for each adapter.
	for( i=0; i<m_iAdapterCount; i++ )
	{
		ExtractAdapterData( i );
	}
	m_bDataValid = true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// ~CD3DDeviceModeData()
// Tidy up all data.
////////////////////////////////////////////////////////////////////////////////////////////////////
CD3DDeviceModeData::~CD3DDeviceModeData()
{
	int i, j;
	for( i=0; i<m_iAdapterCount; i++ )
	{
		for( j=0; j<eDMD_NumModes; j++ )
		{
			delete [] m_pAdapterArray[i].ppAvailableModes[j];
		}
		delete [] m_pAdapterArray[i].ppAvailableModes;
	}
	if( m_pAdapterArray != NULL )
	{
		delete [] m_pAdapterArray;
	}
	m_pD3D9->Release();
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// IsValid()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CD3DDeviceModeData::IsValid()
{
	return m_bDataValid;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ValidateSettings()
// Given a bunch of settings, validate that the device can use the selection. Includes
// using windowed settings.
// Returns ZERO if settings ARE VALID.
////////////////////////////////////////////////////////////////////////////////////////////////////
int CD3DDeviceModeData::ValidateSettings( int iDeviceIndex, int iResolutionIndex, bool bWindowed )
{
/*	HRESULT hr;
	D3DFORMAT reqFormat;

	if( bWindowed == true )
	{
		if( iResolutionIndex >= m_pAdapterArray[ iDeviceIndex ].iNum32BitModes )
		{
			iResolutionIndex	-= m_pAdapterArray[ iDeviceIndex ].iNum32BitModes;
			reqFormat			= m_pAdapterArray[ iDeviceIndex ].p16BitModes[ iResolutionIndex ].Format;
		}
		else
		{
			reqFormat			= m_pAdapterArray[ iDeviceIndex ].p32BitModes[ iResolutionIndex ].Format;
		}

		// Check initial device settings.
		hr = m_pD3D9->CheckDeviceType(	iDeviceIndex,
										D3DDEVTYPE_HAL,
										m_pAdapterArray[ iDeviceIndex ].currentDisplayMode.Format,
										reqFormat,
										TRUE );
	}
	else
	{
		if( iResolutionIndex >= m_pAdapterArray[ iDeviceIndex ].iNum32BitModes )
		{
			iResolutionIndex	-= m_pAdapterArray[ iDeviceIndex ].iNum32BitModes;
			reqFormat			= m_pAdapterArray[ iDeviceIndex ].p16BitModes[ iResolutionIndex ].Format;
		}
		else
		{
			reqFormat			= m_pAdapterArray[ iDeviceIndex ].p32BitModes[ iResolutionIndex ].Format;
		}

		hr = m_pD3D9->CheckDeviceType( iDeviceIndex,
										D3DDEVTYPE_HAL,
										reqFormat,
										reqFormat,
										FALSE );
	}

	if( hr != S_OK )
	{
		return -1;
	}*/

	return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetDeviceCount()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int	CD3DDeviceModeData::GetDeviceCount()
{
	return m_iAdapterCount;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetTotalResolutionCount()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CD3DDeviceModeData::GetTotalResolutionCount( int iDeviceIndex )
{
    ZAssert( iDeviceIndex < m_iAdapterCount );
	return m_pAdapterArray[iDeviceIndex].iTotalModeCount;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetResolutionCount()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int	CD3DDeviceModeData::GetResolutionCount( int iDeviceIndex, D3DFORMAT fmt )
{
	int i;

    ZAssert( iDeviceIndex < m_iAdapterCount );

	for( i=0; i<eDMD_NumModes; i++ )
	{
		if( pSupportedModes[i] == fmt )
		{
			break;
		}
	}

	if( i < eDMD_NumModes )
	{
		return m_pAdapterArray[ iDeviceIndex ].pModeCount[i];
	}
	return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetDeviceNameByIndex()
// Returns a string in pBuffer containing an identifier for the specified device.
// Needs to "Description [DeviceName]", as description can be identical for double header cards.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDeviceModeData::GetDeviceNameByIndex( int iDeviceIndex, char * pBuffer, int iBufferLen )
{
    ZAssert( iDeviceIndex < m_iAdapterCount );
	
	D3DADAPTER_IDENTIFIER9 * pAdapterID = &m_pAdapterArray[ iDeviceIndex ].adapterID;

    ZAssert( iBufferLen > strlen( pAdapterID->DeviceName) + strlen( pAdapterID->Description ) + 4 );
	
	strcpy_s( pBuffer, iBufferLen, pAdapterID->Description );
	strcat_s( pBuffer, iBufferLen, " [" );
	strcat_s( pBuffer, iBufferLen, pAdapterID->DeviceName );
	strcat_s( pBuffer, iBufferLen, "]" );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetResolutionStringByIndex()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDeviceModeData::GetResolutionStringByIndex( int iDeviceIndex, int iResolutionIndex, char * pBuffer, int iBufferLen )
{
	int i;
    ZAssert( iDeviceIndex < m_iAdapterCount );
	D3DDISPLAYMODE * pDisplayMode;
	
	i = 0;
	while(	( i < eDMD_NumModes ) && 
			( iResolutionIndex >= m_pAdapterArray[ iDeviceIndex ].pModeCount[i] ) )
	{
		iResolutionIndex -= m_pAdapterArray[ iDeviceIndex ].pModeCount[i];
		i++;
	}
	if ( !(i < eDMD_NumModes) || !(iResolutionIndex < m_pAdapterArray[ iDeviceIndex ].pModeCount[i])) {
		sprintf_s( pBuffer, iBufferLen, "800x600 Compat. Mode" );
		return; //Imago 8/10
	}
	pDisplayMode = &m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[ i ][ iResolutionIndex ].mode;
	sprintf_s( pBuffer, iBufferLen, "%d x %d @ %dHz [%d bit]", 
				pDisplayMode->Width, 
				pDisplayMode->Height, 
				pDisplayMode->RefreshRate,
				GetFormatBitCount( pDisplayMode->Format ) );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetNumAASettings()
// Includes a "NO AA" setting.
////////////////////////////////////////////////////////////////////////////////////////////////////
int CD3DDeviceModeData::GetNumAASettings( int iDeviceIndex, int iResolutionIndex, bool bWindowed )
{
	int i, iAACount, iCount;
	bool * pAAFlags;

    ZAssert( iDeviceIndex < m_iAdapterCount );
		
	i = 0;
	while(	( i < eDMD_NumModes ) && 
			( iResolutionIndex >= m_pAdapterArray[ iDeviceIndex ].pModeCount[i] ) )
	{
		iResolutionIndex -= m_pAdapterArray[ iDeviceIndex ].pModeCount[i];
		i++;
	}
    //ZAssert( i < eDMD_NumModes );
    //ZAssert( iResolutionIndex < m_pAdapterArray[ iDeviceIndex ].pModeCount[i] );
	//for now Imago 7/10
	if (iResolutionIndex == 0) {
		return 0;
	}

	if( bWindowed == true )
	{
		pAAFlags = m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][iResolutionIndex].pWindowedAA;
	}
	else
	{
		pAAFlags = m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][iResolutionIndex].pFullScreenAA;
	}

	iAACount = 0;
	for( iCount=0; iCount<eDMD_NumAAFormats; iCount++ )
	{
		if( pAAFlags[iCount] == true )
		{
			iAACount ++;
		}
	}
    //ZAssert( iAACount > 0 );	//Should at least have 'none' as an option.
	//ok Imago 7/10
	return iAACount;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetAASettingString()
// Includes a "NO AA" setting.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDeviceModeData::GetAASettingString(	int iDeviceIndex, 
												int iResolutionIndex, 
												bool bWindowed, 
												int iAAIndex, 
												char * pBuffer, 
												int iBufferLen )
{
	int i, iIndex;
	bool * pAAFlags;
    ZAssert( iDeviceIndex < m_iAdapterCount );
	
	iIndex = 0;
	i = 0;
	while(	( i < eDMD_NumModes ) && 
			( iResolutionIndex >= m_pAdapterArray[ iDeviceIndex ].pModeCount[i] ) )
	{
		iResolutionIndex -= m_pAdapterArray[ iDeviceIndex ].pModeCount[i];
		i++;
	}
    //ZAssert( i < eDMD_NumModes );
    //ZAssert( iResolutionIndex < m_pAdapterArray[ iDeviceIndex ].pModeCount[i] );
	if (iResolutionIndex == 0)
		return; //for now Imago 7/10

	if( bWindowed == true )
	{
		pAAFlags = m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][iResolutionIndex].pWindowedAA;
	}
	else
	{
		pAAFlags = m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][iResolutionIndex].pFullScreenAA;
	}

	while(	( iAAIndex > 0 ) &&
			( iIndex < eDMD_NumAAFormats ) )
	{
		iIndex ++;
		if( pAAFlags[ iIndex ] == true )
		{
			iAAIndex --;
		}
	}
	
	sprintf_s( pBuffer, iBufferLen, "%s", pszAAStrings[ iIndex ] );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetResolutionDetails()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDeviceModeData::GetResolutionDetails(	int			iDeviceIndex, 
												int			iModeIndex, 
												int *		pWidth, 
												int *		pHeight, 
												int *		pMonitorFreq, 
												D3DFORMAT * pBackBufferFormat,
												D3DFORMAT * pDeviceFormat,
												HMONITOR *	pHMonitor )
{
	int i, iModeCounter;

	i = 0;
	iModeCounter = iModeIndex;

	while(	( i < eDMD_NumModes ) &&
			( iModeCounter >= m_pAdapterArray[ iDeviceIndex ].pModeCount[i] ) )
	{
		iModeCounter -= m_pAdapterArray[ iDeviceIndex ].pModeCount[i];
		i ++;
	}

    ZAssert( i < eDMD_NumModes );

	if( i < eDMD_NumModes )
	{
		*pWidth				= m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][ iModeCounter ].mode.Width;
		*pHeight			= m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][ iModeCounter ].mode.Height;
		*pMonitorFreq		= m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][ iModeCounter ].mode.RefreshRate;
		*pBackBufferFormat	= m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[i][ iModeCounter ].mode.Format;
		*pDeviceFormat		= m_pAdapterArray[ iDeviceIndex ].currentDisplayMode.Format;
		*pHMonitor			= m_pAdapterArray[ iDeviceIndex ].hMonitor;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetRelatedResolutions()
// For us to build an array of related resolutions for changing up/down in full screen mode.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3DDeviceModeData::GetRelatedResolutions(	int iDeviceIndex,
												int iModeIndex,
												int * pNumResolutions,
												int * pSelectedResolution,
												SVideoResolution ** ppResolutionArray )
{
	int iMode, i, j, iModeCounter, iResCount;
	D3DFORMAT bbFormat;
	SVideoResolution srcRes;
	SVideoResolution resArray[256];			// Seriously, this must be enough.

	*pNumResolutions = -1;
	*pSelectedResolution = -1;
	iMode = 0;
	iModeCounter = iModeIndex;
	memset( resArray, 0, 256 * sizeof( SVideoResolution ) );
	while(	( iMode < eDMD_NumModes ) &&
			( iModeCounter >= m_pAdapterArray[ iDeviceIndex ].pModeCount[iMode] ) )
	{
		iModeCounter -= m_pAdapterArray[ iDeviceIndex ].pModeCount[iMode];
		iMode ++;
	}
    ZAssert( iMode < eDMD_NumModes );

	// Starting point.
	CD3DDeviceModeData::SAdapterMode * pModeArray = m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[iMode];
	srcRes.iWidth	= pModeArray[ iModeCounter ].mode.Width;
	srcRes.iHeight	= pModeArray[ iModeCounter ].mode.Height;
	srcRes.iFreq	= pModeArray[ iModeCounter ].mode.RefreshRate;
	bbFormat		= pModeArray[ iModeCounter ].mode.Format;
	iResCount		= 0;

	// Add all unique resolutions.
	for( i=0; i<m_pAdapterArray[ iDeviceIndex ].pModeCount[ iDeviceIndex ]; i++ )
	{
		for( j=0; j<iResCount; j++ )
		{
			if( bbFormat == pModeArray[i].mode.Format ) 
			{
				if( ( resArray[ j ].iWidth == pModeArray[i].mode.Width ) &&
					( resArray[ j ].iHeight == pModeArray[i].mode.Height ) )
				{
					if( ( pModeArray[i].mode.RefreshRate <= (DWORD)srcRes.iFreq ) &&
						( pModeArray[i].mode.RefreshRate > (DWORD)resArray[j].iFreq ) )
					{
						resArray[j].iFreq = pModeArray[i].mode.RefreshRate;
					}
					
					// Found entry, don't continue the search
					break;
				}
			}
		}
		if( j == iResCount )
		{
			// No entry found, add a new one.
			resArray[ iResCount].iWidth = pModeArray[i].mode.Width;
			resArray[ iResCount].iHeight = pModeArray[i].mode.Height;
			resArray[ iResCount].iFreq = pModeArray[i].mode.RefreshRate;
			iResCount ++;
		}
	}

	// Create storage and store the ordered results in ppResolutionArray.
	*ppResolutionArray = new SVideoResolution[ iResCount ];
	memset( *ppResolutionArray, 0, sizeof( SVideoResolution ) * iResCount );

	// Find start resolution.
	int iCurrMode = 0, iNumAdded = 0, iLastMode;
	for( iMode=1; iMode<iResCount; iMode++ )
	{
		if( ( resArray[ iMode ].iWidth < resArray[ iCurrMode ].iWidth ) ||
			( resArray[ iMode ].iHeight < resArray[ iCurrMode ].iHeight ) )
		{
			iCurrMode = iMode;
		}
	}
	(*ppResolutionArray)[ iNumAdded ].iWidth = resArray[ iCurrMode ].iWidth;
	(*ppResolutionArray)[ iNumAdded ].iHeight = resArray[ iCurrMode ].iHeight;
	(*ppResolutionArray)[ iNumAdded ].iFreq = resArray[ iCurrMode ].iFreq;
	iNumAdded ++;

	while( iNumAdded < iResCount )
	{
		iLastMode = iCurrMode;

		// Find the next resolution up.
		for( iMode=0; iMode<iResCount; iMode++ )
		{
			if( iMode == iLastMode )
			{
				continue;
			}
			if(		( resArray[ iMode ].iWidth > resArray[ iLastMode ].iWidth ) ||
				(	( resArray[ iMode ].iWidth == resArray[ iLastMode ].iWidth ) &&
					( resArray[ iMode ].iHeight > resArray[ iLastMode ].iHeight ) ) )
			{
				if( iCurrMode == iLastMode )
				{
					iCurrMode = iMode;
				}				
				else if( ( resArray[ iMode ].iWidth <= resArray[ iCurrMode ].iWidth ) &&
						( resArray[ iMode ].iHeight < resArray[ iCurrMode ].iHeight ) )
				{
					iCurrMode = iMode;
				}
			}
		}
		(*ppResolutionArray)[ iNumAdded ].iWidth = resArray[ iCurrMode ].iWidth;
		(*ppResolutionArray)[ iNumAdded ].iHeight = resArray[ iCurrMode ].iHeight;
		(*ppResolutionArray)[ iNumAdded ].iFreq = resArray[ iCurrMode ].iFreq;
		iNumAdded ++;
	}

	// Store number of resolutions.
	*pNumResolutions = iNumAdded;
	for( i=0; i<iNumAdded; i++ )
	{
		if( ( (*ppResolutionArray)[ i ].iWidth == srcRes.iWidth ) &&
			( (*ppResolutionArray)[ i ].iHeight == srcRes.iHeight ) )
		{
			*pSelectedResolution = i;
			break;
		}
	}
	//try this hack Imago 7/10
	if ( *pSelectedResolution == -1) {
		ppResolutionArray[0]->iFreq = 60;
		ppResolutionArray[0]->iWidth = 800;
		ppResolutionArray[0]->iHeight = 600;
		*pSelectedResolution = 1;
	}
    ZAssert( *pSelectedResolution != -1 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// GetFormatBitCount()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CD3DDeviceModeData::GetFormatBitCount( D3DFORMAT fmt )
{
	int iRetVal = 0;
	switch( fmt )
	{
	case D3DFMT_R5G6B5:
		iRetVal = 16;
		break;

	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8R8G8B8:
		iRetVal = 32;
		break;
	}
	return iRetVal;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtractAdapterData()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CD3DDeviceModeData::ExtractAdapterData( int iAdapter )
{
	int i, j, k, iLastAdded;
	SAdapter * pAdapter = &m_pAdapterArray[iAdapter];
	SAdapterMode * pMode;
	D3DDISPLAYMODE dispMode;
	D3DFORMAT currMode;
	HRESULT hr;

	// For each display mode, geq min width/height...
	//	Check hardware supported mode available.
	//	Store flag for windowed mode version is allowed... (CheckDeviceFormatConversion)
	//	Check available stencil match, look for 24bit first, then 16bit.
	//	Check multisample support.

	m_pD3D9->GetAdapterIdentifier( iAdapter, 0, &pAdapter->adapterID );
	m_pD3D9->GetAdapterDisplayMode( iAdapter, &pAdapter->currentDisplayMode );

	m_pLogFile->OutputStringV("Extracting adapter %d: %s - %s\n", 
						iAdapter, 
						pAdapter->adapterID.DeviceName,
						pAdapter->adapterID.Description );

	pAdapter->hMonitor = m_pD3D9->GetAdapterMonitor( iAdapter );

	// BT - 10/17 Fixing the DX GetAdapterMonitor failed error. 
	MONITORINFOEX lpmi;
	lpmi.cbSize = sizeof(lpmi);
	if (!GetMonitorInfo(pAdapter->hMonitor,&lpmi)) {
		m_pLogFile->OutputString("DX GetAdapterMonitor failed!\n");
		pAdapter->hMonitor = getPrimaryMonitor();
	}
	pAdapter->pModeCount = new int[ eDMD_NumModes ]; //Fix memory leak -Imago 8/2/09
	pAdapter->ppAvailableModes = new SAdapterMode *[ eDMD_NumModes ];
	pAdapter->iTotalModeCount = 0;
	hr = m_pD3D9->GetDeviceCaps( iAdapter, D3DDEVTYPE_HAL, &pAdapter->devCaps );
	if ( hr != D3D_OK ) {
		m_pLogFile->OutputString("Trying D3DDEVTYPE_SW\n");
		hr = m_pD3D9->GetDeviceCaps( iAdapter, D3DDEVTYPE_SW, &pAdapter->devCaps );
		if (hr != D3D_OK) {
			m_pLogFile->OutputString("Trying D3DDEVTYPE_REF\n");
			hr = m_pD3D9->GetDeviceCaps( iAdapter, D3DDEVTYPE_REF, &pAdapter->devCaps );
		}
	}

	for( i=0; i<eDMD_NumModes; i++ )
	{
		iLastAdded = 0;
		currMode = pSupportedModes[i];
		pAdapter->pModeCount[i] = m_pD3D9->GetAdapterModeCount( iAdapter, currMode );
		pAdapter->ppAvailableModes[i] = new SAdapterMode[ pAdapter->pModeCount[i] ];

		bool bPassed = false;
		for( j=0; j<pAdapter->pModeCount[i]; j++ )
		{
			pMode = &pAdapter->ppAvailableModes[i][iLastAdded];
			m_pD3D9->EnumAdapterModes( iAdapter, currMode, j, &dispMode );

			// Does display mode meet minimum size requirements?
			if( ( dispMode.Width < (UINT) m_iMinWidth ) ||
				( dispMode.Height < (UINT) m_iMinHeight ) )
			{
				// No, go to next one.
				continue;
			}
			pMode->mode.Format		= dispMode.Format;
			pMode->mode.Width		= dispMode.Width;
			pMode->mode.Height		= dispMode.Height;
			pMode->mode.RefreshRate	= dispMode.RefreshRate;

			// Check hardware, then windowed.

			hr = m_pD3D9->CheckDeviceType(	iAdapter,
											D3DDEVTYPE_HAL,
											dispMode.Format,
											dispMode.Format, 
											FALSE ) ;

            pMode->bHWSupport = true; //initialize
			if( hr != D3D_OK )
			{
				pMode->bHWSupport = false;
				hr = m_pD3D9->CheckDeviceType(	iAdapter,
								D3DDEVTYPE_SW,
								dispMode.Format,
								dispMode.Format, 
								FALSE ) ;
				if ( hr != D3D_OK )  {
					m_pLogFile->OutputStringV("SW full-screen device type %d failed\n",dispMode.Format);
					hr = m_pD3D9->CheckDeviceType(	iAdapter,
								D3DDEVTYPE_REF,
								dispMode.Format,
								dispMode.Format, 
								FALSE ) ;
					if ( hr != D3D_OK )  {
						m_pLogFile->OutputStringV("REF full-screen device type %d failed\n",dispMode.Format);
						
						continue;
					}
				} else {
					bPassed = true;
				}
			} else {
				bPassed = true;
			}

            ZAssert(bPassed);

			pMode->bWindowAllowed = false;
			
			hr = m_pD3D9->CheckDeviceType(	iAdapter,
											D3DDEVTYPE_HAL,
											pAdapter->currentDisplayMode.Format,
											dispMode.Format,
											TRUE );
			if( hr == D3D_OK )
			{
				pMode->bWindowAllowed = true;
			} else {
				hr = m_pD3D9->CheckDeviceType(	iAdapter,
								D3DDEVTYPE_SW,
								pAdapter->currentDisplayMode.Format,
								dispMode.Format,
								TRUE );
				if( hr == D3D_OK ) {
					pMode->bWindowAllowed = true;
				} else {
					m_pLogFile->OutputStringV("SW windowed device type %d bb %d failed\n",dispMode.Format,pAdapter->currentDisplayMode.Format);
				}
			}

			pMode->d3dDepthStencil = D3DFMT_UNKNOWN;

			// Check depth/stencil formats.
			for( k=0; k<eDMD_NumDepthStencilFormats; k++ )
			{
				// First, check the depth format is available.
				hr = m_pD3D9->CheckDeviceFormat(	iAdapter,
													D3DDEVTYPE_HAL,
													pAdapter->currentDisplayMode.Format,
													D3DUSAGE_DEPTHSTENCIL,
													D3DRTYPE_SURFACE,
													pSupportedDepthStencil[k] );

				if( hr != D3D_OK )
				{
					hr = m_pD3D9->CheckDeviceFormat(	iAdapter,
													D3DDEVTYPE_SW,
													pAdapter->currentDisplayMode.Format,
													D3DUSAGE_DEPTHSTENCIL,
													D3DRTYPE_SURFACE,
													pSupportedDepthStencil[k] );
					if( hr != D3D_OK ) continue;
				}

				hr = m_pD3D9->CheckDepthStencilMatch(	iAdapter,
														D3DDEVTYPE_HAL,
														pAdapter->currentDisplayMode.Format,
														dispMode.Format,
														pSupportedDepthStencil[k] );
				if( hr == D3D_OK )
				{
					pMode->d3dDepthStencil = pSupportedDepthStencil[k];
					break;
				} else {
					hr = m_pD3D9->CheckDepthStencilMatch(	iAdapter,
														D3DDEVTYPE_SW,
														pAdapter->currentDisplayMode.Format,
														dispMode.Format,
														pSupportedDepthStencil[k] );
					if( hr == D3D_OK ) {
						pMode->d3dDepthStencil = pSupportedDepthStencil[k];
						break;
					} else {
						hr = m_pD3D9->CheckDepthStencilMatch(	iAdapter,
														D3DDEVTYPE_REF,
														pAdapter->currentDisplayMode.Format,
														dispMode.Format,
														pSupportedDepthStencil[k] );
					}
				}
			}
			if( pMode->d3dDepthStencil == D3DFMT_UNKNOWN )
			{
				// Must have a valid depth buffer.
				continue;
			}

			// Finally, multisample support, check both windowed and full screen.
			for( k=0; k<eDMD_NumAAFormats; k++ )
			{
				hr = m_pD3D9->CheckDeviceMultiSampleType(	iAdapter,
															D3DDEVTYPE_HAL,
															dispMode.Format,
															FALSE,
															pMultiSampleTypes[k],
															NULL );
				pMode->pFullScreenAA[k] = ( hr == D3D_OK ) ? true : false;
				hr = m_pD3D9->CheckDeviceMultiSampleType(	iAdapter,
															D3DDEVTYPE_HAL,
															dispMode.Format,
															TRUE,
															pMultiSampleTypes[k],
															NULL );
				pMode->pWindowedAA[k] = ( hr == D3D_OK ) ? true : false;
			}

			// Log settings.
			char szTemp[256];
			DWORD dwAASettings = 0, dwTemp = 1;
			for( k=0; k<eDMD_NumAAFormats; k++ )
			{
				if( pMode->pFullScreenAA[ k ] == true )
				{
					dwAASettings |= dwTemp;
				}
				dwTemp = dwTemp << 1;
			}
			int iResIndex = iLastAdded;
			for( k=0; k<i; k++ )
			{
				iResIndex += pAdapter->pModeCount[k];
			}
			GetResolutionStringByIndex( iAdapter, iResIndex, szTemp, 256 );
			m_pLogFile->OutputStringV( "%s\t\t[HW %d   WND %d   DS %d   AA 0x%02x]\n", 
				szTemp, pMode->bHWSupport, pMode->bWindowAllowed, 
				pMode->d3dDepthStencil, dwAASettings );

			iLastAdded ++;
		}
		// Store the actual number added.
		pAdapter->pModeCount[i] = iLastAdded;
		pAdapter->iTotalModeCount += pAdapter->pModeCount[i];
	}
	return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtractAdapterData()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CD3DDeviceModeData::GetModeParams(	CD3DDevice9::SD3DDeviceSetupParams * pParams,
										int iDeviceIndex,
										int iResolutionIndex,
										int iAAIndex,
										CLogFile * pLogFile )
{
    ZAssert( pParams != NULL );
    ZAssert( iDeviceIndex < m_iAdapterCount );
	//not so picky for now Imago 7/10
    //ZAssert( iResolutionIndex < m_pAdapterArray[iDeviceIndex].iTotalModeCount );
    ZAssert( iAAIndex < eDMD_NumAAFormats );

	SAdapterMode * pMode, * pWindowedMode;
	int i, iModeCounter, iCount, iTemp;

	i = 0;
	iModeCounter = iResolutionIndex;

	while(	( i < eDMD_NumModes ) &&
			( iModeCounter >= m_pAdapterArray[ iDeviceIndex ].pModeCount[i] ) )
	{
		iModeCounter -= m_pAdapterArray[ iDeviceIndex ].pModeCount[i];
		i ++;
	}
//dont sweat it for now Imago 7/10
//	ZAssert( i < eDMD_NumModes );
	if( ( i >= eDMD_NumModes ) ||
		( iModeCounter >= m_pAdapterArray[iDeviceIndex].pModeCount[i] ) )
	{
		// Ended up with dodgy index values.
		// Prepare the full screen mode first.
		pParams->bWindowModeValid				= true;
		pParams->sFullScreenMode.mode.Format		= D3DFMT_R5G6B5;
		pParams->sFullScreenMode.mode.Width			= 800;
		pParams->sFullScreenMode.mode.Height		= 600;
		pParams->sFullScreenMode.mode.RefreshRate	= 60;
		pParams->sFullScreenMode.fmtDepthStencil	= D3DFMT_UNKNOWN;
		return true;
	}
	
	// Prepare the full screen mode first.

    //try finding our preferred default resolution
    pMode = FindMatchingWindowedMode(pParams, WinPoint(pParams->sFullScreenMode.mode.Width, pParams->sFullScreenMode.mode.Height), iDeviceIndex);
    if (!pMode) {
        //not found, select the first one (800x600)
        pMode = &m_pAdapterArray[iDeviceIndex].ppAvailableModes[i][iModeCounter];
    }

	pParams->sFullScreenMode.mode.Format		= pMode->mode.Format;
	pParams->sFullScreenMode.mode.Width			= pMode->mode.Width;
	pParams->sFullScreenMode.mode.Height		= pMode->mode.Height;
	pParams->sFullScreenMode.mode.RefreshRate	= pMode->mode.RefreshRate;
	pParams->sFullScreenMode.fmtDepthStencil	= pMode->d3dDepthStencil;

	pLogFile->OutputString("Fullscreen settings:\n");
	pLogFile->OutputStringV( "FMT %d   W %d   H %d   RR %d   DEP %d\n",
		pParams->sFullScreenMode.mode.Format, pParams->sFullScreenMode.mode.Width,
		pParams->sFullScreenMode.mode.Height, pParams->sFullScreenMode.mode.RefreshRate,
		pParams->sFullScreenMode.fmtDepthStencil );

	// Now the windowed screen mode settings.
	if( pMode->bWindowAllowed == true )
	{
		// Windowed matched full screen ok.
		pWindowedMode = pMode;
	}
	else
	{
		// Find a suitable replacement mode.
		pWindowedMode = FindMatchingWindowedMode( pParams, pMode, iDeviceIndex );
	}

	// Copy over windowed mode details if they're valid.
	if( pWindowedMode != NULL )
	{
		pParams->sWindowedMode.mode.Format		= pWindowedMode->mode.Format;
		pParams->sWindowedMode.mode.Width		= pWindowedMode->mode.Width;
		pParams->sWindowedMode.mode.Height		= pWindowedMode->mode.Height;
		pParams->sWindowedMode.mode.RefreshRate	= pWindowedMode->mode.RefreshRate;
		pParams->sWindowedMode.fmtDepthStencil	= pWindowedMode->d3dDepthStencil;
		pParams->bWindowModeValid				= true;

		pLogFile->OutputString("Windowed settings:\n");
		pLogFile->OutputStringV( "FMT %d   W %d   H %d   RR %d   DEP %d\n",
			pParams->sWindowedMode.mode.Format, pParams->sWindowedMode.mode.Width,
			pParams->sWindowedMode.mode.Height, pParams->sWindowedMode.mode.RefreshRate,
			pParams->sWindowedMode.fmtDepthStencil );
	}
	else
	{
		pLogFile->OutputString("WARNING: failed to find a good windowed match. Disabling windowed mode.\n" );
		OutputDebugString("WARNING: failed to find a good windowed match. Disabling windowed mode.\n" );
		pParams->bWindowModeValid = false;
		pParams->bRunWindowed = false;
	}

	// Process the two AA settings.
	iCount = 0;
	iTemp = iAAIndex;
	while( ( iAAIndex > 0 ) && ( iCount < eDMD_NumAAFormats ) )
	{
		if( pMode->pFullScreenAA[iCount] == true )
		{
			iAAIndex --;
		}
		iCount ++;
	}
	pParams->sFullScreenMode.d3dMultiSampleSetting = pMultiSampleTypes[ iCount ];
	strcpy_s( pParams->szAAType, 64, pszShortAAStrings[ iCount ] );

	// Windowed AA setting - note, may not be as high a setting as the full screen aliasing.
	if( pWindowedMode != NULL )
	{
		iAAIndex = iTemp;
		iCount = 0;
		while( ( iAAIndex > 0 ) && ( iCount < eDMD_NumAAFormats ) )
		{
			if( pWindowedMode->pWindowedAA[iCount] == true )
			{
				iAAIndex --;
			}
			iCount ++;
		}
		if( iAAIndex > 0 )
		{
			// Windowed mode AA didn't match the full screen AA.
			iAAIndex = 0;
			for( iCount = 0; iCount<eDMD_NumAAFormats; iCount++ )
			{
				if( ( pWindowedMode->pWindowedAA[iCount] == true ) && ( iCount < iTemp ) )
				{
					iAAIndex = iCount;
				}
			}
			pParams->sWindowedMode.d3dMultiSampleSetting = pMultiSampleTypes[ iAAIndex ];
		}
		else
		{
			pParams->sWindowedMode.d3dMultiSampleSetting = pMultiSampleTypes[ iCount ];
		}
	}
	//Imago - Bug:  the AA selection box is populated only with your primary adapter's caps
	//a fix would be to populate the AA select list with each adapter, and change the options when you change adapter
    ZAssert( pParams->sWindowedMode.d3dMultiSampleSetting == pParams->sFullScreenMode.d3dMultiSampleSetting );
	pParams->bAntiAliased = pParams->sFullScreenMode.d3dMultiSampleSetting == D3DMULTISAMPLE_NONE ? false : true;

	pLogFile->OutputStringV( "FSMS %d   WINMS %d\n",
		pParams->sFullScreenMode.d3dMultiSampleSetting, pParams->sWindowedMode.d3dMultiSampleSetting );

	DWORD dwTexFilterCaps = m_pAdapterArray[iDeviceIndex].devCaps.TextureFilterCaps;

	// Finally, interrogate the device to find out what texture filtering it supports.
	// For now, we get the best available filter.
	// Texture minification filter.
	for( iCount=0; iCount<eDMD_NumMinFilters; iCount++ )
	{
		if( (dwTexFilterCaps & pTextureMinFilterTypes[iCount] ) != 0 )
		{
			pParams->d3dMinFilter = pTextureFilters[ iCount ];
		}
	}
	// Texture magnification filter.
	for( iCount=0; iCount<eDMD_NumMagFilters; iCount++ )
	{
		if( (dwTexFilterCaps & pTextureMagFilterTypes[iCount] ) != 0 )
		{
			pParams->d3dMagFilter = pTextureFilters[ iCount ];
		}
	}
	// Mipmap generation filter.
	for( iCount=0; iCount<eDMD_NumMipFilters; iCount++ )
	{
		if( (dwTexFilterCaps & pTextureMipFilterTypes[iCount] ) != 0 )
		{
			pParams->d3dMipFilter = pTextureFilters[ iCount ];
		}
	}

	pLogFile->OutputStringV( "MIN %d   MAG %d   MIP %d\n",
		pParams->d3dMinFilter, pParams->d3dMagFilter, pParams->d3dMipFilter );

	return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// FindMatchingWindowedMode()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
CD3DDeviceModeData::SAdapterMode * CD3DDeviceModeData::FindMatchingWindowedMode(	
						CD3DDevice9::SD3DDeviceSetupParams * pParams,
						WinPoint size,
						int iDeviceIndex )
{
    ZAssert( pParams != NULL );

	int iMode, iRes;
	SAdapterMode * pWindowedMode = NULL;
	
	for( iMode=0; iMode<eDMD_NumModes; iMode++ )
	{
		for( iRes=0; iRes<m_pAdapterArray[iDeviceIndex].pModeCount[iMode]; iRes++ )
		{
			pWindowedMode = &m_pAdapterArray[ iDeviceIndex ].ppAvailableModes[iMode][iRes];
			if( ( pWindowedMode->bWindowAllowed == true ) &&
				( pWindowedMode->bHWSupport == true ) &&
				( pWindowedMode->mode.Width == size.X()) &&
				( pWindowedMode->mode.Height == size.Y()) )
			{
				return pWindowedMode;
			}
		}
	}

	return NULL;
}

CD3DDeviceModeData::SAdapterMode * CD3DDeviceModeData::FindMatchingWindowedMode(
    CD3DDevice9::SD3DDeviceSetupParams * pParams,
    SAdapterMode * pFSMode,
    int iDeviceIndex)
{
    ZAssert(pParams != NULL);
    ZAssert(pFSMode != NULL);

    return FindMatchingWindowedMode(pParams, WinPoint(pFSMode->mode.Width, pFSMode->mode.Height), iDeviceIndex);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// GetMaxTextureString()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
const char *CD3DDeviceModeData::GetMaxTextureString( int iIndex )
{
    ZAssert( ( iIndex >= 0 ) && ( iIndex < CD3DDevice9::eMTS_NumTextureSizes ) );
	return pszMaxTextureSizeStrings[ iIndex ];
}
