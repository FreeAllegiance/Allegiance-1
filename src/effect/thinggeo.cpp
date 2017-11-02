//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
//#include "regkey.h"

//////////////////////////////////////////////////////////////////////////////
//
// Reference to objects used by ThingGeo.  It doesn't free the object on destruction
//
//////////////////////////////////////////////////////////////////////////////

template <class Type>
class ThingRef {
private:
    Type* m_pt;

public:
    //
    // constructors
    //

    ThingRef() :
        m_pt(NULL)
    {
    }

    ThingRef(const Type* pt)
    {
        m_pt = (Type*)pt;
        if (m_pt)
            m_pt->AddRef();
    }

    ThingRef(const ThingRef& tref) :
        m_pt(tref.m_pt)
    {
        if (m_pt)
            m_pt->AddRef();
    }
    
    //
    // assignment
    //

    ThingRef* Pointer()
    {
        return this;
    }

    Type** operator&()
    {
        if (m_pt) {
            m_pt->Release();
            m_pt = NULL;
        }
        return &m_pt;
    }

    ThingRef& operator=(const ThingRef& tref)
    {
        Type* ptOld = m_pt;
        m_pt = tref.m_pt;

        if (m_pt)
            m_pt->AddRef();

        if (ptOld)
            ptOld->Release();

        return *this;
    }

    ThingRef& operator=(const Type* ptNew)
    {
        Type* ptOld = m_pt;
        m_pt = (Type*)ptNew;

        if (m_pt)
            m_pt->AddRef();

        if (ptOld)
            ptOld->Release();

        return *this;
    }

    //
    // destructor
    //

    ~ThingRef(void)
    {
        /* don't release on destruct
        if (m_pt)
            m_pt->Release();
        */
    }

    //
    // members
    //

    operator Type*(void) const { return m_pt; }
    Type& operator*(void) const { return *m_pt; }
    Type* operator->(void) const { return m_pt; }

    friend bool operator==(const ThingRef& t1, Type* pt)
    {
        return t1.m_pt == pt;
    }

    friend bool operator!=(const ThingRef& t1, Type* pt)
    {
        return t1.m_pt != pt;
    }


    // define comparison operators for use in associative containers
    inline friend bool operator<(const ThingRef& t1, Type* pt) { return t1.m_pt < pt; };
    inline friend bool operator>(const ThingRef& t1, Type* pt) { return t1.m_pt > pt; };
    inline friend bool operator<=(const ThingRef& t1, Type* pt) { return t1.m_pt <= pt; };
    inline friend bool operator>=(const ThingRef& t1, Type* pt) { return t1.m_pt >= pt; };
};

//////////////////////////////////////////////////////////////////////////////
//
// ThingGeo
//
//////////////////////////////////////////////////////////////////////////////

class ThingGeoImpl : public ThingGeo
{
private:
    ThingRef<Modeler>           m_pmodeler;
    ThingRef<Number>            m_ptime;

    float                       m_timeLast;
    float                       m_time;

    float                       m_radius;
    float                       m_radiusMesh;
    float                       m_radiusCull;

    ThingRef<FrameDataListValue> m_pframes;

    Orientation                 m_orientation;
    Vector                      m_vecPosition;
    Vector                      m_vecPositionLast;
    Vector                      m_vecVelocityLast;

    ThingRef<Geo>               m_pgeoTrail;
    ThingRef<Geo>               m_pgeoMesh;
    ThingRef<HullHitGeo>        m_phullHitGeo;
    ThingRef<Geo>               m_pgeoLights;
    ThingRef<Geo>               m_pgeoBounds;

    bool                        m_bVisibleShip;
    bool                        m_bVisible;
    bool                        m_bClip;
    bool                        m_bDrawLights;
    bool                        m_bFlip;
    bool                        m_bShadeAlways;
    float                       m_scale;
    ThingRef<Surface>           m_psurfaceTexture;

    //
    // Trail
    //

    ThingRef<ModifiableVectorValue> m_pvectorPositionTrail;
    ThingRef<ModifiableVectorValue> m_pvectorRightTrail;
    ThingRef<ModifiableBoolean>     m_pbooleanMoving;
    Vector                          m_vecTrailOffset;

    //
    // Shield Flares
    //

    float                       m_countFlares;
    ThingRef<GroupGeo>          m_pgroupFlares;

    //
    // Thrust
    //

    float                       m_fThrustPower;
    Vector                      m_vecThrustDirection;
    ThingRef<ParticleGeo>       m_pParticleGeo;
    ThingRef<BitsGeo>           m_pbitsGeo;
    ThingRef<Image>             m_pimageGlow;
    float                       m_scaleGlow;
    Color                       m_colorGlow;
    float                       m_sizeSmoke;
    float                       m_fAfterburnerFireDuration;
    float                       m_fAfterburnerSmokeDuration;

    //
    // Damage
    //
    #define MAX_DAMAGE_FRAMES   10
    FrameData                  m_damageFrames[MAX_DAMAGE_FRAMES]; // BT - 10/17 - Made stack object instead of heap object, this enabled SEH to be used.
    int                         m_iLastDamageIndex;
    bool                        m_bShowDamage;

