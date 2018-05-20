#ifndef _efgeo_H_
#define _efgeo_H_

#include <event.h>
#include <model.h>
#include <tref.h>
#include <value.h>

//////////////////////////////////////////////////////////////////////////////
//
// effect geos
//
//////////////////////////////////////////////////////////////////////////////

class Surface;
class Viewport;
TRef<Geo> CreateDebrisGeo(Modeler* pmodeler, Number* ptime, Viewport* pviewport, TRef<ModifiableNumber> distancePerParticle);
TRef<Geo> CreateCullGeo(Geo* pgeo);
TRef<Geo> CreateCopyModeGeo(Geo* pgeo);
TRef<Geo> CreateWireSphereGeo(float minDot, int hSegments);
TRef<Geo> CreateBoltGeo(VectorValue* pstart, VectorValue* pend, float size, Surface* psurface);
TRef<Geo> CreateConeGeo(Number* pangle);

TRef<Geo> CreateTrailGeo(
    Modeler*      pmodeler,
    const Color&  color,
    VectorValue*  pvectorPosition,
    VectorValue*  pvectorRight,
    Boolean*      pbooleanMoving,
    Number*       ptime
);


//////////////////////////////////////////////////////////////////////////////
//
// Aleph Geo
//
//////////////////////////////////////////////////////////////////////////////

TRef<Geo> CreateAlephGeo(Modeler* pmodeler, TEvent<float>::Source* pevent, Number* ptime);

//////////////////////////////////////////////////////////////////////////////
//
// MineField Geo
//
//////////////////////////////////////////////////////////////////////////////

class MineFieldGeo : public Geo {
public:
    virtual void SetStrength(float strength) = 0;
};

TRef<MineFieldGeo> CreateMineFieldGeo(Surface* psurface, float strength, float radius);

//////////////////////////////////////////////////////////////////////////////
//
// Build Effect Geo
//
//////////////////////////////////////////////////////////////////////////////

class BuildEffectGeo : public Geo {
protected:
    BuildEffectGeo(Number* ptime) :
        Geo(ptime)
    {
    }

    Number* GetTime()      { return Number::Cast(GetChild(0)); }

public:
    virtual float         GetRadius()   = 0;
    virtual void          SetRadius(float r)   = 0;
    virtual const Vector& GetPosition() = 0;
    virtual void          SetPosition(const Vector& p) = 0;
    virtual void          SetColors(float aInner, float fInner, float fOuter) = 0;
};

TRef<BuildEffectGeo> CreateBuildEffectGeo(
    Modeler*      pmodeler,
    Number*       ptime,
    const Color&  color
);

//////////////////////////////////////////////////////////////////////////////
//
// Pulse Geo
//
//////////////////////////////////////////////////////////////////////////////

class PulseGeo : public Geo {
protected:
    PulseGeo(Number* ptime) :
        Geo(ptime)
    {
    }

    Number* GetTime()      { return Number::Cast(GetChild(0)); }

public:
    virtual void            AddPulse (float fExplodeTime, float fRadius, const Vector& vecPosition, const Color& color) = 0;
};

TRef<PulseGeo> CreatePulseGeo(Modeler* pmodeler, Number* ptime);

//////////////////////////////////////////////////////////////////////////////
//
// HullHitGeo
//
//////////////////////////////////////////////////////////////////////////////

class HullHitGeo : public Geo {
protected:
    HullHitGeo(Number* ptime) :
        Geo(ptime)
    {
    }

public:
    virtual void AddHullHit(const Vector& vecPosition, const Vector& vecNormal) = 0;
};

TRef<HullHitGeo> CreateHullHitGeo(Modeler* pmodeler, Number* ptime);

//////////////////////////////////////////////////////////////////////////////
//
// BitsGeo
//
//////////////////////////////////////////////////////////////////////////////

class BitsGeo : public Geo {
public:
    BitsGeo(Number* ptime) :
        Geo(ptime)
    {
    }

    virtual void AddBit(
        const Vector& vecPosition, 
        const Vector& dvecPosition,
        float size
    ) = 0;

    //
    // Value methods
    //

    ZString GetFunctionName() { return "BitsGeo"; }
};

TRef<BitsGeo> CreateBitsGeo(Modeler* pmodeler, Number* ptime);

#endif
