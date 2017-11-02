#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Mine Field Geo
//
//////////////////////////////////////////////////////////////////////////////

class MineFieldGeoImpl : public MineFieldGeo {
private:
    TRef<Surface>   m_psurface;
    TVector<Vector> m_vposition;
    float           m_radius;

    static int  GetCount(float strength, float radius)
    {
        return 1 + int(strength * radius * radius / 100.0f);
    }

public:
    MineFieldGeoImpl(Surface* psurface, float strength, float radius) :
        m_psurface(psurface),
        m_vposition(GetCount(strength, radius)),
        m_radius(radius)
    {
        for (int index = 0; index < m_vposition.GetCount(); index++) {
            m_vposition.Set(index, Vector::RandomPosition(radius));
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // MineFieldGeo methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetStrength(float strength)
    {
        int count = GetCount(strength, m_radius);

        ZAssert(count <= m_vposition.GetCount());

        m_vposition.SetCount(count);
        Changed();        
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Geo Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    float GetRadius(const Matrix& mat)
    {
        return m_radius;
    }

    void Render(Context* pcontext)
    {
        Vector vecEye = pcontext->TransformLocalToEye(Vector::GetZero());
        float  bright = bound(100.0f / vecEye.Length(), 0.0f, 1.0f);

        if (bright > 0) {
            Color  color(bright, bright, bright);

            int count = m_vposition.GetCount();

            for (int index = 0; index < count; index++) {
                pcontext->DrawDecal(
                    m_psurface,
                    color,
                    m_vposition[index],
                    Vector::GetZero(),
                    Vector::GetZero(),
                    1,
                    0,
                    BlendModeSourceAlpha //this actually uses BlendModeSourceAlpha now - imago 7/16/09
                );
            }
        }
    }
};

TRef<MineFieldGeo> CreateMineFieldGeo(Surface* psurface, float strength, float radius) 
{
    return new MineFieldGeoImpl(psurface, strength, radius);
}

//////////////////////////////////////////////////////////////////////////////
//
// Build Effect Geo
//
//////////////////////////////////////////////////////////////////////////////

const float c_ratio = 1.1f;

class BuildEffectGeoImpl : 
    public BuildEffectGeo,
    public IGeoCallback
{
private:
    Color     m_colorInner;
    Color     m_colorOuter;
    Color     m_colorComplement;
    TRef<Geo> m_psphere;
    
    float     m_time;
    float     m_radius;
    float     m_fOuter;
    Vector    m_position;

public:
    BuildEffectGeoImpl(
        Modeler*      pmodeler,
        Number*       ptime,
        const Color&  color
    ) :
        BuildEffectGeo(ptime),
        m_colorOuter(color)
    {
        float h;
        float s;
        float b;

        m_colorOuter.GetHSB(h, s, b);
        m_colorComplement.SetHSBA(h + 0.5f, s, b);

		bool bOldValue = pmodeler->SetColorKeyHint( false );
        m_psphere = pmodeler->LoadGeo("build");
		pmodeler->SetColorKeyHint( bOldValue );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // BuildEffectGeo Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    float GetRadius()
    {
        return m_radius;
    }
    void  SetRadius(float f)
    {
        m_radius = f;
    }

    const Vector& GetPosition()
    {
        return m_position;
    }
    void          SetPosition(const Vector& p)
    {
        m_position = p;
    }

    void    SetColors(float aInner, float fInner, float fOuter)
    {
        m_colorInner = m_colorComplement * fInner;
        m_colorInner.SetAlpha(aInner);

        m_fOuter = fOuter;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Geo Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void Evaluate()
    {
        m_time      = GetTime()->GetValue();
    }

    void Render(Context* pcontext)
    {
        pcontext->DrawCallbackGeo(this, true);
    }

    void RenderCallback(Context* pcontext)
    {
        //
        // Rendering modes
        //

        pcontext->SetBlendMode(BlendModeSourceAlpha);
        pcontext->SetShadeMode(ShadeModeGlobalColor);

        //
        //
        //

        pcontext->Translate(m_position);
        pcontext->Scale3(m_radius);

        //
        // inner sphere
        //

        pcontext->PushState();
        pcontext->Scale3(1.0f / c_ratio);
        pcontext->Rotate(Vector(0, 0, 1), m_time / pi);
        pcontext->Rotate(Vector(0, 1, 0), 0.5f * pi * m_time);

        pcontext->SetGlobalColor(m_colorInner);
        m_psphere->Render(pcontext);

        pcontext->PopState();

        //
        // outer sphere
        //

        pcontext->Rotate(Vector(1, 0, 0), m_time);
        pcontext->Rotate(Vector(0, 1, 0), pi * m_time);

        pcontext->SetBlendMode(BlendModeAdd);
        pcontext->SetGlobalColor(m_colorOuter * m_fOuter);
        m_psphere->Render(pcontext);
    }
};

TRef<BuildEffectGeo> CreateBuildEffectGeo(
    Modeler*      pmodeler,
    Number*       ptime,
    const Color&  color
) {
    return 
        new BuildEffectGeoImpl(
            pmodeler,
            ptime,
            color
        );
}

//////////////////////////////////////////////////////////////////////////////
//
// WireSphere
//
//////////////////////////////////////////////////////////////////////////////

class WireSphereGeo : public Geo {
private:
    TVector<VertexL>   m_vertices;
    TVector<MeshIndex> m_indices;

    static const int vSegments;
public:
    WireSphereGeo(float minDot, int hSegments) :
        m_vertices(
            hSegments * (vSegments - 1),
            hSegments * (vSegments - 1)
        ),
        m_indices(
            2 * hSegments * (vSegments * 2 - 3),
            2 * hSegments * (vSegments * 2 - 3)
        )
    {
        //
        // Create the vertex array
        //

        int vIndex;
        int hIndex;

        for (vIndex = 0; vIndex < vSegments - 1; vIndex++)
        {
            float vSin = minDot - float(vIndex) * 0.05f;
            float vCos = float(sqrt(1.0f - vSin * vSin));

            for (hIndex = 0; hIndex < hSegments; hIndex++)
            {
                int index  = vIndex * hSegments + hIndex;
                float hAngle = (float)hIndex * 2.0f * pi / (float)hSegments;

                float x = cos(hAngle) * vCos;
                float y = sin(hAngle) * vCos;
                float z =              -vSin;

                static const float intensity = 1.0f;

                m_vertices.Set(
                    index,
                    VertexL(
                        1000 * x,
                        1000 * y,
                        1000 * z,
                        intensity,
                        intensity,
                        intensity,
                        1,
                        0,
                        0
                    )
                );
            }
        }

        //
        // Vertical lines
        //

        int index = 0;

        for (vIndex = 0; vIndex < vSegments - 2; vIndex++) {
            for (hIndex = 0; hIndex < hSegments; hIndex++) {
                m_indices.Set(index    ,       vIndex * hSegments + hIndex);
                m_indices.Set(index + 1, (vIndex + 1) * hSegments + hIndex);
                ZAssert(m_indices[index    ] >= 0 && m_indices[index    ] < m_vertices.GetCount());
                ZAssert(m_indices[index + 1] >= 0 && m_indices[index + 1] < m_vertices.GetCount());
                index += 2;
            }
        }

        //
        // Horizontal lines
        //

        for (vIndex = 0; vIndex < vSegments - 1; vIndex++) {
            for (hIndex = 0; hIndex < hSegments; hIndex++) {
                m_indices.Set(index, vIndex * hSegments + hIndex);
                m_indices.Set(index + 1,
                    hIndex == hSegments - 1 ?
                          vIndex * hSegments
                        : vIndex * hSegments + hIndex + 1
                );
                ZAssert(m_indices[index    ] >= 0 && m_indices[index    ] < m_vertices.GetCount());
                ZAssert(m_indices[index + 1] >= 0 && m_indices[index + 1] < m_vertices.GetCount());
                index += 2;
            }
        }

        ZAssert(index == m_indices.GetCount());
    }

    void Render(Context* pcontext)
    {
        pcontext->DrawLines(m_vertices, m_indices);
    }

    //
    // Value members
    //

    ZString GetFunctionName() { return "WireSphereGeo"; }
};
const int WireSphereGeo::vSegments = 3;

TRef<Geo> CreateWireSphereGeo(float minDot, int hSegments)
{
    return new WireSphereGeo(minDot, hSegments);
}

//////////////////////////////////////////////////////////////////////////////
//
// Pulse Geo
//
//////////////////////////////////////////////////////////////////////////////

class PulseGeoImpl : 
    public PulseGeo,
    public IGeoCallback
{
protected:
    class WhiteSurfaceSite : public SurfaceSite {
    public:
        void UpdateSurface(Surface* psurface) 
        {
            psurface->FillSurface(Color::White());
        }
    };

    struct  PulseData : public IObject
    {
        float     m_fRadius;
        Vector    m_vecPosition;
        Color     m_color;
        float     m_fStartTime;
        float     m_fExplodeTime;
        PulseData (float fStartTime, float fExplodeTime, float fRadius, const Vector& vecPosition, const Color& color)
        {
            m_fRadius = fRadius;
            m_vecPosition = vecPosition;
            m_color = color;
            m_fStartTime = fStartTime;
            m_fExplodeTime = fExplodeTime;
        }
    };

    TList<TRef<PulseData> > m_pulseData;
    float                   m_fTime;
    TRef<Geo>               m_pSphere;
    TRef<Surface>           m_pTexture;

public:
    PulseGeoImpl (Modeler* pmodeler, Number* ptime) : PulseGeo (ptime)
    {
        m_pSphere = pmodeler->LoadGeo("aleph_sphere");
        m_pTexture = pmodeler->GetEngine ()->CreateSurface (WinPoint (16, 16), SurfaceType2D (), new WhiteSurfaceSite());
    }

    void    AddPulse (float fExplodeTime, float fRadius, const Vector& vecPosition, const Color& color) 
    {
        float   fStartTime = GetTime()->GetValue();
        m_pulseData.PushFront (new PulseData (fStartTime, fExplodeTime, (fRadius / m_pSphere->GetRadius (Matrix::GetIdentity ())) * 0.5f, vecPosition, color));
    }

    void    Evaluate()
    {
        m_fTime = GetTime()->GetValue();
    }

    void    Render(Context* pcontext)
    {
        pcontext->DrawCallbackGeo(this, true);
    }

    void    RenderCallback(Context* pcontext)
    {
        TList<TRef<PulseData> >::Iterator   iter (m_pulseData);

        // Rendering modes
        pcontext->SetBlendMode (BlendModeAdd);
        pcontext->SetShadeMode (ShadeModeGlobalColor);
        pcontext->SetTexture (m_pTexture);

        while (! iter.End ())
        {
            TRef<PulseData>  pGeo = iter.Value ();

            // compute the time
            float   fTimeUntilExplosion = pGeo->m_fExplodeTime - (m_fTime -  pGeo->m_fStartTime);
            float   fEffectDuration = 0.5f;
            if (fTimeUntilExplosion <= fEffectDuration)
            {
                if (fTimeUntilExplosion <= -fEffectDuration)
                {
                    // the effect has expired, so remove this node and move on to the next one
                    iter.Remove ();
                }
                else
                {
                    // compute the factor that gives us the growth and opacity of the effect
                    float   fEffectScale = 1.0f - fabsf (fTimeUntilExplosion / fEffectDuration);
                    assert (fEffectScale <= 1.0f);
                    fEffectScale *= fEffectScale;

                    // transformations
                    pcontext->Translate(pGeo->m_vecPosition);
                    pcontext->Scale3(pGeo->m_fRadius * fEffectScale);

                    // do the rendering
                    pcontext->PushState ();
                    pcontext->SetGlobalColor (Color (pGeo->m_color.R () * fEffectScale, pGeo->m_color.G () * fEffectScale, pGeo->m_color.B () * fEffectScale, fEffectScale));
                    m_pSphere->Render (pcontext);
                    pcontext->PopState ();

                    // skip to the next node in the list
                    iter.Next ();
                }
            }
            else
            {
                // skip to the next node in the list
                iter.Next ();
            }
        }
    }
};

TRef<PulseGeo> CreatePulseGeo(Modeler* pmodeler, Number* ptime)
{
    return new PulseGeoImpl (pmodeler, ptime);
}

//////////////////////////////////////////////////////////////////////////////
//
// Trail Geo
//
//////////////////////////////////////////////////////////////////////////////

const int maxSections = 16;
const float timeDelta = 0.5f;

class TrailGeo : public Geo, public IGeoCallback {
private:
    TVector<VertexL>   m_vertices;
    TVector<MeshIndex> m_indices;
    int                m_section;
    float              m_timeLast;
    Color              m_color;
    TRef<Image>        m_pimage;
    Vector             m_vecRightLast;

    Number*       GetTime()     { return      Number::Cast(GetChild(0)); }
    VectorValue*  GetPosition() { return VectorValue::Cast(GetChild(1)); }
    VectorValue*  GetRight()    { return VectorValue::Cast(GetChild(2)); }
    Boolean*      GetMoving()   { return     Boolean::Cast(GetChild(3)); }

public:
    TrailGeo(
        Modeler*     pmodeler,
        const Color& color,
        VectorValue* pvectorPosition,
        VectorValue* pvectorRight,
        Boolean*     pbooleanMoving,
        Number*      ptime
    ) :
        Geo(ptime, pvectorPosition, pvectorRight, pbooleanMoving),
        m_color(color),
        m_vecRightLast(pvectorRight->GetValue()),
        m_vertices((maxSections + 1) * 2, (maxSections + 1) * 2),
        m_indices(maxSections * 6, maxSections * 6),
        m_section(0),
        m_timeLast(-1)
    {
        m_pimage = pmodeler->LoadImage(AWF_EFFECT_TRAIL, true);

        int index;

        for(index = 0; index < maxSections; index ++) {
            int indexVertex = index * 2;
            int indexIndex  = index * 6;

            m_indices.Set(indexIndex + 0, indexVertex + 0);
            m_indices.Set(indexIndex + 1, indexVertex + 2);
            m_indices.Set(indexIndex + 2, indexVertex + 3);

            m_indices.Set(indexIndex + 3, indexVertex + 0);
            m_indices.Set(indexIndex + 4, indexVertex + 3);
            m_indices.Set(indexIndex + 5, indexVertex + 1);
        }

        int count = m_vertices.GetCount();
        for(index = 0; index < count; index += 2) {
            m_vertices.Get(index    ).SetTextureCoordinate(Point(0, 0));
            m_vertices.Get(index + 1).SetTextureCoordinate(Point(1, 0));
        }
    }

    void UpdateColors(float time)
    {
        if (m_section != 0) {
            float dtime = (time - m_timeLast) / (timeDelta * maxSections);

            float bright  = 1 - dtime;
            float dbright = 1.0f / maxSections;

            for (int index = 0; index <= m_section; index++) {
                Color color = m_color * bright;

                bright -= dbright;
                if (bright < 0) {
                    bright = 0;
                }

                m_vertices.Get(index * 2    ).SetColor(color);
                m_vertices.Get(index * 2 + 1).SetColor(color);
            }
        }
    }

    void Evaluate()
    {
              float   time     = GetTime()->GetValue();
        const Vector& vecPos   = GetPosition()->GetValue();
        const Vector& vecRight = GetRight()->GetValue();
              bool    bMoving  = GetMoving()->GetValue();

        if (vecPos != Vector(0, 0, 0)) {
            if (m_timeLast == -1) {
                m_timeLast = time;
            } else {
                if (time - m_timeLast > timeDelta) {
                    m_timeLast = time;

                    if (m_section < maxSections) {
                        m_section++;
                    }

                    for(int index = m_section * 2 + 1; index > 1; index--) {
                        m_vertices.Set(index, m_vertices[index - 2]);
                    }
                }
            }

            if (bMoving) {
                m_vecRightLast = vecRight;
            }

            m_vertices.Get(0).SetPosition(vecPos - m_vecRightLast);
            m_vertices.Get(1).SetPosition(vecPos + m_vecRightLast);

            UpdateColors(time);
        }
    }

    void Render(Context* pcontext)
    {
        pcontext->DrawCallbackGeo(this, true);
    }

    void RenderCallback(Context* pcontext)
    {
        pcontext->SetMaterial(NULL);
        pcontext->SetShadeMode(ShadeModeGouraud);
        pcontext->SetCullMode(CullModeNone);
        pcontext->SetBlendMode(BlendModeAdd);
        pcontext->SetZWrite(false);
        pcontext->SetTexture(m_pimage->GetSurface());

        if (m_section != 0) {
            pcontext->DrawTriangles(
                &m_vertices[0],
                (m_section + 1) * 2,
                &m_indices[0],
                m_section * 6
            );
        }
    }
};

TRef<Geo> CreateTrailGeo(
    Modeler*     pmodeler,
    const Color& color,
    VectorValue* pvectorPosition,
    VectorValue* pvectorRight,
    Boolean*     pbooleanMoving,
    Number*      ptime
) {
    return new TrailGeo(pmodeler, color, pvectorPosition, pvectorRight, pbooleanMoving, ptime);
}

//////////////////////////////////////////////////////////////////////////////
//
// HullHitGeo
//
//////////////////////////////////////////////////////////////////////////////

class HullHitGeoImpl : public HullHitGeo {
private:
    class HullHitData {
    public:
        Vector m_position;
        Vector m_normal;
        Vector m_forward;
        Vector m_right;
        float  m_timeStart;
    };

    typedef TList<HullHitData, DefaultNoEquals> HullHitDataList;

    HullHitDataList m_listHullHitData;
    TRef<Surface>   m_psurfaceNormal;
    TRef<Surface>   m_psurfaceTangent;

    Number*      GetTime()     { return      Number::Cast(GetChild(0)); }

public:
    HullHitGeoImpl(Modeler* pmodeler, Number* ptime) :
        HullHitGeo(ptime)
    {
        m_psurfaceNormal  = pmodeler->LoadSurface(AWF_EFFECT_NORMAL,  true);
        m_psurfaceTangent = pmodeler->LoadSurface(AWF_EFFECT_TANGENT, true);
    }

    void AddHullHit(const Vector& vecPosition, const Vector& vecNormal)
    {
        m_listHullHitData.PushFront();

        HullHitData& data = m_listHullHitData.GetFront();

        data.m_position = vecPosition;
        data.m_normal   = vecNormal;

        if (vecNormal == Vector(0, 0, 1)) {
            data.m_forward  = CrossProduct(vecNormal, Vector(0, 1, 0)).Normalize();
        } else {
            data.m_forward  = CrossProduct(vecNormal, Vector(0, 0, 1)).Normalize();
        }

        data.m_right = CrossProduct(data.m_forward, vecNormal);

        data.m_timeStart = GetTime()->GetValue();
    }

    void Render(Context* pcontext)
    {
        float timeCurrent = GetTime()->GetValue();

        HullHitDataList::Iterator iter(m_listHullHitData);

        while (!iter.End()) {
            HullHitData& data = iter.Value();
            Color color(1, 1, 1);

            float maxSize = 2.0f;
            float scale = 0.1f + maxSize * 5.0f * (timeCurrent - data.m_timeStart);

            if (scale > maxSize) {
                scale = maxSize;
            }

            //
            // draw the tangent decal
            //

            pcontext->DrawDecal(
                m_psurfaceTangent,
                color,
                data.m_position,
                scale * data.m_forward,
                scale * data.m_right,
                1,
                0
            );

            //
            // draw the normal decal
            //

            pcontext->DrawDecal(
                m_psurfaceNormal,
                color,
                data.m_position + scale * data.m_normal,
                scale * data.m_normal,
                Vector(0, 0, 0),
                1,
                0
            );

            //
            // Remove it if it's reached fullsize
            //

            if (scale == maxSize) {
                iter.Remove();
            } else {
                iter.Next();
            }
        }
    }
};

TRef<HullHitGeo> CreateHullHitGeo(Modeler* pmodeler, Number* ptime)
{
    return new HullHitGeoImpl(pmodeler, ptime);
}

//////////////////////////////////////////////////////////////////////////////
//
// BitsGeo
//
//////////////////////////////////////////////////////////////////////////////

const int g_maxBits      = 32;
const int g_bitsLifeTime = 2;

class BitsGeoImpl : public BitsGeo {
private:
    typedef TRange<g_maxBits> BitsRange;

    class BitsData {
    public:
        Vector m_vecPosition;
        Vector m_dvecPosition;
        Vector m_axis;
        float  m_speed;
        float  m_time;
        float  m_size;
    };

    BitsData    m_data[g_maxBits];
    TRef<Geo>   m_pgeo;
    BitsRange   m_indexFirst;
    BitsRange   m_indexLast;
    float       m_time;

    Number* GetTime() { return Number::Cast(GetChild(0)); }

public:
    BitsGeoImpl(Modeler* pmodeler, Number* ptime) :
        BitsGeo(ptime),
        m_indexFirst(0),
        m_indexLast(0),
        m_time(ptime->GetValue())
    {
        m_pgeo = pmodeler->LoadGeo(AWF_EFFECT_BIT);
    }

    void AddBit(
        const Vector& vecPosition,
        const Vector& dvecPosition,
        float size
    ) {
        --m_indexFirst;

        if (m_indexFirst == m_indexLast) {
            --m_indexLast;
        }

        m_data[m_indexFirst].m_time         = m_time;
        m_data[m_indexFirst].m_vecPosition  = vecPosition;
        m_data[m_indexFirst].m_dvecPosition = dvecPosition;
        m_data[m_indexFirst].m_axis         = Vector::RandomDirection();
        m_data[m_indexFirst].m_speed        = random(pi /2, 2 * pi);
        m_data[m_indexFirst].m_size         = size;
    }

    void Evaluate()
    {
        m_time = GetTime()->GetValue();
    }

    void Render(Context* pcontext)
    {
        if (m_indexFirst != m_indexLast) {

            for(BitsRange index = m_indexFirst; index != m_indexLast; ++index) {
                BitsData& data = m_data[index];

                float dtime = m_time - data.m_time;

                if (dtime > g_bitsLifeTime) {
                    m_indexLast = index;
                    break;
                }

                Vector position = data.m_vecPosition + data.m_dvecPosition * dtime;

                pcontext->PushState();
                pcontext->Translate(position);
                pcontext->Scale3(data.m_size);
                pcontext->Rotate(data.m_axis, data.m_speed * dtime);
                m_pgeo->Render(pcontext);
                pcontext->PopState();
            }
        }
    }
};

TRef<BitsGeo> CreateBitsGeo(Modeler* pmodeler, Number* ptime)
{
    return new BitsGeoImpl(pmodeler, ptime);
}

//////////////////////////////////////////////////////////////////////////////
//
// BoltGeo
//
//////////////////////////////////////////////////////////////////////////////

class BoltGeo : public Geo, IGeoCallback {
private:
    TRef<Surface> m_psurface;
    float         m_size;

    VectorValue* GetStart() { return VectorValue::Cast(GetChild(0)); }
    VectorValue* GetEnd()   { return VectorValue::Cast(GetChild(1)); }

public:
    BoltGeo(VectorValue* pstart, VectorValue* pend, float size, Surface* psurface) :
        Geo(pstart, pend),
        m_size(size),
        m_psurface(psurface)
    {
    }

    void Render(Context* pcontext)
    {
        pcontext->DrawCallbackGeo(this, false);
    }

    void RenderCallback(Context* pcontext)
    {
        int index;

        //
        // Find two orthogonal vectors
        //

        Vector vecStart   = GetStart()->GetValue();
        Vector vecEnd     = GetEnd()->GetValue();
        Vector vecForward = vecEnd - vecStart;
        Vector vecUp      = vecForward.GetOrthogonalVector().Normalize();
        Vector vecRight   = CrossProduct(vecForward, vecUp).Normalize();
        float  length     = vecForward.Length();
        float  size       = m_size * length;

        //
        // Calculate the level of detail
        //

        float lengthImage =
            max(
                pcontext->GetImageRadius(vecStart, size),
                pcontext->GetImageRadius(vecEnd, size)
            );

        int lcount;
        if (lengthImage > 128) {
            lcount = 6;
        } else if (lengthImage > 64) {
            lcount = 5;
        } else if (lengthImage > 32) {
            lcount = 4;
        } else {
            lcount = 3;
        }

        //
        // color is based on size
        //

        float bright = min(1.0f, lengthImage / 128);
        Color color(bright, bright, bright);

        //
        // allocate the vertex and index buffers
        //

        int vcount = (1 << lcount) + 1;
        VertexL* pvertex = pcontext->GetVertexLBuffer(vcount);

        for (index = 0; index < vcount; index++) {
            pvertex[index].SetColor(color);
            pvertex[index].SetTextureCoordinate(
                Point(
                    (float)index / (float)(vcount - 1),
                    0
                )
            );
        }

        //
        // sub divide
        //

        pvertex[0].SetPosition(vecStart);
        pvertex[vcount - 1].SetPosition(vecEnd);

        for (int level = lcount; level > 0 ; level--) {
            int dbig   = 1 << level;
            int dsmall = dbig / 2;

            for (index = dsmall; index < vcount; index += dbig) {
                float angle = random(0, 2 * pi);

                VertexL& vecFirst = pvertex[index - dsmall];
                VertexL& vecLast  = pvertex[index + dsmall];

                Vector vecMiddle = 0.5f * (vecFirst.GetPosition() + vecLast.GetPosition());
                Vector vecRandom = size * (cos(angle) * vecUp + sin(angle) * vecRight);

                pvertex[index].SetPosition(vecMiddle + vecRandom);
            }

            size = size / 2;
        }

        //
        // draw it
        //

        pcontext->SetTexture(m_psurface);
        pcontext->SetLineWidth(4);
        pcontext->SetBlendMode(BlendModeAdd);
        pcontext->DrawLineStrip(pvertex, vcount);
    }
};

TRef<Geo> CreateBoltGeo(VectorValue* pstart, VectorValue* pend, float size, Surface* psurface)
{
    return new BoltGeo(pstart, pend, size, psurface);
}

//////////////////////////////////////////////////////////////////////////////
//
// CullGeo
//
//////////////////////////////////////////////////////////////////////////////

class CullGeo : public WrapGeo {
private:
    float m_radius;

public:
    CullGeo(Geo* pgeo) :
        WrapGeo(pgeo)
    {
        m_radius = GetGeo()->GetRadius(Matrix::GetIdentity());
    }

    void Render(Context* pcontext)
    {
        bool bNoClipping;

        if (!pcontext->IsCulled(Vector::GetZero(), m_radius, bNoClipping)) {
            if (bNoClipping) {
                pcontext->SetClipping(false);
            }

            //float screenRadius = pcontext->GetScreenRadius(Vector::GetZero(), m_radius);
            //pcontext->SetLOD(screenRadius * max(m_lodBiasMin, s_lodBias));

            GetGeo()->Render(pcontext);

            if (bNoClipping) {
                pcontext->SetClipping(true);
            }
        }
    }
};

TRef<Geo> CreateCullGeo(Geo* pgeo)
{
    return new CullGeo(pgeo);
}

//////////////////////////////////////////////////////////////////////////////
//
// CopyModeGeo
//
//////////////////////////////////////////////////////////////////////////////

class CopyModeGeo : public WrapGeo {
public:
    CopyModeGeo(Geo* pgeo) :
        WrapGeo(pgeo)
    {
    }

    void Render(Context* pcontext)
    {
        #ifdef FixPermedia
            pcontext->SetShadeMode(ShadeModeGlobalColor);
            pcontext->SetGlobalColor(Color::White());
        #else
            pcontext->SetShadeMode(ShadeModeCopy);
        #endif
        GetGeo()->Render(pcontext);
    }
};

TRef<Geo> CreateCopyModeGeo(Geo* pgeo)
{
    return new CopyModeGeo(pgeo);
}
