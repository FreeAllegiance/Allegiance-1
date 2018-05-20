/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	stationIGC.cpp
**
**  Author: 
**
**  Description:
**      Implementation of the CstationIGC class. This file was initially created by
**  the ATL wizard for the core object.
**
**  History:
*/
// stationIGC.cpp : Implementation of CstationIGC
#include "stationIGC.h"
#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// CstationIGC
HRESULT     CstationIGC::Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize)
{
    TmodelIGC<IstationIGC>::Initialize(pMission, now, data, dataSize);

    HRESULT rc = S_OK;

    debugf("Station initialize %d\n", ((DataStationIGC*)data)->stationID);
    ZRetailAssert (data && (dataSize == sizeof(DataStationIGC)));
    {
        DataStationIGC*  dataStation = (DataStationIGC*)data;

        //Does this station already exist
        IstationIGC*    pstation = pMission->GetStation(dataStation->stationID);
        if (pstation)
        {
            //Uh-oh ... black magic time.
            //The station already exists, so modify it by changing (the pre-existing) station's hull type and name.
            pstation->SetBaseStationType(pMission->GetStationType(dataStation->stationTypeID));
            pstation->SetName(dataStation->name);

            pstation->SetFraction(dataStation->bpHull);
            pstation->SetShieldFraction(dataStation->bpShield);

            //Certain things should not be changed by the export
            assert (pstation->GetSide()->GetObjectID() == dataStation->sideID);
            assert (pstation->GetCluster()->GetObjectID() == dataStation->clusterID);

            //Set the return code so the existing object is not terminated
            rc = E_ABORT;
        }
        else
        {
            m_stationID = dataStation->stationID;

            SetBaseStationType(pMission->GetStationType(dataStation->stationTypeID));
            assert (m_myStationType.GetStationType());

            SetPosition(dataStation->position);
            {
                Orientation o(dataStation->forward, dataStation->up);
                SetOrientation(o);
            }
            SetRotation(dataStation->rotation);

            SetName(dataStation->name);

            SetSide(pMission->GetSide(dataStation->sideID));

            IclusterIGC*    cluster = pMission->GetCluster(dataStation->clusterID);
            assert (cluster);
            SetCluster(cluster);

            pMission->AddStation(this);

            //Let the IGC site know about the new station
            GetMyMission()->GetIgcSite()->StationTypeChange(this);

            m_hullFraction = dataStation->bpHull;
            SetShieldFraction(dataStation->bpShield);

            this->GetSide()->UpdateTerritory();
        }

        m_timeLastDamageReport = now;
    }

    return rc;
}

