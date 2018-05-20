#ifndef _engine_H_
#define _engine_H_

#include <mask.h>
#include <rect.h>
#include <tref.h>
#include <vector.h>

#include "surface.h"
#include "value.h"
#include "valuetransform.h"

extern bool g_bLuaDebug;

#ifdef _DEBUG
    //
    // Setting this to true will log every dx call made
    //

    extern bool g_bDumpDD;
#endif

class Context;
class Camera;
class Engine;
class Material;
class Palette;
class PixelFormat;
class Window;


//////////////////////////////////////////////////////////////////////////////
//
// Engine
//
//////////////////////////////////////////////////////////////////////////////

class Engine : public IObject {
public:
    //
    // Initialization and cleanup
    //

    virtual void SetFocusWindow(Window* pwindow, bool bStartFullscreen)    = 0;
    virtual void TerminateEngine(bool bEngineAppTerminate = false)               = 0;
    virtual bool IsDeviceReady(bool& bChanges)                             = 0;

    //
    // Attributes
    //

	virtual void SetEnableMipMapGeneration(bool bEnable)					= 0;
	virtual void SetMaxTextureSize(int dwMaxTextureSize)					= 0; //yp / imago 7/18/09
	virtual void SetVSync(bool bEnable)										= 0; //Imago 7/18/09
	virtual void SetAA(DWORD dwEnable)										= 0; //Imago 7/18/09
	virtual void SetAutoGenMipMaps(bool bEnable)							= 0; //Imago 7/18/09
    virtual void SetFullscreen(bool bFullscreen)                           = 0;
    virtual void SetFullscreenSize(const Vector& point)                  = 0;  //imago enhanced to include refresh rate 7/1/09
    virtual void SetFullscreenSize(const WinPoint& point)                 = 0;
    virtual void ChangeFullscreenSize(bool bLarger)                        = 0;
    virtual void SetGammaLevel(float value)                                = 0;
		// imago refactor enginep 6/26/09 - 7/1/09
	virtual Vector           NextMode(const WinPoint& size)                       = 0;
	virtual Vector           PreviousMode(const WinPoint& size)                   = 0;
	virtual void             EliminateModes(const Vector& size)                   = 0; //imago enhanced to include refresh rate 7/1/90

    virtual bool            IsFullscreen()                                 = 0;
    virtual ZString         GetDeviceName()                                = 0;
	virtual const TRef<ModifiableWinPointValue> GetResolutionSizeModifiable()		   = 0;
    virtual const WinPoint GetFullscreenSize()							   = 0;
    virtual float           GetGammaLevel()                                = 0;

    //KGJV 32B
    virtual PixelFormat* GetPrimaryPixelFormat()                           = 0;
    virtual ZString         GetPixelFormatName()                           = 0;
    //
    // For our exception handler
    //

    virtual void DebugSetWindowed() = 0;

    //
    // Screen access
    //

    virtual void BltToWindow(Window* pwindow, const WinPoint& point, Surface* psurface, const WinRect& rect) = 0;
    virtual void Flip()  = 0;

    //
    // Surface Constructors
    //

    virtual TRef<Surface> CreateSurface(HBITMAP hbitmap) = 0;

    virtual TRef<Surface> CreateDummySurface(
        const WinPoint& size, 
        SurfaceSite*    psite = NULL
    ) = 0;

	virtual TRef<Surface> CreateSurface(
        const WinPoint& size, 
        SurfaceType     stype, 
        SurfaceSite*    psite = NULL
    ) = 0;

    virtual TRef<Surface> CreateSurface(
        const WinPoint& size,
        PixelFormat*    ppf,
        SurfaceType     stype    = SurfaceType2D(),
        SurfaceSite*    psite    = NULL
    ) = 0;

    virtual TRef<Surface> CreateSurface(
        const WinPoint& size,
        PixelFormat*    ppf,
        int             pitch,
        BYTE*           pdata,
        IObject*        pobjectMemory,
		const bool		bColorKey,
		const Color &	cColorKey,
		const ZString &	szTextureName = "",
		const bool		bSystemMemory = false
    ) = 0;

	virtual TRef<Surface> CreateSurfaceD3DX(
		const D3DXIMAGE_INFO *	pImageInfo,
		const WinPoint *		pTargetSize,
		IObject *				pobjectMemory,
		const bool				bColorKey,
		const Color &			cColorKey,
		const ZString &			szTextureName = "",
		const bool				bSystemMemory = false 
		) = 0;

	virtual TRef<Surface> CreateRenderTargetSurface(
		const WinPoint & size, 
        SurfaceSite*    psite = NULL ) = 0;

    //
    // Performance counters
    //

    virtual int GetTotalTextureMemory()     = 0;
    virtual int GetAvailableTextureMemory() = 0;
    virtual int GetTotalVideoMemory()       = 0;
    virtual int GetAvailableVideoMemory()   = 0;
};

TRef<Engine> CreateEngine(bool bAllow3DAcceleration, bool bAllowSecondary, DWORD dwBPP, HWND hWindow);

//////////////////////////////////////////////////////////////////////////////
//
// Justification
//
//////////////////////////////////////////////////////////////////////////////

class JustificationBase {};
typedef TBitMask<JustificationBase, DWORD> Justification;

class JustifyLeft          : public Justification { public: JustifyLeft         () : Justification(0x01) {} };
class JustifyRight         : public Justification { public: JustifyRight        () : Justification(0x02) {} };
class JustifyTop           : public Justification { public: JustifyTop          () : Justification(0x04) {} };
class JustifyBottom        : public Justification { public: JustifyBottom       () : Justification(0x08) {} };
class JustifyXCenter       : public Justification { public: JustifyXCenter      () : Justification(0x10) {} };
class JustifyYCenter       : public Justification { public: JustifyYCenter      () : Justification(0x20) {} };
class JustifyLeftClipRight : public Justification { public: JustifyLeftClipRight() : Justification(0x40) {} };
class JustifyCenter        : public Justification { public: JustifyCenter       () : Justification(0x30) {} };

#endif
