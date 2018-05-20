#include <color.h>

#include <d3d9.h>
#include <matrix.h>
#include <point.h>
#include <tref.h>
#include <vertex.h>

#include "context.h"
#include "D3DDevice9.h"
#include "enginep.h"
#include "material.h"

// compile this file for speed

#pragma optimize("t", on)
// #pragma optimize("s", on)

//////////////////////////////////////////////////////////////////////////////
//
// Clipping Planes
//
//////////////////////////////////////////////////////////////////////////////

const int g_maxCountPlanes = 8;

typedef BYTE Code;
const Code ClipFront  = 0x01;
const Code ClipBack   = 0x02;
const Code ClipLeft   = 0x04;
const Code ClipRight  = 0x08;
const Code ClipBottom = 0x10;
const Code ClipTop    = 0x20;
const Code ClipPlane  = 0x40;

//////////////////////////////////////////////////////////////////////////////
//
// Transformed and Lit Vertex
//
//////////////////////////////////////////////////////////////////////////////

class VertexTL {
public:
    HVector    m_pos;
    Color      m_color;
    Point      m_pointTexture;
    MeshIndex  m_index;
    Code       m_code;
    
    VertexTL()
    {
    }

    VertexTL(const VertexTL& v) :
        m_pos(v.m_pos),
        m_color(v.m_color),
        m_pointTexture(v.m_pointTexture),
        m_index(v.m_index),
        m_code(v.m_code)
    {
    }

    void Interpolate(const VertexTL& vertex0, const VertexTL& vertex1, float alpha)
    {
        float beta = 1 - alpha;

        m_pos = ::Interpolate(vertex0.m_pos, vertex1.m_pos, alpha);
        m_color =
            Color(
                alpha * vertex1.m_color.GetRed()   + beta * vertex0.m_color.GetRed()  ,
                alpha * vertex1.m_color.GetGreen() + beta * vertex0.m_color.GetGreen(),
                alpha * vertex1.m_color.GetBlue()  + beta * vertex0.m_color.GetBlue() ,
                alpha * vertex1.m_color.GetAlpha() + beta * vertex0.m_color.GetAlpha()
            );
        m_pointTexture = ::Interpolate(vertex0.m_pointTexture, vertex1.m_pointTexture, alpha);
    }

    void UpdateClipCode()
    {
        m_code = 0;

        if (m_pos.x < -m_pos.w) m_code |= ClipLeft;
        if (m_pos.x >  m_pos.w) m_code |= ClipRight;

        if (m_pos.y < -m_pos.w) m_code |= ClipBottom;
        if (m_pos.y >  m_pos.w) m_code |= ClipTop;

        if (m_pos.z <  0      ) m_code |= ClipFront;
        if (m_pos.z >  m_pos.w) m_code |= ClipBack;
    }

