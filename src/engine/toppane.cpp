#include "pch.h"

/////////////////////////////////////////////////////////////////////////////
//
// TopPane
//
/////////////////////////////////////////////////////////////////////////////

class TopPaneSurfaceSite : public SurfaceSite {
private:
    TopPane* m_ptopPane;

public:
    TopPaneSurfaceSite(TopPane* ptopPane) :
        m_ptopPane(ptopPane)
    {
    }

    void UpdateSurface(Surface* psurface)
    {
        m_ptopPane->RepaintSurface();
    }
};

/////////////////////////////////////////////////////////////////////////////
//
// TopPane
//
/////////////////////////////////////////////////////////////////////////////

TopPane::TopPane(Engine* pengine, bool bColorKey, TopPaneSite* psite, Pane* pchild) :
    Pane(pchild),
    m_pengine(pengine),
//    m_psurface(pengine->CreateSurface(WinPoint(1, 1), stype, new TopPaneSurfaceSite(this))),
    m_psurface(NULL),
    m_psite(psite),
    m_bColorKey(bColorKey),
    m_bNeedLayout(true)
{ //Fix memory leak -Imago 8/2/09
    SetSize(WinPoint(0, 0));
}

void TopPane::RepaintSurface()
{
    m_bNeedPaint = true;
    m_bPaintAll = true;
}

void TopPane::NeedLayout()
{
    if (!m_bNeedLayout) {
        m_bNeedLayout = true;
        m_psite->SizeChanged();
    }
}

void TopPane::NeedPaintInternal()
{
    if (!m_bNeedPaint) {
        m_bNeedPaint = true;
        m_psite->SurfaceChanged();
    }
}

void TopPane::Paint(Surface* psurface)
{
    // psurface->FillSurface(Color(0.8f, 0.5f, 1.0f));
}

void TopPane::Evaluate()
{
    if (m_bNeedLayout) {
        m_bNeedLayout = false;

        WinPoint sizeOld = GetSize();

        UpdateLayout();

		WinPoint sizeNew = GetSize();

		// This creates the top level surface. Create a new render target for now.
        if ( ( sizeNew != sizeOld ) && ( sizeNew != WinPoint(0,0) ) )
		{
            m_bNeedPaint = true;
			m_bPaintAll = true;

/*			if( sizeNew == CD3DDevice9::GetCurrentResolution() )
			{
				m_psurface = m_pengine->CreateDummySurface( 
										sizeNew, 
										new TopPaneSurfaceSite( this ) );
			}
			else*/
			{
				m_psurface = m_pengine->CreateRenderTargetSurface( 
										sizeNew,
										new TopPaneSurfaceSite( this ) );
			}

            if (m_bColorKey) 
			{
                m_psurface->SetColorKey(Color(0, 0, 0));
            }
        }
    }
}

void TopPane::UpdateLayout()
{
    DefaultUpdateLayout();
}

bool g_bPaintAll = false;
bool g_bUpdateOffset = false;

