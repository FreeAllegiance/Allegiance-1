#ifndef _context_h_
#define _context_h_

//////////////////////////////////////////////////////////////////////////////
//
// Plane
//
//////////////////////////////////////////////////////////////////////////////

class Plane {
private:
    HVector m_hvec;

public:
    Plane()
    {
    }

    Plane(const Vector& normal, const Vector& position) :
        m_hvec(
            normal.X(), 
            normal.Y(), 
            normal.Z(),
            - (normal * position)
        )
    {
    }

    Plane(const HVector& vec) :
        m_hvec(vec)
    {
    }

    Plane(const Plane& plane) :
        m_hvec(plane.m_hvec)
    {
    }

    const HVector& GetHVector() const
    {
        return m_hvec;
    }

    float Distance(const HVector& vec) const;
    float Intersect(const HVector& v0, const HVector& v1) const;
};

//////////////////////////////////////////////////////////////////////////////
//
// Index into an array or vercies
//
//////////////////////////////////////////////////////////////////////////////

typedef WORD MeshIndex;

//////////////////////////////////////////////////////////////////////////////
//
// Vertex
//
//////////////////////////////////////////////////////////////////////////////

class Vertex {
public:
    float x, y, z;
    float u, v;
    float nx, ny, nz;

    Vertex()
    {
    }

    Vertex(
        float xArg,
        float yArg,
        float zArg,
        float nxArg,
        float nyArg,
        float nzArg,
        float uArg = 0,
        float vArg = 0
    ) :
        x(xArg),
        y(yArg),
        z(zArg),
        nx(nxArg),
        ny(nyArg),
        nz(nzArg),
        u(uArg),
        v(vArg)
    {
    }

    Vertex(
        const Vector& vec,
        const Vector& vecNormal,
        const Point& pointTexture = Point(0, 0)
    ) {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
        nx = vecNormal.X();
        ny = vecNormal.Y();
        nz = vecNormal.Z();
        u = pointTexture.X();
        v = pointTexture.Y();
    }

    Vertex(const Vertex& vertex)
    {
        memcpy(this, &vertex, sizeof(Vertex));
    }

    void SetPosition(const Vector& vec)
    {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
    }

    Vector GetPosition() const
    {
        return Vector(x, y, z);
    }

    void SetNormal(const Vector& vec)
    {
        nx = vec.X();
        ny = vec.Y();
        nz = vec.Z();
    }

    Vector GetNormal() const
    {
        return Vector(nx, ny, nz);
    }

    Point GetTextureCoordinate()
    {
        return Point(u, v);
    }

	// mdvalley: 2005 likes bools
    friend BOOL operator==(const Vertex& v0, const Vertex& v1)
    {
        return
               v0.x  == v1.x
            && v0.y  == v1.y
            && v0.z  == v1.z
            && v0.nx == v1.nx
            && v0.ny == v1.ny
            && v0.nz == v1.nz
            && v0.u  == v1.u
            && v0.v  == v1.v;
    }
};

class VertexL {
public:
    float x, y, z;
    float u, v;
    float r, g, b, a;

    VertexL()
    {
    }

    VertexL(
        float xArg,
        float yArg,
        float zArg,
        float rArg,
        float gArg,
        float bArg,
        float aArg,
        float uArg,
        float vArg
    ) :
        x(xArg),
        y(yArg),
        z(zArg),
        r(rArg),
        g(gArg),
        b(bArg),
        a(aArg),
        u(uArg),
        v(vArg)
    {
    }

    VertexL(
        const Vector& vec,
        const Color& color = Color::White(),
        const Point& point = Point::GetZero()
    ) :
        x(vec.X()),
        y(vec.Y()),
        z(vec.Z()),
        r(color.R()),
        g(color.G()),
        b(color.B()),
        a(color.A()),
        u(point.X()),
        v(point.Y())
    {
    }

    VertexL(const VertexL& vertex)
    {
        memcpy(this, &vertex, sizeof(VertexL));
    }

    void SetPosition(const Vector& vec)
    {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
    }

