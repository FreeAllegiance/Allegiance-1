/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:    shipIGC.h
**
**  Author: 
**
**  Description:
**      Header for the CshipIGC class. This file was initially created by
**  the ATL wizard.
**
**  History:
*/
// shipIGC.h : Declaration of the CshipIGC

#ifndef __SHIPIGC_H_
#define __SHIPIGC_H_

#include "igc.h"
#include "modelIGC.h"

class       CshipIGC;

class       MyHullType : public IhullTypeIGC
{
    public:
        MyHullType(CshipIGC*    pship)
        :
            m_pship(pship),
            m_pHullType(NULL)
        {
        }

        ~MyHullType(void)
        {
            if (m_pHullType)
                m_pHullType->Release();
        }

        //IbaseIGC
        virtual HRESULT         Initialize(ImissionIGC* pMission, Time now, const void* data, int length);
        virtual void            Terminate(void);
        virtual void            Update(Time   now);

        virtual ObjectType      GetObjectType(void) const;
        virtual ObjectID        GetObjectID(void) const;

        //ItypeIGC
        virtual const void*     GetData(void) const;

        //IbuyableIGC
        virtual const char*             GetName(void) const;
        virtual const char*             GetDescription(void) const;
        virtual const char*             GetModelName(void) const;
        virtual const TechTreeBitMask&  GetRequiredTechs(void) const;
        virtual const TechTreeBitMask&  GetEffectTechs(void) const;

        virtual Money                   GetPrice(void) const;
        virtual DWORD                   GetTimeToBuild(void) const;
        virtual BuyableGroupID          GetGroupID(void) const;

        //IhullTypeIGC
        virtual float                GetLength(void) const;
        virtual float                GetMaxSpeed(void) const;
        virtual float                GetMaxTurnRate(Axis        axis) const;
        virtual float                GetTurnTorque(Axis   axis) const;
        virtual float                GetThrust(void) const;
        virtual float                GetSideMultiplier(void) const;
        virtual float                GetBackMultiplier(void) const;
        virtual float                GetScannerRange(void) const;

        virtual float                GetMaxEnergy(void) const;
        virtual float                GetRechargeRate(void) const;

        virtual HitPoints            GetHitPoints(void) const;
        virtual DefenseTypeID        GetDefenseType(void) const;
        virtual bool                 CanMount(IpartTypeIGC* ppt, Mount  mountID) const;
        virtual PartMask             GetPartMask(EquipmentType et, Mount mountID) const;
        virtual short                GetCapacity(EquipmentType et) const;
        virtual Mount                GetMaxWeapons(void) const;
        virtual Mount                GetMaxFixedWeapons(void) const;
        virtual const HardpointData& GetHardpointData(Mount hardpointID) const;                       

        virtual const char*          GetTextureName(void) const;
        virtual const char*          GetIconName(void) const;

        virtual float                GetMass(void) const;
        virtual float                GetSignature(void) const;

        virtual HullAbilityBitMask   GetCapabilities(void) const;
        virtual bool                 HasCapability(HullAbilityBitMask habm) const;

        virtual const Vector&        GetCockpit(void) const;
		virtual const Vector&		 GetChaffPosition(void) const; // TurkeyXIII 11/09 #94

        virtual const Vector&        GetWeaponPosition(Mount mount) const;
        virtual const Orientation&   GetWeaponOrientation(Mount mount) const;
        virtual float                GetScale() const;

        virtual short                GetMaxAmmo(void) const;
        virtual float                GetMaxFuel(void) const;
        virtual float                GetECM(void) const;
        virtual float                GetRipcordSpeed(void) const;
        virtual float                GetRipcordCost(void) const;

        virtual IhullTypeIGC*        GetSuccessorHullType(void) const;
        /*
        virtual const char*          GetPilotHUDName(void) const;
        virtual const char*          GetObserverHUDName(void) const;
        */

        virtual SoundID              GetInteriorSound(void) const;
        virtual SoundID              GetExteriorSound(void) const;
        virtual SoundID              GetMainThrusterInteriorSound(void) const;
        virtual SoundID              GetMainThrusterExteriorSound(void) const;
        virtual SoundID              GetManuveringThrusterInteriorSound(void) const;
        virtual SoundID              GetManuveringThrusterExteriorSound(void) const;

        virtual const PartTypeListIGC*      GetPreferredPartTypes(void) const;

        virtual IObject*            GetIcon(void) const;

        virtual int                     GetLaunchSlots(void) const;
        virtual const Vector&           GetLaunchPosition(int   slotID) const;
        virtual const Vector&           GetLaunchDirection(int   slotID) const;

        virtual int                     GetLandSlots(void) const;
        virtual int                     GetLandPlanes(int   slotID) const;
        virtual const Vector&           GetLandPosition(int slotID, int planeID) const;
        virtual const Vector&           GetLandDirection(int slotID, int planeID) const;

    private:
        IhullTypeIGC*         GetHullType(void) const
        {
            return m_pHullType;
        }

        void SetHullType(IhullTypeIGC*  pht)
        {
            if (m_pHullType)
                m_pHullType->Release();

            m_pHullType = pht;

            if (pht)
            {
                pht->AddRef();
                m_pHullData = (DataHullTypeIGC*)(pht->GetData());
            }
            else
                m_pHullData = NULL;
        }

        const CshipIGC*         m_pship;
        IhullTypeIGC*           m_pHullType;
        const DataHullTypeIGC*  m_pHullData;

        friend class CshipIGC;
};

class       CshipIGC : public TmodelIGC<IshipIGC>
{
    public:
        CshipIGC(void);

        /*
        DWORD __stdcall  AddRef(void)
        {
            return TmodelIGC<IshipIGC>::AddRef();
        }

        DWORD __stdcall  Release(void)
        {
            return TmodelIGC<IshipIGC>::Release();
        }
        */
    public:
    // IbaseIGC
        virtual HRESULT             Initialize(ImissionIGC* pMission, Time now, const void* data, int length);
        virtual void                Terminate(void);
        virtual void                Update(Time now);
        virtual int                 Export(void* data) const;

        virtual ObjectType  GetObjectType(void) const
        {
            return OT_ship;
        }

        virtual ObjectID            GetObjectID(void) const
        {
            return m_shipID;
        }

    // ImodelIGC
        virtual void    SetCluster(IclusterIGC* cluster)
        {
            IclusterIGC*    pclusterOld = GetCluster();

            if (cluster || pclusterOld)
            {
                //Can't move to a cluster or move to a cluster without a hull
                assert ((!cluster) || (m_myHullType.GetHullType() != NULL) || (m_pshipParent != NULL));
                assert (GetSide() != NULL);

                SetRipcordModel(NULL);

				//Xynth #47 7/2010  Reset rip indicator
				SetStateBits(droneRipMaskIGC, 0);

                if (cluster != pclusterOld)
                {
                    if (pclusterOld)
                        pclusterOld->DeleteShip(this);

                    TmodelIGC<IshipIGC>::SetCluster(cluster);

                    if (cluster)
                    {
                        //Can't be in a cluster and in a station simultaneously
                        assert (m_station == NULL);

                        cluster->AddShip(this);
                        m_maxMineAmount = 0.0f;
                        m_maxMineLauncher = NULL;
                    }
                }

                if (cluster)
                {
                    //Do this even if the cluster does not change: the ship
                    //mostly likely teleported to a new position.
                    Time    lastUpdate = cluster->GetLastUpdate();
                    SetMyLastUpdate(lastUpdate);
                    SetBB(lastUpdate, lastUpdate, 0.0f);
                }

                SetStateBits(oneWeaponIGC | allWeaponsIGC, 0); //Stop shooting any weapons.

                {
                    //Move the children of this ship to the new cluster as well
                    for (ShipLinkIGC*   psl = m_shipsChildren.first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        psl->data()->SetCluster(cluster);
                    }
                }

                GetMyMission()->GetIgcSite()->ChangeCluster(this, pclusterOld, cluster);

                if (pclusterOld == NULL)
                    ResetWaypoint();
            }
        }

        virtual void                 HandleCollision(Time       timeCollision,
                                                     float                  tCollision,
                                                     const CollisionEntry&  entry,
                                                     ImodelIGC* pModel);

        virtual void                SetSide(IsideIGC*   pside);  //override the default SetSide method

        virtual float               GetSignature(void) const
        {
            return TmodelIGC<IshipIGC>::GetSignature() * m_cloaking;
        }

        virtual void                 Move(float t)
        {
            if (m_pshipParent == NULL)
                TmodelIGC<IshipIGC>::Move(t);
        }
        virtual void                 Move(void)
        {
            if (m_pshipParent == NULL)
                TmodelIGC<IshipIGC>::Move();
        }
        virtual void                 SetBB(Time tStart, Time tStop, float dt)
        {
            if (m_pshipParent == NULL)
                TmodelIGC<IshipIGC>::SetBB(tStart, tStop, dt);
        }

        virtual SideID              GetFlag(void) const
        {
            return m_sidFlag;
        }

        virtual void                 SetFlag(SideID sid)
        {
            m_sidFlag = sid;
        }

    // IdamageIGC
        virtual DamageResult        ReceiveDamage(DamageTypeID            type,
                                                  float                   amount,
                                                  Time                    timeCollision,
                                                  const Vector&           position1,
                                                  const Vector&           position2,
                                                  ImodelIGC*              launcher);

        virtual float               GetFraction(void) const
        {
            return m_fraction;
        }
        virtual void                SetFraction(float newVal)
        {
            assert (newVal >= 0.0f);
            assert (newVal <= 1.0f);
            m_fraction = newVal;
            if (newVal > m_fractionLastOrder)
                m_fractionLastOrder = newVal;
            GetThingSite ()->RemoveDamage (m_fraction);
        }
		
		//Xynth #156 7/2010
		virtual void				SetOre(float newOre)
		{
			m_fOre = newOre;
		}