int         CstationIGC::Export(void* data) const
{
    if (data)
    {
        DataStationIGC*  dataStation = (DataStationIGC*)data;
        dataStation->stationTypeID = m_myStationType.GetObjectID();
        dataStation->stationID = m_stationID;

        dataStation->position = GetPosition();
        {
            const Orientation&  o = GetOrientation();
            dataStation->forward = o.GetForward();
            dataStation->up      = o.GetUp();
        }
        dataStation->rotation = GetRotation();

        assert (GetCluster());
        dataStation->clusterID = GetCluster()->GetObjectID();
        UTL::putName(dataStation->name, GetName());
        dataStation->sideID = GetSide()->GetObjectID();

        dataStation->bpHull = m_hullFraction;
        dataStation->bpShield = m_shieldFraction;
    }

    return sizeof(DataStationIGC);
}
DamageResult    CstationIGC::ReceiveDamage(DamageTypeID            type,
                                           float                   amount,
                                           Time                    timeCollision,
                                           const Vector&           position1,
                                           const Vector&           position2,
                                           ImodelIGC*              launcher)
{
    IsideIGC*   pside = GetSide();

    const MissionParams*  pmp = GetMyMission()->GetMissionParams();
    if (pmp->bInvulnerableStations ||
        (m_myStationType.HasCapability(c_sabmPedestal)) ||
        (launcher && (amount >= 0.0f) &&
         (!GetMyMission()->GetMissionParams()->bAllowFriendlyFire) &&
         (IsideIGC::AlliedSides(pside,launcher->GetSide())))) //Andon: #Ally - Fixes Friendly Fire bug with stations
    {
        return c_drNoDamage;
    }

    DamageResult    dr;

    float   maxArmor = (float)m_myStationType.GetMaxArmorHitPoints();
    float   dtmArmor = GetMyMission()->GetDamageConstant(type, m_myStationType.GetArmorDefenseType());
    assert (dtmArmor >= 0.0f);

    if (amount < 0.0f)
    {
        m_hullFraction -= amount * dtmArmor / maxArmor;
        if (m_hullFraction > 1.0f)
            m_hullFraction = 1.0f;
                    else
        GetThingSite ()->RemoveDamage (m_hullFraction);

        dr = c_drNoDamage;
    }
    else
    {
        float   dtmShield = GetMyMission()->GetDamageConstant(type, m_myStationType.GetShieldDefenseType());

        if (dtmShield != 0.0f)
        {
            dr = c_drShieldDamage;

            if (launcher && (launcher->GetObjectType() == OT_ship))
                amount *= ((IshipIGC*)launcher)->GetExperienceMultiplier();

            IIgcSite*   pigc = GetMyMission()->GetIgcSite();
            pigc->DamageStationEvent(this, launcher, type, amount);

            float   maxShield = (float)m_myStationType.GetMaxShieldHitPoints();
            float   absorbed = amount * dtmShield / maxShield;

            if (m_shieldFraction >= absorbed)
            {
                SetShieldFraction(m_shieldFraction - absorbed);
            }
            else
            {
                if (dtmArmor != 0.0f)
                {
                    dr = c_drHullDamage;
                    absorbed -= m_shieldFraction;
                    if (m_hullFraction > 0.0f)
                    {
                        float   dmg = (absorbed * maxShield * dtmArmor) / (maxArmor * dtmShield);
                        m_hullFraction -= dmg;

                        if (m_hullFraction <= 0.0f)
                        {
                            m_hullFraction = 0.0f;
                            pigc->KillStationEvent(this, launcher, amount);
                            dr = c_drKilled;
                        }
                        else if ((type & c_dmgidNoDebris) == 0)
                            GetThingSite ()->AddDamage (position2 - position1, m_hullFraction);
                    }
                }

                SetShieldFraction(0.0f);
            }
        }
        else
            dr = c_drNoDamage;
    }

    return dr;
}

void                CstationIGC::SetBaseStationType(IstationTypeIGC*    pst)
{
    assert (pst);

    IclusterIGC*    pclusterOld;
    Vector          positionOld;
    Orientation     orientationOld;
    Rotation        rotationOld;

    if (m_myStationType.GetStationType())
    {
        assert (GetSide());

        pclusterOld = GetCluster();

        positionOld = GetPosition();
        orientationOld = GetOrientation();
        rotationOld = GetRotation();

        //Move the station to the null cluster while all the black magic happens
        SetCluster(NULL);

        FreeThingSite();
    }
    else
        assert (!GetSide());

    m_myStationType.SetStationType(pst);

    {
        HRESULT rc = Load(0,
                          m_myStationType.GetModelName(),
                          m_myStationType.GetTextureName(),
                          m_myStationType.GetIconName(),
                          c_mtStatic | c_mtDamagable | c_mtHitable | c_mtSeenBySide | c_mtPredictable | c_mtScanner);

        assert (SUCCEEDED(rc));
    }

    SetRadius(m_myStationType.GetRadius());

    SetMass(0.0f);
    SetSignature(m_myStationType.GetSignature());

    m_undockPosition = 0;

    IsideIGC*   pside = GetSide();
    if (pside)
    {
        //Set the station hit test to allow ships of its own side to enter its shields
        HitTest*    pht = GetHitTest();
        pht->SetUseTrueShapeSelf(pside);

        pht->SetShape(c_htsConvexHullMax);

        //We have a side, which means we had a station type before (see above) and the following
        //data is valid:
        SetPosition(positionOld);
        SetOrientation(orientationOld);
        SetRotation(rotationOld);
        SetCluster(pclusterOld);

        //The upgrade really should not be less capable than its predecessor but let's be paranoid
        pside->ResetBuildingTechs();

        //Let the IGC site know
        GetMyMission()->GetIgcSite()->StationTypeChange(this);
    }
}

