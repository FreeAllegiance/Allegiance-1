#ifndef _enginep_h_
#define _enginep_h_

#include "context.h"
#include "ddstruct.h"
#include "engine.h"
#include "namespace.h"
#include "popup.h"
#include "surface.h"

//////////////////////////////////////////////////////////////////////////////
//
// Global hacks
//
//////////////////////////////////////////////////////////////////////////////

extern bool g_bMDLLog;
extern bool g_bWindowLog;

//////////////////////////////////////////////////////////////////////////////
//
// Direct Draw includes
//
//////////////////////////////////////////////////////////////////////////////

//
// DirectX Stuff
//


/*typedef IDirectDraw4                 IDirectDrawX;
typedef IDirectDrawSurface4          IDirectDrawSurfaceX;
typedef IDirectDrawClipper           IDirectDrawClipperX;
typedef IDirectDrawPalette           IDirectDrawPaletteX;
typedef IDirectDrawGammaControl      IDirectDrawGammaControlX;*/


//typedef IDirect3D9						IDirect3DX;
//typedef IDirect3DDevice9				IDirect3DDeviceX;
//typedef IDirect3DTexture9				IDirect3DTextureX;
//#define IID_IDirect3DX					IID_IDirect3D9
//#define IID_IDirect3DTextureX			IID_IDirect3DTexture9

//#define IID_IDirectDrawX					IID_IDirectDraw4
//#define IID_IDirectDrawSurfaceX				IID_IDirectDrawSurface4
//#define IID_IDirectDrawGammaControlX		IID_IDirectDrawGammaControl

//typedef DDSURFACEDESC2               DDSURFACEDESCX;
//typedef DDSCAPS2                     DDSCAPSX;

//
// DirectX Wrapper Classes
//


//////////////////////////////////////////////////////////////////////////////
//
// Direct Draw includes
//
//////////////////////////////////////////////////////////////////////////////

/*bool FillDDPF(
    D3D9PixelFormat&      ppdf,
    IDirectDrawX*         pdd,
    HDC                   hdc,
    HBITMAP               hbitmap,
    IDirectDrawPaletteX** pppalette
);*/

//////////////////////////////////////////////////////////////////////////////
//
// Direct Draw Asserts
//
//////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
    bool DDError(HRESULT hr, const char* pszCall, const char* pszFile, int line, const char* pszModule);
    bool DDSCallImpl(HRESULT hr, const char* pszCall, const char* pszFile, int line, const char* pszModule);

    #define DDCall(hr)  DDError(hr, #hr, __FILE__, __LINE__, __MODULE__)
    #define DDSCall(hr) DDSCallImpl(hr, #hr, __FILE__, __LINE__, __MODULE__)
#else
    bool DDSCallImpl(HRESULT hr);

    #define DDCall(hr) SUCCEEDED(hr)
    #define DDSCall(hr) SUCCEEDED(hr)
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//
//////////////////////////////////////////////////////////////////////////////

//class PrivatePalette;
class PrivateContext;
class PrivateSurface;
class PrivateEngine;
//class VideoSurface;
class DDSurface;
class Rasterizer;
class D3DRasterizer;

//////////////////////////////////////////////////////////////////////////////
//
// Rasterizer
//
//////////////////////////////////////////////////////////////////////////////

class Rasterizer : public IObject {
public:
    virtual void Terminate()                                            = 0;

    //
    // State
    //

    virtual void  SetShadeMode(ShadeMode shadeMode)                     = 0;
    virtual void  SetBlendMode(BlendMode blendMode)                     = 0;
    virtual void  SetTexture(Surface* psurfaceTexture)                  = 0;
    virtual void  SetWrapMode(WrapMode wrapMode)                        = 0;
    virtual void  SetCullMode(CullMode cullMode)                        = 0;
    virtual void  SetZTest(bool bZTest)                                 = 0;
    virtual void  SetZWrite(bool bZWrite)                               = 0;
    virtual void  SetLinearFilter(bool bLinearFilter)                   = 0;
    virtual void  SetPerspectiveCorrection(bool bPerspectiveCorrection) = 0;
    virtual void  SetDither(bool bDither)                               = 0;
    virtual void  SetColorKey(bool bColorKey)                           = 0;

