#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Fonts
//
// External entry points for rendering text:
// DrawString()
// -> DrawStringHandled() - actually render and handle clipping.
// -> DrawColorCodedString() - process colour codes.
//////////////////////////////////////////////////////////////////////////////

class FontImpl : public	IEngineFont	
{
private:
	//////////////////////////////////////////////////////////////////////////////
	//
	// Types
	//
	//////////////////////////////////////////////////////////////////////////////

	class CharData 
	{
	public:
		WinPoint m_size;
		int		 m_offset;
	};

	struct SCharTexData
	{
		float	fU1, fV1;		// Tex coords of top left corner.
		float	fU2, fV2;		// Tex coords of bottom right corner.
	};

	//////////////////////////////////////////////////////////////////////////////
	//
	// Data	members
	//
	//////////////////////////////////////////////////////////////////////////////

	int					m_height;
	int					m_width;
	CharData			m_data[256];
	SCharTexData		m_pCharTexData[256];
	BYTE*				m_pdata;
	int					m_dataSize;

	// Added
	TRef<PrivateSurface>	m_pFontTex;

	//////////////////////////////////////////////////////////////////////////////
	//
	// Private members
	//
	//////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////
	// GenerateFontTextureFromFontData()
	// Generate the font texture from the 1-bit font array.
	////////////////////////////////////////////////////////////////////////////////////////////////
	void GenerateFontTextureFromFontData( )
	{
		// Calculate the size of texture required.
		DWORD i;
		DWORD dwTexWidth = 256, dwTexHeight = 256;		// Initial size, construct a function to find smallest required size,
		int iNumCharsWide = dwTexWidth /  m_width;		// using the font data.
		int iNumCharsHigh = dwTexHeight / m_height;

		if( iNumCharsHigh * iNumCharsWide <= 256 )
		{
			dwTexWidth		= dwTexWidth << 1;
			iNumCharsWide	= dwTexWidth / m_width;
			if( iNumCharsHigh * iNumCharsWide <= 256 )
			{
				dwTexHeight		= dwTexHeight << 1;
				iNumCharsHigh	= dwTexHeight / m_height;

				if( iNumCharsHigh * iNumCharsWide <= 256 )
				{
					// Texture required is ridiculously large, or UNICODE.
					_ASSERT( false );
				}
			}
		}

		D3DLOCKED_RECT lockRect;
		TEXHANDLE hTex;
		DWORD dwXVal, dwYVal, dwIndex;
		BYTE * pSrc;
		BYTE bByte, bMask;
		float fTexWidth, fTexHeight;

		// Create the texture - A8R8G8B8.
		if( CD3DDevice9::Get()->GetDevFlags()->bSupportsA1R5G6B6Format == false )
		{
			m_pFontTex = CreatePrivateSurface( D3DFMT_A8R8G8B8, dwTexWidth, dwTexHeight, "Font texture" );
			hTex = m_pFontTex->GetTexHandle();

			// Lock texture to copy font data into.
			CVRAMManager::Get()->LockTexture( hTex, &lockRect );

			// Copy in all the data.
			DWORD * pData = (DWORD*) lockRect.pBits;
			dwXVal = 0;
			dwYVal = 0;					// Top left pixel index for the current char.
			pSrc = m_pdata;
			fTexWidth = (float) ( dwTexWidth - 1 );
			fTexHeight = (float) ( dwTexHeight - 1 );

			for( i=0; i<256; i++ )
			{
				// Calculate the uv coords for this character.
				m_pCharTexData[i].fU1 = (float) dwXVal / fTexWidth;
				m_pCharTexData[i].fV1 = (float) dwYVal / fTexHeight;
				m_pCharTexData[i].fU2 = (float) ( dwXVal + m_data[i].m_size.X() ) / fTexWidth;
				m_pCharTexData[i].fV2 = (float) ( dwYVal + m_data[i].m_size.Y() ) / fTexHeight;

				// Now process the bit array for this char.
				for( DWORD y=0; y<(DWORD)m_data[i].m_size.Y(); y++ )
				{
					// Calculate initial offset into texture.
					dwIndex = ( ( y + dwYVal ) * ( lockRect.Pitch / 4 ) ) + dwXVal;

					// Prepare the mini data buffer.
					DWORD dwLineSize = ( m_data[i].m_size.X() + 7 ) / 8;
					_ASSERT( dwLineSize <= 32 );

					for( DWORD x=0; x<dwLineSize; x++ )
					{
						// Get the next byte to process and reset the mask.
						bByte = *pSrc++;
						bMask = 0x01;

						// Process this byte.
						for( DWORD dwBitCount=0; dwBitCount<8; dwBitCount ++ )
						{
							if( ( bMask & bByte ) != 0 )
							{
								// Store 0xFFFFFFFF
								pData[ dwIndex ++ ] = 0xFFFFFFFF;
							}
							else
							{
								// Store 0.
								pData[ dwIndex ++ ] = 0;
							}
							bMask = bMask << 1;
						}
					}
				}

				// Update the top left coords.
				//dwXVal += m_data[i].m_size.X();
				dwXVal += m_width;
				if( dwXVal >= ( dwTexWidth - 1) )
				{
					dwXVal = 0;
					dwYVal += m_height;
				}
				else if(	( i < 255 ) &&
							( dwXVal + m_width >= dwTexWidth ) )
							//( dwXVal + m_data[i+1].m_size.X() >= dwTexWidth-1 ) )
				{
					dwXVal = 0;
					dwYVal += m_height;
				}
			}
		}
		else
		{
			// 16 bit version, smaller texture required.
			m_pFontTex = CreatePrivateSurface( D3DFMT_A1R5G5B5, dwTexWidth, dwTexHeight, "Font texture" );
			hTex = m_pFontTex->GetTexHandle();

			// Lock texture to copy font data into.
			CVRAMManager::Get()->LockTexture( hTex, &lockRect );

			// Copy in all the data.
			WORD * pData = (WORD*) lockRect.pBits;
			dwXVal = 0;
			dwYVal = 0;					// Top left pixel index for the current char.
			pSrc = m_pdata;
			fTexWidth = (float) ( dwTexWidth - 1 );
			fTexHeight = (float) ( dwTexHeight - 1 );

			for( i=0; i<256; i++ )
			{
				// Calculate the uv coords for this character.
				m_pCharTexData[i].fU1 = (float) dwXVal / fTexWidth;
				m_pCharTexData[i].fV1 = (float) dwYVal / fTexHeight;
				m_pCharTexData[i].fU2 = (float) ( dwXVal + m_data[i].m_size.X() ) / fTexWidth;
				m_pCharTexData[i].fV2 = (float) ( dwYVal + m_data[i].m_size.Y() ) / fTexHeight;

				// Now process the bit array for this char.
				for( DWORD y=0; y<(DWORD)m_data[i].m_size.Y(); y++ )
				{
					// Calculate initial offset into texture.
					dwIndex = ( ( y + dwYVal ) * ( lockRect.Pitch / 2 ) ) + dwXVal;

					// Prepare the mini data buffer.
					DWORD dwLineSize = ( m_data[i].m_size.X() + 7 ) / 8;
					_ASSERT( dwLineSize <= 32 );

					for( DWORD x=0; x<dwLineSize; x++ )
					{
						// Get the next byte to process and reset the mask.
						bByte = *pSrc++;
						bMask = 0x01;

						// Process this byte.
						for( DWORD dwBitCount=0; dwBitCount<8; dwBitCount ++ )
						{
							if( ( bMask & bByte ) != 0 )
							{
								// Store 0xFFFF
								pData[ dwIndex ++ ] = 0xFFFF;
							}
							else
							{
								// Store 0.
								pData[ dwIndex ++ ] = 0;
							}
							bMask = bMask << 1;
						}
					}
				}

				// Update the top left coords.
				dwXVal += m_width;
				if( dwXVal >= ( dwTexWidth - 1) )
				{
					dwXVal = 0;
					dwYVal += m_height;
				}
				else if(	( i < 255 ) &&
							( dwXVal + m_width >= dwTexWidth ) )
				{
					dwXVal = 0;
					dwYVal += m_height;
				}
			}
		}
		
		// Finished, unlock texture.
		CVRAMManager::Get()->UnlockTexture( hTex );
	}


