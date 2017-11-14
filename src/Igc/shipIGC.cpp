/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:    shipIGC.cpp
**
**  Author:
**
**  Description:
**      Implementation of the CshipIGC class. This file was initially created by
**  the ATL wizard for the core object.
**
**  History:
*/
// shipIGC.cpp : Implementation of CshipIGC
#include "shipIGC.h"
#include <math.h>
#include <limits.h>

#include "StackTracer.h" // BT - 9/17 - Trying to hunt down the mystery damage ship event crash. 

float   c_fRunAway = 0.75f;

/////////////////////////////////////////////////////////////////////////////
// CshipIGC
const float   c_dtTimeBetweenComplaints = 30.0f;

CshipIGC::CshipIGC(void)
:
    m_fuel(0.0f),
    m_stateM(0),
    m_station(NULL),
    m_pmissileLast(NULL),
    m_damageTrack(NULL),
    m_dwPrivate(0),
    m_myHullType(this),
    m_cloaking(1.0f),
    m_pshipParent(NULL),
    m_turretID(NA),
    m_bAutopilot(false),
    m_gotoplan(this),
    m_dtTimeBetweenComplaints(c_dtTimeBetweenComplaints),
    m_nKills(0),
    m_nDeaths(0),
    m_nEjections(0),
    m_sidFlag(NA),
    m_ripcordDebt(0.0f),
	m_wingID(NA), //Imago 6/10 #91
	m_lastLaunch(Time::Now()), //Imago 7/10 #7
	m_lastDock(Time::Now())
{
    //Start with a single kill's worth of exp
    SetExperience(1.0f);

    //Prevent myHullType from ever being deleted via Release() (since we will never
    //call Release() on its "pointer").
    m_myHullType.AddRef();

    {
        for (Command i = 0; (i < c_cmdMax); i++)
        {
            m_commandTargets[i] = NULL;
            m_commandIDs[i] = c_cidNone;
        }
    }
    {
        for (Mount i = 0; (i < c_maxMountedWeapons); i++)
            m_mountedWeapons[i] = NULL;
    }
    {
        for (Mount i = 0; (i < ET_MAX); i++)
            m_mountedOthers[i] = NULL;
    }
    {
        for (Mount i = 0; (i < c_maxCargo); i++)
            m_mountedCargos[i] = NULL;
    }
}

void    CshipIGC::ReInitialize(DataShipIGC * dataShip, Time now)
{
    ImissionIGC * pmission = GetMyMission();
    assert (IMPLIES(!pmission, dataShip->sideID == NA));

    //Set texture mask & decals (before setting the hull type since that
    //actually applies the texture)

    // Clean out old fuel, ammo, parts, mass
    m_fuel = 0.0f;
    m_ammo = 0;
    m_energy = 0.0f;
    {
        PartLinkIGC* ppartlink;
        while (ppartlink = GetParts()->first()) //Not ==
            ppartlink->data()->Terminate();
    }
    m_myHullType.SetHullType(NULL);
    SetMass(0.0f);

    SetLastUpdate(now);

    SetName(dataShip->name);

    m_pilotType = dataShip->pilotType;
    m_abmOrders = dataShip->abmOrders;
    if (dataShip->baseObjectID == NA)
        m_pbaseData = NULL;
    else if (m_pilotType == c_ptBuilder)
    {
        m_pbaseData = pmission->GetStationType(dataShip->baseObjectID);
        assert (m_pbaseData);
    }
    else if (m_pilotType == c_ptLayer)
    {
        m_pbaseData = pmission->GetExpendableType(dataShip->baseObjectID);
        assert (m_pbaseData);
    }
    else
        assert (false);

    m_turnRates[c_axisYaw] = m_turnRates[c_axisPitch] = m_turnRates[c_axisRoll] =
                             m_controls.jsValues[c_axisYaw] = m_controls.jsValues[c_axisPitch] = m_controls.jsValues[c_axisRoll] = 0.0f;
    m_controls.jsValues[c_axisThrottle] = -1.0f;

    IsideIGC * pside = pmission ? pmission->GetSide(dataShip->sideID) : NULL;
    SetSide(pside);

    m_shipID = dataShip->shipID;
    m_wingID = NA; //dataShip->wingID;

    m_nDeaths = dataShip->nDeaths;
    m_nEjections = dataShip->nEjections;
    m_nKills = dataShip->nKills;

    //Get the ship's hull type
    if (dataShip->hullID == NA)
    {
        FreeThingSite();
    }
    else
    {
        assert (pside);

        SetBaseHullType(GetMyMission()->GetHullType(dataShip->hullID));
        assert (m_myHullType.GetData());

        {
            ThingSite* pThingSite = GetThingSite();
            assert (pThingSite);
            pThingSite->SetTrailColor(pside->GetColor());
        }

        //Add the various parts
        {
            const PartData*   pd = (PartData*)(((char*)dataShip) + dataShip->partsOffset);
            for (Mount i = 0; (i < dataShip->nParts); i++)
            {
                CreateAndAddPart(&(pd[i]));
            }
        }

        assert (GetMass() > 0.0f);  //No massless ships
    }
}

HRESULT     CshipIGC::Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize)
{
    ZRetailAssert (data && (dataSize >= sizeof(DataShipIGC)));

    DataShipIGC* dataShip = (DataShipIGC*) data;

    //If the ship already exists ...
    IshipIGC* pshipReplace = pMission->GetShip(dataShip->shipID);
    if (pshipReplace)
    {
        //Update the pre-existing ship
        pshipReplace->ReInitialize(dataShip, now);

        //and abort.
        return E_ABORT;
    }

    TmodelIGC<IshipIGC>::Initialize(pMission, now, data, dataSize);
    ReInitialize(dataShip, now);
    pMission->AddShip(this);

    m_timeLastComplaint = now;
    m_timeLastMineExplosion = now;
    m_timeRanAway = now;
    m_timeReloadAmmo = now;
    m_timeReloadFuel = now;

    m_bRunningAway = false;
	m_stayDocked = false;  //Xynth #48 8/2010
	m_repair = 0; //Xynth amount of nanning performed by ship
	m_achievementMask = 0;
    
	return S_OK;
}

void        CshipIGC::Terminate(void)
{
    AddRef();

    if ((m_pilotType == c_ptBuilder) && ((m_stateM & buildingMaskIGC) != 0))
    {
        assert (m_commandIDs[c_cmdPlan] != NULL);
        assert (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_asteroid);
        IbuildingEffectIGC* pbe = ((IasteroidIGC*)(ImodelIGC*)m_commandTargets[c_cmdPlan])->GetBuildingEffect();

        if (pbe)
        {
            pbe->BuilderTerminated();
            GetMyMission()->GetIgcSite()->KillBuildingEffectEvent(pbe);
        }
    }

    if (m_damageTrack)
    {
        delete m_damageTrack;
        m_damageTrack = NULL;
    }

    //Make the ship look very dead
    m_fraction = 0.0f;

    m_pbaseData = NULL;

    if (m_pmissileLast)
    {
        m_pmissileLast->Release();
        m_pmissileLast = NULL;
    }

    SetRipcordModel(NULL);

    SetParentShip(NULL);

    {
        //Blow away the children
        ShipLinkIGC*    psl;
        while (psl = m_shipsChildren.first())   //intentional assignment
        {
            psl->data()->SetParentShip(NULL);
        }
    }

    //Blow away all of the ships parts
    {
        PartLinkIGC* l;
        while (l = m_parts.first()) //not ==
            l->data()->Terminate();
    }

    TmodelIGC<IshipIGC>::Terminate();

    Release();
}

void    CshipIGC::Update(Time now)
{
    assert (GetCluster() != NULL);

    //Don't bother doing updates for times before the ship is created.
    Time    lastUpdate = GetMyLastUpdate();

    if ((now > lastUpdate) && (m_pshipParent == NULL))
    {
        m_warningMask = 0;

        float   dt = (now - lastUpdate);

        if (m_pmodelRipcord)
        {
            IIgcSite*   pigc = GetMyMission()->GetIgcSite();
            if (pigc->ContinueRipcord(this, m_pmodelRipcord))
            {
                //The countdown continues ...
                m_dtRipcordCountdown -= dt;
                if (m_dtRipcordCountdown <= 0.0f)
                {
                    if (pigc->UseRipcord(this, m_pmodelRipcord))
                        SetRipcordModel(NULL);
                    else
                        m_dtRipcordCountdown = 0.0f;
                }
            }
            else
                SetRipcordModel(NULL);
        }
        // mmf 03/22/07 fix don't let energy go negative
        if((m_myHullType.GetRechargeRate() > 0.0f)||(m_energy != 0.0f))
		{
			m_energy += m_myHullType.GetRechargeRate() * dt;
			float   eMax = m_myHullType.GetMaxEnergy();
			if (m_energy > eMax)
				m_energy = eMax;
			else if (m_energy < 0.0f)
				m_energy = 0.0f;
		}

        {
            //Give the cloak the first dibs on energy
            if (m_mountedOthers[ET_Cloak])
                m_mountedOthers[ET_Cloak]->Update(now);
        }

        {
            Mount maxWeapons = m_myHullType.GetMaxWeapons();
            Mount maxFixedWeapons = m_myHullType.GetMaxFixedWeapons();
            Mount selectedWeapon = (m_stateM & selectedWeaponMaskIGC) >> selectedWeaponShiftIGC;

            for (Mount i = maxFixedWeapons; (i > 0); i--)
            {
                IweaponIGC*   w = m_mountedWeapons[(i + selectedWeapon) % maxFixedWeapons];
                if (w)
                {
                    assert (!w->GetGunner());
                    w->Update(now);
                }
            }

            for (Mount j = maxFixedWeapons; (j < maxWeapons); j++)
            {
                IweaponIGC* w = m_mountedWeapons[j];

                if (w)
                {
                    IshipIGC*   pshipGunner = w->GetGunner();
                    if (pshipGunner)
                    {
                        Orientation o = pshipGunner->GetOrientation();
                        pshipGunner->ExecuteTurretMove(lastUpdate, now, &o);
                        pshipGunner->SetOrientation(o);
                    }

                    w->Update(now);
                }
            }
        }

        if (m_mountedOthers[ET_Shield])
            m_mountedOthers[ET_Shield]->Update(now);

        if (m_mountedOthers[ET_Magazine])
            m_mountedOthers[ET_Magazine]->Update(now);

        if (m_mountedOthers[ET_Dispenser])
            m_mountedOthers[ET_Dispenser]->Update(now);

        if (m_mountedOthers[ET_ChaffLauncher])
            m_mountedOthers[ET_ChaffLauncher]->Update(now);

        if (m_mountedOthers[ET_Afterburner])
            m_mountedOthers[ET_Afterburner]->Update(now);

        if (m_myHullType.GetHullType()->HasCapability(c_habmLifepod) &&
            (now > m_timeToDie))
        {
            GetMyMission()->GetIgcSite()->KillShipEvent(now, this, NULL, 1.0f, GetPosition(), Vector::GetZero());
        }
        else
        {
            //Damage the ship if it is too far out
            const Vector&   position = GetPosition();

            //Find the "effective" radius factoring in the lens factor of the universe
            float   lm = GetMyMission()->GetFloatConstant(c_fcidLensMultiplier);
            float   r2 = position.x * position.x + position.y * position.y +
                         (position.z * position.z / (lm * lm));

            float ru = GetMyMission()->GetFloatConstant(c_fcidRadiusUniverse);
            float oob = GetMyMission()->GetFloatConstant(c_fcidOutOfBounds) * ru * ru;

            if (r2 > oob)
            {
				// mmf added debugf for non players
				if (m_pilotType < c_ptPlayer) {
				    debugf("mmf ship out of bounds %s %f %f %f\n",GetName(),position.x, position.y, position.z);
				}

                m_warningMask |= c_wmOutOfBounds;

                float   d = dt *
                            (r2 - oob) /
                            (50.0f * oob);      //50 == arbitrary constant to adjust damage received
                assert (d >= 0.0f);

                float   oldFraction = m_fraction;
                m_fraction -= d;
                assert (m_fraction <= 1.0f);

                if (m_fraction > 0.0f)
                    GetMyMission()->GetIgcSite()->DamageShipEvent(now, this, NULL, c_dmgidCollision, d, d, position, position * 2.0f);
                else
                {
                    m_fraction = 0.0f;
                    if (oldFraction > 0.0f)  //Only send the death message once.
                        GetMyMission()->GetIgcSite()->KillShipEvent(now, this, NULL, d, position, position * 2.0f);
                }
                assert (m_fraction >= 0.0f);
            }

            if (m_pilotType >= c_ptPlayer)
            {
                IclusterIGC*    pcluster = GetCluster();
                if (pcluster != NULL)   //Might have died
                {
                    float   cost = pcluster->GetCost();
                    if (cost > 0.0f)
                    {
                        m_warningMask |= c_wmCrowdedSector;

                        float   maxDamage = m_myHullType.GetHitPoints();
                        if (m_mountedOthers[ET_Shield])
                            maxDamage += ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetMaxStrength();

                        float   d = cost * maxDamage;
                        ReceiveDamage(c_dmgidCollision, d, now, position, Vector::GetZero(), NULL);
                    }
                }
            }
        }
    }

    TmodelIGC<IshipIGC>::Update(now);
}

int         CshipIGC::Export(void* data) const
{
    if (data)
    {
        DataShipIGC*  dataShip = (DataShipIGC*)data;

        dataShip->hullID = m_myHullType.GetObjectID();
        dataShip->shipID = m_shipID;
        dataShip->sideID = GetSide()->GetObjectID();
        //dataShip->wingID = m_wingID;
        dataShip->nKills = m_nKills;
        dataShip->nDeaths = m_nDeaths;
        dataShip->nEjections = m_nEjections;
        UTL::putName(dataShip->name, GetName());

        dataShip->pilotType = m_pilotType;
        dataShip->abmOrders = m_abmOrders;
        dataShip->baseObjectID = m_pbaseData
                                 ? m_pbaseData->GetObjectID() : NA;

        dataShip->nParts = m_parts.n();
        dataShip->partsOffset = sizeof(DataShipIGC);

        {
            PartData*   pd = (PartData*)(((char*)dataShip) + dataShip->partsOffset);
            for (PartLinkIGC*   l = m_parts.first();
                 (l != NULL);
                 l = l->next())
            {
                IpartIGC*   part = l->data();

                assert (part->GetPartType());
                pd->partID = part->GetPartType()->GetObjectID();
                pd->mountID = part->GetMountID();

                (pd++)->amount = part->GetAmount();
            }
        }
    }

    return sizeof(DataShipIGC) + sizeof(PartData) * m_parts.n();
}

static const float  sqrt1o2 = 0.70710678118654752440084436210485f;
static const float  sqrt1o3 = 0.57735026918962576450914878050196f;

static const float  directions[][3] = {
                                        { 1.0f,  0.0f,  0.0f},
                                        {-1.0f,  0.0f,  0.0f},
                                        { 0.0f,  1.0f,  0.0f},
                                        { 0.0f, -1.0f,  0.0f},
                                        { 0.0f,  0.0f,  1.0f},
                                        { 0.0f,  0.0f, -1.0f},

                                        { sqrt1o2,  sqrt1o2, 0.0f },
                                        {-sqrt1o2,  sqrt1o2, 0.0f },
                                        { sqrt1o2, -sqrt1o2, 0.0f },
                                        {-sqrt1o2, -sqrt1o2, 0.0f },

                                        { sqrt1o2, 0.0f,  sqrt1o2 },
                                        {-sqrt1o2, 0.0f,  sqrt1o2 },
                                        { sqrt1o2, 0.0f, -sqrt1o2 },
                                        {-sqrt1o2, 0.0f, -sqrt1o2 },

                                        { 0.0f,  sqrt1o2,  sqrt1o2 },
                                        { 0.0f, -sqrt1o2,  sqrt1o2 },
                                        { 0.0f,  sqrt1o2, -sqrt1o2 },
                                        { 0.0f, -sqrt1o2, -sqrt1o2 },

                                        { sqrt1o3,  sqrt1o3,  sqrt1o3 },
                                        {-sqrt1o3,  sqrt1o3,  sqrt1o3 },
                                        { sqrt1o3, -sqrt1o3,  sqrt1o3 },
                                        {-sqrt1o3, -sqrt1o3,  sqrt1o3 },
                                        { sqrt1o3,  sqrt1o3, -sqrt1o3 },
                                        {-sqrt1o3,  sqrt1o3, -sqrt1o3 },
                                        { sqrt1o3, -sqrt1o3, -sqrt1o3 },
                                        {-sqrt1o3, -sqrt1o3, -sqrt1o3 }
                                     };

static const int       c_maxAttempts = sizeof(directions) / sizeof(directions[0]);

