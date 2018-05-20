#include "pch.h"

//Imago 6/29/09 - added THE DirectDraw class for showing videos

CDShow::CDShow()
{
	// Reset media open
	m_bMediaOpen=FALSE;

	// Set playing to FALSE
	m_bPlaying=FALSE;
	
	m_pDDSurface7=NULL;
	m_pDDSurface=NULL;
	m_pSample=NULL;
	m_pDDStream=NULL;
	m_pPrimaryVideoStream=NULL;
	m_pMMStream=NULL;
}


CDShow::~CDShow()
{
	// Call CleanUp function.
	this->CleanUp();	
}

// Open multimedia stream.
BOOL CDShow::Open(ZString& pFileName, IDirectDraw7 *pDD)
{
	// Multimedia stream pointer
	IAMMultiMediaStream *pAMStream;
	IGraphBuilder *pGb; // 7/10 #110
	IEnumFilters *pEfs;
	IBasicAudio *pBa;

    
	//7/29/09 we can now do stuff while the video plays
	CoInitializeEx(NULL,COINIT_MULTITHREADED); 
	

    // Create Multimedia stream object
	if (FAILED(CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
				 IID_IAMMultiMediaStream, (void **)&pAMStream)))
	{		
		// Return FALSE to let caller know we failed.	
		return FALSE; 
	}
    
	// Initialize Multimedia stream object
	if (FAILED(pAMStream->Initialize(STREAMTYPE_READ, 0, NULL)))
	{	
		// There are alot of possiblities to fail.....		
		return FALSE; 
	}

    // Add primary video stream.
	if (FAILED((pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL))))
	{		
		return FALSE; 
	}

    // Add default sound render to primary video stream,
	// so sound will be played back automatically.
	if (FAILED(pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL)))
	{
		// Return FALSE to let caller know we failed.		
		return FALSE; 
	}

    // Convert filename to UNICODE.
	// Notice the safe way to get the actual size of a string.
	WCHAR wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pFileName, -1, wPath, sizeof(wPath)/sizeof(wPath[0]));   
    
	// Build the filter graph for our multimedia stream.
	if (FAILED((pAMStream->OpenFile(wPath, 0))))
	{	
		// Return FALSE to let caller know we failed.		
		return FALSE; 
	}

    // Assign member to temperary stream pointer.
	m_pMMStream = pAMStream;
	
	// Add a reference to the file
	pAMStream->AddRef();
    	
	// Get media stream interface
	if (FAILED(m_pMMStream->GetMediaStream(MSPID_PrimaryVideo, &m_pPrimaryVideoStream)))
	{	
		return FALSE;
	}
	
	// Get DirectDraw media stream interface
	if (FAILED(m_pPrimaryVideoStream->QueryInterface(IID_IDirectDrawMediaStream,(void **)&m_pDDStream)))
	{				
		return FALSE;
	}	
	
	// Create stream sample
	if (FAILED(m_pDDStream->CreateSample(NULL,NULL,0,&m_pSample)))
	{			
		return FALSE;
	}	
	
	// Get DirectDraw surface interface from Sample.
	if (FAILED(m_pSample->GetSurface(&m_pDDSurface,&m_rSrcRect)))
	{			

		return FALSE;
	}	
	
	// Get DirectDraw surface7 interface
	if (FAILED(m_pDDSurface->QueryInterface(IID_IDirectDrawSurface7,(void**)&m_pDDSurface7)))
	{		
		return FALSE;
	}	
	
	// Ok. Media is open now.
	m_bMediaOpen=TRUE;
	
	// If we are here, everything turned out ok. Return TRUE.
	return TRUE;
}

// CleanUp function. 
// Called automatically upon object destruction.
void CDShow::CleanUp()
{
	// Reset media open
	m_bMediaOpen=FALSE;	

	// Set playing to FALSE
	m_bPlaying=FALSE;
	
	// Release allocated interfaces in reverse order of creation
	
	SAFE_RELEASE(m_pDDSurface7);
	SAFE_RELEASE(m_pDDSurface);
	SAFE_RELEASE(m_pSample);
	SAFE_RELEASE(m_pDDStream);
	SAFE_RELEASE(m_pPrimaryVideoStream);
	SAFE_RELEASE(m_pMMStream);
}

// Draw video to DirectDraw surface.
BOOL CDShow::Draw(LPDIRECTDRAWSURFACE7 lpDDSurface7)
{
	// Return FALSE if media was not open
	if (!m_bMediaOpen) {
		return FALSE;
	}


	// Update media stream. 
	// If it does not return S_OK, we are not playing.
	if (m_pSample->Update(0,NULL,NULL,0)!=S_OK) {
		m_bPlaying=FALSE;

	}
	
	// Now blit video to specified surface and rect.
	// Restore surface if lost.
	if (lpDDSurface7->Blt(NULL,m_pDDSurface7, &m_rSrcRect,DDBLT_WAIT,NULL)==DDERR_SURFACELOST)
		lpDDSurface7->Restore();
	
	// Ok. return TRUE.
	return TRUE;
}

// Start video playback
BOOL CDShow::Start()
{
	// Return FALSE if media was not open
	if (!m_bMediaOpen) {
		return FALSE;
	}

	// Set stream position to zero
	m_pMMStream->Seek(0);

	// Set state to playback
	m_pMMStream->SetState(STREAMSTATE_RUN);
	
	// Set playing to TRUE
	m_bPlaying=TRUE;
	
	// Everything went ok. Return TRUE.
	return TRUE;
}

// Stop video playback
BOOL CDShow::Stop()
{
	// Return FALSE if media was not open
	if (!m_bMediaOpen) return FALSE;
	
	// Set state to stop
	m_pMMStream->SetState(STREAMSTATE_STOP);

	// Set playing to FALSE
	m_bPlaying=FALSE;
	
	// Everything went ok. Return TRUE.
	return TRUE;
}

// Close multimedia stream
BOOL CDShow::Close()
{
	// Return FALSE if media was not open
	if (!m_bMediaOpen) return FALSE;

	// Call cleanup
	this->CleanUp();
	
	// Everything went ok. Return TRUE.
	return TRUE;
}
		