	void FillFontData(HFONT	hfont)
	{
		int	length = 0;
		int	index;

		// Figure out the total	data size
		for	(index = 0;	index <	256; index++) 
		{
			int	xsize =	m_data[index].m_size.X();
			int	ysize =	m_data[index].m_size.Y();

			m_data[index].m_offset = length;

			length += ((xsize +	7) / 8)	* ysize;
		}

		// Allocate	the	data
		m_dataSize = (length + 3) &	(~3);
		m_pdata	   = new BYTE[m_dataSize];

		// Create a	dib	section	to get the characters with
		class MyHeader 
		{
		public:
			BITMAPINFOHEADER bmih;
			RGBQUAD			 colors[256];
		} bmih;

		bmih.bmih.biSize		  =	sizeof(BITMAPINFOHEADER);
		bmih.bmih.biWidth		  =	m_width; 
		bmih.bmih.biHeight		  =	-m_height; 
		bmih.bmih.biPlanes		  =	1; 
		bmih.bmih.biBitCount	  =	8; 
		bmih.bmih.biCompression	  =	BI_RGB;	
		bmih.bmih.biSizeImage	  =	0; 
		bmih.bmih.biXPelsPerMeter =	0; 
		bmih.bmih.biYPelsPerMeter =	0; 
		bmih.bmih.biClrUsed		  =	0; 
		bmih.bmih.biClrImportant  =	0; 

		for	(index = 0;	index <	256; index++) 
		{
			bmih.colors[index].rgbBlue	   = index;
			bmih.colors[index].rgbGreen	   = index;
			bmih.colors[index].rgbRed	   = index;
			bmih.colors[index].rgbReserved = 0;
		}

		BYTE*	pbits;
		HBITMAP	hbitmap	= ::CreateDIBSection( NULL,	(BITMAPINFO*)&bmih,	DIB_RGB_COLORS,	(void**)&pbits,	NULL, 0	);
		ZAssert(hbitmap	!= NULL);

		HDC	hdcBitmap =	::CreateCompatibleDC(NULL);
		ZAssert(hdcBitmap != NULL);
		HBITMAP	hbitmapOld = (HBITMAP)::SelectObject(hdcBitmap,	hbitmap);
		ZAssert(hbitmapOld != NULL);
		HFONT hfontOld = (HFONT)::SelectObject(hdcBitmap, hfont);
		ZVerify(hfontOld !=	NULL);
		ZVerify(::SetBkMode(hdcBitmap, TRANSPARENT));
		ZVerify(::SetTextColor(hdcBitmap, Color::White().MakeCOLORREF()) !=	CLR_INVALID);
		ZVerify(::SetBkColor  (hdcBitmap, Color::Black().MakeCOLORREF()) !=	CLR_INVALID);

		// Get the character bitmaps
		BYTE* pdata	= m_pdata;
		int	scanBytes =	((m_width +	3) / 4)	* 4;

		for	(index = 0;	index <	256; index++) 
		{
			int	xsize =	m_data[index].m_size.X();
			int	ysize =	m_data[index].m_size.Y();
			unsigned char ch = index;

			ZVerify(::FillRect(	hdcBitmap, &(RECT)WinRect(0, 0,	m_width, m_height),	(HBRUSH)::GetStockObject(BLACK_BRUSH) ));
			ZVerify(::TextOut(hdcBitmap, 0,	0, (PCC)&ch, 1));

			// pull	out	the	data 
			int	xbytes = (xsize	+ 7) / 8;
			for	(int yindex	= 0; yindex	< ysize; yindex++) 
			{
				for	(int xbyte = 0;	xbyte <	xbytes;	xbyte++) 
				{
					WORD word =	0;
					for	(int xbit =	0; xbit	< 8; xbit++) 
					{
						if (pbits[yindex * scanBytes + xbyte * 8 + xbit] ==	0xff) 
						{
							word |=	0x100;
						}
						word >>= 1;
					}	 
					*pdata = (BYTE)word;
					pdata++;
				}
			}
		}
		ZAssert(pdata == m_pdata + length);

		// Release the DC we created
		ZVerify(::SelectObject(hdcBitmap, hfontOld));
		ZVerify(::SelectObject(hdcBitmap, hbitmapOld));
		ZVerify(::DeleteDC(hdcBitmap));
		ZVerify(::DeleteObject(hbitmap));
	}

