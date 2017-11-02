
#include "pch.h"



//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

class D3D9RasterizerImpl : public D3D9Rasterizer
{
private:
    PrivateSurface *		m_psurface;
    D3DVIEWPORT9			m_Viewport;
//	Point					m_Size;
    DWORD					m_dwDrawPrimitiveFlags;
    Rect					m_rectClip;
	bool					m_bValid;

    #ifdef _DEBUG
        bool m_bSurfaceLost;

        void InitializeD3DCall()
        {
            m_bSurfaceLost = false;
        }

        void D3DError(HRESULT hr, const char* pszCall, const char* pszFile, int line, const char* pszModule)
        {
            if (m_bSurfaceLost) 
			{
                // ignore all errors if the surface is lost
            } 
			else 
			{
                if ( 0
//                       hr == DDERR_SURFACELOST
            //        || hr == D3DERR_SCENE_BEGIN_FAILED
                ) {
                    m_bSurfaceLost = true;
                } else {
                    DDError(hr, pszCall, pszFile, line, pszModule);
                }
            } 
        }

        #define D3DCall(hr) D3DError(hr, #hr, __FILE__, __LINE__, __MODULE__)
    #else
        void InitializeD3DCall() {}
        void D3DCall(HRESULT hr) {}
    #endif

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    D3D9RasterizerImpl(PrivateSurface* psurface) :
        m_psurface(psurface),
		m_dwDrawPrimitiveFlags( 0 )
    {
        InitializeD3DCall();

		// Create the D3D viewport
		memset( &m_Viewport, 0, sizeof( D3DVIEWPORT9 ) );
        UpdateViewport();

        // default rendering states
		D3DCall( CD3DDevice9::Get()->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ) );