    void UpdateClipCode(const Plane& plane, Code code)
    {
        if (plane.Distance(m_pos) <= 0) {
            m_code |= code;
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Device3D
//
//////////////////////////////////////////////////////////////////////////////

class Device3D : public IDevice3D {
    //
    // Types
    //

    typedef void (Device3D::*PFNClip)(VertexTL* pvertex, int vcount, const MeshIndex* pindex, int icount);
    typedef void (Device3D::*PFNDraw)(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount);
    typedef void (Device3D::*PFNVertex)(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw);
    typedef void (Device3D::*PFNVertexL)(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw);

    //
    // Interfaces
    //

    TRef<Rasterizer>    m_prasterizer;

    //
    // Current state
    //


    Point                m_sizeSurface;
    Rect                 m_rectClip;
    Rect                 m_rectClipScreen;
    bool                 m_bClip;
    bool                 m_bYAxisInversion;

    bool                 m_bZTest;
    bool                 m_bZWrite;
    ShadeMode            m_shadeMode;
    BlendMode            m_blendMode;

    PFNVertexL           m_pfnTransform;
    PFNVertex            m_pfnLightVertex;
    PFNVertexL           m_pfnLightVertexL;
    PFNDraw              m_pfnDrawLines;

    Matrix               m_mat;
	Matrix               m_matWorldTM;
    Matrix               m_matPerspective;
    Matrix               m_matView;
    Matrix               m_matFull;
    Matrix               m_matInverse;
	Matrix				 m_matInverseWorld;
	Matrix				 m_matLight;
	D3DMATRIX			 m_matPerspectiveD3D;
	D3DMATRIX			 m_matViewD3D;
	D3DMATRIX			 m_matWorldD3D;
 	D3DMATERIAL9		 m_materialD3D;
	D3DLIGHT9			 m_light0D3D;
	D3DLIGHT9			 m_light1D3D;
    bool                 m_bUpdateMatFull;
    bool                 m_bUpdateLighting;
    bool                 m_bUpdatePointers;
    bool                 m_bUpdateInverse;
	bool				 m_bUpdateInverseWorld;

    TRef<Material>       m_pmaterial;
    TRef<Deformation>    m_pdeform;

    //
    // Line drawing attributes
    //

    float                m_lineWidth;

    //
    // Lighting
    //

    float                m_brightAmbient;
    Vector               m_vecLight;
    Vector               m_vecLightWorld;
    Color                m_color;
    Color                m_colorAlt;
    Color                m_colorGlobal;
    bool                 m_bBidirectional;

    //
    // Clipping
    //

    float m_scalex;
    float m_scaley;
    float m_xoffset;
    float m_yoffset;

    int   m_countPlanes;
    Plane m_pplane[g_maxCountPlanes];

    //
    // Temporary vertex and index buffers
    //

    VertexL*      m_pvertexLBuffer;
    int           m_countVertexLBuffer;

    VertexTL*     m_pvertexTLBuffer;
    int           m_countVertexTLBuffer;

    VertexScreen* m_pvertexScreenBuffer;
    int           m_countVertexScreenBuffer;
    int           m_indexVertexScreenBuffer;

    MeshIndex*    m_pindexBuffer;
    int           m_countIndexBuffer;
    int           m_indexIndexBuffer;

    //
    // Performance counters
    //

    #ifdef EnablePerformanceCounters
        int m_countPoints;
        int m_countLines;
        int m_countTriangles;
        int m_countMeshes;
        int m_countObjects;
        int m_countObjectsRendered;
        int m_countMatrixLoads;
    #endif

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    Device3D(Rasterizer* prasterizer) :
        m_prasterizer(prasterizer),

        m_pvertexLBuffer(NULL),
        m_countVertexLBuffer(0),

        m_pvertexTLBuffer(NULL),
        m_countVertexTLBuffer(0),

        m_pvertexScreenBuffer(NULL),
        m_countVertexScreenBuffer(0),
        m_indexVertexScreenBuffer(0),

        m_pindexBuffer(NULL),
        m_countIndexBuffer(0),
        m_indexIndexBuffer(0),

        m_bClip(true),
        m_rectClip(0.0f, 0.0f, 1.0f, 1.0f),

        m_shadeMode(ShadeModeGouraud),
        m_bUpdateMatFull(true),
        m_bUpdateLighting(true),
        m_bUpdatePointers(true),
        m_brightAmbient(0.0f),
        m_vecLightWorld(1, 0, 0),
        m_color(Color::White()),
        m_colorAlt(Color::White()),
        m_colorGlobal(Color::White()),

		// mdvalley: Added pointer and class name. Search for '&Device3D::' to find more.
		m_pfnDrawLines(&Device3D::DrawLinesRasterizer),
        m_lineWidth(0.5f),

        m_bYAxisInversion(true)
    {
        m_sizeSurface = prasterizer->GetSurfaceSize();
        m_mat.SetIdentity();
        m_matPerspective.SetIdentity();

        //
        // Setup the clipping planes
        //

        m_countPlanes = 6;
        m_pplane[0] = Plane(HVector( 0, 0, 1,  0));
        m_pplane[1] = Plane(HVector( 0, 0, 1, -1));
        m_pplane[2] = Plane(HVector( 1, 0, 0,  1));
        m_pplane[3] = Plane(HVector(-1, 0, 0,  1));
        m_pplane[4] = Plane(HVector( 0, 1, 0,  1));
        m_pplane[5] = Plane(HVector( 0,-1, 0,  1));

        //
        // Initialize Performance counters
        //

        #ifdef EnablePerformanceCounters
            ResetPerformanceCounters();
        #endif
   
		m_matLight.SetIdentity( );
		m_matLight.Set( 0, 0, -1.0f );
		m_matLight.Set( 2, 2, -1.0f );
	}

    bool IsValid()
    {
        return m_prasterizer->IsValid();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Performance counters
    //
    //////////////////////////////////////////////////////////////////////////////

    #ifdef EnablePerformanceCounters
        void ResetPerformanceCounters()
        {
            m_countPoints          = 0;
            m_countLines           = 0;
            m_countTriangles       = 0;
            m_countMeshes          = 0;
            m_countObjects         = 0;
            m_countObjectsRendered = 0;
            m_countMatrixLoads     = 0;
        }

        int GetPerformanceCounter(Counter counter)
        {
            switch (counter) {
                case CounterPoints          : return m_countPoints         ;
                case CounterLines           : return m_countLines          ;
                case CounterTriangles       : return m_countTriangles      ;
                case CounterMeshes          : return m_countMeshes         ;
                case CounterObjects         : return m_countObjects        ;
                case CounterObjectsRendered : return m_countObjectsRendered;
                case CounterMatrixLoads     : return m_countMatrixLoads    ;
            }

            ZError("Unknown counter");
            return 0;
        }
    #endif

    //////////////////////////////////////////////////////////////////////////////
    //
    // Destructor
    //
    //////////////////////////////////////////////////////////////////////////////

    ~Device3D()
    {
        if (m_pvertexLBuffer) {
            free(m_pvertexLBuffer);
        }

        if (m_pvertexTLBuffer) {
            free(m_pvertexTLBuffer);
        }

        if (m_pvertexScreenBuffer) {
            free(m_pvertexScreenBuffer);
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
        return m_prasterizer->Has3DAcceleration();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Device
    //
    //////////////////////////////////////////////////////////////////////////////

    void BeginScene()
    {
        m_prasterizer->BeginScene();
    }

    void EndScene()
    {
        m_prasterizer->EndScene();
    }

    void ClearZBuffer()
    {
        m_prasterizer->ClearZBuffer();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // View Rect
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetYAxisInversion(bool bValue) {
        m_bYAxisInversion = bValue;

        GenerateProjectionTransform();
    }

    void SetClipRect(const Rect& rectClip)
    {
        m_rectClip       = rectClip;

        if (!m_bYAxisInversion) {
            m_rectClipScreen =
                Rect(
                    rectClip.XMin(),
                    rectClip.YMin(),
                    rectClip.XMax(),
                    rectClip.YMax()
                );
        }
        else {
            m_rectClipScreen =
                Rect(
                    rectClip.XMin(),
                    m_sizeSurface.Y() - rectClip.YMax(),
                    rectClip.XMax(),
                    m_sizeSurface.Y() - rectClip.YMin()
                );
        }

        m_bUpdateMatFull = true;
        m_prasterizer->SetClipRect(m_rectClipScreen);

        GenerateProjectionTransform();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void DirectionalLight(const Vector& vec, const Color& color)
    {
//		m_vecLightWorld = m_matView.TransformDirection(vec);
  //      m_vecLightWorld = m_mat.TransformDirection(m_vecLightWorld);
		m_vecLightWorld  = vec;
        m_color          = color;
		m_bBidirectional = false;
		m_bUpdateLighting = true;
	}

    void BidirectionalLight(const Vector& vec, const Color& color, const Color& colorAlt)
    {
// 		m_vecLightWorld = m_matView.TransformDirection(vec);
//        m_vecLightWorld = m_mat.TransformDirection(m_vecLightWorld);
		m_vecLightWorld  = vec;
		m_color          = color;
        m_colorAlt       = colorAlt;
        m_bBidirectional = true;
		m_bUpdateLighting = true;
    }

    void SetAmbientLevel(float level)
    {
        m_brightAmbient = level;
		m_bUpdateLighting = true;
    }

    void SetGlobalColor(const Color& color)
    {
        m_colorGlobal = color;
		m_bUpdateLighting = true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

	void GenerateProjectionTransform( )
	{
		Matrix matD3D;

		matD3D = m_matPerspective;
		if (m_bClip) 
		{
			Point pointCenter = m_rectClip.Center();

			// move view rect into NDC
			float scalex = 0.5f * m_rectClip.XSize();
			float scaley = 0.5f * m_rectClip.YSize();

			matD3D.Translate( Vector( -pointCenter.X(), -pointCenter.Y(), 0.0f ) );
			matD3D.Scale( Vector( 1.0f / scalex, 1.0f / scaley, 1.0f ));
		} 
		else 
		{
			// Flip the y coordinate
            ZAssert( false );		// TODO: check this path.
			matD3D.Translate(Vector(0, -m_sizeSurface.Y(), 0));
			matD3D.Scale(Vector(1, -1, 1));
		}

        if (!m_bYAxisInversion) {
            matD3D.Scale(Vector(1, -1, 1));
        }

		// Reorganise the perspective matrix.
		m_matPerspectiveD3D._11 = matD3D[0][0];
		m_matPerspectiveD3D._12 = matD3D[1][0];
		m_matPerspectiveD3D._13 = matD3D[2][0];
		m_matPerspectiveD3D._14 = matD3D[3][0];
		m_matPerspectiveD3D._21 = matD3D[0][1];
		m_matPerspectiveD3D._22 = matD3D[1][1];
		m_matPerspectiveD3D._23 = matD3D[2][1];
		m_matPerspectiveD3D._24 = matD3D[3][1];
		m_matPerspectiveD3D._31 = matD3D[0][2];
		m_matPerspectiveD3D._32 = matD3D[1][2];
		m_matPerspectiveD3D._33 = matD3D[2][2];
		m_matPerspectiveD3D._34 = matD3D[3][2];
		m_matPerspectiveD3D._41 = matD3D[0][3];
		m_matPerspectiveD3D._42 = matD3D[1][3];
		m_matPerspectiveD3D._43 = matD3D[2][3];
		m_matPerspectiveD3D._44 = matD3D[3][3];

		// Set.
		CD3DDevice9::Get()->SetTransform( D3DTS_PROJECTION, &m_matPerspectiveD3D);
	}
	
	void GenerateViewTransform( )
	{
		// Reorganise the view matrix.
		m_matViewD3D._11 = m_matView[0][0];			// First version
		m_matViewD3D._12 = m_matView[1][0];
		m_matViewD3D._13 = m_matView[2][0];
		m_matViewD3D._14 = m_matView[3][0];
		m_matViewD3D._21 = m_matView[0][1];
		m_matViewD3D._22 = m_matView[1][1];
		m_matViewD3D._23 = m_matView[2][1];
		m_matViewD3D._24 = m_matView[3][1];
		m_matViewD3D._31 = m_matView[0][2];
		m_matViewD3D._32 = m_matView[1][2];
		m_matViewD3D._33 = m_matView[2][2];
		m_matViewD3D._34 = m_matView[3][2];
		m_matViewD3D._41 = m_matView[0][3];
		m_matViewD3D._42 = m_matView[1][3];
		m_matViewD3D._43 = m_matView[2][3];
		m_matViewD3D._44 = m_matView[3][3];

		CD3DDevice9::Get()->SetTransform( D3DTS_VIEW, &m_matViewD3D );
	}

	void GenerateWorldTransform( )
	{
		// Finally apply the world transform.
		m_matWorldD3D._11 = m_matWorldTM[0][0];
		m_matWorldD3D._12 = m_matWorldTM[1][0];
		m_matWorldD3D._13 = m_matWorldTM[2][0];
		m_matWorldD3D._14 = m_matWorldTM[3][0];
		m_matWorldD3D._21 = m_matWorldTM[0][1];
		m_matWorldD3D._22 = m_matWorldTM[1][1];
		m_matWorldD3D._23 = m_matWorldTM[2][1];
		m_matWorldD3D._24 = m_matWorldTM[3][1];
		m_matWorldD3D._31 = m_matWorldTM[0][2];
		m_matWorldD3D._32 = m_matWorldTM[1][2];
		m_matWorldD3D._33 = m_matWorldTM[2][2];
		m_matWorldD3D._34 = m_matWorldTM[3][2];
		m_matWorldD3D._41 = m_matWorldTM[0][3];
		m_matWorldD3D._42 = m_matWorldTM[1][3];
		m_matWorldD3D._43 = m_matWorldTM[2][3];
		m_matWorldD3D._44 = m_matWorldTM[3][3];

        CD3DDevice9::Get()->SetTransform(D3DTS_WORLD, &m_matWorldD3D);
	}

    const Matrix& GetFullMatrix()
    {
        if (m_bUpdateMatFull) {
            m_bUpdateMatFull = false;

            m_matFull = m_mat;
            m_matFull.Multiply(m_matPerspective);

            // !!! In D3D pixels centers are on integer boundaries
            //     The device clips to integer boundaries
            //     subtract 0.25 to move the first row of pixels centers into the clip rect

            if (m_bClip) {
                Point pointCenter = m_rectClip.Center();

                //
                // move view rect into NDC
                //

                float scalex = 0.5f * m_rectClip.XSize();
                float scaley = 0.5f * m_rectClip.YSize();

                m_matFull.Translate(
                    Vector(
                        -pointCenter.X(),
                        -pointCenter.Y(),
                        0
                    )
                );
                m_matFull.Scale(
                    Vector(
                        1 / scalex,
                        1 / scaley,
                        1.0f
                    )
                );

                //
                // these are the scales and translates to go from NDC back to the screen
                //

                m_xoffset = pointCenter.X(); // !!! - 0.25f;
                m_yoffset = m_sizeSurface.Y() - pointCenter.Y(); // !!! - 0.25f;

                m_scalex =  0.5f * m_rectClip.XSize();
                m_scaley = -0.5f * m_rectClip.YSize();
            } else {
                //
                // Flip the y coordinate
                //

                m_matFull.Translate(Vector(0, -m_sizeSurface.Y(), 0));
                m_matFull.Scale(Vector(1, -1, 1));
            }
        }

        return m_matFull;
    }

    void UpdateLighting()
    {
        if (m_bUpdateLighting) 
//		if( true )
		{
			CD3DDevice9 * pDev = CD3DDevice9::Get();
            m_bUpdateLighting = false;
            bool bClip = m_bClip || m_countPlanes > 6;

			// Configure lighting depending on flags.
            switch (m_shadeMode) 
			{
			case ShadeModeNone:
			case ShadeModeCopy:
                if (bClip) 
				{
					// Copy vertex, set light value to 1, 1, 1, 1.
//                    ZAssert( false );
                    m_pfnLightVertex = &Device3D::LightVertexCopy;
				} 
				else 
				{
					// No lighting or transform. Uses textured screen space coords.
                    m_pfnLightVertex = &Device3D::TransformNoClipNoLight;
//                    ZAssert( false );
                }
                pDev->SetRenderState(D3DRS_LIGHTING, FALSE);
                break;

            case ShadeModeGlobalColor:
				// Vertex colour set to constant global colour.
				// Set d3dmaterial diffuse to zero, set ambient to global colour.
				// Set d3dlight diffuse to zero, ambient to 1.0f.

				// Configure the material. Only interested in diffuse
				// and ambient settings for now.
				memset( &m_materialD3D, 0, sizeof( D3DMATERIAL9 ) );
				m_materialD3D.Ambient.r = m_colorGlobal.R();
				m_materialD3D.Ambient.g = m_colorGlobal.G();
				m_materialD3D.Ambient.b = m_colorGlobal.B();
				m_materialD3D.Ambient.a = m_colorGlobal.A();
				pDev->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
				pDev->SetRenderState( D3DRS_AMBIENT, m_colorGlobal.MakeCOLORREF() );
				// KG- DX9 ShadeModeGlobalColor fix
				// also set the material diffuse color as the D3DTOP_BLENDFACTORALPHA value
				pDev->SetRenderState( 
					D3DRS_TEXTUREFACTOR , 
					D3DCOLOR_COLORVALUE(m_colorGlobal.R(),m_colorGlobal.B(),m_colorGlobal.B(),m_colorGlobal.A())
				);
				pDev->SetMaterial( &m_materialD3D );

				// Configure the single light, light direction irrelevant.
				memset( &m_light0D3D, 0, sizeof(D3DLIGHT9) );
				m_light0D3D.Ambient.r = 1.0f;
				m_light0D3D.Ambient.g = 1.0f;
				m_light0D3D.Ambient.b = 1.0f;
				m_light0D3D.Ambient.a = 1.0f;
				m_light0D3D.Direction.x = 0.0f;
				m_light0D3D.Direction.y = 0.0f;
				m_light0D3D.Direction.z = 1.0f;
				m_light0D3D.Type = D3DLIGHT_DIRECTIONAL;

				pDev->SetLight( 0, &m_light0D3D );
				pDev->LightEnable( 0, TRUE );
				pDev->LightEnable( 1, FALSE );
				pDev->SetRenderState( D3DRS_LIGHTING, TRUE );
                break;

            case ShadeModeFlat:
            case ShadeModeGouraud:
                if (m_pmaterial) 
				{
					// Use material as source for colour, single light direction.
					// Angle of incidence + m_brightAmbient, multiplied
					// by the diffuse material colour only.
					// Alpha value taken straight from diffuse alpha setting.
					// (i + amb) * diff = ( i * diff ) + ( amb * diff ).
					// Set d3dmaterial diffuse and ambient colour to material diffuse.
					// Set the light diffuse to 1.0f, light ambient to m_brightambient.

					// Configure the material. Only interested in diffuse
					// and ambient settings for now.
					memset( &m_materialD3D, 0, sizeof( D3DMATERIAL9 ) );
					m_materialD3D.Diffuse.r = m_pmaterial->GetDiffuse().R();
					m_materialD3D.Diffuse.g = m_pmaterial->GetDiffuse().G();
					m_materialD3D.Diffuse.b = m_pmaterial->GetDiffuse().B();
					m_materialD3D.Diffuse.a = m_pmaterial->GetDiffuse().A();
					m_materialD3D.Ambient.r = m_pmaterial->GetDiffuse().R();
					m_materialD3D.Ambient.g = m_pmaterial->GetDiffuse().G();
					m_materialD3D.Ambient.b = m_pmaterial->GetDiffuse().B();
					m_materialD3D.Ambient.a = m_pmaterial->GetDiffuse().A();
					pDev->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
					pDev->SetRenderState( D3DRS_AMBIENT, 0 );
					pDev->SetMaterial( &m_materialD3D );

					// Configure the single light.
					m_vecLight = m_matLight.TransformDirection(m_vecLightWorld);
				    m_vecLight.SetNormalize();
					memset( &m_light0D3D, 0, sizeof(D3DLIGHT9) );

					m_light0D3D.Diffuse.r = 1.0f;
					m_light0D3D.Diffuse.g = 1.0f;
					m_light0D3D.Diffuse.b = 1.0f;
					m_light0D3D.Diffuse.a = 1.0f;
					m_light0D3D.Ambient.r = m_brightAmbient;
					m_light0D3D.Ambient.g = m_brightAmbient;
					m_light0D3D.Ambient.b = m_brightAmbient;
					m_light0D3D.Ambient.a = 1.0f;
					m_light0D3D.Direction.x = m_vecLight.x;
					m_light0D3D.Direction.y = m_vecLight.y;
					m_light0D3D.Direction.z = m_vecLight.z;
					m_light0D3D.Type = D3DLIGHT_DIRECTIONAL;

					pDev->SetLight( 0, &m_light0D3D );
					pDev->LightEnable( 0, TRUE );
					pDev->LightEnable( 1, FALSE );
					pDev->SetRenderState( D3DRS_LIGHTING, TRUE );
                } 
				else if (m_bBidirectional) 
				{
					// Use bidirectional lighting model. Angle of incidence * 
					// light colour, plus m_brightAmbient.

					// Set d3dmaterial diffuse to 1.0f, ambient to 1.0f
					// Set d3dlight diffuse to light colour, ambient to m_brightambient.
					// Set opposing directions.
					memset( &m_materialD3D, 0, sizeof( D3DMATERIAL9 ) );
					m_materialD3D.Diffuse.r = 1.0f;
					m_materialD3D.Diffuse.g = 1.0f;
					m_materialD3D.Diffuse.b = 1.0f;
					m_materialD3D.Diffuse.a = 1.0f;
					m_materialD3D.Ambient.r = 1.0f;
					m_materialD3D.Ambient.g = 1.0f;
					m_materialD3D.Ambient.b = 1.0f;
					m_materialD3D.Ambient.a = 1.0f;
					
					HRESULT hr;
					hr = pDev->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
					hr = pDev->SetRenderState( D3DRS_AMBIENT, 0 );
					hr = pDev->SetMaterial( &m_materialD3D );
					hr = pDev->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );

					// Configure both lights, light direction irrelevant.
					// Only set m_brightAmbient in one light, otherwise it gets
					// factored in for each light.
					m_vecLight = m_matLight.TransformDirection(m_vecLightWorld);
				    m_vecLight.SetNormalize();

					memset( &m_light0D3D, 0, sizeof(D3DLIGHT9) );
					memset( &m_light1D3D, 0, sizeof(D3DLIGHT9) );
					m_light0D3D.Diffuse.r = m_color.R();
					m_light0D3D.Diffuse.g = m_color.G();
					m_light0D3D.Diffuse.b = m_color.B();
					m_light0D3D.Diffuse.a = m_color.A();
					m_light0D3D.Ambient.r = m_brightAmbient;
					m_light0D3D.Ambient.g = m_brightAmbient;
					m_light0D3D.Ambient.b = m_brightAmbient;
					m_light0D3D.Ambient.a = 1.0f;
					m_light0D3D.Direction.x = m_vecLight.x;
					m_light0D3D.Direction.y = m_vecLight.y;
					m_light0D3D.Direction.z = m_vecLight.z;
					m_light0D3D.Type = D3DLIGHT_DIRECTIONAL;

					m_light1D3D.Diffuse.r = m_colorAlt.R();
					m_light1D3D.Diffuse.g = m_colorAlt.G();
					m_light1D3D.Diffuse.b = m_colorAlt.B();
					m_light1D3D.Diffuse.a = m_colorAlt.A();
					m_light1D3D.Ambient.r = 0.0f;
					m_light1D3D.Ambient.g = 0.0f;
					m_light1D3D.Ambient.b = 0.0f;
					m_light1D3D.Ambient.a = 1.0f;
					m_light1D3D.Direction.x = -m_vecLight.x;
					m_light1D3D.Direction.y = -m_vecLight.y;
					m_light1D3D.Direction.z = -m_vecLight.z;
					m_light1D3D.Type = D3DLIGHT_DIRECTIONAL;

					pDev->SetLight( 0, &m_light0D3D );
					pDev->SetLight( 1, &m_light1D3D );
					pDev->LightEnable( 0, TRUE );
					pDev->LightEnable( 1, TRUE );
					pDev->SetRenderState( D3DRS_LIGHTING, TRUE );
	            } 
				else 
				{
/*                    if (bClip) 
					{
						// Single light setting.
						// Uses the angle of incidence to generate a brightness 
						// value, not including light colour.
						// Set d3dmaterial diffuse to 1.0f, set d3dlight
						// diffuse to 1.0f. Everything else to zero.

						// Configure the material. Only interested in diffuse
						// settings for now.
						memset( &m_materialD3D, 0, sizeof( D3DMATERIAL9 ) );
						m_materialD3D.Diffuse.r = 1.0f;
						m_materialD3D.Diffuse.g = 1.0f;
						m_materialD3D.Diffuse.b = 1.0f;
						m_materialD3D.Diffuse.a = 1.0f;
						CD3DDevice9::SetRenderState( D3DRS_SPECULARENABLE, FALSE );
						CD3DDevice9::SetRenderState( D3DRS_AMBIENT, 0 );
						CD3DDevice9::SetMaterial( &m_materialD3D );

						// Configure the single light, light direction irrelevant.
						m_vecLight = m_matLight.TransformDirection(m_vecLightWorld);
					    m_vecLight.SetNormalize();
						memset( &m_light0D3D, 0, sizeof(D3DLIGHT9) );
						m_light0D3D.Diffuse.r = 1.0f;
						m_light0D3D.Diffuse.g = 1.0f;
						m_light0D3D.Diffuse.b = 1.0f;
						m_light0D3D.Diffuse.a = 1.0f;
						m_light0D3D.Direction.x = m_vecLight.x;
						m_light0D3D.Direction.y = m_vecLight.y;
						m_light0D3D.Direction.z = m_vecLight.z;
						m_light0D3D.Type = D3DLIGHT_DIRECTIONAL;

						CD3DDevice9::SetLight( 0, &m_light0D3D );
						CD3DDevice9::LightEnable( 0, TRUE );
						CD3DDevice9::LightEnable( 1, FALSE );
						CD3DDevice9::SetRenderState( D3DRS_LIGHTING, TRUE );
                    } 
					else */
					{
						// Single light setting. Use the angle of incidence to 
						// generate a brightness value, then adds on the 
						// m_brightAmbient value. This becomes the grey scale setting
						// for the object. Light colour not used.
						// Set d3dmaterial diffuse and ambient to 1.0f.
						// Set d3dlight diffuse to 1.0f, ambient to m_brightAmbient.

 						// Configure the material. Only interested in diffuse
						// and ambient settings for now.
						memset( &m_materialD3D, 0, sizeof( D3DMATERIAL9 ) );
						m_materialD3D.Diffuse.r = 1.0f;
						m_materialD3D.Diffuse.g = 1.0f;
						m_materialD3D.Diffuse.b = 1.0f;
						m_materialD3D.Diffuse.a = 1.0f;
						m_materialD3D.Ambient.r = 1.0f;
						m_materialD3D.Ambient.g = 1.0f;
						m_materialD3D.Ambient.b = 1.0f;
						m_materialD3D.Ambient.a = 1.0f;
						pDev->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
						pDev->SetRenderState( D3DRS_AMBIENT, 0 );
						pDev->SetMaterial( &m_materialD3D );

						// Configure the single light.
						m_vecLight = m_matLight.TransformDirection(m_vecLightWorld);
						m_vecLight.SetNormalize();
						memset( &m_light0D3D, 0, sizeof(D3DLIGHT9) );

						m_light0D3D.Diffuse.r = 1.0f;
						m_light0D3D.Diffuse.g = 1.0f;
						m_light0D3D.Diffuse.b = 1.0f;
						m_light0D3D.Diffuse.a = 1.0f;
						m_light0D3D.Ambient.r = m_brightAmbient;
						m_light0D3D.Ambient.g = m_brightAmbient;
						m_light0D3D.Ambient.b = m_brightAmbient;
						m_light0D3D.Ambient.a = 1.0f;
						m_light0D3D.Direction.x = m_vecLight.x;
						m_light0D3D.Direction.y = m_vecLight.y;
						m_light0D3D.Direction.z = m_vecLight.z;
						m_light0D3D.Type = D3DLIGHT_DIRECTIONAL;

						pDev->SetLight( 0, &m_light0D3D );
						pDev->LightEnable( 0, TRUE );
						pDev->LightEnable( 1, FALSE );
						pDev->SetRenderState( D3DRS_LIGHTING, TRUE );                   }
					}
                break;

            default:
                ZError("Invalid ShadeMode");
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Update Pointers
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdatePointers()
    {
        if (m_bUpdatePointers) {
            m_bUpdatePointers = false;

            bool bClip = m_bClip || m_countPlanes > 6;

            if (m_pdeform != NULL) {
                bClip = true;
                m_pfnTransform = &Device3D::TransformDeformClip;
            } else if (bClip) {
                m_pfnTransform = &Device3D::TransformClip;
            } else {
                m_pfnTransform = &Device3D::TransformNoClip;
            }

            m_pfnLightVertexL = m_pfnTransform;

            switch (m_shadeMode) {
                case ShadeModeNone:
                case ShadeModeCopy:
                    if (bClip) {
                        m_pfnLightVertex = &Device3D::LightVertexCopy;
                    } else {
                        m_pfnLightVertex = &Device3D::TransformNoClipNoLight;
                    }
                    break;

                case ShadeModeGlobalColor:
                    m_pfnLightVertex = &Device3D::LightVertexGlobalColor;
                    break;

                case ShadeModeFlat:
                case ShadeModeGouraud:
                    if (m_pmaterial) {
                        m_pfnLightVertex = &Device3D::MaterialLightVertex;
                    } else if (m_bBidirectional) {
                        m_pfnLightVertex = &Device3D::BidirectionalLightVertex;
                    } else {
                        if (bClip) {
                            m_pfnLightVertex = &Device3D::LightVertex;
                        } else {
                            m_pfnLightVertex = &Device3D::TransformAndLightNoClip;
                        }
                    }
                    break;

                default:
                    ZError("Invalid ShadeMode");
            }

            //
            // Lines
            //

            if (
                   (m_lineWidth == 0.5f)
                && (
                       m_prasterizer->Has3DAcceleration()
                    || (
                           (m_lineWidth == 0.5f)
                        && (!m_bZTest)
                        && (!m_bZWrite)
                        && (m_shadeMode == ShadeModeFlat)
                        && (m_blendMode == BlendModeSource)
                       )
                   )
            ) {
                m_pfnDrawLines = &Device3D::DrawLinesRasterizer;
            } else {
                m_pfnDrawLines = &Device3D::DrawLinesWithPolygons;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Get State
    //
    //////////////////////////////////////////////////////////////////////////////

    bool GetClipping()
    {
        return m_bClip;
    }

    const Matrix& GetMatrix()
    {
        return m_mat;
    }

    const Matrix& GetInverseModelMatrix()
    {
        if (m_bUpdateInverse) {
            m_bUpdateInverse = false;
            m_matInverse.SetInverse(m_mat);
        }

        return m_matInverse;
    }

    const Matrix& GetInverseModelWorldMatrix()
    {
		if (m_bUpdateInverseWorld) {
            m_bUpdateInverseWorld = false;
			m_matInverseWorld.SetInverse(m_matWorldTM);
        }

        return m_matInverseWorld;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Set State
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetMatrix(const Matrix& mat, const Matrix& matWorldTM )
    {
        m_mat             = mat;
		m_matWorldTM      = matWorldTM;
        m_bUpdateMatFull  = true;
        m_bUpdateLighting = true;
        m_bUpdateInverse  = true;
		m_bUpdateInverseWorld = true;

        GenerateWorldTransform();
        GenerateViewTransform();
    }

    void SetPerspectiveMatrix(const Matrix& mat)
    {
        m_matPerspective = mat;
        m_bUpdateMatFull = true;

		GenerateProjectionTransform();
    }

    void SetViewMatrix(const Matrix& mat)
    {
        m_matView = mat;
        m_bUpdateMatFull = true;

		GenerateViewTransform();
    }

    void SetClipping(bool bClip)
    {
        m_bClip = bClip;
        m_bUpdateMatFull  = true;
        m_bUpdatePointers = true;
    }

    void RemoveClipPlane(int indexRemove)
    {
        ZAssert(indexRemove >= 0 && indexRemove + 6 < m_countPlanes);

        for(int index = m_countPlanes - indexRemove - 1; index < m_countPlanes - 1; index++) {
            m_pplane[index] = m_pplane[index + 1];
        }

        m_countPlanes--;
    }

    void AddClipPlane(const Plane& plane)
    {
        ZAssert(m_countPlanes < g_maxCountPlanes);

        //
        // transform the plane into eye coordinates
        //

        const Matrix& mat = GetFullMatrix();
        Matrix mati;

        mati.SetInverse(mat);
        mati.Transpose();

        HVector p  = plane.GetHVector();
        HVector pp = mati.Transform(p);

        m_pplane[m_countPlanes] = Plane(pp);

        m_countPlanes++;
    }

    void SetShadeMode(ShadeMode shadeMode)
    {
        m_shadeMode = shadeMode;
        m_bUpdatePointers = true;
		m_bUpdateLighting = true;

        m_prasterizer->SetShadeMode(shadeMode);
    }

    void SetMaterial(Material* pmaterial)
    {
        m_pmaterial = pmaterial;
		m_bUpdateLighting = true;
    }

    void SetBlendMode(BlendMode blendMode)
    {
        m_blendMode = blendMode;
        m_bUpdatePointers = true;
        m_prasterizer->SetBlendMode(blendMode);
    }

    void SetTexture(Surface* psurfaceTexture)
    {
        m_prasterizer->SetTexture(psurfaceTexture);
    }

    void SetWrapMode(WrapMode wrapMode)
    {
        m_prasterizer->SetWrapMode(wrapMode);
    }

    void SetCullMode(CullMode cullMode)
    {
        m_prasterizer->SetCullMode(cullMode);
    }

    void SetZTest(bool bZTest)
    {
        m_bZTest = bZTest;
        m_bUpdatePointers = true;
        m_prasterizer->SetZTest(bZTest);
    }

    void SetZWrite(bool bZWrite)
    {
        m_bZWrite = bZWrite;
        m_bUpdatePointers = true;
        m_prasterizer->SetZWrite(bZWrite);
    }

    void SetLinearFilter(bool bLinearFilter)
    {
        m_prasterizer->SetLinearFilter(bLinearFilter);
    }

    void SetPerspectiveCorrection(bool bPerspectiveCorrection)
    {
        m_prasterizer->SetPerspectiveCorrection(bPerspectiveCorrection);
    }

    void SetDither(bool bDither)
    {
        m_prasterizer->SetDither(bDither);
    }

    void SetColorKey(bool bColorKey)
    {
        m_prasterizer->SetColorKey(bColorKey);
    }

    void SetLineWidth(float width)
    {
        m_lineWidth = 0.5f * width;
    }

    void SetDeformation(Deformation* pdeform)
    {
        m_pdeform = pdeform;
        m_bUpdatePointers = true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Buffers
    //
    //////////////////////////////////////////////////////////////////////////////

    //
    //  , may want to grow to more than count
    //

    void SizeVertexLBuffer(int count)
    {
        if (m_countVertexLBuffer < count) {
            m_countVertexLBuffer = count;
            m_pvertexLBuffer = (VertexL*)realloc(m_pvertexLBuffer, sizeof(VertexL) * count);
        }
    }

    void SizeVertexTLBuffer(int count)
    {
        if (m_countVertexTLBuffer < count) {
            m_countVertexTLBuffer = count;
            m_pvertexTLBuffer = (VertexTL*)realloc(m_pvertexTLBuffer, sizeof(VertexTL) * count);
        }
    }

    void SizeVertexScreenBuffer(int count)
    {
        if (m_countVertexScreenBuffer < count) {
            m_pvertexScreenBuffer = (VertexScreen*)realloc(m_pvertexScreenBuffer, sizeof(VertexScreen) * count);
            for (int index = m_countVertexScreenBuffer; index < count; index++) {
                m_pvertexScreenBuffer[index].specular = 0;
            }
            m_countVertexScreenBuffer = count;
        }
    }

    void SizeIndexBuffer(int count)
    {
        if (m_countIndexBuffer < count) {
            m_countIndexBuffer = count;
            m_pindexBuffer = (MeshIndex*)realloc(m_pindexBuffer, sizeof(MeshIndex) * count);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Vertex Buffers
    //
    //////////////////////////////////////////////////////////////////////////////

    VertexL* GetVertexLBuffer(int count)
    {
        SizeVertexLBuffer(count);
        return m_pvertexLBuffer;
    }

    VertexScreen* GetVertexScreenBuffer(int count)
    {
        SizeVertexScreenBuffer(count);
        return m_pvertexScreenBuffer;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void StoreTriangle(MeshIndex index1, MeshIndex index2, MeshIndex index3)
    {
        SizeIndexBuffer(m_indexIndexBuffer + 3);

        m_pindexBuffer[m_indexIndexBuffer    ] = index1;
        m_pindexBuffer[m_indexIndexBuffer + 1] = index2;
        m_pindexBuffer[m_indexIndexBuffer + 2] = index3;

        m_indexIndexBuffer += 3;
    }

    void StoreLine(MeshIndex index1, MeshIndex index2)
    {
        SizeIndexBuffer(m_indexIndexBuffer + 2);

        m_pindexBuffer[m_indexIndexBuffer    ] = index1;
        m_pindexBuffer[m_indexIndexBuffer + 1] = index2;

        m_indexIndexBuffer += 2;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClampVertex(VertexScreen& vertex)
    {
        if (vertex.x < m_rectClipScreen.XMin()) {
            vertex.x = m_rectClipScreen.XMin();
        } else if (vertex.x > m_rectClipScreen.XMax()) {
            vertex.x = m_rectClipScreen.XMax();
        }

        if (vertex.y < m_rectClipScreen.YMin()) {
            vertex.y = m_rectClipScreen.YMin();
        } else if (vertex.y > m_rectClipScreen.YMax()) {
            vertex.y = m_rectClipScreen.YMax();
        }

        if (vertex.z < 0) {
            vertex.z = 0;
        } else if (vertex.z > 1) {
            vertex.z = 1;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    MeshIndex ConvertVertex(VertexTL& vertex)
    {
        SizeVertexScreenBuffer(m_indexVertexScreenBuffer + 1);

        vertex.m_index = m_indexVertexScreenBuffer;
        VertexScreen* pvertex = m_pvertexScreenBuffer + m_indexVertexScreenBuffer;

        float rhw  = 1.0f / vertex.m_pos.w;

        pvertex->x        = m_scalex * rhw * vertex.m_pos.x + m_xoffset;
        pvertex->y        = m_scaley * rhw * vertex.m_pos.y + m_yoffset;
        pvertex->z        = rhw * vertex.m_pos.z;
        pvertex->qw       = rhw;
        pvertex->u        = vertex.m_pointTexture.X();
        pvertex->v        = vertex.m_pointTexture.Y();
        pvertex->color    = MakeD3DCOLOR(vertex.m_color);
        pvertex->specular = 0;

        ClampVertex(*pvertex);

        m_indexVertexScreenBuffer++;

        return vertex.m_index;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    MeshIndex TranslateIndex(VertexTL& vertex)
    {
        if (vertex.m_index == 0xffff) {
            return ConvertVertex(vertex);
        }

        return vertex.m_index;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    int TranslateIntersection(
        const Plane&    plane,
        const VertexTL& vertex0, 
        const VertexTL& vertex1 
    ) {
        VertexTL vertex;

        CalculateIntersection(plane, vertex0, vertex1, vertex);
        return ConvertVertex(vertex);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Plane
    //
    //////////////////////////////////////////////////////////////////////////////

    void CalculateIntersection(
        const Plane&     plane,
        const VertexTL&  vertex0,
        const VertexTL&  vertex1,
              VertexTL&  vertexOut
    ) {
        float alpha = plane.Intersect(vertex0.m_pos, vertex1.m_pos);

        vertexOut.Interpolate(vertex0, vertex1, alpha);
        vertexOut.m_index = 0xffff;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clip a polygon to a plane
    //
    //////////////////////////////////////////////////////////////////////////////

    int ClipPolygonToPlane(
        const Plane&     plane,
              Code       bit,
              VertexTL*  pvertexIn,
              int        vcountIn,
              VertexTL*  pvertexOut
    ) {
        int  vcount         = 0;
        int  vindexPrevious = vcountIn - 1;
        Code codePrevious   = pvertexIn[vindexPrevious].m_code & bit;

        for (int vindex = 0; vindex < vcountIn; vindex++) {
            Code code = pvertexIn[vindex].m_code & bit;

            if ((code ^ codePrevious) != 0) {
                CalculateIntersection(
                    plane,
                    pvertexIn[vindexPrevious],
                    pvertexIn[vindex],
                    pvertexOut[vcount]
                );

                pvertexOut[vcount].UpdateClipCode();
                if (m_countPlanes > 6) {
                    Code code = ClipPlane;

                    for (int index = 6; index < m_countPlanes; index++, code <<= 1) {
                        pvertexOut[vcount].UpdateClipCode(m_pplane[index], code);
                    }
                }
                vcount++;
            }

            if (code == 0) {
                pvertexOut[vcount] = pvertexIn[vindex];
                vcount++;
            }

            ZAssert(vcount <= 8);

            codePrevious = code;
            vindexPrevious = vindex;
        }

        return vcount;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clip a polygon to a set of planess
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClipPolygonToPlanes(
        const Plane* pplane,
              int        pcount,
              Code       codeOr,
        const VertexTL&  vertex0,
        const VertexTL&  vertex1,
        const VertexTL&  vertex2
    ) {
        VertexTL pverticesIn[3] = 
            { 
                vertex0, 
                vertex1, 
                vertex2 
            };
        VertexTL pverticesOut0[8];
        VertexTL pverticesOut1[8];

        VertexTL* pvertexIn    = pverticesIn;
        VertexTL* pvertexOut   = pverticesOut0;
        VertexTL* pvertexOther = pverticesOut1;

        int  vcount = 3;
        Code bit    = 1;
        int  pindex = 0;

        while (pindex < pcount) {
            if ((codeOr & bit) != 0) {
                vcount = ClipPolygonToPlane(pplane[pindex], bit, pvertexIn, vcount, pvertexOut);

                pvertexIn    = pvertexOut;
                pvertexOut   = pvertexOther;
                pvertexOther = pvertexIn;
            }

            bit *= 2;
            pindex++;
        }

        //
        // Translate the vertices to screen coordinates
        //

        MeshIndex aindex[8];
        int index;

        for(index = 0; index < vcount; index++) {
            aindex[index] = TranslateIndex(pvertexIn[index]);
        }

        //
        // Form all of the triangle fans
        //

        //Additional guard to make sure we don't exceed array bounds in the next loop
        if (vcount > 8) {
            vcount = 8;
        }

        for(index = 1; index < vcount - 1; index++) {
            StoreTriangle(
                aindex[0],
                aindex[index],
                aindex[index + 1]
            );
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClipPolygonToPlane(
        const Plane&     plane,
              Code       code0,
              Code       code1,
              Code       code2,
              VertexTL&  vertex0,
              VertexTL&  vertex1,
              VertexTL&  vertex2
    ) {
        if (code0 != 0) {
            if (code1 != 0) {
                // p0 and p1 outside

                StoreTriangle(
                    TranslateIntersection(plane, vertex2, vertex0),
                    TranslateIntersection(plane, vertex1, vertex2),
                    TranslateIndex(vertex2)
                );
            } else if (code2 != 0) {
                // p0 and p2 outside

                StoreTriangle(
                    TranslateIntersection(plane, vertex0, vertex1),
                    TranslateIndex(vertex1),
                    TranslateIntersection(plane, vertex2, vertex1)
                );
            } else {
                // p0 outside

                int index0 = TranslateIntersection(plane, vertex0, vertex1);
                int index2 = TranslateIndex(vertex2);

                StoreTriangle(
                    index0,
                    TranslateIndex(vertex1),
                    index2
                );

                StoreTriangle(
                    index0,
                    index2,
                    TranslateIntersection(plane, vertex0, vertex2)
                );
            }
        } else {
            if (code1 != 0) {
                if (code2 != 0) {
                    // p1 and p2 outside

                    StoreTriangle(
                        TranslateIndex(vertex0),
                        TranslateIntersection(plane, vertex0, vertex1),
                        TranslateIntersection(plane, vertex2, vertex0)
                    );
                } else {
                    // p1 outside

                    int index0 = TranslateIndex(vertex0);
                    int index2 = TranslateIntersection(plane, vertex1, vertex2);

                    StoreTriangle(
                        index0,
                        TranslateIntersection(plane, vertex0, vertex1),
                        index2
                    );

                    StoreTriangle(
                        index0,
                        index2,
                        TranslateIndex(vertex2)
                    );
                }
            } else {
                // p2 outside

                int index0 = TranslateIndex(vertex0);
                int index2 = TranslateIntersection(plane, vertex1, vertex2);

                StoreTriangle(
                    index0,
                    TranslateIndex(vertex1),
                    index2
                );

                StoreTriangle(
                    index0,
                    index2,
                    TranslateIntersection(plane, vertex2, vertex0)
                );
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clip Triangles
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClipTriangles(VertexTL* pvertex, int vcountIn, const MeshIndex* pindex, int icountIn)
    {
        //
        // Clip each triangle
        //

        for(int index = 0; index < icountIn; index += 3) {
            int index0 = pindex[index + 0];
            int index1 = pindex[index + 1];
            int index2 = pindex[index + 2];

            Code code0 = pvertex[index0].m_code;
            Code code1 = pvertex[index1].m_code;
            Code code2 = pvertex[index2].m_code;

            Code codeAnd = code0 & code1 & code2;

            if (codeAnd == 0) {
                Code codeOr  = code0 | code1 | code2;

                if (codeOr == 0) {
                    //
                    // Not clipped
                    //

                    StoreTriangle(
                        TranslateIndex(pvertex[index0]),
                        TranslateIndex(pvertex[index1]),
                        TranslateIndex(pvertex[index2])
                    );
                } else {
                    //
                    // Partially clipped
                    //

                    Code bit = 1;
                    int pindex = 0;

                    while (
                           (pindex < m_countPlanes)
                        && (codeOr != bit)
                    ) {
                        pindex++;
                        bit *= 2;
                    }

                    if (pindex < m_countPlanes) {
                        //
                        // Clipped by a single plane
                        //

                        ClipPolygonToPlane(
                            m_pplane[pindex],
                            code0,
                            code1,
                            code2,
                            pvertex[index0],
                            pvertex[index1],
                            pvertex[index2]
                        );
                    } else {
                        //
                        // Clipped by multiple planes
                        //

                        ClipPolygonToPlanes(
                            m_pplane,
                            m_countPlanes,
                            codeOr,
                            pvertex[index0],
                            pvertex[index1],
                            pvertex[index2]
                        );
                    }
                }
            } else {
                //
                // Completely clipped
                //

                int i = 0;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clip a line to a set of planess
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClipLineToPlanes(
        const Plane* pplane,
              int        pcount,
              Code       codeOr,
        const VertexTL&  vertex0,
        const VertexTL&  vertex1
    ) {
        float t0 = 0;
        float t1 = 1;

        int  vcount = 3;
        Code bit    = 1;
        int  pindex = 0;

        while (pindex < pcount) {
            if ((codeOr & bit) != 0) {
                float t = pplane[pindex].Intersect(vertex0.m_pos, vertex1.m_pos);

                if ((vertex0.m_code & bit) != 0) {
                    if (t > t0) {
                        t0 = t;
                    }
                } else {
                    if (t < t1) {
                        t1 = t;
                    }
                }
            }

            bit *= 2;
            pindex++;
        }

        if (t0 < t1) {
            VertexTL vertexT0; vertexT0.Interpolate(vertex0, vertex1, t0);
            VertexTL vertexT1; vertexT1.Interpolate(vertex0, vertex1, t1);

            StoreLine(
                ConvertVertex(vertexT0),
                ConvertVertex(vertexT1)
            );
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clip Lines
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClipLines(VertexTL* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        for (int index = 0; index < icount; index += 2) {
            int index0 = pindex[index + 0];
            int index1 = pindex[index + 1];

            Code code0 = pvertex[index0].m_code;
            Code code1 = pvertex[index1].m_code;

            Code codeAnd = code0 & code1;

            if (codeAnd == 0) {
                Code codeOr  = code0 | code1;

                if (codeOr == 0) {
                    //
                    // Not clipped
                    //

                    StoreLine(
                        TranslateIndex(pvertex[index0]),
                        TranslateIndex(pvertex[index1])
                    );
                } else {
                    //
                    // Clipped
                    //

                    ClipLineToPlanes(
                        m_pplane,
                        m_countPlanes,
                        codeOr,
                        pvertex[index0],
                        pvertex[index1]
                    );
                }
            } else {
                //
                // Completely clipped
                //
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Clip Points
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClipPoints(VertexTL* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        for(int index = 0; index < vcount; index++) {
            if (pvertex[index].m_code == 0) {
                ConvertVertex(pvertex[index]);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Transform with deformation and clipping
    //
    //////////////////////////////////////////////////////////////////////////////

    void TransformDeformClip(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        SizeVertexTLBuffer(vcount);

        Matrix mat = m_mat;
        m_mat.SetIdentity();
        m_bUpdateMatFull = true;
        const Matrix& matFull = GetFullMatrix();

        for (int index = 0; index < vcount; index++) {
            Vector vecLocal = Vector(pvertex[index].x, pvertex[index].y, pvertex[index].z);

            HVector hvec(
                m_pdeform->Deform(
                    mat.Transform(
                        vecLocal
                    )
                )
            );

            m_pvertexTLBuffer[index].m_pos          = matFull.Transform(hvec);
            m_pvertexTLBuffer[index].m_pointTexture = Point(pvertex[index].u, pvertex[index].v);
            m_pvertexTLBuffer[index].m_color        = 
                Color(
                    pvertex[index].r,
                    pvertex[index].g,
                    pvertex[index].b,
                    pvertex[index].a
                );
            m_pvertexTLBuffer[index].m_index = 0xffff;

            m_pvertexTLBuffer[index].UpdateClipCode();
        }

        if (m_countPlanes > 6) {
            for (int index = 0; index < vcount; index++) {
                Code code = ClipPlane;

                for (int iplane = 6; iplane < m_countPlanes; iplane++, code <<= 1) {
                    m_pvertexTLBuffer[index].UpdateClipCode(m_pplane[iplane], code);
                }
            }
        }

        m_indexVertexScreenBuffer = 0;
        m_indexIndexBuffer = 0;

        (this->*pfnClip)(m_pvertexTLBuffer, vcount, pindex, icount);

        if (m_indexVertexScreenBuffer != 0) {
            (this->*pfnDraw)(
                m_pvertexScreenBuffer, 
                m_indexVertexScreenBuffer, 
                m_pindexBuffer, 
                m_indexIndexBuffer
            );
        }

        m_mat = mat;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Transform with clipping
    //
    //////////////////////////////////////////////////////////////////////////////

    void TransformClip(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        SizeVertexTLBuffer(vcount);

        const Matrix& mat = GetFullMatrix();

        for (int index = 0; index < vcount; index++) {
            HVector hvec(
                pvertex[index].x,
                pvertex[index].y,
                pvertex[index].z,
                1
            );

            m_pvertexTLBuffer[index].m_pos          = mat.Transform(hvec);
            m_pvertexTLBuffer[index].m_pointTexture = Point(pvertex[index].u, pvertex[index].v);
            m_pvertexTLBuffer[index].m_color        = 
                Color(
                    pvertex[index].r,
                    pvertex[index].g,
                    pvertex[index].b,
                    pvertex[index].a
                );
            m_pvertexTLBuffer[index].m_index = 0xffff;

            m_pvertexTLBuffer[index].UpdateClipCode();
        }

        if (m_countPlanes > 6) {
            for (int index = 0; index < vcount; index++) {
                Code code = ClipPlane;

                for (int iplane = 6; iplane < m_countPlanes; iplane++, code <<= 1) {
                    m_pvertexTLBuffer[index].UpdateClipCode(m_pplane[iplane], code);
                }
            }
        }

        m_indexVertexScreenBuffer = 0;
        m_indexIndexBuffer = 0;

        (this->*pfnClip)(m_pvertexTLBuffer, vcount, pindex, icount);

        if (m_indexVertexScreenBuffer != 0) {
            (this->*pfnDraw)(
                m_pvertexScreenBuffer, 
                m_indexVertexScreenBuffer, 
                m_pindexBuffer, 
                m_indexIndexBuffer
            );
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Special case Transform and light without clipping
    //
    //////////////////////////////////////////////////////////////////////////////

    #ifndef USEASM
        void TransformAndLightNoClip(
            const Vertex* pvertex, 
            int vcount, 
            const MeshIndex* pindex, 
            int icount, 
            PFNClip pfnClip, 
            PFNDraw pfnDraw
        ) {
            SizeVertexScreenBuffer(vcount);

            UpdateLighting();
            const Matrix& mat = GetFullMatrix();

            for (int index = 0; index < vcount; index++) {
                const float x = pvertex[index].x;
                const float y = pvertex[index].y;
                const float z = pvertex[index].z;

                const float rhw = 1.0f / (mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3]);

                m_pvertexScreenBuffer[index].x  = rhw * (mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3]);
                m_pvertexScreenBuffer[index].y  = rhw * (mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3]);
                m_pvertexScreenBuffer[index].z  = rhw * (mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3]);
                m_pvertexScreenBuffer[index].qw = rhw;
                m_pvertexScreenBuffer[index].u  = pvertex[index].u;
                m_pvertexScreenBuffer[index].v  = pvertex[index].v;

                {
                    float fbright =
                        255.0f * (
                              pvertex[index].nx * m_vecLight.X()
                            + pvertex[index].ny * m_vecLight.Y()
                            + pvertex[index].nz * m_vecLight.Z()
                            + m_brightAmbient
                        );

                    int bright = MakeInt(fbright); //imago 6/20/09 fix typo 

                    if (bright > 255) {
                        bright = 255;
                    } else if (bright < 0) {
                        bright = 0;
                    }

                    m_pvertexScreenBuffer[index].color = 0xff000000 | bright | (bright << 8) | (bright << 16);
                }
            }

            (this->*pfnDraw)(m_pvertexScreenBuffer, vcount, pindex, icount);
        }
    #else
        void TransformAndLightNoClip(
            const Vertex* pvertex, 
            int vcount, 
            const MeshIndex* pindex, 
            int icount, 
            PFNClip pfnClip, 
            PFNDraw pfnDraw
        ) {
            UpdateLighting();
            SizeVertexScreenBuffer(vcount);

            const Matrix mat = GetFullMatrix();
            const Vector vecLight = m_vecLight;
            const float brightAmbient = m_brightAmbient;
            const float mult = 255.0f;
            const float one  = 1.0f;
            int   bright;

            _asm {
                mov     ebx,    255
                mov     ecx,    this
                mov     esi,    pvertex
                mov     edi,    [ecx].m_pvertexScreenBuffer
                mov     ecx,    vcount

               loopTop:
                // int bright = (int)(255.0f * (pvertex[index].GetNormal() * m_vecLight + m_brightAmbient));
                // const float rhw = 1.0f / (mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3]);
                // m_pvertexScreenBuffer[index].SetX(rhw * (mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3]);
                // m_pvertexScreenBuffer[index].SetY(rhw * (mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3]);
                // m_pvertexScreenBuffer[index].SetZ(rhw * (mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3]));
                // m_pvertexScreenBuffer[index].SetTextureCoordinate(pvertex[index].GetTextureCoordinate());

                fld     [vecLight].x                //
                fmul    [esi]Vertex.nx              // lx
                fld     [vecLight].y                //
                fmul    [esi]Vertex.ny              // ly, lx
                fld     [vecLight].z                //
                fmul    [esi]Vertex.nz              // lz, ly, lx
                fxch    st(2)                       // lx, ly, lz
                faddp   st(1), st(0)                // lx + ly, lz
                fld     one                         // 1, lx + ly, lz
                fld     [mat + 48 +  0]             //
                fmul    [esi]Vertex.x               // wx, 1, lx + ly, lz
                fxch    st(3)                       // lz, 1, lx + ly, wx
                fadd    [brightAmbient]             // lz + dl, 1, lx + ly, wx
                fld     [mat + 48 +  4]             //
                fmul    [esi]Vertex.y               // wy, lz + dl, 1, lx + ly, wx
                fxch    st(1)                       // lz + dl, wy, 1, lx + ly, wx
                faddp   st(3), st(0)                // wy, 1, l, wx
                fld     [mat + 48 +  8]             //
                fmul    [esi]Vertex.z               // wz, wy, 1, l, wx
                fxch    st(1)                       // wy, wz, 1, l, wx
                faddp   st(4), st(0)                // wz, 1, l, wx + wy
                fxch    st(2)                       // l, 1, wz, wx + wy
                fmul    mult                        // 256 * l, 1, wz, wx + wy
                fld     [mat +  0 +  0]             //
                fmul    [esi]Vertex.x               // xx, 255 * l, 1, wz, wx + wy
                fxch    st(3)                       // wz, 255 * l, 1, xx, wx + wy
                fadd    DWORD PTR [mat + 48 + 12]   // wz + dw, 255 * l, 1, xx, wx + wy
                fxch    st(1)                       // 255 * l, wz + dw, 1, xx, wx + wy
                fistp   bright                      // wz + dw, 1, xx, wx + wy
                fld     [mat +  0 +  4]             //
                fmul    [esi]Vertex.y               // xy, wz + dw, 1, xx, wx + wy
                fxch    st(1)                       // wz + dw, xy, 1, xx, wx + wy
                faddp   st(4), st(0)                // xy, 1, xx, w
                fld     [mat +  0 +  8]             //
                fmul    [esi]Vertex.z               // xz, xy, 1, xx, w
                fxch    st(1)                       // xy, xz, 1, xx, w
                faddp   st(3), st(0)                // xz, 1, xx + xy, w
                fxch    st(3)                       // w, 1, xx + xy, xz
                fdivp   st(1), st(0)                // rhw, xx + xy, xz
                fxch    st(2)                       // xz, xx + xy, rhw

                // if (bright > 255) {
                //     bright = 255;
                // } else if (bright < 0) {
                //     bright = 0;
                // }

                mov     eax,bright
                cmp     eax,ebx
                jle     label1
                mov     eax,ebx
                jmp     label2
              label1:
                test    eax,eax
                jge     label2
                xor     eax,eax

                // m_pvertexScreenBuffer[index].color = 0xff000000 | bright | (bright << 8) | (bright << 16);

              label2:
                mov     edx,eax
                shl     edx,8
                or      edx,eax
                shl     edx,8
                or      edx,eax
                or      edx, 0xff000000
                mov     [edi]VertexScreen.color, edx

                mov     eax, [esi]Vertex.u
                mov     edx, [esi]Vertex.v
                mov     [edi]VertexScreen.u, eax
                mov     [edi]VertexScreen.v, edx

                // continue with floating point stuff

                                                    // xz, xx + xy, rhw
                fadd    DWORD PTR [mat +  0 + 12]   // xz + dx, xx + xy, rhw
                fld     [mat + 16 +  0]             //
                fmul    [esi]Vertex.x               // yx, xz + dx, xx + xy, rhw
                fld     [mat + 16 +  4]             //
                fmul    [esi]Vertex.y               // yy, yx, xz + dx, xx + xy, rhw
                fld     [mat + 16 +  8]             //
                fmul    [esi]Vertex.z               // yz, yy, yx, xz + dx, xx + xy, rhw
                fxch    st(1)                       // yy, yz, yx, xz + dx, xx + xy, rhw
                faddp   st(2), st(0)                // yz, yx + yy, xz + dx, xx + xy, rhw
                fld     [mat + 32 +  0]             //
                fmul    [esi]Vertex.x               // zx, yz, yx + yy, xz + dx, xx + xy, rhw
                fld     [mat + 32 +  4]             //
                fmul    [esi]Vertex.y               // zy, zx, yz, yx + yy, xz + dx, xx + xy, rhw
                fld     [mat + 32 +  8]             //
                fmul    [esi]Vertex.z               // zz, zy, zx, yz, yx + yy, xz + dx, xx + xy, rhw
                fxch    st(3)                       // yz, zy, zx, zz, yx + yy, xz + dx, xx + xy, rhw
                fadd    DWORD PTR [mat + 16 + 12]   // yz + dy, zy, zx, zz, yx + yy, xz + dx, xx + xy, rhw
                fxch    st(1)                       // zy, yz + dy, zx, zz, yx + yy, xz + dx, xx + xy, rhw
                faddp   st(2), st(0)                // yz + dy, zx + zy, zz, yx + yy, xz + dx, xx + xy, rhw
                fxch    st(2)                       // zz, zx + zy, yz + dy, yx + yy, xz + dx, xx + xy, rhw
                fadd    DWORD PTR [mat + 32 + 12]   // zz + dz, zx + zy, yz + dy, yx + yy, xz + dx, xx + xy, rhw
                fxch    st(4)                       // xz + dx, zx + zy, yz + dy, yx + yy, zz + dz, xx + xy, rhw
                faddp   st(5), st(0)                // zx + zy, yz + dy, yx + yy, zz + dz, x, rhw
                fxch    st(1)                       // yz + dy, zx + zy, yx + yy, zz + dz, x, rhw
                faddp   st(2), st(0)                // zx + zy, y, zz + dz, x, rhw
                faddp   st(2), st(0)                // y, z, x, rhw
                fxch    st(2)                       // x, z, y, rhw
                fmul    st(0), st(3)                // rhw*x, z, y, rhw
                fxch    st(2)                       // y, z, rhw*x, rhw
                fmul    st(0), st(3)                // rhw*y, z, rhw*x, rhw
                fxch    st(1)                       // z, rhw*y, rhw*x, rhw
                fmul    st(0), st(3)                // rhw*z, rhw*y, rhw*x, rhw
                fxch    st(3)                       // rhw, rhw*y, rhw*x, rhw*z
                fstp    [edi]VertexScreen.qw        // rhw*y, rhw*x, rhw*z
                fxch    st(1)                       // rhw*x, rhw*y, rhw*z
                fstp    [edi]VertexScreen.x         // rhw*y, rhw*z
                fstp    [edi]VertexScreen.y         // rhw*z
                fstp    [edi]VertexScreen.z         //

                // loop

                add     edi, SIZE VertexScreen
                add     esi, SIZE Vertex
                dec     ecx
                jne     loopTop
            }

            (this->*pfnDraw)(m_pvertexScreenBuffer, vcount, pindex, icount);
        }
    #endif

    //////////////////////////////////////////////////////////////////////////////
    //
    // Tranform without clipping
    //
    //////////////////////////////////////////////////////////////////////////////

    void TransformNoClip(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        SizeVertexScreenBuffer(vcount);

        const Matrix& mat = GetFullMatrix();

        for (int index = 0; index < vcount; index++) {
            const float x = pvertex[index].x;
            const float y = pvertex[index].y;
            const float z = pvertex[index].z;

            const float rhw = 1.0f / (mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3]);

            m_pvertexScreenBuffer[index].x  = rhw * (mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3]);
            m_pvertexScreenBuffer[index].y  = rhw * (mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3]);
            m_pvertexScreenBuffer[index].z  = rhw * (mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3]);
            m_pvertexScreenBuffer[index].qw = rhw;
            m_pvertexScreenBuffer[index].u  = pvertex[index].u;
            m_pvertexScreenBuffer[index].v  = pvertex[index].v;

            m_pvertexScreenBuffer[index].color = 
                MakeD3DCOLOR(
                    Color(
                        pvertex[index].r,
                        pvertex[index].g,
                        pvertex[index].b,
                        pvertex[index].a
                    )
                );
        }

        (this->*pfnDraw)(m_pvertexScreenBuffer, vcount, pindex, icount);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Tranform without clipping or lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void TransformNoClipNoLight(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        SizeVertexScreenBuffer(vcount);

        const Matrix& mat = GetFullMatrix();

        for (int index = 0; index < vcount; index++) {
            const float x = pvertex[index].x;
            const float y = pvertex[index].y;
            const float z = pvertex[index].z;

            const float rhw = 1.0f / (mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3]);

            m_pvertexScreenBuffer[index].x  = rhw * (mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3]);
            m_pvertexScreenBuffer[index].y  = rhw * (mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3]);
            m_pvertexScreenBuffer[index].z  = rhw * (mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3]);
            m_pvertexScreenBuffer[index].qw = rhw;
            m_pvertexScreenBuffer[index].u  = pvertex[index].u;
            m_pvertexScreenBuffer[index].v  = pvertex[index].v;
        }

        (this->*pfnDraw)(m_pvertexScreenBuffer, vcount, pindex, icount);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Copy Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void LightVertexCopy(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        SizeVertexLBuffer(vcount);

        for (int index = 0; index < vcount; index++) {
            m_pvertexLBuffer[index].x = pvertex[index].x;
            m_pvertexLBuffer[index].y = pvertex[index].y;
            m_pvertexLBuffer[index].z = pvertex[index].z;
            m_pvertexLBuffer[index].u = pvertex[index].u;
            m_pvertexLBuffer[index].v = pvertex[index].v;
            m_pvertexLBuffer[index].r = 1;
            m_pvertexLBuffer[index].g = 1;
            m_pvertexLBuffer[index].b = 1;
            m_pvertexLBuffer[index].a = 1;
        }

        (this->*m_pfnTransform)(m_pvertexLBuffer, vcount, pindex, icount, pfnClip, pfnDraw);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Copy Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void LightVertexGlobalColor(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        SizeVertexLBuffer(vcount);

        float r = m_colorGlobal.R();
        float g = m_colorGlobal.G();
        float b = m_colorGlobal.B();
        float a = m_colorGlobal.A();

        for (int index = 0; index < vcount; index++) {
            m_pvertexLBuffer[index].x = pvertex[index].x;
            m_pvertexLBuffer[index].y = pvertex[index].y;
            m_pvertexLBuffer[index].z = pvertex[index].z;
            m_pvertexLBuffer[index].u = pvertex[index].u;
            m_pvertexLBuffer[index].v = pvertex[index].v;
            m_pvertexLBuffer[index].r = r;
            m_pvertexLBuffer[index].g = g;
            m_pvertexLBuffer[index].b = b;
            m_pvertexLBuffer[index].a = a;
        }

        (this->*m_pfnTransform)(m_pvertexLBuffer, vcount, pindex, icount, pfnClip, pfnDraw);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Bi-Directional Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    float Saturate(float value)
    {
        if (value > 1) {
            return 1;
        } else if (value < 0) {
            return 0;
        }

        return value;
    }

    void BidirectionalLightVertex(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        UpdateLighting();

        SizeVertexLBuffer(vcount);

        for (int index = 0; index < vcount; index++) {
            m_pvertexLBuffer[index].x = pvertex[index].x;
            m_pvertexLBuffer[index].y = pvertex[index].y;
            m_pvertexLBuffer[index].z = pvertex[index].z;
            m_pvertexLBuffer[index].u = pvertex[index].u;
            m_pvertexLBuffer[index].v = pvertex[index].v;

            float bright =
                  pvertex[index].nx * m_vecLight.X()
                + pvertex[index].ny * m_vecLight.Y()
                + pvertex[index].nz * m_vecLight.Z();

            if (bright >= 0) {
                //bright += m_brightAmbient;
                /*
                bright = Saturate(bright);

                m_pvertexLBuffer[index].r = bright * m_color.R();
                m_pvertexLBuffer[index].g = bright * m_color.G();
                m_pvertexLBuffer[index].b = bright * m_color.B();
                */

                m_pvertexLBuffer[index].r = Saturate(bright * m_color.R() + m_brightAmbient);
                m_pvertexLBuffer[index].g = Saturate(bright * m_color.G() + m_brightAmbient);
                m_pvertexLBuffer[index].b = Saturate(bright * m_color.B() + m_brightAmbient);
            } else {
                bright = /*m_brightAmbient*/ - bright;
                /*
                bright = Saturate(bright);

                m_pvertexLBuffer[index].r = bright * m_colorAlt.R();
                m_pvertexLBuffer[index].g = bright * m_colorAlt.G();
                m_pvertexLBuffer[index].b = bright * m_colorAlt.B();
                */

                m_pvertexLBuffer[index].r = Saturate(bright * m_colorAlt.R() + m_brightAmbient);
                m_pvertexLBuffer[index].g = Saturate(bright * m_colorAlt.G() + m_brightAmbient);
                m_pvertexLBuffer[index].b = Saturate(bright * m_colorAlt.B() + m_brightAmbient);
            }

            m_pvertexLBuffer[index].a = 1;
        }

        (this->*m_pfnTransform)(m_pvertexLBuffer, vcount, pindex, icount, pfnClip, pfnDraw);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Directional Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void LightVertex(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        UpdateLighting();

        SizeVertexLBuffer(vcount);

        for (int index = 0; index < vcount; index++) {
            m_pvertexLBuffer[index].x = pvertex[index].x;
            m_pvertexLBuffer[index].y = pvertex[index].y;
            m_pvertexLBuffer[index].z = pvertex[index].z;
            m_pvertexLBuffer[index].u = pvertex[index].u;
            m_pvertexLBuffer[index].v = pvertex[index].v;

            float bright =
                  pvertex[index].nx * m_vecLight.X()
                + pvertex[index].ny * m_vecLight.Y()
                + pvertex[index].nz * m_vecLight.Z()
                + m_brightAmbient;

            if (bright > 1) {
                bright = 1;
            } else if (bright < 0) {
                bright = 0;
            }

            m_pvertexLBuffer[index].r = bright;
            m_pvertexLBuffer[index].g = bright;
            m_pvertexLBuffer[index].b = bright;
            m_pvertexLBuffer[index].a = 1;
        }

        (this->*m_pfnTransform)(m_pvertexLBuffer, vcount, pindex, icount, pfnClip, pfnDraw);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Directional Lighting
    //
    //////////////////////////////////////////////////////////////////////////////

    void MaterialLightVertex(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount, PFNClip pfnClip, PFNDraw pfnDraw)
    {
        UpdateLighting();

        SizeVertexLBuffer(vcount);

        float r = m_pmaterial->GetDiffuse().GetRed();
        float g = m_pmaterial->GetDiffuse().GetGreen();
        float b = m_pmaterial->GetDiffuse().GetBlue();
        float a = m_pmaterial->GetDiffuse().GetAlpha();

        for (int index = 0; index < vcount; index++) {
            m_pvertexLBuffer[index].x = pvertex[index].x;
            m_pvertexLBuffer[index].y = pvertex[index].y;
            m_pvertexLBuffer[index].z = pvertex[index].z;
            m_pvertexLBuffer[index].u = pvertex[index].u;
            m_pvertexLBuffer[index].v = pvertex[index].v;

            float bright =
                  pvertex[index].nx * m_vecLight.X()
                + pvertex[index].ny * m_vecLight.Y()
                + pvertex[index].nz * m_vecLight.Z()
                + m_brightAmbient;

            if (bright > 1) {
                bright = 1;
            } else if (bright < 0) {
                bright = 0;
            }

            m_pvertexLBuffer[index].r = r * bright;
            m_pvertexLBuffer[index].g = g * bright;
            m_pvertexLBuffer[index].b = b * bright;
            m_pvertexLBuffer[index].a = a;
        }

        (this->*m_pfnTransform)(m_pvertexLBuffer, vcount, pindex, icount, pfnClip, pfnDraw);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw lines using triangles
    //
    //////////////////////////////////////////////////////////////////////////////

    void StoreVertex(
        VertexScreen& pvertex,
        float    x,
        float    y,
        float    z,
        float    qw,
        D3DCOLOR color,
        float    u,
        float    v
    ) {
        pvertex.x        = x;
        pvertex.y        = y;
        pvertex.z        = z;
        pvertex.qw       = qw;
        pvertex.color    = color;
        pvertex.specular = 0;
        pvertex.u        = u;
        pvertex.v        = v;
    }

    void DrawLinesWithPolygons(const VertexScreen* pvertexInArg, int vcount, const MeshIndex* pindexInArg, int icount)
    {
        bool bUsingVertexBuffer = (pvertexInArg == m_pvertexScreenBuffer);
        bool bUsingIndexBuffer  = (pindexInArg  == m_pindexBuffer       );

        //
        // Allocate space for the triangles
        //

        SizeIndexBuffer(icount + icount * 3);

        VertexScreen* pvertexOut = GetVertexScreenBuffer(vcount + icount * 2) + vcount;
        MeshIndex*    pindexOut  = m_pindexBuffer + icount;

        //
        // Figure out which source index point to use
        //

        const VertexScreen* pvertexIn = bUsingVertexBuffer ? m_pvertexScreenBuffer : pvertexInArg;
        const MeshIndex*    pindexIn  = bUsingIndexBuffer  ? m_pindexBuffer        : pindexInArg ;

        //
        // Build all of the triangles
        //

        Vector vecUp(0, 0, 1);
        VertexScreen* pvertex  = pvertexOut;
        MeshIndex*    pindex   = pindexOut;
        int           indexOut = 0;

        for (int index = 0; index < icount; index += 2) {
            const VertexScreen& vertex1 = pvertexIn[pindexIn[index    ]];
            const VertexScreen& vertex2 = pvertexIn[pindexIn[index + 1]];

            if (
                   vertex1.x != vertex2.x 
                || vertex1.y != vertex2.y
            ) {
                Vector vecForward(vertex2.x - vertex1.x, vertex2.y - vertex1.y, 0);
                Vector vecLeft = CrossProduct(vecUp, vecForward);
                vecLeft = vecLeft.Normalize() * m_lineWidth;

                pvertex[0]          = vertex1;
                pvertex[0].x        = vertex1.x + vecLeft.X();// * vertex1.qw;
                pvertex[0].y        = vertex1.y + vecLeft.Y();// * vertex1.qw;
                pvertex[0].z        = vertex1.z;
                pvertex[0].qw       = vertex1.qw;
                pvertex[0].u        = vertex1.u;
                pvertex[0].v        = 0;
                pvertex[0].color    = vertex1.color;
                pvertex[0].specular = vertex1.specular;

                pvertex[1]          = vertex1;
                pvertex[1].x        = vertex1.x - vecLeft.X();// * vertex1.qw;
                pvertex[1].y        = vertex1.y - vecLeft.Y();// * vertex1.qw;
                pvertex[1].z        = vertex1.z;
                pvertex[1].qw       = vertex1.qw;
                pvertex[1].u        = vertex1.u;
                pvertex[1].v        = 1;
                pvertex[1].color    = vertex1.color;
                pvertex[1].specular = vertex1.specular;

                pvertex[2]          = vertex2;
                pvertex[2].x        = vertex2.x + vecLeft.X();// * vertex2.qw;
                pvertex[2].y        = vertex2.y + vecLeft.Y();// * vertex2.qw;
                pvertex[2].z        = vertex2.z;
                pvertex[2].qw       = vertex2.qw;
                pvertex[2].u        = vertex2.u;
                pvertex[2].v        = 0;
                pvertex[2].color    = vertex2.color;
                pvertex[2].specular = vertex2.specular;

                pvertex[3]          = vertex2;
                pvertex[3].x        = vertex2.x - vecLeft.X();// * vertex2.qw;
                pvertex[3].y        = vertex2.y - vecLeft.Y();// * vertex2.qw;
                pvertex[3].z        = vertex2.z;
                pvertex[3].qw       = vertex2.qw;
                pvertex[3].u        = vertex2.u;
                pvertex[3].v        = 1;
                pvertex[3].color    = vertex2.color;
                pvertex[3].specular = vertex2.specular;

                pvertex += 4;

                pindex[0] = indexOut + 0;
                pindex[1] = indexOut + 1;
                pindex[2] = indexOut + 2;
                pindex[3] = indexOut + 1;
                pindex[4] = indexOut + 3;
                pindex[5] = indexOut + 2;
                pindex   += 6;
                indexOut += 4;
            }
        }

        //
        // Update the performance counter
        //

        #ifdef EnablePerformanceCounters
            m_countTriangles += icount;
        #endif

        //
        // Render them
        //

        m_prasterizer->DrawTriangles(pvertexOut, pvertex - pvertexOut, pindexOut, pindex - pindexOut);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Vertices
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawLinesRasterizer(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        m_prasterizer->DrawLines(pvertex, vcount, pindex, icount);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Vertices
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawTriangles(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        m_prasterizer->DrawTriangles(pvertex, vcount, pindex, icount);
    }

    void DrawLines(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        UpdatePointers();
        (this->*m_pfnDrawLines)(pvertex, vcount, pindex, icount);
    }

    void DrawPoints(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
		// Not called.
        m_prasterizer->DrawPoints(pvertex, vcount);
    }

    void DrawPoints(const VertexScreen* pvertex, int vcount)
    {
        m_prasterizer->DrawPoints(pvertex, vcount);
    }

    void DrawTriangles(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        ZAssert(false);
    }

    void DrawLines(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        ZAssert(false);
    }

    void DrawPoints(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        ZAssert(false);
    }

    void DrawPoints(const D3DVertex* pvertex, int vcount)
    {
        ZAssert(false);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Standard unlit, textured polys.
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawTriangles(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        #ifdef EnablePerformanceCounters
            m_countTriangles += icount / 3;
        #endif

		UpdatePointers();
        (this->*m_pfnLightVertex)(pvertex, vcount, pindex, icount, &Device3D::ClipTriangles, &Device3D::DrawTriangles);
    }

    //////////////////////////////////////////////////////////////////////////////
	//
	// New rendering path, using hardware vertex and index buffers to
	// render indexed primitive lists.
	// 
	//////////////////////////////////////////////////////////////////////////////
	void DrawTriangles( const D3DPRIMITIVETYPE primType,
						const CVBIBManager::SVBIBHandle * phVB, 
						const CVBIBManager::SVBIBHandle * phIB)
	{
		HRESULT hr;

		// Now we update the light vectors and settings.
		UpdateLighting( );

		// Get the current world transform from the device state.
		GenerateWorldTransform( );

		hr = CVBIBManager::Get()->SetVertexAndIndexStreams( phVB, phIB );
        ZAssert( hr == D3D_OK );
		hr = CD3DDevice9::Get()->SetFVF( phVB->dwBufferFormat );
        ZAssert( hr == D3D_OK );
		hr = CD3DDevice9::Get()->SetTransform( D3DTS_WORLD, &m_matWorldD3D );
        ZAssert( hr == D3D_OK );
												
		hr = CD3DDevice9::Get()->DrawIndexedPrimitive(
				primType, 0, 0,
				phVB->dwNumElements,
				phIB->dwFirstElementOffset / 2,
				phIB->dwNumElements / 3 );
        ZAssert( hr == S_OK );
	}

    //////////////////////////////////////////////////////////////////////////////
	//
	// New rendering path, using hardware vertex buffers to render primitive lists.
	// 
	//////////////////////////////////////////////////////////////////////////////
	void DrawTriangles( const D3DPRIMITIVETYPE primType,
						const DWORD dwNumPrims,
						const CVBIBManager::SVBIBHandle * phVB)
	{
		HRESULT hr;

		// Now we update the light vectors and settings.
		UpdateLighting( );
        CD3DDevice9::Get()->SetRenderState(D3DRS_LIGHTING, false);

		hr = CVBIBManager::Get()->SetVertexStream( phVB );
        ZAssert( hr == D3D_OK );
		hr = CD3DDevice9::Get()->SetFVF( phVB->dwBufferFormat );
        ZAssert( hr == D3D_OK );
		hr = CD3DDevice9::Get()->DrawPrimitive( primType, phVB->dwFirstElementOffset, dwNumPrims );
        ZAssert( hr == S_OK );
	}


    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawLines(const Vertex* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        #ifdef EnablePerformanceCounters
            m_countLines += icount - 1;
        #endif

        UpdatePointers();
        (this->*m_pfnLightVertex)(pvertex, vcount, pindex, icount, &Device3D::ClipLines, m_pfnDrawLines);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawPoints(const Vertex* pvertex, int vcount)
    {
        #ifdef EnablePerformanceCounters
            m_countPoints += vcount;
        #endif

        UpdatePointers();
        (this->*m_pfnLightVertex)(pvertex, vcount, NULL, 0, &Device3D::ClipPoints, &Device3D::DrawPoints);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw Colored Triangles
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawTriangles(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        #ifdef EnablePerformanceCounters
            m_countTriangles += icount / 3;
        #endif

        UpdatePointers();
        (this->*m_pfnLightVertexL)(pvertex, vcount, pindex, icount, &Device3D::ClipTriangles, &Device3D::DrawTriangles);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw Colored Lines
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawLines(const VertexL* pvertex, int vcount, const MeshIndex* pindex, int icount)
    {
        #ifdef EnablePerformanceCounters
            m_countLines += icount - 1;
        #endif

        UpdatePointers();
        (this->*m_pfnLightVertexL)(pvertex, vcount, pindex, icount, &Device3D::ClipLines, m_pfnDrawLines);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw Colored Points
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawPoints(const VertexL* pvertex, int vcount)
    {
        #ifdef EnablePerformanceCounters
            m_countPoints += vcount;
        #endif

        UpdatePointers();
        (this->*m_pfnLightVertexL)(pvertex, vcount, NULL, 0, &Device3D::ClipPoints, &Device3D::DrawPoints);
    }
};

TRef<IDevice3D> CreateDevice3D(Rasterizer* prasterizer)
{
    return new Device3D(prasterizer);
}