static void Separate(const CollisionEntry&  entry,
                     bool                   fEqualBounce,
                     Vector*                pPosition1,
                     Vector*                pPosition2,
                     Vector*                pNormal)
{
    HitTest*   pHitTest1 = entry.m_pHitTest1;
    HitTest*   pHitTest2 = entry.m_pHitTest2;

    *pPosition1 = pHitTest1->GetPosition() + entry.m_tCollision * pHitTest1->GetVelocity();
    *pPosition2 = fEqualBounce
                  ? (pHitTest2->GetPosition() + entry.m_tCollision * pHitTest2->GetVelocity())
                  : pHitTest2->GetPosition();       //don't bother if object 2 is static

    Vector  dP = *pPosition1 - *pPosition2;
    if ((dP.x == 0.0f) && (dP.y == 0.0f) && (dP.z == 0.0f))
    {
        dP = Vector::RandomDirection();
        *pPosition1 += dP;
    }

    const Orientation&  o1 = pHitTest1->GetOrientation();
    const Orientation&  o2 = pHitTest2->GetOrientation();

    Orientation o = o1.TimesInverse(o2);
    Vector  dpLocal = o2.TimesInverse(dP);

    //Do the two convex hulls intersect anywhere along the interval between *tCollision and tMax?
    Vector  directionBest;
    double  distanceBest;

    {
        directionBest = dpLocal;
        if (entry.m_hts1 >= c_htsConvexHullMin)
            directionBest += pHitTest1->GetCenter(entry.m_hts1) * o;
        if (entry.m_hts2 >= c_htsConvexHullMin)
            directionBest -= pHitTest2->GetCenter(entry.m_hts2);
        directionBest.SetNormalize();

        Vector  eA = pHitTest1->GetMinExtreme(entry.m_hts1, dpLocal,
                                              o, directionBest);
        Vector  eB = pHitTest2->GetMaxExtreme(entry.m_hts2, directionBest);

        Vector  delta = eA - eB;
        distanceBest = (delta * directionBest);
    }

    int attempt = 0;
    do
    {
        const   Vector& direction = *((Vector*)(directions[attempt]));

        //For hull A, the extreme is found by takeing the extreme in the given direction and then
        //offsetting it by the position at start or the end of the interval (which ever is appropriate:
        //there are two sets of negation cancelling out here).
        Vector  eA = pHitTest1->GetMinExtreme(entry.m_hts1, dpLocal,
                                              o, direction);
        Vector  eB = pHitTest2->GetMaxExtreme(entry.m_hts2, direction);

        Vector  delta = eA - eB;
        double  distance = (delta * direction);

        if (distance >= 0.0)
        {
            //False collision ... oh well. Use the direction as the separating
            //direction and minimize the displacement.
            directionBest = direction;
            distanceBest = 0.0;
            break;
        }

        if (distance > distanceBest)
        {
            directionBest = direction;
            distanceBest = distance;
        }

        /*
        //Didn't find a good separating plane ... munge direction in the hopes of finding a better one
        //reflect direction around the vector from the extreme to the sphere
        //Get the vector from direction to its projection onto -delta
        direction -= delta * (2.0f * (direction * delta) / delta.LengthSquared());

        assert (direction.Length() >= 0.98f);
        assert (direction.Length() <= 1.02f);
        */
    }
    while (++attempt < c_maxAttempts);

    *pNormal = directionBest * pHitTest2->GetOrientation();

    //How far do we need to displace the objects so their extreme points
    //along the separation vector do not intersect? (plus a fudge factor --
    //s will be negative)
    float   s = float(distanceBest - 0.5);

    *pPosition1 -= (s * (fEqualBounce ? 0.5f : 1.0f)) * *pNormal;

    if (fEqualBounce)
    {
        *pPosition2 += (s * 0.5f) * *pNormal;
    }
}

void    CshipIGC::HandleCollision(Time                   timeCollision,
                                  float                  tCollision,
                                  const CollisionEntry&  entry,
                                  ImodelIGC*             pModel)
{
    const float c_impactDamageCoefficient = 1.0f / 128.0f;
    const float c_impactJiggleCoefficient = 1.0f / 20.0f;

    ObjectType  type = pModel->GetObjectType();
    switch (type)
    {
        case OT_warp:
        {
            //A ship hitting a warp point
            //Three possibilities:
            //Not closing with the aleph: ignore.
            //Hit the front: go through
            //Hit the back: bounce
            GetMyMission()->GetIgcSite()->HitWarpEvent(this, (IwarpIGC*)pModel);

            /*
            const Vector& velocity1 = GetVelocity();

            Vector        position1 = GetPosition() + velocity1 * tCollision;
            Vector        position2 = pModel->GetPosition();
            Vector        deltaP = position1 - position2;
            if ((deltaP * velocity1) < 0.0f)
            {
                //Ship is moving closer to the aleph ... did we hit the front or the back?
                //if (deltaP * pModel->GetOrientation().GetForward() >= 0.0f)
                {
                    //Hit the front: go through
                    GetMyMission()->GetIgcSite()->HitWarpEvent(this, (IwarpIGC*)pModel);
                }
                else
                {
                    debugf("%s bounced off of %s @ %d\n", GetName(), pModel->GetName(), timeCollision);
                    GetMyMission()->GetIgcSite()->PlaySoundEffect(collisionSound, this);


                    //At the time of the collision, the objects should be exactly radius apart ...
                    //but they may not be due to round-off errors. Make sure they are exactly
                    //radius + epsilon apart
                    Vector        normal;

                    Separate(entry, false, &position1, &position2, &normal);

                    SetPosition(position1);

                    IclusterIGC*    cluster = GetCluster(); //Get the cluster now, before doing damage (and, maybe, terminating the model)

                    AddRef();

                    assert (normal * velocity1 <= 0.0f);

                    const float cElasticity = 1.5;      //Coefficient of elasticity (super-duper balls)
                    //Determine the new velocity as some combination of a
                    //perfectly inelastic and a perfectly elastic collision.
                    //Same basic algorithm as below, but the ship has a mass of 0 and the station has a mass of infinity

                    //Inelastic collision: inelastic velocity is 0, 0, 0 (ship sticks to the station)

                    //Elastic collision: velocity component along the perpendicular is reflected.

                    //Get the speed along the normal vector
                    float   oldSpeed1 = velocity1 * normal;

                    //Get the velocities perpendicular to the normal vector.
                    Vector  perpendicular1 = velocity1 - oldSpeed1 * normal;

                    //invert the parallel component
                    Vector  vElastic1 = perpendicular1 - (oldSpeed1 * normal);

                    Vector  newVelocity1 = vElastic1 * cElasticity;

                    //verify the velocities are valid
                    assert (newVelocity1 * newVelocity1 >= 0.0f);

                    //velocity1 is a reference, so calculate the damage before setting the velocity
                    float   damage1 = (velocity1 - newVelocity1).LengthSquared() * c_impactDamageCoefficient;

                    SetVelocity(newVelocity1);

                    {
                        ReceiveDamage(c_dtmParticle,
                                      2.0f * damage1,
                                      timeCollision,
                                      position1, position2,
                                      pModel);
                    }


                    cluster->RecalculateCollisions(tCollision, this, pModel);

                    Release();
                }
            }
                */
        }
        break;

        case OT_station:
        {
            //A ship hitting a space station
            //The bounce case is similar to the ship-ship case above, except that the math is
            //simplified by treating the space station mass as infinite and only changing the ship's
            //velocity (and only invalidating & regenerating the ship's collisions)
            IstationIGC*            pStation = (IstationIGC*)pModel;
            const IstationTypeIGC*  pst = pStation->GetStationType();
            if (pst->HasCapability(m_myHullType.HasCapability(c_habmFighter)
                                   ? c_sabmLand
                                   : c_sabmCapLand))
			{
                //Get the ship's state ... see if it is in landing mode
                IsideIGC* pside1 = GetSide();
                IsideIGC* pside2 = pModel->GetSide();

                if ((pside1 == pside2) || pside1->AlliedSides(pside1,pside2)) //Imago 7/9/09 ALLY
                {
					//Imago 7/13/09 don't allow pods to rescue in allied ahipyard ;-p
					// don't allow AI carriers to dock in allied stations
 					if ((pside1 != pside2 && pst->HasCapability(c_sabmCapLand) && m_myHullType.HasCapability(c_habmLifepod))
						|| (pside1 != pside2 && m_myHullType.HasCapability(c_habmCarrier) && m_pilotType != c_ptPlayer))
					{
						if (((IdamageIGC*)pModel)->GetFraction() > 0.0f)
			            {
			                //At the time of the collision, the objects should be exactly radius apart ...
			                //but they may not be due to round-off errors. Make sure they are exactly
			                //radius + epsilon apart
			                const Vector& velocity1 = GetVelocity();

			                Vector        position1;
			                Vector        position2;
			                Vector        normal;
			                Separate(entry, false,
			                         &position1, &position2, &normal);

			                IclusterIGC*    cluster = GetCluster(); //Get the cluster now, before doing damage (and, maybe, terminating the model)

			                AddRef();
			                pModel->AddRef();

			                const float cElasticity = 0.75;      //Coefficient of elasticity
			                //Determine the new velocity as some combination of a
			                //perfectly inelastic and a perfectly elastic collision.
			                //Same basic algorithm as below, but the ship has a mass of 0 and the station has a mass of infinity

			                //Inelastic collision: inelastic velocity is 0, 0, 0 (ship sticks to the station)

			                //Elastic collision: velocity component along the perpendicular is reflected.

			                //Get the speed along the normal vector
			                float   oldSpeed1 = velocity1 * normal;

			                //Get the velocities perpendicular to the normal vector.
			                Vector  perpendicular1 = velocity1 - oldSpeed1 * normal;

			                //invert the parallel component
			                Vector  vElastic1 = perpendicular1 - (oldSpeed1 * normal);

			                Vector  newVelocity1 = vElastic1 * cElasticity;

			                //verify the velocities are valid
							// mmf replaced assert with log msg
					        if (!(newVelocity1 * newVelocity1 >= 0.0f)) {
							  debugf("mmf Igc shipIGC.cpp ~835 newVelocity1^2 debug build would have called assert and exited, commented out and set to zero for now\n");
					          newVelocity1.x = 0.0f; newVelocity1.y = 0.0f; newVelocity1.z = 0.0f;
							}
			                // assert (newVelocity1 * newVelocity1 >= 0.0f);

			                //velocity1 is a reference, so calculate the damage before setting the velocity
			                float   damage2 = (velocity1 - newVelocity1).LengthSquared() * c_impactDamageCoefficient;
			                float   damage1 = 2.0f * damage2;

			                float   hp1 = GetHitPoints();
			                if (m_mountedOthers[ET_Shield])
			                    hp1 = ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetFraction() *
			                          ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetMaxStrength();
			                float   hp2 = ((IdamageIGC*)pModel)->GetHitPoints();


			                IIgcSite* igcsite = GetMyMission()->GetIgcSite();

			                DamageResult    dr1 = c_drNoDamage;
			                DamageResult    dr2 = c_drNoDamage;
			                if (!m_myHullType.HasCapability(c_habmLifepod))
			                {
			                    if (hp2 > 0.0f)
			                    {
			                        dr1 = ReceiveDamage(c_dmgidCollision,
			                                            (hp2 * damage1 / (hp2 + damage1)),
			                                            timeCollision,
			                                            position1, position2,
			                                            (type == OT_buildingEffect) ? ((IbuildingEffectIGC*)pModel)->GetAsteroid() : pModel);
			                    }

			                    assert (pModel->GetAttributes() & c_mtDamagable);
			                    if (hp1 > 0.0f)
			                    {
			                        dr2 = ((IdamageIGC*)pModel)->ReceiveDamage(c_dmgidCollision,
			                                                                   hp1 * damage2 / (hp1 + damage2),
			                                                                   timeCollision,
			                                                                   position2, position1,
			                                                                   this);
			                    }
			                }

			                if ((dr1 != c_drKilled) && (dr2 != c_drKilled))
			                {
			                    SetPosition(position1);
			                    SetVelocity(newVelocity1);

			                    cluster->RecalculateCollisions(tCollision, this, pModel);
			                }

			                // play the collision effects...
			                igcsite->PlaySoundEffect(collisionSound, this);
			                igcsite->PlayFFEffect(effectBounce, this);
			                igcsite->PlayVisualEffect(effectJiggle, this, 2.0f * damage1 * c_impactJiggleCoefficient);

			                pModel->Release();
			                Release();
			            }
					}

					if (pStation->InGarage(this, GetPosition() + GetVelocity() * tCollision))
                    {
                        if (GetMyMission()->GetIgcSite()->DockWithStationEvent(this, pStation))
                            break;
                    }
                }
                else if (m_myHullType.HasCapability(c_habmBoard) &&
                         (!GetMyMission()->GetMissionParams()->bInvulnerableStations) &&
                         (pStation->GetShieldFraction() < GetMyMission()->GetFloatConstant(c_fcidDownedShield)))
                {
                    if (pStation->InGarage(this, GetPosition() + GetVelocity() * tCollision))
                    {
                        //Ship is docked ... make the appropriate state change.
                        if (GetMyMission()->GetIgcSite()->DockWithStationEvent(this, pStation))
                            break;
                    }
                }
            }
            else if (m_myHullType.HasCapability(c_habmLifepod) &&
                     pst->HasCapability(c_sabmRescue) &&
					 ((GetSide() == pModel->GetSide()) || IsideIGC::AlliedSides(GetSide(), pModel->GetSide()))) // #ALLY - was: GetSide() == pModel->GetSide()) imago fixed 7/8/09
            {
                if (GetMyMission()->GetIgcSite()->RescueShipEvent(this, NULL))
                    break;
            }
        }
        //No break ... intentionally

        case OT_buildingEffect:
        case OT_asteroid:
        {
            //Ignore any collisions between a drilling builder and an asteroid (could get here by fallthrough from OT_station)
            if (((m_stateM & drillingMaskIGC) != 0) && (type != OT_station))
            {
                assert (m_pilotType == c_ptBuilder);
                if (type == OT_asteroid)
                {
                    //We're a builder and we've collided with an asteroid
                    if (m_bAutopilot && (((IasteroidIGC*)pModel)->GetBuildingEffect() == NULL))
                    {
                        //And we are in control and haven't created a building effect yet: create one
                        GetMyMission()->GetIgcSite()->CreateBuildingEffect(timeCollision, (IasteroidIGC*)pModel, this);
                    }

                    //Create some debris from drilling in
                    const Vector&   backward = GetOrientation().GetBackward();
                    pModel->GetThingSite()->AddHullHit(GetPosition() - backward * GetRadius() - pModel->GetPosition(),
                                                       backward);
                }
                break;
            }
        }

        //No break
        case OT_probe:
        {
            if ((type == OT_probe) && m_myHullType.HasCapability(c_habmLifepod))
            {
                IprobeTypeIGC*              ppt = ((IprobeIGC*)pModel)->GetProbeType();
                ExpendableAbilityBitMask    eabm = ppt->GetCapabilities();

                if ((eabm & c_eabmRescueAny) ||
					((eabm & c_eabmRescue) && ((GetSide() == pModel->GetSide()) || IsideIGC::AlliedSides(GetSide(), pModel->GetSide())))) // #ALLY -was: GetSide() == pModel->GetSide()
                {
                    if (GetMyMission()->GetIgcSite()->RescueShipEvent(this, NULL))
                        break;
                }
            }

            if (((IdamageIGC*)pModel)->GetFraction() > 0.0f)
            {
                //At the time of the collision, the objects should be exactly radius apart ...
                //but they may not be due to round-off errors. Make sure they are exactly
                //radius + epsilon apart
                const Vector& velocity1 = GetVelocity();

                Vector        position1;
                Vector        position2;
                Vector        normal;
                Separate(entry, false,
                         &position1, &position2, &normal);

                IclusterIGC*    cluster = GetCluster(); //Get the cluster now, before doing damage (and, maybe, terminating the model)

                AddRef();
                pModel->AddRef();

                const float cElasticity = 0.75;      //Coefficient of elasticity
                //Determine the new velocity as some combination of a
                //perfectly inelastic and a perfectly elastic collision.
                //Same basic algorithm as below, but the ship has a mass of 0 and the station has a mass of infinity

                //Inelastic collision: inelastic velocity is 0, 0, 0 (ship sticks to the station)

                //Elastic collision: velocity component along the perpendicular is reflected.

                //Get the speed along the normal vector
                float   oldSpeed1 = velocity1 * normal;

                //Get the velocities perpendicular to the normal vector.
                Vector  perpendicular1 = velocity1 - oldSpeed1 * normal;

                //invert the parallel component
                Vector  vElastic1 = perpendicular1 - (oldSpeed1 * normal);

                Vector  newVelocity1 = vElastic1 * cElasticity;

                //verify the velocities are valid
				// mmf replaced assert with log msg
		        if (!(newVelocity1 * newVelocity1 >= 0.0f)) {
				  debugf("mmf Igc shipIGC.cpp ~835 newVelocity1^2 debug build would have called assert and exited, commented out and set to zero for now\n");
		          newVelocity1.x = 0.0f; newVelocity1.y = 0.0f; newVelocity1.z = 0.0f;
				}
                // assert (newVelocity1 * newVelocity1 >= 0.0f);

                //velocity1 is a reference, so calculate the damage before setting the velocity
                float   damage2 = (velocity1 - newVelocity1).LengthSquared() * c_impactDamageCoefficient;
                float   damage1 = 2.0f * damage2;

                float   hp1 = GetHitPoints();
                if (m_mountedOthers[ET_Shield])
                    hp1 = ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetFraction() *
                          ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetMaxStrength();
                float   hp2 = ((IdamageIGC*)pModel)->GetHitPoints();


                IIgcSite* igcsite = GetMyMission()->GetIgcSite();

                DamageResult    dr1 = c_drNoDamage;
                DamageResult    dr2 = c_drNoDamage;
                if (!m_myHullType.HasCapability(c_habmLifepod))
                {
                    if (hp2 > 0.0f)
                    {
                        dr1 = ReceiveDamage(c_dmgidCollision,
                                            (hp2 * damage1 / (hp2 + damage1)),
                                            timeCollision,
                                            position1, position2,
                                            (type == OT_buildingEffect) ? ((IbuildingEffectIGC*)pModel)->GetAsteroid() : pModel);
                    }

                    assert (pModel->GetAttributes() & c_mtDamagable);
                    if (hp1 > 0.0f)
                    {
                        dr2 = ((IdamageIGC*)pModel)->ReceiveDamage(c_dmgidCollision,
                                                                   hp1 * damage2 / (hp1 + damage2),
                                                                   timeCollision,
                                                                   position2, position1,
                                                                   this);
                    }
                }

                if ((dr1 != c_drKilled) && (dr2 != c_drKilled))
                {
                    SetPosition(position1);
                    SetVelocity(newVelocity1);

                    cluster->RecalculateCollisions(tCollision, this, pModel);
                }

                // play the collision effects...
                igcsite->PlaySoundEffect(collisionSound, this);
                igcsite->PlayFFEffect(effectBounce, this);
                igcsite->PlayVisualEffect(effectJiggle, this, 2.0f * damage1 * c_impactJiggleCoefficient);

                pModel->Release();
                Release();
            }
		}
        break;

        case OT_treasure:
        case OT_missile:
        case OT_mine:
        {
            //Let the mine/missile/etc. deal with it.
            pModel->HandleCollision(timeCollision, tCollision, entry, this);
        }
        break;

        case OT_ship:
        {
            IIgcSite* igcsite = GetMyMission()->GetIgcSite();

			bool    bFriendly = ( (GetSide() == pModel->GetSide()) || IsideIGC::AlliedSides(GetSide(), pModel->GetSide())); // #ALLY -was: GetSide() == pModel->GetSide() imago fixed 7/8/09
            if (bFriendly)
            {
                HullAbilityBitMask  habmMe = m_myHullType.GetCapabilities();
                HullAbilityBitMask  habmHim = ((IshipIGC*)pModel)->GetHullType()->GetCapabilities();

                if (habmHim & c_habmLifepod)
                {
                    if ((habmMe & c_habmRescue) && igcsite->RescueShipEvent((IshipIGC*)pModel, this))
                        break;
                }
                else if (habmMe & c_habmLifepod)
                {
                    if ((habmHim & c_habmRescue) && igcsite->RescueShipEvent(this, (IshipIGC*)pModel))
                        break;
                }
				// mmf 3/08 drones (like fighter drones) docking at carrier crash the server
				// if pModel is a drone skip this part (i.e. don't let them dock)
				//
                else if ( (habmHim & c_habmCarrier) && (this->GetPilotType() == c_ptPlayer) )// mmf added && ... tweaked 4/08
                {
                    if ((habmMe & c_habmLandOnCarrier) &&
                        ((IshipIGC*)pModel)->InGarage(this, tCollision) &&
                        igcsite->LandOnCarrierEvent((IshipIGC*)pModel, this, tCollision))
                        break;
                }
                else if ( (habmMe & c_habmCarrier) && (((IshipIGC*)pModel)->GetPilotType() == c_ptPlayer) )// mmf added && ...
                {
                    if ((habmHim & c_habmLandOnCarrier) &&
                        InGarage((IshipIGC*)pModel, tCollision) &&
                        igcsite->LandOnCarrierEvent(this, (IshipIGC*)pModel, tCollision))
                        break;
                }
            }

            //A ship hitting a ship
            //This can get messy ... since it can invalidate all subsequent collisions
            //between either of the ships and every other object in the cluster
            //All collisions will be relative to tOffset & 0.0
            const Vector& velocity1 = GetVelocity();
            const Vector& velocity2 = pModel->GetVelocity();

            //At the time of the collision, the objects should be exactly radius apart ...
            //but they may not be due to round-off errors. Make sure they are exactly
            //radius + epsilon apart
            Vector position1;
            Vector position2;
            Vector normal;
            Separate(entry, true,
                     &position1, &position2, &normal);

            IclusterIGC*    cluster = GetCluster(); //Get the cluster now, before doing damage (and, maybe, terminating the model)

            AddRef();
            pModel->AddRef();

            //Get the speed along the normal vector
            float   oldSpeed1 = velocity1 * normal;
            float   oldSpeed2 = velocity2 * normal;

            //Determine the new velocities as some combination of a
            //perfectly inelastic and a perfectly elastic collision.
            Vector  vInelastic;
            float   newSpeed1;
            float   newSpeed2;

            const float cElasticity = 0.75;      //Coefficient of elasticity
            {
                float   mass1 = GetMass();
                assert (mass1 > 0.0f);

                float   mass2 = pModel->GetMass();
                assert (mass2 > 0.0f);

                //Inelastic collision: both objects have the same final velocity & momentum is conserved
                vInelastic = velocity1 * (mass1 / (mass1 + mass2)) +
                             velocity2 * (mass2 / (mass1 + mass2));
                //Solve:
                //  oldA   * mass1 + oldB   * mass2 = newA   * mass1 + newB   * mass2   (momentum)
                //  oldA^2 * mass1 + oldB^2 * mass2 = newA^2 * mass1 + newB^2 * mass2   (kinetic energy)
                newSpeed1 = (oldSpeed1 * (mass1 - mass2) + 2.0f * oldSpeed2 * mass2) / (mass1 + mass2);
                newSpeed2 = (oldSpeed2 * (mass2 - mass1) + 2.0f * oldSpeed1 * mass1) / (mass1 + mass2);
            }

            //Elastic collision: momentum, kinetic energy and velocity perpendicular to the impact vector
            //are all preserved.

            //Get the velocities perpendicular to the normal vector.
            Vector  perpendicular1 = velocity1 - oldSpeed1 * normal;
            Vector  perpendicular2 = velocity2 - oldSpeed2 * normal;


            Vector  vElastic1 = perpendicular1 + (newSpeed1 * normal);
            Vector  vElastic2 = perpendicular2 + (newSpeed2 * normal);

            Vector  newVelocity1 = vInelastic * (1.0f - cElasticity) + vElastic1 * cElasticity;

            Vector  newVelocity2 = vInelastic * (1.0f - cElasticity) + vElastic2 * cElasticity;

            //verify the velocities are valid
			// mmf replaced assert with log msg
		    if (!(newVelocity1 * newVelocity1 >= 0.0f)) {
		      debugf("mmf Igc shipIGC.cpp ~1008 newVelocity1^2 debug build would have called assert and exited, commented out and set to zero for now\n");
		      newVelocity1.x = 0.0f; newVelocity1.y = 0.0f; newVelocity1.z = 0.0f;
			}
		    // mmf replaced assert with log msg
		    if (!(newVelocity2 * newVelocity2 >= 0.0f)) {
			  debugf("mmf Igc shipIGC.cpp ~1013 newVelocity2^2 debug build would have called assert and exited, commented out and set to zero for now\n");
		      newVelocity2.x = 0.0f; newVelocity2.y = 0.0f; newVelocity2.z = 0.0f;
			}
            //assert (newVelocity1 * newVelocity1 >= 0.0f);
            //assert (newVelocity2 * newVelocity2 >= 0.0f);

            //velocity1 & 2 are references, so calculate the damge before setting the new velocity
            float   base1 = (velocity1 - newVelocity1).LengthSquared() * c_impactDamageCoefficient;
            float   base2 = (velocity2 - newVelocity2).LengthSquared() * c_impactDamageCoefficient;

            float   damage1 = 2.0f * base1 + base2;
            float   damage2 = base1 + 2.0f * base2;

            float   hp1 = GetHitPoints();
            if (m_mountedOthers[ET_Shield])
				// terralthra fix, this was an = should be a +=, comment added by mmf
                hp1 += ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetFraction() *
                      ((IshieldIGC*)m_mountedOthers[ET_Shield])->GetMaxStrength();

            float   hp2 = ((IdamageIGC*)pModel)->GetHitPoints();
            {
                IshieldIGC* pshield = (IshieldIGC*)(((IshipIGC*)pModel)->GetMountedPart(ET_Shield, 0));
                if (pshield)
                    hp2 += pshield->GetFraction() * pshield->GetMaxStrength();
            }

            SetPosition(position1);
            SetVelocity(newVelocity1);

            pModel->SetPosition(position2);
            pModel->SetVelocity(newVelocity2);

            if (!(bFriendly &&
                  (m_myHullType.GetHullType()->HasCapability(c_habmLifepod) ||
                   ((IshipIGC*)pModel)->GetBaseHullType()->HasCapability(c_habmLifepod))))
            {
                if (hp2 > 0.0f)
                {
                    this->ReceiveDamage(c_dmgidCollision,
                                        hp2 * damage1 / (hp2 + damage1),
                                        timeCollision,
                                        position1, position2,
                                        (IshipIGC*)pModel);
                }

                if (hp1 > 0.0f)
                {
                    ((IdamageIGC*)pModel)->ReceiveDamage(c_dmgidCollision,
                                                         hp1 * damage2 / (hp1 + damage2),
                                                         timeCollision,
                                                         position2, position1,
                                                         this);
                }
            }

            igcsite->PlaySoundEffect(collisionSound, this);
            igcsite->PlayFFEffect(effectBounce, this);
            igcsite->PlayVisualEffect(effectJiggle, this, (2.0f * damage1 + damage2) * c_impactJiggleCoefficient);

            igcsite->PlaySoundEffect(collisionSound, pModel);
            igcsite->PlayFFEffect(effectBounce, pModel);
            igcsite->PlayVisualEffect(effectJiggle, pModel, (damage1 + 2.0f * damage2) * c_impactJiggleCoefficient);

            cluster->RecalculateCollisions(tCollision, this, pModel);

            pModel->Release();
            Release();
        }
        break;

        default:
        {
            //This should not happen
            assert (false);
        }
    }
}

