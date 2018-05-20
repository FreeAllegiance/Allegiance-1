/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	probeIGC.cpp
**
**  Author: 
**
**  Description:
**      Implementation of the CprobeIGC class. This file was initially created by
**  the ATL wizard for the core object.
**
**  History:
*/
// probeIGC.cpp : Implementation of CprobeIGC
#include "probeigc.h"

const float c_deceleration = 10.0f;

/////////////////////////////////////////////////////////////////////////////
// CprobeIGC
CprobeIGC::CprobeIGC(void)
:
    m_probeType(NULL),
    m_fraction(1.0f),
    m_projectileType(NULL),
    m_bCreateNow (false)
{
}

CprobeIGC::~CprobeIGC(void)
{
    assert (m_projectileType == NULL);
}

HRESULT CprobeIGC::Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize)
{
    assert (pMission);
    m_pMission = pMission;

    ZRetailAssert (data && (dataSize > sizeof(DataProbeBase)));
    {
        DataProbeBase*  dataProbeBase = (DataProbeBase*)data;

        if ((dataProbeBase->exportF) && 
            ((DataProbeExport*)dataProbeBase)->createNow)
        {
            m_time0 = now;
        }
        else
        {
            m_time0 = pMission->GetIgcSite()->ClientTimeFromServerTime(dataProbeBase->time0);
        }
        TmodelIGC<IprobeIGC>::Initialize(pMission, now, data, dataSize);

        IshipIGC*       pshipLauncher;
        IsideIGC*       pside;
        IclusterIGC*    pcluster;
        if (dataProbeBase->exportF)
        {
            assert (dataSize == sizeof(DataProbeExport));

            DataProbeExport* dataProbeExport = (DataProbeExport*)dataProbeBase;

            m_probeType = (IprobeTypeIGC*)(pMission->GetExpendableType(dataProbeExport->probetypeID));
            pside = pMission->GetSide(dataProbeExport->sideID);
            pcluster = pMission->GetCluster(dataProbeExport->clusterID);

            pshipLauncher = pside->GetShip(dataProbeExport->shipID);

            if (m_probeType->HasCapability(c_eabmShootOnlyTarget))
                m_target = pMission->GetModel(dataProbeExport->otTarget, dataProbeExport->oidTarget);
        }
        else
        {
            assert (dataSize == sizeof(DataProbeIGC));

            DataProbeIGC*    dataProbe = (DataProbeIGC*)dataProbeBase;

            m_probeType = dataProbe->pprobetype;
            pside = dataProbe->pside;
            pcluster = dataProbe->pcluster;

            pshipLauncher = dataProbe->pship;

            if (m_probeType->HasCapability(c_eabmShootOnlyTarget))
                m_target = dataProbe->pmodelTarget;
        }

        assert (m_probeType);
        assert (m_probeType->GetObjectType() == OT_probeType);
        m_probeType->AddRef();

        m_ammo = m_probeType->GetAmmo();

        DataProbeTypeIGC*  dataProbeType = (DataProbeTypeIGC*)(m_probeType->GetData());
        m_projectileType = m_probeType->GetProjectileType();
		if (m_projectileType)
		{
			m_projectileType->AddRef();
			m_bSeenByAll = false;

			if (pshipLauncher && (dataProbeType->launcherDef.price == 0))
				m_launcher = pshipLauncher;
		}
		else
			m_launcher = pshipLauncher; //Xynth would still like to know who deployed the probe

        assert (pcluster);

        //Load the model for the probe
        assert (iswalpha(dataProbeType->modelName[0]));
        HRESULT hr = Load(0, dataProbeType->modelName,
                          dataProbeType->textureName,
                          dataProbeType->iconName, 
                          c_mtDamagable | c_mtHitable | c_mtStatic | c_mtSeenBySide | c_mtPredictable | c_mtScanner);
        assert (SUCCEEDED(hr));

        SetRadius(dataProbeType->radius);
        SetSignature(dataProbeType->signature);
        SetSide(pside);

        SetSecondaryName(dataProbeType->launcherDef.name);

        {
            //Parts get a random orientation
            Vector  v = Vector::RandomDirection();
            Orientation o(v);
            SetOrientation(o);
        }

        //lifespan == 0 => immortal probe that can hit until it gets terminated on the next update; this is bad
        assert (dataProbeType->lifespan > 0.0f);
        m_timeExpire = m_time0 + dataProbeType->lifespan;
        assert (m_timeExpire != m_time0);


        m_nextFire = m_time0 + (m_probeType->HasCapability(c_eabmQuickReady)
                                ? 5.0f        //5 second delay
                                : 30.0f);     //30 second delay before we start to shoot

        assert (GetSide());
        SetMass(0.0f);

        m_probeID = dataProbeBase->probeID;

        SetPosition(dataProbeBase->p0);
        SetCluster(pcluster);

        pMission->AddProbe(this);

        if ((dataProbeType->dtRipcord >= 0.0f) && ((GetMyLastUpdate() - m_time0) >= dataProbeType->dtRipcord))
        {
            pMission->GetIgcSite()->ActivateTeleportProbe(this);
        }
		// mmf 04/08 destroy any probe near aleph (warp) tip as this is viewed as an exploit as the enemy 
		//           often cannot delete it
		// 
		// if (experimental game type) mmf
		const MissionParams* pmp = pMission->GetMissionParams();

		if (pmp->bExperimental) {
			Vector dV, probeV, warpV, tipdistV;
			Vector bV, tipV;
			float warp_rad, distance;
			Orientation warp_orient;
			Rotation warp_rot;
			// loop through list of warps
            for (WarpLinkIGC*   pwl = pcluster->GetWarps()->first(); (pwl != NULL); pwl = pwl->next())
                {
					IwarpIGC*   pwarp = pwl->data();
					warp_rad = pwarp->GetRadius();
					warp_orient = pwarp->GetOrientation();
					warpV = pwarp->GetPosition();
					probeV = this->GetPosition();
					dV= warpV-probeV;
					distance = dV.Length();
					warp_rot = pwarp->GetRotation();

					// if rotating (i.e. rotation is other than 0 0 1 0) abort check
					if (!((warp_rot.x()==0) && (warp_rot.y()==0) && (warp_rot.z()==1) && (warp_rot.angle()==0)))
						break;
					if (distance < (warp_rad * 2) ) {  // only check if close to tip if reasonably close to warp center
						bV = warp_orient.GetBackward();
						tipV = warpV + (bV*warp_rad);
						tipdistV = tipV-probeV;
						distance = tipdistV.Length();
						if (distance < 30) {
							debugf("Destroying probe as it was dropped too close (within 30) of aleph(warp) tip. dist = $f\n",distance);
							return S_FALSE; // this will destroy the probe
						}
					}
				}
		}		
		// mmf end	

		// mmf added code to detect tp drop near asteroid and if too close destroy it
		//     leave ActivateTeleProbe code above so enemy is alerted to the drop even though it may be destroyed
		if (dataProbeType->dtRipcord >= 0.0f)  // check to see if this is a teleprobe
		{
			Vector dV;
			float asteroid_rad, distance;
			// loop through list of asteroids
            for (AsteroidLinkIGC*   pal = pcluster->GetAsteroids()->first(); (pal != NULL); pal = pal->next())
                {
                    asteroid_rad = (pal->data()->GetRadius());
                    dV=this->GetPosition() - pal->data()->GetPosition();
					distance = dV.Length();
					if (distance < (asteroid_rad-5)) {
						debugf("Teleprobe dropped too close to asteroid (within -5) destroying probe. dist = %f, asteroid rad = %f\n",
							distance,asteroid_rad);
						// this->Terminate(); should be terminated when missionigc processes S_FALSE
						return S_FALSE; 
					}
				}
		}		
		// mmf end	
	}

    return S_OK;
}