void TopPane::UpdateBits()
{
/* ORIGINAL ALLEGIANCE VERSION.
	ZEnter("TopPane::UpdateBits()");
    if (m_bNeedPaint) {
        ZTrace("m_bNeedPaint == true");
        if (CalcPaint()) {
            m_bNeedPaint = true;
            m_bPaintAll = true;
        }

        ZTrace("after CalcPaint() m_bNeedPaint ==" + ZString(m_bNeedPaint));
        ZTrace("after CalcPaint() m_bPaintAll  ==" + ZString(m_bPaintAll ));
        m_bPaintAll |= g_bPaintAll;
        InternalPaint(m_psurface);
        m_bNeedPaint = false;
    }
    ZExit("TopPane::UpdateBits()");*/


    ZEnter("TopPane::UpdateBits()");
	{
		HRESULT hr;
		bool bRenderTargetRequired;

        ZAssert(m_psurface != NULL);

		PrivateSurface* pprivateSurface; CastTo(pprivateSurface, m_psurface);
		bRenderTargetRequired = pprivateSurface->GetSurfaceType().Test(SurfaceTypeRenderTarget() ) == true;

		if( bRenderTargetRequired == true )
		{
			TEXHANDLE hTexture = pprivateSurface->GetTexHandle( );
			_ASSERT( hTexture != INVALID_TEX_HANDLE );
			hr = CVRAMManager::Get()->PushRenderTarget( hTexture );
		}

		ZTrace("m_bNeedPaint == true");
        CalcPaint(); 
        m_bNeedPaint = true;
        m_bPaintAll = true;

        ZTrace("after CalcPaint() m_bPaintAll  ==" + ZString(m_bPaintAll ));

		WinPoint offset( 0, 0 );

		// Call InternalPaint() with the child offset and parent size as params and create initial clipping rect.
		WinRect rectClip(	0, 
							0, 
							(int) m_psurface->GetSize().X(),
							(int) m_psurface->GetSize().Y() );

		m_bPaintAll |= g_bPaintAll;
        InternalPaint( m_psurface );
        m_bNeedPaint = false;

		if( bRenderTargetRequired == true )
		{
			CVRAMManager::Get()->PopRenderTarget( );
		}
    }
    ZExit("TopPane::UpdateBits()");

/*	{
        ZTrace("m_bNeedPaint == true");
		CalcPaint();
		m_bNeedPaint = true;
		m_bPaintAll = true;

        ZTrace("after CalcPaint() m_bNeedPaint ==" + ZString(m_bNeedPaint));
        ZTrace("after CalcPaint() m_bPaintAll  ==" + ZString(m_bPaintAll ));
        m_bPaintAll |= g_bPaintAll;

//		localOffset.SetY( localOffset.Y() - (int)m_psurface->GetSize().Y() );
//		localOffset += globalOffset;
		WinPoint offset( localOffset );

		// Remove offset now.
		offset.SetY( offset.Y() - (int)m_psurface->GetSize().Y() );

		// Call InternalPaint() with the child offset and parent size as params and create initial clipping rect.
		WinRect rectClip(	offset.X(), 
							offset.Y(), 
							offset.X() + (int) m_psurface->GetSize().X(),
							offset.Y() + (int) m_psurface->GetSize().Y() );
   
		// m_psurface is a dummy surface. Store the context.
		InternalPaint( m_psurface, offset, rectClip );
        m_bNeedPaint = false;
    }

    ZExit("TopPane::UpdateBits()");*/
}

const WinPoint& TopPane::GetSurfaceSize()
{
    Evaluate();
    return GetSize();
}

Surface* TopPane::GetSurface()
{
    Evaluate();

    //when the size is zero, the surface is not initialized
    if (m_size.X() == 0 || m_size.Y() == 0) {
        return NULL;
    }
    UpdateBits();
    return m_psurface;
}

Point TopPane::TransformLocalToImage(const WinPoint& point)
{
    return
        m_psite->TransformLocalToImage(
            GetPanePoint(
                Point::Cast(point)
            )
        );
}

Point TopPane::GetPanePoint(const Point& point)
{
    return
        Point(
            point.X(),
            (float)GetSize().Y() - 1.0f - point.Y()
        );
}

void TopPane::MouseEnter(IInputProvider* pprovider, const Point& point)
{ 
//	m_bInside = true;
/*	if( m_psurface->GetSurfaceType().Test( SurfaceTypeDummy() ) == false )
	{
		OutputDebugString("MouseEnter\n");
	}*/
}

MouseResult TopPane::HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
{
    return Pane::HitTest(pprovider, GetPanePoint(point), bCaptured);
}

MouseResult TopPane::Button(
    IInputProvider* pprovider, 
    const Point& point, 
    int button, 
    bool bCaptured, 
    bool bInside, 
    bool bDown
) {
    return Pane::Button(pprovider, GetPanePoint(point), button, bCaptured, bInside, bDown);
}