DamageResult CshipIGC::ReceiveDamage(DamageTypeID            type,
                                     float                   amount,
                                     Time                    timeCollision,
                                     const Vector&           position1,
                                     const Vector&           position2,
                                     ImodelIGC*              launcher)
{
    IsideIGC*   pside = GetSide();

    if (launcher &&
        (!GetMyMission()->GetMissionParams()->bAllowFriendlyFire) &&
		((pside == launcher->GetSide()) || IsideIGC::AlliedSides(pside,launcher->GetSide())) && // #ALLY - Imago fixed 7/8/09
        (amount >= 0.0f))
    {
        return c_drNoDamage;
    }

    DamageResult    dr;

    float   maxHP = m_myHullType.GetHitPoints();
    float   dtmArmor = GetMyMission()->GetDamageConstant(type, m_myHullType.GetDefenseType());
	float   repairFraction;
    assert (dtmArmor >= 0.0f);

    float leakage;
    if (amount < 0.0f)
    {
        //Repair the target's hull
        m_fraction -= amount * dtmArmor / maxHP;
		if (m_fraction > 1.0f)
		{
			amount += (m_fraction - 1.0) * maxHP / dtmArmor; //Set amount to amount that had effect for stat			
			m_fraction = 1.0f;
		}            
        GetThingSite ()->RemoveDamage (m_fraction);
		if (GetMyMission()->GetMissionParams()->bAllowFriendlyFire || //no points when FF is on
			!((pside == launcher->GetSide()) || IsideIGC::AlliedSides(pside, launcher->GetSide()))) //no points for healing the enemy
			repairFraction = 0;
		else
			repairFraction = fabs(amount * dtmArmor / maxHP);
        leakage = 0.0f;
        dr = c_drNoDamage;
		if (launcher->GetObjectType() == OT_ship && (pside == launcher->GetSide()) || IsideIGC::AlliedSides(pside, launcher->GetSide()))
		{

			IshipIGC * pIship = ((IshipIGC*)launcher);
			pIship->AddRepair(repairFraction);
			pIship->SetAchievementMask(c_achmNewRepair);
		}
    }
    else
    {
        dr = c_drShieldDamage;

        if (launcher)
        {
            if (launcher->GetObjectType() == OT_ship)
                amount *= ((IshipIGC*)launcher)->GetExperienceMultiplier();

            if (m_damageTrack && (launcher->GetMission() == GetMyMission()))
                m_damageTrack->ApplyDamage(timeCollision, launcher, amount);
        }

        if (m_mountedOthers[ET_Shield])
        {
            Vector  deltaP = position2 - position1;
            leakage = ((IshieldIGC*)(m_mountedOthers[ET_Shield]))->ApplyDamage(timeCollision, type, amount, deltaP);
        }
        else
        {
            //No shield (which means the convex hull, if possible, was used) ... all damage is done to the hull
            leakage = amount;
        }

        float   oldFraction = m_fraction;

        if (leakage > 0.0f)
        {
            leakage *= dtmArmor;
            m_fraction -= leakage / maxHP;
            dr = c_drHullDamage;
        }

#ifndef __GNUC__
		// BT - 9/17 - Not sure why this exception is happening here. The stack traces I get from the mini-dumps are not helping much. Maybe this will
		// show more, and also keep the server from crashing?
		__try
		{
#endif
			if (m_fraction > 0.0f)
			{
				if ((type & c_dmgidNoDebris) == 0)
					GetThingSite()->AddDamage(position2 - position1, m_fraction);
				GetMyMission()->GetIgcSite()->DamageShipEvent(timeCollision, this, launcher, type, amount, leakage, position1, position2);
			}
			else
			{
				m_fraction = 0.0f;
				if (oldFraction > 0.0f)  //Only send the death message once.
				{
					// TE: Get the player credited for the kill
					DamageBucketLink* pdmglink = NULL;
					ImodelIGC* pcredit = launcher;
					DamageTrack*  pdt = this->GetDamageTrack();
					if (pdt)
					{
						pdmglink = pdt->GetDamageBuckets()->first();
						if (pdmglink)
						{
							if (pdmglink->data()->model()->GetMission() == GetMyMission())
								pcredit = pdmglink->data()->model();
						}
					}
					// TE: end
					GetMyMission()->GetIgcSite()->KillShipEvent(timeCollision, this, pcredit, amount, position1, position2);
					dr = c_drKilled;
				}
			}
#ifndef __GNUC__
        }
		__except (StackTracer::ExceptionFilter(GetExceptionInformation()))
		{
			StackTracer::OutputStackTraceToDebugF();
		}
#endif
    }
    //Imago 6/10
    //assert (m_fraction >= 0.0f);
    //assert (m_fraction <= 1.0f);

    return dr;
}

void CshipIGC::SetBaseHullType(IhullTypeIGC* newVal)
{
    assert (m_pshipParent == NULL);

    SetStateM(0);
    ResetDamageTrack();

    assert (newVal);
    IsideIGC*   pside = GetSide();

    //Move the ship to the null cluster before changing the hull type
    IclusterIGC*    pclusterOld = GetCluster();
    Vector          positionOld;
    if (pclusterOld)
    {
        //The ship was in a cluster when it changed hull types
        //(probably an ejecting). Go do almost all the stuff that
        //a SetCluster() would do (but avoid the change cluster callback
        //and
        positionOld = GetPosition();

        pclusterOld->DeleteModel(this);
        GetThingSite()->SetCluster(this, NULL);
    }

    assert (m_parts.n() == 0);

    //Move any gunless gunners to observer status
    if (m_shipsChildren.first())
    {
        Mount   oldFixed = m_myHullType.GetHullType()->GetMaxFixedWeapons();
        Mount   newFixed = newVal->GetMaxFixedWeapons();
        Mount   maxTurrets = newVal->GetMaxWeapons() - newFixed;
        for (ShipLinkIGC*   psl = m_shipsChildren.first(); (psl != NULL); psl = psl->next())
        {
            IshipIGC*   pship = psl->data();

            Mount   tid = pship->GetTurretID();
            if (tid != NA)
            {
                tid -= oldFixed;
                if (tid >= maxTurrets)
                    pship->SetTurretID(NA);
                else if (oldFixed != newFixed)
                    pship->SetTurretID(tid + newFixed);
            }
        }
    }

    m_myHullType.SetHullType(newVal);
    SetMass(m_myHullType.GetMass());

    SetRipcordModel(NULL);
    m_ripcordCost = newVal->GetRipcordCost();

    if (newVal->HasCapability(c_habmLifepod))
        m_timeToDie = GetLastUpdate() + GetMyMission()->GetFloatConstant(c_fcidLifepodEndurance);

    //Try to load the ship's model
    {
        HRESULT hr = Load(0,
                          m_myHullType.GetModelName(),
                          m_myHullType.GetTextureName(),
                          m_myHullType.GetIconName(),
                          c_mtDamagable | c_mtHitable | c_mtCastRay | c_mtSeenBySide | c_mtScanner);
        ZSucceeded(hr);
    }
    SetRadius(0.5f * m_myHullType.GetLength());

    HitTest*    pht = GetHitTest();

    m_cockpit = pht->GetFrameOffset("cockpt");

    //Set the key to get permission to enter the shields of a stations on their side.
    pht->SetUseTrueShapeOther(pside);

    m_fOre = 0.0f;
    m_fuel = 0.0f;
    m_ammo = 0;

    m_nextLaunchSlot = 0;
    m_warningMask = 0;

    m_fraction = 1.0f;
    m_fractionLastOrder = 1.0f;
    m_energy = 0.0f;

    //signature starts at the hull siganture and is then modified by parts
    SetSignature(m_myHullType.GetSignature());

    m_engineVector = Vector::GetZero();


    //Put us back in the cluster we started in.
    if (pclusterOld)
    {
        SetPosition(positionOld);

        pclusterOld->AddModel(this);

        GetThingSite()->SetCluster(this, pclusterOld);
    }

    GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, NULL, c_lcHullChange);

    ResetWaypoint();
}

void    CshipIGC::AddPart(IpartIGC* part)
{
    assert (part);

    ZVerify(m_parts.last(part));
    part->AddRef();

    assert (part->GetPartType());
    SetMass(GetMass() + part->GetMass());

    GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, part, c_lcAdded);

/*
    debugf("Adding\t%s/%d to %s/%d\n", part->GetPartType()->GetName(), m_parts.n(),
           GetName(), GetObjectID());
*/

    assert (GetMass() > 0.0f);  //No massless ships
}

void    CshipIGC::DeletePart(IpartIGC* part)
{
    assert (part);
    part->SetMountID(c_mountNA);

    int iPart = 0;      //NYI debug

    part->AddRef();
    for (PartLinkIGC*   l = m_parts.first(); (l != NULL); l = l->next())
    {
        iPart++;    //NYI debug

        IpartIGC*   p = l->data();

        if (p == part)
        {
            SetMass(GetMass() - part->GetMass());

/*
            debugf("Deleting\t%s/%d to %s/%d (%d)\n", part->GetPartType()->GetName(), iPart,
                   GetName(), GetObjectID(), m_parts.n() - 1);
*/

            delete l;
            p->Release();
            break;
        }

    }

    GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, part, c_lcRemoved);
    part->Release();

    assert (GetMass() > 0.0f);  //No massless ships
}

IpartIGC* const*   CshipIGC::PartLocation(EquipmentType  type,
                                          Mount          mountID) const
{
    IpartIGC* const*  partLocation;
    if (mountID < 0)
    {
        assert (mountID >= -c_maxCargo);

        partLocation = &m_mountedCargos[mountID + c_maxCargo];
    }
    else
    {
        switch (type)
        {
            case ET_Weapon:
            {
                assert (mountID < c_maxMountedWeapons);
                partLocation = (IpartIGC* const*)&(m_mountedWeapons[mountID]);
            }
            break;

            default:
            {
                assert (type < ET_MAX);
                assert (mountID == 0);
                partLocation = &m_mountedOthers[type];
            }
            break;
        }

        assert (partLocation);
    }
    return partLocation;
}

IpartIGC*   CshipIGC::GetMountedPart(EquipmentType  type,
                                     Mount          mountID) const
{
    return *PartLocation(type, mountID);
}

void    CshipIGC::MountPart(IpartIGC*    part,
                            Mount        mountNew,
                            Mount*       pmountOld)
{
    assert (part);

    //part must be among in the ship's part list (though this is not verified).
    IpartTypeIGC*   partType = part->GetPartType();
    assert (partType);

    part->SetMountedFraction(0.0f);

    //Ick! we need to cast the constness of the partLocation away below because that
    //is better than having two identical PartLocation() methods
    EquipmentType   et = partType->GetEquipmentType();

    if (*pmountOld >= -c_maxCargo)
    {
        IpartIGC**      plOld = (IpartIGC**)PartLocation(et, *pmountOld);

        assert (*plOld == part);
        *plOld = NULL;
    }

    if (mountNew >= -c_maxCargo)
    {
        assert (m_myHullType.CanMount(partType, mountNew));

        IpartIGC**  plNew = (IpartIGC**)PartLocation(et, mountNew);
        assert (*plNew == NULL);
        *plNew = part;
    }

    *pmountOld = mountNew;

    GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, part, (mountNew < 0) ? c_lcDismounted : c_lcMounted);
}