	//////////////////////////////////////////////////////////////////////////////
	// GetSizes()
	// Get the width of	each character.
	//////////////////////////////////////////////////////////////////////////////

	void GetSizes(HFONT	hfont)
	{
		HDC	hdc	= ::GetDC(NULL);
		ZVerify(hdc	!= NULL);

		HFONT hfontOld = (HFONT)::SelectObject(hdc,	hfont);
		ZVerify(hfontOld !=	NULL);

		// Get the character widths, offsets
		m_height = 0;
		m_width	 = 0;
		int	index;

		for	(index = 0;	index <	256; index++) 
		{
			SIZE size;
			unsigned char ch = index;

			ZVerify(::GetTextExtentPoint32( hdc, (PCC)&ch, 1,	&size )); // KG- upd to 32b api

			m_data[index].m_size   = WinPoint(size.cx, size.cy);

			m_height = max(m_height, size.cy);
			m_width	 = max(m_width,	 size.cx);
		}

		// release the hdc
		ZVerify(::SelectObject(hdc,	hfontOld));
		ZVerify(::ReleaseDC(NULL, hdc));

		// Make	that all of	the	value GDI returned to us are good
		for	(index = 0;	index <	256; index++) 
		{
			// ZAssert(m_data[index].m_size.X() >	0			); // KG- Vista doesnt like this
			ZAssert(m_data[index].m_size.X() <	m_height * 4);
			ZAssert(m_data[index].m_size.Y() ==	m_height	);
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	//
	// 
	//
	//////////////////////////////////////////////////////////////////////////////

public:
	FontImpl(HFONT hfont) :	m_pdata(NULL)
	{
		GetSizes(hfont);
		FillFontData(hfont);
		ZVerify(::DeleteObject(hfont));

		// Generate the equivalent texture.
		GenerateFontTextureFromFontData( );
	}

	  FontImpl( IBinaryReaderSite * psite )
	  {
		  m_height		= psite->GetDWORD();
		  m_width		= psite->GetDWORD();
		  m_dataSize	= psite->GetDWORD();

		  memcpy(m_data, psite->GetPointer(),	sizeof(m_data));
		  psite->MovePointer(sizeof(m_data));

		  m_pdata = new BYTE[m_dataSize];	
		  memcpy(m_pdata,	psite->GetPointer(), m_dataSize);
		  psite->MovePointer(m_dataSize);

		// Generate the equivalent texture.
		GenerateFontTextureFromFontData( );
	}

	~FontImpl()
	{
		if (m_pdata) 
		{
			delete m_pdata;
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	//
	// members
	//
	//////////////////////////////////////////////////////////////////////////////

	void Write(IMDLBinaryFile* pmdlFile)
	{
		pmdlFile->WriteReference("ImportFont");
		TRef<ZFile>	pfile =	pmdlFile->WriteBinary();

		pfile->Write(m_height);
		pfile->Write(m_width);
		pfile->Write(m_dataSize);
		pfile->Write(m_data, sizeof(m_data));
		pfile->Write(m_pdata, m_dataSize);
	}

	//////////////////////////////////////////////////////////////////////////////
	//
	// IEngineFont members
	//
	//////////////////////////////////////////////////////////////////////////////

	int	GetHeight()
	{
		return m_height;
	}

	WinPoint GetTextExtent(const ZString& str)
	{
		int	count =	str.GetLength();
		int	width =	0;

		for	(int index = 0;	index <	count; index++)	
		{
			if (str[index] == START_COLOR_CODE)
				index += 10	- 1;
			else if	(str[index]	== END_COLOR_CODE)
				index += 2 - 1;
			else
				width += m_data[(BYTE)str[index]].m_size.X();
		}
		return WinPoint(width, m_height);
	}

	WinPoint GetTextExtent(const char* sz)
	{
		int	count =	strlen(sz);
		int	width =	0;

		for	(int index = 0;	index <	count; index++)	{
			if (sz[index] == START_COLOR_CODE)
				index += 10	- 1;
			else if	(sz[index] == END_COLOR_CODE)
				index += 2 - 1;
			else
				width += m_data[(BYTE)sz[index]].m_size.X();
		}

		return WinPoint(width, m_height);
	}

	int	GetMaxTextLength(const ZString&	str, int maxWidth, bool	bLeft)
	{
		int	count =	str.GetLength();
		int	width =	0;

		for	(int index = 0;	index <	count; index++)	
		{
			int	ichar =	bLeft ?	index :	count -	(index + 1);
			// note	that this method is	not	adjusted to	account	for	colored	text, since	it
			// may work	backwards across the string, resulting in a	difficult case that	I
			// don't think is causing problems right now, ergo no need to fix it.
			width += m_data[(BYTE)str[ichar]].m_size.X();
			if (width >	maxWidth)
				return index;
		}

		return count;
	}


	  
	////////////////////////////////////////////////////////////////////////////////////////////////
	// DrawString()
	//
	////////////////////////////////////////////////////////////////////////////////////////////////
	void DrawString(	const WinPoint &	point,
						const ZString &		str,
						const Color &		colour )
	{
		_ASSERT( str.GetLength() < 256 );
		float fX0, fX1, fY0, fY1;
		DWORD dwA, dwR, dwG, dwB;
		dwA = 255;
		dwR = (DWORD) ( colour.R() * 255.0f );
		dwG = (DWORD) ( colour.G() * 255.0f );
		dwB = (DWORD) ( colour.B() * 255.0f );
		D3DCOLOR currColour = D3DCOLOR_ARGB( dwA, dwR, dwG, dwB );
		D3DCOLOR originalColor = currColour; // kg fix: save original color (parameter color)
		if( str.GetLength() == 0 ) 
		{
			return;
		}

		CD3DDevice9 * pDev = pDev->Get();

		// Set the texture function.
		pDev->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
		pDev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		pDev->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		pDev->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
		pDev->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

		pDev->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		pDev->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
		pDev->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		
		pDev->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		pDev->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
		
		pDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		pDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		pDev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		pDev->SetRenderState( D3DRS_COLORVERTEX, TRUE );
		pDev->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );

        pDev->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE ); //imago 8/6/09

		// Generate the geometry for rendering the text.
		int iChar = 0;
		int iCurrVert = 0;
		fX0 = (float) point.X();
		fY0 = (float) point.Y();

		// Sanity check, this is the most we can fit into the current font dyn VB.
		// Should be enough though.
		_ASSERT( str.GetLength() < 2048 );
		
		UIFONTVERTEX * pFontVerts;
		
//		CVBIBManager::SVBIBHandle * pFontDynVB = CVertexGenerator::GetUIFontVertsVB();
		CVBIBManager::SVBIBHandle * pFontDynVB = 
			CVertexGenerator::Get()->GetPredefinedDynamicBuffer( CVertexGenerator::ePDBT_UIFontVB );
		_ASSERT( pFontDynVB != NULL );

		if( CVBIBManager::Get()->LockDynamicVertexBuffer( 
								pFontDynVB, 
								str.GetLength() * 6,
								(void**) &pFontVerts ) == false )
		{
			// Failed to lock the vertex buffer.
			_ASSERT( false );
			return;
		}

		while( iChar < str.GetLength() ) 
		{
			BYTE ch = str[iChar];

			switch (ch)
			{
			case START_COLOR_CODE:
				{
					// Rock: I believe this is actually incorrect for when the code is entered using the client, in which case it has become a single byte representing the number 129.
					// In that case, a character is ignored.
					iChar += 2;

					// Read the color, in the form rrggbbaa, where each pair of digits is a hex value.
					if (IsValidColourCode(str, iChar) == true)
					{
						currColour = ReadColourD3D(str, iChar);
					}
					else {
						//invalid: Skip what is supposed to be the color code
						iChar += 8;
					}
					
					if( iChar >= str.GetLength() )
						break;
				}
				break;

			case END_COLOR_CODE:
				iChar += 2;
				currColour = originalColor; // kg fix: currColour ended so we restore original color (parameter)
				break;

			default:
				{
					const CharData&	charData	= m_data[ch];

					fX1 = fX0 + (float) charData.m_size.X();
					fY1 = fY0 + (float) charData.m_size.Y();

					pFontVerts[ iCurrVert ].x		= fX0;
					pFontVerts[ iCurrVert ].y		= fY0;
					pFontVerts[ iCurrVert ].z		= 0.5f;
					pFontVerts[ iCurrVert ].rhw		= 1.0f;
					pFontVerts[ iCurrVert ].fU		= m_pCharTexData[ ch ].fU1;
					pFontVerts[ iCurrVert ].fV		= m_pCharTexData[ ch ].fV1;
					pFontVerts[ iCurrVert++ ].color	= currColour;

					pFontVerts[ iCurrVert ].x		= fX1;
					pFontVerts[ iCurrVert ].y		= fY0;
					pFontVerts[ iCurrVert ].z		= 0.5f;
					pFontVerts[ iCurrVert ].rhw		= 1.0f;
					pFontVerts[ iCurrVert ].fU		= m_pCharTexData[ ch ].fU2;
					pFontVerts[ iCurrVert ].fV		= m_pCharTexData[ ch ].fV1;
					pFontVerts[ iCurrVert++ ].color	= currColour;

					pFontVerts[ iCurrVert ].x		= fX0;
					pFontVerts[ iCurrVert ].y		= fY1;
					pFontVerts[ iCurrVert ].z		= 0.5f;
					pFontVerts[ iCurrVert ].rhw		= 1.0f;
					pFontVerts[ iCurrVert ].fU		= m_pCharTexData[ ch ].fU1;
					pFontVerts[ iCurrVert ].fV		= m_pCharTexData[ ch ].fV2;
					pFontVerts[ iCurrVert++ ].color	= currColour;

					pFontVerts[ iCurrVert ].x		= fX0;
					pFontVerts[ iCurrVert ].y		= fY1;
					pFontVerts[ iCurrVert ].z		= 0.5f;
					pFontVerts[ iCurrVert ].rhw		= 1.0f;
					pFontVerts[ iCurrVert ].fU		= m_pCharTexData[ ch ].fU1;
					pFontVerts[ iCurrVert ].fV		= m_pCharTexData[ ch ].fV2;
					pFontVerts[ iCurrVert++ ].color	= currColour;

					pFontVerts[ iCurrVert ].x		= fX1;
					pFontVerts[ iCurrVert ].y		= fY0;
					pFontVerts[ iCurrVert ].z		= 0.5f;
					pFontVerts[ iCurrVert ].rhw		= 1.0f;
					pFontVerts[ iCurrVert ].fU		= m_pCharTexData[ ch ].fU2;
					pFontVerts[ iCurrVert ].fV		= m_pCharTexData[ ch ].fV1;
					pFontVerts[ iCurrVert++ ].color	= currColour;

					pFontVerts[ iCurrVert ].x		= fX1;
					pFontVerts[ iCurrVert ].y		= fY1;
					pFontVerts[ iCurrVert ].z		= 0.5f;
					pFontVerts[ iCurrVert ].rhw		= 1.0f;
					pFontVerts[ iCurrVert ].fU		= m_pCharTexData[ ch ].fU2;
					pFontVerts[ iCurrVert ].fV		= m_pCharTexData[ ch ].fV2;
					pFontVerts[ iCurrVert++ ].color	= currColour;

					fX0 += (float) charData.m_size.X();

					iChar ++;
				}
				break;
			}
		}
		
		// Finished, unlock the buffer, set the stream.
		CVBIBManager::Get()->UnlockDynamicVertexBuffer( pFontDynVB );

		// Render.
		CVRAMManager::Get()->SetTexture( m_pFontTex->GetTexHandle(), 0 );
		CVBIBManager::Get()->SetVertexStream( pFontDynVB );
		pDev->SetFVF( D3DFVF_UIFONTVERTEX );
		pDev->DrawPrimitive(	D3DPT_TRIANGLELIST,
									pFontDynVB->dwFirstElementOffset,
									iCurrVert / 3 );

		// Reset the texture unit setup.
        pDev->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE ); //imago 8/6/09
	}
	

	  int	 HexDigitToInt (int	hexDigit)
	  {
		  if (isdigit	(hexDigit))
			  return hexDigit	- '0';
		  else if	(isxdigit (hexDigit))
			  return (tolower	(hexDigit) - 'a') +	10;
		  else
			  return -1;
	  }

	  int	ReadHexPair	(const ZString&	str, int& ichar)
	  {
		  int	highNibble = HexDigitToInt (str[ichar++]);
		  int	lowNibble =	HexDigitToInt (str[ichar++]);
		  assert ((highNibble	>= 0) && (lowNibble	>= 0));
		  return ((highNibble	<< 4) |	lowNibble);
	  }

	  bool IsValidColourCode(const ZString& str, int & ichar)
	  {
		  int i;

		  if (str.GetLength() < ichar + 8)
		  {
			  return false;
		  }

		  for (i = ichar; i < ichar + 8; ++i)
		  {
			  if (HexDigitToInt(str[i]) == -1)
			  {
				  return false;
			  }
		  }
		  return true;
	  }

	  D3DCOLOR ReadColourD3D( const ZString & str, int & ichar )
	  {
		  DWORD dwR, dwG, dwB, dwA;

		  dwR = ReadHexPair ( str,	ichar );
		  dwG = ReadHexPair ( str,	ichar );
		  dwB = ReadHexPair ( str,	ichar );
		  dwA = ReadHexPair ( str,	ichar );
		  return D3DCOLOR_ARGB( dwA, dwR, dwG, dwB );
	  }

	  Color	ReadColor (const ZString& str, int&	ichar)
	  {
		  float	red	= ReadHexPair (str,	ichar) / 255.0f;
		  float	green =	ReadHexPair	(str, ichar) / 255.0f;
		  float	blue = ReadHexPair (str, ichar)	/ 255.0f;
		  float	alpha =	ReadHexPair	(str, ichar) / 255.0f;
		  return Color (red, green, blue,	alpha);
	  }


};

TRef<IEngineFont> CreateEngineFont(HFONT hfont)
{
	return new FontImpl(hfont);
}

TRef<IEngineFont> CreateEngineFont(IBinaryReaderSite* psite)
{
	return new FontImpl(psite);
}


//////////////////////////////////////////////////////////////////////////////
//
// Color code utility function
//
//////////////////////////////////////////////////////////////////////////////
// moved to zlib/color.cpp

//////////////////////////////////////////////////////////////////////////////
//
// Fonts
//
//////////////////////////////////////////////////////////////////////////////

ZString	GetString(int indent, TRef<IEngineFont>	pfont)
{
	ZUnimplemented();

	return "font";
}

void Write(IMDLBinaryFile* pmdlFile, TRef<IEngineFont> pfont)
{
	FontImpl* pimpl; CastTo(pimpl, pfont);

	pimpl->Write(pmdlFile);
}

ZString	GetFunctionName(const TRef<IEngineFont>& value)
{
	return "ImportFont";
}