void                CstationIGC::AddShip(IshipIGC*    pship)
{
    AddIbaseIGC((BaseListIGC*)&m_shipsDocked, pship);
    //At a station implies not in a cluster
    pship->SetCluster(NULL);
}
void                CstationIGC::DeleteShip(IshipIGC* s)
{
    DeleteIbaseIGC((BaseListIGC*)&m_shipsDocked, s);
}
IshipIGC*           CstationIGC::GetShip(ShipID   id) const
{
    return (IshipIGC*)GetIbaseIGC((BaseListIGC*)&m_shipsDocked, id);
}
const ShipListIGC*        CstationIGC::GetShips(void) const
{
    return &m_shipsDocked;
}

void CstationIGC::Launch(IshipIGC* pship)
{
	Orientation orientation;
    const float maxRandomOffset = 5.0f;
    Vector position(random(-maxRandomOffset, maxRandomOffset), random(-maxRandomOffset, maxRandomOffset), random(-maxRandomOffset, maxRandomOffset));
    Vector forward;

    const Orientation&  o = GetOrientation();
    float   vLaunch = GetMyMission()->GetFloatConstant(c_fcidExitStationSpeed); //imago TODO use m_myLaunchSpeed - this will allow core devs to tweak

	IclusterIGC*    pcluster = GetCluster();
    int nLaunchSlots = m_myStationType.GetLaunchSlots();
    if (nLaunchSlots == 0)
    {
        if ((pship->GetPilotType() < c_ptPlayer) && m_myStationType.HasCapability(c_sabmRipcord)) { //imago 8/7/09
            forward = Vector::RandomDirection();
        } else { //do the regular thing
            switch (m_undockPosition++)
            {
                case 4:
                {
                    m_undockPosition = 0;
                }
                //No break
                case 0:
                {
                    forward = o.GetRight();
                }
                break;
                case 1:
                {
                    forward = o.GetUp();
                }
                break;
                case 2:
                {
                    forward = -o.GetRight();
                }
                break;
                case 3:
                {
                    forward = -o.GetUp();
                }
                break;
            }
        }

        position += forward * (GetRadius() + pship->GetRadius() + vLaunch * 0.5f + maxRandomOffset);
    }
    else
    { 
        position += m_myStationType.GetLaunchPosition(m_undockPosition) * o;
        forward = m_myStationType.GetLaunchDirection(m_undockPosition) * o;

		//Imago 6/10
		Time    lastUpdate = pcluster->GetLastUpdate();
		Time    lastLaunch = GetLastLaunch();
		float	m_fDeltaTime = (float)(lastUpdate - lastLaunch);
		//debugf(" *** %s(%i) launch time cluster delta = %f\n\n", m_myStationType.GetName(), m_myStationType.GetObjectID(), m_fDeltaTime);
		if (m_fDeltaTime <= 0.1f) {
			 position += forward * (pship->GetRadius() + vLaunch * 0.85f + maxRandomOffset);
			 //debugf("*** %s(%i) position adjusted to ensure smooth take-off\n",pship->GetName(),pship->GetObjectID());
		} else {
			 position += forward * (pship->GetRadius() + vLaunch * 0.5f + maxRandomOffset);
		}
		//

        m_undockPosition = (m_undockPosition + 1) % nLaunchSlots;
    }

    orientation.Set(forward, o.GetForward());


    
    pship->SetPosition(position + GetPosition());
    pship->SetOrientation(orientation);
    pship->SetVelocity(forward * vLaunch);
    pship->SetCurrentTurnRate(c_axisYaw, 0.0f);
    pship->SetCurrentTurnRate(c_axisPitch, 0.0f);
    pship->SetCurrentTurnRate(c_axisRoll, 0.0f);

    pship->SetCluster(pcluster);
	pship->SetLastTimeLaunched(Time::Now());
	SetLastLaunch(Time::Now());
}