void    CshipIGC::SetStateM(int newVal)
{
    if (m_pshipParent == NULL)
    {
        if ((m_station == NULL) && (m_mountedOthers[ET_Cloak] != NULL))
        {
            if (m_pmodelRipcord)
                newVal &= ~cloakActiveIGC;

            int newCloak = (newVal & cloakActiveIGC);

            if (newCloak != (m_stateM & cloakActiveIGC))
            {
                GetMyMission()->GetIgcSite()->PlayNotificationSound(newCloak
                                                                    ? salCloakEngageSound
                                                                    : salCloakDisengageSound, this);
            }
        }
    }

    m_stateM = newVal;
}

void    CshipIGC::ExecuteTurretMove(Time          timeStart,
                                    Time          timeStop,
                                    Orientation*  pOrientation)
{
    //Adjust turret's facing based
    float   dT = timeStop - timeStart;
    //Note: turret time can run forwards or backwards

    //constrain the desired yaw/pitch/roll rates to a circle rather than a box
    float   l = m_controls.jsValues[c_axisYaw]   * m_controls.jsValues[c_axisYaw] +
                m_controls.jsValues[c_axisPitch] * m_controls.jsValues[c_axisPitch] +
                m_controls.jsValues[c_axisRoll]  * m_controls.jsValues[c_axisRoll];

    if (l > 1.0f)
        l = 1.0f / sqrt(l);
    else
        l = 1.0f;

    float   oldTurnRate2 = m_turnRates[c_axisYaw]   * m_turnRates[c_axisYaw] +
                           m_turnRates[c_axisPitch] * m_turnRates[c_axisPitch];

    //Keep in sync with wintrek.cpp's FOV by throttle
    static const float  c_minRate = RadiansFromDegrees(7.5f);
    static const float  c_maxRate = RadiansFromDegrees(75.0f);
    float   slewVelocity = (c_minRate + 0.5f * (c_maxRate - c_minRate)) +
                           (0.5f * (c_maxRate - c_minRate)) * m_controls.jsValues[c_axisThrottle];

    for (int i = 0; (i < 3); i++)
        m_turnRates[i] = m_controls.jsValues[i] * l * slewVelocity;

    pOrientation->Yaw(   m_turnRates[c_axisYaw] * dT);
    pOrientation->Pitch(-m_turnRates[c_axisPitch] * dT);
    pOrientation->Roll(  m_turnRates[c_axisRoll] * dT);

    assert (m_pshipParent);
    assert (m_turretID >= m_pshipParent->GetHullType()->GetMaxFixedWeapons());
    assert (m_turretID < m_pshipParent->GetHullType()->GetMaxWeapons());

    /*
    const IhullTypeIGC*     pht = m_pshipParent->GetHullType();
    const HardpointData&    hd = pht->GetHardpointData(m_turretID);
    if (hd.minDot >= -0.75f)
    {
        Orientation orientationTurret = m_pshipParent->GetHullType()->GetWeaponOrientation(m_turretID) *
                                        m_pshipParent->GetOrientation();

        Vector  backward = orientationTurret.TimesInverse(pOrientation->GetBackward());

        if (backward.z <= hd.minDot)
        {
            //Clank
            const float c_fNoClank = RadiansFromDegrees(1.0f);
            if (oldTurnRate2 > c_fNoClank * c_fNoClank)
            {
                GetMyMission()->GetIgcSite()->PlayNotificationSound(turretLimitSound, this);
            }

            //Hit the turret limit
            m_turnRates[c_axisYaw] = m_turnRates[c_axisPitch] = 0.0f;

            float   l2 = backward.x * backward.x + backward.y * backward.y;
            assert (l2 != 0.0f);
            float   k = -float(sqrt((1.0f - hd.minDot * hd.minDot) / l2));

            Vector  v(k * backward.x, k * backward.y, -hd.minDot);
            assert (v*v >= 0.98f);
            assert (v*v <= 1.02f);

            v = v * orientationTurret;

            pOrientation->TurnTo(v);
        }
        else
        {
            // Re-normalize the orientation matrix
            pOrientation->Renormalize();
        }
    }
    else
    */
    {
        // Re-normalize the orientation matrix
        pOrientation->Renormalize();
    }
}

void    CshipIGC::PreplotShipMove(Time          timeStop)
{
    IclusterIGC*    pcluster = GetCluster();
    const Vector&   positionMe = GetPosition();

    if (m_bAutopilot && (m_pshipParent == NULL) && ((m_stateM & buildingMaskIGC) == 0))
    {
        // debugf("%-20s %x %f %f %f\n", GetName(), timeStop.clock(), GetPosition().x, GetPosition().y, GetPosition().z);

        //First ... do we need to run away?
        if (m_pilotType < c_ptCarrier && !fRipcordActive())      //Carriers never run //TurkeyXIII added ripcord 7/10 - Imago
        {
            if (m_timeRanAway + c_dtCheckRunaway <= timeStop)
            {
                bool    bDamage = true;
                bool    bRunAway = true;
                if (m_pilotType == c_ptWingman)
				{
					// bahdohday&AEM 7.09.07 Added check to allow certain wingmen drones to never run away: if they have a nan in slot 1 or are have a station as their target
					if ( (m_mountedWeapons[0] && m_mountedWeapons[0]->GetProjectileType()->GetPower() < 0.0 ) || ( m_commandTargets[c_cmdAccepted] && (m_commandTargets[c_cmdAccepted]->GetObjectType() == OT_station) ) )
					{
						bRunAway = false;
					}
					else
					{
						bRunAway = m_fraction < m_fractionLastOrder; //previously just this line was here
					}
                }
                else if ((m_pilotType == c_ptBuilder) || (m_pilotType == c_ptLayer))
                {
                    if (m_fraction < m_fractionLastOrder)
                    {
                        if (m_commandIDs[c_cmdAccepted] == c_cidBuild)
                        {
                            assert ((m_pilotType == c_ptBuilder) || (m_pilotType == c_ptLayer));

                            //Builders do not run if they are ordered to build & closer to their target than the station
                            //but a station or a target in another cluster is always considered infinitely far away
                            assert (m_commandTargets[c_cmdAccepted]);
                            ImodelIGC*  pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest,
                                                            NULL, NULL, NULL, NULL, c_sabmRepair);
                            if (pmodel)
                            {
                                if (m_commandTargets[c_cmdAccepted]->GetCluster() == GetCluster())
                                {
                                    //Found a place to run to in this cluster
                                    if ((positionMe - m_commandTargets[c_cmdAccepted]->GetPosition()).LengthSquared() <=
                                        (positionMe - pmodel->GetPosition()).LengthSquared())
                                    {
                                        //easier to get to the target than the base ... so why run
                                        bRunAway = false;
                                    }
                                }
                            }
                            else
                                bRunAway = false;
                        }
                    }
                    else
                        bRunAway = false;
                }
                else
                {
                    // assert (m_pilotType == c_ptMiner);
					// we are expecting a miner at this stage
					// mmf replaced assert with log msg to track down what is triggering it
					//     recent logs (07/04/2007) show that the pilot type is 1 and it is a Recon or Rescue ship
					//     so don't log if it is a 1 now
					if ( ! (m_pilotType == c_ptMiner) ) {
						if (m_pilotType != 1) {
						  debugf ("mmf shipIGC.cpp assert (m_pilotType == c_ptMiner), m_pilotType = %d\n",
					              m_pilotType);
						}
					}
                    if (m_fraction >= 0.95f)  // mmf added Your_Persona's change, allow Miners to be slightly damaged  (was == 100.0f)
                    {
                        IshieldIGC* pshield = (IshieldIGC*)(m_mountedOthers[ET_Shield]);
                        if ((pshield == NULL) || (pshield->GetFraction() >= 0.75f))
                        {
                            bDamage = false;

                            //full hull & shield
                            //Does anyone see us?
                            IsideIGC*       psideMe = GetSide();

                            int     cEnemy = 0;
                            int     cFriend = 0;
                            float   d2Enemy = FLT_MAX;
                            float   d2Friend = FLT_MAX;

                            for (ShipLinkIGC*   psl = pcluster->GetShips()->first(); (psl != NULL); psl = psl->next())
                            {
                                IshipIGC*   pship = psl->data();
                                if ((pship->GetPilotType() >= c_ptPlayer) &&
                                    (pship->GetParentShip() == NULL) &&
                                    !pship->GetBaseHullType()->HasCapability(c_habmLifepod))
                                {
                                    IsideIGC*   pside = pship->GetSide();

                                    if (((pside == psideMe) || pside->AlliedSides(pside,psideMe)) ||
										(CanSee(pship) && SeenBySide(pside)) //#ALLY - friendly nearby (seen or can see us) IMAGO FIXED 7/10/09
										)
                                    {
                                        cFriend++;
                                        float d2 = (positionMe - pship->GetPosition()).LengthSquared();
                                        if (d2 < d2Friend)
                                            d2Friend = d2;
                                    }
                                    else if (CanSee(pship) && SeenBySide(pside))
                                    {
                                        cEnemy++;
                                        float d2 = (positionMe - pship->GetPosition()).LengthSquared();
                                        if (d2 < d2Enemy)
                                            d2Enemy = d2;
                                    }
                                }
                            }

                            if (cFriend >= cEnemy)
                                bRunAway = false;
                            else
                            {
                                static const float  c_d2AlwaysRun = 1000.0f;
                                if (d2Enemy > c_d2AlwaysRun * c_d2AlwaysRun)
                                    bRunAway = (d2Enemy < d2Friend);
                            }
                        }
                    }
                }

                if (bRunAway)
                {
                    //We'd like to run ... are we running already?
                    if (!m_bRunningAway)
                    {
                        //Not running ... we should be
                        ImodelIGC*  pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
                                                        NULL, NULL, NULL, NULL, c_sabmRepair);

                        //but only if we can find some place to run to
						//mmf debuging code
						// training mission 6 triggers this for GetName = Enemy Support
						//if (!pmodel) {
						//  debugf("mmf Miner/con tried to run but did not find anyhwhere to go\n");
						//  debugf("%-20s %x %f %f %f\n", GetName(), timeStop.clock(), positionMe.x, positionMe.y, positionMe.z);
						//}
						//mmf end debugging code

						if (pmodel)
                        {
                            SetCommand(c_cmdPlan, pmodel, c_cidGoto);

                            if (m_pilotType == c_ptBuilder)
                                GetMyMission()->GetIgcSite()->SendChat(this, CHAT_TEAM, GetSide()->GetObjectID(),
                                                                       constructorRunningSound, "Constructor heading for cover.");
                            else if (bDamage)
                                GetMyMission()->GetIgcSite()->SendChat(this, CHAT_TEAM, GetSide()->GetObjectID(),
                                                                       droneTooMuchDamageSound, "Forget this. I have to go get repaired");
                            else
                                GetMyMission()->GetIgcSite()->SendChat(this, CHAT_TEAM, GetSide()->GetObjectID(),
                                                                       droneEnemyOnScopeSound, "Enemy spotted; returning to base.");

                            //Set m_bRunningAway after the SetCommand (which clears it)
                            m_bRunningAway = true;
                            m_timeRanAway = timeStop;

						    //mmf debuging code
						    //debugf("mmf Miner/con found place to run to.\n");
							//debugf("%-20s %x I am at %f %f %f\n", GetName(), timeStop.clock(), positionMe.x, positionMe.y, positionMe.z);
							//debugf("running to %f %f %f\n",pmodel->GetPosition().x,pmodel->GetPosition().y,pmodel->GetPosition().z);
						    //mmf end debugging code
						}
                    }
                }
                else if (m_bRunningAway)
                {
                    //We want to stop running
                    SetCommand(c_cmdPlan, NULL, c_cidNone);
					// debugf("mmf %-20s stoped running\n", GetName());
                    assert (m_bRunningAway == false);   //Set by SetCommand
                    m_timeRanAway = timeStop;
                }
            }

            if (!LegalCommand(m_commandIDs[c_cmdPlan], m_commandTargets[c_cmdPlan]))
            {
                if (LegalCommand(m_commandIDs[c_cmdAccepted], m_commandTargets[c_cmdAccepted]))
                {
                    //No current target and our accepted order makes sense ... do it instead
                    SetCommand(c_cmdPlan, m_commandTargets[c_cmdAccepted], m_commandIDs[c_cmdAccepted]);
                }
                else
                {
                    PickDefaultOrder(pcluster, positionMe, false);
                }
            }

            if (m_pilotType == c_ptMiner)
            {
                int    stateM = 0;
                if (m_commandTargets[c_cmdPlan] && (m_commandTargets[c_cmdPlan]->GetCluster() == pcluster))
                {
                    if (m_commandIDs[c_cmdPlan] == c_cidMine)
                    {
                        assert (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_asteroid);
                        assert (((IasteroidIGC*)(ImodelIGC*)m_commandTargets[c_cmdPlan])->HasCapability(m_abmOrders));

                        //Are we close enough to mine our target asteroid?
                        Vector  dp = m_commandTargets[c_cmdPlan]->GetPosition() - positionMe;
                        float   distance2 = dp.LengthSquared();
                        float   radius = m_commandTargets[c_cmdPlan]->GetRadius() + GetRadius() + 30.0f;
                        if ((distance2 < radius * radius) && (GetVelocity().LengthSquared() < 1.0f))
                        {
                            //We are in a position to mine the asteroid
                            stateM = wantsToMineMaskIGC;
                        }
                        else if (m_gotoplan.GetMaskWaypoints() == 0)
                            ResetWaypoint();        //Not close enough: got bumped
                    }
                    else if ((m_fOre > 0.0f) &&
                             (m_commandIDs[c_cmdPlan] == c_cidGoto) &&
                             (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_station))
                    {
                        IstationIGC*    pstation = (IstationIGC*)((ImodelIGC*)m_commandTargets[c_cmdPlan]);
                        if (pstation->GetBaseStationType()->HasCapability(c_sabmTeleportUnload))
                        {
                            //Are we close enough to mine our target asteroid? // mmf incorrect comment
                            Vector  dp = pstation->GetPosition() - positionMe;
                            float   distance2 = dp.LengthSquared();
                            float   radius = pstation->GetRadius() + GetRadius() + 100.0f;
                            if (distance2 < radius * radius)
                            {
                                //We can teleport offload ... drop everything and pick a new order
                                IsideIGC*   pside = GetSide();

                                GetMyMission()->GetIgcSite()->PaydayEvent(pside,
                                                                          m_fOre *
                                                                          GetMyMission()->GetFloatConstant(c_fcidValueHe3) *
                                                                          pside->GetGlobalAttributeSet().GetAttribute(c_gaMiningYield));
                                m_fOre = 0.0f;

                                PickDefaultOrder(pcluster, positionMe, false);
                            }
                        }
                    }
                }

                SetStateBits(miningMaskIGC | wantsToMineMaskIGC, stateM);
            }
        }
    }
}