    //
    // Ripcord and Teleport
    //
    #define TELEPORT_SPARK_COUNT        3
    #define TIME_FOR_TELEPORT_SPARKS    3
    float                       m_fTimeUntilRipcord;
    float                       m_fTimeUntilAleph;

private:
    /* Imago removed, now uses Performance 8/16/09
    DWORD LoadPreference(const ZString& szName, DWORD dwDefault)
    {
        HKEY hKey;
        DWORD dwResult = dwDefault;

        if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ, &hKey))
        {
            DWORD dwSize = sizeof(dwResult);
            DWORD dwType = REG_DWORD;

            ::RegQueryValueEx(hKey, szName, NULL, &dwType, (BYTE*)&dwResult, &dwSize);
            ::RegCloseKey(hKey);

            if (dwType != REG_DWORD)
                dwResult = dwDefault;
        }

        return dwResult;
    }
    */

public:
    ThingGeoImpl(Modeler* pmodeler, Number* ptime) :
        m_ptime(ptime),
        m_pmodeler(pmodeler),
        m_bVisible(true),
        m_bVisibleShip(true),
        m_countFlares(0),
        m_sizeSmoke(4.0f),
        m_fAfterburnerFireDuration(0.333f),
        m_fAfterburnerSmokeDuration(2.0f),
        m_scale(1),
        m_bShadeAlways(false),
        m_iLastDamageIndex(-1),
        m_bShowDamage(true),
        m_fTimeUntilRipcord(-1.0f),
        m_fTimeUntilAleph(-1.0f)
    {
		// BT - 10/17 - Don't need to new this, just assign with fixed size.
        //m_damageFrames = new FrameData[MAX_DAMAGE_FRAMES];

        //
        // Create the Geometric Hierarchy with an EmptyGeo at the leaf
        //

        m_time         = m_ptime->GetValue();
        m_vecPosition  = Vector(0, 0, 0);
        m_pgroupFlares = GroupGeo::Create();
        m_phullHitGeo  = CreateHullHitGeo(pmodeler, ptime);

        //
        // thrust
        //

        m_pimageGlow = pmodeler->LoadImage(AWF_EFFECT_AFTERBURNER_GLOW, true);
        SetAfterburnerThrust(Vector::GetZero(), 0.0f);
		SetThrust(0);

        m_timeLast        = m_time;
        m_vecPositionLast = m_vecPosition;
        m_vecVelocityLast = m_vecPosition - m_vecPosition;

        // here, check the registry for a private key - if set, use a different
        // afterburner effect, more like a rocket - one that won't drown performance
        //Imago 8/16/09 now uses Performance setting
        //m_bUsePrivateAfterburners = LoadPreference ("PrivateAfterburners", 0);
        if (s_bUsePrivateAfterburners)
        {
            m_sizeSmoke = 2.0f;
            m_fAfterburnerFireDuration = 0.33f;
            m_fAfterburnerSmokeDuration = 0.66f;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////

    ~ThingGeoImpl()
    {
        //
        // This destructor crashes sometimes.  If it does, just continue.
        //

		// BT - 10/17 - None of this needs to happen. It's a destructor, all of these member vars are going into the ether anyway. 

        //__try { 
        //    /*
        //    static count = 0;
        //    count++;
        //    if (count == 1) {
        //        *(BYTE*)NULL = 0;
        //    }
        //    */

        //    delete[] m_damageFrames; // Replaced this with a standard allocation, instead of new in the constructor. 

        //    m_pmodeler = NULL;
        //    m_pframes = NULL;
        //    m_pgeoTrail = NULL;
        //    m_pgeoMesh = NULL;
        //    m_phullHitGeo = NULL;
        //    m_pgeoLights = NULL;
        //    m_pgeoBounds = NULL;
        //    m_psurfaceTexture = NULL;
        //    m_pvectorPositionTrail = NULL;
        //    m_pvectorRightTrail = NULL;
        //    m_pbooleanMoving = NULL;
        //    m_pgroupFlares = NULL;
        //    m_pParticleGeo = NULL;
        //    m_pbitsGeo = NULL;
        //    m_pimageGlow = NULL;
        //} __except (true) {  
        //    //
        //    // We got an exception.  Just continue.
        //    //

        ////    s_crashCount++;
        //} 
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////

    bool IsValid()
    {
        return (m_pgeoMesh != NULL);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetBoundsGeo(Geo* pgeo)
    {
        m_pgeoBounds = pgeo;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // TargetGeo
    //
    //////////////////////////////////////////////////////////////////////////////

    class TargetGeo : public Geo {
    private:
        TRef<ThingGeoImpl> m_pthing;

    public:
        TargetGeo(ThingGeoImpl* pthing) :
            m_pthing(pthing)
        {
        }

        float GetRadius(const Matrix& mat)
        {
            return m_pthing->GetRadius(mat);
        }

        void Render(Context* pcontext)
        {
            //
            // Scale the target geo so that it is radius 1
            //

            pcontext->Scale3(1.0f / m_pthing->GetRadius());
            m_pthing->RenderTargetGeo(pcontext);
        }

        void Evaluate()
        {
            m_pthing->Evaluate();
            Changed();
        }
    };

    Geo* GetTargetGeo()
    {
        return new TargetGeo(this);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // GeoWrapper
    //
    //////////////////////////////////////////////////////////////////////////////

    class ThingGeoWrapper : public Geo {
    public:
        TRef<ThingGeoImpl> m_pthing;

        ThingGeoWrapper(ThingGeoImpl* pthing) :
            m_pthing(pthing)
        {
        }

        float GetRadius(const Matrix& mat)
        {
            return m_pthing->GetRadius(mat);
        }

        void Render(Context* pcontext)
        {
            m_pthing->Render(pcontext);
        }

        void Evaluate()
        {
            m_pthing->Evaluate();
            Changed();
        }
    };

    Geo* GetGeo()
    {
        return new ThingGeoWrapper(this);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Thrust
    //
    //////////////////////////////////////////////////////////////////////////////

    float m_thrust;

    void UpdateGlow()
    {
		//mmf bounds check m_thrust as it may not have been initialized
		if (m_thrust != m_thrust) m_thrust = 0.0f; // must be NAN
		if (m_thrust < 0.0f) m_thrust = 0.0f;
		if (m_thrust > 100.0f) m_thrust = 100.0f; // it gets raised to the 20th pow below

        float value = m_thrust;

        if (m_fThrustPower != 0) {
            value = 1.0f;    
        }

        m_colorGlow =
            HSBColor(
                pow(value, 5) / 6.0f,
                1 - 0.5f * (pow(value, 20)),
                1 - pow(1 - value, 3)
            );
        m_scaleGlow = 1 - pow(1 - value, 2);
    }

    void SetAfterburnerThrust(const Vector& vecThrustDirection, float power)
    {
        m_vecThrustDirection = vecThrustDirection;
        m_fThrustPower = power;
        UpdateGlow();
    }

    void SetThrust(float value)
    {
        ZAssert(value >= 0.0f && value < 1.0001f);
        if (value > 1.0f)
          value = 1.0f;

        m_thrust = value;

        UpdateGlow();
    }

    void SetParticleGeo (ParticleGeo* pParticleGeo)
    {
        m_pParticleGeo = pParticleGeo;
    }

    void    SetAfterburnerSmokeSize (float fSize)
    {
        m_sizeSmoke = fSize;
    }

    void    SetAfterburnerFireDuration (float fDuration)
    {
        m_fAfterburnerFireDuration = fDuration;
    }

    void    SetAfterburnerSmokeDuration (float fDuration)
    {
        m_fAfterburnerSmokeDuration = fDuration;
    }

    ////////////////////////////////////////////////////////////////////////////
    void    AddFireAndSmoke (void)
    {
        if (m_bVisible && (s_iShowSmoke > 0) && (m_time != m_timeLast))
        {
            // compute the offset since last update, in vector form
            Vector  vecDeltaLastPosition = m_vecPosition - m_vecPositionLast;

            // compute the time since the last update
            float   fDeltaTime = m_time - m_timeLast;

            // compute the velocity vector
            Vector  vecFrameVelocity = vecDeltaLastPosition / fDeltaTime;

            // compute the acceleration
            Vector  vecFrameAcceleration = (vecFrameVelocity - m_vecVelocityLast) / fDeltaTime;

            // add thrust from engine frames
            AddThrustParticles (fDeltaTime, vecFrameVelocity, vecFrameAcceleration);

            if (m_bVisibleShip)
            {
                // add fire from damage frames
                AddDamageParticles (fDeltaTime, vecFrameVelocity, vecFrameAcceleration);

                // add teleport particles
                AddTeleportParticles (fDeltaTime, vecFrameVelocity, vecFrameAcceleration);
            }

            // save the rolling values for the next frame
            m_timeLast        = m_time;
            m_vecPositionLast = m_vecPosition;
            m_vecVelocityLast = vecFrameVelocity;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void    ComputeFireParticleParameters 
    (
        // parameters used for computing the theoretical start and end points
        float           fTimeSinceCreation,
        const Vector&   vecFrameOrigin, 
        const Vector&   vecFrameVelocity,
        const Vector&   vecFrameAcceleration,
        const Vector&   vecFrameThrustDirection,
        float           fEjectionVelocityMin,
        float           fEjectionVelocityMax,
        float           fEffectDuration, 

        // parameters used to compute randomized start and end points
        float           fStartPositionVariance, 
        float           fEffectSize, 

        // the values computed
        Vector&         vecStartPosition, 
        Vector&         vecStartVelocity
    )
    {
        // compute the ejection velocity that we will actually use, based on the size of the effect
        float   fEjectionVelocity = random (fEjectionVelocityMin, fEjectionVelocityMax) * fEffectSize;

        // compute the thrust vector
        Vector  vecThrust = fEjectionVelocity * vecFrameThrustDirection;

        // compute the theoretical start point for the particle accounting for thrust velocity, old frame velocity, and frame acceleration
        Vector  vecTheoreticalStartPosition = vecFrameOrigin + (fTimeSinceCreation * vecThrust);

        // compute the theoretical end point for the particle
        Vector  vecTheoreticalEndPosition = vecTheoreticalStartPosition + (fEffectDuration * vecThrust);

        // compute a randomized start location
        vecStartPosition = vecTheoreticalStartPosition + ((fStartPositionVariance * fEffectSize) * Vector::RandomDirection ());

        // compute the travel vector of the particle and divide it by the effect time to get
        // velocity relative to the ship
        Vector  vecRelativeVelocity = (vecTheoreticalEndPosition - vecStartPosition) / fEffectDuration;

        // compute the actual particle velocity by adding the frame velocity and acceleration to the relative velocity
        vecStartVelocity = vecRelativeVelocity + vecFrameVelocity - (fTimeSinceCreation * vecFrameAcceleration);
    }

    ////////////////////////////////////////////////////////////////////////////
    void    ComputeSmokeParticleParameters 
    (
        // parameters used for computing the theoretical start and end points
        float           fTimeSinceCreation,
        const Vector&   vecFrameOrigin, 
        const Vector&   vecFrameVelocity,
        const Vector&   vecFrameAcceleration,
        const Vector&   vecFrameThrustDirection,
        float           fEjectionVelocityMin,
        float           fEjectionVelocityMax,

        // parameters used to compute randomized start and end points
        float           fStartPositionVariance, 
        float           fEffectSize, 

        // the values computed
        Vector&         vecStartPosition, 
        Vector&         vecStartVelocity
    )
    {
        // compute the ejection velocity that we will actually use, based on the size of the effect
        float   fEjectionVelocity = random (fEjectionVelocityMin, fEjectionVelocityMax) * fEffectSize;

        // compute the thrust vector
        Vector  vecThrust = fEjectionVelocity * vecFrameThrustDirection;

        // compute the theoretical start point for the particle accounting for thrust velocity, old frame velocity, and frame acceleration
        Vector  vecTheoreticalStartPosition = vecFrameOrigin + (fTimeSinceCreation * vecThrust);

        // compute a randomized start location
        vecStartPosition = vecTheoreticalStartPosition + ((fStartPositionVariance * fEffectSize) * Vector::RandomDirection ());

        // compute the actual particle velocity by adding the frame velocity and acceleration to the relative velocity
        vecStartVelocity = vecThrust + vecFrameVelocity - (fTimeSinceCreation * vecFrameAcceleration);
    }

    ////////////////////////////////////////////////////////////////////////////
    void    AddTeleportParticles (float fDeltaTime, const Vector& vecFrameVelocity, const Vector& vecFrameAcceleration)
    {
        float   fTimeUntilTeleport = m_fTimeUntilRipcord;
        if ((fTimeUntilTeleport < 0.0f) || ((m_fTimeUntilAleph >= 0.0f) && (m_fTimeUntilAleph < m_fTimeUntilRipcord)))
            fTimeUntilTeleport = m_fTimeUntilAleph;
        // run the effect until it ends
        if (fTimeUntilTeleport >= 0.0f)
        {
            // the axis vectors that we'll use to spray the particles
            Vector          vecX;
            Vector          vecY;
            Vector          vecZ;

            // check to see if the thing is moving
            float           fSpeedSquared = vecFrameVelocity.LengthSquared ();
            if (fSpeedSquared > 0.0f)
            {
                // it is, so compute the axis vectors from the velocity
                vecZ = vecFrameVelocity / sqrtf (fSpeedSquared);
                vecX = vecZ.GetOrthogonalVector ();
                vecY = CrossProduct (vecX, vecZ);
            }
            else
            {
                // it's not, so compute the axis vectors from the orientation
                vecZ = m_orientation.GetForward ();
                vecX = m_orientation.GetRight ();
                vecY = CrossProduct (vecX, vecZ);
            }

            // compute how many sparks to show, based on a linear increase
            // of particles over the leadup time
            float           fTimeToShowSparks = 3.0f;
            fTimeToShowSparks = fTimeUntilTeleport / fTimeToShowSparks;
            if (fTimeToShowSparks > 1.0f)
                fTimeToShowSparks = 1.0f;
            if (fTimeToShowSparks < 0.0f)
                fTimeToShowSparks = 0.0f;
            float           fCountUpToOne = 1.0f - fTimeToShowSparks;
            float           fScaledUpCount = ((fCountUpToOne * 2.0f) + 1.0f) * m_radius;
            int             iSparkCount = int (floorf (TELEPORT_SPARK_COUNT * float (s_iShowSmoke) * fCountUpToOne) + 0.5f);

            // now loop over all the spark count, spewing them as necessary
            float           fSparkTimeDelta = fDeltaTime / float (iSparkCount);
            for (int i = 0; i < iSparkCount; i++)
            {
                // start by figuring how big of a "blast" to make, and building the initial blast vector
                float           fRadius = m_radius * random (0.75f, 1.25f);
                Vector          vecParticleVelocity = Vector::RandomDirection () * fRadius;

                // project the blast vector onto the plane in front of the ship, and compute the
                // offset from the desired vector to the projected vector
                Vector          vecParticleVelocityProjected = ((vecParticleVelocity * vecX) * vecX) + ((vecParticleVelocity * vecY) * vecY);
                Vector          vecDeltaProjection = vecParticleVelocityProjected - vecParticleVelocity;

                // the final velocity vector is an interpolation from the blast vector to the projected
                // vector, based on the duration of the effect
                vecParticleVelocity = (fTimeToShowSparks * vecDeltaProjection) + vecParticleVelocity;

                // create the particle and set its parameters
                ParticleData*   pParticleData = m_pParticleGeo->AddSpark ();
                float           fRandomStartTime = random (float (i), float (i) + 1.0f) * fSparkTimeDelta;
                pParticleData->m_vecVelocity = vecFrameVelocity + vecParticleVelocity;
                pParticleData->m_vecAcceleration = -vecFrameVelocity;
                pParticleData->m_vecPosition = 
                      m_vecPositionLast 
                    + (fTimeToShowSparks * m_radius * 1.5f * vecZ) 
                    + (vecParticleVelocity * ((1.0f - fTimeToShowSparks) + fRandomStartTime)) 
                    + (0.5f * fRandomStartTime * fRandomStartTime * pParticleData->m_vecAcceleration);
                pParticleData->m_fSize = random (fScaledUpCount * 0.15f, fScaledUpCount * 0.3f);
                pParticleData->m_fDuration = 0.5f;
            }
        }

        // decrement the time until teleport
        m_fTimeUntilRipcord -= fDeltaTime;
        m_fTimeUntilAleph -= fDeltaTime;
    }

    ////////////////////////////////////////////////////////////////////////////
    void    AddDamageParticles (float fDeltaTime, const Vector& vecFrameVelocity, const Vector& vecFrameAcceleration)
    {
        if ((m_iLastDamageIndex >= 0) && m_pParticleGeo && m_bShowDamage) 
        {
            // compute the length of the offset since last frame
            float   fDeltaLength = vecFrameVelocity.Length() * fDeltaTime;

            // Figure how many particles we will draw at each frame to fill the space,
            // and how much time to pass between each particle. We clamp the max according
            // to some user set preference for performance reasons. The clamping may lead 
            // to some clumping, but it's better than drowning the performance of the 
            // engine and blowing out the effect
			int     iParticlesPerFrame = int (fDeltaLength);
            if (iParticlesPerFrame < 1)
                iParticlesPerFrame = 1;
            if (iParticlesPerFrame > s_iShowSmoke)
                iParticlesPerFrame = s_iShowSmoke;
            float   fUniformTimeBetweenParticles = fDeltaTime / float (iParticlesPerFrame);

            // parameters that will be used repeatedly
            Vector  vecStartPosition;
            Vector  vecStartVelocity;

            // loop over the particle emission frames
            for (int i = 0; i <= m_iLastDamageIndex; i++)
            {
                // make an easy alias for the frame
                FrameData&  frame = m_damageFrames[i];

                // Compute the point where the smoke effect should appear to be originating from,
                // and the direction is should be emanating.
                // some awkwardness of the engine means that we have to use the last position
                // but current characteristics like velocity and such.
                Vector  vecFrameOrigin = m_vecPositionLast + (frame.m_vecPosition * m_orientation);
                Vector  vecFrameForward = frame.m_vecForward * m_orientation;

                // loop over the particles
                for (int iParticleCount = 0; iParticleCount < iParticlesPerFrame; iParticleCount++)
                {
                    // compute the size to be used for this particle pair
                    float   fEffectSize = random (1.5f, 2.5f);

                    // compute the origin time for this particle
                    float   fJitteredTimeSinceCreation = fUniformTimeBetweenParticles * (float (iParticleCount) + random (0.0f, 1.0f));

                    // set up a smoke particle
                    ComputeFireParticleParameters (fJitteredTimeSinceCreation, vecFrameOrigin, vecFrameVelocity, vecFrameAcceleration, vecFrameForward, 4.0f, 6.0f, 0.35f, 0.05f, fEffectSize, vecStartPosition, vecStartVelocity);
                    ParticleData*   pParticleData = m_pParticleGeo->AddSmoke ();
                    pParticleData->m_vecPosition = vecStartPosition;
                    pParticleData->m_vecVelocity = vecStartVelocity;
                    pParticleData->m_vecAcceleration = vecStartVelocity * -0.333f;
                    pParticleData->m_fSize = fEffectSize;
                    pParticleData->m_fDuration = 0.35f;
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void    AddThrustParticles (float fDeltaTime, const Vector& vecFrameVelocity, const Vector& vecFrameAcceleration)
    {
        if ((m_pframes != NULL) && m_pParticleGeo && (m_fThrustPower > 0)) 
        {
            // compute the length of the offset since last frame
            float   fDeltaLength = vecFrameVelocity.Length() * fDeltaTime;

            // Figure how many particles we will draw at each frame to fill the space,
            // and how much time to pass between each particle. We clamp the max according
            // to some user set preference for performance reasons. The clamping may lead 
            // to some clumping, but it's better than drowning the performance of the 
            // engine and blowing out the effect
			int     iParticlesPerFrame = int (2.0f * fDeltaLength / (m_sizeSmoke * m_fThrustPower));
            if (iParticlesPerFrame < 1)
                iParticlesPerFrame = 1;
            else if (iParticlesPerFrame > s_iShowSmoke)
                iParticlesPerFrame = s_iShowSmoke;
            float   fUniformTimeBetweenParticles = fDeltaTime / float (iParticlesPerFrame);

            // parameters that will be used repeatedly
            Vector  vecStartPosition;
            Vector  vecStartVelocity;

            // loop over the particle emission frames
            FrameDataListValue::FrameList::Iterator iter(m_pframes->GetList ());
            while (!iter.End ())
            {
                FrameData&  frame = iter.Value ();
                int         iFrameType = 0;
                if (frame.m_strName.Find ("rocket") != -1)
                    iFrameType = 1;
                else if (frame.m_strName.Find ("smoke") != -1)
                {
                    if (s_bUsePrivateAfterburners)
                        iFrameType = 1;
                    else
                        iFrameType = 2;
                }
                switch (iFrameType)
                {
                    case 1: //  rocket
                        {
                            // Compute the point where the smoke effect should appear to be originating from.
                            // some awkwardness of the engine means that we have to use the last position
                            // but current characteristics like velocity and such.
                            Vector  vecFrameOrigin = m_vecPositionLast + TransformOffset (frame.m_vecPosition);

                            // loop over the particles
                            for (int iParticleCount = 0; iParticleCount < iParticlesPerFrame; iParticleCount++)
                            {
                                // compute the size to be used for this particle pair based on the thrust and pre-parameterized values
                                float   fEffectSize = m_sizeSmoke * random (0.875f, 1.125f);

                                // compute the origin time for this particle
                                float   fJitteredTimeSinceCreation = fUniformTimeBetweenParticles * (float (iParticleCount) + random (0.0f, 1.0f));

                                // set up a fire particle
                                {
                                    ComputeFireParticleParameters (
                                        fJitteredTimeSinceCreation, 
                                        vecFrameOrigin, 
                                        vecFrameVelocity, 
                                        vecFrameAcceleration, 
                                        m_vecThrustDirection, 
                                        9.0f, 
                                        12.0f, 
                                        m_fAfterburnerFireDuration, 
                                        0.2f, 
                                        fEffectSize * m_fThrustPower, 
                                        vecStartPosition, 
                                        vecStartVelocity
                                    );

                                    ParticleData*   pParticleData = m_pParticleGeo->AddFire ();
                                    pParticleData->m_vecPosition = vecStartPosition;
                                    pParticleData->m_vecVelocity = vecStartVelocity;
                                    pParticleData->m_vecAcceleration = Vector::GetZero ();
                                    pParticleData->m_fSize = fEffectSize;
                                    pParticleData->m_fDuration = m_fAfterburnerFireDuration;
                                }

                                // set up a smoke particle
                                {
                                    ComputeSmokeParticleParameters (
                                        fJitteredTimeSinceCreation, 
                                        vecFrameOrigin, 
                                        vecFrameVelocity, 
                                        vecFrameAcceleration, 
                                        m_vecThrustDirection, 
                                        18.0f, 
                                        24.0f, 
                                        0.1f, 
                                        fEffectSize * m_fThrustPower, 
                                        vecStartPosition, 
                                        vecStartVelocity
                                    );

                                    ParticleData*   pParticleData = m_pParticleGeo->AddSmoke ();
                                    pParticleData->m_vecPosition = vecStartPosition;
                                    pParticleData->m_vecVelocity = vecStartVelocity;
                                    pParticleData->m_vecAcceleration = Vector::GetZero ();
                                    pParticleData->m_fSize = fEffectSize;
                                    pParticleData->m_fDuration = m_fAfterburnerSmokeDuration;
                                }
                            }
                        }
                        break;

                    case 2: //  smoke
                        {
                            // Compute the point where the smoke effect should appear to be originating from.
                            // some awkwardness of the engine means that we have to use the last position
                            // but current characteristics like velocity and such.
                            Vector  vecFrameOrigin = m_vecPositionLast + TransformOffset (frame.m_vecPosition);

                            // loop over the particles
                            for (int iParticleCount = 0; iParticleCount < iParticlesPerFrame; iParticleCount++)
                            {
                                // compute the size to be used for this particle pair based on the thrust and pre-parameterized values
                                float   fEffectSize = m_sizeSmoke * random (0.875f, 1.125f);

                                // compute a random modulation of the particle velocity
                                float   fRandomDelta = m_fThrustPower * fEffectSize;
                                Vector  vecRandomModulation (random (-fRandomDelta, fRandomDelta), random (-fRandomDelta, fRandomDelta), random (-fRandomDelta, fRandomDelta));

                                // compute the origin time for this particle
                                float   fJitteredTimeSinceCreation = fUniformTimeBetweenParticles * (float (iParticleCount) + random (0.0f, 1.0f));
                                //fJitteredTimeSinceCreation *= fJitteredTimeSinceCreation;

                                // set up a smoke particle
                                {
                                    ComputeSmokeParticleParameters (
                                        fJitteredTimeSinceCreation, 
                                        vecFrameOrigin, 
                                        vecFrameVelocity, 
                                        Vector::GetZero(), 
                                        m_vecThrustDirection, 
                                        12.0f, 
                                        15.0f, 
                                        0.1f, 
                                        fEffectSize * m_fThrustPower, 
                                        vecStartPosition, 
                                        vecStartVelocity
                                    );

                                    ParticleData*   pParticleData = m_pParticleGeo->AddAfterburner ();
                                    pParticleData->m_vecPosition = vecStartPosition;
                                    pParticleData->m_vecVelocity = vecStartVelocity + vecRandomModulation;
                                    pParticleData->m_vecAcceleration = Vector::GetZero ();
                                    pParticleData->m_fSize = fEffectSize;
                                    pParticleData->m_fDuration = m_fAfterburnerSmokeDuration;
                                }
                            }
                        }
                        break;
                }
                iter.Next ();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    void SetTrailColor(const Color& color)
    {
        //
        // Is there any frame data?
        //

        if (m_pframes)
        {
            m_vecTrailOffset = Vector(0, 0, 0);
            FrameDataListValue::FrameList::Iterator iter(m_pframes->GetList());
            while (!iter.End())
            {
                FrameData& data = iter.Value();

                if (data.m_strName == "trail") {
                    m_vecTrailOffset = data.m_vecPosition;

                    m_pgeoTrail = 
                        CreateTrailGeo(
                            m_pmodeler, 
                            Color (color.R() * 0.5f, color.G() * 0.5f, color.B() * 0.5f),
                            m_pvectorPositionTrail = new ModifiableVectorValue (Vector(0, 0, 0)),
                            m_pvectorRightTrail    = new ModifiableVectorValue (Vector(0, 0, 0)),
                            m_pbooleanMoving       = new ModifiableBoolean (false),
                            m_ptime
                        );

                    UpdateTrailPosition ();
                    break;
                }
                iter.Next();
            }
        }
    }

    void SetShadeAlways(bool bShadeAlways)
    {
        m_bShadeAlways = bShadeAlways;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Load
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<INameSpace> LoadMDL(short options, const ZString& strName, Image* pimageTexture)
    {
        TRef<INameSpace> pns = m_pmodeler->GetNameSpace(strName);

        if (pns) {
            if (ZSucceeded(LoadMDL(options, pns, pimageTexture))) {
                return pns;
            }
        }

        return NULL;
    }

    HRESULT LoadMDL(short options, INameSpace* pns, Image* pimageTexture)
    {
        //
        // Pull stuff out of the name space
        //

        TRef<Geo> pgeo = Geo::Cast(pns->FindMember("object"));

        if (pgeo) {
            //
            // Are there any lights
            //

            m_pgeoLights = Geo::Cast(pns->FindMember("lights"));

            //
            // Is there any frame data?
            //

            m_pframes = (FrameDataListValue*)pns->FindMember("frames");

            //
            // Do the rest of the Load
            //

            HRESULT hr = Load(options, pgeo, pimageTexture);

            if (ZFailed(hr)) {
                return E_FAIL;
            }

            return S_OK;
        }

        return E_FAIL;
    }

    HRESULT Load(short options, Geo* pgeo, Image* pimageTexture)
    {
        //
        // Use the passed in mesh
        //

        m_pgeoMesh = pgeo;

        //
        //  : this should be done in the preprocessor
        //

        m_bFlip = (options == 0);

        //
        // Override the texture if specified
        //

        if (pimageTexture) {
            m_psurfaceTexture = pimageTexture->GetSurface();
        } else {
            pimageTexture = m_pgeoMesh->FindTexture();
            if (pimageTexture) {
                m_psurfaceTexture = pimageTexture->GetSurface();
            } else {
                m_psurfaceTexture = NULL;
            }
        }

        //
        // Get the radius
        //

        m_pgeoMesh->Update();
        m_radius      =
        m_radiusCull  =
        m_radiusMesh  = m_pgeoMesh->GetRadius(Matrix::GetIdentity());

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Attributes
    //
    //////////////////////////////////////////////////////////////////////////////

    Vector TransformOffset(const Vector& vecArg)
    {
        Vector vec = vecArg;

        if (m_bFlip) {
            vec = Vector(-vec.X(), vec.Y(), -vec.Z());
        }

        vec  = vec * m_orientation;
        vec *= m_scale;

        return vec;
    }

    Vector TransformOffsetNoScale(const Vector& vecArg)
    {
        Vector vec = vecArg;

        if (m_bFlip) {
            vec = Vector(-vec.X(), vec.Y(), -vec.Z());
        }

        return vec * m_orientation;
    }

    bool GetChildModelOffset(const ZString& str, Vector& vec)
    {
        if (m_pframes) {
            FrameDataListValue::FrameList::Iterator iter(m_pframes->GetList());

            while (!iter.End()) {
                FrameData& data = iter.Value();

                if (data.m_strName == str) {
                    vec = data.m_vecPosition;

                    //
                    // transform the offset
                    //

                    vec *= m_scale;

                    return true;
                }

                iter.Next();
            }
        }

        return false;
    }

    bool GetChildOffset(const ZString& str, Vector& vec)
    {
        if (GetChildModelOffset(str, vec))
        {
            if (m_bFlip) {
                vec = Vector(-vec.X(), vec.Y(), -vec.Z());
            }

            vec  = vec * m_orientation;
            return true;
        }

        return false;
    }

    float GetRadius()
    {
        return m_radius;
    }

    float GetRadius(const Matrix& mat)
    {
        return m_radius * mat.GetScale();
    }

    void SetVisible(bool bVisible)
    {
        m_bVisible = bVisible;
    }

    void SetVisibleShip(bool bVisible)
    {
        m_bVisibleShip = bVisible;
    }

    void UpdateTrailPosition()
    {
        if (m_pvectorPositionTrail) {
            m_pvectorPositionTrail->SetValue(
                  m_vecPosition
                + TransformOffset(m_vecTrailOffset)
            );
        }
    }

    void SetPosition(const Vector& vec)
    {
        if (m_pbooleanMoving != NULL) {
            m_pbooleanMoving->SetValue(m_vecPosition != vec);
        }

        if (m_vecPosition != vec) {
            m_vecPosition = vec;

            UpdateTrailPosition();
        }
    }

    void SetOrientation(const Orientation& orient)
    {
        m_orientation = orient;

        if (m_pvectorRightTrail) {
            m_pvectorRightTrail->SetValue(orient.GetRight() * m_radius * 0.25);
        }

        UpdateTrailPosition();
    }

    float SetRadius(float radius)
    {
        m_scale = radius / m_radiusMesh;

        if (m_radiusCull == m_radius) {
            m_radiusCull = radius;
        }

        m_radius = radius;

        UpdateTrailPosition();

        return m_radius;
    }

    void SetTexture(Image* pimageTexture)
    {
        m_psurfaceTexture = pimageTexture->GetSurface();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Shield flares
    //
    //////////////////////////////////////////////////////////////////////////////

    float GetFlareCount(void) const
    {
        return m_countFlares;
    }

    class EventSink : public IEventSink {
    public:
        TRef<ThingGeoImpl> m_pthing;
        
        EventSink(ThingGeoImpl* pthing) :
            m_pthing(pthing)
        {
        }

        bool OnEvent(IEventSource* pevent)
        {
            return m_pthing->OnEvent(pevent);
        }
    };

    bool OnEvent(IEventSource* pevent)
    {
        ZAssert(m_countFlares > 0);

        m_countFlares--;
        if (m_countFlares == 0) {
            m_radiusCull = m_radius;
        }

        return false;
    }

    void AddFlare(Geo* pgeo, Number* ptime, const Vector& vecDirection, const Vector* pvectorEllipse)
    {
        // !!! how about only one flare per thing
        /* !!! why is this commented out?
        if (m_countFlares >= 4) {
            return;
        }
        */

        //
        // Transform the direction into local space
        //

        Vector vecLocal = m_orientation.TimesInverse(-vecDirection);

        //
        // Orient the geo to point along vecLocal
        //

        Orientation orient;

        if (pvectorEllipse) {
            Vector vector = vecLocal;

            vector.x /= pvectorEllipse->x;
            vector.y /= pvectorEllipse->y;
            vector.z /= pvectorEllipse->z;

            orient.Set(vector);
            orient.Scale((*pvectorEllipse) / m_radius);

            //
            // The culling radius is equal to the largest extent of the ellipse
            //

            m_radiusCull = max(m_radiusCull, pvectorEllipse->X());
            m_radiusCull = max(m_radiusCull, pvectorEllipse->Y());
            m_radiusCull = max(m_radiusCull, pvectorEllipse->Z());
        } else {
            orient.Set(vecLocal);
        }

        //
        // Create the local geo
        //

        Matrix mat(orient, Vector::GetZero(), m_radiusMesh);


        TRef<TimedGeo> m_ptimedGeo =
            new TimedGeo(
                new TransformGeo(
                    CreateCopyModeGeo(
                        CreateBlendGeo(
                            pgeo,
                            BlendModeAdd
                        )
                    ),
                    new MatrixTransform(mat)
                ),
                ptime,
                0.75f
            );

        m_pgroupFlares->AddGeo(m_ptimedGeo);

        m_ptimedGeo->AddSink(new EventSink(this));
        m_countFlares++;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Hull Hits
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetBitsGeo(BitsGeo* pbitsGeo)
    {
        m_pbitsGeo = pbitsGeo;
    }

    void AddHullHit(const Vector& vecPosition, const Vector& vecNormal)
    {
        m_phullHitGeo->AddHullHit(vecPosition, vecNormal);

        float size   = m_radius / 2;
        float speed  = 0.4f * size;
        float spread = 0.25;

        Vector vecWorldPosition = m_vecPosition + vecPosition;

        for (int index = 0; index < 2; index++) {
            m_pbitsGeo->AddBit(
                vecWorldPosition,
                speed * Vector(
                    vecNormal.X() + random(-spread, spread),
                    vecNormal.Y() + random(-spread, spread),
                    vecNormal.Z() + random(-spread, spread)
                ),
                size
            );
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Damage Frames
    //
    //////////////////////////////////////////////////////////////////////////////

    void    SetShowDamage (bool bShowDamage)
    {
        m_bShowDamage = bShowDamage;
    }

    void    AddDamage (const Vector& vecDamagePosition, float fDamageFraction)
    {
        int iDamageIndexOpen = int((1.0f - fDamageFraction) * MAX_DAMAGE_FRAMES) - 1;
        int iDamageIndex = bound(iDamageIndexOpen, -1, MAX_DAMAGE_FRAMES - 1);

        if (iDamageIndex != iDamageIndexOpen) {
            s_trashCount++;
        }

        if (iDamageIndex > m_iLastDamageIndex)
        {
            Vector  vecPosition = vecDamagePosition * 0.33f;
            Vector  vecFromDirection = vecPosition.Normalize ();
            Vector  vecForward = m_orientation.TimesInverse (vecFromDirection);
            vecPosition = m_orientation.TimesInverse (vecPosition);
            while (m_iLastDamageIndex <= iDamageIndex)
            {
                m_iLastDamageIndex++;
                m_damageFrames[m_iLastDamageIndex].m_vecForward = vecForward;
                m_damageFrames[m_iLastDamageIndex].m_vecPosition = vecPosition;
            }
        }
        else
            m_iLastDamageIndex = iDamageIndex;
    }

    void    RemoveDamage (float fDamageFraction)
    {
        int iDamageIndexOpen = int((1.0f - fDamageFraction) * MAX_DAMAGE_FRAMES) - 1;
        int iDamageIndex = bound(iDamageIndexOpen, -1, MAX_DAMAGE_FRAMES - 1);

        if (iDamageIndex != iDamageIndexOpen) {
            s_trashCount++;
        }

        if (iDamageIndex < m_iLastDamageIndex)
            m_iLastDamageIndex = iDamageIndex;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Teleport effects
    //
    //////////////////////////////////////////////////////////////////////////////

    void    SetTimeUntilRipcord (float fTimeUntilTeleport)
    {
        m_fTimeUntilRipcord = fTimeUntilTeleport;
    }

    void    SetTimeUntilAleph (float fTimeUntilTeleport)
    {
        m_fTimeUntilAleph = fTimeUntilTeleport;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Rendering
    //
    //////////////////////////////////////////////////////////////////////////////

    void RenderGlow(Context* pcontext)
    {
        if ((m_scaleGlow > 0) && m_pframes)
        {
            FrameDataListValue::FrameList::Iterator iter(m_pframes->GetList());

            while (!iter.End()) {
                FrameData& data = iter.Value();

                if (data.m_strName.Find("thrust") != -1) {
                    pcontext->DrawDecal(
                        m_pimageGlow->GetSurface(),
                        m_colorGlow,
                        m_vecPosition + TransformOffset(data.m_vecPosition),
                        Vector::GetZero(),
                        Vector::GetZero(),
                        m_scaleGlow * m_scale,
                        0
	                );
                }

                iter.Next();
            }
        }
    }

    class Callback : public IGeoCallback {
    public:
        TRef<ThingGeoImpl> m_pthing;

        Callback(ThingGeoImpl* pthing) :
            m_pthing(pthing)
        {
        }

        void RenderCallback(Context* pcontext) {
            m_pthing->RenderCallback(pcontext);
        }
    };

    void RenderCallback(Context* pcontext)
    {
        #ifdef _DEBUG
        pcontext->SetBlendMode(BlendModeAdd);
        pcontext->SetShadeMode(ShadeModeGlobalColor);

        if (s_bTransparentObjects)
        {
            pcontext->SetGlobalColor(Color(0.25f, 0.5f, 0.25f));
            m_pgeoMesh->Render(pcontext);
        }
        else 
        {
            if (m_pgeoBounds != NULL) 
            {
                pcontext->SetGlobalColor(Color(0.5f, 0.25f, 0.25f));
                pcontext->Rotate(Vector(0, 0, 1), pi);
                m_pgeoBounds->Render(pcontext);
            }
        }
        #endif
    }

    void RenderTargetGeo(Context* pcontext)
    {
        pcontext->Multiply(Matrix(m_orientation, Vector(0, 0, 0), 1));
        pcontext->Scale3(m_scale);

        //
        // Draw flares
        //

        if (m_pgroupFlares->GetList()->GetCount() != 0) {
            pcontext->PushState();
            m_pgroupFlares->Render(pcontext);
            pcontext->PopState();
        }

        //
        // Flip the model if needed
        //

        if (m_bFlip) {
            pcontext->Rotate(Vector(0, 1, 0), pi);
        }

        //
        // Draw the lights
        //

        if (m_bDrawLights && m_pgeoLights != NULL) {
            m_pgeoLights->Render(pcontext);
        }

        //
        // Clip if needed
        //

        bool bClipSave = pcontext->GetClipping();
        pcontext->SetClipping(m_bClip);

        //
        // Draw the object
        //

		// Statement is never true - always 3d acceleration.
//        if ((!pcontext->Has3DAcceleration()) && (!m_bShadeAlways)) {
//            pcontext->SetShadeMode(ShadeModeCopy);
//        }
		// KGJV- fix transparent models
		bool bColorKey = pcontext->GetColorKey();

        if (m_psurfaceTexture) {
            pcontext->SetTexture(m_psurfaceTexture);
        }
		else
		{
			//KGJV- fix transparent models - blendmodeadd and turn off colorkey
			pcontext->SetBlendMode(BlendModeAdd);
			pcontext->SetColorKey(false,true);
		}

        // draw transparent stuff
        pcontext->DrawCallbackGeo(new Callback(this), false);

        #ifdef _DEBUG
            if (s_bShowBounds) {
                pcontext->DrawCallbackGeo(new Callback(this), false);

                if (s_bTransparentObjects) {
                    if (m_pgeoBounds) {
                        pcontext->Rotate(Vector(0, 0, 1), pi);
                        m_pgeoBounds->Render(pcontext);
                    }
                } else {
                    m_pgeoMesh->Render(pcontext);
                }
            } else {
                m_pgeoMesh->Render(pcontext);
            }
        #else
            m_pgeoMesh->Render(pcontext);
        #endif

        //
        // Restore clipping state
        //

        pcontext->SetClipping(bClipSave);

		// KGJV- fix transparent models - restore colorkey
		pcontext->SetColorKey(bColorKey,true); 
    }

    void RenderGeo(Context* pcontext)
    {
        pcontext->Translate(m_vecPosition);

        if (s_bShowHullHits) {
            m_phullHitGeo->Render(pcontext);
        }

        RenderTargetGeo(pcontext);
    }

    void Render(Context* pcontext)
    {
        if (m_pgeoTrail) {
            m_pgeoTrail->Update();
        }
        if (m_pgeoLights) {
            m_pgeoLights->Update();
        }
        m_phullHitGeo->Update();
        m_pgroupFlares->Update();
        m_pgeoMesh->Update();

        if (m_bVisible) {
            //
            // Draw the trail
            //

            if (s_bShowTrails && m_pgeoTrail != NULL) {
                m_pgeoTrail->Render(pcontext);
            }

            //
            // If visible draw the ship
            //

            if (m_bVisibleShip) {
                bool bNoClipping;
                if (!pcontext->IsCulled(m_vecPosition, m_radiusCull, bNoClipping)) {
                    m_bClip = !bNoClipping;

                    //
                    // Figure out the screen radius
                    //

                    float screenRadius = pcontext->GetImageRadius(m_vecPosition, m_radius);

                    //
                    // Set the LOD
                    //

                    pcontext->SetLOD(screenRadius * s_lodBias);

                    //
                    // Only draw the lights if the object is above a certain size
                    //

                    m_bDrawLights = screenRadius > 32 && s_bShowLights;

                    //
                    // Draw the engine glow
                    //

                    RenderGlow(pcontext);

                    //
                    // Draw the object
                    //

					BlendMode oldMode = pcontext->GetBlendMode();
					pcontext->SetBlendMode( BlendModeSourceAlpha );
                    RenderGeo(pcontext);
					pcontext->SetBlendMode( oldMode );
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Value
    //
    //////////////////////////////////////////////////////////////////////////////

    void Evaluate()
    {
        m_ptime->Update();
        m_time = m_ptime->GetValue();

        AddFireAndSmoke ();
    }

    ZString GetFunctionName() { return "ThingGeo"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// ThingGeo Statics
//
//////////////////////////////////////////////////////////////////////////////

float ThingGeo::s_lodBias                   = 1;
bool  ThingGeo::s_bShowLights               = true;
bool  ThingGeo::s_bShowHullHits             = true;
bool  ThingGeo::s_bShowTrails               = true;
bool  ThingGeo::s_bShowBounds               = false;
int   ThingGeo::s_iShowSmoke                = 1;
bool  ThingGeo::s_bTransparentObjects       = false;
int   ThingGeo::s_crashCount                = 0;
int   ThingGeo::s_trashCount                = 0;
bool  ThingGeo::s_bUsePrivateAfterburners   = false; //Imago 8/16/09

void ThingGeo::SetTransparentObjects(bool bTransparentObjects)
{
    s_bTransparentObjects = bTransparentObjects;
}

bool ThingGeo::GetTransparentObjects()
{
    return s_bTransparentObjects;
}

void ThingGeo::SetShowTrails(bool bShowTrails)
{
    s_bShowTrails = bShowTrails;
}

void ThingGeo::SetShowBounds(bool bShowBounds)
{
    s_bShowBounds = bShowBounds;
}

int  ThingGeo::GetShowSmoke()
{
    return (s_bUsePrivateAfterburners) ? 2 : s_iShowSmoke;
}

//Imago 8/16/09
void ThingGeo::SetPerformance(bool bUse) {
    s_bUsePrivateAfterburners = bUse;
}

void ThingGeo::SetShowSmoke(int iShowSmoke)
{
	 s_iShowSmoke = iShowSmoke;
}

bool ThingGeo::GetShowTrails()
{
    return s_bShowTrails;
}

bool ThingGeo::GetShowBounds()
{
    return s_bShowBounds;
}

void ThingGeo::SetShowLights(bool bShowLights)
{
    s_bShowLights = bShowLights;
}

bool ThingGeo::GetShowLights()
{
    return s_bShowLights;
}

void ThingGeo::SetShowHullHits(bool bShowHullHits)
{
    s_bShowHullHits = bShowHullHits;
}

bool ThingGeo::GetShowHullHits()
{
    return s_bShowHullHits;
}

void ThingGeo::SetLODBias(float lodBias)
{
    ZAssert(lodBias >=0 && lodBias <=1);
    ThingGeo::s_lodBias = lodBias;
}

int ThingGeo::GetCrashCount()
{
    return ThingGeo::s_crashCount;
}

int ThingGeo::GetTrashCount()
{
    return ThingGeo::s_trashCount;
}

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<ThingGeo> ThingGeo::Create(Modeler* pmodeler, Number* ptime)
{
    return new ThingGeoImpl(pmodeler, ptime);
}