void    CprobeIGC::Terminate(void)
{
    AddRef();

    DataProbeTypeIGC*  dataProbeType = (DataProbeTypeIGC*)(m_probeType->GetData());

    if (dataProbeType->dtRipcord >= 0.0f)
    {
        GetMyMission()->GetIgcSite()->DestroyTeleportProbe(this);
    }

    if (m_projectileType)
    {
        m_projectileType->Release();
        m_projectileType = NULL;
    }

    m_launcher = NULL;
    m_target = NULL;

    GetCluster()->GetClusterSite()->AddExplosion(GetPosition(), GetRadius(), c_etProbe);

    GetMyMission()->DeleteProbe(this);
	TmodelIGC<IprobeIGC>::Terminate();

    if (m_probeType)
    {
        m_probeType->Release();
        m_probeType = NULL;
    }
    Release();
}

inline void CprobeIGC::ValidTarget(ImodelIGC*  pmodel,
                        IsideIGC*            pside,
                        const Vector&        myPosition,
                        float                dtUpdate,
                        float                accuracy,
                        float                speed,
                        float                lifespan,
                        ObjectType           type,
                        ImodelIGC**          ppmodelMin,
                        float*               pdistance2Min,
                        Vector*              pdirectionMin)
{
    //to something that does not include observers, then the check can be removed.
	if ((pmodel->GetSide() != pside) && !IsideIGC::AlliedSides(pside,pmodel->GetSide())) //#ALLY -was: if (pmodel->GetSide() != pside)  imago fixed 7/8/09
    {
        ModelAttributes ma = pmodel->GetAttributes();

        if ((((ma & c_mtStatic) && pmodel->SeenBySide(pside)) || InScannerRange(pmodel)) &&
            (ma & c_mtDamagable) &&
            ((type != OT_ship) || !((IshipIGC*)pmodel)->GetBaseHullType()->HasCapability(c_habmLifepod)))
        {
            //Is it a contender?
            const Vector&   hisVelocity = pmodel->GetVelocity();
            Vector  dp = pmodel->GetPosition() -
                         myPosition +
                         (hisVelocity * dtUpdate);

            float   distance2 = dp.LengthSquared();
            if (distance2 < *pdistance2Min)
            {
                Vector  direction;
                float   t = solveForImpact(dp,
                                           accuracy * hisVelocity,
                                           speed, pmodel->GetRadius(),
                                           &direction);

                if (t <= lifespan)
                {
                    *ppmodelMin = pmodel;
                    *pdistance2Min = distance2;
                    *pdirectionMin = direction;
                }
            }
        }
    }
}
inline void  CprobeIGC::GetTarget(const ModelListIGC*  models,
                              IsideIGC*            pside,
                              const Vector&        myPosition,
                              float                dtUpdate,
                              float                accuracy,
                              float                speed,
                              float                lifespan,
                              ObjectType           type,
                              ImodelIGC**          ppmodelMin,
                              float*               pdistance2Min,
                              Vector*              pdirectionMin)
{
    for (ModelLinkIGC*   l = models->first(); (l != NULL); l = l->next())
    {
        ImodelIGC*   pmodel = l->data();

        assert (pmodel->GetObjectType() == type);

        ValidTarget(pmodel, 
                    pside,
                    myPosition,
                    dtUpdate,
                    accuracy,
                    speed,
                    lifespan,
                    type,
                    ppmodelMin,
                    pdistance2Min,
                    pdirectionMin);
    }
}
                               