		m_bValid = true;
    }

    bool IsValid()
    {
		return m_bValid;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Destructor
    //
    //////////////////////////////////////////////////////////////////////////////

    ~D3D9RasterizerImpl()
    {
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Termination
    //
    //////////////////////////////////////////////////////////////////////////////

    void Terminate()
    {
		m_bValid = false;
    }

/*    TRef<IDirect3DDevice9> GetD3DDeviceX()
    {
		_ASSERT( false );
        return NULL;
    }

    TRef<IDirect3DX> GetD3D()
    {
		return CD3DDevice9::GetD3D9();
    }*/

    D3DVIEWPORT9 * GetViewport()
    {
        return &m_Viewport;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Change Render target
    //
    //////////////////////////////////////////////////////////////////////////////

    /*  , make this work
    void SetSurface(ISurfaceImpl* psurface)
    {
        ZAssert(!m_bRendering);
        m_psurface = psurface;
        D3DCall(m_pD3DDevice->SetRenderTarget(m_psurface->GetDDS(), 0));
    }
    */

    //////////////////////////////////////////////////////////////////////////////
    //
    // Scene
    //
    //////////////////////////////////////////////////////////////////////////////

    void BeginScene()
	{
		// Clear out the last texture.
		CVRAMManager::Get()->SetTexture( INVALID_TEX_HANDLE, 0 );
	}

    void EndScene()
    {
        //   11/16/99 not checking return codes here since the Permedia 2
        //     driver on Win2K returns error every once in a while
//		m_pD3DDevice->EndScene();
  //      m_pdddevice->EndScene();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Viewport
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateViewport()
    {
        const WinPoint& size = m_psurface->GetSize();
//		const WinPoint& size = WinPoint::Cast( m_Size );
        float aspect = (float)size.X() / (float)size.Y();

		m_Viewport.X		= 0;
		m_Viewport.Y		= 0;
		m_Viewport.Width	= (int) size.X();
		m_Viewport.Height	= (int) size.Y();
		m_Viewport.MinZ		= 0.0f;
		m_Viewport.MaxZ		= 1.0f;

		CD3DDevice9::Get()->SetViewport( &m_Viewport );
    }

    void SetClipRect(const Rect& rectClip)
    {
        m_rectClip = rectClip;
    }

    void ClearZBuffer()
    {
        ZTrace(
              "Clearing ZBuffer (" 
            + ZString(m_rectClip.XMin()) + ", "
            + ZString(m_rectClip.YMin()) + ", "
            + ZString(m_rectClip.XMax()) + ", "
            + ZString(m_rectClip.YMax()) + ")"
        );

        D3DRect rect(
            (int)m_rectClip.XMin(),
            (int)m_rectClip.YMin(),
            (int)m_rectClip.XMax(),
            (int)m_rectClip.YMax()
        );

		HRESULT hr = CD3DDevice9::Get()->Clear( 1, &rect, D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
        if( hr != D3D_OK ) 
		{
            D3DCall(hr);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Attributes
    //
    //////////////////////////////////////////////////////////////////////////////

    bool HasZBuffer()
    {
        return true;
    }

    Point GetSurfaceSize()
    {
        return Point::Cast(m_psurface->GetSize());
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // State
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetTexture(Surface* psurfaceTextureArg)
    {
        if (psurfaceTextureArg != NULL) 
		{
			D3DCall( CVRAMManager::Get()->SetTexture( psurfaceTextureArg->GetTexHandle(), 0 ) );
        } 
		else 
		{
			CVRAMManager::Get()->SetTexture( INVALID_TEX_HANDLE, 0 );
        }
    }

    void SetShadeMode(ShadeMode shadeMode)
    {
        switch (shadeMode) 
		{
            case ShadeModeNone:
            case ShadeModeCopy:
                break;

            case ShadeModeFlat:
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT));
                break;

            case ShadeModeGlobalColor:
				// KG- dx9 fix
				// DX7 globalcolor mode uses the material diffuse color to blend but DX9 doesnt
				// instead we use D3DTOP_BLENDFACTORALPHA to set the origin of the alpha.
				// That value is set in Device3D::UpdateLighting() and is the same as
				// the current material diffuse color.
				// TODO: check that D3DTSS_ALPHAOP is reset to default value if not ShadeModeGlobalColor ?
				D3DCall(CD3DDevice9::Get()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_BLENDFACTORALPHA));
					
            case ShadeModeGouraud:
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));
                break;

            default:
                ZError("Invalid ShadeMode");
        }
    }

    void SetBlendMode(BlendMode blendMode)
    {
        switch (blendMode) {
 
            case BlendModeSource:
				CD3DDevice9::Get()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				CD3DDevice9::Get()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_ALPHABLENDENABLE, true));
				D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR));
				D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR));
				break;

            case BlendModeAdd:
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_ALPHABLENDENABLE, true));
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
                break;

			case BlendModeSourceAlpha:
				CD3DDevice9::Get()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				CD3DDevice9::Get()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_ALPHABLENDENABLE, true));
				D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
				D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
				break;

            default:

                ZError("Invalid BlendMode");
        }
    }

    void SetWrapMode(WrapMode wrapMode)
    {
        switch (wrapMode) {
            case WrapModeNone:
				D3DCall(CD3DDevice9::Get()->SetRenderState( D3DRS_WRAP0, 0 ) );
                break;

            case WrapModeUCylinder:
				D3DCall(CD3DDevice9::Get()->SetRenderState( D3DRS_WRAP0, D3DWRAPCOORD_0 ) );
                break;

            case WrapModeVCylinder:
				D3DCall(CD3DDevice9::Get()->SetRenderState( D3DRS_WRAP0, D3DWRAPCOORD_1 ) );
                break;

            case WrapModeTorus:
				D3DCall(CD3DDevice9::Get()->SetRenderState( D3DRS_WRAP0, D3DWRAPCOORD_0 | D3DWRAPCOORD_1 ) );
                break;

            default:
                ZError("Invalid WrapMode");
        }
    }

    void SetCullMode(CullMode cullMode)
    {
        switch (cullMode) {
            case CullModeNone:
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
                break;

            case CullModeCW:
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW));
                break;

            case CullModeCCW:
                D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
                break;

            case CullModeBoth:
                ZUnimplemented();
                break;

            default:
                ZError("Invalid CullMode");
        }
    }

	bool Has3DAcceleration( )
	{
		return true;
	}

    void SetZTest(bool bZTest)
    {
        D3DCall(CD3DDevice9::Get()->SetRenderState( D3DRS_ZENABLE, HasZBuffer() && bZTest ));
    }

    void SetZWrite(bool bZWrite)
    {
        D3DCall( CD3DDevice9::Get()->SetRenderState( D3DRS_ZWRITEENABLE, HasZBuffer() && bZWrite ) );
    }

    void SetLinearFilter(bool bLinearFilter)
    {
        if( bLinearFilter ) 
		{
			D3DCall( CD3DDevice9::Get()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR ) );
			D3DCall( CD3DDevice9::Get()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR ) );
        } 
		else 
		{
			D3DCall( CD3DDevice9::Get()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT ) );
			D3DCall( CD3DDevice9::Get()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT ) );
        }
    }

    void SetPerspectiveCorrection(bool bPerspectiveCorrection)
    {
    }

    void SetDither(bool bDither)
    {
		// Dither disabled. Looks rubbish anyway.
        //D3DCall(CD3DDevice9::Get()->SetRenderState(D3DRS_DITHERENABLE,bDither));
    }

    void SetColorKey(bool bColorKey)
	{
		// No longer supported - it's configured via the texture sampler now.
//        D3DCall(m_pD3DDevice->SetRenderState(D3DRS_COLORKEYENABLE, bColorKey));
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Debug
    //
    //////////////////////////////////////////////////////////////////////////////

    void CheckVertices(const VertexScreen* pvertex, int vcount)
    {
        VertexScreen* pcheck = (VertexScreen*)pvertex;

        for (int index = 0; index < vcount; index++) {
            if (pcheck[index].x < m_rectClip.XMin()) {
                pcheck[index].x = m_rectClip.XMin();
            } else if (pcheck[index].x > m_rectClip.XMax()) {
                pcheck[index].x = m_rectClip.XMax();
            }

            if (pcheck[index].y < m_rectClip.YMin()) {
                pcheck[index].y = m_rectClip.YMin();
            } else if (pcheck[index].y > m_rectClip.YMax()) {
                pcheck[index].y = m_rectClip.YMax();
            }

            if (pcheck[index].z < 0) {
                pcheck[index].z = 0;
            } else if (pcheck[index].z > 1) {
                pcheck[index].z = 1;
            }
        }
    }

    void CheckIndices(const MeshIndex* pindex, int icount, int vcount)
    {
        #ifdef _DEBUG
            for (int index = 0; index < icount; index++) {
                ZAssert(pindex[index] >= 0 && pindex[index] < vcount);
            }
        #endif
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Rendering
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawTrianglesD3D(const D3DLVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
		_ASSERT( false );
		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_LVERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawIndexedPrimitiveUP(	
													D3DPT_TRIANGLELIST,
													0,
													vcount,
													icount / 3,
													pindex,
													D3DFMT_INDEX16,
													pvertex,
													sizeof( D3DLVertex ) ) );
    }

    void DrawLinesD3D(const D3DLVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
		_ASSERT( false );
 		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_LVERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawIndexedPrimitiveUP(	
													D3DPT_LINELIST,
													0,
													vcount,
													icount / 2,
													pindex,
													D3DFMT_INDEX16,
													pvertex,
													sizeof( D3DLVertex ) ) );
    }

    void DrawPointsD3D(const D3DLVertex* pvertex, int vcount)
    {
		_ASSERT( false );
 		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_LVERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawPrimitiveUP(	D3DPT_POINTLIST,
													vcount,
													pvertex,
													sizeof( D3DLVertex ) ) );
    }

    void DrawTrianglesD3D(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
  		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_VERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawIndexedPrimitiveUP(	D3DPT_TRIANGLELIST,
													0,
													vcount,
													icount / 3,
													pindex,
													D3DFMT_INDEX16,
													pvertex,
													sizeof( D3DVertex ) ) );
    }

    void DrawLinesD3D(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
 		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_VERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawIndexedPrimitiveUP(	D3DPT_LINELIST,
													0,
													vcount,
													icount / 2,
													pindex,
													D3DFMT_INDEX16,
													pvertex,
													sizeof( D3DVertex ) ) );
    }

    void DrawPointsD3D(const D3DVertex* pvertex, int vcount)
    {
 		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_VERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawPrimitiveUP(	D3DPT_POINTLIST,
											vcount,
											pvertex,
											sizeof( D3DVertex ) ) );
    }


    void DrawTriangles(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        CheckVertices(pvertex, vcount);
        CheckIndices(pindex, icount, vcount);

  		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_TLVERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawIndexedPrimitiveUP(	D3DPT_TRIANGLELIST,
													0,
													vcount,
													icount / 3,
													pindex,
													D3DFMT_INDEX16,
													pvertex,
													sizeof( VertexScreen ) ) );
    }

    void DrawLines(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        CheckVertices(pvertex, vcount);
        CheckIndices(pindex, icount, vcount);

 		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_TLVERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawIndexedPrimitiveUP(	D3DPT_LINELIST,
													0,
													vcount,
													icount / 2,
													pindex,
													D3DFMT_INDEX16,
													pvertex,
													sizeof( VertexScreen ) ) );
    }

	void DrawPoints(const VertexScreen* pvertex, int vcount)
	{
		CheckVertices(pvertex, vcount);
		D3DCall( CD3DDevice9::Get()->SetFVF( D3DFVF_TLVERTEX ) );
		D3DCall( CD3DDevice9::Get()->DrawPrimitiveUP(	D3DPT_POINTLIST,
											vcount,
											pvertex,
											sizeof( VertexScreen ) ) );
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateD3D9Rasterizer()
//
////////////////////////////////////////////////////////////////////////////////////////////////////
TRef<D3D9Rasterizer> CreateD3D9Rasterizer(PrivateSurface* psurface)
{
	TRef<D3D9Rasterizer> pRasterizer = new D3D9RasterizerImpl(psurface);
	return pRasterizer->IsValid() ? pRasterizer : NULL;
}