bool    CstationIGC::InGarage(IshipIGC* pship, const Vector& position)
{
    bool    bInside = false;

    int i = (pship->GetBaseHullType()->HasCapability(c_habmFighter)
             ? m_myStationType.GetLandSlots()
             : m_myStationType.GetCapLandSlots()) - 1;

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

        Vector              dp = position - GetPosition();
        do
        {
            int j =  m_myStationType.GetLandPlanes(i) - 1;
            assert (j >= 0);
            do
            {
                Vector  direction = m_myStationType.GetLandDirection(i, j) * orientationStation;
                Vector  point = m_myStationType.GetLandPosition(i, j) * orientationStation;

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

void    CstationIGC::RepairAndRefuel(IshipIGC* pship) const
{
    assert ((pship->GetParentShip() == NULL) && (pship->GetBaseHullType() != NULL));

    //Fully mount all parts
    {
        for (PartLinkIGC*   ppl = pship->GetParts()->first();
             (ppl != NULL);
             ppl = ppl->next())
        {
            ppl->data()->SetMountedFraction(1.0f);
        }
    }

    if (m_myStationType.HasCapability(c_sabmRepair))
    {
        pship->SetFraction(1.0f);
    }

    {
        IshieldIGC* s = (IshieldIGC*)(pship->GetMountedPart(ET_Shield, 0));
        if (s)
            s->SetFraction(1.0f);
    }

    {
        IafterburnerIGC*    pafter = (IafterburnerIGC*)(pship->GetMountedPart(ET_Afterburner, 0));
        if (pafter)
            pafter->Deactivate();
    }

    //Give a full load of fuel and ammo
    const IhullTypeIGC* pht = pship->GetHullType();
    if (m_myStationType.HasCapability(c_sabmReload))
    {
        short   maxAmmo = pht->GetMaxAmmo();
        short   maxFuel = short(pht->GetMaxFuel());

        pship->SetAmmo(maxAmmo);
        pship->SetFuel(maxFuel);

        for (PartLinkIGC*   ppl = pship->GetParts()->first();
             (ppl != NULL);
             ppl = ppl->next())
        {
            IpartIGC*   ppart = ppl->data();
            if (ppart->GetObjectType() == OT_pack)
            {
                IpackIGC*   ppack = (IpackIGC*)ppart;

                ppack->SetAmount(ppack->GetPackType() == c_packAmmo ?  maxAmmo : maxFuel);
            }
        }
    }

    pship->SetEnergy(pht->GetMaxEnergy());
}

//Ibase
HRESULT     MyStationType::Initialize(ImissionIGC* pMission, Time now, const void* data, int length)
{
    assert (false);
    return E_FAIL;
}
void                MyStationType::Terminate(void)
{
    assert (false);
}
void                MyStationType::Update(Time   now)
{
    assert (false);
}

ObjectType          MyStationType::GetObjectType(void) const
{
    return OT_stationType;
}

ObjectID            MyStationType::GetObjectID(void) const
{
    return m_pStationData->stationTypeID;
}

// ItypeIGC
const void*         MyStationType::GetData(void) const
{
    return m_pStationData;
}

// IbuyableIGC
const char*          MyStationType::GetModelName(void) const
{
    return m_pStationData->modelName;
}


const char*          MyStationType::GetName(void) const
{
    return m_pStationData->name;
}
const char*          MyStationType::GetDescription(void) const
{
    return m_pStationData->description;
}
Money                MyStationType::GetPrice(void) const
{
    return m_pStationData->price;
}

DWORD                MyStationType::GetTimeToBuild(void) const
{
    return m_pStationData->timeToBuild;
}

BuyableGroupID       MyStationType::GetGroupID(void) const
{
    return m_pStationData->groupID;
}

const TechTreeBitMask&  MyStationType::GetRequiredTechs(void) const
{
    return m_pStationData->ttbmRequired;
}

const TechTreeBitMask&  MyStationType::GetEffectTechs(void) const
{
    return m_pStationData->ttbmEffects;
}

// IstationTypeIGC
float               MyStationType::GetSignature(void) const
{
    return m_pStationData->signature;
}

float                MyStationType::GetRadius(void) const
{
    return m_pStationData->radius;
}

float                MyStationType::GetScannerRange(void) const
{
    return m_pStationData->scannerRange * m_pstation->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaScanRange);
}

HitPoints            MyStationType::GetMaxArmorHitPoints(void) const
{
    return m_pStationData->maxArmorHitPoints * m_pstation->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMaxArmorStation);
}

DefenseTypeID        MyStationType::GetArmorDefenseType(void) const
{
    return m_pStationData->defenseTypeArmor;
}

HitPoints            MyStationType::GetMaxShieldHitPoints(void) const
{
    return m_pStationData->maxShieldHitPoints * m_pstation->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMaxShieldStation);
}

DefenseTypeID        MyStationType::GetShieldDefenseType(void) const
{
    return m_pStationData->defenseTypeShield;
}

float                MyStationType::GetArmorRegeneration(void) const
{
    return m_pStationData->armorRegeneration * m_pstation->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaArmorRegenerationStation);
}

