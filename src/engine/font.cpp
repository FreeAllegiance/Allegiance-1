#include "font.h"

#include <base.h>

#include "D3DDevice9.h"
#include "UIVertexDefn.h"
#include "VertexGenerator.h"
#include "enginep.h"

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
    int m_bitsPerPixel;
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

        DWORD dwInitialWidth = 256, dwInitialHeight = 256;
        // Calculate the size of texture required.
        DWORD dwTexWidth, dwTexHeight;		// Initial size

        //we try different sizes for the texture. Increase width by 2x, then increase height by 2x, then width again.
        //for a max texture size of 2048x2048, we need to increase width 8x and height 8x from the starting 256x256, so three rounds each
        for (int round = 0; round < 6; ++round) {
            int exponential_width, exponential_height;
            if (round % 2 == 0) {
                exponential_width = round / 2;
                exponential_height = round / 2;
            }
            else {
                //only increase width on odd rounds
                exponential_width = (round + 1) / 2;
                exponential_height = (round - 1) / 2;
            }

            dwTexWidth = dwInitialWidth << exponential_width;
            dwTexHeight = dwInitialHeight << exponential_height;

            int iNumCharsWide = dwTexWidth / m_width;		// using the font data.
            int iNumCharsHigh = dwTexHeight / m_height;

            if (iNumCharsHigh * iNumCharsWide >= 256) {
                //we are able to place 256 characters, it fits.
                break;
            }
        }

        {
            int iNumCharsWide = dwTexWidth / m_width;
            int iNumCharsHigh = dwTexHeight / m_height;
            if (iNumCharsHigh * iNumCharsWide < 256)
            {
                // Texture required is ridiculously large, or UNICODE.
                ZAssert(false);
            }
        }

		D3DLOCKED_RECT lockRect;
		TEXHANDLE hTex;
		DWORD dwXVal, dwYVal, dwIndex;
		BYTE * pSrc;
		BYTE bByte, bMask;
		float fTexWidth, fTexHeight;

		// Create the texture - D3DFMT_A8L8.
		m_pFontTex = CreatePrivateSurface(D3DFMT_A8L8, dwTexWidth, dwTexHeight, "Font texture" );
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

		for(int i=0; i<256; i++ )
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
                DWORD xwidth = m_data[i].m_size.X();
                ZAssert( dwLineSize <= 32 );

                switch (m_bitsPerPixel) {
                case 1:
                    for (DWORD x = 0; x < dwLineSize; x++)
                    {
                        // Get the next byte to process and reset the mask.
                        bByte = *pSrc++;
                        bMask = 0x01;
                        // Process this byte.
                        for (DWORD dwBitCount = 0; dwBitCount < 8; dwBitCount++)
                        {
                            if ((bMask & bByte) != 0)
                            {
                                // Store 0xFFFF
                                pData[dwIndex++] = 0xFFFF;
                            }
                            else
                            {
                                // Store 0.
                                pData[dwIndex++] = 0;
                            }
                            bMask = bMask << 1;
                        }
                    }
                    break;
                case 8:
                    for (DWORD x = 0; x < xwidth; x++) {
                        bByte = *pSrc++;
                        pData[dwIndex++] = (bByte << 8) | 0x00FF;
                    }

                    break;
                default:
                    ZAssert(false);
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

			length += xsize	* ysize;
		}

		// Allocate	the	data
		m_dataSize = length;
		m_pdata	   = new BYTE[m_dataSize];

        // We are going to use an 8 bit bitmap
        m_bitsPerPixel = 8;

        // Rock: I can't get anti aliasing to work with an 8 bit bitmap. Just use a 32 bit one
        BITMAPINFO bmih;
        ZeroMemory(&bmih, sizeof(bmih));
        bmih.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmih.bmiHeader.biWidth = m_width;
        bmih.bmiHeader.biHeight = -m_height;
        bmih.bmiHeader.biPlanes = 1;
        bmih.bmiHeader.biBitCount = 32;
        bmih.bmiHeader.biCompression = BI_RGB;

		BYTE*	pbits;
        DWORD*	pcolorbytes;
		//HBITMAP	hbitmap	= ::CreateDIBSection( NULL,	(BITMAPINFO*)&bmih,	DIB_RGB_COLORS,	(void**)&pbits,	NULL, 0	);
        HBITMAP	hbitmap = ::CreateDIBSection(NULL, &bmih, DIB_RGB_COLORS, (void**)&pcolorbytes, NULL, 0);
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
		int	scanBytes = m_width;

		for	(index = 0;	index <	256; index++) 
		{
			int	xsize =	m_data[index].m_size.X();
			int	ysize =	m_data[index].m_size.Y();
			unsigned char ch = index;
            RECT r = WinRect(0, 0,	m_width, m_height);
            ZVerify(::FillRect(	hdcBitmap, &r,	(HBRUSH)::GetStockObject(BLACK_BRUSH) ));
			ZVerify(::TextOut(hdcBitmap, 0,	0, (PCC)&ch, 1));

			// pull	out	the	data 
			for	(int yindex	= 0; yindex	< ysize; yindex++) 
			{
                for (int xindex = 0; xindex < xsize; xindex++)
                {
                    DWORD pixel = pcolorbytes[(yindex * scanBytes + xindex)];
                    BYTE pixelbyte = (BYTE)((pixel & 0xff) >> 0); //just use the blue channel
                    *pdata = pixelbyte;
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

            m_height = std::max<uint32_t>(m_height, size.cy);
            m_width	 = std::max<uint32_t>(m_width,	 size.cx);
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
          m_bitsPerPixel = 1;

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

    void DrawString(const Point &	point,
        const ZString &		str,
        const Color &		colour,
        bool bYAxisInversion)
    {
        ZAssert(str.GetLength() < 256);
        float fX0, fX1, fY0, fY1;
        DWORD dwA, dwR, dwG, dwB;
        dwA = (DWORD)(colour.A() * 255.0f);
        dwR = (DWORD)(colour.R() * 255.0f);
        dwG = (DWORD)(colour.G() * 255.0f);
        dwB = (DWORD)(colour.B() * 255.0f);
        D3DCOLOR currColour = D3DCOLOR_ARGB(dwA, dwR, dwG, dwB);
        D3DCOLOR originalColor = currColour; // kg fix: save original color (parameter color)
        if (str.GetLength() == 0)
        {
            return;
        }

        CD3DDevice9 * pDev = pDev->Get();

        // when we are not resizing, we would like nearest point sampling. when we are upscaling, (bi)linear. This seems to achieve that.
        pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);

        pDev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE); //imago 8/6/09

        DWORD dwCurrentLighting;
        pDev->GetRenderState(D3DRS_LIGHTING, &dwCurrentLighting);

        pDev->SetRenderState(D3DRS_LIGHTING, (DWORD)false);

        pDev->SetRenderState(D3DRS_COLORVERTEX, TRUE);
        pDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);

                                                                 // Generate the geometry for rendering the text.
        int iChar = 0;
        int iCurrVert = 0;

        // If we are going to sample using nearest point, we want to have the characters on integer boundaries. Just force it in any case.
        fX0 = (float)(int)point.X();
        fY0 = (float)(int)point.Y();

        // Sanity check, this is the most we can fit into the current font dyn VB.
        // Should be enough though.
        ZAssert(str.GetLength() < 2048);

        UIFONTVERTEX2 * pFontVerts;

        //		CVBIBManager::SVBIBHandle * pFontDynVB = CVertexGenerator::GetUIFontVertsVB();
        CVBIBManager::SVBIBHandle * pFontDynVB =
            CVertexGenerator::Get()->GetPredefinedDynamicBuffer(CVertexGenerator::ePDBT_UIFont2VB);
        ZAssert(pFontDynVB != NULL);

        if (CVBIBManager::Get()->LockDynamicVertexBuffer(
            pFontDynVB,
            str.GetLength() * 6,
            (void**)&pFontVerts) == false)
        {
            // Failed to lock the vertex buffer.
            ZAssert(false);
            return;
        }

        while (iChar < str.GetLength())
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

                    if (iChar >= str.GetLength())
                        break;
                }
                break;

                case END_COLOR_CODE:
                    iChar += 2;
                    currColour = originalColor; // kg fix: currColour ended so we restore original color (parameter)
                    break;

                default:
                {
                    const CharData&	charData = m_data[ch];

                    fX1 = fX0 + (float)charData.m_size.X();
                    fY1 = fY0 + (float)charData.m_size.Y();
                    if (bYAxisInversion) {
                        float tmp = fY1;
                        fY1 = fY0;
                        fY0 = tmp;
                    }
                    float z = 0.0f;

                    pFontVerts[iCurrVert].x = fX0;
                    pFontVerts[iCurrVert].y = fY0;
                    pFontVerts[iCurrVert].z = z;
                    pFontVerts[iCurrVert].fU = m_pCharTexData[ch].fU1;
                    pFontVerts[iCurrVert].fV = m_pCharTexData[ch].fV1;
                    pFontVerts[iCurrVert++].color = currColour;

                    pFontVerts[iCurrVert].x = fX1;
                    pFontVerts[iCurrVert].y = fY0;
                    pFontVerts[iCurrVert].z = z;
                    pFontVerts[iCurrVert].fU = m_pCharTexData[ch].fU2;
                    pFontVerts[iCurrVert].fV = m_pCharTexData[ch].fV1;
                    pFontVerts[iCurrVert++].color = currColour;

                    pFontVerts[iCurrVert].x = fX0;
                    pFontVerts[iCurrVert].y = fY1;
                    pFontVerts[iCurrVert].z = z;
                    pFontVerts[iCurrVert].fU = m_pCharTexData[ch].fU1;
                    pFontVerts[iCurrVert].fV = m_pCharTexData[ch].fV2;
                    pFontVerts[iCurrVert++].color = currColour;

                    pFontVerts[iCurrVert].x = fX0;
                    pFontVerts[iCurrVert].y = fY1;
                    pFontVerts[iCurrVert].z = z;
                    pFontVerts[iCurrVert].fU = m_pCharTexData[ch].fU1;
                    pFontVerts[iCurrVert].fV = m_pCharTexData[ch].fV2;
                    pFontVerts[iCurrVert++].color = currColour;

                    pFontVerts[iCurrVert].x = fX1;
                    pFontVerts[iCurrVert].y = fY0;
                    pFontVerts[iCurrVert].z = z;
                    pFontVerts[iCurrVert].fU = m_pCharTexData[ch].fU2;
                    pFontVerts[iCurrVert].fV = m_pCharTexData[ch].fV1;
                    pFontVerts[iCurrVert++].color = currColour;

                    pFontVerts[iCurrVert].x = fX1;
                    pFontVerts[iCurrVert].y = fY1;
                    pFontVerts[iCurrVert].z = z;
                    pFontVerts[iCurrVert].fU = m_pCharTexData[ch].fU2;
                    pFontVerts[iCurrVert].fV = m_pCharTexData[ch].fV2;
                    pFontVerts[iCurrVert++].color = currColour;

                    fX0 += (float)charData.m_size.X();

                    if (bYAxisInversion) {
                        float tmp = fY1;
                        fY1 = fY0;
                        fY0 = tmp;
                    }

                    iChar++;
                }
                break;
            }
        }

        // Finished, unlock the buffer, set the stream.
        CVBIBManager::Get()->UnlockDynamicVertexBuffer(pFontDynVB);

        // Render.
        LPDIRECT3DBASETEXTURE9 pPreviousTexture = nullptr;
        pDev->GetTexture(0, &pPreviousTexture);

        CVRAMManager::Get()->SetTexture(m_pFontTex->GetTexHandle(), 0);
        CVBIBManager::Get()->SetVertexStream(pFontDynVB);
        pDev->SetFVF(D3DFVF_UIFONTVERTEX2);
        pDev->DrawPrimitive(D3DPT_TRIANGLELIST,
            pFontDynVB->dwFirstElementOffset,
            iCurrVert / 3);

        // Reset the texture unit setup.
        pDev->SetTexture(0, pPreviousTexture);
        if (pPreviousTexture) {
            pPreviousTexture->Release(); //According to the documentation this must be called to reduce refcounter
        }

        pDev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE); //imago 8/6/09
        pDev->SetRenderState(D3DRS_LIGHTING, dwCurrentLighting);
    }

	void DrawStringScreenSpace(	const WinPoint &	point,
						const ZString &		str,
						const Color &		colour )
	{
        ZAssert( str.GetLength() < 256 );
		float fX0, fX1, fY0, fY1;
		DWORD dwA, dwR, dwG, dwB;
		dwA = (DWORD) ( colour.A() * 255.0f );
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
		
		pDev->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		pDev->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );

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
        ZAssert( str.GetLength() < 2048 );
		
		UIFONTVERTEX * pFontVerts;
		