    Vector GetPosition() const
    {
        return Vector(x, y, z);
    }

    void SetTextureCoordinate(const Point& point)
    {
        u = point.X();
        v = point.Y();
    }

    Point GetTextureCoordinate()
    {
        return Point(u, v);
    }

    void SetNormal(const Vector& vec)
    {
    }

    Vector GetNormal() const
    {
        return Vector(1, 0, 0);
    }

    void SetColor(const Color& color)
    {
        r = color.R();
        g = color.G();
        b = color.B();
        a = color.A();
    }

    Color GetColor() const
    {
        return Color(r, g, b, a);
    }

	// mdvalley: bool
    friend BOOL operator==(const VertexL& v0, const VertexL& v1)
    {
        return
               v0.x == v1.x
            && v0.y == v1.y
            && v0.z == v1.z
            && v0.r == v1.r
            && v0.g == v1.g
            && v0.b == v1.b
            && v0.a == v1.a
            && v0.u == v1.u
            && v0.v == v1.v;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// D3D
//
//////////////////////////////////////////////////////////////////////////////

#ifdef FLOATASM
    inline D3DCOLOR MakeD3DCOLOR(const Color& color)
    {
        DWORD r, g, b, a;

        __asm {
            mov     ecx, color
            fld     [ecx]Color.m_r
            fmul    g_255       // r
            fld     [ecx]Color.m_g
            fmul    g_255       // g r
            fld     [ecx]Color.m_b
            fmul    g_255       // b g r
            fld     [ecx]Color.m_a
            fmul    g_255       // a b g r
            fxch    st(3)       // r b g a
            fistp   r           // b g a
            fistp   b
            fistp   g
            fistp   a
        }

        return 
               (a << 24)
            | (r << 16)
            | (g <<  8)
            | (b      );
    }
#else
    inline D3DCOLOR MakeD3DCOLOR(const Color& color)
    {
        return 
              (MakeInt(255.0f * color.GetAlpha()) << 24)
            | (MakeInt(255.0f * color.GetRed()  ) << 16)
            | (MakeInt(255.0f * color.GetGreen()) <<  8)
            | (MakeInt(255.0f * color.GetBlue() )      );
    }
#endif

inline Color MakeColor(const COLORVALUE& color) // was D3DCOLORVALUE
{
    return
        Color(
            (float)color.r,
            (float)color.g,
            (float)color.b,
            (float)color.a
        );
}

inline Color MakeColor(D3DCOLOR color)
{
    return
        Color(
            (float)((color >> 24) & 0xff) / 255.0f,
            (float)((color >> 16) & 0xff) / 255.0f,
            (float)((color >>  8) & 0xff) / 255.0f,
            (float)((color >>  0) & 0xff) / 255.0f
        );
}

class D3DVector : public D3DVECTOR {
public:
    D3DVector() {}
    D3DVector(float xArg, float yArg, float zArg)
    {
        x = xArg;
        y = yArg;
        z = zArg;
    }
    D3DVector(const Vector& vec)
    {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
    }

};

class D3DVertex : public D3DVERTEX {
public:
    D3DVertex() {}
    D3DVertex(
        const Vector& vec,
        const Vector& vecNormal,
        const Point& pointTexture = Point(0, 0)
    ) {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
        nx = vecNormal.X();
        ny = vecNormal.Y();
        nz = vecNormal.Z();
        tu = pointTexture.X();
        tv = pointTexture.Y();
    }

    D3DVertex(
        float xArg,
        float yArg,
        float zArg,
        float nxArg,
        float nyArg,
        float nzArg,
        float uArg = 0,
        float vArg = 0
    ) {
        x = xArg;
        y = yArg;
        z = zArg;
        nx = nxArg;
        ny = nyArg;
        nz = nzArg;
        tu = uArg;
        tv = vArg;
    }

    float  X()                    const { return x;                  }
    float  Y()                    const { return y;                  }
    float  Z()                    const { return z;                  }
    Vector GetPosition()          const { return Vector(x, y, z);    }
    Vector GetNormal()            const { return Vector(nx, ny, nz); }
    Point  GetTextureCoordinate() const { return Point(tu, tv);      }

    void SetPosition(const Vector& vec)
    {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
    }

    void SetNormal(const Vector& vec)
    {
        nx = vec.X();
        ny = vec.Y();
        nz = vec.Z();
    }

    void SetTextureCoordinate(const Point& point)
    {
        tu = point.X();
        tv = point.Y();
    }

    friend bool operator==(const D3DVertex& v1, const D3DVertex& v2)
    {
        return
              v1.x  == v2.x
           && v1.y  == v2.y
           && v1.z  == v2.z
           && v1.nx == v2.nx
           && v1.ny == v2.ny
           && v1.nz == v2.nz
           && v1.tu == v2.tu
           && v1.tv == v2.tv;
    }
};

class D3DLVertex : public D3DLVERTEX {
public:
    D3DLVertex() {}
    D3DLVertex(
        const Vector& vec,
        const Color& colorArg,
        const Color& colorSpecular = Color(0, 0, 0),
        const Point& pointTexture = Point(0, 0)
    ) {
        dwReserved = 0;
        x          = vec.X();
        y          = vec.Y();
        z          = vec.Z();
        color      = MakeD3DCOLOR(colorArg);
        specular   = MakeD3DCOLOR(colorSpecular);
        tu         = pointTexture.X();
        tv         = pointTexture.Y();
    }

    float  X()                    const { return x;                   }
    float  Y()                    const { return y;                   }
    float  Z()                    const { return z;                   }
    Vector GetPosition()          const { return Vector(x, y, z);     }
    Point  GetTextureCoordinate() const { return Point(tu, tv);       }
    Color  GetColor()             const { return MakeColor(color);    }
    Color  GetSpecularColor()     const { return MakeColor(specular); }

    void SetPosition(const Vector& vec)
    {
        x = vec.X();
        y = vec.Y();
        z = vec.Z();
    }

    void SetNormal(const Vector& vec)
    {
    }

    Vector GetNormal() const
    {
        return Vector(1, 0, 0);
    }


    void SetTextureCoordinate(const Point& point)
    {
        tu = point.X();
        tv = point.Y();
    }

    void SetColor(const Color& colorArg)
    {
        color = MakeD3DCOLOR(colorArg);
    }

    void SetSpecularColor(const Color& colorArg)
    {
        specular = MakeD3DCOLOR(colorArg);
    }

    friend bool operator==(const D3DLVertex& v1, const D3DLVertex& v2)
    {
        return
              v1.x        == v2.x
           && v1.y        == v2.y
           && v1.z        == v2.z
           && v1.color    == v2.color
           && v1.specular == v2.specular
           && v1.tu       == v2.tu
           && v1.tv       == v2.tv;
    }
};


class D3DTLVertex : public D3DTLVERTEX {
public:
    D3DTLVertex() {}
    D3DTLVertex(
        const Point& point,
        float z,
        float rhwArg,
        const Color& colorArg,
        const Color& colorSpecular = Color::Black(),
        const Point& pointTexture = Point(0, 0)
    ) {
        sx         = point.X();
        sy         = point.Y();
        sz         = z;
        rhw        = rhwArg;
        color      = MakeD3DCOLOR(colorArg);
        specular   = MakeD3DCOLOR(colorSpecular);
        tu         = pointTexture.X();
        tv         = pointTexture.Y();
    }

    float  X()                    const { return sx;                  }
    float  Y()                    const { return sy;                  }
    float  Z()                    const { return sz;                  }
    float  RHW()                  const { return rhw;                 }
    Point  GetPosition()          const { return Point(sx, sy);       }
    Point  GetTextureCoordinate() const { return Point(tu, tv);       }
    Color  GetColor()             const { return MakeColor(color);    }
    Color  GetSpecularColor()     const { return MakeColor(specular); }

    void SetPosition(const Point& point)
    {
        sx = point.X();
        sy = point.Y();
    }

    void SetX(float xArg)
    {
        sx = xArg;
    }

    void SetY(float yArg)
    {
        sy = yArg;
    }

    void SetZ(float zArg)
    {
        sz = zArg;
    }

    void SetRHW(float rhwArg)
    {
        rhw = rhwArg;
    }

    void SetTextureCoordinate(const Point& point)
    {
        tu = point.X();
        tv = point.Y();
    }

    void SetColor(const Color& colorArg)
    {
        color = MakeD3DCOLOR(colorArg);
    }
   
    void SetSpecularColor(const Color& colorArg)
    {
        specular = MakeD3DCOLOR(colorArg);
    }

    friend bool operator==(const D3DTLVertex& v1, const D3DTLVertex& v2)
    {
        return
              v1.sx       == v2.sx
           && v1.sy       == v2.sy
           && v1.sz       == v2.sz
           && v1.rhw      == v2.rhw
           && v1.color    == v2.color
           && v1.specular == v2.specular
           && v1.tu       == v2.tu
           && v1.tv       == v2.tv;
    }
};


inline ZString GetString(const D3DLVertex& v)
{
    return
          ZString(v.x) + ", "
        + ZString(v.y) + ", "
        + ZString(v.z) + ", "
        + ZString(v.tu) + ", "
        + ZString(v.tv);
}

inline ZString GetString(const D3DVertex& v)
{
    return
          ZString(v.x) + ", "
        + ZString(v.y) + ", "
        + ZString(v.z) + ", "
        + ZString(v.tu) + ", "
        + ZString(v.tv);
}

struct VertexScreen {
    float    x;
    float    y;
    float    z;
    float    qw;
    D3DCOLOR color;
    D3DCOLOR specular;
    float    u;
    float    v;
};

//////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//
//////////////////////////////////////////////////////////////////////////////

enum ShadeMode {  
    ShadeModeNone,
    ShadeModeCopy,
    ShadeModeFlat,
    ShadeModeGouraud,
    ShadeModeGlobalColor
};

enum BlendMode {
    BlendModeSource,
    BlendModeAdd,
	BlendModeSourceAlpha
};

enum WrapMode {
    WrapModeNone,
    WrapModeUCylinder,
    WrapModeVCylinder,
    WrapModeTorus
};

enum CullMode {
    CullModeNone,
    CullModeCW,
    CullModeCCW,
    CullModeBoth
};

enum Counter {
    CounterPoints,
    CounterLines,
    CounterTriangles,
    CounterMeshes,
    CounterObjects,
    CounterObjectsRendered,
    CounterMatrixLoads,
    CounterDrawStrings,
    CounterDrawStringChars
};

//////////////////////////////////////////////////////////////////////////////
//
// Geo Callback
//
//////////////////////////////////////////////////////////////////////////////

class IGeoCallback : public IObject {
public:
    virtual void RenderCallback(Context* pcontext) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Deformation
//
//////////////////////////////////////////////////////////////////////////////

class Deformation : public IObject {
public:
    virtual Vector Deform(const Vector& vec) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Real coordinate system drawing context.
//
//////////////////////////////////////////////////////////////////////////////

class Context : public IObject {
public:
    //
    // Capabilities
    //

    virtual bool Has3DAcceleration() = 0;

    //
    // 3D Layers
    //

    virtual void Begin3DLayer(Camera* pcamera, bool bZBuffer) = 0;
    virtual void End3DLayer()                                 = 0;

    //
    // Level of Detail
    //

    virtual void  SetLOD(float lod) = 0;
    virtual float GetLOD()          = 0;

    //
    // These states are maintained in the attibutes stack.
    //

    virtual void PushState() = 0;
    virtual void PopState()  = 0;

    virtual void SetGlobalColor(const Color& color, bool bOverride = false)   = 0;
    virtual void SetZTest(bool b, bool bOverride = false)                     = 0;
    virtual void SetZWrite(bool b, bool bOverride = false)                    = 0;
    virtual void SetPerspectiveCorrection(bool b, bool bOverride = false)     = 0;
    virtual void SetDither(bool b, bool bOverride = false)                    = 0;
    virtual void SetColorKey(bool b, bool bOverride = false)                  = 0;
    virtual void SetLinearFilter(bool b, bool bOverride = false)              = 0;
    virtual void SetShadeMode(ShadeMode shadeMode, bool bOverride = false)    = 0;
    virtual void SetBlendMode(BlendMode blendMode, bool bOverride = false)    = 0;
    virtual void SetWrapMode(WrapMode wrapMode, bool bOverride = false)       = 0;
    virtual void SetCullMode(CullMode cullMode, bool bOverride = false)       = 0;
    virtual void SetMaterial(Material* pmaterial, bool bOverride = false)     = 0;
    virtual void SetTexture(Surface* psurface, bool bOverride = false)        = 0;
    virtual void SetLineWidth(float width, bool bOverride = false)            = 0;
    virtual void SetDeformation(Deformation* pdeform, bool bOverride = false) = 0;

    virtual const Color& GetGlobalColor()           const = 0;
    virtual bool         GetZTest()                 const = 0;
    virtual bool         GetZWrite()                const = 0;
    virtual bool         GetPerspectiveCorrection() const = 0;
    virtual bool         GetDither()                const = 0;
    virtual bool         GetColorKey()              const = 0;
    virtual bool         GetLinearFilter()          const = 0;
    virtual ShadeMode    GetShadeMode()             const = 0;
    virtual BlendMode    GetBlendMode()             const = 0;
    virtual CullMode     GetCullMode()              const = 0;
    virtual Material*    GetMaterial()              const = 0;
    virtual Surface*     GetTexture()               const = 0;
    virtual Deformation* GetDeformation()           const = 0;

    //
    // Lighting
    //

    virtual void DirectionalLight(const Vector& direction, const Color& color) = 0;
    virtual void BidirectionalLight(const Vector& direction, const Color& color, const Color& colorAlt) = 0;
    virtual void SetAmbientLevel(float ambientLevel) = 0;

    //
    // 3D Transforms
    //

	virtual void SetPerspectiveMatrix( Camera * pcamera) = 0;
	virtual void SetViewMatrix(Camera * pcamera) = 0;
    virtual void SetTransform(const Matrix& mat, const Matrix& matWorldTM)        = 0;

    virtual void Multiply(const Matrix& mat)            = 0;
    virtual void Rotate(const Vector& vec, float angle) = 0;
    virtual void Translate(const Vector& vec)           = 0;
    virtual void Scale(const Vector& vec)               = 0;
    virtual void Scale3(float scale)                    = 0;

    //
    // 2D Transforms
    //

    virtual void SetMatrix(const Matrix2& mat) = 0;
    virtual void Multiply(const Matrix2& mat)  = 0;
    virtual void Rotate(float angle)           = 0;
    virtual void Translate(const Point& vec)   = 0;
    virtual void Scale(const Point& point)     = 0;
    virtual void Scale2(float scale)           = 0;

    //
    // Transforms Applications
    //

    virtual const Matrix&   GetLocalToEyeMatrix()                               = 0;

    virtual Vector   TransformDirectionToEye(const Vector& vec)                 = 0;
    virtual bool     TransformDirectionToImage(const Vector& vec, Point& point) = 0;
    virtual Vector   TransformLocalToEye(const Vector& vec)                     = 0;
    virtual bool     TransformLocalToImage(const Vector& vec, Point& point)     = 0;
    virtual bool     TransformEyeToImage(const Vector& vec, Point& point)       = 0;
    virtual float    GetImageRadius(const Vector& vec, float radius)            = 0;

    virtual Vector   TransformEyeToLocal(const Vector& vec)                     = 0;

    //
    // 3D Clipping
    //

    virtual void Clip(const Rect& rect)  = 0;
    virtual void AddClipPlane(const Plane& plane) = 0;

    virtual bool IsCulled(const Vector& vecEye, float radius, bool& bOnScreen) = 0;
    virtual bool GetClipping()           = 0;
    virtual void SetClipping(bool bClip) = 0;

    //
    // Vertex Buffers
    //

    virtual Vertex*       GetVertexBuffer(int count)       = 0;
    virtual VertexL*      GetVertexLBuffer(int count)      = 0;
    virtual VertexScreen* GetVertexScreenBuffer(int count) = 0;
    virtual MeshIndex*    GetIndexBuffer(int count)        = 0;

    //
    // 2D Rendering
    //

    virtual void DrawImage(Surface* psurface, const Rect& rect, bool bCentered = false, const Point& point = Point(0, 0)) = 0;
    virtual void DrawImage(Surface* psurface, bool bCentered = false, const Point& point = Point(0, 0))   = 0;
    virtual void DrawImage3D(Surface* psurface, const Rect& rect, const Color& color, bool bCentered = false, const Point& point = Point(0, 0)) = 0;
    virtual void DrawImage3D(Surface* psurface, const Color& color, bool bCentered = false, const Point& point = Point(0, 0)) = 0;

    virtual void DrawString(
        IEngineFont*   pfont,
        const Color&   color,
        const Point&   point,
        const ZString& str
    ) = 0;
    virtual void DrawRectangle(const Rect& rect, const Color& color) = 0;
    virtual void FillRect(const Rect& rect, const Color& color) = 0;
    virtual void FillInfinite(const Color& color) = 0;

    //
    // 3D Rendering
    //

    virtual void DrawCallbackGeo(IGeoCallback* pgeo, bool bSortObject) = 0;

    virtual void DrawPoints(const D3DVertex*    pvertex, int vcount) = 0;
    virtual void DrawPoints(const Vertex*       pvertex, int vcount) = 0;
    virtual void DrawPoints(const VertexL*      pvertex, int vcount) = 0;
    virtual void DrawPoints(const VertexScreen* pvertex, int vcount) = 0;

    virtual void DrawLineStrip(const D3DVertex*    pvertex, int vcount) = 0;
    virtual void DrawLineStrip(const Vertex*       pvertex, int vcount) = 0;
    virtual void DrawLineStrip(const VertexL*      pvertex, int vcount) = 0;
    virtual void DrawLineStrip(const VertexScreen* pvertex, int vcount) = 0;

    virtual void DrawLines(const D3DVertex*    pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void DrawLines(const Vertex*       pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void DrawLines(const VertexL*      pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void DrawLines(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;

    virtual void DrawTriangles(const D3DVertex*    pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void DrawTriangles(const Vertex*       pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void DrawTriangles(const VertexL*      pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;
    virtual void DrawTriangles(const VertexScreen* pvertex, int vcount, const MeshIndex* pindex, int icount) = 0;

    //
    // Rendering with TVectors
    //

    virtual void DrawPoints(const TVector<VertexL>& vertices) = 0;

    virtual void DrawLines(const TVector<D3DVertex>& vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void DrawLines(const TVector<Vertex>&    vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void DrawLines(const TVector<VertexL>&   vertices, const TVector<MeshIndex>& indices) = 0;

    virtual void DrawTriangles(const TVector<D3DVertex>& vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void DrawTriangles(const TVector<Vertex>&    vertices, const TVector<MeshIndex>& indices) = 0;
    virtual void DrawTriangles(const TVector<VertexL>&   vertices, const TVector<MeshIndex>& indices) = 0;

	virtual void DrawTriangles(	const CVBIBManager::SVBIBHandle * phVB, 
								const CVBIBManager::SVBIBHandle * phIB ) = 0;

	//
    // Decals
    //

    virtual float DrawDecal(
        Surface* psurface,
        const Color& color,
        const Vector& position,
        const Vector& forward,
        const Vector& right,
        float scale,
        float angle,
        BlendMode blendMode = BlendModeAdd
    ) = 0;

    //
    // Performance Counters
    //

    #ifdef EnablePerformanceCounters
        virtual int GetPerformanceCounter(Counter counter) = 0;
        virtual void ResetPerformanceCounters() = 0;
    #endif
};

#endif