float                MyStationType::GetShieldRegeneration(void) const
{
    return m_pStationData->shieldRegeneration * m_pstation->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaShieldRegenerationStation);
}
Money                MyStationType::GetIncome(void) const
{
    return m_pStationData->income;
}

const TechTreeBitMask&  MyStationType::GetLocalTechs(void) const
{
    return m_pStationData->ttbmLocal;
}

StationAbilityBitMask   MyStationType::GetCapabilities(void) const
{
    return m_pStationData->sabmCapabilities;
}
bool                    MyStationType::HasCapability(StationAbilityBitMask sabm) const
{
    return (m_pStationData->sabmCapabilities & sabm) != 0;
}
const char*             MyStationType::GetTextureName(void) const
{
    return m_pStationData->textureName;
}
const char*             MyStationType::GetBuilderName(void) const
{
    return m_pStationData->builderName;
}
const char*             MyStationType::GetIconName(void) const
{
    return m_pStationData->iconName;
}
IstationTypeIGC*  MyStationType::GetSuccessorStationType(const IsideIGC*    pside)
{
    return m_pStationType->GetSuccessorStationType(pside);
}
// EF5P
IstationTypeIGC*  MyStationType::GetDirectSuccessorStationType(void)
{
    return m_pStationType->GetDirectSuccessorStationType();
}
AsteroidAbilityBitMask   MyStationType::GetBuildAABM(void) const
{
    return m_pStationData->aabmBuild;
}


int                     MyStationType::GetLaunchSlots(void) const
{
    return m_pStationType->GetLaunchSlots();
}
const Vector&           MyStationType::GetLaunchPosition(int   slotID) const
{
    return m_pStationType->GetLaunchPosition(slotID);
}
const Vector&           MyStationType::GetLaunchDirection(int   slotID) const
{
    return m_pStationType->GetLaunchDirection(slotID);
}

int                     MyStationType::GetLandSlots(void) const
{
    return m_pStationType->GetLandSlots();
}
int                     MyStationType::GetCapLandSlots(void) const
{
    return m_pStationType->GetCapLandSlots();
}
int                     MyStationType::GetLandPlanes(int   slotID) const
{
    return m_pStationType->GetLandPlanes(slotID);
}
const Vector&           MyStationType::GetLandPosition(int slotID, int planeID) const
{
    return m_pStationType->GetLandPosition(slotID, planeID);
}
const Vector&           MyStationType::GetLandDirection(int slotID, int planeID) const
{
    return m_pStationType->GetLandDirection(slotID, planeID);
}

SoundID                 MyStationType::GetInteriorSound() const
{
    return m_pStationType->GetInteriorSound();
}
SoundID                 MyStationType::GetInteriorAlertSound() const
{
    return m_pStationType->GetInteriorAlertSound();
}
SoundID                 MyStationType::GetExteriorSound() const
{
    return m_pStationType->GetExteriorSound();
}
SoundID                 MyStationType::GetConstructorNeedRockSound() const
{
    return m_pStationType->GetConstructorNeedRockSound();
}
SoundID                 MyStationType::GetConstructorUnderAttackSound() const
{
    return m_pStationType->GetConstructorUnderAttackSound();
}
SoundID                 MyStationType::GetConstructorDestroyedSound() const
{
    return m_pStationType->GetConstructorDestroyedSound();
}
SoundID                 MyStationType::GetCompletionSound() const
{
    return m_pStationType->GetCompletionSound();
}
SoundID                 MyStationType::GetUnderAttackSound() const
{
    return m_pStationType->GetUnderAttackSound();
}
SoundID                 MyStationType::GetCriticalSound() const
{
    return m_pStationType->GetCriticalSound();
}
SoundID                 MyStationType::GetDestroyedSound() const
{
    return m_pStationType->GetDestroyedSound();
}
SoundID                 MyStationType::GetCapturedSound() const
{
    return m_pStationType->GetCapturedSound();
}
SoundID                 MyStationType::GetEnemyCapturedSound() const
{
    return m_pStationType->GetEnemyCapturedSound();
}
SoundID                 MyStationType::GetEnemyDestroyedSound() const
{
    return m_pStationType->GetEnemyDestroyedSound();
}
StationClassID          MyStationType::GetClassID() const
{
    return m_pStationType->GetClassID();
}

IdroneTypeIGC*          MyStationType::GetConstructionDroneType(void) const
{
    return m_pStationType->GetConstructionDroneType();
}