    virtual void  BeginScene()                                          = 0;
    virtual void  EndScene()                                            = 0;
    virtual void  ClearZBuffer()                                        = 0;
    virtual void  SetClipRect(const Rect& rectClip)                     = 0;

    //
    // Attributes
    //

    virtual Point GetSurfaceSize()                                      = 0;
    virtual bool  Has3DAcceleration()                                   = 0;

    //
    // Rendering
    //

    virtual void DrawTriangles(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLines(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPoints(const VertexScreen* pvertex, int vcount)                                 = 0;
};

/*class D3DRasterizer : public Rasterizer {
public:
    
    virtual TRef<IDirect3DDeviceX>   GetD3DDeviceX()                    = 0;
    virtual TRef<IDirect3DX>         GetD3D()                           = 0;
    virtual D3DVIEWPORT9 *			GetViewport()                      = 0;
    virtual void DrawTrianglesD3D(const D3DLVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLinesD3D(const D3DLVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPointsD3D(const D3DLVertex* psource, int count                                ) = 0;

    virtual void DrawTrianglesD3D(const D3DVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLinesD3D(const D3DVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPointsD3D(const D3DVertex* psource, int count                                ) = 0;
};*/

class D3D9Rasterizer : public Rasterizer 
{
public:
//	virtual TRef<IDirect3DDevice9>   GetD3DDeviceX() = 0;
//	virtual TRef<IDirect3D9> GetD3D() = 0;
	virtual D3DVIEWPORT9 * GetViewport() = 0;
	virtual void DrawTrianglesD3D(const D3DLVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
	virtual void DrawLinesD3D(const D3DLVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
	virtual void DrawPointsD3D(const D3DLVertex* psource, int count ) = 0;
	virtual void DrawTrianglesD3D(const D3DVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
	virtual void DrawLinesD3D(const D3DVertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
	virtual void DrawPointsD3D(const D3DVertex* psource, int count ) = 0;
};

TRef<D3D9Rasterizer> CreateD3D9Rasterizer(PrivateSurface* psurface);
//TRef<D3DRasterizer> CreateD3DRasterizer(PrivateSurface* psurface, HWND hParentWindow);
//TRef<Rasterizer>    CreateSoftwareRasterizer(PrivateSurface* psurface);

//////////////////////////////////////////////////////////////////////////////
//
// 3D Device
//
//////////////////////////////////////////////////////////////////////////////

class IDevice3D : public IObject {
public:
    //
    // Capabilities
    //

    virtual bool Has3DAcceleration() = 0;

    //
    // State
    //

    virtual void SetYAxisInversion(bool bValue)                        = 0;
    virtual bool GetClipping()                                         = 0;
    virtual const Matrix& GetMatrix()                                  = 0;
    virtual const Matrix& GetInverseModelMatrix()                      = 0;

    virtual void AddClipPlane(const Plane& plane)                      = 0;
    virtual void RemoveClipPlane(int indexRemove)                      = 0;

    virtual void SetMatrix(const Matrix& mat, const Matrix& matWorldTM)= 0;
    virtual void SetPerspectiveMatrix(const Matrix& mat)               = 0;
    virtual void SetViewMatrix(const Matrix& mat)                      = 0;
    virtual void SetClipping(bool bClip)                               = 0;
    virtual void SetShadeMode(ShadeMode shadeMode)                     = 0;
    virtual void SetBlendMode(BlendMode blendMode)                     = 0;
    virtual void SetTexture(Surface* psurfaceTexture)                  = 0;
    virtual void SetMaterial(Material* pmaterial)                      = 0;
    virtual void SetWrapMode(WrapMode wrapMode)                        = 0;
    virtual void SetCullMode(CullMode cullMode)                        = 0;
    virtual void SetZTest(bool bZTest)                                 = 0;
    virtual void SetZWrite(bool bZWrite)                               = 0;
    virtual void SetLinearFilter(bool bLinearFilter)                   = 0;
    virtual void SetPerspectiveCorrection(bool bPerspectiveCorrection) = 0;
    virtual void SetDither(bool bDither)                               = 0;
    virtual void SetColorKey(bool bColorKey)                           = 0;
    virtual void SetLineWidth(float width)                             = 0;
    virtual void SetDeformation(Deformation* pdeform)                  = 0;

    virtual void SetGlobalColor(const Color& color)                                               = 0;
    virtual void DirectionalLight(const Vector& vec, const Color& color)                          = 0;
    virtual void BidirectionalLight(const Vector& vec, const Color& color, const Color& colorAlt) = 0;
    virtual void SetAmbientLevel(float level)                                                     = 0;

    //
    //
    //

    virtual void SetClipRect(const Rect& rectClip)                     = 0;
    virtual void BeginScene()                                          = 0;
    virtual void EndScene()                                            = 0;
    virtual void ClearZBuffer()                                        = 0;

    //
    // VertexBuffer
    //

    virtual VertexL*      GetVertexLBuffer(int count)      = 0;
    virtual VertexScreen* GetVertexScreenBuffer(int count) = 0;

    //
    // Rendering
    //

    virtual void DrawTriangles(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLines(const D3DVertex* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPoints(const D3DVertex* pvertex, int vcount)                                 = 0;

    virtual void DrawTriangles(const Vertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLines(const Vertex* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPoints(const Vertex* psource, int count)                                 = 0;

    virtual void DrawTriangles(const VertexL* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLines(const VertexL* psource, int count, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPoints(const VertexL* psource, int count                                ) = 0;

    virtual void DrawTriangles(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void     DrawLines(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void    DrawPoints(const VertexScreen* pvertex, int vcount)                                 = 0;

	virtual void DrawTriangles( const D3DPRIMITIVETYPE primType,
								const CVBIBManager::SVBIBHandle * phVB, 
								const CVBIBManager::SVBIBHandle * phIB) = 0;

	virtual void DrawTriangles( const D3DPRIMITIVETYPE primType,
								const DWORD dwNumPrims,
								const CVBIBManager::SVBIBHandle * phVB) = 0;

	//
    // Performance Counters
    //

    virtual int GetPerformanceCounter(Counter counter) = 0;
    virtual void ResetPerformanceCounters() = 0;
	};

TRef<IDevice3D> CreateDevice3D(Rasterizer* prasterizer);

//////////////////////////////////////////////////////////////////////////////
//
// PrivateContext
//
//////////////////////////////////////////////////////////////////////////////

class PrivateContext : public Context {
public:
	//
	// Called from Surface
	//

    virtual void            BeginRendering() = 0;
    virtual void            EndRendering()   = 0;
    virtual PrivateSurface* GetSurface()     = 0;
};

TRef<PrivateContext> CreateContextImpl( PrivateSurface* psurface );

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class BinarySurfaceInfo {
public:
    WinPoint m_size;
    int      m_pitch;
    DWORD    m_bitCount;
    DWORD    m_redMask;
    DWORD    m_greenMask;
    DWORD    m_blueMask;
    DWORD    m_alphaMask;
    bool     m_bColorKey;
};

//////////////////////////////////////////////////////////////////////////////
//
// PrivateSurface
//
//////////////////////////////////////////////////////////////////////////////

class PrivateSurface : 
    public Surface
{
public:
};

//////////////////////////////////////////////////////////////////////////////
//
// PrivateSurface
//
//////////////////////////////////////////////////////////////////////////////

TRef<PrivateSurface> CreatePrivateDummySurface(	PrivateEngine * pengine,
												const WinPoint & size,
												SurfaceSite * psite );

TRef<PrivateSurface> CreatePrivateRenderTargetSurface(	PrivateEngine * pengine,
														const WinPoint & size,
														SurfaceSite * psite );

TRef<PrivateSurface> CreatePrivateSurface(
    PrivateEngine*  pengine,
    PixelFormat*    ppf,
    const WinPoint& size,
    SurfaceType     stype,
    SurfaceSite*    psite
);

TRef<PrivateSurface> CreatePrivateSurface(
					PrivateEngine*		pengine,
					PixelFormat*		ppf,
					const WinPoint&     size,
					int					pitch,
					BYTE*				pdata,
					IObject*			pobjectMemory,
					const bool			bColorKey,
					const Color &		cColourKey,
					const ZString &		szTextureName = "",
					const bool			bSystemMemory = false );

TRef<PrivateSurface> CreatePrivateSurface(	
					PrivateEngine*			pengine,
					const D3DXIMAGE_INFO *	pImageInfo,
					const WinPoint *		pTargetSize,
					IObject *				pobjectMemory,
					const bool				bColorKey,
					const Color &			cColorKey,
					const ZString &			szTextureName = "",
					const bool				bSystemMemory = false );

/*TRef<PrivateSurface> CreatePrivateSurface(
    PrivateEngine*	pengine,
    DDSurface *		pvideoSurface,
    SurfaceSite*	psite
);*/

TRef<PrivateSurface> CreatePrivateSurface(	D3DFORMAT	texFormat,
											DWORD		dwWidth,
											DWORD		dwHeight,
                                            const char *szTexName = NULL );


//////////////////////////////////////////////////////////////////////////////
//
// PrivateEngine
//
//////////////////////////////////////////////////////////////////////////////

class PrivateEngine : public Engine {
public:
    //virtual void AddDeviceDependant(DeviceDependant* pdeviceDependant)    = 0;
    //virtual void RemoveDeviceDependant(DeviceDependant* pdeviceDependant) = 0;
    virtual void RemovePrivateSurface(PrivateSurface* psurface)           = 0;

//    virtual DDDevice*       GetCurrentDevice() = 0;
//    virtual DDDevice*       GetPrimaryDevice() = 0;
//    virtual DDSDescription  GetPrimaryDDSD()   = 0;
//    virtual PrivateSurface* GetBackBuffer()    = 0;

	virtual TRef<PrivateSurface> CreateCompatibleSurface(
        PrivateSurface* psurface, 
        const WinPoint& size, 
        SurfaceType     stype, 
        SurfaceSite*    psite
    ) = 0;

/*    virtual TRef<DDSurface> CreateVideoSurface(	SurfaceType     stype,
												PixelFormat*    ppf,
												PrivatePalette* ppalette,
												const WinPoint& size,
												int             pitch,
												BYTE*           pbits	) = 0;*/

    //
    // Pixel Formats
    //

    virtual PixelFormat*      GetPrimaryPixelFormat() = 0;
//    virtual TRef<PixelFormat> GetPixelFormat(const DDPixelFormat& ddpf) = 0;
    virtual TRef<PixelFormat> GetPixelFormat(
        int   bits,
        DWORD redMask,
        DWORD greenMask,
        DWORD blueMask,
        DWORD alphaMask
    ) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// IEngineFont
//
//////////////////////////////////////////////////////////////////////////////

TRef<IEngineFont> CreateEngineFont(HFONT hfont);
TRef<IEngineFont> CreateEngineFont(IBinaryReaderSite* psite);
TRef<IEngineFont> CreateEngineFont(std::string name, int size, int stretch, bool bold, bool italic, bool underline);
TRef<IEngineFont> CreateEngineFont( D3DXFONT_DESC * pFontDesc );			// ADDED

//////////////////////////////////////////////////////////////////////////////
//
// NameSpace stuff
//
//////////////////////////////////////////////////////////////////////////////

TRef<INameSpaceInfo> CreateNameSpaceInfo(INameSpace* pns, const ZString& str);

TRef<INameSpace> CreateNameSpace(const ZString& strName);
TRef<INameSpace> CreateNameSpace(const ZString& strName, INameSpace* pnsParent);
TRef<INameSpace> CreateNameSpace(const ZString& strName, INameSpaceList& parents);
TRef<INameSpace> CreateNameSpace(const ZString& strName, Modeler* pmodeler, ZFile* pfile);

TRef<INameSpace> CreateBinaryNameSpace(const ZString& strName, Modeler* pmodeler, ZFile* pfile);

//////////////////////////////////////////////////////////////////////////////
//
// Popup stuff
//
//////////////////////////////////////////////////////////////////////////////

class IPopupContainerPrivate : public IPopupContainer {
public:
    virtual void Initialize(Engine* pengine, RectValue* prectValue) = 0;
};

TRef<IPopupContainerPrivate> CreatePopupContainer();

#endif