//		CVBIBManager::SVBIBHandle * pFontDynVB = CVertexGenerator::GetUIFontVertsVB();
		CVBIBManager::SVBIBHandle * pFontDynVB = 
			CVertexGenerator::Get()->GetPredefinedDynamicBuffer( CVertexGenerator::ePDBT_UIFontVB );
        ZAssert( pFontDynVB != NULL );

		if( CVBIBManager::Get()->LockDynamicVertexBuffer( 
								pFontDynVB, 
								str.GetLength() * 6,
								(void**) &pFontVerts ) == false )
		{
			// Failed to lock the vertex buffer.
            ZAssert( false );
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

struct FontIdentifier {
    const std::string name;
    int size;
    int stretch;
    bool bold;
    bool italic;
    bool underline;

    bool operator<(FontIdentifier const& other) const {
        //compare each value
        if (name < other.name) return true;
        if (name > other.name) return false;

        if (size < other.size) return true;
        if (size > other.size) return false;

        if (stretch < other.stretch) return true;
        if (stretch > other.stretch) return false;

        if (bold < other.bold) return true;
        if (bold > other.bold) return false;

        if (italic < other.italic) return true;
        if (italic > other.italic) return false;

        if (underline < other.underline) return true;
        if (underline > other.underline) return false;

        return false;
    }
};

//cache each font created with this function
std::map<FontIdentifier, TRef<IEngineFont>> mapFontCache;

TRef<IEngineFont> CreateEngineFont(std::string name, int size, int stretch, bool bold, bool italic, bool underline)
{
    FontIdentifier id{ name, size, stretch, bold, italic, underline };

    auto found = mapFontCache.find(id);
    if (found == mapFontCache.end()) {
        //not found
        HFONT hfont = CreateFont(
            size,
            stretch, 0, 0,
            bold ? FW_BOLD : FW_DONTCARE,
            italic ? TRUE : FALSE,
            underline ? TRUE : FALSE,
            FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_MODERN,
            name.c_str()
        );
        TRef<IEngineFont> pFont = CreateEngineFont(hfont);
        mapFontCache[id] = pFont;
        return pFont;
    }
    return found->second;
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
