#ifndef _Model_h_
#define _Model_h_

#include <memory>

#include "bounds.h"
#include "context.h"
#include "value.h"

#include <input.h>
#include <tref.h>
#include <tvector.h>

#include "FileLoader.h"

class Context;
class D3DVertex;
class D3DLVertex;
class Engine;
class Matrix;
class Surface;
class Transform;
class Vertex;
class VertexL;

//////////////////////////////////////////////////////////////////////////////
//
// 3D Hit Data
//
//////////////////////////////////////////////////////////////////////////////

class HitData3D {
public:
    float  m_distance;
    Vector m_vecHit;
    class Geo* m_pgeo;
};

//////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//
//////////////////////////////////////////////////////////////////////////////

class TextureGeo;
class GroupGeo;
class MeshGeo;
class Image;

//////////////////////////////////////////////////////////////////////////////
//
// Group Geo
//
//////////////////////////////////////////////////////////////////////////////

class GroupGeoCallback : public IObject {
public:
    virtual TRef<Geo> Execute(const Matrix& mat, GroupGeo* pgroup) = 0;
    virtual void ReportTriangles(const Matrix& mat, const TVector<Vertex>&  vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void ReportTriangles(const Matrix& mat, const TVector<VertexL>& vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void ReportTriangles(const Matrix& mat, const TVector<D3DVertex>& vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void ReportTriangles(const Matrix& mat, const TVector<D3DLVertex>& vertices, const TVector<MeshIndex>& indices) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Geometry
//
//////////////////////////////////////////////////////////////////////////////

class Geo : public Value {
protected:
    Geo() : Value() {}
    DefineSubclassMembers(Geo);

public:
    //
    // Constructors
    //

    DefineCastMember(Geo);

    static Geo* GetSquare();
    static Geo* GetIcosahedron();
    static Geo* GetEmpty();
    static Geo* GetWhiteEmissiveSquare();

    static TRef<Geo> CreateMesh(
        Vertex* pvertices,
        int vcount,
        MeshIndex* pindices,
        int icount,
        IObject* pobjectMemory
    );

    static TRef<Geo> CreateMesh(
        D3DVertex* pvertices,
        int vcount,
        MeshIndex* pindices,
        int icount,
        IObject* pobjectMemory
    );

    static TRef<Geo> CreateMesh(const TVector<D3DVertex>& vertices, const TVector<MeshIndex>& indices);
    static TRef<Geo> CreateMesh(const TVector<Vertex>& vertices, const TVector<MeshIndex>& indices);
    static TRef<Geo> CreateMesh(const TVector<VertexL>& vertices, const TVector<MeshIndex>& indices);

    //
    // Geo members
    //

    void SetEmpty();
    virtual void Render(Context* pcontext);

    //
    // Hit Testing
    //

    virtual void RemoveCapture();
    virtual bool HitTest(const Vector& vecOrigin, const Vector& vecRay, HitData3D& hitData, bool bCaptured);
    virtual void MouseMove(const Vector& vec, bool bCaptured, bool bInside);
    virtual void MouseEnter(const Vector& vec);
    virtual void MouseLeave();
    virtual MouseResult Button(const Vector& vec, int button, bool bCaptured, bool bInside, bool bDown);

    //
    // attributes
    //

    virtual Image*      FindTexture();
    virtual float       GetRadius(const Matrix& mat);

    //
    // Optimization
    //

    virtual int         GetTriangleCount();
    virtual TextureGeo* GetTextureGeo();
    virtual MeshGeo*    GetMeshGeo();
    virtual GroupGeo*   GetGroupGeo();
    virtual bool        AnyChildGroups(bool bHead = true);

    virtual TRef<Geo>   FoldTexture();
    virtual TRef<Geo>   RemoveMaterial();
    virtual TRef<Geo>   ApplyTransform(Transform* ptrans);

    virtual void        CallGroupGeoCallback(const Matrix& mat, GroupGeoCallback* pcallback);
};

//////////////////////////////////////////////////////////////////////////////
//
// Image
//
//////////////////////////////////////////////////////////////////////////////

class Image : public Value, public IMouseInput {
protected:
    Bounds2 m_bounds;

    Image() : Value() {};
    DefineSubclassMembers(Image);

public:
    //
    // Constructors
    //

    DefineCastMember(Image);

    static Image* GetEmpty();

    //
    // Image members
    //

    void SetEmpty();

    virtual const Bounds2& GetBounds();
    virtual void CalcBounds();
    virtual void Render(Context* pcontext);
    virtual TRef<Surface> GetSurface();

    //
    // IMouseInput methods
    //

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured);

    //
    // Value members
    //

    virtual void Evaluate();
};

//////////////////////////////////////////////////////////////////////////////
//
// Animation Set
//
//////////////////////////////////////////////////////////////////////////////

class AnimationSet : public IObject {
public:
    virtual void SetTime(float time) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Modeler
//
//////////////////////////////////////////////////////////////////////////////

class ModelerSite : public IObject {
public:
    virtual void Error(const ZString& str) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Modeler
//
//////////////////////////////////////////////////////////////////////////////

class Modeler : public IObject {
public:
    //
    // Constructor
    //

    static TRef<Modeler> Create(Engine* pengine);

    //
    // Termination
    //

    virtual void Terminate() = 0;

    //
    // Modeler members
    //

    virtual void            SetSite(ModelerSite* psite) = 0; 
    virtual void SetFileLoader(const std::shared_ptr<IFileLoader>& loader) = 0;
    virtual void            SetArtPath(const PathString& pathStr) = 0;

    virtual Engine*         GetEngine() = 0;
    virtual ZString         GetArtPath() = 0;
    virtual std::shared_ptr<IFileLoader>         GetFileLoader() = 0;

    // KGJV 32B - move this to abstrat class modeler
    virtual TRef<ZFile> GetFile(const PathString& pathStr, const ZString& strExtensionArg, bool bError) = 0;

    virtual INameSpace*     CreateNameSpace(const ZString& str)                        = 0;
    virtual INameSpace*     CreateNameSpace(const ZString& str, INameSpace* pnsParent) = 0;
    virtual INameSpace*     GetNameSpace(const ZString& str, bool bError = true, bool bSystemMem = false )       = 0;
    virtual void            UnloadNameSpace(const ZString& str)                        = 0;
    virtual void            UnloadNameSpace(INameSpace* pns)                           = 0;

    virtual TRef<ZFile>     LoadFile(const PathString& pathStr, const ZString& strExtensionArg, bool bError = true)                 = 0;
    virtual HBITMAP         LoadBitmap(const PathString& pathStr, bool bError = true)               = 0;
    virtual TRef<Image>     LoadImage(const ZString& pathStr, bool bColorKey, bool bError = true, bool bSystemMem = false )   = 0;
    virtual TRef<Surface>   LoadSurface(const ZString& pathStr, bool bColorKey, bool bError = true, bool bSystemMem = false ) = 0;
    virtual TRef<Geo>       LoadGeo(const ZString& pathStr, bool bError = true)                     = 0;

    virtual TRef<Geo>       LoadXFile(const PathString& pathStr, Number* pnumberFrame, bool& bAnimation, bool bError = true) = 0;

	// Hint accessor functions.
	virtual bool			SetColorKeyHint( const bool bColorKey ) = 0;
	virtual bool 			GetColorKeyHint( ) = 0;
	virtual bool			SetSystemMemoryHint( const bool bSysMem ) = 0;
	virtual bool			GetSystemMemoryHint( ) = 0;
	virtual bool			SetUIImageUsageHint( const bool bUIImageUsage ) = 0;
	virtual bool			GetUIImageUsageHint( ) = 0;
};

#endif