void    CshipIGC::PlotShipMove(Time          timeStop)
{
    if (m_bAutopilot && (m_pshipParent == NULL))
    {
        Time    timeStart = GetMyLastUpdate();

        {
            if (m_pclusterRequestRipcord && (m_timeRequestRipcord > timeStart) && (GetCluster() != m_pclusterRequestRipcord))
            {
                if (IsSafeToRipcord())
                {
                    GetMyMission()->GetIgcSite()->RequestRipcord(this, m_pclusterRequestRipcord);
                    m_pclusterRequestRipcord = NULL;
					//Xynth #47 7/2010					
					SetStateBits(droneRipMaskIGC, droneRipMaskIGC);
					SetStateBits(miningMaskIGC, 0); //stop mining when you rip
                }
                else
                {
                    m_timeRequestRipcord = timeStart + 5.0f;
                }
            }
        }

        bool    bControlsSet = false;
        float   dT = timeStop - timeStart;

        //Special case up front: miners mine
        if (m_pilotType == c_ptMiner)
		{
            if ((m_stateM & wantsToMineMaskIGC) != 0)
            {
                IclusterIGC*    pcluster = GetCluster();

                //We would like to mine ... but other ships may feel the same way
                //Count the number of ships wanting to mine and also find the friendly ship
                //with the least ore
                IsideIGC*   pside = GetSide();

                short   nFriendly = 0;
                short   nEnemy = 0;

                IshipIGC*   pshipMin;

                {
                    float       fOreMin = FLT_MAX;
                    for (ShipLinkIGC*   psl = pcluster->GetShips()->first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        IshipIGC*   pship = psl->data();

                        if (((pship->GetStateM() & wantsToMineMaskIGC) != 0) &&
                            (pship->GetCommandTarget(c_cmdPlan) == m_commandTargets[c_cmdPlan]))
                        {
                            if ((pship->GetSide() == pside) || IsideIGC::AlliedSides(pside,pship->GetSide()))  //ALLY imago 7/9/09
                            {
                                //Have a miner on our side that is actively trying to mine this asteroid
                                nFriendly++;

								float   fOre = pship->GetOre();
                                if (fOre < fOreMin)
                                {
                                    fOreMin = fOre;
                                    pshipMin = pship;
                                }
                            }
                            else
									nEnemy++;
                        }
                    }
                }

                assert (nFriendly > 0);

                if (nEnemy == 0)
                {
                    if (nFriendly > 1)
                    {
                        //Multiple people on our side want to mine ... but only pshipMin can
                        //For the others ... tell them where to go
                        {
                            //Tell all the unlucky sods where to go to unload
                            for (ShipLinkIGC*   psl = pcluster->GetShips()->first();
                                 (psl != NULL);
                                 psl = psl->next())
                            {
                                IshipIGC*   pship = psl->data();

                                if (((pship->GetStateM() & wantsToMineMaskIGC) != 0) &&
                                    (pship->GetCommandTarget(c_cmdCurrent) == m_commandTargets[c_cmdPlan]) &&
                                    (pship->GetSide() == pside) &&
                                    pship != pshipMin)
                                {
                                    //Have something other than the ship that will be allowed to mine
                                    //tell it that it can no longer mine
                                    pship->SetStateBits(miningMaskIGC | wantsToMineMaskIGC, 0);

                                    {
                                        ImodelIGC*  pmodel = FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
                                                                        NULL, NULL, NULL, NULL, c_sabmUnload | c_sabmLand);

                                        //If we can't find a place to unload ... stick around here for lack of a better place to go
                                        if (pmodel)
                                        {
                                            float   capacity = GetMyMission()->GetFloatConstant(c_fcidCapacityHe3) *
                                                               GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMiningCapacity);

                                            pship->SetCommand(c_cmdPlan, pmodel, c_cidGoto);

                                            if (pship->GetOre() < capacity)
                                                GetMyMission()->GetIgcSite()->SendChat(pship, CHAT_TEAM, GetSide()->GetObjectID(),
                                                                                       droneComingHomeEmptySound, "Coming home empty");
                                        }
										else { // mmf added debugf and else curly braces
                                            pship->SetCommand(c_cmdAccepted, NULL, c_cidNone);
											// debugf("mmf %-20s no place to unload staying here, I am at %f %f %f\n",
											//	GetName(), GetPosition().x, GetPosition().y, GetPosition().z);
										}
                                    }
                                }
                            }
                        }
                    }

                    pshipMin->SetStateBits(miningMaskIGC | wantsToMineMaskIGC, miningMaskIGC);
                }
                else
                {
                    //Enemy ships present: no one can mine and everyone is unhappy
                    for (ShipLinkIGC*   psl = GetCluster()->GetShips()->first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        IshipIGC*   pship = psl->data();

                        if (((pship->GetStateM() & wantsToMineMaskIGC) != 0) &&
                            (pship->GetCommandTarget(c_cmdCurrent) == m_commandTargets[c_cmdPlan]))
                        {
                            pship->Complain(droneTooCrowdedSound, "Miner requesting unoccupied He3 asteriod.");
                            pship->SetStateBits(miningMaskIGC | wantsToMineMaskIGC, 0);
                        }
                    }
                }
            }

            if ((m_stateM & miningMaskIGC) != 0)
            {
                //We are (now) the only side at this asteroid
                IasteroidIGC*   pasteroid = ((IasteroidIGC*)(ImodelIGC*)m_commandTargets[c_cmdPlan]);

                const GlobalAttributeSet&   ga = GetSide()->GetGlobalAttributeSet();
                float   minedOre = dT * ga.GetAttribute(c_gaMiningRate);
                float   capacity = GetMyMission()->GetFloatConstant(c_fcidCapacityHe3) * ga.GetAttribute(c_gaMiningCapacity);

                if (m_fOre + minedOre >= capacity)
                {
                    minedOre = capacity - m_fOre;
                    {
                        ImodelIGC*  pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
                                                        NULL, NULL, NULL, NULL, c_sabmUnload);

                        //If we can't find a place to unload ... stick around here for lack of a better place to go
                        if (pmodel)
                            SetCommand(c_cmdPlan, pmodel, c_cidGoto);
						// mmf added else and debugf
						// else debugf("mmf %-20s no place to unload staying here, I am at %f %f %f\n",
						// 						GetName(), GetPosition().x, GetPosition().y, GetPosition().z);
                    }
                }

                float   actualOre = pasteroid->MineOre(minedOre);
                m_fOre += actualOre;

                if (actualOre != minedOre)
                {
                    //Tapped this asteroid out ... pick a new default order.
                    PickDefaultOrder(GetCluster(), GetPosition(), false);
                }

                // Keep facing the target, and add a little roll for effect
                Vector  dp = pasteroid->GetPosition() - GetPosition();
                turnToFace(dp, dT, this, &m_controls, 1.0f);

                m_controls.jsValues[c_axisRoll] = 1.0f;
                m_controls.jsValues[c_axisThrottle] = -1.0f;

		        // Zero the motion & weapon bits
                SetStateBits(weaponsMaskIGC | (buttonsMaskIGC & ~miningMaskIGC), 0);

                return;
            }
        }
        else if (m_pilotType == c_ptBuilder)
        {
            if ((m_stateM & buildingMaskIGC) != 0)
            {
                //Still on track ... continue
                this->m_controls.jsValues[c_axisYaw] = 0.0f;
                this->m_controls.jsValues[c_axisPitch] = 0.0f;
                this->m_controls.jsValues[c_axisRoll] = 1.0f;
                this->m_controls.jsValues[c_axisThrottle] = -0.8f;

                return;
            }
            else if ((m_stateM & drillingMaskIGC) != 0)
            {
                assert (m_commandTargets[c_cmdPlan]);
                assert (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_asteroid);

                //Are we still in a position to drill?
                Vector  dp = GetPosition() - m_commandTargets[c_cmdPlan]->GetPosition();
                const Vector&   axis = m_commandTargets[c_cmdPlan]->GetRotation().axis();

                float   radius = GetRadius() + m_commandTargets[c_cmdPlan]->GetRadius() + 25.0f;

                float   dot = axis * dp;
                float   distance2 = dp.LengthSquared();
                float   offset2 = distance2 - dot*dot;
                if ((offset2 < 50.0f) && (distance2 < radius * radius))
                {
                    //Still on track ... continue
                    this->m_controls.jsValues[c_axisYaw] = 0.0f;
                    this->m_controls.jsValues[c_axisPitch] = 0.0f;
                    this->m_controls.jsValues[c_axisRoll] = 1.0f;
                    this->m_controls.jsValues[c_axisThrottle] = 0.0f;

                    return;
                }
                else
                {
                    //Off-track ... clear the masks and get back into position
                    assert (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_asteroid);

                    SetStateM(0);
                }
            }
        }

        if (m_commandTargets[c_cmdPlan] == NULL)
        {
            if ((m_pilotType < c_ptCarrier) && (m_commandIDs[c_cmdPlan] != c_cidDoNothing))
            {
                switch (m_pilotType)
                {
                    case c_ptMiner:
                        Complain(droneWhereToSound, "Miner requesting He3 asteriod.");
                        break;

                    case c_ptBuilder:
                        Complain(
                            ((IstationTypeIGC*)(IbaseIGC*)m_pbaseData)->GetConstructorNeedRockSound(),
                            "Constructor requesting asteroid.");
                        break;

                    case c_ptLayer:
                        if (m_pbaseData->GetObjectType() == OT_mineType)
                            Complain(droneWhereToLayMinefieldSound, "Minefield requesting location.");
                        else
                            Complain(droneWhereToLayTowerSound, "Tower requesting location.");
                        break;
                }
            }

            int state = 0;
            Dodge(this, NULL, &state);

            SetStateBits(backwardButtonIGC |
                         forwardButtonIGC |
                         leftButtonIGC |
                         rightButtonIGC |
                         upButtonIGC |
                         downButtonIGC |
                         afterburnerButtonIGC |
                         coastButtonIGC |
                         oneWeaponIGC |      //Disable any weapon fire
                         allWeaponsIGC, state);

            m_controls.Reset();
        }
        else
        {
            m_dtTimeBetweenComplaints = c_dtTimeBetweenComplaints;

            if (((m_pilotType == c_ptWingman) || (m_pilotType == c_ptCheatPlayer)) &&
                (m_commandTargets[c_cmdPlan]->GetCluster() == GetCluster()))
            {
                if ( m_commandIDs[c_cmdPlan] == c_cidAttack )
                {
                    //In the same cluster as the target ... we dodge, turn to face the aim point and fire if close enough
                    float   fShootSkill = 0.75f;
                    float   fTurnSkill = 0.75f;
                    int     state = 0;
                    bool    bDodge = Dodge(this, NULL, &state);

                    Vector  direction;
                    float   t = solveForLead(this,
                                             m_commandTargets[c_cmdPlan],
                                             m_mountedWeapons[0],
                                             &direction,
                                             fShootSkill);

                    float   da = turnToFace(direction, dT, this, &m_controls, fTurnSkill);

                    // make the drone always fly hard - throttle range 0.5 to 1.0, depending on how they are
                    // angled to their target
                    m_controls.jsValues[c_axisThrottle] = 0.5f + ((pi - da) / (2.0f * pi));

                    const float c_fMaxOffAngle = 0.10f;
                    float lifespan = m_mountedWeapons[0]->GetLifespan();

                    // commenting this to get strafe behavior from the drones BSW 10/28/1999
                    /*
                    if ((!bDodge) && (t <= lifespan * 0.25f))
                    {
                        //We are close and not dodging anything so ...
                        //strafe
                        state = leftButtonIGC;
                        if (da < pi * 0.5f)
                        {
                            if (da > c_fMaxOffAngle)
                            {
                                //too close ... back off
                                state |= backwardButtonIGC;

                            }
                            else
                                state |= forwardButtonIGC;
                        }
                    }
                    */

                    SetStateBits(~selectedWeaponMaskIGC,
                                 ((da <= c_fMaxOffAngle) && (t <= lifespan * 0.9f))
                                 ? (state | allWeaponsIGC)
                                 : state);

                    return;
                }
				//AEM 7.9.07 allow wingman to repair if they are equipped with a nan
                else if ( m_commandIDs[c_cmdPlan] == c_cidRepair && m_mountedWeapons[0]->GetProjectileType()->GetPower()<0.0 )
                {
                    //In the same cluster as the target ... we dodge, turn to face the aim point and fire if close enough
                    float   fShootSkill = 1.00f;
                    float   fTurnSkill = 1.00f;
                    int     state = 0;
                    bool    bDodge = Dodge(this, NULL, &state);

                    Vector  direction;
                    float   t = solveForLead(this,
                                             m_commandTargets[c_cmdPlan],
                                             m_mountedWeapons[0],
                                             &direction,
                                             fShootSkill);

                    float   da = turnToFace(direction, dT, this, &m_controls, fTurnSkill);

                    // make the drone always fly hard - throttle range 0.5 to 1.0, depending on how they are
                    // angled to their target
                    m_controls.jsValues[c_axisThrottle] = 0.5f + ((pi - da) / (2.0f * pi));

                    const float c_fMaxOffAngle = 0.10f;
                    float lifespan = m_mountedWeapons[0]->GetLifespan();

                    if ((!bDodge) && (t <= lifespan * 0.25f))
                    {
                        //We are close and not dodging anything so ...
                        //strafe
                        state = leftButtonIGC;
                        if (da < pi * 0.5f)
                        {
                            if (da > c_fMaxOffAngle)
                            {
                                //too close ... back off
                                state |= backwardButtonIGC;

                            }
                            else
                                state |= forwardButtonIGC;
                        }
                    }

                    SetStateBits(~selectedWeaponMaskIGC,
                                 ((da <= c_fMaxOffAngle) && (t <= lifespan * 0.9f))
                                 ? (state | allWeaponsIGC)
                                 : state);

                    return;
                }
                else if (m_commandIDs[c_cmdPlan] == c_cidDefend)
                {
                    //NYI
                }
            }

            if (m_gotoplan.Execute(timeStart, dT, true))
            {
                if (m_commandIDs[c_cmdPlan] == c_cidGoto)
                {
                    //if ((m_pilotType >= c_ptPlayer) && (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_buoy))
                    {
                        ImodelIGC*  pmodelPlan = m_commandTargets[c_cmdPlan];

                        for (Command i = 0; (i < c_cmdMax); i++)
                        {
                            if (m_commandTargets[i] == pmodelPlan)
                                SetCommand(i, NULL, c_cidNone);
                        }
                    }
                }
                else if (m_commandIDs[c_cmdPlan] == c_cidBuild)
                {
                    if (m_pilotType == c_ptBuilder)
                    {
                        assert (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_asteroid);
                        assert (((IasteroidIGC*)((ImodelIGC*)m_commandTargets[c_cmdPlan]))->HasCapability(m_abmOrders));

                        SetStateM(drillingMaskIGC);

                        this->m_controls.jsValues[c_axisYaw] = 0.0f;
                        this->m_controls.jsValues[c_axisPitch] = 0.0f;
                        this->m_controls.jsValues[c_axisRoll] = 1.0f;
                        this->m_controls.jsValues[c_axisThrottle] = 0.0f;   //50% throttle
                    }
                    else
                    {
                        assert (m_pilotType == c_ptLayer);
                        assert (m_commandTargets[c_cmdPlan]->GetObjectType() == OT_buoy);

                        GetMyMission()->GetIgcSite()->LayExpendable(timeStart, (IexpendableTypeIGC*)(IbaseIGC*)m_pbaseData, this);
                    }
                }
            }
        }
    }
}

void    CshipIGC::ExecuteShipMove(Time          timeStop)
{
    Vector      v = GetVelocity();
    Orientation o = GetOrientation();

    ExecuteShipMove(GetMyLastUpdate(), timeStop, &v, &o);

    SetVelocity(v);
    SetOrientation(o);
}

void    CshipIGC::ExecuteShipMove(Time          timeStart,
                                  Time          timeStop,
                                  Vector*       pVelocity,
                                  Orientation*  pOrientation)
{
    if (timeStop > timeStart)
    {
        //Adjust ship's heading, velocity, etc. based on its control settings.
        float   dT = timeStop - timeStart;
        assert (dT > 0.0f);

        float   thrust = m_myHullType.GetThrust();
        float   thrust2 = thrust * thrust;

        //Conversion factor ... Newtons to deltaV
        assert (GetMass() > 0.0f);
        float   thrustToVelocity = dT / GetMass();

		//No maneuvering if ripcording
        /*
        {
            float   tm = GetTorqueMultiplier() * thrustToVelocity;
            float   maxDelta = tm * m_myHullType.GetTurnTorque(c_axisRoll);

            if (maxDelta < m_turnRates[c_axisRoll])
                m_turnRates[c_axisRoll] -= maxDelta;
            else if (-maxDelta > m_turnRates[c_axisRoll])
                m_turnRates[c_axisRoll] += maxDelta;
            else
                m_turnRates[c_axisRoll] = 0.0f;
        }
        else
        */
        if (!m_pmodelRipcord)
        {
            //constrain the desired yaw/pitch/roll rates to an sphere rather than a box
				float   l = m_controls.jsValues[c_axisYaw]   * m_controls.jsValues[c_axisYaw] +
                        m_controls.jsValues[c_axisPitch] * m_controls.jsValues[c_axisPitch] +
                        m_controls.jsValues[c_axisRoll]  * m_controls.jsValues[c_axisRoll];

            if (l > 1.0f)
                l = 1.0f / sqrt(l);
            else
                l = 1.0f;

            float   tm = GetTorqueMultiplier() * thrustToVelocity;
            for (int i = 0; (i < 3); i++)
            {
                float   desiredRate = m_controls.jsValues[i] * l * m_myHullType.GetMaxTurnRate(i);
                float   maxDelta = tm * m_myHullType.GetTurnTorque(i);

                if (desiredRate < m_turnRates[i] - maxDelta)
                    m_turnRates[i] -= maxDelta;
                else if (desiredRate > m_turnRates[i] + maxDelta)
                    m_turnRates[i] += maxDelta;
                else
                    m_turnRates[i] = desiredRate;
            }
        }

        pOrientation->Yaw(   m_turnRates[c_axisYaw] * dT);
        pOrientation->Pitch(-m_turnRates[c_axisPitch] * dT);
        pOrientation->Roll(  m_turnRates[c_axisRoll] * dT);

        // Re-normalize the orientation matrix
        pOrientation->Renormalize();

        const Vector&   myBackward = pOrientation->GetBackward();

        float   speed = pVelocity->Length();
        float   maxSpeed = m_myHullType.GetMaxSpeed();

        assert (maxSpeed > 0.0f);

        //What would our velocity be if we simply let drag slow us down
        Vector  drag;
        {
            // double   f = exp(-thrust * thrustToVelocity / maxSpeed);
			double   f = exp(double(double(-thrust) * double(thrustToVelocity) / (double)maxSpeed));  // mmf type cast changes

            //New velocity = old velocity * f
            //drag = thrust required to create this change in velocity
            drag = *pVelocity * float((1.0 - f) / double(thrustToVelocity));
        }

        m_engineVector.x = m_engineVector.y = m_engineVector.z = 0.0f;    //Zero out the thrust

        bool    afterF = (m_stateM & afterburnerButtonIGC) != 0;
        float   thrustRatio = 0.0f;
        {
            IafterburnerIGC*    afterburner = (IafterburnerIGC*)(m_mountedOthers[ET_Afterburner]);

            if (afterburner)
            {
                float   abThrust = afterburner->GetMaxThrustWithGA(); //TheRock 15-8-2009
				if (afterF) {
                    thrustRatio = abThrust / thrust;
				}
                afterburner->IncrementalUpdate(timeStart, timeStop, false);

                float power = afterburner->GetPower();
                if (power != 0.0f)
                {
                    //Factor the afterburner thrust into drag (so that it will factor into the engine thrust)
                    drag += (power * abThrust) * myBackward;
                }
            }
        }

        //no maneuvering while ripcording
        if (!m_pmodelRipcord)
        {
            Vector  localThrust;
            if (m_stateM & (leftButtonIGC | rightButtonIGC |
                            upButtonIGC | downButtonIGC |
                            forwardButtonIGC | backwardButtonIGC))
            {
                //Under manual control: find out which direction to thrust in
                //get the throttle setting, but ramp between 0.2 and 1.0 (instead of 0.0 & 1.0)
                int   x = ((m_stateM & leftButtonIGC)     ? -1 : 0) + ((m_stateM & rightButtonIGC)   ?  1 : 0);
                int   y = ((m_stateM & downButtonIGC)     ? -1 : 0) + ((m_stateM & upButtonIGC)      ?  1 : 0);
                int   z = ((m_stateM & backwardButtonIGC) ?  1 : 0) + ((m_stateM & forwardButtonIGC) ? -1 : 0);

                if (x || y || z)
                {
                    //thrusting in at least one direction
                    localThrust.x = (thrust * (float)x);
                    localThrust.y = (thrust * (float)y);
                    localThrust.z = (thrust * (float)z);
                }
                else
                    localThrust = Vector::GetZero();
            }
            else
            {
                if ((m_stateM & coastButtonIGC) && !afterF)
                    localThrust = pOrientation->TimesInverse(drag);
                else
                {
                    float   negDesiredSpeed;
                    if (afterF)
                        negDesiredSpeed = maxSpeed * (-1.0f - thrustRatio);
                    else
                        negDesiredSpeed = (-0.5f * (1.0f + m_controls.jsValues[c_axisThrottle])) *
                                          ((speed > maxSpeed) ? speed : maxSpeed);

                    Vector  desiredVelocity = myBackward * negDesiredSpeed;

                    //Find out how much thrust is required to obtain our desired velocity,
                    //accounting for drag
					// mmf added zero check and debugf
					if (thrustToVelocity == 0.0f) debugf("shipIGC.cpp ~2394 thrustToVelocity = 0 about to devide by zero\n");
                    localThrust = pOrientation->TimesInverse((desiredVelocity - *pVelocity) / thrustToVelocity + drag);
                }
            }

            {
                //Clip the engine vector the the available thrust from the engine
                float   sm = m_myHullType.GetSideMultiplier();
				// mmf added zero checks and debugf
				if (sm == 0.0f) debugf("shipIGC.cpp ~2403 sm = 0 about to devide by zero\n");
				if ((m_myHullType.GetBackMultiplier()==0.0f)&&(localThrust.z<=0.0f))
					debugf("shipIGC.cpp ~2405 backmultip = 0 about to devide by zero\n");
				Vector  scaledThrust(localThrust.x / sm,
                                     localThrust.y / sm,
                                     localThrust.z <= 0.0f ? localThrust.z : (localThrust.z / m_myHullType.GetBackMultiplier()));

                float   r2 = scaledThrust.LengthSquared();

                if (r2 == 0.0f)
                    m_engineVector = Vector::GetZero();
                else if (r2 <= thrust2)
                {
                    //No clipping of engine thrust required
                    m_engineVector = localThrust * *pOrientation;
                }
                else
                {
                    //Trying to thrust too much ... clip it back.
					m_engineVector = (localThrust * *pOrientation) * (thrust / (float)sqrt(r2));
                }
            }
        }


        *pVelocity += thrustToVelocity * (m_engineVector - drag);
		// mmf added log msg for large velocity^2, mmf 10/07 increased threshold to 800^2 as some cores commonly have ships with speeds in the 500's
		if ((*pVelocity * *pVelocity) > 640000.0f) {
			debugf("mmf pVelocity^2 = %g ship = %s\n",(*pVelocity * *pVelocity),GetName());
		}

		// mmf other velocity checks were added for debugging, this one was definitely being tripped
		// replaced assert with log msg
		if (!(*pVelocity * *pVelocity >= 0.0f)) {
			debugf("mmf pVelocity^2 < 0.0 ship = %s\n",GetName());
			debugf("pVelocity x=%g y=%g z=%g\n",(*pVelocity).x,(*pVelocity).y,(*pVelocity).z);
			debugf("Igc shipIGC.cpp debug build would have called assert and exited, commented out for now\n");
			// cause an exception for debugging
			// (*(int*)0) = 0;
		}
        // assert (*pVelocity * *pVelocity >= 0.0f); // mmf commented out
    }
}