void    CprobeIGC::Update(Time now)
{
    if (now >= m_timeExpire)	
        GetMyMission()->GetIgcSite()->KillProbeEvent(this);
    else
    {
        {
            float   dt = m_probeType->GetRipcordDelay();
            if (dt >= 0.0f)
            {
                Time    timeActivate = m_time0 + dt;
                if ((GetMyLastUpdate() < timeActivate) &&
                    (now >= timeActivate))
                {
                    GetMyMission()->GetIgcSite()->ActivateTeleportProbe(this);
                }
            }
        }

        if (m_projectileType)
        {
            if (m_nextFire < now)
            {
                IclusterIGC*    pcluster = GetCluster();
                assert (pcluster);

                //We'll be able to take a shot
                float   lifespan = GetProjectileLifespan();
                IsideIGC*   pside = GetSide();

                const Vector&       myPosition = GetPosition();

                float   speed = m_projectileType->GetSpeed();
                if (m_ammo != 0)
                    speed *= GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaSpeedAmmo);

                float   accuracy = GetAccuracy();
                float   dtimeBurst = GetDtBurst();
                float   dispersion = m_probeType->GetDispersion();
                Time    lastUpdate = GetMyLastUpdate();
                if (m_nextFire < lastUpdate)
                    m_nextFire = lastUpdate;
                assert (m_nextFire <= now);

                TmodelIGC<IprobeIGC>::Update(now);

                float   dtUpdate = m_nextFire - lastUpdate;

                //If we have a target ... find the closest enemy ship who is a valid target
                ExpendableAbilityBitMask    eabm = m_probeType->GetCapabilities();
                float       distance2Min = speed * lifespan / 1.05f;
                distance2Min *= distance2Min;
                Vector      directionMin;



                ImodelIGC*  pmodelTarget = NULL;
                if (eabm & c_eabmShootOnlyTarget)
                {
                    if (m_target && (m_target->GetCluster() == pcluster))
                    {
                        ObjectType  type = m_target->GetObjectType();
                        ValidTarget((type == OT_ship) ? ((IshipIGC*)(ImodelIGC*)m_target)->GetSourceShip() : m_target,
                                    pside, myPosition, dtUpdate, accuracy, speed, lifespan, type,
                                    &pmodelTarget, &distance2Min, &directionMin);
                    }
                }
                else
                {
                    if (eabm & c_eabmShootStations)
                    {
                        GetTarget((const ModelListIGC*)(pcluster->GetStations()),
                            pside, myPosition, dtUpdate, accuracy, speed, lifespan, OT_station,
                            &pmodelTarget, &distance2Min, &directionMin);
                    }

					bool hasAOE = false;
					if (GetProjectileType() != nullptr)
					{
						hasAOE = (GetProjectileType()->GetBlastRadius() > 1.0f);
						if (hasAOE && !pmodelTarget && (eabm & c_eabmShootMissiles))
						{
							if (pcluster != nullptr && pcluster->GetMissiles() != nullptr)
							{
								GetTarget((const ModelListIGC*)(pcluster->GetMissiles()),
									pside, myPosition, dtUpdate, accuracy, speed, lifespan, OT_missile,
									&pmodelTarget, &distance2Min, &directionMin);
							}
						}
					}

                    if (!pmodelTarget && (eabm & c_eabmShootShips))
                    {
                        GetTarget((const ModelListIGC*)(pcluster->GetShips()),
                                  pside, myPosition, dtUpdate, accuracy, speed, lifespan, OT_ship,
                                  &pmodelTarget, &distance2Min, &directionMin);
                    }

                    if (!hasAOE && !pmodelTarget && (eabm & c_eabmShootMissiles))
                    {
                        GetTarget((const ModelListIGC*)(pcluster->GetMissiles()),
                                  pside, myPosition, dtUpdate, accuracy, speed, lifespan, OT_missile, 
                                  &pmodelTarget, &distance2Min, &directionMin);
                    }
                }

                if (pmodelTarget)
                {
                    if (m_launcher && (m_launcher->GetMission() != GetMyMission()))
                        m_launcher = NULL;

                    //It is going to shoot ... make it visible to everyone in the sector
                    if (!m_bSeenByAll)
                    {
                        m_bSeenByAll = true;
                        for (SideLinkIGC*   psl = m_pMission->GetSides()->first();
                             (psl != NULL);
                             psl = psl->next())
                        {
                            IsideIGC*   psideOther = psl->data();

                            if (!SeenBySide(psideOther))
                            {
                                //Does this side have any scanners in the sector?
                                ClusterSite*    pcs = pcluster->GetClusterSite();
                                const ScannerListIGC*   psl = pcs->GetScanners(psideOther->GetObjectID());
                                if ((psl->n() != 0) || (m_pMission->GetMissionParams()->bAllowAlliedViz && psideOther->AlliedSides(psideOther,pside))) //ALLY 7/3/09 VISIBILITY 7/11/09 imago
                                    SetSideVisibility(psideOther, true);
                                else
                                    m_bSeenByAll = false;
                            }
                        }
                    }

                    //We have a target ... fire along directionMin (modulo dispersion)
                    Orientation o = GetOrientation();
                    o.TurnTo(directionMin);
                    SetOrientation(o);
                    Vector  position = myPosition + m_probeType->GetEmissionPt() * o;

                    DataProjectileIGC   dataProjectile;
                    dataProjectile.projectileTypeID = m_projectileType->GetObjectID();

                    short   nShots = 0;
                    do
                    {
                        //Permute the "forward" direction slightly by a random amount
                        dataProjectile.forward = directionMin;
                        if (dispersion != 0.0f)
                        {
                            float   r = random(0.0f, dispersion);
                            float   a = random(0.0f, 2.0f * pi);
                            dataProjectile.forward += (r * cos(a)) * o.GetRight();
                            dataProjectile.forward += (r * sin(a)) * o.GetUp();

                            dataProjectile.forward.SetNormalize();
                        }

                        //We never move, so skip all the velocity calculations
                        dataProjectile.velocity = speed * dataProjectile.forward;

                        dataProjectile.lifespan = lifespan;

                        IprojectileIGC*  p = (IprojectileIGC*)(m_pMission->CreateObject(m_nextFire, OT_projectile, 
                                                                                        &dataProjectile, sizeof(dataProjectile)));
                        assert (p);
                        {
                            p->SetLauncher(m_launcher ? ((ImodelIGC*)m_launcher) : ((ImodelIGC*)this));
                            p->SetPosition(position);

                            p->SetCluster(pcluster);

                            p->Release();
                        }

                        nShots++;
                        m_nextFire += dtimeBurst;
                    }
                    while (m_nextFire < now);

                    if (m_ammo > 0)
                    {
                        m_ammo -= nShots;
                        if (m_ammo <= 0)
                        {
                            m_ammo = 0;
                            GetMyMission()->GetIgcSite()->KillProbeEvent(this);
                        }
                    }
                }
                else
                {
                    //No shots this cycle
                    m_nextFire = now;
                }
            }
        }

        TmodelIGC<IprobeIGC>::Update(now);
    }
}

int                 CprobeIGC::Export(void*    data) const
{
    if (data)
    {
        DataProbeExport* pdme = (DataProbeExport*)data;

        pdme->p0 = GetPosition();
        pdme->time0 = m_time0;

        pdme->probeID = GetObjectID();
        pdme->exportF = true;

        pdme->clusterID = GetCluster()->GetObjectID();
        pdme->probetypeID = m_probeType->GetObjectID();

        pdme->sideID = GetSide()->GetObjectID();
        pdme->shipID = m_launcher ? m_launcher->GetObjectID() : NA;

        if (m_target)
        {
            pdme->otTarget = m_target->GetObjectType();
            pdme->oidTarget = m_target->GetObjectID();
        }
        else
        {
            pdme->otTarget = NA;
            pdme->oidTarget = NA;
        }

        pdme->createNow = m_bCreateNow;
    }

    return sizeof(DataProbeExport);
}
