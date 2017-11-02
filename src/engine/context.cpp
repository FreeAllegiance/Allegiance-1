#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Plane
//
//////////////////////////////////////////////////////////////////////////////

float Plane::Distance(const HVector& vec) const
{
    return
          vec.X() * m_hvec.X()
        + vec.Y() * m_hvec.Y()
        + vec.Z() * m_hvec.Z()
        + vec.W() * m_hvec.W();
}

float Plane::Intersect(const HVector& v0, const HVector& v1) const
{
    HVector  vd = v1 - v0;
    float     a = m_hvec * v0;
    float     b = m_hvec * vd;
    float alpha = -a / b;

    return bound(alpha, 0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////////
//
// State
//
//////////////////////////////////////////////////////////////////////////////

class StateChangeBase {};
typedef TBitMask<StateChangeBase, DWORD> StateChange;

class StateChangeMatrix                : public StateChange { public: StateChangeMatrix               () : StateChange(0x000001) {} };
class StateChangePerspectiveMatrix     : public StateChange { public: StateChangePerspectiveMatrix    () : StateChange(0x000002) {} };
class StateChangeViewMatrix            : public StateChange { public: StateChangeViewMatrix           () : StateChange(0x200000) {} };
class StateChangeMaterial              : public StateChange { public: StateChangeMaterial             () : StateChange(0x000004) {} };
class StateChangeTexture               : public StateChange { public: StateChangeTexture              () : StateChange(0x000008) {} };
class StateChangeShadeMode             : public StateChange { public: StateChangeShadeMode            () : StateChange(0x000010) {} };
class StateChangeBlendMode             : public StateChange { public: StateChangeBlendMode            () : StateChange(0x000020) {} };
class StateChangeZTest                 : public StateChange { public: StateChangeZTest                () : StateChange(0x000040) {} };
class StateChangeZWrite                : public StateChange { public: StateChangeZWrite               () : StateChange(0x000080) {} };
class StateChangeLinearFilter          : public StateChange { public: StateChangeLinearFilter         () : StateChange(0x000100) {} };
class StateChangePerspectiveCorrection : public StateChange { public: StateChangePerspectiveCorrection() : StateChange(0x000200) {} };
class StateChangeDither                : public StateChange { public: StateChangeDither               () : StateChange(0x000400) {} };
class StateChangeWrapMode              : public StateChange { public: StateChangeWrapMode             () : StateChange(0x000800) {} };
class StateChangeCullMode              : public StateChange { public: StateChangeCullMode             () : StateChange(0x001000) {} };
class StateChangeClipRect              : public StateChange { public: StateChangeClipRect             () : StateChange(0x002000) {} };
class StateChangeClipPlanes            : public StateChange { public: StateChangeClipPlanes           () : StateChange(0x004000) {} };
class StateChangeColor                 : public StateChange { public: StateChangeColor                () : StateChange(0x008000) {} };
class StateChangeAmbientLevel          : public StateChange { public: StateChangeAmbientLevel         () : StateChange(0x010000) {} };
class StateChangeLineWidth             : public StateChange { public: StateChangeLineWidth            () : StateChange(0x020000) {} };
class StateChangeColorKey              : public StateChange { public: StateChangeColorKey             () : StateChange(0x040000) {} };
class StateChangeDeformation           : public StateChange { public: StateChangeDeformation          () : StateChange(0x080000) {} };
class StateChangeAll                   : public StateChange { public: StateChangeAll                  () : StateChange(0xffffff) {} };

const StateChange stCommon =
      StateChangeMatrix()
    | StateChangeMaterial()
    | StateChangeTexture();

const StateChange stUncommon = StateChangeAll().Clear(stCommon);

class State : public IObjectSingle {
public:
    TRef<State>       m_pnext;
    TRef<Material>    m_pmaterial;
    TRef<Surface>     m_psurfaceTexture;
    TRef<Deformation> m_pdeform;

    Matrix            m_mat;
	Matrix            m_matWorldTM;
    Matrix            m_matPerspective;
	Matrix            m_matView;
    Rect              m_rectClip;
    int               m_countClipPlanes;
    bool              m_bZTest;
    bool              m_bZWrite;
    bool              m_bDither;
    bool              m_bColorKey;
    bool              m_bLinearFilter;
    bool              m_bPerspectiveCorrection;
    ShadeMode         m_shadeMode;
    BlendMode         m_blendMode;
    WrapMode          m_wrapMode;
    CullMode          m_cullMode;
    Color             m_color;
    float             m_ambientLevel;
    float             m_lineWidth;

    StateChange       m_maskChanges;
    StateChange       m_maskOverride;

    State()
    {
    }

    State(State* pstate) :
        m_pnext(pstate),
        m_maskChanges(),
        m_maskOverride()
    {
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Drawing Context
//
//////////////////////////////////////////////////////////////////////////////

class ContextImpl : public PrivateContext {
private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // types
    //
    //////////////////////////////////////////////////////////////////////////////

    class WhiteSurfaceSite : public SurfaceSite {
    public:
        void UpdateSurface(Surface* psurface) 
        {
            psurface->FillSurface(Color::White());
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    // members
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<IDevice3D>          m_pdevice3D;
    TRef<State>              m_pstate;
    TRef<State>              m_pstateDevice;
    PrivateSurface*          m_psurface;

    //
    // Vertex buffers
    //

    Vertex*                  m_pvertexBuffer;
    int                      m_countVertexBuffer;

    MeshIndex*               m_pindexBuffer;
    int                      m_countIndexBuffer;

    //
    // What mode is the device in?
    //

    bool                     m_bRendering;
    bool                     m_bIn3DLayer;
    bool                     m_bInScene;
    bool                     m_bRenderingCallbacks;

    //
    // Culling
    //

    float                    m_focus;
    float                    m_near;
    float                    m_far;
    float                    m_aspect;
    float                    m_rxscale;
    float                    m_ryscale;
    float                    m_yscale;

    //
    // Level of detail
    //

    float                    m_lod;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Decal Data
    //
    //////////////////////////////////////////////////////////////////////////////

    class Decal {
    public:
        Vector m_position;
        Color  m_color;
        float  m_scale;
        float  m_angle;
        Vector m_forward;
        Vector m_right;

        bool GetVertices(VertexL* pvertex) const
        {
            Vector v0;
            Vector v1;
            Vector v2;
            Vector v3;

            if (m_forward.IsZero()) {
                float c = cos(m_angle) * m_scale;
                float s = sin(m_angle) * m_scale;
                float dx = c - s;
                float dy = c + s;

                float x = m_position.X();
                float y = m_position.Y();
                float z = m_position.Z();

                v0 = Vector(x - dy, y + dx, z);
                v1 = Vector(x - dx, y - dy, z);
                v2 = Vector(x + dy, y - dx, z);
                v3 = Vector(x + dx, y + dy, z);
            } else {
                v0 = m_position - m_forward - m_right;
                v1 = m_position + m_forward - m_right;
                v2 = m_position + m_forward + m_right;
                v3 = m_position - m_forward + m_right;
            }

            float r = m_color.R();
            float g = m_color.G();
            float b = m_color.B();
            float a = m_color.A();

            pvertex[0] = VertexL(v0.X(), v0.Y(), v0.Z(), r, g, b, a, 0, 1);
            pvertex[1] = VertexL(v1.X(), v1.Y(), v1.Z(), r, g, b, a, 0, 0);
            pvertex[2] = VertexL(v2.X(), v2.Y(), v2.Z(), r, g, b, a, 1, 0);
            pvertex[3] = VertexL(v3.X(), v3.Y(), v3.Z(), r, g, b, a, 1, 1);

            return true;
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    // Decal Sets
    //
    //////////////////////////////////////////////////////////////////////////////

    class DecalSet {
    public:
        TRef<Surface>  m_psurface;
        TVector<Decal> m_vdecal;
    };

    TVector<DecalSet> m_vdecalSet;
    TVector<DecalSet> m_vdecalSetOpaque;

    //
    // Delayed 3D Rendering
    //

    class GeoCallbackData {
    public:
        TRef<IGeoCallback> m_pgeoCallback;
        Matrix             m_mat;
		Matrix			   m_matWorldTM;
        float              m_distance;
    };

    class CompareDistance {
    public:
        bool operator () (const GeoCallbackData& value1, const GeoCallbackData& value2)
        {
            return value1.m_distance < value2.m_distance;
        }
    };

    typedef TList<GeoCallbackData, DefaultNoEquals, CompareDistance> GeoCallbackDataList;

    GeoCallbackDataList m_listGeoCallbacks;
    GeoCallbackDataList m_listGeoCallbacksSorted;

    //
    // Counters
    //

    #ifdef EnablePerformanceCounters
        int m_countDrawString;
        int m_countDrawStringChars;
    #endif

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

	ContextImpl( PrivateSurface* psurface ) :
        #ifdef EnablePerformanceCounters
            m_countDrawString(0),
            m_countDrawStringChars(0),
        #endif
        m_pvertexBuffer(NULL),
        m_countVertexBuffer(0),
        m_pindexBuffer(NULL),
        m_countIndexBuffer(0),
        m_psurface( psurface ),
        m_bRendering(false),
        m_bIn3DLayer(false),
        m_bInScene(false),
        m_bRenderingCallbacks(false)
    {

		// Create a rasterizer and a 3D device
		TRef<Rasterizer> pRasterizer = CreateD3D9Rasterizer(psurface);
		if( pRasterizer == NULL )
		{
			return;
		}
		m_pdevice3D = CreateDevice3D( pRasterizer );

        // Setup the initial state
        m_pstateDevice = new State();

        m_pstateDevice->m_mat.SetIdentity();
		m_pstateDevice->m_matWorldTM.SetIdentity();
        m_pstateDevice->m_matPerspective.SetIdentity();
		m_pstateDevice->m_matView.SetIdentity();
        m_pstateDevice->m_pmaterial              = NULL;
        m_pstateDevice->m_bZTest                 = false;
        m_pstateDevice->m_bZWrite                = false;
        m_pstateDevice->m_bPerspectiveCorrection = false;
        m_pstateDevice->m_bDither                = false; //true;
        m_pstateDevice->m_bColorKey              = true;
        m_pstateDevice->m_bLinearFilter          = true;
        m_pstateDevice->m_shadeMode              = ShadeModeGouraud;
        m_pstateDevice->m_blendMode              = BlendModeSourceAlpha;
        m_pstateDevice->m_wrapMode               = WrapModeNone;
        m_pstateDevice->m_cullMode               = CullModeCCW;
        m_pstateDevice->m_maskChanges            = StateChangeAll();
        m_pstateDevice->m_maskOverride           = StateChange();
        m_pstateDevice->m_countClipPlanes        = 0;
        m_pstateDevice->m_rectClip               = 
            Rect(
                Point(0, 0),
                Point::Cast(m_psurface->GetSize())
            );
        m_pstateDevice->m_color                  = Color::White();
        m_pstateDevice->m_lineWidth              = 1;

		// Mirror default states with d3d9.
		CD3DDevice9::Get()->SetRenderState( D3DRS_ZENABLE, FALSE );
		CD3DDevice9::Get()->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		CD3DDevice9::Get()->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
		CD3DDevice9::Get()->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
		CD3DDevice9::Get()->SetRenderState( D3DRS_WRAP0, 0 );
		CD3DDevice9::Get()->SetRenderState( D3DRS_ALPHABLENDENABLE, false );
    }

    bool IsValid()
    {
        return m_pdevice3D != NULL && m_pdevice3D->IsValid();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Destructor
    //
    //////////////////////////////////////////////////////////////////////////////

    ~ContextImpl()
    {
        if (m_pvertexBuffer) {
            free(m_pvertexBuffer);
        }

        if (m_pindexBuffer) {
            free(m_pindexBuffer);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Capabilities
    //
    //////////////////////////////////////////////////////////////////////////////

    bool Has3DAcceleration()
    {
        return m_pdevice3D->Has3DAcceleration();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Rendering Initialization
    //
    //////////////////////////////////////////////////////////////////////////////

    void BeginRendering()
    {
        ZAssert(!m_bRendering);
        ZAssert(!m_bInScene);
        ZAssert(!m_bIn3DLayer);

        m_bRendering = true;

        PushState();
    }

    void EndRendering()
    {
        ZAssert(m_bRendering);

        PopState();

        if (m_bIn3DLayer) {
            End3DLayer();
        }
        EndScene();

        m_bRendering = false;
    }

    PrivateSurface* GetSurface()
    {
        return m_psurface;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Attributes
    //
    //////////////////////////////////////////////////////////////////////////////

    bool GetClipping()
    {
        return m_pdevice3D->GetClipping();
    }

    void SetClipping(bool bClip)
    {
        m_pdevice3D->SetClipping(bClip);
    }

    bool IsCulled(
        const Vector& vecOrigin,
              float   radius,
              bool&   bNotClipped
    ) {
        //  , this code assumes there are no non-uniform scales
        //     actualy it doesn't handle scales at all since radius isn't
        //     scaled.

        Vector vecEye = TransformLocalToEye(vecOrigin);

        float x      =  vecEye.X();
        float y      =  vecEye.Y();
        float z      =  vecEye.Z();
        float back   =  z - m_far;
        float front  =  m_near - z;
        float rx;
        float ry;
        float left;
        float right;
        float bottom;
        float top;

        if (m_focus == 0) {
            rx     = radius;
            ry     = radius;
            left   =  x - 1;
            right  = -x - 1;
            bottom =  y - 1;
            top    = -y - 1;
        } else {
            rx     = radius * m_rxscale;
            ry     = radius * m_ryscale;
            left   =  m_focus * x - z;
            right  = -m_focus * x - z;
            bottom =  m_yscale * y - z;
            top    = -m_yscale * y - z;
        }

        //
        // Is the sphere completely outside the frustum?
        //

        if (
               left   < -rx
            || right  < -rx
            || bottom < -ry
            || top    < -ry
            || back   < -radius
            || front  < -radius
        ) {
            bNotClipped = false;
            return true;
        }

        //
        // Is the sphere completely inside the frustum?
        //

        if (
               left   > rx
            && right  > rx
            && bottom > ry
            && top    > ry
            && back   > radius
            && front  > radius
        ) {
            bNotClipped = true;
            return false;
        }

        //
        // The sphere intersects the frustum's boundary.
        //

        bNotClipped = false;
        return false;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 2D Image Rendering
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawImage(
        Surface* psurface,
        const Rect& rectSource,
        bool bCentered,
        const Point& point
    ) {
        // !!! should really look at the device mode and the current transform
        // !!! this uses upper left as (0, 0) on psurface and DrawImage3D() uses lower left as (0, 0)
        // !!! this doesn't scale or rotate the image

        ZAssert(!m_bIn3DLayer);
        ZAssert(m_pstateDevice->m_matPerspective.GetType() < TransformRotate);
		PrivateSurface * pprivateSurfSource;
		CastTo( pprivateSurfSource, psurface );

		if( pprivateSurfSource->GetSurfaceType().Test(SurfaceTypeDummy() ) == true )
		{
			return;
		}

        DD2D();

        Point pointImage;
        if (TransformLocalToImage(Vector(point.X(), point.Y(), 0), pointImage)) {
            WinPoint pointScreen = TransformImageToSurface(pointImage);

            if (bCentered) {
                pointScreen.SetX(pointScreen.X() - (int)(rectSource.XSize() / 2.0f));
                pointScreen.SetY(pointScreen.Y() - (int)(rectSource.YSize() / 2.0f));
            } else {
                pointScreen.SetY(pointScreen.Y() - (int)rectSource.YSize());
            }

            m_psurface->BitBlt(
                pointScreen, 
                psurface, 
                WinRect::Cast(rectSource)
            );
        }
    }

    void DrawImage(
        Surface* psurface,
        bool bCentered,
        const Point& point
    ) {
		if( psurface != NULL )
		{
/*			if ( m_psurface->GetSize() == WinPoint( 800,500 ) )
			{
				DrawImage(
					psurface, 
    				WinRect(
        				WinPoint(0, 0),
        				WinPoint( 700, 400 )
        			), 
        			bCentered, 
        			point
        		);
			}
			else*/
			{
				DrawImage(
					psurface, 
    				WinRect(
        				WinPoint(0, 0),
        				psurface->GetSize()
        			), 
        			bCentered, 
        			point
        		);
			}
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 3D Image Rendering
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawImage3D(
              Surface* psurface,
        const Rect&    rectSource,
        const Color&   color,
              bool     bCentered,
        const Point&   point
    ) {
        ZAssert(m_bRendering);

        if (!m_bInScene) {
            Texture2D();
        }

        Point sizeSource(Point::Cast(psurface->GetSize()));

        float xOffset = point.X() - (bCentered ? rectSource.XSize() / 2 : 0);
        float yOffset = point.Y() - (bCentered ? rectSource.YSize() / 2 : 0);

        float xmin = xOffset;
        float xmax = xOffset + rectSource.XSize();
        float ymin = yOffset;
        float ymax = yOffset + rectSource.YSize();

/*		xmin *= m_pstateDevice->m_matPerspective[0][0];
		xmax *= m_pstateDevice->m_matPerspective[0][0];
		ymin *= m_pstateDevice->m_matPerspective[1][1];
		ymax *= m_pstateDevice->m_matPerspective[1][1];

		xmin += m_pstateDevice->m_matPerspective[0][3];
		xmax += m_pstateDevice->m_matPerspective[0][3];
		ymin += m_pstateDevice->m_matPerspective[1][3];
		ymax += m_pstateDevice->m_matPerspective[1][3];

		ymin = m_psurface->GetSize().Y() - ymin;
		ymax = m_psurface->GetSize().Y() - ymax;*/

        float xt;
        float yt;

        //if (psurface->GetSurfaceType().Test(SurfaceTypeTile())) {
            xt = 1.0f / sizeSource.X();
            yt = 1.0f / sizeSource.Y();
        //} else {
        //    //  , this is broken if the texture size is smaller that the surface size
        //    Point pointTextureSize(Point::Cast(psurface->GetTextureRect().Size()));
        //
        //    xt = 1.0f / pointTextureSize.X();
        //    yt = 1.0f / pointTextureSize.Y();
        //}

        float xtmin = xt * rectSource.XMin();
        float xtmax = xt * rectSource.XMax();

        float ytmin = yt * (sizeSource.Y() - rectSource.YMin());
        float ytmax = yt * (sizeSource.Y() - rectSource.YMax());

		// Grab the handle of the textured vertex dynamic VB.
		CVBIBManager::SVBIBHandle * phVB;

        static MeshIndex indices[6] = { 0, 2, 1, 0, 3, 2 };

        UpdateState();

		CD3DDevice9 * pDev = CD3DDevice9::Get();
		CVRAMManager::Get()->SetTexture( psurface->GetTexHandle(), 0 );

		switch (GetBlendMode())
		{
			case BlendModeSource:
				pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
				pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
				break;
			case BlendModeAdd:
				pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				break;
			case BlendModeSourceAlpha:
				pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				break;
			default:
				ZError("Invalid blend mode");
		}

		switch (GetShadeMode()) 
		{
            case ShadeModeCopy:
                {
                    Vertex vertices[4] = {
                        Vertex(xmin, ymax, 0, 0, 0, 1, xtmin, ytmax),
                        Vertex(xmin, ymin, 0, 0, 0, 1, xtmin, ytmin),
                        Vertex(xmax, ymin, 0, 0, 0, 1, xtmax, ytmin),
                        Vertex(xmax, ymax, 0, 0, 0, 1, xtmax, ytmax)
                    };
//                    m_pdevice3D->DrawTriangles(vertices, 4, indices, 6);
					_ASSERT( false && "TBD" );
                }
                break;

            case ShadeModeFlat:
            case ShadeModeGouraud:
                {
					UIFONTVERTEX * pVertexData;

					// New dynamic VB path.
					phVB = CVertexGenerator::Get()->GetPredefinedDynamicBuffer( CVertexGenerator::ePDBT_UIFontVB );

					if( CVBIBManager::Get()->LockDynamicVertexBuffer( phVB, 4, (void**) &pVertexData ) == false )
					{
						// Failed to lock the vertex buffer.
						_ASSERT( false );
						return;
					}

					Matrix & matRef = m_pstateDevice->m_matPerspective;
					yOffset = (float) m_psurface->GetSize().Y();

					// Transform the vertex positions by the values in m_pstateDevice->m_matPerspective.
					// The y-value is inverted relative to the target surface.
					pVertexData[0].x = ( xmin * matRef[0][0] ) + ( ymax * matRef[0][1] ) + matRef[0][3];
					pVertexData[0].y = yOffset - (	( xmin * matRef[1][0] ) + 
													( ymax * matRef[1][1] ) + 
													matRef[1][3] );
					pVertexData[0].z = 0.0f;
					pVertexData[0].rhw = 1.0f;
					pVertexData[0].color = D3DRGBA( color.R(), color.G(), color.B(), color.A() );
					pVertexData[0].fU = xtmin;
					pVertexData[0].fV = ytmax;

					pVertexData[1].x = ( xmax * matRef[0][0] ) + ( ymax * matRef[0][1] ) + matRef[0][3];
					pVertexData[1].y = yOffset - (	( xmax * matRef[1][0] ) + 
													( ymax * matRef[1][1] ) + 
													matRef[1][3] );
					pVertexData[1].z = 0.0f;
					pVertexData[1].rhw = 1.0f;
					pVertexData[1].color = D3DRGBA( color.R(), color.G(), color.B(), color.A() );
					pVertexData[1].fU = xtmax;
					pVertexData[1].fV = ytmax;

					pVertexData[2].x = ( xmin * matRef[0][0] ) + ( ymin * matRef[0][1] ) + matRef[0][3];
					pVertexData[2].y = yOffset - (	( xmin * matRef[1][0] ) + 
													( ymin * matRef[1][1] ) + 
													matRef[1][3] );
					pVertexData[2].z = 0.0f;
					pVertexData[2].rhw = 1.0f;
					pVertexData[2].color = D3DRGBA( color.R(), color.G(), color.B(), color.A() );
					pVertexData[2].fU = xtmin;
					pVertexData[2].fV = ytmin;

					pVertexData[3].x = ( xmax * matRef[0][0] ) + ( ymin * matRef[0][1] ) + matRef[0][3];
					pVertexData[3].y = yOffset - (	( xmax * matRef[1][0] ) + 
													( ymin * matRef[1][1] ) + 
													matRef[1][3] );
					pVertexData[3].z = 0.0f;
					pVertexData[3].rhw = 1.0f;
					pVertexData[3].color = D3DRGBA( color.R(), color.G(), color.B(), color.A() );
					pVertexData[3].fU = xtmax;
					pVertexData[3].fV = ytmin;

					// Adjust x and y coordinates for correct texture lookup.
					pVertexData[0].x -= 0.5f;
					pVertexData[0].y -= 0.5f;
					pVertexData[1].x -= 0.5f;
					pVertexData[1].y -= 0.5f;
					pVertexData[2].x -= 0.5f;
					pVertexData[2].y -= 0.5f;
					pVertexData[3].x -= 0.5f;
					pVertexData[3].y -= 0.5f;

					// Finished, unlock the buffer, set the stream.
					CVBIBManager::Get()->UnlockDynamicVertexBuffer( phVB );

					pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, GetLinearFilter() ? D3DTEXF_LINEAR : D3DTEXF_POINT);
					pDev->SetSamplerState(0, D3DSAMP_MINFILTER, GetLinearFilter() ? D3DTEXF_LINEAR : D3DTEXF_POINT);
					pDev->SetSamplerState(0, D3DSAMP_MIPFILTER, GetLinearFilter() ? D3DTEXF_LINEAR : D3DTEXF_POINT);

					pDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
                    m_pdevice3D->SetShadeMode(ShadeModeFlat);
					m_pdevice3D->DrawTriangles( D3DPT_TRIANGLESTRIP, 2, phVB );
                    m_pdevice3D->SetShadeMode(m_pstateDevice->m_shadeMode);
				}
                break;

            default:
                ZError("Invalid shade mode");
                break;
        }

        m_pdevice3D->SetTexture(m_pstateDevice->m_psurfaceTexture);
    }

    void DrawImage3D(
        Surface* psurface,
        const Color& color,
        bool bCentered,
        const Point& point
    ) {
        DrawImage3D(
            psurface, 
            Rect(
                Point(0, 0),
                Point::Cast(psurface->GetSize())
            ),
            color,
            bCentered, 
            point
        );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Text Rendering
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawString(	IEngineFont*   pfont,
						const Color&   color,
						const Point&   point,
						const ZString& str ) 
	{
/*        #ifdef EnablePerformanceCounters
            m_countDrawString++;
            m_countDrawStringChars += str.GetLength();
        #endif

        DD2D();

        Point pointImage;
        if (TransformLocalToImage(Vector(point.X(), point.Y(), 0), pointImage)) 
		{
            WinPoint pointScreen = TransformImageToSurface(pointImage);
//        	WinPoint pointScreen( pointImage.X(), pointImage.Y() );
//			WinPoint pointScreen( point.X(), point.Y() );

		    WinPoint size = pfont->GetTextExtent(str);

			// 03.12.05
			pointScreen.SetY(pointScreen.Y() - size.Y());
			Point pointOffset;
			WinRect rectClip(	m_UIOffset.X(), m_UIOffset.Y(),
								m_UIOffset.X() + m_UISize.X(),
								m_UIOffset.Y() + m_UISize.Y() );
			
			WinRect currRectClip; 
			GetCurrClipRect( &currRectClip );
//			pfont->DrawString( NULL, pointScreen + WinPoint( currRectClip.XMin(), currRectClip.YMin() ), rectClip, str, color );
//			pfont->DrawString( NULL, pointScreen + WinPoint( currRectClip.XMin(), currRectClip.YMin() ), rectClip, str, color );
//			pfont->DrawString( NULL, pointScreen + m_UIOffset, rectClip, str, color );
			pfont->DrawString( NULL, pointScreen + WinPoint( currRectClip.XMin(), currRectClip.YMin() ), rectClip, str, color );
        }*/

/*       if (TransformLocalToImage(Vector(point.X(), point.Y(), 0), pointImage)) 
	   {
			WinPoint pointScreen = TransformImageToSurface(pointImage);
			WinPoint size        = pfont->GetTextExtent(str);

			pointScreen.SetY(pointScreen.Y() - size.Y());
			WinRect rectClip(	pointScreen.X(), pointScreen.Y(),
								pointScreen.X() + m_UISize.X(),
								pointScreen.Y() + m_UISize.Y() );

			//			m_psurface->DrawString(pfont, color, pointScreen, str);
			WinRect currRectClip; 
			GetCurrClipRect( &currRectClip );
			pfont->DrawString( NULL, pointScreen, rectClip, str, color );
        }*/

		if( str.GetLength() == 0 )
		{
			return;
		}

       #ifdef EnablePerformanceCounters
            m_countDrawString++;
            m_countDrawStringChars += str.GetLength();
        #endif

        DD2D();

        Point pointImage;
        if (TransformLocalToImage(Vector(point.X(), point.Y(), 0), pointImage)) 
		{
            WinPoint pointScreen = TransformImageToSurface(pointImage);
            WinPoint size        = pfont->GetTextExtent(str);

            pointScreen.SetY( pointScreen.Y() - size.Y() );
//			pointScreen += m_UIOffset;

            //m_psurface->DrawString(pfont, color, pointScreen, str);
			pfont->DrawString(pointScreen, str, color );
        }
    }

    void DrawRectangle(const Rect& rect, const Color& color)
    {
        Texture2D();

        static MeshIndex indices[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };

        float xmin = rect.XMin();
        float xmax = rect.XMax();
        float ymin = rect.YMin();
        float ymax = rect.YMax();
        float r    = color.R();
        float g    = color.G();
        float b    = color.B();
        float a    = color.A();

        VertexL vertices[4] = {
            VertexL(xmin, ymax, 0, r, g, b, a, 0, 0),
            VertexL(xmin, ymin, 0, r, g, b, a, 0, 0),
            VertexL(xmax, ymin, 0, r, g, b, a, 0, 0),
            VertexL(xmax, ymax, 0, r, g, b, a, 0, 0)
        };

        m_pdevice3D->DrawLines(vertices, 4, indices, 8);
    }

    void FillRect(const Rect& rect, const Color& color)
    {
        Texture2D();

        static MeshIndex indices[6] = { 0, 2, 1, 0, 3, 2 };

        float xmin = rect.XMin();
        float xmax = rect.XMax();
        float ymin = rect.YMin();
        float ymax = rect.YMax();
        float r    = color.R();
        float g    = color.G();
        float b    = color.B();
        float a    = color.A();

        VertexL vertices[4] = {
            VertexL(xmin, ymax, 0, r, g, b, a, 0, 1),
            VertexL(xmin, ymin, 0, r, g, b, a, 0, 0),
            VertexL(xmax, ymin, 0, r, g, b, a, 1, 0),
            VertexL(xmax, ymax, 0, r, g, b, a, 1, 1)
        };

        UpdateState();

		CD3DDevice9 * pDev = CD3DDevice9::Get();
		switch (GetBlendMode())
		{
		case BlendModeSource:
			pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
			pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
			break;
		case BlendModeAdd:
			pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		case BlendModeSourceAlpha:
			pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		default:
			ZError("Invalid blend mode");
		}

        m_pdevice3D->SetShadeMode(ShadeModeFlat);
		m_pdevice3D->SetColorKey(false); // KGJV 32B
        m_pdevice3D->DrawTriangles(vertices, 4, indices, 6);
        m_pdevice3D->SetShadeMode(m_pstateDevice->m_shadeMode);
		m_pdevice3D->SetColorKey(m_pstateDevice->m_bColorKey); // KGJV 32B

    }

    void FillInfinite(const Color& color)
    {
        DD2D();
        m_psurface->FillSurface(color);        
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 3D Layers
    //
    //////////////////////////////////////////////////////////////////////////////

    void Begin3DLayer(Camera* pcamera, bool bZBuffer)
    {
        ZEnter("Context::Begin3DLayer");

        ZAssert(m_bRendering);
        ZAssert(!m_bIn3DLayer);

        //
        // Push the state so we can undo all the state changes
        //

        PushState();

		// Configure texture settings.
		CD3DDevice9 * pDev = CD3DDevice9::Get();
		CVRAMManager::Get()->SetTexture( INVALID_TEX_HANDLE, 0 );
		pDev->SetSamplerState( 0, D3DSAMP_MAGFILTER, pDev->GetMagFilter() );
		pDev->SetSamplerState( 0, D3DSAMP_MINFILTER, pDev->GetMinFilter() );
		pDev->SetSamplerState( 0, D3DSAMP_MIPFILTER, pDev->GetMipFilter() );
		if( pDev->IsAntiAliased() == true )
		{
			// Use this to view the difference between aa and non-aa.
			// Switches between the two every few frames.
/*			static int iCounter = 0;
			if( iCounter < 100 )
			{
				CD3DDevice9::SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
			}
			else
			{
				CD3DDevice9::SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE );
			}
			iCounter ++;
			if( iCounter >= 200 )
			{
				iCounter = 0;
			}*/

			// Switch AA on for the 3d layer.
			pDev->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
		}

		// Wacky scales in the world TM lead to wacky scales and normals.
		pDev->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE );

        //
        // Integrate the 2D transforms up until this point with the 3D transform
        //

        pcamera->Update();

		// Store off a copy of the perspective matrix before the software clipping
		// settings have been applied.
        SetPerspectiveMatrix(pcamera->GetPerspectiveMatrix());

        //
        // Change this flag after we change the prespective matrix
        //

        m_bIn3DLayer = true;

        //
        // Apply the camera's transform
        //

		// D3D9 update. Separate out the view and world transform matrices.
        SetViewMatrix(pcamera->GetModelMatrix());
		SetTransform(pcamera->GetModelMatrix(), Matrix::GetIdentity());

        //
        // start the 3D Layer
        //

        BeginScene();

        //
        // Save some information needed for culling
        //

        WinPoint point = m_psurface->GetSize();

        m_focus   = pcamera->GetFocus();
        m_near    = pcamera->GetNear();
        m_far     = pcamera->GetFar();
        m_aspect  = float(point.Y()) / float(point.X());
        m_yscale  = m_focus / m_aspect;
        m_rxscale = sqrt(m_focus * m_focus + 1.0f);
        m_ryscale = sqrt(m_yscale * m_yscale + 1.0f);

        //
        // Default to being frustum clipped
        //

        SetClipping(true);

        //
        // Initialize state
        //

        m_lod = FLT_MAX;

        //
        // Clear the ZBuffer
        //

        if (bZBuffer) {
            ZTrace("Clearing ZBuffer");
            UpdateState();
            m_pdevice3D->ClearZBuffer();

            SetZWrite(true, false);
            SetZTest(true, false);
        } else {
            //
            // turn off zbuffering for any future drawing
            //

            SetZWrite(false, false);
            SetZTest(false, false);
        }

        PushState();
    }

    void RenderCallbackGeos(GeoCallbackDataList& list)
    {
        GeoCallbackDataList::Iterator iter(list);

        while (!iter.End()) {
            GeoCallbackData& data = iter.Value();

            PushState();
			SetTransform(data.m_mat, data.m_matWorldTM);
            data.m_pgeoCallback->RenderCallback(this);
            PopState();

            iter.Next();
        }

        list.SetEmpty();
    }

    void End3DLayer()
    {
        ZExit("Context::Begin3DLayer");

        ZAssert(m_bRendering);
        ZAssert(m_bIn3DLayer);
        ZAssert(m_bInScene);

		//w0dka removed 7/8/09
		//CD3DDevice9::Get()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		//CD3DDevice9::Get()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );

		if( CD3DDevice9::Get()->IsAntiAliased() == true )
		{
			CD3DDevice9::Get()->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, FALSE );
		}

        //
        // Restore the original state
        //

        PopState();

		//
        // Render any GeoCallbacks
        //

        m_bRenderingCallbacks = true;

        RenderCallbackGeos(m_listGeoCallbacksSorted);
        RenderCallbackGeos(m_listGeoCallbacks);

        m_bRenderingCallbacks = false;

        //
        // Draw decals
        //

        DrawCachedDecals();

        //
        // End the 3D layer
        //

        PopState();

        m_bIn3DLayer = false;
    }

    void BeginScene()
    {
        if (!m_bInScene) {
            m_pdevice3D->BeginScene();
            m_bInScene = true;
        }
    }

    void EndScene()
    {
        if (m_bInScene) {
            m_pdevice3D->EndScene();
            m_bInScene = false;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Level of detail
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetLOD(float lod)
    {
        m_lod = lod;
    }

    float GetLOD()
    {
        return m_lod;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Get State
    //
    //////////////////////////////////////////////////////////////////////////////

    const Color&         GetGlobalColor()           const { return m_pstateDevice->m_color                 ; }
    bool                 GetZTest()                 const { return m_pstateDevice->m_bZTest                ; }
    bool                 GetZWrite()                const { return m_pstateDevice->m_bZWrite               ; }
    bool                 GetPerspectiveCorrection() const { return m_pstateDevice->m_bPerspectiveCorrection; }
    bool                 GetDither()                const { return m_pstateDevice->m_bDither               ; }
    bool                 GetColorKey()              const { return m_pstateDevice->m_bColorKey             ; }
    bool                 GetLinearFilter()          const { return m_pstateDevice->m_bLinearFilter         ; }
    ShadeMode            GetShadeMode()             const { return m_pstateDevice->m_shadeMode             ; }
    BlendMode            GetBlendMode()             const { return m_pstateDevice->m_blendMode             ; }
    WrapMode             GetWrapMode()              const { return m_pstateDevice->m_wrapMode              ; }
    CullMode             GetCullMode()              const { return m_pstateDevice->m_cullMode              ; }
    Surface*             GetTexture()               const { return m_pstateDevice->m_psurfaceTexture       ; }
    Material*            GetMaterial()              const { return m_pstateDevice->m_pmaterial             ; }
    Deformation*         GetDeformation()           const { return m_pstateDevice->m_pdeform               ; }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Efficient State Changes
    //
    //////////////////////////////////////////////////////////////////////////////

    template<class ValueType>
    void DoSetState(
        const ValueType& value,
        ValueType& valueDevice,
        ValueType& valueState,
        bool bOverride,
        const StateChange& st
    ) {
        if (!m_pstateDevice->m_maskOverride.Test(st)) {
            if (value != valueDevice) {
                if (!m_pstate->m_maskChanges.Test(st)) {
                    valueState = valueDevice;
                    m_pstate->m_maskChanges.Set(st);
                }

                valueDevice = value;
                m_pstateDevice->m_maskChanges.Set(st);
            }

            if (bOverride) {
                m_pstate->m_maskOverride.Set(st);
                m_pstateDevice->m_maskOverride.Set(st);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Efficient State Changes
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetGlobalColor(const Color& color, bool bOverride)
    {
        DoSetState(
            color,
            m_pstateDevice->m_color,
            m_pstate->m_color,
            bOverride,
            StateChangeColor()
        );
    }

    void SetLinearFilter(bool b, bool bOverride)
    {
        DoSetState(
            b,
            m_pstateDevice->m_bLinearFilter,
            m_pstate->m_bLinearFilter,
            bOverride,
            StateChangeLinearFilter()
        );
    }

    void SetPerspectiveCorrection(bool b, bool bOverride)
    {
        DoSetState(
            b,
            m_pstateDevice->m_bPerspectiveCorrection,
            m_pstate->m_bPerspectiveCorrection,
            bOverride,
            StateChangePerspectiveCorrection()
        );
    }

    void SetDither(bool b, bool bOverride)
    {
        DoSetState(
            b,
            m_pstateDevice->m_bDither,
            m_pstate->m_bDither,
            bOverride,
            StateChangeDither()
        );
    }

    void SetColorKey(bool b, bool bOverride)
    {
        DoSetState(
            b,
            m_pstateDevice->m_bColorKey,
            m_pstate->m_bColorKey,
            bOverride,
            StateChangeColorKey()
        );
    }

    void SetZTest(bool b, bool bOverride)
    {
        DoSetState(
            b,
            m_pstateDevice->m_bZTest,
            m_pstate->m_bZTest,
            bOverride,
            StateChangeZTest()
        );
    }

    void SetZWrite(bool b, bool bOverride)
    {
        DoSetState(
            b,
            m_pstateDevice->m_bZWrite,
            m_pstate->m_bZWrite,
            bOverride,
            StateChangeZWrite()
        );
    }

    void SetShadeMode(ShadeMode shadeMode, bool bOverride)
    {
        DoSetState(
            shadeMode,
            m_pstateDevice->m_shadeMode,
            m_pstate->m_shadeMode,
            bOverride,
            StateChangeShadeMode()
        );
    }

    void SetBlendMode(BlendMode blendMode, bool bOverride)
    {
        DoSetState(
            blendMode,
            m_pstateDevice->m_blendMode,
            m_pstate->m_blendMode,
            bOverride,
            StateChangeBlendMode()
        );
    }

    void SetWrapMode(WrapMode wrapMode, bool bOverride)
    {
        DoSetState(
            wrapMode,
            m_pstateDevice->m_wrapMode,
            m_pstate->m_wrapMode,
            bOverride,
            StateChangeWrapMode()
        );
    }

    void SetCullMode(CullMode cullMode, bool bOverride)
    {
        DoSetState(
            cullMode,
            m_pstateDevice->m_cullMode,
            m_pstate->m_cullMode,
            bOverride,
            StateChangeCullMode()
        );
    }

    void SetMaterial(Material* pmaterial, bool bOverride)
    {
        DoSetState(
            TRef<Material>(pmaterial),
            m_pstateDevice->m_pmaterial,
            m_pstate->m_pmaterial,
            bOverride,
            StateChangeMaterial()
        );
    }

    void SetTexture(Surface* psurface, bool bOverride)
    {
        DoSetState(
            TRef<Surface>(psurface),
            m_pstateDevice->m_psurfaceTexture,
            m_pstate->m_psurfaceTexture,
            bOverride,
            StateChangeTexture()
        );
    }

    void SetDeformation(Deformation* pdeform, bool bOverride)
    {
        DoSetState(
            TRef<Deformation>(pdeform),
            m_pstateDevice->m_pdeform,
            m_pstate->m_pdeform,
            bOverride,
            StateChangeDeformation()
        );
    }

    void SetLineWidth(float width, bool bOverride = false)
    {
        DoSetState(
            width,
            m_pstateDevice->m_lineWidth,
            m_pstate->m_lineWidth,
            bOverride,
            StateChangeLineWidth()
        );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Update the device states
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateState()
    {
        const StateChange& maskChanges = m_pstateDevice->m_maskChanges;

        if (maskChanges.Test(stCommon)) {
            if (maskChanges.Test(StateChangeMatrix())) {
				m_pdevice3D->SetMatrix(m_pstateDevice->m_mat, m_pstateDevice->m_matWorldTM);
            }

            if (maskChanges.Test(StateChangeMaterial())) {
                m_pdevice3D->SetMaterial(m_pstateDevice->m_pmaterial);
            }

            if (maskChanges.Test(StateChangeTexture())) {
                m_pdevice3D->SetTexture(m_pstateDevice->m_psurfaceTexture);
            }
        }

        if (maskChanges.Test(stUncommon)) {
            if (maskChanges.Test(StateChangeDeformation())) {
                m_pdevice3D->SetDeformation(m_pstateDevice->m_pdeform);
            }

            if (maskChanges.Test(StateChangePerspectiveMatrix())) {
                m_pdevice3D->SetPerspectiveMatrix(m_pstateDevice->m_matPerspective);
			}

			if( maskChanges.Test(StateChangeViewMatrix())) {
				m_pdevice3D->SetViewMatrix(m_pstateDevice->m_matView);
            }

            if (maskChanges.Test(StateChangeColor())) {
                m_pdevice3D->SetGlobalColor(m_pstateDevice->m_color);
            }

            if (maskChanges.Test(StateChangeClipRect())) {
                m_pdevice3D->SetClipRect(m_pstateDevice->m_rectClip);
            }

            if (maskChanges.Test(StateChangeShadeMode())) {
                m_pdevice3D->SetShadeMode(m_pstateDevice->m_shadeMode);
            }

            if (maskChanges.Test(StateChangeBlendMode())) {
                m_pdevice3D->SetBlendMode(m_pstateDevice->m_blendMode);
            }

            if (maskChanges.Test(StateChangeWrapMode())) {
                m_pdevice3D->SetWrapMode(m_pstateDevice->m_wrapMode);
            }

            if (maskChanges.Test(StateChangeCullMode())) {
                m_pdevice3D->SetCullMode(m_pstateDevice->m_cullMode);
            }

            if (maskChanges.Test(StateChangeZTest())) {
                m_pdevice3D->SetZTest(m_pstateDevice->m_bZTest);
            }

            if (maskChanges.Test(StateChangeZWrite())) {
                m_pdevice3D->SetZWrite(m_pstateDevice->m_bZWrite);
            }

            if (maskChanges.Test(StateChangeLinearFilter())) {
                m_pdevice3D->SetLinearFilter(m_pstateDevice->m_bLinearFilter);
            }

            if (maskChanges.Test(StateChangePerspectiveCorrection())) {
                m_pdevice3D->SetPerspectiveCorrection(m_pstateDevice->m_bPerspectiveCorrection);
            }

            if (maskChanges.Test(StateChangeDither())) {
                m_pdevice3D->SetDither(m_pstateDevice->m_bDither);
            }

            if (maskChanges.Test(StateChangeColorKey())) {
                m_pdevice3D->SetColorKey(m_pstateDevice->m_bColorKey);
            }

            if (maskChanges.Test(StateChangeLineWidth())) {
                m_pdevice3D->SetLineWidth(m_pstateDevice->m_lineWidth);
            }
        }

        m_pstateDevice->m_maskChanges.Clear(StateChangeAll());
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Push and Pop
    //
    //////////////////////////////////////////////////////////////////////////////

    void PushState()
    {
        m_pstate = new State(m_pstate);
    }

    void PopState()
    {
        const StateChange& maskChanges = m_pstate->m_maskChanges;

        if (maskChanges.Test(stCommon)) {
            if (maskChanges.Test(StateChangeMatrix())) {
                m_pstateDevice->m_mat = m_pstate->m_mat;
				m_pstateDevice->m_matWorldTM = m_pstate->m_matWorldTM;
            }

            if (maskChanges.Test(StateChangeMaterial())) {
                m_pstateDevice->m_pmaterial = m_pstate->m_pmaterial;
            }

            if (maskChanges.Test(StateChangeTexture())) {
                m_pstateDevice->m_psurfaceTexture = m_pstate->m_psurfaceTexture;
            }
        }

        if (maskChanges.Test(stUncommon)) {
            if (maskChanges.Test(StateChangeDeformation())) {
                m_pstateDevice->m_pdeform = m_pstate->m_pdeform;
            }

            if (maskChanges.Test(StateChangeClipPlanes())) {
                while (m_pstateDevice->m_countClipPlanes > m_pstate->m_countClipPlanes) {
                    m_pdevice3D->RemoveClipPlane(0);
                    m_pstateDevice->m_countClipPlanes--;
                }
            }

            if (maskChanges.Test(StateChangeColor())) {
                m_pstateDevice->m_color = m_pstate->m_color;
            }

            if (maskChanges.Test(StateChangeClipRect())) {
                m_pstateDevice->m_rectClip = m_pstate->m_rectClip;
            }

            if (maskChanges.Test(StateChangePerspectiveMatrix())) {
                m_pstateDevice->m_matPerspective = m_pstate->m_matPerspective;
			}

			if (maskChanges.Test(StateChangeViewMatrix())) {
				m_pstateDevice->m_matView = m_pstate->m_matView;
            }

            if (maskChanges.Test(StateChangeShadeMode())) {
                m_pstateDevice->m_shadeMode = m_pstate->m_shadeMode;
            }

            if (maskChanges.Test(StateChangeBlendMode())) {
                m_pstateDevice->m_blendMode = m_pstate->m_blendMode;
            }

            if (maskChanges.Test(StateChangeWrapMode())) {
                m_pstateDevice->m_wrapMode = m_pstate->m_wrapMode;
            }

            if (maskChanges.Test(StateChangeCullMode())) {
                m_pstateDevice->m_cullMode = m_pstate->m_cullMode;
            }

            if (maskChanges.Test(StateChangeZTest())) {
                m_pstateDevice->m_bZTest = m_pstate->m_bZTest;
            }

            if (maskChanges.Test(StateChangeZWrite())) {
                m_pstateDevice->m_bZWrite = m_pstate->m_bZWrite;
            }

            if (maskChanges.Test(StateChangeLinearFilter())) {
                m_pstateDevice->m_bLinearFilter = m_pstate->m_bLinearFilter;
            }

            if (maskChanges.Test(StateChangePerspectiveCorrection())) {
                m_pstateDevice->m_bPerspectiveCorrection = m_pstate->m_bPerspectiveCorrection;
            }

            if (maskChanges.Test(StateChangeDither())) {
                m_pstateDevice->m_bDither = m_pstate->m_bDither;
            }

            if (maskChanges.Test(StateChangeColorKey())) {
                m_pstateDevice->m_bColorKey = m_pstate->m_bColorKey;
            }

            if (maskChanges.Test(StateChangeLineWidth())) {
                m_pstateDevice->m_lineWidth = m_pstate->m_lineWidth;
            }
        }

        m_pstateDevice->m_maskChanges.Set(m_pstate->m_maskChanges);
        m_pstateDevice->m_maskOverride.Clear(m_pstate->m_maskOverride);

        m_pstate = m_pstate->m_pnext;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clipping Rect
    //
    //////////////////////////////////////////////////////////////////////////////

    Point Transform2D(const Point& point)
    {
        Point pointImage;

        ZVerify(TransformLocalToImage(Vector(point.X(), point.Y(), 0), pointImage));

        return pointImage;
    }

    WinRect GetSurfaceClipRect()
    {
        return 
            WinRect(
                int(m_pstateDevice->m_rectClip.XMin()),
                m_psurface->GetSize().Y() - int(m_pstateDevice->m_rectClip.YMax()),
                int(m_pstateDevice->m_rectClip.XMax()),
                m_psurface->GetSize().Y() - int(m_pstateDevice->m_rectClip.YMin())
            );
    }

    void Clip(const Rect& rect)
    {
        ZAssert(!m_bIn3DLayer);

        if (!m_pstate->m_maskChanges.Test(StateChangeClipRect())) {
            m_pstate->m_rectClip = m_pstateDevice->m_rectClip;
            m_pstate->m_maskChanges.Set(StateChangeClipRect());
        }

        m_pstateDevice->m_rectClip.Intersect(
            Rect(
                Transform2D(rect.Min()),
                Transform2D(rect.Max())
            )
        );

        m_pstateDevice->m_maskChanges.Set(StateChangeClipRect());
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clipping planes
    //
    //////////////////////////////////////////////////////////////////////////////

    void AddClipPlane(const Plane& plane)
    {
        if (!m_pstate->m_maskChanges.Test(StateChangeClipPlanes())) {
            m_pstate->m_countClipPlanes = m_pstateDevice->m_countClipPlanes;
            m_pstate->m_maskChanges.Set(StateChangeClipPlanes());
        }

        UpdateState();
        m_pdevice3D->AddClipPlane(plane);
        m_pstateDevice->m_countClipPlanes++;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 3D Modeling Transforms
    //
    //////////////////////////////////////////////////////////////////////////////

    void PreMatrixChanged()
    {
        ZAssert(m_bIn3DLayer);

        if (!m_pstate->m_maskChanges.Test(StateChangeMatrix())) {
            m_pstate->m_mat = m_pstateDevice->m_mat;
			m_pstate->m_matWorldTM = m_pstateDevice->m_matWorldTM;
            m_pstate->m_maskChanges.Set(StateChangeMatrix());
        }

        m_pstateDevice->m_maskChanges.Set(StateChangeMatrix());
    }

    void SetTransform(const Matrix& mat, const Matrix& worldTM)
    {
        PreMatrixChanged();
        m_pstateDevice->m_mat = mat;
		m_pstateDevice->m_matWorldTM = worldTM;
    }

	void SetPerspectiveMatrix( Camera * pcamera )
	{
        SetPerspectiveMatrix(pcamera->GetPerspectiveMatrix());
	}

	void SetViewMatrix( Camera * pcamera )
	{
		SetViewMatrix(pcamera->GetModelMatrix());
	}

    void Multiply(const Matrix& mat)
    {
        PreMatrixChanged();
        m_pstateDevice->m_mat.PreMultiply(mat);
        m_pstateDevice->m_matWorldTM.PreMultiply(mat);
    }

    void Rotate(const Vector& vec, float angle)
    {
        PreMatrixChanged();
        m_pstateDevice->m_mat.PreRotate(vec, angle);
        m_pstateDevice->m_matWorldTM.PreRotate(vec, angle);
    }

    void Translate(const Vector& vec)
    {
        PreMatrixChanged();
        m_pstateDevice->m_mat.PreTranslate(vec);
        m_pstateDevice->m_matWorldTM.PreTranslate(vec);
    }

    void Scale(const Vector& vec)
    {
        PreMatrixChanged();
        m_pstateDevice->m_mat.PreScale(vec);
        m_pstateDevice->m_matWorldTM.PreScale(vec);
    }

    void Scale3(float scale)
    {
        PreMatrixChanged();
        m_pstateDevice->m_mat.PreScale(scale);
        m_pstateDevice->m_matWorldTM.PreScale(scale);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 2D Modeling Transforms
    //
    //////////////////////////////////////////////////////////////////////////////

    void PrePerspectiveMatrixChanged()
    {
        ZAssert(!m_bIn3DLayer);

        if (!m_pstate->m_maskChanges.Test(StateChangePerspectiveMatrix())) {
            m_pstate->m_matPerspective = m_pstateDevice->m_matPerspective;
            m_pstate->m_maskChanges.Set(StateChangePerspectiveMatrix());
        }

        m_pstateDevice->m_maskChanges.Set(StateChangePerspectiveMatrix());
    }

    void SetMatrix(const Matrix2& mat)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective = mat;
    }

    void Multiply(const Matrix2& mat)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective.PreMultiply(mat);
    }

    void Rotate(float angle)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective.PreRotate(Vector(0, 0, 1), angle);
    }

    void Translate(const Point& point)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective.PreTranslate(Vector(point.X(), point.Y(), 0));
    }

    void Scale(const Point& point)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective.PreScale(Vector(point.X(), point.Y(), 0));
    }

    void Scale2(float scale)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective.PreScale(Vector(scale, scale, 1));
    }

    void SetPerspectiveMatrix(const Matrix& mat)
    {
        PrePerspectiveMatrixChanged();
        m_pstateDevice->m_matPerspective.PreMultiply(mat);
    }

    void SetViewMatrix(const Matrix& mat)
    {
        if (!m_pstate->m_maskChanges.Test(StateChangeViewMatrix())) {
            m_pstate->m_matView = m_pstateDevice->m_matView;
            m_pstate->m_maskChanges.Set(StateChangeViewMatrix());
        }
        m_pstateDevice->m_maskChanges.Set(StateChangeViewMatrix());
        m_pstateDevice->m_matView = mat;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Transform Operations
    //
    //////////////////////////////////////////////////////////////////////////////

    const Matrix& GetLocalToEyeMatrix()
    {
        return m_pstateDevice->m_mat;
    }

    const Matrix& GetEyeToLocalMatrix()
    {
        UpdateState();
        return m_pdevice3D->GetInverseModelMatrix();
    }

    Vector TransformEyeToLocal(const Vector& vec)
    {
        return GetEyeToLocalMatrix().Transform(vec);
    }

    Vector TransformLocalToEye(const Vector& vec)
    {
        return GetLocalToEyeMatrix().Transform(vec);
    }

    Vector TransformDirectionToEye(const Vector& vec)
    {
        HVector hvec = m_pstateDevice->m_mat.Transform(HVector(vec.X(), vec.Y(), vec.Z(), 0));
        return Vector(hvec.X(), hvec.Y(), hvec.Z());
    }

    bool TransformEyeToImage(const Vector& vecEye, Point& point)
    {
        HVector hvecImage = m_pstateDevice->m_matPerspective.Transform(HVector(vecEye));

        if (hvecImage.W() < 0) {
            return false;
        }

        float rhw = 1.0f / hvecImage.W();
        point =
            Point(
                rhw * hvecImage.X(),
                rhw * hvecImage.Y()
            );

        return true;
    }

    bool TransformDirectionToImage(const Vector& vec, Point& point)
    {
        return TransformEyeToImage(TransformDirectionToEye(vec), point);
    }

    bool TransformLocalToImage(const Vector& vec, Point& point)
    {
        return TransformEyeToImage(TransformLocalToEye(vec), point);
    }

    WinPoint TransformImageToSurface(const Point& point)
    {
        const WinPoint& size = m_psurface->GetSize();

        return
            WinPoint(
                MakeInt(point.X()),
                size.Y() - MakeInt(point.Y())
            );
    }

    float GetImageRadius(const Vector& vec, float radius)
    {
        Vector vecEye = TransformLocalToEye(vec);

        Point point;
        if (TransformEyeToImage(vecEye, point)) {
            Point pointRadius;
            if (TransformEyeToImage(vecEye + Vector(radius, 0, 0), pointRadius)) {
                return pointRadius.X() - point.X();
            }
        }

        return radius;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void DirectionalLight(const Vector& vec, const Color& color)
    {
        UpdateState();
        m_pdevice3D->DirectionalLight(vec, color);
    }

    void BidirectionalLight(const Vector& direction, const Color& color, const Color& colorAlt)
    {
        UpdateState();
        m_pdevice3D->BidirectionalLight(direction, color, colorAlt);
    }

    void SetAmbientLevel(float ambientLevel)
    {
        //  , this should be on the state stack
        m_pdevice3D->SetAmbientLevel(ambientLevel);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Callbacks
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawCallbackGeo(IGeoCallback* pgeoCallback, bool bSortObject)
    {
        if (m_bIn3DLayer) {
            if (m_bRenderingCallbacks) {
                PushState();
                pgeoCallback->RenderCallback(this);
                PopState();
            } else {
                if (bSortObject) {
                    GeoCallbackData dataNew;
                    dataNew.m_mat          = m_pstateDevice->m_mat;
					dataNew.m_matWorldTM   = m_pstateDevice->m_matWorldTM;
                    dataNew.m_pgeoCallback = pgeoCallback;
                    dataNew.m_distance     = m_pstateDevice->m_mat.GetTranslate().LengthSquared();

                    m_listGeoCallbacksSorted.InsertSorted(dataNew);
                } else {
                    m_listGeoCallbacks.PushFront();

                    GeoCallbackData& data = m_listGeoCallbacks.GetFront();
                    data.m_mat            = m_pstateDevice->m_mat;
					data.m_matWorldTM     = m_pstateDevice->m_matWorldTM;
                    data.m_pgeoCallback   = pgeoCallback;
                }
            }
        } else {
            //
            // Drawing a geo in 2D.  Draw it now even though it is a callback geo
            //

            pgeoCallback->RenderCallback(this);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Buffers
    //
    //////////////////////////////////////////////////////////////////////////////

    void SizeVertexBuffer(int count)
    {
        if (m_countVertexBuffer < count) {
            m_countVertexBuffer = count;
            m_pvertexBuffer = (Vertex*)realloc(m_pvertexBuffer, sizeof(Vertex) * count);
        }
    }

    void SizeIndexBuffer(int count)
    {
        if (m_countIndexBuffer < count) {
            m_countIndexBuffer = count;
            m_pindexBuffer = (MeshIndex*)realloc(m_pindexBuffer, sizeof(MeshIndex) * count);
        }
    }

    Vertex* GetVertexBuffer(int count)
    {
        SizeVertexBuffer(count);
        return m_pvertexBuffer;
    }

    MeshIndex* GetIndexBuffer(int count)
    {
        SizeIndexBuffer(count);
        return m_pindexBuffer;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Buffers implemented by Device3D
    //
    //////////////////////////////////////////////////////////////////////////////

    VertexL* GetVertexLBuffer(int count)
    {
        return m_pdevice3D->GetVertexLBuffer(count);
    }

    VertexScreen* GetVertexScreenBuffer(int count)
    {
        return m_pdevice3D->GetVertexScreenBuffer(count);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Context Drawing modes
    //
    //////////////////////////////////////////////////////////////////////////////

    void DD2D()
    {
        ZAssert(m_bRendering);
        ZAssert(!m_bIn3DLayer);

        EndScene();

        UpdateState();
    }

    void Texture2D()
    {
        ZAssert(m_bRendering);
        ZAssert(!m_bIn3DLayer);

        BeginScene();
        UpdateState();
    }

    void Mode3D()
    {
        ZAssert(m_bRendering);

        if (!m_bIn3DLayer) {
            Texture2D();
        } else {
            ZAssert(m_bInScene);
            UpdateState();
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // External Rendering Calls
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawTriangles(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        if (icount != 0) {
            Mode3D();
            m_pdevice3D->DrawTriangles(pvertex, vcount, pindex, icount);
        }
    }

    void DrawTriangles(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        if (icount != 0) {
            Mode3D();
            m_pdevice3D->DrawTriangles(pvertex, vcount, pindex, icount);
        }
    }

    void DrawTriangles(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        if (icount != 0) {
            Mode3D();
            m_pdevice3D->DrawTriangles(pvertex, vcount, pindex, icount);
        }
    }

    void DrawTriangles(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        Mode3D();
        m_pdevice3D->DrawTriangles(pvertex, vcount, pindex, icount);
    }

    void DrawLineStrip(const Vertex* pvertex, int vcount)
    {
        int icount = (vcount - 1) * 2;
        MeshIndex* pindex = GetIndexBuffer(icount);

        for (int index = 0; index < vcount - 1; index++) {
            pindex[index * 2    ] = index;
            pindex[index * 2 + 1] = index + 1;
        }

        DrawLines(pvertex, vcount, pindex, icount);
    }

    void DrawLineStrip(const VertexL* pvertex, int vcount)
    {
        int icount = (vcount - 1) * 2;
        MeshIndex* pindex = GetIndexBuffer(icount);

        for (int index = 0; index < vcount - 1; index++) {
            pindex[index * 2    ] = index;
            pindex[index * 2 + 1] = index + 1;
        }

        DrawLines(pvertex, vcount, pindex, icount);
    }

    void DrawLineStrip(const VertexScreen* pvertex, int vcount)
    {
        int icount = (vcount - 1) * 2;
        MeshIndex* pindex = GetIndexBuffer(icount);

        for (int index = 0; index < vcount - 1; index++) {
            pindex[index * 2    ] = index;
            pindex[index * 2 + 1] = index + 1;
        }

        DrawLines(pvertex, vcount, pindex, icount);
    }

    void DrawLineStrip(const D3DVertex* pvertex, int vcount)
    {
        int icount = (vcount - 1) * 2;
        MeshIndex* pindex = GetIndexBuffer(icount);

        for (int index = 0; index < vcount - 1; index++) {
            pindex[index * 2    ] = index;
            pindex[index * 2 + 1] = index + 1;
        }

        DrawLines(pvertex, vcount, pindex, icount);
    }

    void DrawLines(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        if (icount != 0) {
            Mode3D();
            m_pdevice3D->DrawLines(pvertex, vcount, pindex, icount);
        }
    }

    void DrawLines(const VertexL*  pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        if (icount != 0) {
            Mode3D();
            m_pdevice3D->DrawLines(pvertex, vcount, pindex, icount);
        }
    }

    void DrawLines(const VertexScreen*  pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        if (icount != 0) {
            Mode3D();
            m_pdevice3D->DrawLines(pvertex, vcount, pindex, icount);
        }
    }

    void DrawLines(const D3DVertex*  pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        Mode3D();
        m_pdevice3D->DrawLines(pvertex, vcount, pindex, icount);
    }

    void DrawPoints(const Vertex* pvertex, int vcount)
    {
        if (vcount != 0) {
            Mode3D();
            m_pdevice3D->DrawPoints(pvertex, vcount);
        }
    }

    void DrawPoints(const VertexL* pvertex, int vcount)
    {
        if (vcount != 0) {
            Mode3D();
            m_pdevice3D->DrawPoints(pvertex, vcount);
        }
    }

    void DrawPoints(const VertexScreen* pvertex, int vcount)
    {
        if (vcount != 0) {
            Mode3D();
            m_pdevice3D->DrawPoints(pvertex, vcount);
        }
    }

    void DrawPoints(const D3DVertex* pvertex, int vcount)
    {
        Mode3D();
        m_pdevice3D->DrawPoints(pvertex, vcount);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Vector Rendering Calls
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawTriangles(const TVector<Vertex>& vertices, const TVector<MeshIndex>& indices)
    {
        DrawTriangles(&(vertices[0]), vertices.GetCount(), &(indices[0]), indices.GetCount());
    }

	//NEW RENDER PATH
	void DrawTriangles(	const CVBIBManager::SVBIBHandle * phVB, 
						const CVBIBManager::SVBIBHandle * phIB )
	{
		if (phIB->dwNumElements != 0) 
		{
            Mode3D();
            m_pdevice3D->DrawTriangles( D3DPT_TRIANGLELIST, phVB, phIB );
		}
	}
	
    void DrawTriangles(const TVector<VertexL>& vertices, const TVector<MeshIndex>& indices)
    {
        DrawTriangles(&(vertices[0]), vertices.GetCount(), &(indices[0]), indices.GetCount());
    }

    void DrawTriangles(const TVector<D3DVertex>& vertices, const TVector<MeshIndex>& indices)
    {
        DrawTriangles(&(vertices[0]), vertices.GetCount(), &(indices[0]), indices.GetCount());
    }

    void DrawLines(const TVector<Vertex>& vertices, const TVector<MeshIndex>& indices)
    {
        DrawLines(&(vertices[0]), vertices.GetCount(), &(indices[0]), indices.GetCount());
    }

    void DrawLines(const TVector<VertexL>& vertices, const TVector<MeshIndex>& indices)
    {
        DrawLines(&(vertices[0]), vertices.GetCount(), &(indices[0]), indices.GetCount());
    }

    void DrawLines(const TVector<D3DVertex>& vertices, const TVector<MeshIndex>& indices)
    {
        DrawLines(&(vertices[0]), vertices.GetCount(), &(indices[0]), indices.GetCount());
    }

    void DrawPoints(const TVector<Vertex>& vertices)
    {
        DrawPoints(&(vertices[0]), vertices.GetCount());
    }

    void DrawPoints(const TVector<VertexL>& vertices)
    {
        DrawPoints(&(vertices[0]), vertices.GetCount());
    }

    void DrawPoints(const TVector<D3DVertex>& vertices)
    {
        DrawPoints(&(vertices[0]), vertices.GetCount());
    }
    
    //////////////////////////////////////////////////////////////////////////////
    //
    // Add a new Decal
    //
    //////////////////////////////////////////////////////////////////////////////

    Decal& AddDecal(TVector<DecalSet>& vdecalSet, Surface* psurface)
    {
        //
        // find the associated decal set
        //

        int countDecalSets = vdecalSet.GetCount();
		// mdvalley: Define that out here.
		int indexSet;
        for(indexSet = 0; indexSet < countDecalSets; indexSet++) {
            const DecalSet& set = vdecalSet[indexSet];

            if (psurface == set.m_psurface) {
                break;
            }
        }

        //
        // Add a new decal set
        //

        if (indexSet == countDecalSets) {
            vdecalSet.PushEnd();
            vdecalSet.Get(indexSet).m_psurface = psurface;
        }

        DecalSet& set = vdecalSet.Get(indexSet);

        //
        // Return a reference to the newly created decal
        //

        set.m_vdecal.PushEnd();
        return set.m_vdecal.Get(set.m_vdecal.GetCount() - 1);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw a Decal so that it faces the eye point and lies along a direction vector
    //
    //////////////////////////////////////////////////////////////////////////////

    float DrawDecal(
        Surface*      psurface,
        const Color&  color,
        const Vector& positionLocal,
        const Vector& forwardLocal,
        const Vector& rightLocal,
        float         scale,
        float         angle,
        BlendMode     blendMode
    ) {
        ZAssert(
               blendMode == BlendModeSource
            || blendMode == BlendModeAdd
        );

        const Matrix& mat = GetLocalToEyeMatrix();

        Vector position = mat.Transform(positionLocal);

        if (position.Z() < 0) {
            Decal& decal =
                AddDecal(
                      (blendMode == BlendModeSource)
                    ? m_vdecalSetOpaque
                    : m_vdecalSet,
                    psurface
                );

            float sizeScale = 4.0f / (position.Z() * position.Z());

            float scaleMat = mat.GetScale();

            decal.m_position = position;
            decal.m_color    = color;
            decal.m_angle    = angle;
            decal.m_scale    = scaleMat * scale;

            if (forwardLocal.IsZero()) {
                decal.m_forward = Vector(0, 0, 0);

                return sizeScale * decal.m_scale * decal.m_scale;
            } else {
                decal.m_forward = scaleMat * mat.TransformDirection(forwardLocal);

                if (rightLocal.IsZero()) {
                    Vector right         = decal.m_forward.GetOrthogonalVector();
                    float  lengthForward = forwardLocal.Length();
                    float  lengthRight   = decal.m_scale * lengthForward;

                    decal.m_right = lengthRight * right.Normalize();
                    return sizeScale * lengthForward * lengthRight;
                } else {
                    decal.m_right = scaleMat * mat.TransformDirection(rightLocal);

                    return sizeScale * decal.m_right.Length() * decal.m_forward.Length();
                }
            }
        }

        return 0;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Decal Drawing
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawVDecalSet(TVector<DecalSet>& vdecalSet)
    {
        //
        // Iterate through the decal sets
        //

        int countDecalSets = vdecalSet.GetCount();
        for(int indexSet = 0; indexSet < countDecalSets; indexSet++) 
		{
            DecalSet& set = vdecalSet.Get(indexSet);
            int countDecal = set.m_vdecal.GetCount();

            if (countDecal > 0) 
			{


                SetTexture(set.m_psurface, false);

                VertexL*   pvertex = GetVertexLBuffer(countDecal * 4);
                MeshIndex* pindex  = GetIndexBuffer(countDecal * 6);
				// mdvalley: Another moved definition
				int index;

                for (index = 0; index < countDecal; index++) 
				{
                    set.m_vdecal[index].GetVertices(pvertex + index * 4);

                    pindex[index * 6 + 0] = index * 4 + 0;
                    pindex[index * 6 + 1] = index * 4 + 2;
                    pindex[index * 6 + 2] = index * 4 + 1;
                    pindex[index * 6 + 3] = index * 4 + 0;
                    pindex[index * 6 + 4] = index * 4 + 3;
                    pindex[index * 6 + 5] = index * 4 + 2;
                }

               	UpdateState();

                m_pdevice3D->DrawTriangles( &pvertex[0],
						                    index * 4,
											&pindex[0],
											index * 6 );

                set.m_vdecal.SetCount(0);
            }
        }
    }

    void DrawCachedDecals()
    {
        //
        // Render States
        //

        SetCullMode(CullModeNone, false);
        SetShadeMode(ShadeModeFlat, false);
        SetTransform(Matrix::GetIdentity(),Matrix::GetIdentity());

        //
        // Opaque decals
        //

        SetBlendMode(BlendModeSource, false); //Imago 7/16/09 7/31/09
        SetZWrite(true, false);

        DrawVDecalSet(m_vdecalSetOpaque);


        //
        // Emissive Transparent decals
        //

        SetBlendMode(BlendModeAdd, false);
        SetZWrite(false, false);

        DrawVDecalSet(m_vdecalSet);



    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Performance Counters
    //
    //////////////////////////////////////////////////////////////////////////////

    #ifdef EnablePerformanceCounters

        int GetPerformanceCounter(Counter counter)
        {
            if (counter == CounterDrawStrings)
                return m_countDrawString;
            else if (counter == CounterDrawStringChars)
                return m_countDrawStringChars;

            return m_pdevice3D->GetPerformanceCounter(counter);
        }

        void ResetPerformanceCounters()
        {
            m_countDrawString = 0;
            m_countDrawStringChars = 0;

            m_pdevice3D->ResetPerformanceCounters();
        }

    #endif
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateContextImpl()
// A particular context used to be associated with a particular surface usually the highest
// level pane which would be whole window.
// Now, we just pass in the size of the window to the constructor.
////////////////////////////////////////////////////////////////////////////////////////////////////
TRef<PrivateContext> CreateContextImpl( PrivateSurface* psurface )
{
	return new ContextImpl( psurface );
}