void    CshipIGC::ProcessFractions(const CompactShipFractions& fractions)
{
    SetFraction(fractions.GetHullFraction());
    if (m_mountedOthers[ET_Shield])
        ((IshieldIGC*)(m_mountedOthers[ET_Shield]))->SetFraction(fractions.GetShieldFraction());
    SetFuel(fractions.GetFuel(m_myHullType.GetMaxFuel()));
    SetAmmo(fractions.GetAmmo(m_myHullType.GetMaxAmmo()));
    SetEnergy(fractions.GetEnergy(m_myHullType.GetMaxEnergy()));
	SetOre(fractions.GetOre(GetOreCapacity()));  //Xynth #156 7/10
}

#define GetSC           {                                                                                                       \
                            int stateM;                                                                                         \
                            shipupdate.stateM.Export(&stateM);                                                                  \
                            SetStateM(stateM);                                                                      \
                        }                                                                                                       \
                        shipupdate.controls.Export(&m_controls);

#define GetOVTP         Vector      velocity;                                                                                   \
                        shipupdate.velocity.Export(&velocity);                                                                  \
                        Orientation orientation;                                                                                \
                        shipupdate.orientation.Export(&orientation);                                                            \
                        shipupdate.turnRates.Export(m_turnRates);                                                               \
                        if (m_mountedOthers[ET_Afterburner])                                                                    \
                            ((IafterburnerIGC*)(m_mountedOthers[ET_Afterburner]))->SetPower(shipupdate.power);

#define GetF            SetFraction(shipupdate.fractions.GetHullFraction());                                                    \
                        if (m_mountedOthers[ET_Shield])                                                                         \
                            ((IshieldIGC*)(m_mountedOthers[ET_Shield]))->SetFraction(shipupdate.fractions.GetShieldFraction()); \
                        SetFuel(shipupdate.fractions.GetFuel(m_myHullType.GetMaxFuel()));                                       \
                        SetAmmo(shipupdate.fractions.GetAmmo(m_myHullType.GetMaxAmmo()));                                       \
                        SetEnergy(shipupdate.fractions.GetEnergy(m_myHullType.GetMaxEnergy()));									\
						SetOre(shipupdate.fractions.GetOre(GetOreCapacity()));  //Xynth #156 7/10

static inline bool  LegalPosition(const Vector& position)
{
    float   l = position * position;
    return (l > 0.0f) &&
           (l < 90000.0f * 90000.0f);
}

ShipUpdateStatus    CshipIGC::ProcessShipUpdate(const ServerLightShipUpdate& shipupdate)
{
    GetSC;

    return c_susAccepted;
}

ShipUpdateStatus    CshipIGC::ProcessShipUpdate(Time    timeReference,
                                                Vector  positionReference,
                                                const   ServerHeavyShipUpdate& shipupdate)
{
    ShipUpdateStatus    rc;

    Time    timeUpdate;
    shipupdate.time.Export(timeReference, &timeUpdate);
    timeUpdate = GetMyMission()->GetIgcSite()->ClientTimeFromServerTime(timeUpdate);

    float   deltaT = GetMyLastUpdate() - timeUpdate;
    if ((deltaT < 1.5f) && (deltaT > -1.5f))
    {
        Vector  position;
        shipupdate.position.Export(positionReference, &position);
        if (LegalPosition(position))
        {
            rc = c_susAccepted;

            GetSC;

            GetOVTP;

            GetF;
            WarpShip(timeUpdate, deltaT, &position, &velocity, &orientation);
        }
        else
            rc = c_susInvalid;
    }
    else
        rc = c_susOutOfDate;

    return rc;
}

ShipUpdateStatus    CshipIGC::ProcessShipUpdate(const ClientShipUpdate& shipupdate)
{
    ShipUpdateStatus    rc;

    //The clinet converted the time to server time already

    float   deltaT = GetMyLastUpdate() - shipupdate.time;
    if ((deltaT < 1.5f) && (deltaT > -1.5f) && LegalPosition(shipupdate.position))
    {
        /*
        {
                debugf("(%8.3f %8.3f %8.3f) (%8.3f %8.3f %8.3f) %d\n",
                        GetPosition().x,
                        GetPosition().y,
                        GetPosition().z,
                        GetVelocity().x,
                        GetVelocity().y,
                        GetVelocity().z,
                        GetLastUpdate().clock());
        }
        */

        GetSC;
        GetOVTP;

        Vector      position = shipupdate.position;

        /*
        {
                debugf("(%8.3f %8.3f %8.3f) (%8.3f %8.3f %8.3f) %d\n",
                        position.x, position.y, position.z, velocity.x, velocity.y, velocity.z,
                        shipupdate.time.clock());
        }
        */

        if (LegalPosition(position))
        {
            CalculateShip(shipupdate.time, deltaT, &position, &velocity, &orientation);
            SetOrientation(orientation);    //Always accept orientation

            float   d = (GetPosition() - position).LengthSquared();
            if (d < 400.0f)
            {
                rc = c_susAccepted;

                SetPosition(position);
                SetVelocity(velocity);
            }
            else
            {
                debugf("Position error %10.2f %10.4f (%s)\n", sqrt(d), deltaT, GetName());
                rc = c_susRejected;
            }
        }
        else
            rc = c_susInvalid;

        /*
        {
                debugf("(%8.3f %8.3f %8.3f) (%8.3f %8.3f %8.3f) %f\n\n",
                        GetPosition().x,
                        GetPosition().y,
                        GetPosition().z,
                        GetVelocity().x,
                        GetVelocity().y,
                        GetVelocity().z,
                        deltaT);
        }
        */
    }
    else
        rc = c_susOutOfDate;

    return rc;
}

ShipUpdateStatus    CshipIGC::ProcessShipUpdate(const ServerSingleShipUpdate& shipupdate, bool bOrient)
{
    GetSC;
    GetOVTP;

    GetF;

    Vector      position = shipupdate.position;
    assert (LegalPosition(shipupdate.position));

    float   deltaT = GetMyLastUpdate() - shipupdate.time;
    if ((deltaT < 1.5f) && (deltaT > -1.5f))
    {
        CalculateShip(shipupdate.time, deltaT, &position, &velocity, &orientation);
    }

    SetPosition(position);
    SetVelocity(velocity);
    if (bOrient)
        SetOrientation(orientation);

    return c_susAccepted;
}

#undef GetSC
#undef GetOVTP
#undef GetF

void    CshipIGC::CalculateShip(Time timeUpdate, float    deltaT, Vector* pPosition, Vector* pVelocity, Orientation* pOrientation)
{
    //assert (pPosition->LengthSquared() != 0.0f);

    //Calculate what the ship's position, orientation & velocity would be at the time of the ship's last update.
    if (deltaT <= 0.0f)
    {
        //
        // For whatever reason ... the update applies to some
        // time after now, so move the ship backwards
        // to where it should be at the last update time (assume
        // constant velocity, etc. since backing out a the acutal
        // velocity will be hard).
        //
        *pPosition += *pVelocity * deltaT;

        //Adjust facing as well
        pOrientation->Roll( -m_turnRates[c_axisRoll] * deltaT);
        pOrientation->Pitch( m_turnRates[c_axisPitch] * deltaT);
        pOrientation->Yaw(  -m_turnRates[c_axisYaw] * deltaT);

        // Re-normalize the orientation matrix
        pOrientation->Renormalize();
    }
    else
    {
        //
        // Update the ship's snap shot, never advancing the
        // clock more than 1/4 second. (basically use the same algorithm as coreigc::update()).
        int n = 1 + (int)(deltaT / c_fTimeIncrement);

        Time    thisTime = timeUpdate;
        for (int i = 1; (i <= n); i++)
        {
            Time nextTime = timeUpdate + (deltaT * (((float)i) / ((float)n)));

            ExecuteShipMove(thisTime, nextTime,
                            pVelocity,
                            pOrientation);
            *pPosition += *pVelocity * (nextTime - thisTime);

            thisTime = nextTime;
        }
    }
}

void    CshipIGC::WarpShip(Time timeUpdate, float    deltaT, Vector* pPosition, Vector* pVelocity, Orientation* pOrientation)
{
    CalculateShip(timeUpdate, deltaT, pPosition, pVelocity, pOrientation);

    SetPosition(*pPosition);
    SetVelocity(*pVelocity);
    SetOrientation(*pOrientation);
}

ShipUpdateStatus    CshipIGC::ProcessShipUpdate(Time                timeReference,
                                    const ServerActiveTurretUpdate& shipupdate)
{
    ShipUpdateStatus    rc;

    Time    timeUpdate;
    shipupdate.time.Export(timeReference, &timeUpdate);
    timeUpdate = GetMyMission()->GetIgcSite()->ClientTimeFromServerTime(timeUpdate);

    float   deltaT = GetMyLastUpdate() - timeUpdate;
    if ((deltaT < 1.5f) && (deltaT > -1.5f))
    {
        rc = c_susAccepted;

        //If we got this ... we're shooting.
        SetStateM(weaponsMaskIGC);

        shipupdate.controls.Export(&m_controls);

        Orientation orientation;
        shipupdate.orientation.Export(&orientation);

        //Calculate what the ship's position, orientation & velocity would be at the time of the ship's last update.
        {
            //
            // Update the ship's snap shot, never advancing the
            // clock more than 1/4 second. (basically use the same algorithm as coreigc::update()).
            int n = 1 + (int)(float(fabs(deltaT)) / c_fTimeIncrement);

            Time    thisTime = timeUpdate;
            for (int i = 1; (i <= n); i++)
            {
                Time nextTime = timeUpdate + (deltaT * (((float)i) / ((float)n)));

                ExecuteTurretMove(thisTime,
                                  nextTime,
                                  &(orientation));

                thisTime = nextTime;
            }
        }

        SetOrientation(orientation);
    }
    else
        rc = c_susOutOfDate;

    return rc;
}

ShipUpdateStatus    CshipIGC::ProcessShipUpdate(const ClientActiveTurretUpdate& shipupdate)
{
    ShipUpdateStatus    rc;

    float   deltaT = GetMyLastUpdate() - shipupdate.time;
    if ((deltaT < 1.5f) && (deltaT > -1.5f))
    {
        rc = c_susAccepted;

        //If we got this ... we're shooting.
        SetStateM(weaponsMaskIGC);

        shipupdate.controls.Export(&m_controls);

        Orientation orientation;
        shipupdate.orientation.Export(&orientation);

        //Calculate what the ship's position, orientation & velocity would be at the time of the ship's last update.
        {
            //
            // Update the ship's snap shot, never advancing the
            // clock more than 1/4 second. (basically use the same algorithm as coreigc::update()).
            int n = 1 + (int)(float(fabs(deltaT)) / c_fTimeIncrement);

            Time    thisTime = shipupdate.time;
            for (int i = 1; (i <= n); i++)
            {
                Time nextTime = shipupdate.time + (deltaT * (((float)i) / ((float)n)));

                ExecuteTurretMove(thisTime,
                                  nextTime,
                                  &(orientation));

                thisTime = nextTime;
            }
        }

        SetOrientation(orientation);
    }
    else
        rc = c_susOutOfDate;

    return rc;
}

void CshipIGC::SetSide(IsideIGC* pside)  //override the default SetSide method
{
    IsideIGC*   psideOld = GetSide();
    if (psideOld != pside)
    {
        if (psideOld)
        {
            //Was anyone on my old side donating to me ... if so ... stop them from donating
            for (ShipLinkIGC*   psl = psideOld->GetShips()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                if (psl->data()->GetAutoDonate() == this)
                    psl->data()->SetAutoDonate(NULL);
            }
        }
        m_pshipAutoDonate = NULL;

        //You can only change sides from inside the neutral zone
        SetCluster(NULL);
        SetParentShip(NULL);

        //Blow away the children
        ShipLinkIGC*    psl;
        while (psl = m_shipsChildren.first())   //intentional assignment
        {
            psl->data()->SetParentShip(NULL);
        }

        if (m_station)
        {
            //or a station ... so undock without generating a
            //change station event
            m_station->DeleteShip(this);
            m_station->Release();
            m_station = NULL;
        }
        assert (m_station == NULL);

        if (psideOld)
            psideOld->DeleteShip(this);

        TmodelIGC<IshipIGC>::SetSide(pside);

        if (pside)
            pside->AddShip(this);
    }
}

void CshipIGC::SetMission(ImissionIGC* pMission)
{
    ImissionIGC*    pmisionOld = GetMyMission();
    if (pMission != pmisionOld)
    {
        AddRef();

        SetSide(NULL);

        //Blow away all of the ships parts ... essentially re-initialize it with no parts and no hull
        {
            PartLinkIGC* l;
            while (l = m_parts.first()) //not ==
                l->data()->Terminate();
        }

        //Hack the fuel and ammo to avoid problems with the debug asserts
        m_ammo = 0;
        m_fuel = 0.0f;

        {
            for (Command i = 0; (i < c_cmdMax); i++)
            {
                if (m_commandTargets[i])
                {
                    m_commandTargets[i]->Release();
                    m_commandTargets[i] = NULL;
                }
                m_commandIDs[i] = c_cidNone;
            }
        }

        m_myHullType.SetHullType(NULL);

        //Force the mass to zero since that is what it should be
        SetMass(0.0f);

        FreeThingSite();

        {
            if (pmisionOld)
            {
                pmisionOld->DeleteShip(this);
                SetMyMission(NULL);
            }
        }

        assert (!GetMyMission());
        assert (GetStation() == NULL);
        assert (GetCluster() == NULL);
        assert (GetSide() == NULL);

        assert (m_myHullType.GetHullType() == NULL);

        if (pMission)
        {
            SetMyMission(pMission);
            pMission->AddShip(this);
        }

        m_stateM = 0;

        Release();
    }
}

void    CshipIGC::Promote(void)
{
    assert (m_pshipParent);

    IshipIGC*   pshipParent = m_pshipParent;
    Mount       turretid    = m_turretID;

    //Leave the parent
    SetParentShip(NULL);

    //Give the same ship the parent had
    SetBaseHullType(pshipParent->GetBaseHullType());

    //Copy over all of the inventory from the parent
    {
        for (PartLinkIGC*   ppl = pshipParent->GetParts()->first(); (ppl != NULL); ppl = ppl->next())
        {
            IpartIGC*   ppart = ppl->data();
            IpartIGC*   ppartNew = CreateAndAddPart(ppart->GetPartType(), ppart->GetMountID(), ppart->GetAmount());

            ppartNew->SetMountedFraction(ppart->GetMountedFraction());

            if (IlauncherIGC::IsLauncher(ppart->GetObjectType()))
            {
                ((IlauncherIGC*)ppartNew)->SetTimeFired(GetMyLastUpdate());
            }
        }

        SetAmmo(pshipParent->GetAmmo());
        SetFuel(pshipParent->GetFuel());
        SetEnergy(pshipParent->GetEnergy());
    }

    //Transfer all of the gunners from the old hull to the new hull
    {
        const ShipListIGC*  pships = pshipParent->GetChildShips();
        ShipLinkIGC*        psl;
        while (psl = pships->first())       //Intentional
        {
            IshipIGC*   pship = psl->data();

            Mount   turretID = pship->GetTurretID();
            pship->SetParentShip(this);
            pship->SetTurretID(turretID);
        }
    }

	// mdvalley: If parent in base, put child in base.
//	IstationIGC * currentBase = pshipParent->GetStation();
//	if(currentBase)
//		SetStation(currentBase);
//	else
//	{
    const Vector& position = pshipParent->GetPosition();
    SetPosition(position);
    SetVelocity(pshipParent->GetVelocity());
    SetOrientation(pshipParent->GetOrientation());
//	}

    SetFraction(pshipParent->GetFraction());
    {
        IshieldIGC* pshield = (IshieldIGC*)(pshipParent->GetMountedPart(ET_Shield, 0));
        if (pshield)
        {
            assert (m_mountedOthers[ET_Shield]);
            ((IshieldIGC*)(m_mountedOthers[ET_Shield]))->SetFraction(pshield->GetFraction());
        }
    }
    //NYI damage tracks?

    pshipParent->SetParentShip(this);
    pshipParent->SetTurretID(turretid);

    //Swap link in the cluster's model list
    IclusterIGC*    pcluster = GetCluster();
    if (pcluster)
    {
        const ModelListIGC* pmodels = pcluster->GetModels();
        ModelLinkIGC*       pmlParent = pmodels->find(pshipParent);
        ModelLinkIGC*       pmlThis   = pmodels->find(this);

        pmlParent->data(this);
        pmlThis->data(pshipParent);
    }
}

