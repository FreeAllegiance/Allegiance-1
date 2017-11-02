/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	probeIGC.h
**
**  Author: 
**
**  Description:
**      Header for the CprobeIGC class. This file was initially created by
**  the ATL wizard.
**
**  History:
*/
// probeIGC.h : Declaration of the CprobeIGC

#ifndef __PROBEIGC_H_
#define __PROBEIGC_H_

#include    "modelIGC.h"

class CprobeIGC : public TmodelIGC<IprobeIGC>
{
    public:
        CprobeIGC(void);
        ~CprobeIGC(void);

    public:
    // IbaseIGC
	    virtual HRESULT Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize);
	    virtual void    Terminate(void);
        virtual void    Update(Time now);
        virtual int     Export(void*    data) const;

        virtual ObjectType  GetObjectType(void) const
        {
            return OT_probe;
        }

        virtual ObjectID    GetObjectID(void) const
        {
            return m_probeID;
        }

    // ImodelIGC
        virtual void    SetCluster(IclusterIGC* cluster)
        {
            AddRef();

            {
                IclusterIGC*    c = GetCluster();
                if (c)
                    c->DeleteProbe(this);
            }

            TmodelIGC<IprobeIGC>::SetCluster(cluster);

            if (cluster)
                cluster->AddProbe(this);

            Release();
        }

    // IdamageIGC
        virtual DamageResult        ReceiveDamage(DamageTypeID            type,
                                                  float                   amount,
                                                  Time                    timeCollision,
                                                  const Vector&           position1,
                                                  const Vector&           position2,
                                                  ImodelIGC*              launcher)
        {
            IsideIGC*   pside = GetSide();

            if (launcher &&
                (!GetMyMission()->GetMissionParams()->bAllowFriendlyFire) &&
				((pside == launcher->GetSide()) || IsideIGC::AlliedSides(pside, launcher->GetSide())) && // #ALLY was: pside == launcher->GetSide()
                (amount >= 0.0f) &&
                !m_probeType->HasCapability(c_eabmRescueAny))
            {
				// mmf 04/08 added exception for probes.  Always damage probes.  No more friendly probes blocking base exits and alephs !!!
				// make this experimental for now for testing
				// Only damage 'sensor' probes (ICE projectile type = -1) and not other types of probes like towers

				//Xynth #98 if ( (!(GetMyMission()->GetMissionParams()->bExperimental)) || (m_probeType->GetProjectileType() != NULL) )
				// end mmf
                //Xynth #98 Allow friendly probes to be damaged
				if (m_probeType->GetProjectileType() != NULL) 
					return c_drNoDamage;
            }

            DamageResult    dr = c_drHullDamage;
            float   oldFraction = m_fraction;

            float   maxHP = m_probeType->GetHitPoints();
            amount *= GetMyMission()->GetDamageConstant(type, m_probeType->GetDefenseType());

            if (amount < 0.0f)
            {
                if (maxHP > 0.0f)
                {
                    m_fraction -= amount / maxHP;
                    if (m_fraction > 1.0f)
                        m_fraction = 1.0f;
                    GetThingSite ()->RemoveDamage (m_fraction);
                }
            }
            else if (amount > 0.0f)
            {
                if (launcher && (launcher->GetObjectType() == OT_ship))
                    amount *= ((IshipIGC*)launcher)->GetExperienceMultiplier();

                m_fraction = maxHP > 0.0f ? (m_fraction - amount / maxHP) : 0.0f;
                if (m_fraction <= 0.0f)
                {
                    m_fraction = 0.0f;

                    if (oldFraction > 0.0f)
                    {
						//Xynth Set flag in IGC ship to later trigger an achievement for destroying a probe
						if (launcher != nullptr && launcher->GetObjectType() == OT_ship)
						{
							ObjectID theID = launcher->GetObjectID();
							IsideIGC * plSide = launcher->GetSide();

							if (plSide != nullptr)
							{
								IshipIGC * pShip = plSide->GetShip(theID);

								if (pShip != nullptr && pside != nullptr && !((pside == launcher->GetSide()) || IsideIGC::AlliedSides(pside, launcher->GetSide())))
									pShip->SetAchievementMask(c_achmProbeKill); //Xynth for enemy probe kill achievement
							}
						}
						
                        GetMyMission()->GetIgcSite()->KillProbeEvent(this);
                        dr = c_drKilled;
                    }
                }
                else if ((type & c_dmgidNoDebris) == 0)
                    GetThingSite ()->AddDamage (position2 - position1, m_fraction);
            }

            return dr;
        }
        virtual float   GetFraction(void) const
        {
            return m_fraction;
        }
        virtual void    SetFraction(float newVal)
        {
            m_fraction = newVal;
        }

        virtual float               GetHitPoints(void) const
        {
            return m_fraction * m_probeType->GetHitPoints();
        }

    // IscannerIGC
        bool            InScannerRange(ImodelIGC*   pModel) const
        {
            assert (pModel);

            bool    rc;
            IclusterIGC*    pcluster = GetCluster();
            if (pModel->GetCluster() == pcluster)
            {
                if (pModel->GetFlag() == NA)
                {
                    float   m = m_probeType->GetScannerRange() * pModel->GetSignature() * GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaScanRange);
                    {
                        IsideIGC*   pside = pModel->GetSide();
                        if (pside)
                            m /= pside->GetGlobalAttributeSet().GetAttribute(c_gaSignature);
                    }
                    float   r = GetRadius() + pModel->GetRadius() + m;

                    rc = ((GetPosition() - pModel->GetPosition()).LengthSquared() <= r * r) &&
                         LineOfSightExist(pcluster, this, pModel);
                }
                else
                    rc = true;
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

    //IprobeIGC
        virtual IprobeTypeIGC*       GetProbeType(void) const
        {
            return m_probeType;
        }

        virtual IprojectileTypeIGC*  GetProjectileType(void) const
        {
            return m_projectileType;
        }
        virtual const Vector&        GetEmissionPt(void) const
        {
            return m_probeType->GetEmissionPt();
        }

        virtual float               GetProjectileLifespan(void) const
        {
            float   ls;
            if (m_projectileType)
            {
                ls = m_projectileType->GetLifespan();
                if (m_ammo == 0)
                    ls *= GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaLifespanEnergy);
            }
            else
                ls = 0.0f;

            return ls;
        }
        virtual float               GetLifespan(void) const
        {
            return m_probeType->GetLifespan();
        }

        virtual float               GetDtBurst(void) const
        {
            return m_probeType->GetDtBurst();
        }

        virtual float               GetAccuracy(void) const
        {
            return m_probeType->GetAccuracy();
        }

        virtual bool                GetCanRipcord(float ripcordSpeed) const
        {
            float   dt = m_probeType->GetRipcordDelay();
            return (dt >= 0.0f) &&
                   ((GetMyLastUpdate() - m_time0) >= dt) &&
                   ((GetMyLastUpdate() + (ripcordSpeed + 1.0f)) <= m_timeExpire);
        }

        virtual float               GetRipcordDelay(void) const
        {
            return m_probeType->GetRipcordDelay();
        }

        virtual SoundID              GetAmbientSound(void) const
        {
            return m_probeType->GetAmbientSound();
        }

        virtual void            SetCreateNow (void)
        {
            m_bCreateNow = true;
        }

        virtual float            GetTimeFraction(void) const
        {
            Time    timeNow = GetMyLastUpdate();
            float   f = (m_timeExpire - timeNow) / (m_timeExpire - m_time0);
            if (f < 0.0f)
                f = 0.0f;
            else if (f > 1.0f)
                f = 1.0f;

            return f;
        }
		
		//Xynth - 7/3/10 adding function to set expiration time ticket #10
		virtual void				SetExpiration(Time time)
		{
			m_timeExpire = time;
		}
		//Xynth end new expiration function
		
		virtual IshipIGC * GetProbeLauncherShip() const
		//Xynth new function to identify who deployed the probe
		{
			return m_launcher;
		}


    private:
        void ValidTarget(ImodelIGC*  pmodel,
                         IsideIGC*            pside,
                         const Vector&        myPosition,
                         float                dtUpdate,
                         float                accuracy,
                         float                speed,
                         float                lifespan,
                         ObjectType           type,
                         ImodelIGC**          ppmodelMin,
                         float*               pdistance2Min,
                         Vector*              pdirectionMin);

        void  GetTarget(const ModelListIGC*  models,
                        IsideIGC*            pside,
                        const Vector&        myPosition,
                        float                dtUpdate,
                        float                accuracy,
                        float                speed,
                        float                lifespan,
                        ObjectType           type,
                        ImodelIGC**          ppmodelMin,
                        float*               pdistance2Min,
                        Vector*              pdirectionMin);

        ImissionIGC*        m_pMission;
        IprobeTypeIGC*      m_probeType;
        IprojectileTypeIGC* m_projectileType;

        TRef<IshipIGC>      m_launcher;
        TRef<ImodelIGC>     m_target;

        float               m_fraction;
        Time                m_time0;
        Time                m_timeExpire;

        Time                m_nextFire;

        short               m_ammo;

        ProbeID             m_probeID;
        bool                m_bSeenByAll;
        bool                m_bCreateNow;		
		
};

#endif //__PROBEIGC_H_
