#include "pch.h"
#include "ddvideo.h" //Macros and crap
#include "ddstream.h" //DShow object

DDVideo::DDVideo()
{
	m_hWnd         = NULL;
    m_lpDD         = NULL;
    m_lpDDSPrimary = NULL;
    m_lpDDSBack    = NULL;
	m_Running      = TRUE; 
}

// This is it...
HRESULT DDVideo::Play(ZString& strPath, bool bWindowed)
{
	m_bWindowed = bWindowed;
    HRESULT hr;  
	if( FAILED( hr = InitDirectDraw() ) ) {
		ZDebugOutput("InitDirectDraw() failed\n");
		return hr;
	}

	m_pVideo=new CDShow();

	if (m_pVideo==NULL) 
		return FALSE;	                 
    	
	if(!( m_pVideo->Open(strPath,m_lpDD) ))
		{
	
		// remember to destroy DShow (pVideo) object to clean up after us.
		// Return FALSE to let caller know we failed.
			ZDebugOutput("m_pVideo->Open failed\n");
		return FALSE;
		}			
	else
		m_pVideo->Start();     

	return S_OK;
}

HRESULT DDVideo::InitDirectDraw()
{
	DDSURFACEDESC2 ddsd;
    DDSCAPS2 ddscaps;
	HRESULT		hRet;
 
    //Create the main DirectDraw object - updated to do it the hard way.... 7/10
    LPDIRECTDRAW pDD;

	HINSTANCE hInstDDraw;
    LPDIRECTDRAWCREATE pDDCreate = NULL;
    hInstDDraw = LoadLibrary("ddraw.dll");
	pDDCreate = ( LPDIRECTDRAWCREATE )GetProcAddress( hInstDDraw, "DirectDrawCreate" );
    pDDCreate( NULL, &pDD, NULL );
	//hRet = DirectDrawCreate(NULL,&pDD, NULL);

    hRet = pDD->QueryInterface(IID_IDirectDraw7, (LPVOID *) & m_lpDD);
      
	//Set cooperative level
	if (m_bWindowed) { //#112
		m_lpDD->SetCooperativeLevel(m_hWnd, DDSCL_ALLOWREBOOT | DDSCL_NORMAL | DDSCL_MULTITHREADED);
	} else {
		hRet = m_lpDD->SetCooperativeLevel(m_hWnd,DDSCL_EXCLUSIVE |DDSCL_ALLOWREBOOT| DDSCL_ALLOWMODEX | DDSCL_FULLSCREEN | DDSCL_MULTITHREADED);
		m_lpDD->SetDisplayMode(800,600,16,g_DX9Settings.m_refreshrate,0);
	}
	
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	
	//#112
	ddsd.dwFlags = (m_bWindowed) ? DDSD_CAPS : DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = (m_bWindowed) ? DDSCAPS_PRIMARYSURFACE : DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

	if (m_bWindowed) {
		LPDIRECTDRAWCLIPPER lpClipper;
		hRet = m_lpDD->CreateSurface(&ddsd, &m_lpDDSPrimary, NULL);
		hRet = m_lpDD->CreateClipper( 0, &lpClipper, NULL );
		hRet = lpClipper->SetHWnd( 0, m_hWnd );
		hRet = m_lpDDSPrimary->SetClipper( lpClipper );
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = 800;
		ddsd.dwHeight = 600;
		hRet = m_lpDD->CreateSurface(&ddsd, &m_lpDDSBack, NULL);
	} else {
		ddsd.dwBackBufferCount = 1;
		hRet = m_lpDD->CreateSurface(&ddsd, &m_lpDDSPrimary, NULL);

		// BT - 9/17 - Fixing occosional crash on movie playback.
		if (FAILED(hRet) == true)
			return hRet;

		ZeroMemory( &ddscaps, sizeof( ddscaps ) );
		ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
		hRet = m_lpDDSPrimary->GetAttachedSurface(&ddscaps,&m_lpDDSBack);
	}

	pDD->Release();
	return hRet;
}

void DDVideo::DestroyDirectDraw()
{	
	m_lpDD->SetCooperativeLevel(m_hWnd,DDSCL_NORMAL);

	SAFE_RELEASE(m_lpDDSBack);
	SAFE_RELEASE(m_lpDDSPrimary);
	SAFE_RELEASE(m_lpDD);
}	  

void DDVideo::DestroyDDVid()
{	
	delete m_pVideo; 
	DestroyDirectDraw();
}

//#112
BOOL DDVideo::Flip()
{
    HRESULT ddrval;
    RECT rcRectSrc;
    RECT rcRectDest;
    POINT p;
    p.x = 0; p.y = 0;
    ClientToScreen(m_hWnd, &p);
    GetClientRect(m_hWnd, &rcRectDest);
    OffsetRect(&rcRectDest, p.x, p.y);
    SetRect(&rcRectSrc, 0, 0, 800, 600);
    ddrval = m_lpDDSPrimary->Blt( &rcRectDest, m_lpDDSBack, &rcRectSrc, DDBLT_WAIT, NULL);

    return (ddrval == DD_OK);
}