void    CshipIGC::SetParentShip(IshipIGC*   pshipParent)
{
    if (m_pshipParent != pshipParent)
    {
        debugf("Parent of %s changing to ", GetName());
        IshipIGC*   pshipOldParent = m_pshipParent;
        if (pshipParent)
        {
            debugf("%s\n", pshipParent->GetName());

            {
                ShipLinkIGC*    psl;
                while (psl = m_shipsChildren.first())       //Intentional
                {
                    debugf("%s <- %s \n", this->GetName(), psl->data()->GetName());
                    psl->data()->SetParentShip(NULL);
                }
            }
            assert (m_shipsChildren.n() == 0);
            assert (pshipParent->GetParentShip() == NULL);

            if (pshipOldParent == NULL)
            {
                //No pre-existing parent
                {
                    //No parts allowed on an observer or turret
                    PartLinkIGC* l;
                    while (l = m_parts.first()) //not ==
                        l->data()->Terminate();
                }
                m_fuel = 0.0f;
                m_ammo = 0;
                m_energy = 0.0f;

                m_myHullType.SetHullType(NULL);
                SetMass(0.0f);

                HRESULT hr = Load(0, NULL, NULL, NULL, c_mtNotPickable);
                ZSucceeded(hr);

                SetVisibleF(false);

                IclusterIGC*    pcluster = GetCluster();
                if (pcluster)
                {
                    //If we were in a cluster ... remove us as a scanner
                    pcluster->GetClusterSite()->DeleteScanner(GetSide()->GetObjectID(), this);
                }

                assert (m_turretID == NA);
            }
            else
            {
                if (m_turretID != NA)
                {
                    IweaponIGC* pw = (IweaponIGC*)(pshipOldParent->GetMountedPart(ET_Weapon, m_turretID));
                    if (pw)
                        pw->SetGunner(NULL);

                    m_turretID = NA;
                }

                pshipOldParent->DeleteChildShip(this);
                pshipOldParent->Release();
            }

            pshipParent->AddRef();
            m_pshipParent = pshipParent;

            pshipParent->AddChildShip(this);

            if (pshipOldParent)
                GetMyMission()->GetIgcSite()->LoadoutChangeEvent(pshipOldParent, NULL, c_lcRemovePassenger);
            GetMyMission()->GetIgcSite()->LoadoutChangeEvent(pshipParent, NULL, c_lcAddPassenger);
            GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, NULL, c_lcTurretChange);
        }
        else
        {
            debugf("NULL\n");

            assert (pshipOldParent != NULL);

            if (m_turretID != NA)
            {
                IweaponIGC* pw = (IweaponIGC*)(pshipOldParent->GetMountedPart(ET_Weapon, m_turretID));
                if (pw)
                    pw->SetGunner(NULL);

                m_turretID = NA;
            }

            pshipOldParent->DeleteChildShip(this);

            pshipOldParent->Release();
            m_pshipParent = NULL;

            //Put us in an eject pod
            //The caller of SetParentShip() must give us a hull
            //especially since we are going to be counted as a scanner.

            IclusterIGC*    pcluster = GetCluster();
            if (pcluster)
            {
                //Became an independant ship while inside a cluster ...
                //(ain't ejection fun) ... we can now see things.
                pcluster->GetClusterSite()->AddScanner(GetSide()->GetObjectID(), this);
            }

            GetMyMission()->GetIgcSite()->LoadoutChangeEvent(pshipOldParent, NULL, c_lcRemovePassenger);
            GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, NULL, c_lcDisembark);
        }
    }
}

struct  ShipPair
{
    IshipIGC*       pship;
    IclusterIGC*    pcluster;
};

typedef Slist_utl<ShipPair> ShipPairList;
typedef Slink_utl<ShipPair> ShipPairLink;

ImodelIGC*    CshipIGC::FindRipcordModel(IclusterIGC*   pcluster)
{
    IsideIGC*       pside = GetSide();

    IIgcSite*   pigc = GetMyMission()->GetIgcSite();

    ShipPairList pairs;
    if (m_pilotType >= c_ptPlayer)
    {
        HullAbilityBitMask  habm = m_myHullType.HasCapability(c_habmCanLtRipcord)
                                   ? (c_habmIsRipcordTarget | c_habmIsLtRipcordTarget)
                                   : c_habmIsRipcordTarget;

		//ALLY ripcord is on 7/8/09 imago
		if (GetMyMission()->GetMissionParams()->bAllowAlliedRip) {
       		 //Make a list of undocked ships (on our side and allied sides) that are ripcord targets as well
	        for (ShipLinkIGC*  psl = GetMyMission()->GetShips()->first(); (psl != NULL); psl = psl->next()) //ALLY RIPCORD 7/8/09 imago
	        {
	            IshipIGC*       pship = psl->data();
	            if (pship != GetSourceShip() &&
					(pside->AlliedSides(pside,pship->GetSide()) || (pside == pship->GetSide()))) //&&
					//pship->SeenBySide(pside)) //Imago - if VISIBILITY ever becomes an option it needs to work with RIPCORD 7/10/09 ALLYTD
	            {
	                IclusterIGC*    pc = pigc->GetRipcordCluster(pship, habm);
	                if (pc)
	                {
	                    ShipPairLink*   spl = new ShipPairLink;
	                    spl->data().pship = pship;
	                    spl->data().pcluster = pc;


	                    pairs.last(spl);
	                }
	            }
			}
		} else {
       		 //Make a list of undocked ships (on only our side) that are ripcord targets as well
	        for (ShipLinkIGC*  psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
	        {
	            IshipIGC*       pship = psl->data();
	            if (pship != GetSourceShip())
	            {
	                IclusterIGC*    pc = pigc->GetRipcordCluster(pship, habm);
	                if (pc)
	                {
	                    ShipPairLink*   spl = new ShipPairLink;
	                    spl->data().pship = pship;
	                    spl->data().pcluster = pc;


	                    pairs.last(spl);
	                }
	            }
			}
		}
    }

	ImodelIGC*      pmodelGoal = NULL;
    const Vector*   positionGoal = NULL;
    {
        pmodelGoal = m_commandTargets[c_cmdCurrent];
        if (!pmodelGoal)
            pmodelGoal = m_commandTargets[c_cmdAccepted];

        if (pmodelGoal)
        {
            if (pmodelGoal->SeenBySide(pside) &&
                (pigc->GetCluster(this, pmodelGoal) == pcluster))
            {
                positionGoal = &(pmodelGoal->GetPosition());
            }
        }
    }

    //Search adjacent clusters for an appropriate target
    WarpListIGC     warps;
    ClusterListIGC  clustersVisited;

    float   ripcordSpeed = m_myHullType.GetRipcordSpeed();

    while (true)
    {
        assert (pcluster);

	ImodelIGC*  pmodelRipcord = NULL;
	if (pmodelGoal) //TheRock 13-12-2009 Allow ripcording to a probe or ship when a teleport is in the same sector.
	{
		if (pmodelGoal->GetObjectType() == OT_probe)
		{
			if (pmodelGoal->GetSide()==pside || (pside->AlliedSides(pside,pmodelGoal->GetSide()) && GetMission()->GetMissionParams()->bAllowAlliedRip))
			{
				IprobeIGC* pProbeSelected = (IprobeIGC*)pmodelGoal;
				if (pProbeSelected->GetCanRipcord(ripcordSpeed))
				{
					pmodelRipcord = pProbeSelected;
				}
			}
		}
		else if (pmodelGoal->GetObjectType() == OT_ship)
		{
			if (pmodelGoal->GetSide()==pside || (pside->AlliedSides(pside,pmodelGoal->GetSide()) && GetMission()->GetMissionParams()->bAllowAlliedRip))
			{
				IshipIGC* pShipSelected = (IshipIGC*)pmodelGoal;
				IhullTypeIGC*   pht = pShipSelected->GetBaseHullType();
				if (pht) { //TheRock 9-1-2010 fix rev512 (TheRock 13-12-2009)
					if (GetBaseHullType()->HasCapability(c_habmCanLtRipcord)) {
						if (pht->HasCapability((c_habmIsRipcordTarget | c_habmIsLtRipcordTarget))) {
							pmodelRipcord = pShipSelected;
						} else if (pht->HasCapability(c_habmIsRipcordTarget)) {
							pmodelRipcord = pShipSelected;
						}
					}
				}
			}
		}
	}

	if (pmodelRipcord == NULL) {
        	pmodelRipcord = FindTarget(this, positionGoal ? (c_ttFriendly | c_ttStation | c_ttNearest) : (c_ttFriendly | c_ttStation),
                                               NULL, pcluster, positionGoal, NULL,
                                               c_sabmRipcord);
	}

        if ((pmodelRipcord == NULL) && (m_pilotType >= c_ptPlayer))
        {
            float   d2Goal = FLT_MAX;

            if (GetMission()->GetMissionParams()->bAllowAlliedRip) {
                //No station in the cluster to ripcord to ... try allied  and our probes
                //Search backwords so that we'll get the most recently dropped probe
                //if multiple probes without a target
                for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->last(); (ppl != NULL); ppl = ppl->txen())
                {
                    IprobeIGC*  pprobe = ppl->data();
                    if ((pprobe->GetSide() == pside || pside->AlliedSides(pside,pprobe->GetSide())) && pprobe->GetCanRipcord(ripcordSpeed)) //ALLY RIPCORD imago 7/8/09
                    {
                        if (positionGoal)
                        {
                            float   d2 = (pprobe->GetPosition() - *positionGoal).LengthSquared();
                            if (d2 < d2Goal)
                            {
                                pmodelRipcord = pprobe;
                                d2Goal = d2;
                            }
                        }
                        else
                        {
                            pmodelRipcord = pprobe;
                            break;
                        }
                    }
                }
			} else {
                //No station in the cluster to ripcord to ... try probes
                //Search backwords so that we'll get the most recently dropped probe
                //if multiple probes without a target
                for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->last(); (ppl != NULL); ppl = ppl->txen())
                {
                    IprobeIGC*  pprobe = ppl->data();
                    if ((pprobe->GetSide() == pside) && pprobe->GetCanRipcord(ripcordSpeed))
                    {
                        if (positionGoal)
                        {
                            float   d2 = (pprobe->GetPosition() - *positionGoal).LengthSquared();
                            if (d2 < d2Goal)
                            {
                                pmodelRipcord = pprobe;
                                d2Goal = d2;
                            }
                        }
                        else
                        {
                            pmodelRipcord = pprobe;
                            break;
                        }
                    }
                }

            }

            if (pmodelRipcord == NULL)
            {
                float   debtMin = FLT_MAX;

                //No station or probe in the cluster to ripcord to ... try ships
                for (ShipPairLink*   psl = pairs.first(); (psl != NULL); psl = psl->next())
                {
                    if (psl->data().pcluster == pcluster)
                    {
                        float   debt = psl->data().pship->GetRipcordDebt();

                        if (positionGoal == NULL)
                        {
                            if (debt < debtMin)
                            {
                                debtMin = debt;
                                pmodelRipcord = psl->data().pship;
                            }
                        }
                        else
                        {
                            float   d2 = (psl->data().pship->GetPosition() - *positionGoal).LengthSquared();
                            if ((debt < debtMin) ||
                                ((debt == debtMin) && (d2 < d2Goal)))
                            {
                                debtMin = debt;
                                d2Goal = d2;
                                pmodelRipcord = psl->data().pship;
                            }
                        }
                    }
                }
            }
        }

        if (pmodelRipcord)
            return pmodelRipcord;

        clustersVisited.first(pcluster);

        //Push the destinations of the warps in pcluster onto the end the list of
        //warps that are an extra jump away
        {
            for (WarpLinkIGC*   l = pcluster->GetWarps()->first(); (l != NULL); l = l->next())
            {
                IwarpIGC*   w = l->data();
                if (CanSee(w))
                {
                    IwarpIGC*   pwarpDestination = w->GetDestination();
                    if (pwarpDestination)
                    {
                        IclusterIGC*    pclusterOther = pwarpDestination->GetCluster();

                        //Have we visited pclusterOther?
                        if (clustersVisited.find(pclusterOther) == NULL)
                        {
                            //No
                            warps.last(pwarpDestination);
                        }
                    }
                }
            }
        }

        WarpLinkIGC*    plink = warps.first();
        if (plink == NULL)
            return NULL;

        IwarpIGC*       pwarp = plink->data();

        delete plink;

        pcluster = pwarp->GetCluster();
        positionGoal = &(pwarp->GetPosition());
    }
}
void    CshipIGC::SetAutopilot(bool bAutopilot)
{
    m_bAutopilot = bAutopilot;

    if (bAutopilot)
        SetCommand(c_cmdPlan, m_commandTargets[c_cmdCurrent], m_commandIDs[c_cmdCurrent]);
    else
        SetCommand(c_cmdPlan, NULL, c_cidNone);
}

void    CshipIGC::ResetWaypoint(void)
{
    m_pclusterRequestRipcord = NULL;

    if (GetCluster() != NULL)
    {
        if (m_commandTargets[c_cmdPlan])
        {
            Waypoint::Objective o;
            switch (m_commandTargets[c_cmdPlan]->GetObjectType())
            {
                case OT_ship:
                {
                    o = (m_commandIDs[c_cmdPlan] == c_cidPickup)
                        ? Waypoint::c_oEnter
                        : Waypoint::c_oGoto;
                }
                break;

                case OT_asteroid:
                {
                    o = (m_commandIDs[c_cmdPlan] == c_cidBuild) && (m_pilotType == c_ptBuilder)
                        ? Waypoint::c_oEnter
                        : Waypoint::c_oGoto;
                }
                break;
                case OT_station:
                {
                    o = Waypoint::c_oGoto;
                    if (m_myHullType.GetHullType())
                    {
                        if ((m_commandIDs[c_cmdPlan] == c_cidGoto) || (m_commandIDs[c_cmdPlan] == c_cidNone))
                        {
							if ((m_commandTargets[c_cmdPlan]->GetSide() == GetSide()) || IsideIGC::AlliedSides(m_commandTargets[c_cmdPlan]->GetSide(), GetSide())) //#ALLY (TheRock) we can still dock here (Imago) 7/8/09
                            {
                                const IstationTypeIGC*  pst = ((IstationIGC*)m_commandTargets[c_cmdPlan])->GetStationType();
                                HullAbilityBitMask  habm = m_myHullType.GetCapabilities();

                                if ((habm & c_habmFighter) == 0)
                                {
                                    if (pst->HasCapability(c_sabmCapLand))
                                        o = Waypoint::c_oEnter;
                                }
                                else if (habm & c_habmLifepod)
                                {
                                    if (pst->HasCapability(c_sabmLand | c_sabmRescue))
                                        o = Waypoint::c_oEnter;
									//Imago 7/13/09 part of the fix for not allowing pod rescue at allied ahipyard.
									if (m_commandTargets[c_cmdPlan]->GetSide() != GetSide() && pst->HasCapability(c_sabmCapLand))
										o = Waypoint::c_oGoto;

                                }
                                else if (pst->HasCapability(c_sabmLand))
                                    o = Waypoint::c_oEnter;

                            }
                        }
                    }
                }
                break;
                case OT_warp:
                {
                    if ((m_commandIDs[c_cmdPlan] == c_cidBuild) && (m_pilotType == c_ptLayer))
                    {
                        DataBuoyIGC    db;

                        const Vector&       p = m_commandTargets[c_cmdPlan]->GetPosition();
                        const Orientation&  orientation = m_commandTargets[c_cmdPlan]->GetOrientation();

                        IexpendableTypeIGC* pet = (IexpendableTypeIGC*)(IbaseIGC*)m_pbaseData;
                        assert (pet);
                        if (pet->GetObjectType() == OT_mineType)
                        {
                            DataMineTypeIGC*   pdmt = (DataMineTypeIGC*)(pet->GetData());

                            db.position = p -
                                          orientation.GetBackward() *
                                          pdmt->radius;
                        }
                        else
                        {
                            assert (pet->GetObjectType() == OT_probeType);
                            IprobeTypeIGC*  ppt = (IprobeTypeIGC*)pet;

                            float   rMajor;
                            IprojectileTypeIGC* pprojectile = ppt->GetProjectileType();
                            if (pprojectile) // KG -bug fix, was: ppt
                                rMajor = pprojectile->GetLifespan() * pprojectile->GetSpeed() * 0.5f;
                            else
                                rMajor = ppt->GetScannerRange() * 0.4f;

                            db.position = p -
                                          (orientation.GetBackward() *
                                           rMajor) +
                                           Vector::RandomPosition(rMajor / 3.0f);
                        }

                        db.clusterID = m_commandTargets[c_cmdPlan]->GetCluster()->GetObjectID();
                        db.type = c_buoyWaypoint;

                        IbuoyIGC*   b = (IbuoyIGC*)(GetMission()->CreateObject(GetMyLastUpdate(), OT_buoy, &db, sizeof(db)));
                        assert (b);

                        //Something of a hack here: change the plan to build at the newly created buoy
                        //without changing the accepted order
                        SetCommand(c_cmdPlan, b, c_cidBuild);

                        b->Release();
                        return;     //Return since the recursive call will handle setting the waypoint appropriately
                    }
                    else
                        o = ((m_commandIDs[c_cmdPlan] == c_cidGoto) || (m_commandIDs[c_cmdPlan] == c_cidPickup) || (m_commandIDs[c_cmdPlan] == c_cidNone))
                            ? Waypoint::c_oEnter
                            : Waypoint::c_oGoto;
                }
                break;
                case OT_treasure:
                {
                    o = ((m_commandIDs[c_cmdPlan] == c_cidGoto) || (m_commandIDs[c_cmdPlan] == c_cidPickup) || (m_commandIDs[c_cmdPlan] == c_cidNone))
                        ? Waypoint::c_oEnter
                        : Waypoint::c_oGoto;
                }
                break;

                case OT_probe:
                {
                    o = Waypoint::c_oGoto;
                    if (((m_commandIDs[c_cmdPlan] == c_cidGoto) || (m_commandIDs[c_cmdPlan] == c_cidNone)) &&
                        m_myHullType.GetHullType() && m_myHullType.HasCapability(c_habmLifepod))
                    {
                        IprobeTypeIGC*  ppt = ((IprobeIGC*)(m_commandTargets[c_cmdPlan]))->GetProbeType();
                        ExpendableAbilityBitMask    eabm = ppt->GetCapabilities();

                        if ( (eabm & c_eabmRescueAny) ||
                            ((eabm & c_eabmRescue) &&
							((m_commandTargets[c_cmdPlan]->GetSide() == GetSide()) || IsideIGC::AlliedSides(m_commandTargets[c_cmdPlan]->GetSide(),GetSide())))) //#ALLY - imago 7/3/09
                        {
                            o = Waypoint::c_oEnter;
                        }
                    }
                }
                break;

                default:
                    o = Waypoint::c_oGoto;
            }

            m_gotoplan.Set(o, m_commandTargets[c_cmdPlan]);

            if (m_bAutopilot)
            {
                IIgcSite*       pigc = GetMyMission()->GetIgcSite();
                IclusterIGC*    pclusterTarget = pigc->GetCluster(this, m_commandTargets[c_cmdPlan]);

                if (pclusterTarget && bShouldUseRipcord(pclusterTarget))
                {
                    if (IsSafeToRipcord())
					{
                        pigc->RequestRipcord(this, pclusterTarget);
						//Xynth #47 7/2010
						SetStateBits(droneRipMaskIGC, droneRipMaskIGC);
						SetStateBits(miningMaskIGC, 0); //stop mining when you rip
					}
                    else
                    {
                        m_pclusterRequestRipcord = pclusterTarget;
                        m_timeRequestRipcord = GetMyLastUpdate() + 5.0f;
                    }
                }
                else if (m_pmodelRipcord)
				{
                    pigc->RequestRipcord(this, NULL);
					//Xynth #182 7/2010					
					SetStateBits(droneRipMaskIGC, 0);
				}
            }
        }
        else
        {
            m_gotoplan.Reset();

            if (m_pmodelRipcord)
			{
                GetMyMission()->GetIgcSite()->RequestRipcord(this, NULL);
				//Xynth #182 7/2010
				SetStateBits(droneRipMaskIGC, 0);
			}
        }
    }
    else
    {
        if (m_commandTargets[c_cmdPlan] == NULL)
            m_gotoplan.Reset();
    }
}