		virtual float				GetOreCapacity() const
		{			
			float minerCapacity;
			minerCapacity = GetMyMission()->GetFloatConstant(c_fcidCapacityHe3) *
                            GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMiningCapacity);
			return minerCapacity;
		}

        virtual float               GetHitPoints(void) const
        {
            return m_fraction * m_myHullType.GetHitPoints();
        }

        // IscannerIGC
        bool            InScannerRange(ImodelIGC*   pModel) const
        {
            assert (pModel);

            bool    rc;
            if (m_pshipParent == NULL)
            {
                IclusterIGC*    pcluster = GetCluster();
                if (pcluster && (pModel->GetCluster() == pcluster))
                {
                    if (pModel->GetFlag() == NA)
                    {
                        float   m = m_myHullType.GetScannerRange() * pModel->GetSignature();
                        {
                            IsideIGC*   pside = pModel->GetSide();
                            if (pside)
                                m /= pside->GetGlobalAttributeSet().GetAttribute(c_gaSignature);
                        }
                        float   r = GetRadius() + pModel->GetRadius() + m;

                        //Get the difference in positions in the local coordinate space.
                        Vector  deltaP = GetOrientation().TimesInverse(GetPosition() - pModel->GetPosition());
                        if (deltaP.z < 0.0f)
                            deltaP.z *= 4.0f;

                        rc = (deltaP.LengthSquared() <= r * r) && LineOfSightExist(pcluster, this, pModel);
                    }
                    else
                        rc = true;
                }
                else
                    rc = false;
            }
            else
                rc = false;

            return rc;
        }

        bool             CanSee(ImodelIGC*   pModel) const
        {
            assert (pModel);

            IsideIGC*   side = GetSide();

            return (pModel->GetSide() == side) ||                               //Always see things on our side
                   pModel->SeenBySide(side) ||
                   InScannerRange(pModel);                                       //or we can see it ourselves
        }

    // IshipIGC
        virtual void                SetObjectID(ObjectID id)
        {
            m_shipID = id;
        }
        virtual Money               GetValue(void) const
        {
            //Money for the hull
            Money money = m_myHullType.GetHullType() ? m_myHullType.GetPrice() : 0;

            //Money for the parts
            for (PartLinkIGC* ppartlink = m_parts.first(); ppartlink; ppartlink = ppartlink->next())
                money += ppartlink->data()->GetPrice();

            return money;
        }

        virtual void                SetMission(ImissionIGC* pMission);

        virtual void                ReInitialize(DataShipIGC * dataShip, Time now);

        virtual void                SetShipID(ShipID    shipID)
        {
            m_shipID = shipID;
        }

        virtual IstationIGC*         GetStation(void) const
        {
            return m_station;
        }
        virtual void                 SetStation(IstationIGC* s)
        {
            IstationIGC*    pstationOld = m_station;
            if (pstationOld != s)
            {
                if (m_station)
                {
                    m_station->DeleteShip(this);
                    m_station->Release();
                }

                m_station = s;

                if (s)
                {
                    if (m_pshipParent == NULL)
                    {
                        IafterburnerIGC*    pafter = (IafterburnerIGC*)(m_mountedOthers[ET_Afterburner]);
                        if (pafter)
                            pafter->Deactivate();   //The station is a no smoking area

                        if ((m_fOre > 0.0f) && (s->GetStationType()->HasCapability(c_sabmUnload) && (GetSide() == s->GetSide()))) //#ALLY: Only offload at your own bases (TheRock)
                        {
                            IsideIGC*   pside = GetSide();

                            GetMyMission()->GetIgcSite()->PaydayEvent(pside,
                                                                      m_fOre *
                                                                      GetMyMission()->GetFloatConstant(c_fcidValueHe3) *
                                                                      pside->GetGlobalAttributeSet().GetAttribute(c_gaMiningYield));
                            m_fOre = 0.0f;
                        }

                        ResetDamageTrack();
                    }

                    s->AddRef();
                    s->AddShip(this);           //This also sets the ship's cluster to NULL

                    assert (GetCluster() == NULL);

                    if (m_bAutopilot)
                    {
                        if (m_pilotType >= c_ptPlayer)
                        {
                            //Players always exit autopilot when docking
                            m_bAutopilot = false;
                        }
                        else
                        {
                            if ((m_commandTargets[c_cmdAccepted] == s) && (m_commandIDs[c_cmdAccepted] == c_cidGoto))
                            {
                                //We docked with the station under autopilot ... that order is complete
                                SetCommand(c_cmdAccepted, NULL, c_cidNone);
                            }
                            else
                            {
                                SetCommand(c_cmdPlan, NULL, c_cidNone);
                            }
                        }
                    }
                }

                GetMyMission()->GetIgcSite()->ChangeStation(this, pstationOld, s);
            }
        }

        virtual void                 Reset(bool bFull)
        {
            m_nKills = 0;
            if (bFull)
            {
                m_nDeaths = 0;
                m_nEjections = 0;
            }

            m_experience = bFull ? 1.0f : 0.0f;

            SetFlag(NA);
            ResetDamageTrack();

            SetAutoDonate(NULL);

            SetCommand(c_cmdQueued, NULL, c_cidNone);
            SetCommand(c_cmdAccepted, NULL, c_cidNone);
            SetCommand(c_cmdCurrent, NULL, c_cidNone);
            SetCommand(c_cmdPlan, NULL, c_cidNone);

            if (m_station)
            {
                m_station->DeleteShip(this);
                m_station->Release();
                m_station = NULL;
            }
            SetRipcordModel(NULL);

            SetCluster(NULL);
            //TmodelIGC<IshipIGC>::SetCluster(NULL);
            assert (m_station == NULL);

            if (m_pshipParent != NULL)
            {
                SetParentShip(NULL);
                assert (m_myHullType.GetHullType() == NULL);
            }
            else
            {
                PartLinkIGC*    pl;
                while (pl = m_parts.first())    //intentional assignment
                    pl->data()->Terminate();

                //Blow away the children
                ShipLinkIGC*    psl;
                while (psl = m_shipsChildren.first())   //intentional assignment
                {
                    psl->data()->SetParentShip(NULL);
                }
            }            

            SetBaseHullType(GetSide()->GetCivilization()->GetLifepod());
        }

        virtual float               GetTorqueMultiplier(void) const
        {
            static const float  c_fMultiplierAtZero = 0.50f;

            float   fraction = GetVelocity().Length() / m_myHullType.GetMaxSpeed();

            return  c_fMultiplierAtZero + (1.0f - c_fMultiplierAtZero) * 2.0f * fraction / (fraction + 1.0f);
        }

        virtual float               GetCurrentTurnRate(Axis axis) const
        {
            return m_turnRates[axis];
        }
        virtual void                SetCurrentTurnRate(Axis axis, float newVal)
        {
            m_turnRates[axis] = newVal;
        }

        virtual const IhullTypeIGC*       GetHullType(void) const
        {
            return &m_myHullType;
        }

        virtual IhullTypeIGC*       GetBaseHullType(void) const
        {
            return m_myHullType.GetHullType();
        }
        virtual void                SetBaseHullType(IhullTypeIGC* newVal);

        virtual void                AddPart(IpartIGC*    part);
        virtual void                DeletePart(IpartIGC* part);
        virtual const PartListIGC*  GetParts(void) const
        {
            return &m_parts;
        }

        virtual IpartIGC*           GetMountedPart(EquipmentType type, Mount mount) const;
        virtual void                MountPart(IpartIGC* p, Mount mountNew, Mount* pmountOld);

        virtual const Vector&       GetCockpit(void) const
        {
            return m_cockpit;
        }

		// TurkeyXIII 11/09 #94
		virtual const Vector&		GetChaffPosition(void) const
		{
			return m_chaff;
		}

        virtual short                 GetAmmo(void) const
        {
            return m_ammo;
        }

        virtual void                SetAmmo(short newVal)
        {
            short   maxAmmo = m_myHullType.GetMaxAmmo();
            if (newVal > maxAmmo)
                newVal = maxAmmo;
            
            short   oldAmmo = m_ammo;
            m_ammo = newVal;

            if ((newVal == 0) && (oldAmmo > 0) && (GetMyLastUpdate() >= m_timeReloadAmmo))
            {
                m_timeReloadAmmo = GetMyLastUpdate() + 0.5f / GetMyMission()->GetFloatConstant(c_fcidMountRate);
                IIgcSite*   pigc = GetMyMission()->GetIgcSite();
                if (!pigc->Reload(this, NULL, ET_Weapon))
                {
                    pigc->PostNotificationText(this, false, "Ammo depleted.");
                }
                else
                {
                    pigc->PlayNotificationSound(salReloadingAmmoSound, this);
                    pigc->PlayNotificationSound(startReloadSound, this);
                    pigc->PostNotificationText(this, false, "Reloading ammo...");
                }
            }
        }

        virtual float               GetFuel(void) const
        {
            return m_fuel;
        }
        virtual void                SetFuel(float newVal)
        {
            float   maxFuel = m_myHullType.GetMaxFuel();
            if (newVal > maxFuel)
                newVal = maxFuel;

            float   oldFuel = m_fuel;
            m_fuel = newVal;

            if ((newVal == 0.0f) && (oldFuel > 0.0f) && (GetMyLastUpdate() >= m_timeReloadFuel))
            {
                m_timeReloadFuel = GetMyLastUpdate() + 0.5f / GetMyMission()->GetFloatConstant(c_fcidMountRate);
                IIgcSite*   pigc = GetMyMission()->GetIgcSite();
                if (!pigc->Reload(this, NULL, ET_Afterburner))
                {
                    pigc->PostNotificationText(this, false, "Fuel depleted.");
                }
                else
                {
                    pigc->PlayNotificationSound(salReloadingFuelSound, this);
                    pigc->PlayNotificationSound(startReloadSound, this);
                    pigc->PostNotificationText(this, false, "Reloading fuel...");
                }
            }
        }

        virtual float               GetEnergy(void) const
        {
            return m_energy;
        }
        virtual void                SetEnergy(float newVal)
        {
            m_energy = newVal;
        }

        virtual float                GetCloaking(void) const
        {
            return m_cloaking;
        }
        virtual void                 SetCloaking(float newVal)
        {
            m_cloaking = newVal;
        }

        virtual bool                GetVectorLock(void) const
        {
            return (m_stateM & coastButtonIGC) ? true : false;
        }
        virtual void                SetVectorLock(bool bVectorLock)
        {
            if (bVectorLock)
                m_stateM |= coastButtonIGC;
            else
                m_stateM &= ~coastButtonIGC;
        }

        virtual const ControlData&  GetControls(void) const
        {
            return m_controls;
        }
        virtual void                SetControls(const ControlData&  newVal)
        {
            m_controls = newVal;
        }

        virtual const Vector&       GetEngineVector(void) const
        {
            return m_engineVector;
        }

        virtual int                 GetStateM(void) const
        {
            return m_stateM;
        }
        virtual void                SetStateM(int newVal);
        virtual void                SetStateBits(int mask, int newBits)
        {
            SetStateM((m_stateM & (~mask)) | (newBits & mask));
        }

        virtual ImodelIGC*           GetCommandTarget(Command i)  const
        {
            assert (i >= 0);
            assert (i < c_cmdMax);

            return m_commandTargets[i];
        }
        virtual CommandID            GetCommandID(Command i) const
        {
            assert (i >= 0);
            assert (i < c_cmdMax);

            return m_commandIDs[i];
        }
        virtual void                 SetCommand(Command i, ImodelIGC* target, CommandID cid)
        {
            //debugf("%s: SetCommand(%d, %s in %s, %d)\n", GetName(), i, (target ? GetModelName(target) : "NULL"), ((target && target->GetCluster()) ? target->GetCluster()->GetName() : "-"), cid);
            assert (i >= 0);
            assert (i < c_cmdMax);

            CommandID cidOld = m_commandIDs[i];
            ImodelIGC*  pmodelOld = m_commandTargets[i];
            //if ((target != pmodelOld) || (cid != m_commandIDs[i]))
            {
                m_commandTargets[i] = target;
                if (target)
                {
                    target->AddRef();
                    if (target->GetObjectType() == OT_buoy)
                        ((IbuoyIGC*)target)->AddConsumer();
                }

                m_commandIDs[i] = cid;

                switch (i)
                {
                    case c_cmdCurrent:
                    {
                        if ((pmodelOld != target) && m_mountedOthers[ET_Magazine])
                            ((ImagazineIGC*)m_mountedOthers[ET_Magazine])->SetLock(0.0f);
                    }
                    break;

                    case c_cmdPlan:
                    {
                        ResetWaypoint();

                        if (m_pilotType < c_ptPlayer) {
                            //Clear the drilling mask (either it wasn't set or it no longer applies).
                            m_stateM &= ~drillingMaskIGC;

                            m_fractionLastOrder = m_fraction;
                            m_timeRanAway = GetMyLastUpdate();
                            m_bRunningAway = false;
                            m_bGettingAmmo = false;
                            m_targetFirstNotSeen = 0;
                            m_checkCooldown = 0;
                        }
                    }
                    break;

                    case c_cmdAccepted:
                    {
                        if (m_bAutopilot && (m_pilotType < c_ptPlayer))
                        {
                            SetCommand(c_cmdCurrent, target, cid);
                            SetCommand(c_cmdPlan, target, cid);
                        }
                    }
                    break;
                }

                if (pmodelOld)
                {
                    if (pmodelOld->GetObjectType() == OT_buoy)
                        ((IbuoyIGC*)pmodelOld)->ReleaseConsumer();
                    pmodelOld->Release();
                }

                if ((i == c_cmdAccepted || i == c_cmdCurrent) && 
                    (cidOld != cid || pmodelOld != target))
                    GetMyMission()->GetIgcSite()->CommandChangedEvent(i, this, target, cid);
            }
        }

        virtual void                 ExecuteTurretMove(Time          timeStart,
                                                      Time          timeStop,
                                                      Orientation*  pOrientation);

        virtual void                 PreplotShipMove(Time          timeStop);
        virtual void                 PlotShipMove(Time          timeStop);
        virtual void                 ExecuteShipMove(Time          timeStop);

        virtual void                 ExecuteShipMove(Time          timeStart,
                                                     Time          timeStop,
                                                     Vector*       pVelocity,
                                                     Orientation*  pOrientation);
        virtual short               ExportShipLoadout(ShipLoadout*  ploadout)
        {
            if (ploadout)
            {
                ploadout->hullID = m_myHullType.GetObjectID();
                assert (ploadout->hullID != NA);

                ExpandedPartData*   ppd = ploadout->PartData0();
                for (PartLinkIGC*   ppl = m_parts.first(); (ppl != NULL); ppl = ppl->next())
                {
                    IpartIGC*   ppart = ppl->data();

                    ppd->partID              = ppart->GetPartType()->GetObjectID();
                    ppd->mountID             = ppart->GetMountID();
                    ppd->amount              = ppart->GetAmount();
                    (ppd++)->fractionMounted = ppart->GetMountedFraction();
                }
            }
            return sizeof(ShipLoadout) + sizeof(ExpandedPartData) * m_parts.n();
        }

        virtual bool                EquivalentShip(IshipIGC* pship) const
        {
            if (m_myHullType.GetHullType() != pship->GetBaseHullType())
                return false;
            else
            {
                Mount   maxWeapons = m_myHullType.GetMaxWeapons();
                for (Mount i = 0; (i < maxWeapons); i++)
                {
                    IpartIGC*   p1 = m_mountedWeapons[i];
                    IpartIGC*   p2 = pship->GetMountedPart(ET_Weapon, i);

                    if ((p1 != NULL) || (p2 != NULL))
                    {
                        if ((p1 == NULL) || (p2 == NULL) ||
                            (p1->GetPartType() != p2->GetPartType()))
                        {
                            return false;
                        }
                    }
                }

                for (Mount j = 0; (j < ET_MAX); j++)
                {
                    IpartIGC*   p1 = GetMountedPart(j, 0);
                    IpartIGC*   p2 = pship->GetMountedPart(j, 0);

                    if ((p1 != NULL) || (p2 != NULL))
                    {
                        if ((p1 == NULL) || (p2 == NULL) ||
                            (p1->GetPartType() != p2->GetPartType()) ||
                            (p1->GetAmount() != p2->GetAmount()))
                        {
                            return false;
                        }
                    }
                }

                for (Mount k = -1; (k >= -c_maxCargo); k--)
                {
                    IpartIGC*   p1 = m_mountedCargos[k + c_maxCargo];
                    IpartIGC*   p2 = pship->GetMountedPart(NA, k);

                    if ((p1 != NULL) || (p2 != NULL))
                    {
                        if ((p1 == NULL) || (p2 == NULL) ||
                            (p1->GetPartType() != p2->GetPartType()) ||
                            (p1->GetAmount() != p2->GetAmount()))
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }
        virtual void                ProcessShipLoadout(short                cbLoadout,
                                                       const ShipLoadout*   ploadout,
                                                       bool                 bReady)
        {
            //Did the hull change?
            {
                IhullTypeIGC*   pht = m_myHullType.GetHullType();
                if ((pht == NULL) || (ploadout->hullID != pht->GetObjectID()))
                {
                    //Yes ... change it. Trash all the parts first, though
                    {
                        PartLinkIGC*    ppl;
                        while (ppl = m_parts.first())   //Intentional
                            ppl->data()->Terminate();
                    }
                    SetBaseHullType(GetMyMission()->GetHullType(ploadout->hullID));
                }
            }

            const ExpandedPartData* ppartLC     = ploadout->PartData0();
            const ExpandedPartData* ppdLastPart = ploadout->PartDataN(cbLoadout);

            PartLinkIGC*            pplShip = m_parts.first();

            //Walk both lists in parallel, looking for differences
            while (true)
            {
                if (ppartLC != ppdLastPart)
                {
                    //We have a part in the message
                    if (pplShip)
                    {
                        //We have a corresponding part on the ship ... are they the same part?
                        IpartIGC*   ppart = pplShip->data();
            
                        if (ppart->GetPartType()->GetObjectID() == ppartLC->partID)
                        {
                            //Yes
                
                            //has the mount changed?
                            if (ppart->GetMountID() != ppartLC->mountID)
                            {
                                //Is there another part already mounted in the desired slot?
                                //If so, remove it
                                IpartIGC*   ppartMounted = GetMountedPart(ppart->GetEquipmentType(), ppartLC->mountID);
                                if (ppartMounted)
                                    ppartMounted->Terminate();

                                ppart->SetMountID(ppartLC->mountID);
                            }

                            //has the quantity changed (for parts where that matters)?
                            ObjectType  type = ppart->GetObjectType();
                            if (ppartLC->amount != ppart->GetAmount())
                            {
                                ppart->SetAmount(ppartLC->amount);
                            }

                            //In any case ... set the mounted fraction to agree with the fraction specified in the part definition
                            if (bReady)
                                ppart->Arm();
                            else
                                ppart->SetMountedFraction(ppartLC->fractionMounted);

                            //Go to the next part pair.
                            pplShip = pplShip->next();
                            ppartLC++;
                        }
                        else
                        {
                            //No ... delete this part and go to the next part on the ship
                            //while not advancing the ppartLC pointer
                            pplShip = pplShip->next();

                            ppart->Terminate();
                        }
                    }
                    else
                    {
                        //No corresponding part on the ship: create one
                        IpartIGC* ppart = CreateAndAddPart(ppartLC);
                        if (bReady)
                            ppart->Arm();
                        else
                            ppart->SetMountedFraction(ppartLC->fractionMounted);

                        ppartLC++;
                    }
                }
                else if (pplShip)
                {
                    //No part in the message but one on the ship. Nuke it.
                    IpartIGC*   ppart = pplShip->data();

                    //Go to the next part before the nuke
                    pplShip = pplShip->next();

                    ppart->Terminate();
                }
                else
                {
                    //No parts in either the  message or the ship
                    //All done.
                    break;
                }
            }
        }
        virtual bool                PurchaseShipLoadout(short                cbLoadout,
                                                        const ShipLoadout*   ploadout)
        {
            assert (m_station);

            //Did the hull change?
            IhullTypeIGC*   pht = m_myHullType.GetHullType();
            if ((pht == NULL) || (ploadout->hullID != pht->GetObjectID()))
            {
                pht = GetMyMission()->GetHullType(ploadout->hullID);

                if (!(pht && m_station->CanBuy(pht)))
                    return false;               //Can't buy the hull: abort

                //Select the successor of the part.
                pht = (IhullTypeIGC*)(m_station->GetSuccessor(pht));
            }

            //So far so good ... trash the parts
            PartListIGC     partsOld;
            {
                PartLinkIGC*    ppl;
                while (ppl = m_parts.first())   //Intentional
                {
                    IpartIGC*   ppart = ppl->data();
                    ppart->AddRef();
                    ppart->SetShip(NULL, NA);
                    partsOld.last(ppart);
                }
            }
            if (pht != m_myHullType.GetHullType())
                SetBaseHullType(pht);

            const ExpandedPartData* ppdNext     = ploadout->PartData0();
            const ExpandedPartData* ppdLastPart = ploadout->PartDataN(cbLoadout);

            bool    bComplete = true;
            while (ppdNext < ppdLastPart)
            {
                IpartTypeIGC*   ppt = GetMyMission()->GetPartType(ppdNext->partID);
                if (ppt &&
                    m_myHullType.CanMount(ppt, ppdNext->mountID) &&
                    (GetMountedPart(ppt->GetEquipmentType(), ppdNext->mountID) == NULL))
                {
                    //Use an upgraded version if available
                    if (m_station->CanBuy(ppt))
                    {
                        ppt = (IpartTypeIGC*)(m_station->GetSuccessor(ppt));
                        assert (m_myHullType.CanMount(ppt, ppdNext->mountID));

                        IpartIGC*   ppart = CreateAndAddPart(ppt, ppdNext->mountID, ppdNext->amount);
                        ppart->Arm();
                    }
                    else
                    {
                        //Can't buy the part: can we re-use an existing part?
                        for (PartLinkIGC*   pplOld = partsOld.first(); (pplOld != NULL); pplOld = pplOld->next())
                        {
                            if (ppt == pplOld->data()->GetPartType())
                            {
                                IpartIGC*   ppart = pplOld->data();

                                ppart->SetShip(this, ppdNext->mountID);
                                ppart->Arm();

                                ppart->Release();
                                delete pplOld;

                                //If this is an expendable ... scavenge all the old parts to fill it up
                                if (IlauncherTypeIGC::IsLauncherType(ppt->GetEquipmentType()))
                                {
                                    short   maxAmmo = ppdNext->amount;
                                    short   ammo = ppart->GetAmount();
                                    if (ammo < maxAmmo)
                                    {
                                        PartLinkIGC*    pplNext;
                                        for (PartLinkIGC*   pplOther = partsOld.first(); (pplOther != NULL); pplOther = pplNext)
                                        {
                                            pplNext = pplOther->next();

                                            IpartIGC*   ppartOther = pplOther->data();
                                            if (ppartOther->GetPartType() == ppt)
                                            {
                                                short   newAmmo = ammo + ppartOther->GetAmount();
                                                if (newAmmo > maxAmmo)
                                                {
                                                    ppartOther->SetAmount(maxAmmo - newAmmo);
                                                    newAmmo = maxAmmo;
                                                }
                                                else
                                                {
                                                    ppartOther->Terminate();
                                                    ppartOther->Release();

                                                    delete pplOther;
                                                }

                                                ppart->SetAmount(newAmmo);
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                else
                    bComplete = false;

                ppdNext++;
            }

            {
                PartLinkIGC*    ppl;
                while (ppl = partsOld.first())  //Intentional
                {
                    ppl->data()->Terminate();
                    ppl->data()->Release();
                    delete ppl;
                }
            }

            return bComplete;
        }

        virtual void                ExportFractions(CompactShipFractions* pfractions)  const
        {
            pfractions->SetHullFraction(m_fraction);
            {                                                                   
                IshieldIGC* s = (IshieldIGC*)(m_mountedOthers[ET_Shield]);
                pfractions->SetShieldFraction(s ? s->GetFraction() : 0.0f);
            }                                                             
            pfractions->SetFuel(m_myHullType.GetMaxFuel(), m_fuel);  
            pfractions->SetAmmo(m_myHullType.GetMaxAmmo(), m_ammo);   
            pfractions->SetEnergy(m_myHullType.GetMaxEnergy(), m_energy);
			pfractions->SetOre(GetOreCapacity(), m_fOre);  //Xynth #156 7/2010
        }


#define SetSC           pshipupdate->stateM.Set(m_stateM);                  \
                        pshipupdate->controls.Set(m_controls);

#define SetOVTP         pshipupdate->orientation.Set(GetOrientation());                                         \
                        pshipupdate->velocity.Set(GetVelocity());                                               \
                        pshipupdate->turnRates.Set(m_turnRates);                                                \
                        {                                                                                       \
                            IafterburnerIGC* p = (IafterburnerIGC*)(m_mountedOthers[ET_Afterburner]);           \
                            pshipupdate->power = (p ? BytePercentage(p->GetPower()) : BytePercentage(0.0f));    \
                        }

#define SetF            pshipupdate->fractions.SetHullFraction(m_fraction);                             \
                        {                                                                               \
                            IshieldIGC* s = (IshieldIGC*)(m_mountedOthers[ET_Shield]);                  \
                            pshipupdate->fractions.SetShieldFraction(s ? s->GetFraction() : 0.0f);      \
                        }                                                                               \
                        pshipupdate->fractions.SetFuel(m_myHullType.GetMaxFuel(), m_fuel);              \
                        pshipupdate->fractions.SetAmmo(m_myHullType.GetMaxAmmo(), m_ammo);              \
                        pshipupdate->fractions.SetEnergy(m_myHullType.GetMaxEnergy(), m_energy);		\
						pshipupdate->fractions.SetOre(GetOreCapacity(), m_fOre);  //Xynth #156 7/2010

        virtual void                ExportShipUpdate(ServerLightShipUpdate*     pshipupdate) const
        {
            pshipupdate->shipID = m_shipID;

            SetSC;
        }
        virtual void                ExportShipUpdate(Time                       timeReference,
                                                     const Vector&              positionReference,
                                                     ServerHeavyShipUpdate*     pshipupdate) const
        {
            assert (GetPosition().LengthSquared() != 0.0f);
            pshipupdate->shipID = m_shipID;

            pshipupdate->time.Set(timeReference, GetMyLastUpdate());
            pshipupdate->position.Set(positionReference, GetPosition());

            SetSC;

            SetOVTP;

            SetF;
        }

        virtual void                ExportShipUpdate(ClientShipUpdate*     pshipupdate) const
        {
            assert (GetPosition().LengthSquared() != 0.0f);

            //Message generated on the client so convert to server time
            pshipupdate->time = GetMyMission()->GetIgcSite()->ServerTimeFromClientTime(GetMyLastUpdate());
            pshipupdate->position = GetPosition();

            SetSC;

            SetOVTP;

            pshipupdate->orientation.Validate();
        }

        virtual void                ExportShipUpdate(ServerSingleShipUpdate*     pshipupdate) const
        {
            pshipupdate->shipID = m_shipID;

            pshipupdate->time = GetMyLastUpdate();
            pshipupdate->position = GetPosition();

            SetSC;

            SetOVTP;

            SetF;
        }

#undef  SetSC
#undef  SetOVTP
#undef  SetF

        virtual void                ExportShipUpdate(ClientActiveTurretUpdate*  pshipupdate) const
        {
            pshipupdate->time = GetMyMission()->GetIgcSite()->ServerTimeFromClientTime(GetMyLastUpdate());

            pshipupdate->controls.Set(m_controls);
            pshipupdate->orientation.Set(GetOrientation());
        }

        virtual void                ProcessFractions(const CompactShipFractions& fractions);

        virtual ShipUpdateStatus    ProcessShipUpdate(const ServerLightShipUpdate&      shipupdate);
        virtual ShipUpdateStatus    ProcessShipUpdate(Time                              timeReference,
                                                      Vector                            positionReference,
                                                      const ServerHeavyShipUpdate&      shipupdate);

        virtual ShipUpdateStatus    ProcessShipUpdate(const ClientShipUpdate&           shipupdate);
        virtual ShipUpdateStatus    ProcessShipUpdate(const ServerSingleShipUpdate&     shipupdate, bool bOrient = true);

        virtual ShipUpdateStatus    ProcessShipUpdate(Time                              timeReference,
                                                      const ServerActiveTurretUpdate&   shipupdate);
        virtual ShipUpdateStatus    ProcessShipUpdate(const ClientActiveTurretUpdate&   shipupdate);


        virtual void                SetPrivateData(DWORD dwPrivate)
        {
            m_dwPrivate = dwPrivate;
        }

        virtual DWORD               GetPrivateData(void) const
        {
            return m_dwPrivate;
        }

        Mount   HitTreasure(TreasureCode treasureCode, ObjectID objectID, short amount)
        {
            assert (m_myHullType.GetHullType());

            switch (treasureCode)
            {
                case c_tcPart:
                {
                    IpartTypeIGC*   ppt = GetMyMission()->GetPartType(objectID);
                    assert (ppt);

                    //Can we carry the thing?
                    EquipmentType   et = ppt->GetEquipmentType();
                    Mount           maxMounts = (et == ET_Weapon)
                                                ? m_myHullType.GetMaxWeapons()
                                                : 1;

                    //Try to mount the part in each possible location
                    IpartIGC*   ppart;
                    Mount       mount;
                    short       a;

                    for (mount = 0; (mount < maxMounts); mount++)
                    {
                        if (m_myHullType.CanMount(ppt, mount))
                        {
                            ppart = *PartLocation(et, mount);
                            if (ppart == NULL)
                                break;

                            if ((ppart->GetPartType() == ppt) &&
                                IlauncherIGC::IsLauncher(ppart->GetObjectType()))
                            {
                                short   maxAmount = ppt->GetAmount(this);
                                a = ppart->GetAmount();
                                if ((a < maxAmount) && ((a + amount) <= maxAmount))
                                    break;
                            }
                        }
                    }

                    if (mount == maxMounts)
                    {
                        //No place to mount it ... see if there is room in cargo
                        for (mount = -1; (mount >= -c_maxCargo); mount--)
                        {
                            ppart = *PartLocation(NA, mount);
                            if (ppart == NULL)
                                break;

                            if ((ppart->GetPartType() == ppt) &&
                                IlauncherIGC::IsLauncher(ppart->GetObjectType()))
                            {
                                short   maxAmount = ppt->GetAmount(this);
                                a = ppart->GetAmount();
                                if ((a < maxAmount) && ((a + amount) <= maxAmount))
                                    break;
                            }
                        }

                        if (mount < -c_maxCargo)
                        {
                            //No place to put it in cargo
                            GetMyMission()->GetIgcSite()->PlayNotificationSound(salCargoFullSound, this);
                            return c_mountNA;
                        }
                    }

                    GetMyMission()->GetIgcSite()->PlayNotificationSound(pickUpPartSound, this);

                    if (ppart)
                        ppart->SetAmount(a + amount);
                    else
                        CreateAndAddPart(ppt, mount, amount);

                    return mount;
                }
                break;
                case c_tcDevelopment:
                {
                    IbucketIGC*         pb = (IbucketIGC*)GetIbaseIGC((BaseListIGC*)(GetSide()->GetBuckets()), objectID);
                    if (pb && !pb->GetCompleteF())
                    {
                        pb->ForceComplete(GetMyLastUpdate());
                    }
                }
                break;
                case c_tcPowerup:
                {
                    if (objectID & c_pcHull)
                        SetFraction(1.0f);

                    if (objectID & c_pcShield)
                    {
                        IshieldIGC* pshield = (IshieldIGC*)(m_mountedOthers[ET_Shield]);
                        if (pshield)
                            pshield->SetFraction(1.0f);
                    }

                    if (objectID & c_pcEnergy)
                        m_energy = m_myHullType.GetMaxEnergy();

                    if (objectID & c_pcFuel)
                        SetFuel(FLT_MAX);

                    if (objectID & c_pcAmmo)
                        SetAmmo(0x7fff);
                }
                break;
                case c_tcFlag:
                {
                    assert (m_sidFlag == NA);
                    assert (objectID != NA);
                    m_sidFlag = objectID;
                }
                break;
            }

            return c_mountNA;
        }

        virtual ImissileIGC*        GetLastMissileFired(void) const
        {
            return m_pmissileLast;
        }
        virtual void                SetLastMissileFired(ImissileIGC* pmissile)
        {
            if (m_pmissileLast)
                m_pmissileLast->Release();

            m_pmissileLast = pmissile;

            if (pmissile)
                pmissile->AddRef();
        }

		//Imago 6/10 #7
        virtual void                   SetLastTimeLaunched(Time timeLastLaunch)
        {
           m_lastLaunch = timeLastLaunch;
        }
        virtual Time                  GetLastTimeLaunched(void) const
        {
            return m_lastLaunch;
        }
        virtual void                   SetLastTimeDocked(Time timeLastDock)
        {
           m_lastDock = timeLastDock;
        }
        virtual Time                  GetLastTimeDocked(void) const
        {
            return m_lastDock;
        }
		//

        virtual void                Promote(void);

        virtual void                SetParentShip(IshipIGC* pship);
        virtual IshipIGC*           GetParentShip(void) const
        {
            return m_pshipParent;
        }
        virtual Mount               GetTurretID(void) const
        {
            return m_turretID;
        }
        virtual void                SetTurretID(Mount   turretID)
        {
            if (m_turretID != turretID)
            {
                assert (m_pshipParent);

                if (m_turretID != NA)
                {
                    assert (!m_pshipParent->GetHullType()->GetHardpointData(m_turretID).bFixed);

                    IweaponIGC* pw = (IweaponIGC*)(m_pshipParent->GetMountedPart(ET_Weapon, m_turretID));
                    if (pw)
                    {
                        assert (pw->GetGunner() == this);
                        pw->SetGunner(NULL);
                    }
                }

                m_turretID = turretID;

                if (m_turretID != NA)
                {
                    IweaponIGC* pw = (IweaponIGC*)(m_pshipParent->GetMountedPart(ET_Weapon, m_turretID));
                    if (pw)
                    {
                        assert (!m_pshipParent->GetHullType()->GetHardpointData(m_turretID).bFixed);
                        pw->SetGunner(this);
                    }
                }

                GetMyMission()->GetIgcSite()->LoadoutChangeEvent(this, NULL, c_lcTurretChange);
                GetMyMission()->GetIgcSite()->LoadoutChangeEvent(m_pshipParent, NULL, c_lcPassengerMove);
            }
        }

        virtual IshipIGC*           GetGunner(Mount turretID) const
        {
            IweaponIGC* pweapon = (IweaponIGC*)*PartLocation(ET_Weapon, turretID);

            if (pweapon)
            {
                return pweapon->GetGunner();
            }
            else
            {
                // we have to manually walk through the child ships
                for (ShipLinkIGC*   psl = m_shipsChildren.first();
                     (psl != NULL);
                     psl = psl->next())
                {
                    IshipIGC* pshipChild = psl->data();

                    if (pshipChild->GetTurretID() == turretID)
                        return pshipChild;
                }

                return NULL;
            }
        }

        virtual const ShipListIGC*  GetChildShips(void) const
        {
            return &m_shipsChildren;
        }

        virtual void                AddChildShip(IshipIGC* pship)
        {
            assert (pship);
            AddIbaseIGC((BaseListIGC*)&m_shipsChildren, pship);
        }
        virtual void                DeleteChildShip(IshipIGC* pship)
        {
            assert (pship);
            DeleteIbaseIGC((BaseListIGC*)&m_shipsChildren, pship);
        }

        virtual IshipIGC*           GetSourceShip(void)
        {
            return m_pshipParent ? m_pshipParent : this;
        }
        virtual IpartIGC*          CreateAndAddPart(const PartData* ppd)
        {
            return CreateAndAddPart(GetMyMission()->GetPartType(ppd->partID), ppd->mountID, ppd->amount);
        }

        virtual IpartIGC*           CreateAndAddPart(IpartTypeIGC*  ppt, Mount mount, short amount)
        {
            assert (ppt);

            assert (GetMountedPart(ppt->GetEquipmentType(), mount) == NULL);
            assert (m_myHullType.CanMount(ppt, mount));

            IpartIGC*   part = GetMyMission()->CreatePart(GetMyLastUpdate(), ppt);
            assert (part);

			// Xynth -"Fix to avoid crash 8963864" 
			if (part)
			{
				part->SetShip(this, mount);
				assert(part->GetShip() == this);
				assert(part->GetMountID() == mount);

				part->SetAmount(amount);

				part->Release();
			}

            return part;        //Bad form to return after a release but it is not dead since the ship holds a pointer
        }

        virtual WingID              GetWingID(void) const
        {
            return m_wingID;
        }

        virtual void                SetWingID(WingID    wid)
        {
            m_wingID = wid;
        }

        virtual bool                fRipcordActive(void) const 
        {
            return m_pmodelRipcord != NULL;
        }

        virtual float               GetRipcordTimeLeft(void) const
        {
            // assert(fRipcordActive());

            return m_dtRipcordCountdown;
        }
        virtual void                ResetRipcordTimeLeft(void)
        {
            assert (m_myHullType.GetHullType());
            m_dtRipcordCountdown = m_myHullType.GetRipcordSpeed();
        }

        virtual PilotType           GetPilotType(void) const
        {
            return m_pilotType;
        }

        virtual AbilityBitMask      GetOrdersABM(void) const
        {
            return m_abmOrders;
        }

        virtual bool                GetAutopilot(void) const
        {
            return m_bAutopilot;
        }
        
        virtual void                SetAutopilot(bool bAutopilot);

        virtual bool                LegalCommand(CommandID          cid) const
        {
            bool    bLegal = true;
            switch (m_pilotType)
            {
                case c_ptCarrier:
                {
                    bLegal = (cid == c_cidDefault) || (cid == c_cidGoto) || (cid == c_cidJoin) || (cid == c_cidDoNothing);
                }
                break;
                case c_ptMiner:
                {
                    bLegal = (cid == c_cidDefault) || (cid == c_cidGoto) || (cid == c_cidJoin) || (cid == c_cidDoNothing) || (cid == c_cidMine);
                }
                break;
                case c_ptLayer:
                {
                    bLegal = (cid == c_cidDefault) || (cid == c_cidGoto) || (cid == c_cidJoin) || (cid == c_cidDoNothing) || (cid == c_cidBuild);
                }
                break;

                case c_ptBuilder:
                {
                    bLegal = ((m_stateM & (drillingMaskIGC | buildingMaskIGC)) == 0) &&
                             ((cid == c_cidDefault) || (cid == c_cidGoto) || (cid == c_cidJoin) || (cid == c_cidDoNothing) || (cid == c_cidBuild));
                }
                break;

                case c_ptWingman:
                {
					//AEM 7.9.07 Wingman can now be ordered to Repair (no effect if not equipped with nan)
                    bLegal = (cid == c_cidDefault) || (cid == c_cidGoto) || (cid == c_cidAttack) || (cid == c_cidPickup) || (cid == c_cidDoNothing) || (cid == c_cidRepair);
                }
                break;

                case c_ptPlayer:
                case c_ptCheatPlayer:
                {
                    bLegal = (cid >= c_cidDefault) && (cid < c_cidMine);
                }
            }

            return bLegal;
        }

        virtual bool                LegalCommand(CommandID    cid,
                                                 ImodelIGC*   pmodel) const
        {
            bool    bLegal = true;
            if ((pmodel == NULL) || 
                ((m_stateM & buildingMaskIGC) != 0) ||
                (pmodel == (ImodelIGC*)this) ||
                (pmodel->GetMission() != GetMyMission()) ||
                (pmodel->GetObjectType() == OT_buoy &&
                 ((IbuoyIGC*)pmodel)->GetBuoyType() == c_buoyCluster &&
                 ((IbuoyIGC*)pmodel)->GetCluster() == GetCluster()))
            {
                bLegal = (cid == c_cidDoNothing);
            }
            else
            {
                ObjectType  type = pmodel->GetObjectType();
				bool        bFriendly = ((pmodel->GetSide() == GetSide()) || IsideIGC::AlliedSides(pmodel->GetSide(), GetSide())); // #ALLY IMAGO 7/8/09

                switch (cid)
                {
                    case c_cidNone:
                    {
                        bLegal = false;
                    }
                    break;

                    case c_cidDefault:
                    case c_cidGoto:
                    {
                        bLegal = (m_pilotType >= c_ptPlayer) || (type != OT_station) || bFriendly;
                    }
                    break;

                    case c_cidAttack:
                    {
                        bLegal = ((m_pilotType == c_ptWingman) ||
                                  (m_pilotType >= c_ptPlayer)) && (type != OT_warp) && (type != OT_treasure) && !bFriendly;
                        if (bLegal && type == OT_ship) {
                            IshipIGC* s = (IshipIGC*)pmodel;
                            bLegal = (s->GetPilotType() == c_ptMiner || s->GetCluster()); // Don't try to fight docked ships
                        }
                    }
                    break;

                    case c_cidCapture:
                    {
                        bLegal = (m_pilotType >= c_ptPlayer) && (type == OT_station) && !bFriendly;
                    }
                    break;

                    case c_cidDefend:
                    {
                        bLegal = (m_pilotType == c_ptWingman || m_pilotType >= c_ptPlayer);
                    }
                    break;

                    case c_cidPickup:
                    {
                        if (((m_pilotType == c_ptWingman) || (m_pilotType >= c_ptPlayer)) && (m_pshipParent == NULL))
                        {
                            if (type == OT_ship)
                            {
                                IhullTypeIGC*   pht = ((IshipIGC*)pmodel)->GetBaseHullType();
                                bLegal = (pht && m_myHullType.GetHullType() &&
                                          ((pht->HasCapability(c_habmLifepod) && m_myHullType.GetHullType()->HasCapability(c_habmRescue)) ||
                                           (pht->HasCapability(c_habmRescue) && m_myHullType.GetHullType()->HasCapability(c_habmLifepod))));
                            }
                            else if (type != OT_treasure)
                                bLegal = false;
                        }
                        else
                            bLegal = false;
                    }
                    break;

                    case c_cidRepair:
                    {
                        //Can only repair friendly ships or stations
						//AEM 7.9.07 Wingman pilotType now acceptable
                        bLegal = ((m_pilotType == c_ptWingman) || (m_pilotType >= c_ptPlayer)) && ((type == OT_ship) || (type == OT_station)) && bFriendly;
                    }
                    break;

                    case c_cidJoin:
                    {
                        //Can only join other ships
                        bLegal = type == OT_ship;
                    }
                    break;

                    case c_cidBuild:
                    {
                        if (m_pilotType == c_ptLayer)
                            bLegal = (type == OT_buoy) || (type == OT_warp);
                        else if (m_pilotType == c_ptBuilder)
                        {
                            bLegal = (type == OT_asteroid) &&
                                     ((IasteroidIGC*)pmodel)->HasCapability(m_abmOrders);
                        }
                        else
                            bLegal = false;
                    }
                    break;

                    case c_cidMine:
                    {
                        if (m_pilotType == c_ptMiner)
                            bLegal = (type == OT_asteroid) &&
                                     ((IasteroidIGC*)pmodel)->HasCapability(m_abmOrders);
                        else
                            bLegal = false;
                    }
                }
            }

            return bLegal;
        }

        virtual void    Complain(SoundID    sid, const char* pszMsg)
        {
            if (GetMyLastUpdate() >= m_timeLastComplaint + m_dtTimeBetweenComplaints)
            {
                //There is another side here at the asteroid ... complain
                m_timeLastComplaint = GetMyLastUpdate();
                if (m_dtTimeBetweenComplaints < 600.0f)
                    m_dtTimeBetweenComplaints *= random(1.5f, 2.5f);

                GetMyMission()->GetIgcSite()->SendChat(this, CHAT_TEAM, GetSide()->GetObjectID(),
                                                       sid, pszMsg);
            }
        }

        virtual void SetRunawayCheckCooldown(float dtRunAway) {
            m_dtCheckRunaway = dtRunAway;
        }

        virtual IshipIGC*           GetAutoDonate(void) const
        {
            return m_pshipAutoDonate;
        }
        virtual void                SetAutoDonate(IshipIGC* pship)
        {
            assert (pship != this);

            //Anyone donating to me now donates to my new target
            if (pship)
            {
                IsideIGC*   pside = GetSide();
                assert (pside);
                assert (pship->GetSide() == pside);
                assert (pship->GetAutoDonate() == NULL);

                //Was anyone on my ide donating to me ... if so, they start donating to the new person
                for (ShipLinkIGC*   psl = pside->GetShips()->first();
                     (psl != NULL);
                     psl = psl->next())
                {
                    if (psl->data()->GetAutoDonate() == this)
                        psl->data()->SetAutoDonate(pship == psl->data() ? NULL : pship);
                }
            }

            m_pshipAutoDonate = pship;
        }

        virtual short                      GetKills(void) const
        {
            return m_nKills;
        }
        virtual void                       SetKills(short n)
        {
            m_nKills = n;
        }
        virtual void                        AddKill(void)
        {
            m_nKills++;
            GetSide()->AddKill();
        }

        virtual short                       GetDeaths(void) const
        {
            return m_nDeaths;
        }
        virtual void                       SetDeaths(short n)
        {
            m_nDeaths = n;
        }
        virtual void                        AddDeath(void)
        {
            m_nDeaths++;
            GetSide()->AddDeath();
        }

        virtual short                       GetEjections(void) const
        {
            return m_nEjections;
        }
        virtual void                       SetEjections(short n)
        {
            m_nEjections = n;
        }
        virtual void                        AddEjection(void)
        {
            m_nEjections++;
            GetSide()->AddEjection();
        }

        virtual float               GetExperienceMultiplier(void) const
        {
            const float c_maxBonus = 0.5f;
            const float c_halfExperience = 4.0f;

            return 1.0f + c_maxBonus * m_experience / (m_experience + c_halfExperience);
        }
        virtual float               GetExperience(void) const
        {
            return m_experience;
        }
        virtual void                SetExperience(float f)
        {
            m_experience = f;
        }
        virtual void                AddExperience(void)
        {
            m_experience += 1.0f;
        }

        virtual ImodelIGC*          GetRipcordModel(void) const
        {
            return m_pmodelRipcord;
        }
        virtual void                SetRipcordModel(ImodelIGC*  pmodel)
        {
            //Turn off cloak if ripcording
            if (pmodel && (m_stateM & cloakActiveIGC))
                SetStateM(m_stateM & ~cloakActiveIGC);

            if (m_pmodelRipcord && (m_pmodelRipcord->GetObjectType() == OT_ship))
                ((IshipIGC*)(ImodelIGC*)m_pmodelRipcord)->AdjustRipcordDebt(-m_ripcordCost);

            m_pmodelRipcord = pmodel;

            if (m_pmodelRipcord && (m_pmodelRipcord->GetObjectType() == OT_ship))
                ((IshipIGC*)(ImodelIGC*)m_pmodelRipcord)->AdjustRipcordDebt(m_ripcordCost);
        }
        virtual ImodelIGC*          FindRipcordModel(IclusterIGC*   pcluster);

        virtual float               GetRipcordDebt(void) const
        {
            return m_ripcordDebt;
        }
        virtual void                AdjustRipcordDebt(float delta)
        {
            m_ripcordDebt += delta;
        }
		//Xynth #48 8/2010
		virtual void				SetStayDocked(bool stayDock)
		{
			m_stayDocked = stayDock;
		}
		virtual bool				GetStayDocked(void) const
		{
			return m_stayDocked;
		}
        void                        SetGettingAmmo(bool gettingAmmo)
        {
            m_bGettingAmmo = gettingAmmo;
        }
        void                        SetWingmanBehaviour(WingmanBehaviourBitMask wingmanBehaviour)
        {
            m_wingmanBehaviour = wingmanBehaviour;
        }
        WingmanBehaviourBitMask      GetWingmanBehaviour()
        {
            return m_wingmanBehaviour;
        }
        
		virtual void AddRepair(float repair)
		{
			m_repair += repair; //Xynth amount of nanning performed by ship as a fraction of hull repaired
		}

		virtual void MarkPreviouslySpotted(void)
		{
            m_timePreviouslySpotted = GetMyLastUpdate();
		}

		virtual bool RecentlySpotted(void) const
		{
            return (GetMyLastUpdate() - m_timePreviouslySpotted < 50.0f);
		}
		virtual float GetRepair(void) const
		{
			return m_repair;
		}
		virtual void				SetAchievementMask(AchievementMask am)
		{
			m_achievementMask = m_achievementMask | am;
		}
		virtual void				ClearAchievementMask(void)
		{
			m_achievementMask = 0;
		}
		virtual AchievementMask		GetAchievementMask(void) const
		{
			return m_achievementMask;
		}
        virtual bool                OkToLaunch(Time now)
        {
            //Spend 10 seconds docked (MyLastUpdate is not being updated while docked)
            if ((now > GetMyLastUpdate() + 10.0f) && !m_stayDocked) //Xynth #48 8/10 add staydocked bool
            {
                IclusterIGC*    pcluster   = m_station->GetCluster();
                const Vector&   positionMe = m_station->GetPosition();
                if (LegalCommand(m_commandIDs[c_cmdAccepted], m_commandTargets[c_cmdAccepted]) ||
                    PickDefaultOrder(pcluster, positionMe, true))
                {
                    assert (m_station);
                    IsideIGC*   psideMe = GetSide();

                    int     cEnemy = 0;
                    int     cFriend = 0;
                    float   d2Enemy = FLT_MAX;
                    float   d2Friend = FLT_MAX;
                    for (ShipLinkIGC*   psl = m_station->GetCluster()->GetShips()->first(); (psl != NULL); psl = psl->next())
                    {
                        IshipIGC*   pship = psl->data();
                        if ((pship->GetPilotType() >= c_ptPlayer) && (pship->GetParentShip() == NULL) &&
                            !pship->GetBaseHullType()->HasCapability(c_habmLifepod))
                        {
                            IsideIGC*   pside = pship->GetSide();

                            if (pside == psideMe || pside->AlliedSides(psideMe,pside)) //#ALLY - imago 7/3/09
                            {
                                cFriend++;
                                float d2 = (positionMe - pship->GetPosition()).LengthSquared();
                                if (d2 < d2Friend)
                                    d2Friend = d2;
                            }
                            else if (pship->SeenBySide(psideMe))
                            {
                                cEnemy++;
                                float d2 = (positionMe - pship->GetPosition()).LengthSquared();
                                if (d2 < d2Enemy)
                                    d2Enemy = d2;
                            }
                        }
                    }

                    if (cEnemy == 0 || cFriend > cEnemy)
                        return true;
                    else if (cFriend == cEnemy) {
                        static const float  c_d2AlwaysRun = 2300.0f;
                        static const float  c_d2SafeishDist = 3000.0f;
                        if ((d2Enemy > c_d2AlwaysRun * c_d2AlwaysRun) &&
                            ((d2Enemy >= d2Friend) || (d2Enemy > c_d2SafeishDist * c_d2SafeishDist)))
                        {
                            return true;
                        }
                    }
                    else if (cFriend == cEnemy - 1) {
                        static const float  c_d2IgnoreDist = 4000.0f;
                        if (d2Enemy > c_d2IgnoreDist * c_d2IgnoreDist)
                            return true;
                    }
                }

                SetMyLastUpdate(now);
            }

            return false;
        }

        virtual DamageTrack*    GetDamageTrack(void)
        {
            return m_damageTrack;
        }
        virtual void            CreateDamageTrack(void)
        {
            delete m_damageTrack;
            m_damageTrack = GetMyMission()->CreateDamageTrack();
        }
        virtual void            DeleteDamageTrack(void)
        {
            delete m_damageTrack;
            m_damageTrack = NULL;
        }
        virtual void            ResetDamageTrack(void)
        {
            if (m_damageTrack)
                m_damageTrack->Reset();
        }

        virtual void                AddMineDamage(DamageTypeID            type,
                                                  float                   amount,
                                                  Time                    timeCollision,
                                                  ImodelIGC*              pmodelLauncher,
                                                  const Vector&           position1,
                                                  const Vector&           position2)
        {
            if (amount > m_maxMineAmount)
            {
                m_maxMineType = type;
                m_maxMineAmount = amount;
                m_maxMineTime = timeCollision;
                m_maxMineP1 = position1;
                m_maxMineP2 = position2;

                m_maxMineLauncher = pmodelLauncher;

            }
        }
        virtual void                ApplyMineDamage(void)
        {
            if (m_maxMineAmount > 0.0f)
            {
                if (m_maxMineTime > m_timeLastMineExplosion + 0.5f)
                {
                    m_timeLastMineExplosion = m_maxMineTime;
                    GetCluster()->GetClusterSite()->AddExplosion(m_maxMineP2, 1.5f, c_etMine);
                }

                TRef<ImodelIGC> pmodelLauncher = m_maxMineLauncher;
                m_maxMineLauncher = NULL;

                float amount = m_maxMineAmount;
                m_maxMineAmount = 0.0f;

                ReceiveDamage(m_maxMineType, amount, m_maxMineTime, m_maxMineP1, m_maxMineP2, pmodelLauncher);
            }
        }

        virtual WarningMask         GetWarningMask(void) const
        {
            return m_warningMask;
        }
        virtual void                SetWarningMaskBit(WarningMask wm)
        {
            m_warningMask |= wm;
        }
        virtual void                ClearWarningMaskBit(WarningMask wm)
        {
            m_warningMask &= ~wm;
        }

        //Builders ...
        virtual void                SetBaseData(IbaseIGC*  pbase)
        {
            m_pbaseData = pbase;
            if (m_pilotType == c_ptBuilder)
            {
                assert (pbase->GetObjectType() == OT_stationType);
                m_abmOrders = ((IstationTypeIGC*)pbase)->GetBuildAABM();
            }
            else
            {
                assert (m_pilotType == c_ptLayer);
                assert ((pbase->GetObjectType() == OT_mineType) ||
                        (pbase->GetObjectType() == OT_probeType));
            }
        }
        virtual IbaseIGC*           GetBaseData(void) const
        {
            return m_pbaseData;
        }

        virtual CommandID           GetDefaultOrder(ImodelIGC*  pmodel)
        {
            IsideIGC*   psideHim = pmodel->GetSide();
            ObjectType  type = pmodel->GetObjectType();

            CommandID   cid = c_cidGoto;

            switch (m_pilotType)
            {
                case c_ptMiner:
                case c_ptBuilder:
                {
                    if ((psideHim == NULL) &&
                        (type == OT_asteroid) &&
                        ((IasteroidIGC*)pmodel)->HasCapability(m_abmOrders))
                    {
                        cid = (m_pilotType == c_ptMiner) ? c_cidMine : c_cidBuild;
                    }
                }
                break;

                case c_ptLayer:
                {
                    if ((type == OT_buoy) || (type == OT_warp))
                        cid = c_cidBuild;
                }
                break;

                case c_ptWingman:
                case c_ptPlayer:
                case c_ptCheatPlayer:
                {
                    if ((psideHim == GetSide()) || GetSide()->AlliedSides(GetSide(),psideHim)) //ALLY imago 7/9/09
                    {
                        cid = c_cidDefend;
                        if (m_pshipParent == NULL)
                        {
                            if (m_myHullType.GetHullType()) {
                                if (type == OT_ship)
                                {
                                    IhullTypeIGC*   pht = ((IshipIGC*)pmodel)->GetBaseHullType();
                                    if (pht)
                                    {
                                        if ((pht->HasCapability(c_habmLifepod) && m_myHullType.GetHullType()->HasCapability(c_habmRescue)) ||
                                            (pht->HasCapability(c_habmRescue) && m_myHullType.GetHullType()->HasCapability(c_habmLifepod)))
                                            cid = c_cidPickup;
                                        else if (pht->HasCapability(c_habmCarrier) && m_myHullType.GetHullType()->HasCapability(c_habmLandOnCarrier))
                                            cid = c_cidGoto;
                                    }
                                }
                                else if (type == OT_station)
                                {
                                    StationAbilityBitMask   sabm = ((IstationIGC*)pmodel)->GetStationType()->GetCapabilities();
                                    HullAbilityBitMask      habm = m_myHullType.GetCapabilities();
                                    if ((habm & c_habmFighter) == 0)
                                    {
                                        if (sabm & c_sabmCapLand)
                                            cid = c_cidGoto;
                                    }
                                    else if (habm & c_habmLifepod)
                                    {
                                        if (sabm & (c_sabmRescue | c_sabmLand))
                                            cid = c_cidGoto;
                                    }
                                    else if (sabm & c_sabmLand)
                                        cid = c_cidGoto;
                                }
                                else if (type == OT_probe)
                                {
                                    if (m_myHullType.HasCapability(c_habmLifepod) &&
                                        ((IprobeIGC*)pmodel)->GetProbeType()->HasCapability(c_eabmRescue | c_eabmRescueAny))
                                    {
                                        cid = c_cidGoto;
                                    }
                                }
                            }
                        }
                    }
                    else if (type == OT_treasure)
                        cid = c_cidPickup;
                    else if (type == OT_probe)
                    {
                        cid = c_cidAttack;
                        if (m_myHullType.GetHullType() && m_myHullType.HasCapability(c_habmLifepod) &&
                            ((IprobeIGC*)pmodel)->GetProbeType()->HasCapability(c_eabmRescueAny))
                        {
                            cid = c_cidGoto;
                        }
                    }
                    else if (psideHim != NULL)
                        cid = c_cidAttack;
                }
            }

            return cid;
        }

        virtual short               GetLaunchSlot(void)
        {
            m_nextLaunchSlot = (m_nextLaunchSlot + 1) % m_myHullType.GetHullType()->GetLaunchSlots();

            return m_nextLaunchSlot;
        }

        //Miners
        virtual float               GetOre(void) const
        {
            return m_fOre;
        }

        static ImodelIGC* GetLocalStationToHide(IshipIGC* pship) {
            ImodelIGC* pstationToHide = FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest,
                NULL, NULL, NULL, NULL,
                c_sabmLand);
            if (!pstationToHide) {
                CompactShipFractions fractions;
                pship->ExportFractions(&fractions);
                if (fractions.GetShieldFraction() < 0.8f) {
                    pstationToHide = FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest,
                        NULL, NULL, NULL, NULL,
                        c_sabmRipcord);
                    if (pstationToHide) {

                        // check if we need to protect the secret location of the teleport
                        bool knownStation = true;
                        for (SideLinkIGC* l = pship->GetMission()->GetSides()->first(); l != NULL; l = l->next()) {
                            IsideIGC* pside = l->data();
                            if (pside != pship->GetSide() && !pstationToHide->SeenBySide(pside)) {
                                knownStation = false;
                                break;
                            }
                        }
                        if (!knownStation) {
                            DataBuoyIGC buoyData;
                            buoyData.position = pstationToHide->GetPosition() + Vector::RandomDirection()*2000.0f;
                            buoyData.type = c_buoyWaypoint;
                            buoyData.clusterID = pship->GetCluster()->GetObjectID();
                            pstationToHide = (ImodelIGC*)(pship->GetMission()->CreateObject(pship->GetLastUpdate(), OT_buoy, &buoyData, sizeof(buoyData)));
                        }
                    }
                }
            }
            return pstationToHide;
        }

        bool                PickDefaultOrder(IclusterIGC*   pcluster,
                                             const Vector&  position,
                                             bool           bDocked)
        {
            m_dtCheckRunaway = 5.0f; // Be ready to run after making up a command
            
            bool fGaveOrder = false;

            //No orders ... pick something
            switch (m_pilotType)
            {
                case c_ptMiner:
                {
                    // Take cover at a local station, if too damaged
                    if (GetFraction() < 1.0f && !bDocked) {
                        CompactShipFractions fractions;
                        ExportFractions(&fractions);
                        if ((fractions.GetShieldFraction() == 0.0f) ||
                            (fractions.GetShieldFraction() < 0.5f && GetFraction() < 0.7f) ||
                            (GetFraction() < 0.5f))
                        {
                            ImodelIGC* pstationToHide = GetLocalStationToHide(this);

                            if (pstationToHide) {
                                SetCommand(c_cmdPlan, pstationToHide, c_cidGoto);
                                fGaveOrder = true;

                                m_dtCheckRunaway = 90.0f; // we already got a plan for running, don't change it too soon
                            }
                        }
                    }

                    float   capacity = GetMyMission()->GetFloatConstant(c_fcidCapacityHe3) *
                        GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaMiningCapacity);

                    // Handle arriving in a sector after player command
                    if (!fGaveOrder && m_commandIDs[c_cmdQueued] == c_cidJoin && m_commandTargets[c_cmdQueued] && m_commandTargets[c_cmdQueued]->GetCluster() == GetCluster()) {
                        ImodelIGC* phe3Asteroid = FindTarget(this,
                            c_ttNeutral | c_ttAsteroid | c_ttNearest | c_ttLeastTargeted,
                            NULL, pcluster, &position, NULL,
                            m_abmOrders);
                        if (phe3Asteroid && m_fOre < capacity / 2.0f) {
                            SetCommand(c_cmdAccepted, phe3Asteroid, c_cidMine);
                            fGaveOrder = true;
                        }
                        else {
                            ImodelIGC* pstationToUnload = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest,
                                NULL, pcluster, &position, NULL,
                                c_sabmUnload);
                            if (pstationToUnload && m_fOre > 0.0f) {
                                SetCommand(c_cmdAccepted, pstationToUnload, c_cidGoto);
                                fGaveOrder = true;
                            }
                            else if (phe3Asteroid && m_fOre < capacity) {
                                SetCommand(c_cmdAccepted, phe3Asteroid, c_cidMine);
                                fGaveOrder = true;
                            }
                            // else see where default sends us and make sure we don't go back to the previous sector afterwards
                        }
                        m_dtCheckRunaway = 30.0f; // the player wants us to be in here, don't run right away
                    }
                    
                    if (!fGaveOrder && m_fOre < capacity / 2.0f)
                    {
                        ImodelIGC*  pmodel = NULL;

                        // Check if there is an old player command we should be continuing
                        if (!pmodel && m_commandTargets[c_cmdQueued]) {
                            IclusterIGC* pcommandCluster = m_commandTargets[c_cmdQueued]->GetCluster();
                            assert(pcommandCluster);
                            pmodel = FindTarget(this,
                                c_ttNeutral | c_ttAsteroid | c_ttNearest |
                                c_ttLeastTargeted | c_ttCowardly,
                                NULL, pcommandCluster, &Vector(0.0f, 0.0f, 0.0f), NULL,
                                m_abmOrders);
                            if (!pmodel && m_commandIDs[c_cmdQueued] != c_cidJoin) {
                                //Don't remember the cluster after it's been mined out
                                SetCommand(c_cmdQueued, NULL, c_cidNone);
                            }
                        }

                        // Look for something to mine normally
                        if (!pmodel) 
                            pmodel = FindTarget(this,
                                                c_ttNeutral | c_ttAsteroid | c_ttNearest |
                                                c_ttLeastTargeted | c_ttAnyCluster | c_ttCowardly,
                                                NULL, pcluster, &position, NULL,
                                                m_abmOrders);

                        if (pmodel) {
                            // Check if we should unload first
                            if (pmodel->GetCluster() != GetCluster() && m_fOre > capacity * 0.2f) {
                                ImodelIGC* pstationModel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest,
                                    NULL, pcluster, &position, NULL,
                                    c_sabmUnload);
                                if (pstationModel) {
                                    SetCommand(c_cmdAccepted, pstationModel, c_cidGoto);
                                    fGaveOrder = true;
                                }
                            }
                            // Otherwise mine
                            if (!fGaveOrder) {
                                SetCommand(c_cmdAccepted, pmodel, c_cidMine);
                                fGaveOrder = true;
                            }
                        }
                    }

                    if ((!m_commandTargets[c_cmdPlan]) && ((m_fOre > 0.0f) || !bDocked))
                    {
                        ImodelIGC*  pmodel = NULL;

                        if (m_fOre > 0.0f) {
                            pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster | c_ttCowardly,
                                                NULL, pcluster, &position, NULL,
                                                c_sabmUnload);
                            if (!pmodel) //no safe station available
                                pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
                                                    NULL, pcluster, &position, NULL,
                                                    c_sabmUnload);
                        }

                        if (pmodel == NULL)
                            pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster | c_ttCowardly,
                                                NULL, pcluster, &position, NULL,
                                                c_sabmLand);
                        if (!pmodel)
                            pmodel = FindTarget(this, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
                                                NULL, pcluster, &position, NULL,
                                                c_sabmLand);

                        if (pmodel)
                        {
                            SetCommand(c_cmdAccepted, pmodel, c_cidGoto);
                            fGaveOrder = true;
                        }
                    }

                    // Make sure the miner doesn't go straight back into the sector it was sent from
                    if (m_commandIDs[c_cmdQueued] == c_cidJoin && m_commandTargets[c_cmdQueued] && m_commandTargets[c_cmdQueued]->GetCluster() == GetCluster()) { // arriving in a sector after player command
                        if (m_commandTargets[c_cmdPlan] && m_commandTargets[c_cmdPlan]->GetCluster() != GetCluster()) {
                            IwarpIGC* nextWarp = FindPath(this, m_commandTargets[c_cmdPlan], true);
                            if (nextWarp != nullptr && (position - nextWarp->GetPosition()).Length() < 500.0f) {
                                debugf("%s trying to go back to the previous sector. Find an alternative.\n", GetName());

                                ImodelIGC* pstationToHide = GetLocalStationToHide(this);

                                if (pstationToHide) {
                                    SetCommand(c_cmdPlan, pstationToHide, c_cidGoto);
                                    fGaveOrder = true;
                                }
                                else {
                                    // move to the center
                                    DataBuoyIGC         buoyData;
                                    buoyData.position = Vector(0.0f, 0.0f, 0.0f);
                                    buoyData.type = c_buoyWaypoint;
                                    buoyData.clusterID = pcluster->GetObjectID();
                                    ImodelIGC* buoy = (ImodelIGC*)(GetMyMission()->CreateObject(GetMyLastUpdate(), OT_buoy, &buoyData, sizeof(buoyData)));

                                    SetCommand(c_cmdPlan, buoy, c_cidGoto);
                                    fGaveOrder = true;
                                }
                            }
                        }
                    }

                    // Mark as not a fresh player command anymore
                    if (m_commandIDs[c_cmdQueued] != c_cidDefault && m_commandIDs[c_cmdQueued] != c_cidNone)
                        SetCommand(c_cmdQueued, m_commandTargets[c_cmdQueued], c_cidDefault);
                }
                break;

                case c_ptBuilder:
                {
                    if ((m_abmOrders == c_aabmBuildable) && !bDocked)
                        break;

                    ImodelIGC*  pmodel = FindTarget(this,
                                                    c_ttNeutral | c_ttAsteroid | c_ttNearest |
                                                    c_ttLeastTargeted | c_ttAnyCluster | c_ttCowardly,
                                                    NULL, pcluster, &position, NULL,
                                                    m_abmOrders);

                    if (pmodel)
                    {
                        assert (m_pbaseData);

                        CommandID   cid = (m_abmOrders != c_aabmBuildable) ? c_cidBuild : c_cidGoto;

                        SetCommand(c_cmdAccepted, pmodel, cid);
                        fGaveOrder = true;
                        if (cid == c_cidBuild)
                            GetMyMission()->GetIgcSite()->SendChatf(this, CHAT_TEAM, GetSide()->GetObjectID(),
                                                                    droneInTransitSound,
                                                                    "Building %s at %s",
                                                                    ((IstationTypeIGC*)(IbaseIGC*)m_pbaseData)->GetName(),
                                                                    GetModelName(pmodel));
                    }
                }
                break;

                case c_ptLayer:
                {
                    if (!bDocked)
                        break;

                    ImodelIGC*  pmodel = FindTarget(this,
                                                    c_ttNeutral | c_ttAsteroid | c_ttNearest |
                                                    c_ttLeastTargeted | c_ttAnyCluster | c_ttCowardly,
                                                    NULL, pcluster, &position, NULL,
                                                    0);

                    if (pmodel)
                    {
                        SetCommand(c_cmdAccepted, pmodel, c_cidGoto);
                        fGaveOrder = true;
                    }
                }
                break;

                case c_ptWingman:
                {
                    fGaveOrder = GetMyMission()->GetIgcSite()->HandlePickDefaultOrder(this); // for training missions / co-op

                    if (!fGaveOrder) {
                        ImodelIGC*  pmodel = FindTarget(this,
                            c_ttEnemy | c_ttShip | c_ttNearest,
                            NULL, pcluster, &position, NULL, 0);

                        if (pmodel)
                        {
                            SetCommand(c_cmdAccepted, pmodel, c_cidAttack);
                            fGaveOrder = true;
                            GetMyMission()->GetIgcSite()->SendChatf(this, CHAT_TEAM, GetSide()->GetObjectID(),
                                droneInTransitSound,
                                "Attacking %s", GetModelName(pmodel));
                        }
                    }
                }
                break;
            }

            if (!fGaveOrder)
                SetCommand(c_cmdAccepted, NULL, c_cidNone);
              

            return (m_commandIDs[c_cmdAccepted] != c_cidNone);
        }

        bool    IsGhost(void) const
        {
            assert (GetMyMission());
            return m_nDeaths > GetMyMission()->GetMissionParams()->iLives;
        }

        virtual bool                InGarage(IshipIGC*  pship, float tCollision) const;

        float   GetEndurance(void) const
        {
            float   e;

            if (m_myHullType.GetHullType() &&
                m_myHullType.GetHullType()->HasCapability(c_habmLifepod) &&
                GetCluster())
            {
                e = (m_timeToDie - GetLastUpdate()) / GetMyMission()->GetFloatConstant(c_fcidLifepodEndurance);
                if (e < 0.0f)
                    e = 0.0f;
                else if (e > 1.0f)
                    e = 1.0f;
            }
            else
                e = 1.0f;

            return e;
        }

        bool IsUsingAreaOfEffectWeapon(void) const
        {
            if (m_pshipParent != NULL)
            {
                if (m_turretID != NA)
                {
                    IweaponIGC* pweapon = (IweaponIGC*)(m_pshipParent->GetMountedPart(ET_Weapon, m_turretID));

                    if (pweapon && pweapon->GetProjectileType()->GetBlastRadius() > 0.0f)
                        return true;
                }
            }
            else
            {
                IhullTypeIGC* pht = m_myHullType.GetHullType();

                if (pht)
                {
                    for (Mount mount = 0; mount < pht->GetMaxFixedWeapons(); ++mount)
                    {
                        IweaponIGC* pweapon = m_mountedWeapons[mount];

                        if (pweapon && pweapon->GetProjectileType()->GetBlastRadius() > 0.0f)
                            return true;
                    }
                }
            }

            return false;
        }

		//imago 10/14
		virtual void SetSkills(float fShoot, float fTurn, float fGoto) {
			m_fShootSkill = fShoot;
			m_fTurnSkill = fTurn;
			m_gotoplan.SetSkill(fGoto);
		}
		virtual void SetWantBoost(bool bOn) { m_bBoost = bOn; }
		virtual bool GetWantBoost() { return m_bBoost; }

    private:
        bool    bShouldUseRipcord(IclusterIGC*  pcluster);

        void                WarpShip(Time           timeUpdate,
                                     float          deltaT,
                                     Vector*        position,
                                     Vector*        velocity,
                                     Orientation*   orientation);
        void                CalculateShip(Time           timeUpdate,
                                          float          deltaT,
                                          Vector*        position,
                                          Vector*        velocity,
                                          Orientation*   orientation);

        bool    IsSafeToRipcord(void);

        void                ResetWaypoint(void);

        IpartIGC* const*    PartLocation(EquipmentType type, Mount mount) const;

        DWORD               m_dwPrivate; // private data for consumer

        MyHullType          m_myHullType;

        IstationIGC*        m_station;

        TRef<ImodelIGC>     m_pmodelRipcord;

        IshipIGC*           m_pshipParent;
        ShipListIGC         m_shipsChildren;

        PartListIGC         m_parts;
        IweaponIGC*         m_mountedWeapons[c_maxMountedWeapons];
        IpartIGC*           m_mountedOthers[ET_MAX];
        IpartIGC*           m_mountedCargos[c_maxCargo];

        TRef<IclusterIGC>   m_pclusterRequestRipcord;
        DamageTrack*        m_damageTrack;

        TRef<IshipIGC>      m_pshipAutoDonate;

        TRef<IbaseIGC>      m_pbaseData;

        Vector              m_cockpit;
		Vector				m_chaff; 				// TurkeyXIII 11/09 #94

        GotoPlan            m_gotoplan;

        ControlData         m_controls;
        int                 m_stateM;

        float               m_energy;

        float               m_cloaking;

        float               m_turnRates[3];

        float               m_dtRipcordCountdown;

        float               m_fraction;
        float               m_fractionLastOrder;
        float               m_fuel;
        float               m_ripcordDebt;
        float               m_ripcordCost;
        Vector              m_engineVector;

        float               m_dtCheckRunaway;
        Time                m_timeLastComplaint;
        Time                m_timeRanAway;
        Time                m_timeLastMineExplosion;

        Time                m_timeReloadAmmo;
        Time                m_timeReloadFuel;

        Time                m_timeToDie;

        Time                m_timeRequestRipcord;
        float               m_experience;
        float               m_maxMineAmount;
        Time                m_maxMineTime;
        Vector              m_maxMineP1;
        Vector              m_maxMineP2;
        TRef<ImodelIGC>     m_maxMineLauncher;

        DamageTypeID        m_maxMineType;

        short               m_nKills;
        short               m_nDeaths;
        short               m_nEjections;


        short               m_ammo;

        ImissileIGC*        m_pmissileLast;

		Time				m_lastLaunch; //Imago #7 7/10
		Time				m_lastDock; //Imago #7 7/10

        ImodelIGC*          m_commandTargets[c_cmdMax];

        float               m_dtTimeBetweenComplaints;
        float               m_fOre;		
        AbilityBitMask      m_abmOrders;

        WingID              m_wingID;
        CommandID           m_commandIDs[c_cmdMax];

        ShipID              m_shipID;
        SideID              m_sidFlag;

        Mount               m_turretID;

        short               m_nextLaunchSlot;

        PilotType           m_pilotType;
        bool                m_bAutopilot;
        bool                m_bRunningAway;
        WarningMask         m_warningMask;

		bool				m_stayDocked;  //Xynth #48 8/10
		IclusterIGC*		m_miningCluster; //Spunky #268
		bool				m_newMiningCluster; //Spunky #268
		bool				m_doNotBuild; //Spunky #304
										  //imago 10/14
		float				m_fShootSkill;
		float				m_fTurnSkill;
		bool				m_bBoost;

        bool                m_bGettingAmmo; // wingman AI
        char                m_dodgeCooldown; // wingman AI
        char                m_checkCooldown; // wingman AI - defend FindTarget()s
        WingmanBehaviourBitMask m_wingmanBehaviour; // wingman AI
        Time                m_targetFirstNotSeen; // wingman AI
        
		float				m_repair; //Xynth amount of nanning performed by ship
		AchievementMask		m_achievementMask;
		Time				m_timePreviouslySpotted;

};

#endif //__SHIPIGC_H_