bool    CshipIGC::IsSafeToRipcord(void)
{
    float   speed2 = GetVelocity().LengthSquared();
    if (speed2 < 1.0f)
        return true;

    //How far are we going to drift over the ripcord time
    float   tRipcord = m_myHullType.GetRipcordSpeed();
    float   v0 = float(sqrt(speed2));

    float   vMax = m_myHullType.GetMaxSpeed();
    float   thrust = m_myHullType.GetThrust();
    float   mass = GetMass();

    float   k = thrust / (vMax * mass);

    //dV/dT for a ripcording ship is -k V(t)
    //
    // V(t) = v0 exp(-kt)
    // S(t) = -v0 (exp(-kt) - 1) / k

    float   s = v0 * float(1.0 - exp(-k * tRipcord)) / k;

    //"effective" time till we ripcord is s / current velocity

    int state = 0;
    Dodge(this, NULL, &state, false, 1.0f + s / v0);   //Add a bit of a fudge

    return (state == 0);
}

bool    CshipIGC::bShouldUseRipcord(IclusterIGC*  pcluster)
{
    if ((m_myHullType.GetHullType() == NULL) || m_myHullType.HasCapability(c_habmNoRipcord))
        return false;

    IclusterIGC*    pclusterShip = GetCluster();
    if (pcluster == pclusterShip)
        return NULL;

    int dShip = GetDistance(this, pcluster, pclusterShip);

    IsideIGC*       pside = GetSide();

    ClusterListIGC  shipRipcords;
    if (m_pilotType >= c_ptPlayer)
    {
        IIgcSite*   pigc = GetMyMission()->GetIgcSite();

        HullAbilityBitMask  habm = m_myHullType.HasCapability(c_habmCanLtRipcord)
                                   ? (c_habmIsRipcordTarget | c_habmIsLtRipcordTarget)
                                   : c_habmIsRipcordTarget;

		if (GetMission()->GetMissionParams()->bAllowAlliedRip) {
	        //Make a list of undocked ships on our team and allied teams that are ripcord targets as well
	        for (ShipLinkIGC*  psl = GetMission()->GetShips()->first(); (psl != NULL); psl = psl->next()) //ALLY ripcord 7/8/09 was pside
	        {
	            IshipIGC*       ps = psl->data();
	            if (ps != this &&
					(ps->GetSide() == pside || pside->AlliedSides(pside,ps->GetSide()))) //ALLY
	            {
					//if (ps->SeenBySide(pside)) {  //Imago VISIBILITY RIPCORD 7/10/09
	                	IclusterIGC*    pc = pigc->GetRipcordCluster(ps, habm);
	                	if (pc)
	                	{
	                    	if (pcluster == pc)
	                        	return true;

	                    	if (shipRipcords.find(pc) == NULL)
	                        	shipRipcords.last(pc);
						}
	              //  }
	            }
	        }
		} else {
	        //Make a list of undocked ships that are ripcord targets as well
	        for (ShipLinkIGC*  psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
	        {
	            IshipIGC*       ps = psl->data();
	            if (ps != this)
	            {
	                IclusterIGC*    pc = pigc->GetRipcordCluster(ps, habm);
	                if (pc)
	                {
	                    if (pcluster == pc)
	                        return true;

	                    if (shipRipcords.find(pc) == NULL)
	                        shipRipcords.last(pc);
	                }
	            }
	        }
		}
    }

    //Search adjacent clusters for an appropriate target
    WarpListIGC     warpsOne;
    WarpListIGC     warpsTwo;
    ClusterListIGC  clustersVisited;

    WarpListIGC* pwlOneAway = &warpsOne;
    WarpListIGC* pwlTwoAway = &warpsTwo;

    float   ripcordSpeed = m_myHullType.GetRipcordSpeed();

    while (true)
    {
        assert (pcluster);
        ImodelIGC*  pmodelRipcord = FindTarget(this, c_ttFriendly | c_ttStation,
                                               NULL, pcluster, NULL, NULL,
                                               c_sabmRipcord);

        if (pmodelRipcord)
            return true;

        if (m_pilotType >= c_ptPlayer)
        {
            //No station in the cluster to ripcord to ... try ships
            if (shipRipcords.find(pcluster))
                return true;

            //No station in the cluster to ripcord to ... try probes
            for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->first(); (ppl != NULL); ppl = ppl->next())
            {
                IprobeIGC*  pprobe = ppl->data();
				if (GetMission()->GetMissionParams()->bAllowAlliedRip) { //ALLY RIPCORD imago 7/8/09
                	if ((pprobe->GetSide() == pside || pside->AlliedSides(pside,pprobe->GetSide())) && pprobe->GetCanRipcord(ripcordSpeed)) //ALLY ripcord imago 7/8/09
                    	return true;
				} else {
                	if ((pprobe->GetSide() == pside) && pprobe->GetCanRipcord(ripcordSpeed))
                    	return true;
				}
            }
        }
        clustersVisited.first(pcluster);

        //Push the destinations of the warps in pcluster onto the end the list of
        //warps that are an extra jump away
        {
            for (WarpLinkIGC*   l = pcluster->GetWarps()->first(); (l != NULL); l = l->next())
            {
                IwarpIGC*   w = l->data();
                if (CanSee(w))
                {
                    IwarpIGC*   pwarpDestination = w->GetDestination();
                    if (pwarpDestination)
                    {
                        IclusterIGC*    pclusterOther = pwarpDestination->GetCluster();

                        //Have we visited pclusterOther?
                        if (clustersVisited.find(pclusterOther) == NULL)
                        {
                            //No
                            pwlTwoAway->last(pwarpDestination);
                        }
                    }
                }
            }
        }

        //Find the next cluster to search
        if (pwlOneAway->n() == 0)
        {
            if ((pwlTwoAway->n() == 0) || (dShip-- <= 1))
                return false;                   //No place left to search or no place closer than the ship

            //No clusters in the current distance bracket ... start on the clusters in the next distance bracket
            WarpListIGC* pwl = pwlOneAway;
            pwlOneAway = pwlTwoAway;
            pwlTwoAway = pwl;
        }

        assert (pwlOneAway->n() > 0);
        WarpLinkIGC*    plink = pwlOneAway->first();
        IwarpIGC*       pwarp = plink->data();

        delete plink;

        pcluster = pwarp->GetCluster();
    }
}

bool    CshipIGC::InGarage(IshipIGC* pship, float   tCollision) const
{
    bool    bInside = false;

    int i = m_myHullType.GetHullType()->GetLandSlots() - 1;

    if (i >= 0)
    {
        HitTest*            pht = pship->GetHitTest();

        HitTestShape        kMax = pht->GetTrueShape();
        HitTestShape        kMin;
        if (kMax > 0)
        {
            assert (kMax != 0);
            kMin = 0;
        }
        else
        {
            kMin = kMax++;
        }

        const Orientation&  orientationStation = GetOrientation();
        const Orientation&  orientationShip = pship->GetOrientation();

        Vector              dp = (pship->GetPosition() - GetPosition()) +
                                 (pship->GetVelocity() - GetVelocity()) * tCollision;
        do
        {
            int j =  m_myHullType.GetHullType()->GetLandPlanes(i) - 1;
            assert (j >= 0);
            do
            {
                Vector  direction = m_myHullType.GetHullType()->GetLandDirection(i, j) * orientationStation;
                Vector  point = m_myHullType.GetHullType()->GetLandPosition(i, j) * orientationStation;

                int k = kMin;
                do
                {
                    Vector  pMin = pht->GetMinExtreme(k, dp, orientationShip, direction);
                    if ((pMin - point) * direction < 0.0f)
                        break;
                }
                while (++k < kMax);

                if (k < kMax)
                    break;
            }
            while (j-- > 0);

            if (j == -1)
            {
                //Found a place to land
                bInside = true;
                break;
            }
        }
        while (i-- > 0);
    }

    return bInside;
}

//Ibase
HRESULT     MyHullType::Initialize(ImissionIGC* pMission, Time now, const void* data, int length)
{
    assert (false);
    return E_FAIL;
}
void                MyHullType::Terminate(void)
{
    assert (false);
}
void                MyHullType::Update(Time   now)
{
    assert (false);
}

ObjectType          MyHullType::GetObjectType(void) const
{
    return OT_hullType;
}

ObjectID            MyHullType::GetObjectID(void) const
{
    return m_pHullData ? m_pHullData->hullID : NA;
}

// ItypeIGC
const void*         MyHullType::GetData(void) const
{
    return m_pHullData;
}

// IbuyableIGC
const char*          MyHullType::GetModelName(void) const
{
    return m_pHullData->modelName;
}

const char*          MyHullType::GetName(void) const
{
    return m_pHullData->name;
}
const char*          MyHullType::GetDescription(void) const
{
    return m_pHullData->description;
}
Money                MyHullType::GetPrice(void) const
{
    return m_pHullData->price;
}
DWORD                MyHullType::GetTimeToBuild(void) const
{
    return m_pHullData->timeToBuild;
}
BuyableGroupID       MyHullType::GetGroupID(void) const
{
    return m_pHullData->groupID;
}

const TechTreeBitMask&  MyHullType::GetRequiredTechs(void) const
{
    return m_pHullData->ttbmRequired;
}

const TechTreeBitMask&  MyHullType::GetEffectTechs(void) const
{
    return m_pHullData->ttbmEffects;
}

// IhullTypeIGC
float                MyHullType::GetMass(void) const
{
    return m_pHullData->mass;
}

float               MyHullType::GetSignature(void) const
{
    return m_pHullData->signature;
}

float                MyHullType::GetLength(void) const
{
    return (float)(m_pHullData->length);
}
float                MyHullType::GetMaxSpeed(void) const
{
    return m_pHullData->speed * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMaxSpeed);
}
float                MyHullType::GetMaxTurnRate(Axis    axis) const
{
    assert (axis >= 0);
    assert (axis <= 3);
    return m_pHullData->maxTurnRates[axis] * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaTurnRate);
}
float                MyHullType::GetTurnTorque(Axis    axis) const
{
    assert (axis >= 0);
    assert (axis <= 3);
    return m_pHullData->turnTorques[axis]  * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaTurnTorque);
}
float                MyHullType::GetThrust(void) const
{
    return m_pHullData->thrust * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaThrust);
}
float                MyHullType::GetSideMultiplier(void) const
{
    return m_pHullData->sideMultiplier;
}
float                MyHullType::GetBackMultiplier(void) const
{
    return m_pHullData->backMultiplier;
}
float                MyHullType::GetScannerRange(void) const
{
    return m_pHullData->scannerRange * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaScanRange);
}

float                MyHullType::GetMaxEnergy(void) const
{
    return m_pHullData->maxEnergy * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMaxEnergy);
}
float                MyHullType::GetRechargeRate(void) const
{
    return m_pHullData->rechargeRate;
}

HitPoints            MyHullType::GetHitPoints(void) const
{
    return (HitPoints)(m_pHullData->hitPoints * m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMaxArmorShip));
}
DefenseTypeID        MyHullType::GetDefenseType(void) const
{
    return m_pHullData->defenseType;
}
PartMask              MyHullType::GetPartMask(EquipmentType et, Mount mountID) const
{
    assert (m_pHullType);
    return m_pHullType->GetPartMask(et, mountID);
}
short                 MyHullType::GetCapacity(EquipmentType et) const
{
    assert (m_pHullType);
    return m_pHullType->GetCapacity(et);
}
Mount                MyHullType::GetMaxWeapons(void) const
{
    return m_pHullData->maxWeapons;
}
Mount                MyHullType::GetMaxFixedWeapons(void) const
{
    return m_pHullData->maxFixedWeapons;
}
const HardpointData& MyHullType::GetHardpointData(Mount hardpointID) const
{
    assert ((hardpointID >= 0) && (hardpointID < m_pHullData->maxWeapons));
    return ((HardpointData*)(((char*)m_pHullData) + m_pHullData->hardpointOffset))[hardpointID];
}

const char*          MyHullType::GetTextureName(void) const
{
    return m_pHullData->textureName;
}

const char*          MyHullType::GetIconName(void) const
{
    return m_pHullData->iconName;
}

HullAbilityBitMask   MyHullType::GetCapabilities(void) const
{
    return m_pHullData->habmCapabilities;
}
bool                 MyHullType::HasCapability(HullAbilityBitMask habm) const
{
    return (m_pHullData->habmCapabilities & habm) != 0;
}
const Vector&        MyHullType::GetCockpit(void) const
{
    assert (m_pHullType);
    return m_pHullType->GetCockpit();
}

// TurkeyXIII 11/09 #94
const Vector&        MyHullType::GetChaffPosition(void) const
{
    assert (m_pHullType);
    return m_pHullType->GetChaffPosition();
}

const Vector&        MyHullType::GetWeaponPosition(Mount mount) const
{
    assert (m_pHullType);
    return m_pHullType->GetWeaponPosition(mount);
}
const Orientation&        MyHullType::GetWeaponOrientation(Mount mount) const
{
    assert (m_pHullType);
    return m_pHullType->GetWeaponOrientation(mount);
}

float MyHullType::GetScale() const
{
    return m_pHullType->GetScale();
}

float   MyHullType::GetMaxFuel(void) const
{
    return m_pHullData->maxFuel;
}

float    MyHullType::GetECM(void) const
{
    return m_pHullData->ecm;
}

float    MyHullType::GetRipcordSpeed(void) const
{
    return m_pHullData->ripcordSpeed / m_pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaRipcordTime);
}

float    MyHullType::GetRipcordCost(void) const
{
    return m_pHullData->ripcordCost;
}

short   MyHullType::GetMaxAmmo(void) const
{
    return m_pHullData->maxAmmo;
}
bool                 MyHullType::CanMount(IpartTypeIGC* ppt, Mount  mountID) const
{
    assert (m_pHullType);
    return m_pHullType->CanMount(ppt, mountID);
}

IhullTypeIGC*        MyHullType::GetSuccessorHullType(void) const
{
    assert (m_pHullType);
    return m_pHullType->GetSuccessorHullType();
}

/*
const char*          MyHullType::GetPilotHUDName(void) const
{
    return m_pHullData->pilotHUDName;
}
const char*          MyHullType::GetObserverHUDName(void) const
{
    return m_pHullData->observerHUDName;
}
*/

SoundID              MyHullType::GetInteriorSound(void) const
{
    return m_pHullData->interiorSound;
}
SoundID              MyHullType::GetExteriorSound(void) const
{
    return m_pHullData->exteriorSound;
}

SoundID              MyHullType::GetMainThrusterInteriorSound(void) const
{
    return m_pHullData->mainThrusterInteriorSound;
}
SoundID              MyHullType::GetMainThrusterExteriorSound(void) const
{
    return m_pHullData->mainThrusterExteriorSound;
}

SoundID              MyHullType::GetManuveringThrusterInteriorSound(void) const
{
    return m_pHullData->manuveringThrusterInteriorSound;
}
SoundID              MyHullType::GetManuveringThrusterExteriorSound(void) const
{
    return m_pHullData->manuveringThrusterExteriorSound;
}
const PartTypeListIGC* MyHullType::GetPreferredPartTypes(void) const
{
    return m_pHullType->GetPreferredPartTypes();
}
IObject*            MyHullType::GetIcon(void) const
{
    return  m_pHullType->GetIcon();
}

int                     MyHullType::GetLaunchSlots(void) const
{
    return m_pHullType->GetLaunchSlots();
}

const Vector&           MyHullType::GetLaunchPosition(int   slotID) const
{
    return m_pHullType->GetLaunchPosition(slotID);
}

const Vector&           MyHullType::GetLaunchDirection(int   slotID) const
{
    return m_pHullType->GetLaunchDirection(slotID);
}

int                     MyHullType::GetLandSlots(void) const
{
    return m_pHullType->GetLandSlots();
}

int                     MyHullType::GetLandPlanes(int   slotID) const
{
    return m_pHullType->GetLandPlanes(slotID);
}

const Vector&           MyHullType::GetLandPosition(int slotID, int planeID) const
{
    return m_pHullType->GetLandPosition(slotID, planeID);
}

const Vector&           MyHullType::GetLandDirection(int slotID, int planeID) const
{
    return m_pHullType->GetLandDirection(slotID, planeID);
}
