/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:    trekigc.cpp
**
**  Author: 
**
**  Description:
**      This file contains most of the IGC calls from wintrek.
**
**  History:
*/

#include "pch.h"
#include <stddef.h>
#include "training.h"
#include "regkey.h"
#include <zreg.h>
#include "badwords.h"
#include "valuetransform.h"

class ClusterSiteImpl : public ClusterSite
{
    public:
        ClusterSiteImpl(Modeler* pmodeler, Number* ptime, Viewport* pviewport, IclusterIGC* pcluster)
        :
            m_pcluster(pcluster)
        {
            m_pGroupScene = GroupGeo::Create();
            m_pposterImage = CreatePosterImage(pviewport);

            m_pParticleGeo = CreateParticleGeo(pmodeler, ptime);
            m_pGroupScene->AddGeo(m_pParticleGeo);

            m_pbitsGeo = CreateBitsGeo(pmodeler, ptime);
            m_pGroupScene->AddGeo(m_pbitsGeo);

            m_pexplosionGeo = CreateExplosionGeo(GetEngineWindow()->GetTime());
            m_pGroupScene->AddGeo(m_pexplosionGeo);

            m_pPulseGeo = CreatePulseGeo (pmodeler, ptime);
            m_pGroupScene->AddGeo(m_pPulseGeo);

            m_assetmask = 0;
            m_assetmaskLastWarning = 0;
        }

        void Terminate(void)
        {
            //Clear out anything left in the scanners array
            //(it wasn't cleared because the player's side was set to NULL
            //before all objects were removed from the array).
            {
                ScannerLinkIGC* l;
                while (l = m_scanners.first())  //intentional assignment
                {
                    l->data()->Release();
                    delete l;
                }
                while (l = m_enemyScanners.first())  //intentional assignment
                {
                    l->data()->Release();
                    delete l;
                }
            }
            m_pGroupScene   = NULL;
            m_pposterImage  = NULL;
            m_pexplosionGeo = NULL;
        }


        //////////////////////////////////////////////////////////////////////////////
        //
        // Explosions
        //
        //////////////////////////////////////////////////////////////////////////////

        void AddExplosion(
            const Vector& vecPosition,
            const Vector& vecForward,
            const Vector& vecRight,
            const Vector& vecVelocity,
            float scale,
            int   type
        ) {
            //
            // Play a sound if the player can hear it
            //

            IclusterIGC*    pcluster = trekClient.GetCluster();
            if (pcluster &&
                (pcluster->GetClusterSite() == this))
            {
                GenericSoundSource* psource = new GenericSoundSource(vecPosition, vecVelocity);

                if (scale > 100.0f)
                    trekClient.StartSound(largeExplosionSound, psource);
                else if (scale > 10.0f)
                    trekClient.StartSound(mediumExplosionSound, psource);
                else
                    trekClient.StartSound(smallExplosionSound, psource);
            }

            //
            // Create an explosion geo
            //

            TRef<Image> pimageShockWave;
            Color       color(Color::White());
            float       radiusWave      = 0;
            float       radiusExplosion = scale;
            int         count           = 0;

            switch(type) {
                case 1: 
                    count           = 4;
                    radiusWave      = 50;
                    radiusExplosion = 25;
                    pimageShockWave = trekClient.m_pimageShockWave;
                    color           = Color(154.0f / 255.0f, 130.0f / 255.0f, 190.0f / 255.0f);
                    break;

                case 2:
                    count           = 6;
                    radiusExplosion = 1;        //Hack: ignore the scale passed in
                    break;

                case 3:
                    count           = 12;
                    radiusWave      = 70;
                    radiusExplosion = 35;
                    pimageShockWave = trekClient.m_pimageShockWave;
                    color = Color(200.0f / 255.0f, 130.0f / 255.0f, 50.0f / 255.0f);
                    break;

                case 4:
                    count           = 16;
                    radiusWave      = 250;
                    radiusExplosion = 165;
                    pimageShockWave = trekClient.m_pimageShockWave;
                    color = Color(200.0f / 255.0f, 130.0f / 255.0f, 50.0f / 255.0f);
                    break;

                case 5:
                    count           = 16;
                    radiusWave      = 800;
                    radiusExplosion = 100;
                    pimageShockWave = trekClient.m_pimageShockWave;
                    color = Color(220.0f / 255.0f, 130.0f / 255.0f, 50.0f / 255.0f);
                    break;

                case 6:
                    count           = 5;
                    radiusExplosion = 150;
                    break;

                case 7:
                    count           = 24;
                    radiusWave      = 1000;
                    radiusExplosion = 350;
                    pimageShockWave = trekClient.m_pimageShockWave;
                    color = Color(200.0f / 255.0f, 130.0f / 255.0f, 50.0f / 255.0f);
                    break;
            }

            m_pexplosionGeo->AddExplosion(
                vecPosition,
                vecForward,
                vecRight,
                vecVelocity,
                radiusExplosion,
                radiusWave,
                color,
                count,
                trekClient.m_vpimageExplosion[type],
                pimageShockWave
            );
        }

        void AddExplosion(ImodelIGC* pmodel, int type)
        {
            const Orientation& orient = pmodel->GetOrientation();

            float scale = 2 * pmodel->GetThingSite()->GetRadius();
            AddExplosion(
                pmodel->GetPosition(),
                orient.GetForward(),
                orient.GetRight(),
                pmodel->GetVelocity(),
                scale,
                type
            );
        }

        void AddExplosion(
            const Vector& vecPosition,
            float scale,
            int   type
        ) {
            AddExplosion(
                vecPosition,
                Vector(0, 1, 0),
                Vector(1, 0, 0),
                Vector(0, 0, 0),
                scale,
                type
            );
        }

        void AddPulse (float fExplodeTime, const Vector& vecPosition, float fRadius, const Color& color)
        {
            // add a pulse geo to something or other
            //m_pPulseGeo->AddPulse (fExplodeTime, fRadius, vecPosition, color);
        }

        void AddThingSite(ThingSite*    pThingSite)
        {
            TRef<ThingGeo> pthing = pThingSite->GetThingGeo();

            if (pthing)
            {
                assert (pthing->IsValid());

                pthing->SetParticleGeo(m_pParticleGeo);
                pthing->SetBitsGeo(m_pbitsGeo);
            }

            Geo* pgeo = pThingSite->GetGeo();
            if (pgeo)
            {
                assert (pgeo->IsValid());
                m_pGroupScene->AddGeo(pgeo);
            }
        }

        void DeleteThingSite(ThingSite* pThingSite)
        {
            Geo*    pgeo = pThingSite->GetGeo();
            if (pgeo)
                m_pGroupScene->RemoveGeo(pgeo);
        }

        HRESULT AddPoster(const char* textureName, const Vector& vec, float radius)
        {
            TRef<Image> pimage = GetWindow()->GetModeler()->LoadImage(textureName, true);

            if (pimage) {
                m_pposterImage->AddPoster(pimage, vec, radius);
                return S_OK;
            }

            return E_FAIL;
        }

        void SetEnvironmentGeo(const char* pszName)
        {
            TRef<INameSpace> pns = GetModeler()->GetNameSpace(pszName);

            if (pns) {
                m_pgeoEnvironment = Geo::Cast(pns->FindMember("object"));
            }
        }

        Geo* GetEnvironmentGeo()
        {
            if (m_pgeoEnvironment == NULL) {
                return Geo::GetEmpty();
            } else {
                return m_pgeoEnvironment;
            }
        }

        GroupGeo*       GetGroupScene()
        {
            return m_pGroupScene;
        }

        PosterImage* GetPosterImage()
        {
            return m_pposterImage;
        }

        virtual void                    AddScanner(SideID   sid, IscannerIGC* scannerNew)
        {
            assert (sid >= 0);
            assert (sid < c_cSidesMax);
            assert (scannerNew);

            if (sid == trekClient.GetSideID() ||
				trekClient.GetSide()->AlliedSides(trekClient.GetCore()->GetSide(sid),trekClient.GetSide())) //ALLY SCAN 7/13/09 imago
                AddIbaseIGC((BaseListIGC*)&(m_scanners), scannerNew);
            else if (!trekClient.m_fm.IsConnected())
                AddIbaseIGC((BaseListIGC*)&(m_enemyScanners), scannerNew);
        }
        virtual void                    DeleteScanner(SideID   sid, IscannerIGC* scannerOld)
        {
            assert (sid >= 0);
            assert (sid < c_cSidesMax);
            assert (scannerOld);
            if (sid == trekClient.GetSideID() ||
                trekClient.GetSide()->AlliedSides(trekClient.GetCore()->GetSide(sid), trekClient.GetSide())) //ALLY SCAN 7/13/09 imago
                DeleteIbaseIGC((BaseListIGC*)&(m_scanners), scannerOld);
            else if (!trekClient.m_fm.IsConnected())
                DeleteIbaseIGC((BaseListIGC*)&(m_enemyScanners), scannerOld);
                
        }
        virtual const ScannerListIGC*      GetScanners(SideID   sid) const
        {
            assert (sid >= 0);
            assert (sid < c_cSidesMax);

            if (!trekClient.m_fm.IsConnected() && sid != trekClient.GetSide()->GetObjectID()) // Doesn't account for allies
                return &m_enemyScanners;
            return &(m_scanners);
        }

        virtual AssetMask               GetClusterAssetMask(void) { return m_assetmask; }
        virtual void                    SetClusterAssetMask(AssetMask am) 
        {
            m_assetmask=am; 

            // if the game hasn't started yet, don't warn them about events
            if (!trekClient.IsInGame())
                m_assetmaskLastWarning = m_assetmask;
        };

        void UpdateClusterWarnings()
        {
            if (m_assetmaskLastWarning == m_assetmask)
                return; // nothing to do

            bool bInvulnerableStations = trekClient.MyMission()->GetMissionParams().bInvulnerableStations;
            ClusterWarning  cwOld = GetClusterWarning(m_assetmaskLastWarning, bInvulnerableStations);
            ClusterWarning  cwNew = GetClusterWarning(m_assetmask, bInvulnerableStations);

            if ((cwNew != cwOld) && (cwNew > c_cwNoThreat))
            {
                static const SoundID c_vAlertSounds[c_cwMax] = 
                {
                    NA, 
                    NA,
                    NA,
                    NA,
                    NA,
                    minerThreatenedSound, 
                    constructorThreatenedSound, 
                    carrierThreatenedSound, 
                    bomberDetectedSound,
                    carrierDetectedSound, 
                    capitalDetectedSound,
                    teleportDetectedSound,
                    transportDetectedSound,
                    stationThreatenedSound,
                    stationThreatenedSound,
                    stationThreatenedSound
                };

                if ((cwNew >= c_cwMinerThreatened) || (trekClient.GetShip()->GetWingID() == 0))
                    trekClient.PostText(cwNew > cwOld, "%s in %s", GetClusterWarningText(cwNew), m_pcluster->GetName());

                if (c_vAlertSounds[cwNew] != NA)
                    trekClient.PlaySoundEffect(c_vAlertSounds[cwNew]);
            }

            if (trekClient.IsInGame())
            {
                bool bNewSectorSecured = (m_assetmask & c_amStation) 
                    && !(m_assetmask & c_amEnemyStation);
                bool bOldSectorSecured = (m_assetmaskLastWarning & c_amStation)
                    && !(m_assetmaskLastWarning & c_amEnemyStation);
            
                if (bNewSectorSecured && !bOldSectorSecured)
                {
                    trekClient.PostText(false, "%s secured.", m_pcluster->GetName());
                    trekClient.PlaySoundEffect(sectorSecuredSound);
                }
                else if (!(m_assetmask & c_amStation) 
                    && (m_assetmaskLastWarning & c_amStation))
                {
                    trekClient.PostText(false, "%s lost.", m_pcluster->GetName());
                    trekClient.PlaySoundEffect(sectorLostSound);
                }
            }

            m_assetmaskLastWarning = m_assetmask;
        }

        virtual void                MoveShip(void)
        {
            SectorID        sid = m_pcluster->GetObjectID();
            IsideIGC*       pside = trekClient.GetSide();

            AssetMask      am = m_assetmask;
            am &= (c_amStation | c_amEnemyStation | c_amEnemyTeleport);

            for (ShipLinkIGC*    psl = trekClient.m_pCoreIGC->GetShips()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IshipIGC*   pship = psl->data();
                PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());

                if (ppi)
                {
                    const ShipStatus&   ss = ppi->GetShipStatus();
                    if ((!ss.GetUnknown()) && (ss.GetSectorID() == sid) && (ss.GetState() == c_ssFlying))
                    {
                        IhullTypeIGC*   pht = trekClient.m_pCoreIGC->GetHullType(ss.GetHullID());
                        assert (pht);
						am |= GetAssetMask(pship, pht, ( (pside == pship->GetSide()) || IsideIGC::AlliedSides(pside, pship->GetSide()) )); // #ALLY -was: pside == pship->GetSide() IMAGO FIXED 7/8/09
                    }
                }
            }

            SetClusterAssetMask(am);
        }

        virtual void                MoveStation(void)
        {
            IsideIGC*       pside = trekClient.GetSide();

            AssetMask      am = m_assetmask;
            am &= ~(c_amStation | c_amEnemyStation);

            for (StationLinkIGC*    psl = m_pcluster->GetStations()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
				am |= ((psl->data()->GetSide() == pside) || IsideIGC::AlliedSides(psl->data()->GetSide(), pside)) // #ALLY -was: psl->data()->GetSide() == pside
                      ? c_amStation
                      : c_amEnemyStation;
            }

            SetClusterAssetMask(am);
        }
    private:
        TRef<GroupGeo>          m_pGroupScene;
        TRef<PosterImage>       m_pposterImage;
        TRef<ParticleGeo>       m_pParticleGeo;
        TRef<BitsGeo>           m_pbitsGeo;
        TRef<ExplosionGeo>      m_pexplosionGeo;
        TRef<PulseGeo>          m_pPulseGeo;
        ScannerListIGC          m_scanners;
        ScannerListIGC          m_enemyScanners;
        TRef<Geo>               m_pgeoEnvironment;
        AssetMask               m_assetmask;
        AssetMask               m_assetmaskLastWarning;
        IclusterIGC*            m_pcluster;
};


// a sound source attached to model
class ModelPointSoundSource : public ISoundPositionSource
{
private:
    // the model to which this is attached.    
    ImodelIGC* m_pmodel;

    // the position of the sound source on the model
    Vector m_vectPosition;

    // the orientation of the source on the model
    Vector m_vectOrientation;

    bool IsRelative()
    {
        return m_pmodel == trekClient.GetShip()
            && GetWindow()->GetCameraMode() == TrekWindow::cmCockpit;
    }

public:

    // constructs a sound source with the given position and orientation in
    // model space.
    ModelPointSoundSource(ImodelIGC* pmodel, 
            const Vector& vectPosition = Vector(0,0,0), 
            const Vector& vectOrientation = Vector(0,0,-1)) :
        m_pmodel(pmodel), 
        m_vectPosition(vectPosition), 
        m_vectOrientation(vectOrientation)
    {
    }

    // Any playing sounds attached to this source, used when a model is 
    // destroyed or leaves the cluster
    void Destroy()
    {
        m_pmodel = NULL;
    }


    //
    // ISoundPositionSource
    //

    // Gets the position of the sound in space
    virtual HRESULT GetPosition(Vector& vectPosition)
    {
        ZAssert(m_pmodel != NULL);
        if (m_pmodel == NULL) return E_FAIL;

        if (IsRelative())
        {
            vectPosition = m_vectPosition;
        }
        else
        {
            // get the position of the source based on the position and orientation
            // of the model.
            vectPosition = m_pmodel->GetPosition() 
                + m_vectPosition * m_pmodel->GetOrientation();
        }

        return S_OK;
    };
    
    // Gets the velocity of the sound in space
    virtual HRESULT GetVelocity(Vector& vectVelocity)
    {
        ZAssert(m_pmodel != NULL);
        if (m_pmodel == NULL) return E_FAIL;

        if (IsRelative())
            vectVelocity = Vector(0,0,0);
        else
            vectVelocity = m_pmodel->GetVelocity();

        return S_OK;
    };
    
    // Gets the orientation of the sound in space, used for sound cones.
    virtual HRESULT GetOrientation(Vector& vectOrientation)
    {
        ZAssert(m_pmodel != NULL);
        if (m_pmodel == NULL) return E_FAIL;

        if (IsRelative())
            vectOrientation = m_vectOrientation;
        else
            vectOrientation = m_vectOrientation * m_pmodel->GetOrientation();

        return S_OK;
    };

    // Returns S_OK if the position, velocity and orientation reported are 
    // relative to the listener, S_FALSE otherwise.  
    virtual HRESULT IsListenerRelative()
    {
        return IsRelative() ? S_OK : S_FALSE;
    }

    // Returns S_OK if this source is still playing the sound, S_FALSE 
    // otherwise.  This might be false if a sound emitter is destroyed, for 
    // example, in which case the sound might fade out.  Once it returns
    // S_FALSE once, it should never return S_OK again.
    virtual HRESULT IsPlaying()
    {
        return (m_pmodel != NULL) ? S_OK : S_FALSE;
    };
};


// an interface for models which emit sounds
class SoundSite
{
private:
    ImodelIGC* m_pmodel;
    TRef<ModelPointSoundSource> m_psourceCenter;

    class RegisteredModelPointSoundSource : public ModelPointSoundSource
    {
        // the site with which this is registered
        SoundSite* m_pSite;

    public:
        // constructor
        RegisteredModelPointSoundSource(ImodelIGC* pmodel, 
                SoundSite* site,
                const Vector& vectPosition = Vector(0,0,0), 
                const Vector& vectOrientation = Vector(0,0,-1)) :
            ModelPointSoundSource(pmodel, vectPosition, vectOrientation),
            m_pSite(site)
        {
        };

        // destructor
        ~RegisteredModelPointSoundSource()
        {
            if (m_pSite)
                m_pSite->UnregisterSource(this);
        };

        // destroy this sound source, usually due to a model being destroyed 
        // or moved out of the sector.
        void Destroy()
        {
            ModelPointSoundSource::Destroy();
            m_pSite = NULL;
        };
    };

    TList<RegisteredModelPointSoundSource*> m_listRegisteredSources;
    TList<TRef<ISoundInstance> > m_listHitSounds;

public:

    SoundSite(ImodelIGC* pmodel) :
        m_pmodel(pmodel)
    {
    }

    virtual ~SoundSite() 
    {
        StopSounds();
    };

    // updates the current sounds for this object acording to its state.
    virtual void UpdateSounds(DWORD dwElapsedTime) {};

    // adds a hit sound (only keeps the most recent few sounds)
    virtual void AddHitSound(TRef<ISoundInstance> psound)
    {
        const int nMaxHitSounds = 3;
        m_listHitSounds.PushFront(psound);

        if (m_listHitSounds.GetCount() > nMaxHitSounds)
        {
            TRef<ISoundTweakable> ptweak = m_listHitSounds.PopEnd()->GetISoundTweakable();

            if (ptweak)
                ptweak->SetPriority(-10);
        }
    }

    // stops all sounds on this object - usually used when changing clusters.
    virtual void StopSounds()
    {
        // stop the hit sounds
        while (!m_listHitSounds.IsEmpty())
        {
            m_listHitSounds.PopEnd()->Stop();
        }

        // destroy the center source
        if (m_psourceCenter)
        {
            m_psourceCenter->Destroy();
            m_psourceCenter = NULL;
        }

        // destroy any registered sources
        if (!m_listRegisteredSources.IsEmpty())
        {
            TList<RegisteredModelPointSoundSource*>::Iterator iter(m_listRegisteredSources);

            while (!iter.End()) {
                iter.Value()->Destroy();
                iter.Next();
            }
            m_listRegisteredSources.SetEmpty();
        }
    }

    // Gets a sounds source that can be used for this object
    TRef<ISoundPositionSource> GetSoundSource()
    {
        if (!m_psourceCenter)
            m_psourceCenter = new ModelPointSoundSource(m_pmodel);

        return m_psourceCenter;
    }

    // Gets a sounds source that can be used for this object with the given 
    // offset from center.
    TRef<ISoundPositionSource> GetSoundSource(const Vector& vectOffset, 
        const Vector& vectOrientation = Vector(0, 0, -1))
    {
        TRef<RegisteredModelPointSoundSource> psource = 
            new RegisteredModelPointSoundSource(m_pmodel, this, vectOffset);
        m_listRegisteredSources.PushFront(psource);

        return psource;
    }

    // allows a registered source to unregister itself.
    virtual void UnregisterSource(RegisteredModelPointSoundSource* psource)
    {
        ZVerify(m_listRegisteredSources.Remove(psource));
    }
};


// A sound site for a model with one continuous ambient sound
class AmbientSoundSite : public SoundSite
{
private:

    TRef<ISoundInstance> m_pAmbientSound;
    SoundID m_idAmbientSound;

public:

    AmbientSoundSite(ImodelIGC* pmodel, SoundID idAmbientSound) :
        m_idAmbientSound(idAmbientSound),
        SoundSite(pmodel)
    {
    }

    // updates the current sounds for this object acording to its state.
    void UpdateSounds(DWORD dwElapsedTime)
    {
        if (!m_pAmbientSound)
        {
            m_pAmbientSound = trekClient.StartSound(m_idAmbientSound, GetSoundSource());
        }
    };

    // stops all sounds on this object - usually used when changing clusters.
    void StopSounds()
    {
        SoundSite::StopSounds();
        m_pAmbientSound = NULL;
    };
};


// A site for a ship, possibly the player's ship
class ShipSoundSite : public SoundSite
{
private:
    IshipIGC* m_pship;
    Mount m_mountLastTurret;
    
    float m_fHullFraction;
    float m_fAmmoFraction;
    float m_fFuelFraction;
    float m_fEnergyFraction;
    bool  m_bTryingToFireWeaponWithoutAmmo;
    bool  m_bTryingToUseAfterburnersWithoutFuel;
    int   m_nRipcordCountdown;

    TRef<ISoundPositionSource> m_psourceEngine;
    TRef<ISoundPositionSource> m_psounceWeaponMount[c_maxMountedWeapons];

    // sounds used for the player's ship only
    TRef<ISoundInstance> m_pOutOfBoundsSound;
    TRef<ISoundInstance> m_pRipcordingSound;
    TRef<ISoundInstance> m_pMissileToneSound;

    // sounds used inside of a ship
    TRef<ISoundInstance> m_pPilotInteriorSound;
    TRef<ISoundInstance> m_pGunnerInteriorSound;
    TRef<ISoundInstance> m_pMainThrusterInteriorSound;
    TRef<ISoundInstance> m_pTurnThrusterInteriorSound;
    TRef<ISoundInstance> m_pAfterburnerInteriorSound;
	TRef<ISoundInstance> m_pSideThrustInteriorSound;

    // sounds used outside of a ship
    TRef<ISoundInstance> m_pExteriorSound;
    TRef<ISoundInstance> m_pMainThrusterExteriorSound;
    TRef<ISoundInstance> m_pTurnThrusterExteriorSound;
    TRef<ISoundInstance> m_pAfterburnerExteriorSound;
	TRef<ISoundInstance> m_pSideThrustExteriorSound;
    TRef<ISoundInstance> m_pMiningSound;

    // weapon sounds for each mount
    IpartTypeIGC* m_vpCurrentWeaponType[c_maxMountedWeapons];
    TRef<ISoundInstance> m_vpBurstSound[c_maxMountedWeapons];
    TRef<ISoundInstance> m_vpActiveSound[c_maxMountedWeapons];
    TRef<ISoundInstance> m_vpTurretSound[c_maxMountedWeapons];

    // the last state of this sound
    enum { wasSilent, wasPlaying } m_stateLast;
    float m_fThrustSoundLevel;
    float m_fTurnSoundLevel;
    
    // calculate the current fraction of the potential amount of forward thrust
    float ForwardThrustFraction()
    {
        //float fForwardThrust = -1 * m_pship->GetEngineVector() 
        //    * m_pship->GetOrientation().GetBackward();
        //float fForwardThrustFraction = fForwardThrust / m_pship->GetHullType()->GetThrust();
        
        // return fForwardThrustFraction;
        // realistic as this may be, it does not reflect the player's 
        // expectations.  Thus, use the throttle instead. 
        const ControlData&  controls = m_pship->GetControls();
        return std::max(controls.jsValues[c_axisThrottle] * 0.5f + 0.5f, 
            (m_pship->GetStateM() & afterburnerButtonIGC) ? 1.0f : 0.0f);
    }

    // calculate the current fraction of the potential amount of sideways thrust
    float SidewaysThrustFraction()
    {
        const IhullTypeIGC* pht = m_pship->GetHullType();
        float fThrust = m_pship->GetHullType()->GetThrust() 
            / m_pship->GetHullType()->GetSideMultiplier();
        float fForwardThrust = std::max(0.0f, -1 * m_pship->GetEngineVector() 
            * m_pship->GetOrientation().GetBackward());
        Vector vectSideThrust = m_pship->GetEngineVector() + 
            fForwardThrust * m_pship->GetOrientation().GetBackward();

        return vectSideThrust.LengthSquared() / (fThrust * fThrust);
    }

    // calculate a rough and dirty aproximation of the amount of current turning
    float TurnRate(IshipIGC* pship)
    {
        const ControlData&  controls = pship->GetControls();
        return (controls.jsValues[c_axisYaw] * controls.jsValues[c_axisYaw] +
                        controls.jsValues[c_axisPitch] * controls.jsValues[c_axisPitch] +
                        controls.jsValues[c_axisRoll] * controls.jsValues[c_axisRoll]);
    }
    float TurnRate()
    {
        return TurnRate(m_pship);
    }

    // calculate the current afterburner power
    float AfterburnerPower()
    {
        IafterburnerIGC* pafterburner = 
            (IafterburnerIGC*)(m_pship->GetMountedPart(ET_Afterburner, 0));
        return pafterburner ? pafterburner->GetPower() : 0.0f;
    }

    // true if the player is trying to fire an ammo weapon without enough 
    // ammunition, or observing someone doing the same.
    bool TryingToFireWeaponWithoutAmmo()
    {
        // we should only be called for the player's ship
        ZAssert(m_pship == trekClient.GetShip()->GetSourceShip());

        Mount mount = trekClient.GetShip()->GetTurretID();

        // if the player is a turret gunner...
        if (mount != NA)
        {
            // just check that gun
            const IweaponIGC* pweapon;
            CastTo(pweapon, m_pship->GetMountedPart(ET_Weapon, mount));

            return (pweapon && pweapon->fActive() && pweapon->GetMountedFraction() >= 1.0f
                && pweapon->GetAmmoPerShot() > m_pship->GetAmmo());
        }
        else
        {
            // check all of the fixed weapon mounts
            Mount maxFixedWeapons = m_pship->GetHullType()->GetMaxFixedWeapons();
            for (mount = 0; mount < maxFixedWeapons; mount++) 
            {
                const IweaponIGC* pweapon;
                CastTo(pweapon, m_pship->GetMountedPart(ET_Weapon, mount));

                if (pweapon && pweapon->fActive() && pweapon->GetMountedFraction() >= 1.0f
                    && pweapon->GetAmmoPerShot() > m_pship->GetAmmo()) 
                {
                    return true;
                }
            }

            return false;
        }
    }

    bool TryingToFireAfterburnersWithoutFuel()
    {
        IafterburnerIGC* pafterburner = 
            (IafterburnerIGC*)(m_pship->GetMountedPart(ET_Afterburner, 0));
        return pafterburner 
            && (m_pship->GetStateM() & afterburnerButtonIGC)
            && m_pship->GetFuel() <= 0.0f 
            && pafterburner->GetMountedFraction() >= 1.0;;
    }

    bool HasMissileLock()
    {
        // we should only be called for the player's ship
        ZAssert(m_pship == trekClient.GetShip()->GetSourceShip());

        ImagazineIGC* pmagazine = (ImagazineIGC*)trekClient.GetShip()->GetSourceShip()->GetMountedPart(ET_Magazine, 0);

        return (pmagazine && pmagazine->GetLock() == 1.0f);
    }

    // set the volume of all playing internal sounds
    void SetInternalVolume(float fGain)
    {
        if (m_pPilotInteriorSound)
            m_pPilotInteriorSound->GetISoundTweakable()->SetGain(fGain);
        if (m_pGunnerInteriorSound)
            m_pGunnerInteriorSound->GetISoundTweakable()->SetGain(fGain);
        float fThrustVolumeFactor = (1 - m_fThrustSoundLevel);
        fThrustVolumeFactor *= fThrustVolumeFactor;
        fThrustVolumeFactor *= fThrustVolumeFactor;
        fThrustVolumeFactor *= fThrustVolumeFactor;
        fThrustVolumeFactor *= fThrustVolumeFactor;
        fThrustVolumeFactor *= fThrustVolumeFactor;
        fThrustVolumeFactor *= fThrustVolumeFactor;

        if (m_pMainThrusterInteriorSound)
            m_pMainThrusterInteriorSound->GetISoundTweakable()->SetGain(fGain + 
            fThrustVolumeFactor * -60);

        float fTurnThrustVolumeFactor = (1 - m_fTurnSoundLevel);
        fTurnThrustVolumeFactor *= fTurnThrustVolumeFactor;

        if (m_pTurnThrusterInteriorSound)
            m_pTurnThrusterInteriorSound->GetISoundTweakable()->SetGain(fGain + 
                fTurnThrustVolumeFactor * -60);
        if (m_pAfterburnerInteriorSound)
            m_pAfterburnerInteriorSound->GetISoundTweakable()->SetGain(fGain);
    }

    // set the volume of all playing external noise
    void SetExternalVolume(float fGain)
    {
        if (m_pExteriorSound)
            m_pExteriorSound->GetISoundTweakable()->SetGain(fGain);
        if (m_pMainThrusterExteriorSound)
            m_pMainThrusterExteriorSound->GetISoundTweakable()->SetGain(fGain + 
                (1 - m_fThrustSoundLevel) * -40);
        if (m_pTurnThrusterExteriorSound)
            m_pTurnThrusterExteriorSound->GetISoundTweakable()->SetGain(fGain + 
                (1 - m_fTurnSoundLevel) * -40);
        if (m_pAfterburnerExteriorSound)
            m_pAfterburnerExteriorSound->GetISoundTweakable()->SetGain(fGain);
    }

    void PlaySoundIf(TRef<ISoundInstance>& pSound, SoundID id, 
            ISoundPositionSource* psource, bool bPlay)
    {
        if (bPlay)
        {
            // start the sound in question, if it's not already playing
            if (!pSound)
            {
                pSound = trekClient.StartSound(id, psource);
            }
        }
        else
        {
            // stop the sound in questions
            if (pSound)
            {
                pSound->Stop();
                pSound= NULL;
            }
        }
    };

	//Sidethrusters
	bool PlaySidethrust()
	{
		return m_pship->GetStateM() & (backwardButtonIGC | leftButtonIGC | rightButtonIGC);
	}

    void PlayWeaponSounds()
    {
        // check all of the fixed weapon mounts
        Mount maxWeapons = m_pship->GetHullType()->GetMaxWeapons();        
        for (Mount mount = 0; mount < maxWeapons; mount++) 
        {
            const IweaponIGC* pweapon;
            CastTo(pweapon, m_pship->GetMountedPart(ET_Weapon, mount));

            // if the weapon has been changed...
            if (pweapon && pweapon->GetPartType() != m_vpCurrentWeaponType[mount])
            {
                // silence the old weapon, since the sounds may be different 
                // for the new one.
                if (m_vpBurstSound[mount])
                    m_vpBurstSound[mount]->Stop();
                if (m_vpActiveSound[mount])
                    m_vpActiveSound[mount]->Stop();
                
                m_vpBurstSound[mount] = NULL;
                m_vpActiveSound[mount] = NULL;

                m_vpCurrentWeaponType[mount] = pweapon->GetPartType();
            }

            // if the weapon is just being activated, play the relevant sound
            PlaySoundIf(m_vpActiveSound[mount], pweapon ? pweapon->GetActivateSound() : NA, 
                m_psounceWeaponMount[mount], pweapon && pweapon->fActive());

            // if the turret is manned and moving, play the turret sound
            // REVIEW: it looks like we only get turrent move info for other
            // players if the turret is firing!
            PlaySoundIf(m_vpTurretSound[mount], 
                m_pship->GetHullType()->GetHardpointData(mount).turnSound, 
                m_psounceWeaponMount[mount], 
                pweapon && pweapon->GetGunner()
                    && (pweapon->fActive() 
                        || pweapon->GetGunner() == trekClient.GetShip())
                    && TurnRate(pweapon->GetGunner()) > 0.01);

            // if the weapon is burst-firing, play that sound
            bool bWasBurstFiring = m_vpBurstSound[mount] != NULL;
            SoundID idBurstSound = pweapon ? pweapon->GetBurstSound() : NA;
            PlaySoundIf(m_vpBurstSound[mount], idBurstSound, 
                m_psounceWeaponMount[mount], 
                pweapon && pweapon->fFiringBurst() && (idBurstSound != NA));

            // if we shot this frame and we are not playing a burst-fire sound,
            // play a single shot sound.
            if (pweapon && pweapon->fFiringShot() 
                && m_vpBurstSound[mount] == NULL && !bWasBurstFiring)
            {
                trekClient.StartSound(pweapon->GetSingleShotSound(), 
                    m_psounceWeaponMount[mount]);
            }
        }
    }

    void UpdateRipcordCountdown()
    {
        if (!trekClient.GetShip()->GetSourceShip()->fRipcordActive())
        {
            m_nRipcordCountdown = 11;
        }
        else
        {
            int nRipcordCountdown = 1 + (int)trekClient.GetShip()->GetSourceShip()->GetRipcordTimeLeft();

            if (nRipcordCountdown != m_nRipcordCountdown)
            {
                switch (nRipcordCountdown)
                {
                case 10:
                    trekClient.StartSound(ripcord10Sound, GetSoundSource());
                    break;

                case 9:
                    trekClient.StartSound(ripcord9Sound, GetSoundSource());
                    break;

                case 8:
                    trekClient.StartSound(ripcord8Sound, GetSoundSource());
                    break;

                case 7:
                    trekClient.StartSound(ripcord7Sound, GetSoundSource());
                    break;

                case 6:
                    trekClient.StartSound(ripcord6Sound, GetSoundSource());
                    break;

                case 5:
                    trekClient.StartSound(ripcord5Sound, GetSoundSource());
                    break;

                case 4:
                    trekClient.StartSound(ripcord4Sound, GetSoundSource());
                    break;

                case 3:
                    trekClient.StartSound(ripcord3Sound, GetSoundSource());
                    break;

                case 2:
                    trekClient.StartSound(ripcord2Sound, GetSoundSource());
                    break;

                case 1:
                    trekClient.StartSound(ripcord1Sound, GetSoundSource());
                    break;
                }

                m_nRipcordCountdown = nRipcordCountdown;
            }
        }
    }

    // creates the various sound sources for sounds on the ship
    void CreateSoundSources()
    {
        m_psourceEngine = GetSoundSource(
            m_pship->GetThingSite()->GetChildModelOffset("trail"), Vector(0,0,1));

        // create sound sources for each of the weapon mounts
        const IhullTypeIGC* pht = m_pship->GetHullType();
        Mount maxWeapons = pht->GetMaxWeapons();        
        for (Mount mount = 0; mount < maxWeapons; mount++) 
        {
            // review: does not handle turret orientation
            m_psounceWeaponMount[mount] = GetSoundSource(
                pht->GetWeaponPosition(mount), 
                pht->GetWeaponOrientation(mount).GetForward()
                );
        }
    }


    SoundID GetGunnerInteriorSoundID()
    {
        if (trekClient.GetShip()->GetParentShip())
        {
            const IhullTypeIGC* pht = m_pship->GetHullType();
            ZAssert(pht);

            Mount mount = trekClient.GetShip()->GetTurretID();

            if (mount != NA)
            {
                return pht->GetHardpointData(mount).interiorSound;
            }
        }

        return NA;
    }


    SoundID GetAfterburnerSoundID(bool fInterior)
    {
        IafterburnerIGC* pafterburner = 
            (IafterburnerIGC*)(m_pship->GetMountedPart(ET_Afterburner, 0));

        if (pafterburner)
        {
            if (fInterior)
                return pafterburner->GetInteriorSound();
            else
                return pafterburner->GetExteriorSound();
        }
        else
        {
            return NA;
        }
    }

    void UpdateEngineSoundLevels(DWORD dwElapsedTime)
    {
        float fNewThrustSoundLevel = ForwardThrustFraction();
        float fNewTurnSoundLevel = std::min(1.0f, std::max(TurnRate() * 20, SidewaysThrustFraction()));

        // if we were playing this sound a moment ago...
        if (wasSilent != m_stateLast)
        {
            const float cfMaxThrustRateOfChange = 0.0005f;
            const float cfMaxTurnRateOfChange = 0.002f;

            // clip the new sound level according to the max rate of change 
            // allowed.
            fNewThrustSoundLevel = std::max(
                m_fThrustSoundLevel - cfMaxThrustRateOfChange * dwElapsedTime,
                std::min(
                    m_fThrustSoundLevel + cfMaxThrustRateOfChange * dwElapsedTime,
                    fNewThrustSoundLevel
                ));
            fNewTurnSoundLevel = std::max(
                m_fTurnSoundLevel - cfMaxTurnRateOfChange * dwElapsedTime,
                std::min(
                    m_fTurnSoundLevel + cfMaxTurnRateOfChange * dwElapsedTime,
                    fNewTurnSoundLevel
                ));
        }

        m_fThrustSoundLevel = fNewThrustSoundLevel;
        m_fTurnSoundLevel = fNewTurnSoundLevel;

        // adjust the pitch of the engines
        if (m_pMainThrusterInteriorSound)
            m_pMainThrusterInteriorSound->GetISoundTweakable()->SetPitch(
                0.5f + m_fThrustSoundLevel/2);
        if (m_pTurnThrusterInteriorSound)
            m_pTurnThrusterInteriorSound->GetISoundTweakable()->SetPitch(
                0.50f + m_fTurnSoundLevel/3);
        if (m_pMainThrusterExteriorSound)
            m_pMainThrusterExteriorSound->GetISoundTweakable()->SetPitch(
                0.5f + m_fThrustSoundLevel/2);
        if (m_pTurnThrusterExteriorSound)
            m_pTurnThrusterExteriorSound->GetISoundTweakable()->SetPitch(
                0.50f + m_fTurnSoundLevel/2);
     }

public:

    ShipSoundSite(IshipIGC* pship) :
        m_pship(pship),
        SoundSite(pship),
        m_mountLastTurret(NA),
        m_stateLast(wasSilent),
        m_fHullFraction(1.0f),
        m_fAmmoFraction(1.0f),
        m_fFuelFraction(1.0f),
        m_fEnergyFraction(1.0f),
        m_bTryingToFireWeaponWithoutAmmo(false),
        m_bTryingToUseAfterburnersWithoutFuel(false),
        m_nRipcordCountdown(11)
    {
        for (Mount mount = 0; mount < c_maxMountedWeapons; mount++)
        {
            m_vpBurstSound[mount] = NULL;
        };
    }

    // updates the current sounds for this object acording to its state.
    void UpdateSounds(DWORD dwElapsedTime)
    {
        // if this is not a turret or an observer 
        // REVIEW: Assumes that we can never go from in flight to observer/turret
        // (but the reverse works properly, and is needed for eject pods)
        if (!m_pship->GetParentShip()) 
        {
            // make sure we have the sound sources that we need.
            if (!m_psourceEngine)
            {
                CreateSoundSources();
            };


            const IhullTypeIGC* pht = m_pship->GetHullType();
            ZAssert(pht);

            // if this is the player's current ship at the moment
            bool bIsPlayersShip = trekClient.GetShip()->GetSourceShip() == m_pship;
        
            // are we on the inside of this ship?
            bool bIsInterior = bIsPlayersShip 
                && TrekWindow::InternalCamera(GetWindow()->GetCameraMode());

            // make sure we reset the turret interior sound when switching turrets
            if (bIsPlayersShip && m_mountLastTurret != trekClient.GetShip()->GetTurretID())
            {
                if (m_pGunnerInteriorSound)
                {
                    m_pGunnerInteriorSound->Stop();
                    m_pGunnerInteriorSound = NULL;
                };

                m_mountLastTurret = trekClient.GetShip()->GetTurretID();
            }
            bool bIsGunner = bIsPlayersShip && m_mountLastTurret != NA;
        
            float fNewHullFraction = 1.0f;
            float fNewAmmoFraction = 1.0f;
            float fNewFuelFraction = 1.0f;
            float fNewEnergyFraction = 1.0f;
            bool fNewFiringWithoutAmmo = false;
            bool fNewThrustingWithoutFuel = false;

            if (bIsPlayersShip)
            {
                fNewHullFraction = m_pship->GetFraction();
                fNewAmmoFraction = m_pship->GetAmmo() 
                    / (float)m_pship->GetHullType()->GetMaxAmmo();
                if (m_pship->GetHullType()->GetMaxFuel() > 0)
                {
                    fNewFuelFraction = m_pship->GetFuel() 
                        / m_pship->GetHullType()->GetMaxFuel();
                }
                if (m_pship->GetHullType()->GetMaxEnergy() > 0)
                {
                    fNewEnergyFraction = m_pship->GetEnergy() 
                        / m_pship->GetHullType()->GetMaxEnergy();
                }
                fNewFiringWithoutAmmo = TryingToFireWeaponWithoutAmmo();
                fNewThrustingWithoutFuel = TryingToFireAfterburnersWithoutFuel();
            }

            if (bIsPlayersShip && fNewHullFraction < 0.15f && m_fHullFraction >= 0.15f)
                trekClient.StartSound(hullLowSound, GetSoundSource());
            if (bIsPlayersShip && fNewAmmoFraction < 0.15f && m_fAmmoFraction >= 0.15f)
                trekClient.StartSound(ammoLowSound, GetSoundSource());
            if (bIsPlayersShip && fNewFuelFraction < 0.15f && m_fFuelFraction >= 0.15f)
                trekClient.StartSound(fuelLowSound, GetSoundSource());
            if (bIsPlayersShip && fNewEnergyFraction < 0.05f && m_fEnergyFraction >= 0.05f)
                trekClient.StartSound(energyLowSound, GetSoundSource());
            
            if (!m_bTryingToFireWeaponWithoutAmmo && fNewFiringWithoutAmmo)
                trekClient.StartSound(salNoAmmoSound, GetSoundSource());
            if (!m_bTryingToUseAfterburnersWithoutFuel && fNewThrustingWithoutFuel)
                trekClient.StartSound(salNoFuelSound, GetSoundSource());
            UpdateRipcordCountdown();

            m_fHullFraction = fNewHullFraction;
            m_fAmmoFraction = fNewAmmoFraction;
            m_fFuelFraction = fNewFuelFraction;
            m_fEnergyFraction = fNewEnergyFraction;
            m_bTryingToFireWeaponWithoutAmmo = fNewFiringWithoutAmmo;
            m_bTryingToUseAfterburnersWithoutFuel = fNewThrustingWithoutFuel;

            PlaySoundIf(m_pOutOfBoundsSound, outOfBoundsLoopSound, GetSoundSource(),
                (trekClient.GetShip()->GetWarningMask() & (c_wmOutOfBounds | c_wmCrowdedSector)) != 0);
            PlaySoundIf(m_pRipcordingSound, ripcordOnLoopSound, GetSoundSource(), 
                bIsPlayersShip && m_pship->fRipcordActive());
            PlaySoundIf(m_pMissileToneSound, missileToneSound, GetSoundSource(),
                bIsPlayersShip && HasMissileLock());

            // play the mining sound if this ship is actively mining
            PlaySoundIf(m_pMiningSound, miningSound, GetSoundSource(), 
				((m_pship->GetStateM() & miningMaskIGC) != 0) && trekClient.GetShip()->CanSee(m_pship));	// mdvalley: Uneyed miners silent.

            // play the sounds for each weapon
            PlayWeaponSounds();

            // play the interior sounds (perhaps silently) even if we are
            // in an exterior view, because we may need to switch back in 
            // the middle of a sound.  
            if (bIsPlayersShip)
            {
                //
                // play the appropriate interior sounds
                //

                SoundID gunnerInteriorSoundID = GetGunnerInteriorSoundID();

                PlaySoundIf(m_pGunnerInteriorSound, gunnerInteriorSoundID, 
                    GetSoundSource(), bIsGunner);
                PlaySoundIf(m_pPilotInteriorSound, pht->GetInteriorSound(), 
                    GetSoundSource(), !bIsGunner);
                PlaySoundIf(m_pMainThrusterInteriorSound, 
                    pht->GetMainThrusterInteriorSound(),
                    m_psourceEngine, m_fThrustSoundLevel > 0.01f);
                PlaySoundIf(m_pTurnThrusterInteriorSound,  
                    pht->GetManuveringThrusterInteriorSound(), GetSoundSource(), 
                    m_fTurnSoundLevel > 0.01f);
                PlaySoundIf(m_pAfterburnerInteriorSound, 
                    GetAfterburnerSoundID(true), 
                    m_psourceEngine, AfterburnerPower() > 0.0f);
				//Sidethrusters
				if (m_pship->GetHullType()->HasCapability(c_habmFighter))
				{	//if its a small ship, play small ship sound
					PlaySoundIf(m_pSideThrustInteriorSound, SidethrustInteriorSound, GetSoundSource(), PlaySidethrust());
				}
				else { //else play capship sound
					PlaySoundIf(m_pSideThrustInteriorSound, CapSidethrustInteriorSound, GetSoundSource(), PlaySidethrust());
				}
            }

            //
            // play the appropriate exterior sounds
            //

            PlaySoundIf(m_pExteriorSound, pht->GetExteriorSound(), GetSoundSource(), true);
            PlaySoundIf(m_pMainThrusterExteriorSound, 
                pht->GetMainThrusterExteriorSound(), m_psourceEngine, 
                m_fThrustSoundLevel > 0.01f);
            PlaySoundIf(m_pTurnThrusterExteriorSound, 
                pht->GetManuveringThrusterExteriorSound(), GetSoundSource(), 
                m_fTurnSoundLevel > 0.01f);
            PlaySoundIf(m_pAfterburnerExteriorSound,                
                GetAfterburnerSoundID(false), 
                m_psourceEngine,
                AfterburnerPower() > 0.0f);
			//Sidethrusters
			if (m_pship->GetHullType()->HasCapability(c_habmFighter))
			{	//if its a small ship, play small ship sound
				PlaySoundIf(m_pSideThrustInteriorSound, SidethrustExteriorSound, GetSoundSource(), PlaySidethrust());
			}
			else { //else play capship sound
				PlaySoundIf(m_pSideThrustInteriorSound, CapSidethrustExteriorSound, GetSoundSource(), PlaySidethrust());
			}
            UpdateEngineSoundLevels(dwElapsedTime);

            // if we are inside of the ship...
            if (bIsInterior)
            {
                // switch from external noise to internal noise
                SetExternalVolume(-100);
                SetInternalVolume(0);
            }
            else 
            {
                // we are outside of the ship

                // switch from internal noise to external noise
                SetExternalVolume(0);
                SetInternalVolume(-100);
            }

            m_stateLast = wasPlaying;
        }
    };

    // stops all sounds on this object - usually used when changing clusters.
    void StopSounds()
    {
        SoundSite::StopSounds();

        if (m_psourceEngine)
        {
            m_psourceEngine = NULL;

            m_pOutOfBoundsSound = NULL;
            m_pRipcordingSound = NULL;

            m_pPilotInteriorSound = NULL;
            m_pGunnerInteriorSound = NULL;
            m_pMainThrusterInteriorSound = NULL;
            m_pTurnThrusterInteriorSound = NULL;
            m_pAfterburnerInteriorSound = NULL;
    
            m_pExteriorSound = NULL;
            m_pMainThrusterExteriorSound = NULL;
            m_pTurnThrusterExteriorSound = NULL;
            m_pAfterburnerExteriorSound = NULL;

            Mount maxWeapons = m_pship->GetHullType()->GetMaxWeapons();
            for (Mount mount = 0; mount < maxWeapons; mount++) 
            {
                m_psounceWeaponMount[mount] = NULL;

                m_vpBurstSound[mount] = NULL;
                m_vpActiveSound[mount] = NULL;
                m_vpTurretSound[mount] = NULL;
            }

            m_stateLast = wasSilent;
        }
    };
};

class ThingSiteImpl : public ThingSitePrivate
{
    public:
        ThingSiteImpl(ImodelIGC*    pmodel)
        :
            m_bSideVisibility(false),
            m_pmodel(pmodel),
            m_mask(0),
            m_bPlayedHitSoundThisFrame(false)            
        {
            assert (pmodel);
            //Don't bother to AddRef pmodel -- it lifespan always exceeds that of the thingsite.

            // create the appropriate sound site for this
            switch (pmodel->GetObjectType())
            {
            case OT_asteroid:
                m_pSoundSite = new AmbientSoundSite(pmodel, asteroidSound);
                break;

            case OT_warp:
                m_pSoundSite = new AmbientSoundSite(pmodel, alephSound);
                break;

            case OT_buildingEffect:
                m_pSoundSite = new AmbientSoundSite(pmodel, buildSound);
                break;

            case OT_probe:
                m_pSoundSite = new AmbientSoundSite(pmodel, 
                    ((IprobeIGC*)pmodel)->GetAmbientSound());
                break;

            case OT_missile:
                m_pSoundSite = new AmbientSoundSite(pmodel, 
                    ((ImissileIGC*)pmodel)->GetMissileType()->GetAmbientSound());
                break;

            case OT_projectile:
                {
                    SoundID sound = ((IprojectileIGC*)pmodel)
                        ->GetProjectileType()->GetAmbientSound();
                    if (sound = NA)
                        m_pSoundSite = NULL;
                    else
                        m_pSoundSite = new AmbientSoundSite(pmodel, sound);
                }
                break;

            case OT_station:
                m_pSoundSite = new AmbientSoundSite(pmodel, 
                    ((IstationIGC*)pmodel)->GetExteriorSound());
                break;

            case OT_ship:
                m_pSoundSite = new ShipSoundSite((IshipIGC*)pmodel);
                break;

            default:
                m_pSoundSite = NULL;
                break;
            }
        }

        ~ThingSiteImpl(void)
        {
            if (m_pSoundSite)
                delete m_pSoundSite;
        }

        void Purge(void)
        {
            if (m_pthing) {
                assert (!m_pdecal);
                m_pthingGeo->SetEmpty();
                m_pthingGeo = NULL;
                m_pthing = NULL;
            }
            else if (m_pdecal) {
                m_pdecal->SetEmpty();
                m_pdecal = NULL;
            }
        }

        void Terminate(void)
        {
            if (m_pdecal)
            {
                m_pdecal->SetEmpty();
                m_pdecal = NULL;

                assert(m_pvisibleGeoBolt == NULL);
            }
            else
            {
                if (m_pthing)
                {
                    assert (!m_pdecal);
                    m_pthingGeo->SetEmpty();
                    m_pthingGeo = NULL;
                    m_pthing = NULL;


                    if (m_pvisibleGeoBolt)
                    {
                        m_pvisibleGeoBolt->SetEmpty();
                        m_pvisibleGeoBolt = NULL;
                    }
                }
                else if (m_pbuildeffect)
                {
                    m_pbuildeffect->SetEmpty();
                    m_pbuildeffect = NULL;
                }
            }
        }

        Vector GetChildModelOffset(const ZString& strFrame)
        {
            //m_pthing->Update();
            Vector vec(0, 0, 0);
            if (!m_pthing->GetChildModelOffset(strFrame, vec))
                return Vector(0,0,0);
            return vec;
        }

        Vector GetChildOffset(const ZString& strFrame)
        {
            //m_pthing->Update();
            Vector vec(0, 0, 0);
            ZVerify(m_pthing->GetChildOffset(strFrame, vec));
            return vec;
        }

        void SetAfterburnerSmokeSize (float size)
        {
            m_pthing->SetAfterburnerSmokeSize (size);
        }

        void    SetAfterburnerFireDuration (float duration)
        {
            m_pthing->SetAfterburnerFireDuration (duration);
        }

        void    SetAfterburnerSmokeDuration (float duration)
        {
            m_pthing->SetAfterburnerSmokeDuration (duration);
        }

        void    AddDamage (const Vector& vecDamagePosition, float fDamageFraction)
        {
            m_pthing->AddDamage (vecDamagePosition, fDamageFraction);
        }

        void    RemoveDamage (float fDamageFraction)
        {
            m_pthing->RemoveDamage (fDamageFraction);
        }

        void    SetTimeUntilRipcord (float fTimeUntilTeleport)
        {
            m_pthing->SetTimeUntilRipcord (fTimeUntilTeleport);
        }

        void    SetTimeUntilAleph (float fTimeUntilTeleport)
        {
            m_pthing->SetTimeUntilAleph (fTimeUntilTeleport);
        }

        void AddExplosion(const Vector& vecPosition, float scale, int type)
        {
            IclusterIGC* pcluster = m_pmodel->GetCluster();

            pcluster->GetClusterSite()->AddExplosion(
                m_pmodel->GetPosition() + vecPosition * m_pmodel->GetOrientation(),
                scale, type);
        }

        void AddPulse (float fExplodeTime, const Vector& vecPosition, float fRadius, const Color& color)
        {
            if ((m_pmodel->GetObjectType()) == OT_warp)
            {
                // tell the alephgeo about the impending bomb
                m_peventSourceAleph->Trigger(fExplodeTime);
            }
        }

        void AddHullHit(const Vector& vecPosition, const Vector& vecNormal)
        {
            if (m_pthing)
                m_pthing->AddHullHit(vecPosition, vecNormal);
        }

        void SetAfterburnerThrust(const Vector& thrust, float power)
        {
            if (m_pthing) {
                m_pthing->SetAfterburnerThrust(thrust, power);
            } else {
                assert (m_pdecal);
            }
        }

        void AddMuzzleFlare(const Vector& vecEmissionPoint, float duration)
        {
            if (this == trekClient.GetShip()->GetThingSite())
                GetWindow()->AddMuzzleFlare(vecEmissionPoint, duration);
        }

        void AddFlare(Time timeArg, const Vector& vecPosition, int id,
                      const Vector* ellipseEquation)
        {
            if (m_pthing->GetFlareCount() < 4.0f)
            {
                TRef<Number> ptimeArg = GetEngineWindow()->GetTime();
                TRef<Number> ptime = NumberTransform::Subtract(ptimeArg, ptimeArg->MakeConstant());

                m_pthing->AddFlare(
                    new TextureGeo(
                        trekClient.m_pgeoFlares[id],
                        new AnimatedImage(
                            ptime,
                            trekClient.m_pimageFlare
                        )
                    ),
                    ptime,
                    -vecPosition,       //NYI we can avoid the - by changing the Geo
                    ellipseEquation
                );
            }
        }

        void SetVisible(unsigned char render)
        {
            if (m_pthing)
            {
                bool    vship = render == c_ucRenderAll;

                m_pthing->SetVisible(render >= c_ucRenderTrail);
                m_pthing->SetVisibleShip(vship);

                if (m_pvisibleGeoBolt) {
                    m_pvisibleGeoBolt->SetVisible(vship);
                }
            }
        }

        ThingGeo* GetThingGeo()
        {
            return m_pthing;
        }

        Geo* GetGeo()
        {
            if (m_pthingGeo) {
                return m_pthingGeo;
            } else {
                if (m_pdecal) {
                    return m_pdecal;
                } else {
                    return m_pbuildeffect;
                }
            }
        }

        void SetPosition(const Vector& position)
        {
            if (m_pthing) {
                m_pthing->SetPosition(position);
            } else  if (m_pdecal) {
                m_pdecal->SetPosition(position);
            }
            else if (m_pbuildeffect)
                m_pbuildeffect->SetPosition(position);
        }

        float GetRadius(void)
        {
            if (m_pthing)
            {
                return m_pthing->GetRadius();
            }
            else if (m_pdecal)
            {
                return sqrt2;
            }
            else if (m_pbuildeffect)
                return m_pbuildeffect->GetRadius();
            else
                return 0.0f;
        }

        void SetRadius(float r)
        {
            m_radius = r;

            if (m_pdecal)
            {
                if (m_pdecal->GetForward().IsZero())
                    m_pdecal->SetScale(r / sqrt2);
                else
                    m_pdecal->SetForward(m_pdecal->GetForward().Normalize() * m_radius);
            }
            else if (m_pthing)
            {
                assert (m_pthing);
                //m_pthing->Update();
                m_pthing->SetRadius(r);
            }
            else if (m_pbuildeffect)
            {
                m_pbuildeffect->SetRadius(r);
            }
        }

        void SetColors(float aInner, float fInner, float fOuter)
        {
            assert (m_pbuildeffect);
            m_pbuildeffect->SetColors(aInner, fInner, fOuter);
        }

        void SetOrientation(const Orientation& orientation)
        {
            if (m_pthing)
                m_pthing->SetOrientation(orientation);
            else if (m_pdecal)
            {
                if (!m_pdecal->GetForward().IsZero())
                    m_pdecal->SetForward(orientation.GetBackward() * -m_radius);
            }
        }

        void Spin(float r)
        {
            if (m_pdecal)
            {
                assert (!m_pthing);
                m_pdecal->SetAngle(m_pdecal->GetAngle() + r);
            }
        }

        void SetTexture(const char* pszTextureName)
        {
            assert (m_pthing);
            m_pthing->SetTexture(GetWindow()->GetModeler()->LoadImage(pszTextureName, false));
        }

        HRESULT LoadEffect(const Color& color)
        {
            m_pbuildeffect = CreateBuildEffectGeo(GetWindow()->GetModeler(),
                                                  GetWindow()->GetTime(),
                                                  color);

                    
            return S_OK;
        }

        HRESULT LoadDecal(const char* textureName, bool bDirectional, float width)
        {
            ZAssert(m_pthing == NULL && m_pdecal == NULL);
// BUILD_DX9
			GetEngine()->SetEnableMipMapGeneration( true );
// BUILD_DX9

            Number* ptime = GetWindow()->GetTime();
            TRef<AnimatedImage> pimage = 
                GetWindow()->LoadAnimatedImage(
                    NumberTransform::Divide(
                        NumberTransform::Subtract(ptime, ptime->MakeConstant()),
                        new Number(2)  // number of seconds to animate through images
                    ),
                    ZString(textureName) + "bmp"
                );

            if (pimage) {
                m_pdecal =
                    new DecalGeo(
                        pimage,
                        Color::White(),
                        Vector::GetZero(),
                        Vector::GetZero(),
                        Vector::GetZero(),
                        width,
                        0
                    );

                if (bDirectional) {
                    m_pdecal->SetForward(Vector(0, 0, -1));
                }
// BUILD_DX9
				GetEngine()->SetEnableMipMapGeneration( false );
// BUILD_DX9

                return S_OK;
            }
// BUILD_DX9
			GetEngine()->SetEnableMipMapGeneration( false );
// BUILD_DX9
            return E_FAIL;
        }

        HRESULT LoadModel(
            int options,
            const char* modelName,
            const char* textureName
        ) {
            HRESULT rc = S_OK;
            ZAssert(m_pthing == NULL && m_pdecal == NULL);

            if (modelName)
            {
// BUILD_DX9
				bool bOldColorKeyValue = GetModeler()->SetColorKeyHint( false );
				GetEngine()->SetEnableMipMapGeneration( true );
// BUILD_DX9
                m_pthing =
                    ThingGeo::Create(
                        GetWindow()->GetModeler(),
                        GetWindow()->GetTime()
                    );
                m_pthingGeo = m_pthing->GetGeo();

                if (textureName && ((!isalpha(textureName[0])) || (strcmp(modelName, textureName) == 0))) {
                    textureName = NULL;
                }

                TRef<Image> pimageTexture;

                if (textureName)
                {
                    pimageTexture = GetModeler()->LoadImage(textureName, false);
                }

                TRef<INameSpace> pns = GetModeler()->GetNameSpace(modelName);

                if (pns != NULL) {
                    rc = m_pthing->LoadMDL(options, pns, pimageTexture);
                } else {
// BUILD_DX9
					GetModeler()->SetColorKeyHint( bOldColorKeyValue );
					GetEngine()->SetEnableMipMapGeneration( false );
// BUILD_DX9
                    return E_FAIL;
                }

                #ifdef _DEBUG
                    {
                        bool bAnimation;
                        ZString str = ZString(modelName) + "_m.x";

                        TRef<Number> pnumber = new Number(0.0f);
                        TRef<Geo> pgeo = GetModeler()->LoadXFile(str, pnumber, bAnimation, false);

                        if (pgeo) {
                            m_pthing->SetBoundsGeo(
                                pgeo
                                /*
                                new TransformGeo(
                                    Geo::GetIcosahedron(),
                                    new ScaleTransform(2.0f)
                                )
                                */
                            );
                        }
                    }
                #endif

// BUILD_DX9
				GetModeler()->SetColorKeyHint( bOldColorKeyValue );
				GetEngine()->SetEnableMipMapGeneration( false );
// BUILD_DX9
			}
            return rc;
        }

        void    SetTrailColor(const Color& color)
        {
            if (m_pthing)
                m_pthing->SetTrailColor(color);
            else
            {
                assert (m_pdecal);
            }
        }

        HRESULT LoadAleph(const char* textureName)
        {
            ZAssert(m_pthing == NULL && m_pdecal == NULL);

            m_pthing =
                ThingGeo::Create(
                    GetWindow()->GetModeler(),
                    GetWindow()->GetTime()
                );
            m_pthingGeo = m_pthing->GetGeo();

            TRef<Image> pimageAleph = 
                GetWindow()->GetModeler()->LoadImage(ZString(textureName) + "bmp", false);

            if (pimageAleph) {
                m_peventSourceAleph = new TEvent<float>::SourceImpl;

                HRESULT hr = 
                    m_pthing->Load(
                        0, 
                        CreateAlephGeo(
                            GetWindow()->GetModeler(), 
                            m_peventSourceAleph,
                            GetWindow()->GetTime()
                        ),
                        pimageAleph
                    );

                return hr;
            }

            return E_FAIL;
        }

        HRESULT LoadMine(const char* textureName, float strength, float radius)
        {
            ZAssert(m_pthing == NULL && m_pdecal == NULL);

            m_pthing =
                ThingGeo::Create(
                    GetWindow()->GetModeler(),
                    GetWindow()->GetTime()
                );
            m_pthingGeo = m_pthing->GetGeo();

            TRef<Surface> psurface = 
                GetWindow()->GetModeler()->LoadSurface(ZString(textureName) + "bmp", true);

            if (psurface) {
                m_pmineFieldGeo = CreateMineFieldGeo(psurface, strength, radius);

                HRESULT hr = m_pthing->Load(0, m_pmineFieldGeo, NULL);

                return hr;
            }

            return E_FAIL;
        }

        void SetMineStrength(float strength)
        {
            m_pmineFieldGeo->SetStrength(strength);
        }

        void UpdateScreenPosition(
            const           Point& pointScreenPosition,
            float           fScreenRadius,
            float           distanceToEdge,
            unsigned char   ucRadarState
        ) {
            m_pointScreenPosition = pointScreenPosition;
            m_fScreenRadius       = fScreenRadius;
            m_distanceToEdge      = distanceToEdge;
            m_ucRadarState        = ucRadarState;
        }

        const Point& GetScreenPosition(void) const
        {
            return m_pointScreenPosition;
        }

        float GetDistanceToEdge()
        {
            return m_distanceToEdge;
        }
        
        float GetScreenRadius()
        {
            return m_fScreenRadius;
        }
        
        unsigned char GetRadarState()
        {
            return m_ucRadarState;
        }

        void SetCluster(ImodelIGC* pmodel, IclusterIGC* pcluster)
        {
            ModelAttributes ma = pmodel->GetAttributes();

            m_bSideVisibility = (ma & c_mtSeenBySide) != 0;
            m_bIsShip = (pmodel->GetObjectType() == OT_ship);

            if (pcluster)
            {
                if ((ma & c_mtPredictable) || !m_bSideVisibility)
                {
                    m_sideVisibility.fVisible(true);
					m_sideVisibility.CurrentEyed(true);  //Xynth #100 7/2010
                    m_enemySideVisibility.fVisible(true);
                    m_enemySideVisibility.CurrentEyed(true);

                    switch (pmodel->GetObjectType())
                    {
                        case OT_asteroid:
                            trekClient.GetClientEventSource()->
                                OnDiscoveredAsteroid((IasteroidIGC*)pmodel);
                        break;

                        case OT_station:
                            trekClient.GetClientEventSource()->
                                OnDiscoveredStation((IstationIGC*)pmodel);
                        break;

                        case OT_probe:
                        {
                            if ( ((IprobeIGC*)pmodel)->GetProbeType()->HasCapability(c_eabmRipcord) )
                            {
                                IsideIGC* pside = pmodel->GetSide();

                                if ( (pside != trekClient.GetSide()) && (!pside->AlliedSides(pside,trekClient.GetSide())) ) //ALLY - imago 7/3/09
                                {
                                    assert (pside);
                                    ClusterSite*    pcs = pcluster->GetClusterSite();
                                    pcs->SetClusterAssetMask(pcs->GetClusterAssetMask() | c_amEnemyTeleport);

                                    trekClient.PostText(true, START_COLOR_STRING "%s %s" END_COLOR_STRING " spotted in %s",
                                                        (PCC) ConvertColorToString (pside->GetColor ()), pside->GetName(), GetModelName(pmodel), pcluster->GetName());
                                }
                                else
                                {
                                    trekClient.PostText(true, "%s deployed in %s", GetModelName(pmodel), pcluster->GetName());
                                }
                            }
                        }
                    }
                }
                else
                    UpdateSideVisibility(pmodel, pcluster);
            }
            else
            {
                if (m_pSoundSite)
                    m_pSoundSite->StopSounds();
            }
        }

        void UpdateSideVisibility(ImodelIGC* pmodel, IclusterIGC* pcluster)
        {
			//Xynth #100 7/2010
			bool currentEye = false;
            //We can only update it if we have one & if the client is actually on a side.
            if (m_bSideVisibility && trekClient.GetSide())
            {
                //Update the visibility of hidden or non-static objects
                //(visibile static objects stay visible)
                //if (!(m_sideVisibility.fVisible() && (pmodel->GetAttributes() & c_mtPredictable))) Xynth #100 we need to run test for static objects
                //{
                    //We, trivially, see anything on our side. beyond that ...  Imago ALLY VISIBILITY 7/11/09
                    //does the ship that saw the object last still see it
                    //(if such a ship exists)
                    if ( 
						(trekClient.GetSide() == pmodel->GetSide()) || 
						(trekClient.GetSide()->AlliedSides(pmodel->GetSide(),trekClient.GetSide()) && trekClient.MyMission()->GetMissionParams().bAllowAlliedViz) ||
						(m_sideVisibility.pLastSpotter() && m_sideVisibility.pLastSpotter()->InScannerRange(pmodel) && trekClient.GetSide() == m_sideVisibility.pLastSpotter()->GetSide()) ||
						(m_sideVisibility.pLastSpotter() && (trekClient.GetSide()->AlliedSides(m_sideVisibility.pLastSpotter()->GetSide(),trekClient.GetSide()) && trekClient.GetSide() != m_sideVisibility.pLastSpotter()->GetSide()) && m_sideVisibility.pLastSpotter()->InScannerRange(pmodel) && trekClient.MyMission()->GetMissionParams().bAllowAlliedViz)
					   )
                    {
                        //yes
						currentEye = true; //Xynth #100 7/2010
                        if (!m_sideVisibility.fVisible())
                        {
                            if (m_bIsShip)
                            {
                                IsideIGC* pside = pmodel->GetSide();

								//this is where you would put in rate limiting for the sound effects if someone "eye" spams };-) -imago
                                if ((pside != trekClient.GetSide()) && (!pside->AlliedSides(pside,trekClient.GetSide()))) //ALLY Imago 7/3/09
                                {
                                    trekClient.PlaySoundEffect(newShipSound, pmodel);
                                }
                                else
                                {
                                    trekClient.PlaySoundEffect(newEnemySound, pmodel);
                                }
                            }
                            //Xynth #100 7/2010 m_sideVisibility.fVisible(true);
                        }
                    }
                    else
                    {
                        //do it the hard way
                        currentEye = false; //Xynth #100 7/2010 m_sideVisibility.fVisible(false);
                        for (ScannerLinkIGC*   l = pcluster->GetClusterSite()->GetScanners(0)->first(); 
                             (l != NULL);
                             l = l->next())
                        {
                            IscannerIGC*   s = l->data();
                            assert (s->GetCluster() == pcluster);

                            if (s->InScannerRange(pmodel))
							{
                                //Ship s's side does not see the ship but this ship does
                                if (m_bIsShip)
                                    trekClient.PlaySoundEffect(newShipSound, pmodel);

                                currentEye = true; //Xynth #100 7/2010 m_sideVisibility.fVisible(true);
                                m_sideVisibility.pLastSpotter(s);								
								if (trekClient.MyMission()->GetMissionParams().bAllowAlliedViz) //ALLY should be SCAN Imago 7/13/09
								{
									//lets get a list of allied sideIDs
								    for (SideLinkIGC* psidelink = trekClient.GetCore()->GetSides()->first();
										(psidelink != NULL);
										psidelink = psidelink->next())
									{
										IsideIGC*   otherside = psidelink->data();
										//this spotter's side is ally...and not ours...and we dont already see it
										if (s->GetSide()->AlliedSides(s->GetSide(),otherside) && s->GetSide() != otherside && !pmodel->SeenBySide(otherside))
		                                	pmodel->SetSideVisibility(otherside,true);
									}
								}		
							}
                        }
                    }
                //}

                    if (!trekClient.m_fm.IsConnected()) {
                        if (pmodel->GetObjectType() == OT_ship) {
                            IshipIGC* pShip = (IshipIGC*)pmodel;
                            if (!m_sideVisibility.CurrentEyed() && currentEye) {
                                debugf(">>UpdateSideVis for %s\n", pmodel->GetName());
                                PlayerInfo* ppi = (PlayerInfo*)(pShip->GetPrivateData());
                                if (ppi) {
                                    ShipStatus ss = ppi->GetShipStatus();
                                    ss.SetStateTime(trekClient.m_lastUpdate.clock());
                                    ss.SetDetected(true);
                                    ss.SetSectorID(pcluster->GetObjectID());
                                    ss.SetState(c_ssFlying);
                                    ss.SetHullID(((IshipIGC*)pmodel)->GetHullType()->GetObjectID());
                                    if (pmodel->GetCluster())
                                        ss.SetState(c_ssFlying);
                                    ppi->SetShipStatus(ss);
                                }
                            }
                            else if (m_sideVisibility.CurrentEyed() && !currentEye) {
                                debugf(">>UpdateSideVis for %s - hide\n", pmodel->GetName());
                                PlayerInfo* ppi = (PlayerInfo*)(pShip->GetPrivateData());
                                if (ppi) {
                                    ShipStatus ss = ppi->GetShipStatus();
                                    ss.SetDetected(false);
                                    ss.SetUnknown();
                                    ppi->SetShipStatus(ss);
                                }
                            }
                        }
                    }

                    m_sideVisibility.CurrentEyed(currentEye);
                    currentEye = currentEye || (m_sideVisibility.fVisible() && (pmodel->GetAttributes() & c_mtPredictable));
                    m_sideVisibility.fVisible(currentEye);
			
			}
            if (!trekClient.m_fm.IsConnected()) {
                // Done with player side, now update visibilities for the enemy -- ignore allies for now
                currentEye = false;
                if ((pmodel->GetSide() != trekClient.GetSide()) ||
                    (m_enemySideVisibility.pLastSpotter() && m_enemySideVisibility.pLastSpotter()->InScannerRange(pmodel))) //&& trekClient.GetSide() != m_enemySideVisibility.pLastSpotter()->GetSide()) ||
                {
                    currentEye = true;
                }
                else
                {
                    //do it the hard way
                    for (ScannerLinkIGC* l = pcluster->GetClusterSite()->GetScanners(1)->first(); (l != NULL); l = l->next())
                    {
                        IscannerIGC*   s = l->data();

                        if (s->InScannerRange(pmodel))
                        {
                            currentEye = true;
                            m_enemySideVisibility.pLastSpotter(s);
                            break;
                        }
                    }
                }
                m_enemySideVisibility.CurrentEyed(currentEye);
                currentEye = currentEye || (m_enemySideVisibility.fVisible() && (pmodel->GetAttributes() & c_mtPredictable));
                m_enemySideVisibility.fVisible(currentEye);

                
                if (pmodel->GetObjectType() == OT_ship) {
                    IshipIGC* pShip = (IshipIGC*)pmodel;
                    if (pShip == trekClient.GetShip()) {
                        // let the player know if he is eyed
                        PlayerInfo* ppi = (PlayerInfo*)(pShip->GetPrivateData());
                        if (ppi) {
                            ShipStatus ss = ppi->GetShipStatus();
                            ss.SetDetected(currentEye); // For UI eye
                            ppi->SetShipStatus(ss);
                        }
                    }
                }
            }

        }

        bool GetSideVisibility(IsideIGC* side)
        {
            assert (side);

            if (Training::IsTraining ())
            {
                //if ((trekClient.GetShip()->GetSide() != side) && (Training::GetTrainingMissionID() != Training::c_TM_6_Practice_Arena) && (Training::GetTrainingMissionID() != Training::c_TM_10_Free_Flight))
                //    return false;
                if (side != trekClient.GetSide())
                    return m_enemySideVisibility.fVisible();
            }
            return m_sideVisibility.fVisible();
        }

		bool GetCurrentEye(IsideIGC* side)
        {
            assert (side);
            if (!trekClient.m_fm.IsConnected() && side != trekClient.GetSide())
                return m_enemySideVisibility.CurrentEyed();
			return m_sideVisibility.CurrentEyed();
        }

        void SetSideVisibility(IsideIGC* side, bool fVisible)
        {
            if (m_bSideVisibility) {
                if (side == trekClient.GetSide()) // || (side->AlliedSides(side,trekClient.GetSide()) && trekClient.MyMission()->GetMissionParams().bAllowAlliedViz) ) ) //imago viz ALLY VISIBILITY 7/11/09
                {
                    m_sideVisibility.fVisible(fVisible);
                    m_sideVisibility.CurrentEyed(fVisible);
                }
                else if (!trekClient.m_fm.IsConnected())
                {
                    m_enemySideVisibility.fVisible(fVisible);
                    m_enemySideVisibility.CurrentEyed(fVisible);
                }
            }
            if (fVisible)
            {
                switch (m_pmodel->GetObjectType())
                {
                case OT_asteroid:
                    trekClient.GetClientEventSource()->
                        OnDiscoveredAsteroid((IasteroidIGC*)m_pmodel);
                    break;

                case OT_station:
                    trekClient.GetClientEventSource()->
                        OnDiscoveredStation((IstationIGC*)m_pmodel);
                    break;
                }
            }
        }

        virtual void             ActivateBolt(void)
        {
            if (m_pmodel->GetVisibleF())
            {
                float           r = 0.5f * m_pmodel->GetRadius();
                Vector          f = m_pmodel->GetOrientation().GetBackward();
                Vector          p1 = m_pmodel->GetPosition() - f * r;    //Hack till we get an emit point
                Vector          p2 = p1 - (100.0f + r) * f;

                if (m_pvisibleGeoBolt)
                {
                    m_pvvBoltP1->SetValue(p1);
                    m_pvvBoltP2->SetValue(p2);
                }
                else
                {

                    m_pvvBoltP1 = new ModifiableVectorValue(p1);
                    m_pvvBoltP2 = new ModifiableVectorValue(p2);

                    m_pvisibleGeoBolt =
                        new VisibleGeo(
                            CreateBoltGeo(
                                m_pvvBoltP1, 
                                m_pvvBoltP2, 
                                0.125f,
                                GetModeler()->LoadSurface("lightningbmp", true)
                            )
                        );

                    m_pmodel->GetCluster()->GetClusterSite()->GetGroupScene()->AddGeo(m_pvisibleGeoBolt);
                }
            }
        }

        virtual void             DeactivateBolt(void)
        {
            if (m_pvisibleGeoBolt) {
                m_pvisibleGeoBolt->SetEmpty();
                m_pvisibleGeoBolt = NULL;
            }
        }

        virtual int         GetMask(void) const
        {
            return m_mask;
        }
        virtual void        SetMask(int mask)
        {
            m_mask = mask;
        }
        virtual void        OrMask(int mask)
        {
            m_mask |= mask;
        }
        virtual void        AndMask(int mask)
        {
            m_mask &= mask;
        }
        virtual void        XorMask(int mask)
        {
            m_mask ^= mask;
        }

        // updates the current sounds for this object acording to its state.
        void UpdateSounds(DWORD dwElapsedTime)
        {
            if (m_pSoundSite)
                m_pSoundSite->UpdateSounds(dwElapsedTime);

            m_bPlayedHitSoundThisFrame = false;
        };

        // stops all sounds on this object - usually used when changing clusters.
        void StopSounds()
        {
            if (m_pSoundSite)
                m_pSoundSite->StopSounds();
        };

        TRef<ISoundPositionSource> GetSoundSource()
        {
            if (!m_pSoundSite)
                m_pSoundSite = new SoundSite(m_pmodel);

            return m_pSoundSite->GetSoundSource();
        };

        TRef<ISoundPositionSource> GetSoundSource(const Vector& vectOffset)
        {
            if (!m_pSoundSite)
                m_pSoundSite = new SoundSite(m_pmodel);

            return m_pSoundSite->GetSoundSource(vectOffset);
        };

        void RegisterHit(float fAmount, const Vector& vectOffset, bool bAbsorbedByShield = false)
        {
            // REVIEW: at the moment, this should only be called for ships
            assert(m_bIsShip);

            if (!m_bPlayedHitSoundThisFrame)
            {
                m_bPlayedHitSoundThisFrame = true;

                // figure out which sound to play
                SoundID soundId;
                if (m_pmodel == trekClient.GetShip()->GetSourceShip())
                {
                    if (bAbsorbedByShield)
                        soundId = myShieldHitSound;
                    else
                        soundId = myHullHitSound;                
                }
                else
                {
                    if (bAbsorbedByShield)
                        soundId = otherShieldHitSound;
                    else
                        soundId = otherHullHitSound;                
                }

                // play the appropriate sound
                TRef<ISoundPositionSource> psource = GetSoundSource(vectOffset);
                m_pSoundSite->AddHitSound(trekClient.StartSound(soundId, psource));
           }
        }

        TRef<ClusterSite> GetClusterSite()
        {
            return m_pmodel->GetCluster()->GetClusterSite();
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // Members
        //
        //////////////////////////////////////////////////////////////////////////////

    private:
        TRef<BuildEffectGeo>        m_pbuildEffectGeo;

        TRef<TEvent<float>::SourceImpl>   m_peventSourceAleph;

        SideVisibility              m_sideVisibility;
        SideVisibility              m_enemySideVisibility;
        TRef<VisibleGeo>            m_pvisibleGeoBolt;
        TRef<ThingGeo>              m_pthing;
        TRef<Geo>                   m_pthingGeo;
        TRef<DecalGeo>              m_pdecal;
        TRef<BuildEffectGeo>        m_pbuildeffect;
        TRef<MineFieldGeo>          m_pmineFieldGeo;
        ImodelIGC*                  m_pmodel;
        Point                       m_pointScreenPosition;
        TRef<ModifiableVectorValue> m_pvvBoltP1;
        TRef<ModifiableVectorValue> m_pvvBoltP2;
        float                       m_fScreenRadius;
        float                       m_radius;
        float                       m_distanceToEdge;
        int                         m_mask;
        bool                        m_bSideVisibility;
        unsigned char               m_ucRadarState;
        bool                        m_bIsShip;
        bool                        m_bPlayedHitSoundThisFrame;
        SoundSite*                  m_pSoundSite;
};

const float c_fGrooveLevelDuration = 10.0f;

WinTrekClient::WinTrekClient(void)
:
    mountSelected(-1),
    fGroupFire(true),
    wmOld(0),
    bInitTrekThrottle (true),
    joyThrottle(true),
    trekThrottle(-1.0f),
    fOldJoyThrottle (-1.0f),
    m_sideidLastWinner(NA),
    m_bWonLastGame(false),
    m_bLostLastGame(false),
    m_nNumEndgamePlayers(0),
    m_nNumEndgameSides(0),
    m_vplayerEndgameInfo(NULL),
    m_nGrooveLevel(0),
    m_bDisconnected(false),
    m_strDisconnectReason(""),
    m_bFilterChatsToAll(false),
    m_bFilterQuickComms(false),
	m_bFilterUnknownChats(true), //TheBored 30-JUL-07: Filter Unknown Chat patch
    m_dwFilterLobbyChats(3), //TheBored 25-JUN-07: Changed value to 3 (Don't Filter Lobby)
	bTrainingFirstClick(false),
    m_pChatLogger(nullptr)
{
    
}

WinTrekClient::~WinTrekClient(void)
{
    if (m_vplayerEndgameInfo)
        delete m_vplayerEndgameInfo;
}

void WinTrekClient::Initialize(Time timeNow)
{
    BaseClient::Initialize(timeNow);

    // making sure that this only happens once, instead of also being run on a reset
    if (m_pChatLogger == nullptr) {

        // I'm not entirely sure if this is used. It was at some point proposed to be used as a way to authenticate bots (pigs) to the servers.
        auto strCdKeyConfig = GetConfiguration()->GetStringValue(
            "Online.AuthenticationId",
            GetConfiguration()->GetStringValue("CDKey", "")
        );
        if (strCdKeyConfig != "") {
            BaseClient::SetCDKey(strCdKeyConfig.c_str());
        }

        m_pLogChatEnabled = GetConfiguration()->GetBool("Chat.Log", GetConfiguration()->GetBoolValue("LogChat", false));
        if (m_pLogChatEnabled->GetValue()) {
            m_pChatLogger = CreateTimestampedFileLogger(GetExecutablePath() + "\\logs\\chat_");
        }
        else {
            m_pChatLogger = NullLogger::GetInstance();
        }

        SoundEngine::bConvertToMono = GetConfiguration()->GetBool("Sound.ConvertToMono", false == GetConfiguration()->GetBoolValue("MonoOff", false))->GetValue();
    }

    GetShip()->CreateDamageTrack();

    m_pwindow = GetWindow();
    m_pengine = GetEngineWindow()->GetEngine();
    m_pmodeler = m_pwindow->GetModeler();

    {
        TRef<Surface> psurfaceFlare = m_pmodeler->LoadSurface(AWF_SHIELD_FLARE_TEXTURE, true);
        ZAssert(psurfaceFlare);
        m_pimageFlare = new AnimatedImage(m_pwindow->GetTime(), psurfaceFlare);
    }

    {
         TRef<INameSpace> pns = m_pmodeler->GetNameSpace(AWF_SHIELD_FLARE);
         if (pns) {
             CastTo(m_pgeoFlares[0], pns->FindMember("lens30"));
             CastTo(m_pgeoFlares[1], pns->FindMember("lens60"));
             CastTo(m_pgeoFlares[2], pns->FindMember("lens90"));
         }
    }

    //
    // Explosion images
    //

    m_pimageShockWave = GetModeler()->LoadImage(AWF_SHOCKWAVE, true);

    m_vpimageExplosion[0].SetCount(5);
    m_vpimageExplosion[0].Set(0, LoadExplosionImage(AWF_EXPLOSION_00));
    m_vpimageExplosion[0].Set(1, LoadExplosionImage(AWF_EXPLOSION_01));
    m_vpimageExplosion[0].Set(2, LoadExplosionImage(AWF_EXPLOSION_02));
    m_vpimageExplosion[0].Set(3, LoadExplosionImage(AWF_EXPLOSION_03));
    m_vpimageExplosion[0].Set(4, LoadExplosionImage(AWF_EXPLOSION_04));

    m_vpimageExplosion[1].SetCount(1);
    m_vpimageExplosion[1].Set(0, m_vpimageExplosion[0][3]);

    m_vpimageExplosion[2].SetCount(1);
    m_vpimageExplosion[2].Set(0, m_vpimageExplosion[0][0]);

    m_vpimageExplosion[3].SetCount(2);
    m_vpimageExplosion[3].Set(0, m_vpimageExplosion[0][4]);
    m_vpimageExplosion[3].Set(1, m_vpimageExplosion[0][1]);

    m_vpimageExplosion[4].SetCount(2);
    m_vpimageExplosion[4].Set(0, m_vpimageExplosion[0][4]);
    m_vpimageExplosion[4].Set(1, m_vpimageExplosion[0][1]);

    m_vpimageExplosion[5].SetCount(2);
    m_vpimageExplosion[5].Set(0, m_vpimageExplosion[0][4]);
    m_vpimageExplosion[5].Set(1, m_vpimageExplosion[0][2]);

    m_vpimageExplosion[6].SetCount(1);
    m_vpimageExplosion[6].Set(0, m_vpimageExplosion[0][3]);

    m_vpimageExplosion[7].SetCount(2);
    m_vpimageExplosion[7].Set(0, m_vpimageExplosion[0][4]);
    m_vpimageExplosion[7].Set(1, m_vpimageExplosion[0][2]);
}

TRef<AnimatedImage> WinTrekClient::LoadExplosionImage(const ZString& str)
{
// BUILD_DX9
	// Load source AnimatedImage into system memory rather than VRAM.
    return new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface(str, true, true, true));
//#else
//    return new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface(str, true));
// BUILD_DX9
}

void WinTrekClient::Terminate(void)
{
    m_pgeoFlares[2] = NULL;
    m_pgeoFlares[1] = NULL;
    m_pgeoFlares[0] = NULL;

    m_pimageFlare = NULL;

    m_pmodeler = NULL;
    m_pengine = NULL;
    m_pwindow = NULL;
    m_psoundAmbient = NULL;
    m_psoundMissileWarning = NULL;

    BaseClient::Terminate();
}


ImissionIGC*    WinTrekClient::ResetStaticData (void)
{
    Time    now = m_pCoreIGC->GetLastUpdate ();
    FlushGameState();
    m_pCoreIGC = CreateMission();
    Initialize(now);
    m_pMissionInfo = new MissionInfo(static_cast<DWORD> (m_pCoreIGC->GetMissionID ()));
    return m_pCoreIGC;
}

IpartIGC*   WinTrekClient::GetCargoPart(void)
{
    assert (mountSelected < 0 && mountSelected >= -c_maxCargo);

    IpartIGC* ppart = GetShip()->GetMountedPart(NA, mountSelected);

    // make sure it's the first item of a grouped set of items.  
    if (mountSelected < -1) // first item in the list is always the first item of its type
    {
        mountSelected++;
        NextCargoPart();
        ppart = GetShip()->GetSourceShip()->GetMountedPart(NA, mountSelected);
    }

    return ppart;
}

void    WinTrekClient::NextCargoPart(void)
{
    // select a real part or the first empty cargo slot
    bool bFoundPart = false;
    do
    {
        --mountSelected;
        if (mountSelected < -c_maxCargo)
            mountSelected = -1;

        IpartIGC *ppart = GetShip()->GetSourceShip()->GetMountedPart(NA, mountSelected);
        IpartTypeIGC *ppartType = ppart ? ppart->GetPartType() : NULL;

        bFoundPart = true;

        // If this is ammo/missiles/etc, is this the first instance of this part?
        ObjectType ot = ppart ? ppart->GetObjectType() : NA;
        if (ppart == NULL || ot == OT_pack || ot == OT_magazine || ot == OT_dispenser)
        {
            for (Mount mountPrev = mountSelected + 1; mountPrev < 0; ++mountPrev)
            {
                IpartIGC *ppartPrev = GetShip()->GetSourceShip()->GetMountedPart(NA, mountPrev);
                IpartTypeIGC *ppartTypePrev = ppartPrev ? ppartPrev->GetPartType() : NULL;

                if (ppartType == ppartTypePrev)
                {
                    bFoundPart = false;
                    break;
                }
            }
        }
    }
    while (!bFoundPart);
}
Mount     WinTrekClient::GetSelectedCargoMount() 
{ 
    assert (mountSelected < 0 && mountSelected >= -c_maxCargo);

    IpartIGC* ppart = GetShip()->GetSourceShip()->GetMountedPart(NA, mountSelected);

    // make sure it's the first empty slot if it's a slot.  
    if (mountSelected < -1 && !ppart)
    {
        mountSelected++;
        NextCargoPart();
    }

    return mountSelected;
}

void      WinTrekClient::OnReload(IpartIGC* ppart, bool bConsumed)
{
    if (bConsumed && ppart == GetCargoPart())
    {
        NextCargoPart();
        GetCore()->GetIgcSite()->LoadoutChangeEvent(GetShip(), ppart, c_lcCargoSelectionChanged);
    }
}
bool      WinTrekClient::SelectCargoPartOfType(EquipmentType et, PartMask maxPartMask, IpartTypeIGC* pptNotThis)
{
    IpartIGC* ppartStart = GetCargoPart();
        
    if (ppartStart && (
               (ppartStart->GetEquipmentType() != et) ||
               ((ppartStart->GetPartType()->GetPartMask() & ~maxPartMask) != 0) ||
               (ppartStart->GetPartType() == pptNotThis))
        || (!ppartStart && !pptNotThis))
    {
        IpartIGC* ppart;

        do
        {
            NextCargoPart();                    
            ppart = GetCargoPart();
        }
        while (ppart != ppartStart
            &&  (
                ppart && (
                    ppart->GetEquipmentType() != et
                    || ((ppart->GetPartType()->GetPartMask() & ~maxPartMask) != 0)
                    || ppart->GetPartType() == pptNotThis)
                || (!ppart && !pptNotThis)
            )
            );

        if (ppart == ppartStart)
        {
            // no part in cargo found
            return false;
        }

        return true;
    }
    else
    {
        return true;
    }
}

void WinTrekClient::LoadoutChangeEvent(IshipIGC* pship, IpartIGC* ppart, LoadoutChange lc)
{
    if (pship == trekClient.GetShip())
    {
        if (lc == c_lcHullChange)
        {
            GetWindow()->ResetCameraFOV();
        }
    }
    BaseClient::LoadoutChangeEvent(pship, ppart, lc);
}

IObject*    WinTrekClient::LoadRadarIcon(const char* szName)
{
    Surface*    psurface;

    if (szName && (szName[0] != '\0'))
    {
        psurface = GetModeler()->LoadSurface(ZString(szName) + "bmp", true);
        assert (psurface);
    }
    else
        psurface = NULL;

    return psurface;
}

void WinTrekClient::ChangeCluster(IshipIGC*  pship, IclusterIGC* pclusterOld, IclusterIGC* pclusterNew)
{
    if (pship == GetShip())
    {
        if (pclusterOld && m_fm.IsConnected())
            pclusterOld->SetActive(false);

        BaseClient::ChangeCluster(pship, pclusterOld, pclusterNew);

        if (pclusterNew && m_fm.IsConnected())
            pclusterNew->SetActive(true);

        // if there was an old cluster, stop any sound sources in the old cluster
        if (pclusterOld && (pclusterOld != GetViewCluster()))
        {
            // stop the sounds on every object in the sector
            for (ModelLinkIGC*   pml = pclusterOld->GetModels()->first(); (pml != NULL); pml = pml->next())
            {
                ImodelIGC*   pmodel = pml->data();
                ThingSite*  pts = pmodel->GetThingSite();
                if (pts)
                {
                    ((ThingSiteImpl*)pts)->StopSounds();
                }
            }
        }

        // if we have a command to go to this cluster, clear the command.
        for (Command i = 0; i < c_cmdMax; i++)
        {
            ImodelIGC* ptarget = pship->GetCommandTarget(i);
            
            // if this command has a cluster buoy in this cluster...
            if (ptarget && ptarget->GetObjectType() == OT_buoy 
                && ((IbuoyIGC*)ptarget)->GetBuoyType() == c_buoyCluster
                && ((IbuoyIGC*)ptarget)->GetCluster() == pclusterNew)
            {
                // clear the command
                pship->SetCommand(i, NULL, c_cidNone);
            }
        }

        // notify the UI of the change
        GetWindow()->SetCluster(pclusterNew);
        GetWindow()->PositionCommandView(NULL, 0.0f);
        trekClient.GetClientEventSource()->OnClusterChanged(pclusterNew);

		// pkk - fix for #9 - disable the afterburner on sector change
		trekClient.GetShip()->SetStateBits(afterburnerButtonIGC, 0);
    }
}

void WinTrekClient::ChangeStation(IshipIGC*  pship, IstationIGC* pstationOld, IstationIGC* pstationNew)
{
    if (pship == GetShip() && trekClient.MyMission()->GetStage() == STAGE_STARTED)
	{
        if (pstationNew)
		{
            if (pstationOld == NULL)
			{
                if (!m_fm.IsConnected()) {
                    // Go over the ship's parts and give the side the tech bits for any equipment onboard
                    TechTreeBitMask ttbm;
                    ttbm.ClearAll();
                    for (PartLinkIGC* ppl = pship->GetParts()->first(); (ppl != NULL); ppl = ppl->next())
                    {
                        IpartIGC*   ppart = ppl->data();

                        ttbm |= ppart->GetPartType()->GetEffectTechs();
                        if (pship->GetSide()->IsNewDevelopmentTechs(ppart->GetPartType()->GetEffectTechs()))
                        {
                            // send message that player secured new tech
                            PostText(true, "%s has secured %s", pship->GetName(), ppart->GetPartType()->GetName());
                        }
                    }

                    IsideIGC*   pside = pship->GetSide();
                    assert(pside);
                    pside->ApplyDevelopmentTechs(ttbm);
                }

                ConsoleImage*   pconsole = GetWindow()->GetConsoleImage();
                if (pconsole)
                {
                    pconsole->SetDisplayMDL(pstationNew->GetSide()->GetCivilization()->GetHUDName());

                    if ((trekClient.GetShip()->GetParentShip() == NULL) && !trekClient.GetShip()->IsGhost())
                    {
                        trekClient.RestoreLoadout(pstationNew);
                    }
                }

                if (GetWindow()->screen() == ScreenIDCombat)
                {
                    trekClient.wmOld = 0;

                    //imago 8/10/09 turreted ship rearms @ allied base fix ALLY
                    if ((pstationNew->GetSide() != trekClient.GetSide()) && 
                        (trekClient.GetShip()->GetParentShip() == NULL) && 
                        (trekClient.GetShip()->GetChildShips()->n() != 0) &&
                        (pstationNew->GetSide()->AlliedSides(pstationNew->GetSide(),trekClient.GetSide()))) {
                        if (IsLockedDown())
                            EndLockDown(lockdownDonating | lockdownLoadout | lockdownTeleporting);
                        trekClient.BuyLoadout(trekClient.GetShip(), true);
                        return;
                    }

                    if (GetWindow()->GetCameraMode() != TrekWindow::cmExternalOverride && (pstationNew->GetSide() == trekClient.GetSide()))
                    {
                        //if ((!Training::IsTraining ()) || (Training::GetTrainingMissionID () != Training::c_TM_5_Command_View))
                        {
                            GetWindow()->SetViewMode(trekClient.GetShip()->IsGhost() 
                                ? TrekWindow::vmCommand : TrekWindow::vmHangar);
                            PlaySoundEffect(dockedSound);
                            PlaySoundEffect(salWelcomeHomeSound);
                        }
                    }
                }
            }
            else
            {
                //NYI do anything appropriate for switching stations
            }
		}
        else
        {
            assert (pstationOld);

            //Save the loadout except for the launch in underwear case (which should be very rare)
            assert (pstationOld);
            IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();
            assert (pshipSource);

            IhullTypeIGC*   pht = pshipSource->GetBaseHullType();
            assert (pht);

			pstationOld->RepairAndRefuel(pshipSource);
	

            /*
            const char* pszDisplayMDL;
            if (pshipSource == trekClient.GetShip())
            {
                //If no weapon is selected, try to select a weapon
                {
                    Mount nHardpoints = pht->GetMaxFixedWeapons();
                    if ((trekClient.m_selectedWeapon >= nHardpoints) ||
                        (trekClient.GetShip()->GetMountedPart(ET_Weapon, trekClient.m_selectedWeapon) == NULL))
                    {                       
                        trekClient.NextWeapon();
                    }
                }

                pszDisplayMDL = pht->GetPilotHUDName();
            }
            else
            {
                Mount   turretID = trekClient.GetShip()->GetTurretID();
                if (turretID == NA)
                {
                    pszDisplayMDL = pht->GetObserverHUDName();
                }
                else
                {
                    pszDisplayMDL = pht->GetHardpointData(turretID).hudName;
                }
            }

            GetWindow()->GetConsoleImage()->SetDisplayMDL(pszDisplayMDL);
            */

            RequestViewCluster(NULL);
            GetWindow()->OverrideCamera(trekClient.m_now, pstationOld, false);

            //joyThrottle = true;
            //trekThrottle = -1.0f;
            OverrideThrottle (1.0f);

            trekClient.GetShip()->SetStateBits(coastButtonIGC, 0);
        }
    }
}

void WinTrekClient::OverrideCamera(ImodelIGC*  pmodel)
{
    GetWindow()->OverrideCamera(trekClient.m_now, pmodel, false);
}

void WinTrekClient::TargetKilled(ImodelIGC*  pmodel)
{
    ImodelIGC*  pmodelCurrent = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);
    if (pmodel == pmodelCurrent)
    {
        IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();
        if (pshipSource->GetCluster())
        {
            extern int GetSimilarTargetMask(ImodelIGC*  pmodel);
            int mask = GetSimilarTargetMask(pmodel) | c_ttNearest;

            int abm;
            if (mask == (c_ttShip | c_ttEnemy | c_ttNearest))
                abm = c_habmRescue;
            else
                abm = 0;

            ImodelIGC*  pmodelTarget = FindTarget(pshipSource,
                                                  mask,
                                                  pmodel, NULL, NULL, NULL, abm);
            if ((pmodelTarget == NULL) && abm)
            {
                pmodelTarget = FindTarget(pshipSource,
                                          mask,
                                          pmodel, NULL, NULL, NULL, c_habmMiner | c_habmBuilder);

                if (pmodelTarget == NULL)
                    pmodelTarget = FindTarget(pshipSource,
                                              mask,
                                              pmodel);
            }

            if (pmodelTarget == pmodel)
                pmodelTarget = NULL;

            GetWindow()->SetTarget(pmodelTarget, c_cidDefault);
            if (trekClient.autoPilot() && (trekClient.GetShip()->GetCommandTarget(c_cmdPlan) == pmodel))
                trekClient.SetAutoPilot(true);
        }
    }
}

void WinTrekClient::ShipWarped(IshipIGC*    pship, 
                               SectorID    sidOld,
                               SectorID    sidNew)
{
    /*
    if ((GetWindow()->GetViewMode() == TrekWindow::vmCommand) && trekClient.GetShip()->GetStation())
    {
        IclusterIGC*    pcluster = trekClient.GetViewCluster();
        if (pcluster && (pcluster->GetObjectID() == sidOld))
        {
            const ShipListIGC*  pships = GetWindow()->GetConsoleImage()->GetSubjects();
            if (pships && pships->find(pship))
                RequestViewCluster(trekClient.m_pCoreIGC->GetCluster(sidNew));
        }
    }
    */
}

void WinTrekClient::PostNotificationText(ImodelIGC* pmodel, bool bCritical, const char* pszText, ...)
{
    if (GetWindow()->GetConsoleImage() && pmodel == trekClient.GetShip())
    {
        assert (pszText);
        const size_t size = 256;
        char bfr[size];

        va_list vl;
        va_start(vl, pszText);
        _vsnprintf(bfr, size, pszText, vl);
        va_end(vl);

        if (bCritical) 
        {
            PlaySoundEffect(newCriticalMsgSound);
            GetWindow()->GetConsoleImage()->GetConsoleData()->SetCriticalTipText(bfr);
        }
        else
        {
            PlaySoundEffect(newNonCriticalMsgSound);
            GetWindow()->GetConsoleImage()->GetConsoleData()->SetTipText(bfr);
        }
    }
}
void WinTrekClient::ActivateTeleportProbe(IprobeIGC* pprobe)
{
    IsideIGC*       pside = pprobe->GetSide();
    IclusterIGC*    pcluster = pprobe->GetCluster();

    if ( (pside != trekClient.GetSide()) && !pside->AlliedSides(pside,trekClient.GetSide()) ) //ALLY - imago 7/3/09
    {
        assert (pside);
        PostText(true, START_COLOR_STRING "%s %s" END_COLOR_STRING " active in %s",
                 (PCC) ConvertColorToString (pside->GetColor ()), pside->GetName(), GetModelName(pprobe), pcluster->GetName());
    }
    else
    {
        PostText(true, "%s active in %s", GetModelName(pprobe), pcluster->GetName());
    }
}

void WinTrekClient::DestroyTeleportProbe(IprobeIGC* pprobe)
{
    IsideIGC*       pside = pprobe->GetSide();
    IsideIGC*       psideMe = trekClient.GetSide();
    IclusterIGC*    pcluster = pprobe->GetCluster();

	if ( (pside != psideMe) && !pside->AlliedSides(pside,psideMe) ) //ALLY - imago 7/3/09
    {
        assert (pside);
        ClusterSite*    pcs = pcluster->GetClusterSite();

        AssetMask      am = pcs->GetClusterAssetMask() & ~c_amEnemyTeleport;
        {
            for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->first(); (ppl != NULL); ppl = ppl->next())
            {
                IprobeIGC*  pp = ppl->data();
                if ((pp != pprobe) && (pp->GetSide() != psideMe) && pp->GetProbeType()->HasCapability(c_eabmRipcord))
                    am |= c_amEnemyTeleport;
            }
        }
        pcs->SetClusterAssetMask(am);

        PostText(false, START_COLOR_STRING "%s %s" END_COLOR_STRING " in %s was destroyed",
                 (PCC) ConvertColorToString (pside->GetColor ()), pside->GetName(), GetModelName(pprobe), pcluster->GetName());
    }
    else
    {
        PostText(true, "%s in %s was destroyed", GetModelName(pprobe), pcluster->GetName());
    }
}

void WinTrekClient::PostPlainText(bool bCritical, const char* pszText)
{
    if (GetWindow()->GetConsoleImage())
    {
        assert(pszText);

        if (bCritical)
        {
            PlaySoundEffect(newCriticalMsgSound);
            GetWindow()->GetConsoleImage()->GetConsoleData()->SetCriticalTipText(pszText);
        }
        else
        {
            PlaySoundEffect(newNonCriticalMsgSound);
            GetWindow()->GetConsoleImage()->GetConsoleData()->SetTipText(pszText);
        }
    }
}

void WinTrekClient::PostText(bool bCritical, const char* pszText, ...)
{
    if (GetWindow()->GetConsoleImage())
    {
        assert (pszText);
        const size_t size = 256;
        char bfr[size];

        va_list vl;
        va_start(vl, pszText);
        _vsnprintf(bfr, size, pszText, vl);
        va_end(vl);

        PostPlainText(bCritical, bfr);
    }
}

void WinTrekClient::EjectPlayer(ImodelIGC*  pcredit)
{
    GetWindow()->OverrideCamera(trekClient.m_now, pcredit, true);
}

int WinTrekClient::GetSavedWingAssignment(){ // kolie 6/10
    return GetConfiguration()->GetIntValue("Ui.DefaultWing", GetConfiguration()->GetIntValue("WingAssignment", 0));
}

void WinTrekClient::SaveWingAssignment(int index){ // kolie 6/10
    GetConfiguration()->GetInt("Ui.DefaultWing", 0)->SetValue((int)index);
}

// KGJV : added utility functions for cores & server names
// find the user friendly name of a core - return param if not found
ZString WinTrekClient::CfgGetCoreName(const char *s) 
{
	char temp[c_cbName];
	DWORD l = GetCfgInfo().GetCfgProfileString("Cores",s,s,temp,c_cbName);
	return ZString(temp,(int)l);
}
bool WinTrekClient::CfgIsOfficialCore(const char *s)
{
	char temp[c_cbName];
	DWORD l = GetCfgInfo().GetCfgProfileString("OfficialCores",s,"false",temp,c_cbName);
	return (_stricmp(temp,"true") == 0);
}
bool WinTrekClient::CfgIsOfficialServer(const char *name, const char *addr)
{
	char temp[c_cbName];
	DWORD l = GetCfgInfo().GetCfgProfileString("OfficialServers",name,"",temp,c_cbName);
	return (_stricmp(temp,addr) == 0);
}
// KGJV end

class AutoDownloadProgressDialogPopup : 
    public IPopup, 
    public EventTargetContainer<AutoDownloadProgressDialogPopup>,
    public IIntegerEventSink,
    public IAutoUpdateSink
{
private:
    TRef<Pane> m_ppane;
    TRef<ButtonPane>  m_pbuttonAbort;
    TRef<StringPane>  m_pstrpaneCurrentFile;
    TRef<StringPane>  m_pstrpaneApproxMinutes;

    TRef<ModifiableNumber> m_pModifiableNumberFileListPercent;
    TRef<ModifiableNumber> m_pModifiableNumberVerifyPercent;
    TRef<ModifiableNumber> m_pModifiableNumberDownloadPercent;

    TRef<IMessageBox> m_pmsgBox;

    unsigned long     m_cGrandTotalBytes; // total bytes of transfer
    int               m_cFilesCompleted;
    unsigned long     m_cTotalFiles;
        
    const char*       m_szPrevCurrentFile; // last file transfered
    bool              m_bAborted;
    IAutoUpdateSink * m_pSink;
    bool              m_bConnectToLobby; // connect to lobby once completed

public:

    
    AutoDownloadProgressDialogPopup(IAutoUpdateSink * pSink, bool bConnectToLobby) :
        m_cFilesCompleted(0),
        m_bAborted(false),
        m_pmsgBox(NULL),
        m_szPrevCurrentFile(NULL),
        m_bConnectToLobby(bConnectToLobby),
        m_pSink(pSink)
    {
        //
        // exports
        //
        TRef<INameSpace> pnsData = GetModeler()->CreateNameSpace("autodownloaddialogdata");

        pnsData->AddMember("FileListPercentDone", m_pModifiableNumberFileListPercent = new ModifiableNumber(0.0f));
        pnsData->AddMember("VerifyPercentDone",   m_pModifiableNumberVerifyPercent = new ModifiableNumber(0.0f));
        pnsData->AddMember("DownloadPercentDone", m_pModifiableNumberDownloadPercent = new ModifiableNumber(0.0f));

        //
        // Imports
        //
        Modeler* pmodeler = GetModeler();

        TRef<INameSpace> pns = pmodeler->GetNameSpace("autodownloaddialog");

        CastTo(m_ppane,                  pns->FindMember("AutoDownloadDialog"        ));
        CastTo(m_pbuttonAbort,           pns->FindMember("AutoDownloadAbortButton"  ));
        CastTo(m_pstrpaneCurrentFile,    pns->FindMember("AutoDownloadCurrentFileStringPane"  ));
        CastTo(m_pstrpaneApproxMinutes,  pns->FindMember("AutoDownloadApproxMinutes"  ));

		// mdvalley: '05 needs a pointer and class name for arg1. Should still work in '03
        AddEventTarget(&AutoDownloadProgressDialogPopup::OnButtonAbort, m_pbuttonAbort->GetEventSource());


        pmodeler->UnloadNameSpace(pns);
    }

    virtual ~AutoDownloadProgressDialogPopup() 
    {
    }

    ////////////////////////////////////////////////////////////////////
    //
    // IPopup methods
    //
    ////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    bool OnButtonAbort()
    {
        if(trekClient.m_pAutoDownload)
            trekClient.m_pAutoDownload->Abort();
        return true;
    }

    ////////////////////////////////////////////////////////////////////
    //    
    // Events associated with IAutoUpdateSink
    //
    ////////////////////////////////////////////////////////////////////

    void OnBeginDownloadProgressBar(unsigned cTotalBytes, int cFiles) 
    {
        m_cGrandTotalBytes = cTotalBytes;
        m_cTotalFiles = cFiles;
    };

    ////////////////////////////////////////////////////////////////////

    virtual void OnAutoUpdateSystemTermination(bool bErrorOccurred, bool bRestarting) 
    {
        if(m_pSink)
            m_pSink->OnAutoUpdateSystemTermination(bErrorOccurred, bRestarting);

        trekClient.m_pAutoDownload = NULL;

        if (!bErrorOccurred)
        {
            //
            // Make all bars full, if not already
            //
            m_pModifiableNumberFileListPercent->SetValue(1.0f);
            m_pModifiableNumberFileListPercent->Update();

            m_pModifiableNumberVerifyPercent->SetValue(1.0f);
            m_pModifiableNumberVerifyPercent->Update();

            m_pModifiableNumberDownloadPercent->SetValue(1.0f);
            m_pModifiableNumberDownloadPercent->Update();

            if (bRestarting)  
            {
                GetWindow()->SetWaitCursor();
                m_pmsgBox = CreateMessageBox("Restarting to complete update...", NULL, false);
                GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
            }
            else
            {
                // reconnect with same settings
                if(m_bConnectToLobby)
                   trekClient.ConnectToLobby(NULL);

                // this causes this class to be deleted
                GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
                GetWindow()->RestoreCursor();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////

    virtual void OnUserAbort() 
    {
        m_bAborted = true; 

        // trap multiple presses of Abort
        if(m_pmsgBox != NULL)
            return;

        m_pmsgBox = CreateMessageBox("AutoUpdate Aborted. ");

        m_pmsgBox->GetEventSource()->AddSink(IIntegerEventSink::CreateDelegate(this));

//        GetWindow()->screen(ScreenIDIntroScreen);

        Point point(c_PopupX, c_PopupY);
        Rect rect(point, point);
        GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, rect, false);
    };

    ////////////////////////////////////////////////////////////////////

    virtual void OnRetrievingFileListProgress(unsigned long nFileSize, unsigned long cCurrentBytes)
    {
        debugf("FileList.txt %d out of %d \n", cCurrentBytes, nFileSize);

        m_pModifiableNumberFileListPercent->SetValue(float(cCurrentBytes)/float(nFileSize));
        m_pModifiableNumberFileListPercent->Update();
    }

    ////////////////////////////////////////////////////////////////////

    virtual void OnAnalysisProgress(float fPercentDone) 
    {
        debugf("Verify %3.3f%% \n", 100.0f*fPercentDone);

        m_pModifiableNumberVerifyPercent->SetValue(float(fPercentDone));
        m_pModifiableNumberVerifyPercent->Update();
    }

    /*-------------------------------------------------------------------------
     * ShouldFilterFile()
     *-------------------------------------------------------------------------
     * Parameters:
     *      szFileName: file in question
     *
     * Returns:
     *      true iff file is not to be downloaded based on it's name.
     */
    bool ShouldFilterFile(const char * szFileName)
    {
        //
        // Skip special files
        //
#ifdef DEBUG
        if (_stricmp(szFileName, "AllegianceRetail.exe") == 0 ||
            _stricmp(szFileName, "AllegianceRetail.pdb") == 0 ||
            _stricmp(szFileName, "AllegianceRetail.map") == 0 ||
            _stricmp(szFileName, "AllegianceRetail.sym") == 0)
            return true;
#endif

#ifdef OPTIMIZED
        if (_stricmp(szFileName, "AllegianceDebug.exe") == 0 ||
            _stricmp(szFileName, "AllegianceDebug.pdb") == 0 ||
            _stricmp(szFileName, "AllegianceDebug.map") == 0 ||
            _stricmp(szFileName, "AllegianceDebug.sym") == 0)
            return true;
#endif

#if !defined(OPTIMIZED) || !defined(DEBUG)
        if (_stricmp(szFileName, "AllegianceTest.exe") == 0 ||
            _stricmp(szFileName, "AllegianceTest.pdb") == 0 ||
            _stricmp(szFileName, "AllegianceTest.map") == 0 ||
            _stricmp(szFileName, "AllegianceTest.sym") == 0)
            return true;
#endif
        return false;

    }

    ////////////////////////////////////////////////////////////////////

    virtual void OnProgress(unsigned long cTotalBytes, const char* szCurrentFile, unsigned long cCurrentFileBytes, unsigned nEstimatedSecondsLeft) 
    {
		{
	        char sz[80];

	        sprintf(sz, "%2.2f%%   %i  %s  %i\n", 100.0f*float(cTotalBytes)/float(m_cGrandTotalBytes), cTotalBytes, szCurrentFile, cCurrentFileBytes);
            g_pDebugLogger->Log(sz);
		}

        //
        // Detect current file change
        //
        if (szCurrentFile != NULL && szCurrentFile != m_szPrevCurrentFile)
        {
            m_cFilesCompleted++;

            char szBuffer[15];
            sprintf(szBuffer, " (%i/%i)", m_cFilesCompleted, m_cTotalFiles);
                           
            m_pstrpaneCurrentFile->SetString(ZString(szCurrentFile) + ZString(szBuffer));

            m_szPrevCurrentFile = szCurrentFile;
        }

        if (nEstimatedSecondsLeft != -1 && 
            m_pstrpaneApproxMinutes) // if they have an up-to-date art file
          m_pstrpaneApproxMinutes->SetString(ZString("Min Left ") + ZString((((int)nEstimatedSecondsLeft-1)/60)+1));

        m_pModifiableNumberDownloadPercent->SetValue(float(cTotalBytes)/float(m_cGrandTotalBytes));
        m_pModifiableNumberDownloadPercent->Update();
    }

    ////////////////////////////////////////////////////////////////////
    //    
    // Events associated with IFTPSessionUpdateSink
    //
    ////////////////////////////////////////////////////////////////////

    virtual void OnError(char * szErrorMessage) 
    {
        char * szBuffer = new char[strlen(szErrorMessage) + 100];

        sprintf(szBuffer, "AutoUpdate Error \n%s ", szErrorMessage);

        assert(m_pmsgBox == NULL);
         
        m_pmsgBox = CreateMessageBox(szBuffer);

        m_pmsgBox->GetEventSource()->AddSink(IIntegerEventSink::CreateDelegate(this));

//        GetWindow()->screen(ScreenIDIntroScreen);

        Point point(150, 8); // match this with point in OnBeginAutoUpdate()
        Rect rect(point, point);
        GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, rect, false);

        delete[] szBuffer;
    }

    ////////////////////////////////////////////////////////////////////
    //    
    // Events associated with IIntegerEventSink
    //
    ////////////////////////////////////////////////////////////////////

    bool OnEvent(IIntegerEventSource* pevent, int value)
    {
        //
        // User must have pressed Okay button after error or abort
        //
        assert(m_pmsgBox);
        GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);

        return false;
    }

};


IAutoUpdateSink * WinTrekClient::OnBeginAutoUpdate(IAutoUpdateSink * pSink, bool bConnectToLobby)
{
    // destroy any open popups
    if (!GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);

    GetWindow()->RestoreCursor();

    m_pAutoDownloadProgressDialogPopup = new AutoDownloadProgressDialogPopup(pSink, bConnectToLobby);
    Point point(150, 8); // match this with point in OnError()
    Rect rect(point, point);
    GetWindow()->GetPopupContainer()->OpenPopup(m_pAutoDownloadProgressDialogPopup, rect, false);

    return m_pAutoDownloadProgressDialogPopup;
}

bool WinTrekClient::ShouldCheckFiles()
{
    extern bool g_bCheckFiles;
    bool bTemp = g_bCheckFiles;
    g_bCheckFiles = false; // only check files once
    return bTemp;
}

void WinTrekClient::CreateMissionReq()
{
    GetWindow()->SetWaitCursor();
    TRef<IMessageBox> pmsgBox = CreateMessageBox("Creating a game...", NULL, false);
    GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    BaseClient::CreateMissionReq();
}
// KGJV #114
void WinTrekClient::ServerListReq()
{
	//if (!g_bQuickstart) {
    	GetWindow()->SetWaitCursor();
    	TRef<IMessageBox> pmsgBox = CreateMessageBox("Asking about servers and cores...", NULL, false);
    	GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
	//}
	BaseClient::ServerListReq();
}
void WinTrekClient::CreateMissionReq(const char *szServer,const char *szAddr, const char *szIGCStaticFile, const char *szGameName)
{
    GetWindow()->SetWaitCursor();
    TRef<IMessageBox> pmsgBox = CreateMessageBox("Creating a game...", NULL, false);
    GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
	BaseClient::CreateMissionReq(szServer,szAddr,szIGCStaticFile,szGameName);
}

void WinTrekClient::JoinMission(MissionInfo * pMission, const char* szMissionPassword)
{
    GetWindow()->SetWaitCursor();
    TRef<IMessageBox> pmsgBox = CreateMessageBox("Joining the game...", NULL, false);
    GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    BaseClient::JoinMission(pMission, szMissionPassword);
}


void WinTrekClient::OnLogonAck(bool fValidated, bool bRetry, LPCSTR szFailureReason)
{
    // close the "logging in" popup
    if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);
    GetWindow()->RestoreCursor();

    if (fValidated) 
    {
        GetClientEventSource()->OnLogonGameServer();
		/* pkk May 6th: Disabled bandwidth patch
		// w0dk4 June 2007: Bandwith Patch
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
		BEGIN_PFM_CREATE(this->m_fm, pfmBandwidth, C, BANDWIDTH)
		END_PFM_CREATE
		pfmBandwidth->value = this->m_nBandwidth;*/
    }
    else
    {
        Disconnect();
        GetClientEventSource()->OnLogonGameServerFailed(bRetry, szFailureReason);
        g_bQuickstart = false;
    }
}

void WinTrekClient::OnLogonLobbyAck(bool fValidated, bool bRetry, LPCSTR szFailureReason)
{
    //
    // We've just logged onto lobby server.
    //
    // close the "logging in" popup
    if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);
    GetWindow()->RestoreCursor();
        
    if (!fValidated) 
    {
        DisconnectLobby();
        GetClientEventSource()->OnLogonLobbyFailed(bRetry, szFailureReason);
        g_bQuickstart = false;
    }
    else 
    {
        GetClientEventSource()->OnLogonLobby();
    }
}


void WinTrekClient::OnLogonClubAck(bool fValidated, bool bRetry, LPCSTR szFailureReason)
{
    //
    // We've just logged onto Zone Club server.
    //
    // close the "logging in" popup
    if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);
    GetWindow()->RestoreCursor();
        
    if (!fValidated) 
    {
        DisconnectClub();
        GetClientEventSource()->OnLogonClubFailed(bRetry, szFailureReason);
        g_bQuickstart = false;
    }
    else 
    {
        GetClientEventSource()->OnLogonClub(); 
    }
}


void WinTrekClient::Disconnect(void) 
{
    BaseClient::Disconnect();
    m_bDisconnected = true;
}


HRESULT WinTrekClient::OnSessionLost(char* szReason, FedMessaging * pthis)
{
    m_strDisconnectReason = "";

    BaseClient::OnSessionLost(szReason, pthis);

    if (MyMission())
    {
        if (GetSide() && GetSideID() != SIDE_TEAMLOBBY)
            RemovePlayerFromSide(MyPlayerInfo(), QSR_LinkDead);
    }

    if (pthis == &m_fm)
    {
        ZString strMsg = "Your connection to the game server was lost.\n"
            "Reason: " + ZString(szReason) + ".\n";

        if (trekClient.GetIsLobbied())
        {
            if (!m_fmLobby.IsConnected())
            {
                GetWindow()->screen(ScreenIDGameScreen);
                m_strDisconnectReason = strMsg;
            }
            else
            {
                assert(GetWindow()->screen() == ScreenIDGameScreen);

                // close the "connecting..." popup
                if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
                    GetWindow()->GetPopupContainer()->ClosePopup(NULL);
                GetWindow()->RestoreCursor();

                TRef<IMessageBox> pmsgBox = 
                    CreateMessageBox(
                        "Your connection to the game server was lost.\n"
                        "Reason: " + ZString(szReason) + ".\n"
                    );

                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
        }
        else
        {
            // close any existing popup
            if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
                GetWindow()->GetPopupContainer()->ClosePopup(NULL);
            GetWindow()->RestoreCursor();

            if (Training::GetTrainingMissionID () == Training::c_TM_7_Live)
                GetWindow()->screen(ScreenIDTrainScreen);
            else
                GetWindow()->screen(ScreenIDIntroScreen);

            TRef<IMessageBox> pmsgBox = CreateMessageBox(strMsg);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
    }
    else if (pthis == &m_fmClub)
    {
        // close the "connecting..." popup
        if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
            GetWindow()->GetPopupContainer()->ClosePopup(NULL);
        GetWindow()->RestoreCursor();

        GetWindow()->screen(ScreenIDZoneClubScreen);

        TRef<IMessageBox> pmsgBox = 
            CreateMessageBox(
                "Your connection to the Club server was lost.\n"
                "Reason: " + ZString(szReason) + ".\n"
            );

        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }
    else if (pthis == &m_fmLobby)
    {
        // close the "connecting..." popup
        if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
            GetWindow()->GetPopupContainer()->ClosePopup(NULL);
        GetWindow()->RestoreCursor();

        GetWindow()->screen(ScreenIDZoneClubScreen);

        TRef<IMessageBox> pmsgBox = 
            CreateMessageBox(
                "Your connection to the lobby server was lost.\n"
                "Reason: " + ZString(szReason) + ".\n"
            );

        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        
        // if we lost the lobby while we were connecting to a server, we 
        // need to kill the server connection too.
        Disconnect();
    }
    else
        assert(false);

    if (MyMission())
    {
        RemovePlayerFromMission(MyPlayerInfo(), QSR_LinkDead);
    }

    return S_OK;
}

void WinTrekClient::FlushSessionLostMessage()
{
    if (!m_strDisconnectReason.IsEmpty())
    {
        TRef<IMessageBox> pmsgBox = CreateMessageBox(m_strDisconnectReason);
        m_strDisconnectReason = "";
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }
}

bool WinTrekClient::OnEvent(IIntegerEventSource* pevent, int value)
{
    Win32App::Exit(value);
    return true;
}

#ifndef NO_MSG_CRC
void WinTrekClient::OnBadCRC(FedMessaging * pthis, CFMConnection & cnxn, BYTE * pMsg, DWORD cbMsg)
{
  // uh, bad crc from the server? Let's just assert for now
  debugf("ACK! Bad crc *from* the %s!!\n", pthis == &m_fmLobby ? "Lobby" : "Server");
  assert(0);
}
#endif


HRESULT WinTrekClient::OnAppMessage(FedMessaging * pthis, CFMConnection & cnxnFrom, FEDMESSAGE * pfm)
{
    HRESULT hr = E_FAIL; 

    FEDMSGID fmid = pfm->fmid;
    ZAssert(0 != fmid);

    //if (fmid != FM_CS_PING && 
    //    fmid != FM_S_LIGHT_SHIPS_UPDATE &&
    //    fmid != FM_S_HEAVY_SHIPS_UPDATE &&
    //    fmid != FM_CS_CHATMESSAGE && 
    //    fmid != FM_S_STATIONS_UPDATE && 
    //    fmid != FM_S_PROBES_UPDATE)
    // debugf("Received %s at time %u\n", g_rgszMsgNames[fmid], m_now.clock()); mmf took this out, too much debug output

    if (m_bDisconnected && pthis == &m_fm)
    {
        debugf("Client is disconnected - message ignored.\n");
        hr = S_FALSE;
    }
    else 
    {
		// KGJV: fill in server ip for FM_S_MISSIONDEF
		// this is a bit hacky: we cant do this in HandleMsg where FM_S_MISSIONDEF is handled
		// because pthis and cnxnFrom are not available
		// and we cant do this server side either because of NAT/Firewall 
		if (pfm->fmid == FM_S_MISSIONDEF)
		{
			CASTPFM(pfmMissionDef, S, MISSIONDEF, pfm);
			//char szAddr[64];
			pthis->GetIPAddress(cnxnFrom, pfmMissionDef->szServerAddr); // get the real addr 
			//Strncpy(pfmMissionDef->szServerAddr,szAddr,16);
			//strcpy_s(pfmMissionDef->szServerAddr,16,szAddr); // IMAGO REVIEW CRASH 7/24/09
			//OutputDebugString("Connection from "+ZString(pfmMissionDef->szServerAddr)+"\n");
		}
		// KGJV: end

        hr = HandleMsg(pfm, m_lastUpdate, m_now);

        bool bWasHandled = hr == S_OK;

        // review: - we check m_fm.IsConnected() here because HandleMissionMessage might blow 
        // away the connection as a side effect of retrying a logon... should review
        if (SUCCEEDED(hr) && m_fm.IsConnected())
        {
            hr = GetWindow()->HandleMsg(pfm, m_lastUpdate, m_now);

            // someone had better handle the message (otherwise, why are we sending it?)
            // If you want to leave the message as NYI for the moment, add it to the NYI
            // section of HandleMsg in WinTrek.cpp so we can track it.  
            assert(bWasHandled || hr == S_OK);
        }
    }

    return(hr);
}

void    WinTrekClient::OverrideThrottle (float fDesiredThrottle)
{
    bInitTrekThrottle = true;
    trekThrottle = fDesiredThrottle;
}

//
// IgcSite implementation
//

void    WinTrekClient::DevelopmentCompleted(IbucketIGC*  b, IdevelopmentIGC*   d, Time now)
{
    assert (b);
    IsideIGC*   pside = b->GetSide();
    assert (pside);

    if (!m_fm.IsConnected())
    {
        pside->ApplyDevelopmentTechs(d->GetEffectTechs());
        pside->ApplyGlobalAttributeSet(d->GetGlobalAttributeSet());
    }
    else if ((!d->GetTechOnly()) && d->GetEffectTechs().GetAllZero())
    {
        trekClient.SideDevelopmentTechChange(pside);
    }

}

void WinTrekClient::StationTypeCompleted(IbucketIGC * pbucket, IstationIGC* pstation, IstationTypeIGC * pstationtype, Time now)
{
    if (!m_fm.IsConnected())
    {
        //NYI decide on the correct hack for the placement of space stations
        IsideIGC * pside = pbucket->GetSide();
        assert (pside);

        //Hack alert
        {
            IdroneTypeIGC*  dt = pstationtype->GetConstructionDroneType();

            IshipIGC*   pship = CreateDrone(m_pCoreIGC,
                                            trekClient.GetCore()->GenerateNewShipID(),
                                            dt->GetPilotType(),
                                            dt->GetName(),
                                            dt->GetHullTypeID(),
                                            pstation->GetSide(),
                                            0,
                                            dt->GetShootSkill(),
                                            dt->GetMoveSkill(),
                                            dt->GetBravery());

            if (pship)
            {
                pship->SetBaseData(pstationtype);

                pship->SetStation(pstation);
                pstation->Launch(pship);
            }
        }
    }
}

void WinTrekClient::BuildStation(IasteroidIGC* pasteroid, IsideIGC* pside, IstationTypeIGC* pstationtype, Time now, bool pbseensides[])
{
    DataStationIGC  ds;
    strcpy(ds.name, pstationtype->GetName());
    ds.clusterID    = pasteroid->GetCluster()->GetObjectID();
    ds.position     = pasteroid->GetPosition();
    ds.forward      = pasteroid->GetOrientation ().GetForward ();
    ds.up           = pasteroid->GetOrientation ().GetUp ();
    ds.rotation.axis(ds.forward);
    ds.rotation.angle(0.0f);
    ds.sideID        = pside->GetObjectID();
    ds.stationID     = m_pCoreIGC->GenerateNewStationID();
    ds.stationTypeID = pstationtype->GetObjectID();

    ds.bpHull = pasteroid->GetFraction();
    ds.bpShield = 0.0f;

    pasteroid->Terminate();

    IstationIGC * pstationNew = (IstationIGC *) (m_pCoreIGC->CreateObject(now, OT_station, &ds, sizeof(ds)));
    assert (pstationNew);
    pstationNew->Release();
}

void WinTrekClient::LayExpendable(Time now, IexpendableTypeIGC* pet, IshipIGC* pshipLayer)
{
    assert (pet);
    assert (pshipLayer);

    ObjectType  type = pet->GetObjectType();

    const Vector&   position = pshipLayer->GetPosition();
    IclusterIGC*    pcluster = pshipLayer->GetCluster();
    IsideIGC*       pside = pshipLayer->GetSide();

    if (type == OT_mineType)
    {
        DataMineIGC  dm;
        dm.pshipLauncher = pshipLayer;
        dm.psideLauncher = pside;
        dm.mineID = m_pCoreIGC->GenerateNewMineID();

        dm.time0 = now;
        dm.p0 = position;
        dm.pminetype = (ImineTypeIGC*)pet;
        assert (dm.pminetype);

        dm.pcluster = pcluster;
        dm.exportF = false;

        ImineIGC * m = (ImineIGC*)(m_pCoreIGC->CreateObject(now, OT_mine, &dm, sizeof(dm)));

        assert (m);
        m->Release();
    }
    else
    {
        assert (type == OT_probeType);

        DataProbeIGC dp; 
        dp.pside = pside;
        dp.pship = NULL;
        dp.pmodelTarget = NULL;
        dp.probeID = m_pCoreIGC->GenerateNewProbeID();

        dp.time0 = now;
        dp.p0 = position;
        dp.pprobetype = (IprobeTypeIGC*)pet;
        assert (dp.pprobetype);

        dp.pcluster = pcluster;
        dp.exportF = false;

        IprobeIGC * p = (IprobeIGC*)(m_pCoreIGC->CreateObject(now, OT_probe, &dp, sizeof(dp)));

        assert (p);
        p->Release();
    }

    //Quietly kill the ship (after nuking its parts to prevent treasure from being created)
    {
        const PartListIGC*  parts = pshipLayer->GetParts();
        PartLinkIGC*    plink;
        while (plink = parts->first())  //Not ==
            plink->data()->Terminate();
    }
    pshipLayer->SetAmmo(0);
    pshipLayer->SetFuel(0.0f);

    KillShipEvent(now, pshipLayer, NULL, 0.0f, position, Vector::GetZero());
}

void    WinTrekClient::DroneTypeCompleted(IbucketIGC*  b, IstationIGC* pstation, IdroneTypeIGC*   dt, Time now)
{
    if (!m_fm.IsConnected())
    {
        IshipIGC*   pship = CreateDrone(m_pCoreIGC,
                                        trekClient.GetCore()->GenerateNewShipID(),
                                        dt->GetPilotType(),
                                        dt->GetName(),
                                        dt->GetHullTypeID(),
                                        pstation->GetSide(),
                                        c_aabmMineHe3,              //Assume any purchased drone is an He3 miner
                                        dt->GetShootSkill(),
                                        dt->GetMoveSkill(),
                                        dt->GetBravery());

        if (pship)
        {
            pship->SetStation(pstation);
            pstation->Launch(pship);
        }
    }
}

bool WinTrekClient::ContinueRipcord(IshipIGC* pship, ImodelIGC*  pmodel)
{
    return true;
}

bool WinTrekClient::UseRipcord(IshipIGC* pship, ImodelIGC*  pmodel)
{
    assert (pmodel);
    if (!m_fm.IsConnected())
    {
        if (pmodel->GetObjectType() == OT_station)
            ((IstationIGC*)pmodel)->Launch(pship);
        else
        {
            //debug
            const char* name = pship->GetName();
            const char* modelName = pmodel->GetName();
            float r1 = pship->GetRadius();
            float r2 = pmodel->GetRadius();
            //

            float   r = pmodel->GetRadius() + pship->GetRadius() + 25.0f;
            Vector  v = Vector::RandomDirection();
            Orientation o(v);

            IclusterIGC*    pcluster = pmodel->GetCluster();
            Time    lastUpdate = pcluster->GetLastUpdate();

            pship->SetPosition(pmodel->GetPosition() + v * r);
            pship->SetVelocity(v * trekClient.m_pCoreIGC->GetFloatConstant(c_fcidExitStationSpeed));
            pship->SetOrientation(o);
            pship->SetCurrentTurnRate(c_axisYaw, 0.0f);
            pship->SetCurrentTurnRate(c_axisPitch, 0.0f);
            pship->SetCurrentTurnRate(c_axisRoll, 0.0f);

            pship->SetLastUpdate(lastUpdate);
            pship->SetBB(lastUpdate, lastUpdate, 0.0f); 

            pship->SetCluster(pcluster);
        }
        
        if (pship == trekClient.GetShip()) {
            trekClient.PlaySoundEffect(jumpSound, static_cast<ImodelIGC*>(pship)); //Not using trekClient's PlaySoundEffect messes up system sound over time. Persistent after quiting Allegiance.
            trekClient.PostText(true, "");
        }

        //Set the player data for the minimap
        if (pship->SeenBySide(trekClient.GetSide())) {
            PlayerInfo* ppi = (PlayerInfo*)(((IshipIGC*)pship)->GetPrivateData());
            if (ppi) {
                ShipStatus ss = ppi->GetShipStatus();
                ZAssert(pmodel->GetCluster());
                ss.SetSectorID(pmodel->GetCluster()->GetObjectID());
                ppi->SetShipStatus(ss);
            }
        }

        return true;
    }

    return false;
}

void WinTrekClient::SetViewCluster(IclusterIGC* pcluster, const Vector* pposition)
{
    //Pretend the server sends a ship delete message for everything the player could see
    IclusterIGC*    pclusterOld = trekClient.GetViewCluster();
    if (pclusterOld != pcluster)
    {
        if (pclusterOld)
        {
            const ShipListIGC*  ships = m_pCoreIGC->GetShips();
            assert (ships);

            if (m_fm.IsConnected ())
            {
                for (ShipLinkIGC*   l = ships->first();
                     (l != NULL);
                     l = l->next())
                {
                    ImodelIGC*  m = l->data();
                    m->SetCluster(NULL);
                }
            }

            // stop the sounds on every object in the sector
            for (ModelLinkIGC*   pml = pclusterOld->GetModels()->first(); (pml != NULL); pml = pml->next())
            {
                ImodelIGC*   pmodel = pml->data();
                ThingSite*  pts = pmodel->GetThingSite();
                if (pts)
                {
                    ((ThingSiteImpl*)pts)->StopSounds();
                }
            }

            if (m_fm.IsConnected())
                pclusterOld->SetActive(false);
        }

		//Xynth #225 Inhibit updates for all asteroids in the new cluster for a few seconds		
		if (pcluster)
		{
		for (AsteroidLinkIGC* pal = pcluster->GetAsteroids()->first(); (pal != NULL); pal = pal->next())
		{
			IasteroidIGC* pAsteroid = pal->data();
			if ((pAsteroid->GetCapabilities() & c_aabmMineHe3) != 0)
				pAsteroid->SetInhibitUpdate(true);
		}
		}
        BaseClient::SetViewCluster(pcluster);
        GetWindow()->SetCluster(pcluster);
        GetWindow()->PositionCommandView(pposition, 0.0f);
        trekClient.GetClientEventSource()->OnClusterChanged(pcluster);

        if (m_fm.IsConnected() && pcluster)
            pcluster->SetActive(true);
    }
    else
    {
        GetWindow()->PositionCommandView(pposition, 0.0f);
    }
}

void WinTrekClient::RequestViewCluster(IclusterIGC* pcluster, ImodelIGC* pmodelTarget)
{
    if (m_fm.IsConnected() && (trekClient.GetCluster() != pcluster))
    {
        if (pcluster == NULL)
            SetViewCluster(pcluster);

        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmViewCluster, C, VIEW_CLUSTER)
        END_PFM_CREATE

        pfmViewCluster->clusterID = pcluster ? pcluster->GetObjectID() : NA;
        if (pmodelTarget)
        {
            pfmViewCluster->otTarget = pmodelTarget->GetObjectType();
            pfmViewCluster->oidTarget = pmodelTarget->GetObjectID();
        }
        else
        {
            pfmViewCluster->otTarget = NA;
            pfmViewCluster->oidTarget = NA;
        }
    }
    else
    {
        SetViewCluster(pcluster, pmodelTarget && pmodelTarget->SeenBySide(trekClient.GetSide()) ? &(pmodelTarget->GetPosition()) : NULL);
    }
}

void WinTrekClient::TerminateModelEvent(ImodelIGC* pmodel)
{
    TargetKilled(pmodel);

    BaseClient::TerminateModelEvent(pmodel);
}

bool WinTrekClient::HandlePickDefaultOrder(IshipIGC* pship) {
    if (!m_fm.IsConnected()) {
        return Training::HandlePickDefaultOrder(pship);
    }
    return false;
}

bool WinTrekClient::DockWithStationEvent(IshipIGC* pShip, IstationIGC* pStation)
{
    if (!m_fm.IsConnected())
    {
        // full fuel and ammo
        pStation->RepairAndRefuel (pShip);

		pShip->SetStateBits(keyMaskIGC, 0);  //Xynth #210 8/2010
        if ((pShip != GetShip ()) || Training::ShipLanded ())
        {
            IstationIGC*    pOldStation = pShip->GetStation ();
            pShip->SetStation (pStation);

            if (pShip == GetShip()) {
                // let anyone who can see us know that we are docked
                if ((pShip->GetSide() == GetSide() || pShip->GetSide()->AlliedSides(pShip->GetSide(), GetSide())) || // #ALLY Imago 7/8/09 VISIBILITY?
                    (pShip->SeenBySide(GetSide()) && pStation->SeenBySide(GetSide())))
                {
                    SideID      sideID = pShip->GetSide()->GetObjectID();
                    PlayerInfo* ppi = (PlayerInfo*)pShip->GetPrivateData();
                    ShipStatus ss = ppi->GetShipStatus();

                    ss.SetStateTime(m_lastUpdate.clock());
                    ss.SetState(c_ssDocked);
                    ss.SetParentID(NA);
                    ss.SetStationID(pStation->GetObjectID());

                    ppi->SetShipStatus(ss);

                    /*//Adjust the ship status for all of the children as well
                    {
                        for (ShipLinkIGC* psl = pShip->GetChildShips()->first(); (psl != NULL); psl = psl->next()) {
                            IshipIGC*   pship = psl->data();
                            PlayerInfo* ppi2 = (PlayerInfo*)pship->GetPrivateData();
                            ShipStatus ss2 = ppi->GetShipStatus();
                            ss2.SetStateTime(m_lastUpdate.clock());
                            ss2.SetStationID(pStation->GetObjectID());
                            ppi2->SetShipStatus(ss2);
                        }
                    }*/
                }
            }
            else {
                //relaunch miners
                pShip->SetStation (pOldStation);
                pStation->Launch (pShip);
            }
		}
    }
    return true;
}

bool WinTrekClient::LandOnCarrierEvent(IshipIGC* pshipCarrier, IshipIGC* pship, float tCollision) {
    if (!m_fm.IsConnected())
    {
        trekClient.RestoreLoadoutOnCarrier(pship);

        pship->SetAmmo(SHRT_MAX);
        pship->SetFuel(FLT_MAX);
        pship->SetEnergy(pship->GetHullType()->GetMaxEnergy());
        pship->SetFraction(1.0f);
        {
            IshieldIGC* pshield = (IshieldIGC*)(pship->GetMountedPart(ET_Shield, 0));
            if (pshield)
                pshield->SetFraction(1.0f);
        }

        const Orientation&  orientation = pshipCarrier->GetOrientation();

        float   vLaunch = m_pCoreIGC->GetFloatConstant(c_fcidExitStationSpeed);

        Vector  position = pshipCarrier->GetPosition();
        Vector  velocity = pshipCarrier->GetVelocity();

        position.x += random(-0.5f, 0.5f);
        position.y += random(-0.5f, 0.5f);
        position.z += random(-0.5f, 0.5f);

        IhullTypeIGC*   phtCarrier = pshipCarrier->GetBaseHullType();

        Orientation         orientationBfr;
        const Orientation*  porientation;
        if (phtCarrier->GetLaunchSlots() == 0)
        {
            position -= orientation.GetBackward() * (pshipCarrier->GetRadius() + pship->GetRadius() + vLaunch * 0.5f);
            velocity -= orientation.GetBackward() * vLaunch;

            porientation = &orientation;
        }
        else
        {
            short   slot = pshipCarrier->GetLaunchSlot();

            position += phtCarrier->GetLaunchPosition(slot) * orientation;

            Vector  forward = phtCarrier->GetLaunchDirection(slot) * orientation;

            position += forward * (pship->GetRadius() + vLaunch * 0.5f);
            velocity += forward * vLaunch;

            orientationBfr = orientation;
            orientationBfr.TurnTo(forward);

            porientation = &orientationBfr;
        }

        pship->SetPosition(position);
        pship->SetVelocity(velocity);
        pship->SetOrientation(*porientation);

        IclusterIGC*    pcluster = pship->GetCluster();
        pcluster->RecalculateCollisions(tCollision, pship, NULL);

        if (pship == trekClient.GetShip()) {
            PlayNotificationSound(salRepairedAtCarrierSound, GetShip());
            OverrideCamera(pshipCarrier);
        }

        if (pship->GetAutopilot()) {
            if (pship->GetCommandTarget(c_cmdAccepted) == (ImodelIGC*)pshipCarrier && pship->GetCommandID(c_cmdAccepted) == c_cidGoto) {
                pship->SetCommand(c_cmdAccepted, pshipCarrier, c_cidDefend);
                if (pship->GetPilotType() >= c_ptPlayer)
                    pship->SetCommand(c_cmdPlan, pshipCarrier, c_cidDefend);
            }
            else
                pship->SetCommand(c_cmdPlan, NULL, c_cidNone); // drones will do c_cmdAccepted
        }
    }
    return true; 
}

void WinTrekClient::KillShipEvent(Time now, IshipIGC* pShip, ImodelIGC* pLauncher, float flAmount, const Vector& p1, const Vector& p2)
{
    if (Training::IsTraining ())
    {
        if (pShip->GetPilotType () != c_ptLayer)
            pShip->GetCluster()->GetClusterSite()->AddExplosion(pShip, c_etSmallShip);

        if (pShip == GetShip())
            Training::ShipDied (pLauncher);
        else {
            BaseClient::KillShipEvent(now, pShip, pLauncher, flAmount, p1, p2);
            Training::ShipKilled(pShip, pLauncher);
        }
    }
}

void WinTrekClient::DamageShipEvent(Time now,
                                IshipIGC * pShip,
                                ImodelIGC * pcredit,
                                DamageTypeID type, 
                                float flAmount,
                                float flLeakage,
                                const Vector& p1,
                                const Vector& p2)
{
    if (pcredit)
    {
        if (pShip == trekClient.GetShip()->GetSourceShip() && flAmount > 0.0f)
        {
            GetWindow()->SetTimeDamaged(now);

            if (flAmount > 0.0f)
            {
                if (pcredit->GetSide() && pcredit->GetSide() != GetSide())
                {
                    // damaged by an enemy - adjust the groove level
                    m_nGrooveLevel = std::max(m_nGrooveLevel, 2);
                    m_vtimeGrooveDrops[2] = Time::Now() + c_fGrooveLevelDuration;
                }
            }

            const Vector& vRight = pShip->GetOrientation().GetRight();
            const Vector& vBackward = pShip->GetOrientation().GetBackward();
            Vector  vDeltaP = p2 - p1;
            float dotBackward = vBackward*vDeltaP;
            float dotRight = vRight*vDeltaP;
            long direction = ((long)DegreesFromRadians(atan2(dotBackward, dotRight)) + 90);
//            debugf("Impact @ %d degrees.", direction);
            GetWindow()->PlayFFEffect(effectBounce, direction);
        }

        // if we might hear it, queue up the sound effect
        if (pShip->GetCluster() == trekClient.GetCluster())
        {
            ThingSiteImpl* pts = ((ThingSiteImpl*)pShip->GetThingSite());

            if (pts)
            {
                pts->RegisterHit(
                    flAmount, 
                    pShip->GetOrientation().TimesInverse(p2 - p1), 
                    flLeakage == 0.0f
                    );
            }
        }
    }
}

void WinTrekClient::KillStationEvent(IstationIGC* pStation, ImodelIGC* pLauncher, float amount)
{
    debugf("%s KillStationEvent\n", pStation->GetName());
    if (!m_fm.IsConnected())
    {
        pStation->GetCluster()->GetClusterSite()->AddExplosion(pStation, c_etLargeStation);
        
        pStation->GetCluster()->GetClusterSite()->AddExplosion(pStation,
            pStation->GetStationType()->HasCapability(c_sabmFlag)
            ? c_etLargeStation
            : c_etSmallStation);

        if (pStation->GetSide() == trekClient.GetSide())
            trekClient.PlaySoundEffect(pStation->GetStationType()->GetDestroyedSound());
        else
            trekClient.PlaySoundEffect(pStation->GetStationType()->GetEnemyDestroyedSound());

        trekClient.PostText(true, START_COLOR_STRING "%s" END_COLOR_STRING " has destroyed " START_COLOR_STRING "%s's %s" END_COLOR_STRING,
            (PCC)ConvertColorToString(pLauncher ? pLauncher->GetSide()->GetColor() : Color::White()),
            (pLauncher ? pLauncher->GetName() : "Unknown"),
            (PCC)ConvertColorToString(pStation ? pStation->GetSide()->GetColor() : Color::White()),
            (pStation ? pStation->GetSide()->GetName() : "unknown side"),
            (pStation ? pStation->GetName() : "unknown station")
        );
        /*trekClient.PostText(true, "The " START_COLOR_STRING "%s" END_COLOR_STRING " station has been destroyed.",
            (PCC)ConvertColorToString(pStation ? pStation->GetSide()->GetColor() : Color::White()),
            (pStation ? pStation->GetName() : "Unknown station")
        );*/

        Training::KillStationEvent(pStation, pLauncher);

        if (GetShip()->GetStation() == pStation) {
            //played docked at this station, launch
            GetShip()->SetStation(NULL);
            pStation->Launch(GetShip());
        }

        pStation->Terminate();

        if (pLauncher)
            pLauncher->GetSide()->AddBaseKill();
    }
}

void WinTrekClient::HitWarpEvent(IshipIGC* ship, IwarpIGC* warp)
{
    //
    // This code is offline only.  Any future online code should be
    // added to the base client.
    //
    if (!m_fm.IsConnected())
    {
        IwarpIGC*   destination = warp->GetDestination();
        if (destination)
        {
            IclusterIGC*    cluster = destination->GetCluster();
            assert (cluster);

            ship->SetCluster(cluster);

            {
                Orientation alephOrientation = destination->GetOrientation();
                const Vector&   v = ship->GetVelocity();
                float           speed2 = v.LengthSquared();
                float           speed  = float(sqrt(speed2));
                float           error;
                {
                    //How close is the ship coming to the center of the warp?
                    Vector          dp = warp->GetPosition() - ship->GetPosition();

                    float   t = (dp * v) / v.LengthSquared();
                    float   d = (dp - t * v).LengthSquared();
                    float   r = warp->GetRadius();

                    error = d / (r*r) + 0.125f;      //Error ranges from 0.125 to 1.125
                }

                alephOrientation.Pitch(random(-error, error));
                alephOrientation.Yaw(random(-error, error));

                ship->SetCurrentTurnRate(c_axisRoll,
                                         ship->GetCurrentTurnRate(c_axisRoll) +
                                         random(pi * 0.5f * error, pi * 2.0f * error));
                const Vector&   backward = alephOrientation.GetBackward();
                ship->SetOrientation(alephOrientation);

                speed = -(speed + trekClient.m_pCoreIGC->GetFloatConstant(c_fcidExitWarpSpeed));
                ship->SetVelocity(backward * speed);
                ship->SetPosition(destination->GetPosition() +
                                  (alephOrientation.GetUp() * random(2.0f, 5.0f)) +
                                  (alephOrientation.GetRight() * random(2.0f, 5.0f)) -
                                  (ship->GetRadius() + 5.0f) * backward);
                if (ship == trekClient.GetShip())
                    trekClient.PlaySoundEffect(jumpSound, static_cast<ImodelIGC*>(ship));

                //Reset the ship's trail
                assert(ship->GetThingSite());
                ship->GetThingSite()->SetTrailColor(ship->GetSide()->GetColor());

                //Set the player data for the minimap
                if (ship->SeenBySide(trekClient.GetSide())) {
                    PlayerInfo* ppi = (PlayerInfo*)(((IshipIGC*)ship)->GetPrivateData());
                    if (ppi) {
                        ShipStatus ss = ppi->GetShipStatus();
                        ss.SetStateTime(m_lastUpdate.clock());
                        ss.SetSectorID(cluster->GetObjectID());
                        ss.SetState(c_ssFlying);
                        ppi->SetShipStatus(ss);
                    }
                }

                {
                    Time    t = ship->GetLastUpdate();
                    ship->SetBB(t, t, 0.0f);
                }
            }
        }
    }
}


bool WinTrekClient::HitTreasureEvent(Time now, IshipIGC* ship, ItreasureIGC* treasure)
{
    //
    // This code is offline only.  Any future online code should be
    // added to the base client.
    //
    if (!m_fm.IsConnected())
    {
        ship->HitTreasure(treasure->GetTreasureCode(), treasure->GetBuyable()->GetObjectID(), treasure->GetAmount());
        return true;
    }
    else
        return false;
}

void WinTrekClient::PaydayEvent(IsideIGC* pside, float money)
{
}

void    WinTrekClient::GetMoneyRequest(IshipIGC*    pshipSender, Money  amount, HullID hidFor)
{
    GetWindow()->SetQueuedCommand(pshipSender, amount, hidFor);
}

void      WinTrekClient::ReceiveChat(IshipIGC*   pshipSender,
                                     ChatTarget  ctRecipient,
                                     ObjectID    oidRecipient,
                                     SoundID     idSonicChat,
                                     const char* pszText,
                                     CommandID   cid,
                                     ObjectType  otTarget,
                                     ObjectID    oidTarget,
                                     ImodelIGC*  pmodelTarget,
                                     bool        bObjectModel)
{
    assert (ctRecipient != CHAT_GROUP_NOECHO);

    bool bIsLeader = false;

    if (pshipSender)
    {
		//TheBored 25-JUN-07: Checking to see if admin is PMing the user. If so, bypass the filter.
		bool bPrivilegedUserPM = false;
		PlayerInfo* ppi = (PlayerInfo*)(pshipSender->GetPrivateData());
        if((ctRecipient == CHAT_INDIVIDUAL) && (UTL::PrivilegedUser(ppi->CharacterName(),trekClient.m_pMissionInfo->GetCookie()))) //Imago 6/10 #2
		{
			bPrivilegedUserPM = true;
		}
		
		if (ppi->GetMute() 
            || (m_bFilterChatsToAll && ctRecipient == CHAT_EVERYONE && trekClient.IsInGame())
//          || (m_bFilterQuickComms && ppi->IsHuman() && idSonicChat != NA && ctRecipient != CHAT_INDIVIDUAL)		// mdvalley: commented out
            || ((((m_dwFilterLobbyChats == 1) && (ctRecipient != CHAT_INDIVIDUAL)) || (m_dwFilterLobbyChats == 2)) && (ppi->SideID() == SIDE_TEAMLOBBY) && (trekClient.IsInGame()) && (!bPrivilegedUserPM)) //TheBored 25-JUN-07: Changed conditions for the lobby mute options.
			|| (m_bFilterUnknownChats && (ppi->IsHuman()) && (pszText == NULL) && (idSonicChat != NA) && (GetWindow()->GetSonicChatText(idSonicChat, 0) == "Unknown chat"))) //TheBored 20-JUL-07: Don't display unknown VCs.
            return;
        bIsLeader = ppi->IsTeamLeader();
    }
    else if (ctRecipient == CHAT_ADMIN)
        bIsLeader = true;

    if ((cid != c_cidNone) &&
        ((trekClient.m_pCoreIGC->GetMissionStage() != STAGE_STARTED) ||
         (trekClient.GetSideID() < 0)))
        return;

    bool bIsSonicChat = false;

    if (idSonicChat != NA)
    {
        if (pszText == NULL)
        {
            // make sure the string sticks around through the entire function
            pszText = GetWindow()->GetSonicChatText(idSonicChat, 0);
            bIsSonicChat = true;

            // if quick coms are being filtered, at least don't play the sound
            if (m_bFilterQuickComms && pshipSender 
                && ((PlayerInfo*)(pshipSender->GetPrivateData()))->IsHuman())
                idSonicChat = NA;
        }
    } 

    // only play a sound if the chat would be visible in the curret chat window
    if (ctRecipient == CHAT_INDIVIDUAL
        || GetWindow()->screen() == ScreenIDCombat
        || GetWindow()->GetLobbyChatTarget() == ctRecipient)
    {
        if (bIsSonicChat && idSonicChat != NA)
        {
            GetWindow()->PlaySonicChat(idSonicChat, 0);
        }
        else if (idSonicChat != NA)
        {
            PlaySoundEffect(idSonicChat);
        }
        else if ((cid != c_cidNone) && (ctRecipient != CHAT_INDIVIDUAL_ECHO))
        {
            PlaySoundEffect(newCommandMsgSound);
        }
        else if (CHAT_INDIVIDUAL == ctRecipient
            && ((NA == oidRecipient) || (trekClient.GetShipID() == oidRecipient)))
        {
            if (Training::IsTraining())
                PlaySoundEffect(newChatMsgFromCommanderSound);
            else
                PlaySoundEffect(newPersonalMsgSound);
        }
        else
        {
            if (m_fm.IsConnected())
            {
                if (bIsLeader && (pshipSender != trekClient.GetShip()))
                    PlaySoundEffect(newChatMsgFromCommanderSound);
                else
                    PlaySoundEffect(newChatMsgSound);
            }
            else
                PlaySoundEffect(newOfflineChatMsgSound);
        }
    }

    if (pmodelTarget == NULL)
        pmodelTarget = trekClient.m_pCoreIGC->GetModel(otTarget, oidTarget);

    if (Training::IsTraining ())
    {
        // prevent players from giving commands to themselves
        if (pshipSender && (oidRecipient == pshipSender->GetObjectID ()) && !Training::CommandViewEnabled())
            pmodelTarget = NULL;

        // send out the chat we are getting to see if we are waiting for it...
        Training::RecordChat (ctRecipient);
    }

    bool    bForMe;

    ZString     strSender;
    ZString     strRecipient;
    ZString     strOrder;

    {
        Color   color;

        switch (ctRecipient)
        {
            case CHAT_NOSELECTION:
            {
                static const ZString c_strNone = "none";
                strRecipient = c_strNone;
                bForMe = false;
            }
            break;

            case CHAT_EVERYONE:
            {
                static const ZString c_strEveryone = "all";
                strRecipient = c_strEveryone;
                bForMe = true;
            }
            break;

            case CHAT_ALLIES: //ALLY imago 7/4/09
            {
                static const ZString c_strAllies = "allies";
                strRecipient = c_strAllies;
                bForMe = true;    
            }
            break;

            case CHAT_LEADERS:
            {
                static const ZString c_strLeaders = "leaders";
                strRecipient = c_strLeaders;
                bForMe = false;     //NYI
            }
            break;

            case CHAT_GROUP:
            {
                static const ZString c_strGroup = "group";
                strRecipient = c_strGroup;
                bForMe = oidRecipient == trekClient.GetShipID();
            }
            break;

            case CHAT_SHIP:
            {
                static const ZString c_strShip = "ship";
                strRecipient = c_strShip;
                bForMe = true;
            }
            break;

            case CHAT_TEAM:
            {
                IsideIGC*   pside = trekClient.GetShip()->GetSide();
                if ((oidRecipient == NA) || (oidRecipient == pside->GetObjectID()))
                {
                    strRecipient = pside->GetName();
                    bForMe = true;
                }
                else
                {
                    strRecipient = trekClient.m_pCoreIGC->GetSide(oidRecipient)->GetName();
                    bForMe = false;
                }
            }
            break;

            case CHAT_ALL_SECTOR:
            {
                //NYI need to distinguish between all and friendly
            }
            case CHAT_FRIENDLY_SECTOR:
            {
                IclusterIGC*    pcluster = trekClient.GetChatCluster();
                if (!pcluster)
                {
                    strRecipient = "<unknown>";
                    bForMe = false;
                }
                else if ((oidRecipient == NA) || (oidRecipient == pcluster->GetObjectID()))
                {
                    assert (pcluster);
                    strRecipient = pcluster->GetName();
                    bForMe = true;
                }
                else
                {
                    strRecipient = trekClient.m_pCoreIGC->GetCluster(oidRecipient)->GetName();
                    bForMe = false;
                }
            }
            break;

            case CHAT_ADMIN:
            {
                static const ZString c_strAdmin = "admin";
                strRecipient = c_strAdmin;
                bForMe = false;
            }
            break;
            case CHAT_WING:
            {
                WingID wid = trekClient.GetShip()->GetWingID();
                if ((oidRecipient == NA) || (oidRecipient == wid))
                {
                    bForMe = true;
                }
                else 
                {
                    wid = oidRecipient;
                    bForMe = false;
                }

				// BT - 9/17 - Prevent crashes in the Training missions when the pilot is not on any particular wing.
				if (wid == NA)
					wid = 0;

                strRecipient = c_pszWingName[wid];
            }
            break;

            case CHAT_INDIVIDUAL_ECHO:
            case CHAT_INDIVIDUAL:
            {
                ShipID sid = trekClient.GetShipID();
                IshipIGC*   pshipRecipient;
                if ((oidRecipient == NA) || (oidRecipient == sid))
                {
                    strRecipient = trekClient.GetShip()->GetName();
                    bForMe = true;

                    if ((cid == c_cidDefault) && pmodelTarget)
                        cid = trekClient.GetShip()->GetDefaultOrder(pmodelTarget);
                }
                else
                {
                    pshipRecipient = trekClient.m_pCoreIGC->GetShip(oidRecipient);
                    strRecipient = pshipRecipient->GetName();
                    bForMe = false;

                    if ((cid == c_cidDefault) && pmodelTarget)
                        cid = pshipRecipient->GetDefaultOrder(pmodelTarget);
                }
            }
            break;
        }

        if (pshipSender)
        {
            strSender = pshipSender->GetName();
            color = pshipSender->GetSide()->GetColor();

            if ((pshipSender == trekClient.GetShip()) && !bForMe)
                color = color * 0.75f;
            else if (cid == c_cidNone)
                color = color * 0.85f;
        }
        else
        {
            color = trekClient.GetSide()->GetColor();
            static const ZString  c_strHQ = "HQ";
            strSender = c_strHQ;

            bForMe = true;
        }

        if (pszText)
        {
            strOrder = pszText;
        }
        else
        {
            assert (cid >= 0);
            strOrder += c_cdAllCommands[cid].szVerb;
            if (pmodelTarget)
            {
                const char*   modelname = GetModelName (pmodelTarget);
                strOrder += " ";
                strOrder += modelname;

                ObjectType  type = pmodelTarget->GetObjectType();
                if ((type != OT_buoy) || (((IbuoyIGC*)pmodelTarget)->GetBuoyType() != c_buoyCluster))
                {
                    IclusterIGC*  pcluster = trekClient.GetCluster(trekClient.GetShip(), pmodelTarget);
                    if (pcluster)
                    {
                        strOrder += " in ";
                        strOrder += pcluster->GetName();
                    }
                }
            }

            if (ctRecipient == CHAT_INDIVIDUAL_ECHO)
            {
                assert (!bForMe);
                cid = c_cidNone;
                pmodelTarget = NULL;
            }
        }

        if ((cid == c_cidNone) ||
            (ctRecipient == CHAT_INDIVIDUAL_ECHO) ||
            ((bForMe && (ctRecipient == CHAT_INDIVIDUAL)) == (pshipSender != trekClient.GetShip())))
        {

            // make sure we limit the number of chats
            int nMaxChats = 600;
            while (trekClient.GetChatList()->n() >= nMaxChats)
            {
                assert(trekClient.GetChatList()->n() == nMaxChats);
                ChatLink*   lOldestChat = trekClient.GetChatList()->first();
                trekClient.GetClientEventSource()->OnDeleteChatMessage(&lOldestChat->data());
                delete lOldestChat;
            }
            
            bool bFromPlayer = pshipSender 
                               && (!pshipSender->GetPrivateData()
                               || ((PlayerInfo*)pshipSender->GetPrivateData())->IsHuman());

            ChatLink*   l = new ChatLink;
            assert (l);

            static const ZString c_str1(" (");
            static const ZString c_str2("): ");

			l->data().SetChat(ctRecipient, strSender + c_str1 + strRecipient + c_str2 + strOrder,
                              c_cidNone, pmodelTarget, color, bFromPlayer, bObjectModel, bIsLeader);
            trekClient.GetChatList()->last(l);

            m_pChatLogger->Log(std::string(strSender + c_str1 + strRecipient + c_str2 + strOrder));

            BaseClient::ReceiveChat(pshipSender,
                                    ctRecipient, oidRecipient,
                                    idSonicChat, pszText,
                                    cid, otTarget, oidTarget, pmodelTarget);
        }
    }

    if (bForMe)
    {
	// TE: Removed cheat-board
    ////NYI hack to piggify a pilot
    //    if ((cid == c_cidNone) && pszText && (strncmp(pszText, "cheat-", 6) == 0))
    //    {
    //        pszText += 6;
    //        if (strncmp(pszText, "board ", 6) == 0)
    //        {
    //            //Find a matching player
    //            pszText += 6;

    //            IshipIGC*   pship = NULL;
    //            for (ShipLinkIGC*   psl = trekClient.GetSide()->GetShips()->first();
    //                 (psl != NULL);
    //                 psl = psl->next())
    //            {
    //                if (strcmp(pszText, psl->data()->GetName()) == 0)
    //                {
    //                    pship = psl->data();
    //                    break;
    //                }
    //            }

    //            if (pship)
    //            {
    //                trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
    //                BEGIN_PFM_CREATE(trekClient.m_fm, pfmBoardShip, C, BOARD_SHIP)
    //                END_PFM_CREATE

    //                pfmBoardShip->sidParent = pship->GetObjectID();
    //            }
    //        }
    //        else if (strncmp(pszText, "log ", 4) == 0)
    //        {
    //            pszText += 4;
    //            GetWindow()->SetShowFPS(*pszText == '1',
    //                                    *(pszText + 1) == '\0' ? NULL : pszText + 1);
    //        }
    //    }

        if (pmodelTarget && trekClient.GetShip()->LegalCommand(cid, pmodelTarget))
        {

            Command cmd = (ctRecipient == CHAT_INDIVIDUAL) && (pshipSender == trekClient.GetShip())
                          ? c_cmdAccepted
                          : c_cmdQueued;

            if (cmd == c_cmdAccepted)
            {
                GetWindow()->SetAccepted(pmodelTarget, cid);
                GetWindow()->SetTarget(pmodelTarget, cid);

                if (trekClient.GetShip()->GetCluster() &&
                    (trekClient.GetShip()->GetParentShip() == NULL) &&
                    trekClient.GetCluster(trekClient.GetShip(), pmodelTarget))
                {
                    trekClient.SetAutoPilot(true);
                    trekClient.bInitTrekJoyStick = true;
                    PlaySoundEffect(salAutopilotEngageSound);
                }
            }
            else
            {
                GetWindow()->SetQueuedCommand(pshipSender, cid, pmodelTarget);
                if (pshipSender != trekClient.GetShip())
                {
					//Xynth #14 7/2010
					PlayerInfo* ppi = (PlayerInfo*)(pshipSender->GetPrivateData());					

                    if ((cid == c_cidPickup) && (pmodelTarget == pshipSender) &&
                        pshipSender->  GetBaseHullType()->HasCapability(c_habmRescue))
                    {
                        trekClient.PostText(true, "New orders from %s: prepare for recovery. Press [insert] to accept.", 
                                            (const char*)strSender);
                    }
                    else
						if (ppi->IsTeamLeader())  //Xynth #14 7/2010 change color if from comm
							trekClient.PostText(true, "\x81 " + ConvertColorToString(Color::Orange()) + "New orders from %s to %s: %s. Press [insert] to accept." + END_COLOR_STRING, 
                                            (const char*)strSender, (const char*)strRecipient, (const char*)strOrder);
						else
							trekClient.PostText(true, "New orders from %s to %s: %s. Press [insert] to accept.", 
                                            (const char*)strSender, (const char*)strRecipient, (const char*)strOrder);
                }
            }
        }
    }
}

void            WinTrekClient::Preload(const char*  pszModelName,
                                       const char*  pszTextureName)
{
// BUILD_DX9
	bool bOldColorKeyValue = GetModeler()->SetColorKeyHint( false );
	GetEngine()->SetEnableMipMapGeneration( true );
// BUILD_DX9

    if (pszModelName)
        GetModeler()->GetNameSpace(pszModelName);

    if (pszTextureName)
    {
        char    bfr[c_cbFileName + 4];
        strcpy(bfr, pszTextureName);
        strcat(bfr, "bmp");

        GetModeler()->GetNameSpace(bfr);
    }

// BUILD_DX9
	GetModeler()->SetColorKeyHint( bOldColorKeyValue );
	GetEngine()->SetEnableMipMapGeneration( false );
// BUILD_DX9
}

TRef<ThingSite> WinTrekClient::CreateThingSite(ImodelIGC* pModel)
{
    return new ThingSiteImpl(pModel);
}

TRef<ClusterSite>   WinTrekClient::CreateClusterSite(IclusterIGC* pCluster)
{
    return 
        new ClusterSiteImpl(
            GetWindow()->GetModeler(), 
            GetWindow()->GetTime(), 
            GetWindow()->GetPosterViewport(),
            pCluster
        );
}


void WinTrekClient::PlaySoundEffect(SoundID soundID, ImodelIGC* model)
{
    if (!model || (model->GetCluster() && model->GetCluster() == GetCluster()))
    {
        TRef<ISoundPositionSource> psource = model ? 
            ((ThingSiteImpl*)model->GetThingSite())->GetSoundSource() : NULL;
        StartSound(soundID, psource);
    }
}

void WinTrekClient::PlaySoundEffect(SoundID soundID, ImodelIGC* model, const Vector& vectOffset)
{
    if (!model || (model->GetCluster() && model->GetCluster() == GetCluster()))
    {
        TRef<ISoundPositionSource> psource = model ? 
            ((ThingSiteImpl*)model->GetThingSite())->GetSoundSource(vectOffset) : NULL;
        StartSound(soundID, psource);
    }
}

void WinTrekClient::PlayNotificationSound(SoundID soundID, ImodelIGC* model)
{
    if (model == GetShip() || model == GetShip()->GetSourceShip())
    {
        TRef<ISoundPositionSource> psource = model ? 
            ((ThingSiteImpl*)model->GetThingSite())->GetSoundSource() : NULL;
        StartSound(soundID, psource);
    }
}

void WinTrekClient::PlayFFEffect(ForceEffectID effectID, ImodelIGC* model, LONG lDirection)
{
    if ((model == NULL) || (model == GetShip())) {
        GetWindow()->PlayFFEffect(effectID, lDirection);
    }
}

void WinTrekClient::PlayVisualEffect(VisualEffectID effectID, ImodelIGC* model, float fIntensity)
{
    if ((model == NULL) || (model == GetShip()))
    {
        switch (effectID)
        {
        case effectJiggle:
            GetWindow()->SetJiggle(fIntensity);
            break;
        }
    }
}

void WinTrekClient::UpdateAmbientSounds(DWORD dwElapsedTime)
{
    // update the sector warnings
    for (ClusterLinkIGC* cLink = GetCore()->GetClusters()->first(); cLink != NULL; cLink = cLink->next())
    {
        IclusterIGC* pCluster = cLink->data();

        ((ClusterSiteImpl*)pCluster->GetClusterSite())->UpdateClusterWarnings();
    }

    // if we are currently in a cluster
    if (GetCluster())
    {
        // kill any station sounds
        if (m_psoundAmbient)
        {
            m_psoundAmbient->Stop();
            m_psoundAmbient = NULL;
        }

        bool bPlayMissileWarning = false;
        float fBestLock = 0.0f;

        IclusterIGC* pcluster = GetCluster();
        
        // let every object in the sector update its sounds
        for (ModelLinkIGC*   pml = pcluster->GetModels()->first(); (pml != NULL); pml = pml->next())
        {
            ImodelIGC*   pmodel = pml->data();
            ThingSite*  pts = pmodel->GetThingSite();
            if (pts)
            {
                ((ThingSiteImpl*)pts)->UpdateSounds(dwElapsedTime);

                if (pmodel->GetObjectType() == OT_missile)
                {
                    ImissileIGC* pmissile = (ImissileIGC*)(pmodel);

                    if (pmissile->GetTarget() == trekClient.GetShip()->GetSourceShip())
                    {
                        bPlayMissileWarning = true;

                        if (pmissile->GetLock() > fBestLock)
                            fBestLock = pmissile->GetLock();
                    }
                }
            }
        }

        if (bPlayMissileWarning && trekClient.GetShip()->GetCluster())
        {
            if (!m_psoundMissileWarning)
            {
                ThingSite*  pts = trekClient.GetShip()->GetThingSite();
                if (pts)
                {                    
                    m_psoundMissileWarning = StartSound(missileLockSound, ((ThingSiteImpl*)pts)->GetSoundSource());
                }
            }

            m_psoundMissileWarning->GetISoundTweakable()->SetPitch(0.75 + fBestLock/2);
        }
        else if (m_psoundMissileWarning)
        {
            m_psoundMissileWarning->Stop();
            m_psoundMissileWarning = NULL;
        }
    }
    else if (trekClient.GetShip()->GetStation() 
        && GetWindow()->screen() == ScreenIDCombat)
    {
        SoundID newAmbientSound = trekClient.GetShip()->GetStation()->GetInteriorSound();
        
        if (!m_psoundAmbient || m_idAmbient != newAmbientSound)
        {
            m_idAmbient = newAmbientSound;
        
            if (m_psoundAmbient)
                m_psoundAmbient->Stop();

            m_psoundAmbient = StartSound(m_idAmbient, NULL);
        }

        // kill the missile warning
        if (m_psoundMissileWarning)
        {
            m_psoundMissileWarning->Stop();
            m_psoundMissileWarning = NULL;
        }
    }
    else
    {
        // kill any station sounds
        if (m_psoundAmbient)
        {
            m_psoundAmbient->Stop();
            m_psoundAmbient = NULL;
        }

        // kill the missile warning
        if (m_psoundMissileWarning)
        {
            m_psoundMissileWarning->Stop();
            m_psoundMissileWarning = NULL;
        }
    }
}

void WinTrekClient::ResetSound()
{
    // kill any station sounds
    if (m_psoundAmbient)
    {
        m_psoundAmbient->Stop();
        m_psoundAmbient = NULL;
    }

    // if we are currently in a cluster
    if (GetCluster())
    {
        IclusterIGC* pcluster = GetCluster();
        
        // stop the sounds from every object in the sector
        for (ModelLinkIGC*   pml = pcluster->GetModels()->first(); (pml != NULL); pml = pml->next())
        {
            ImodelIGC*   pmodel = pml->data();
            ThingSite*  pts = pmodel->GetThingSite();
            if (pts)
            {
                ((ThingSiteImpl*)pts)->StopSounds();
            }
        }
    }

    // the sounds will be restarted on the next call to UpdateAmbientSounds
}


HRESULT WinTrekClient::ConnectToServer(BaseClient::ConnectInfo & ci, DWORD dwCookie, Time now, const char* szPassword, bool bStandalonePrivate)
{
    // The actual connect happens in BaseClient
    HRESULT hr = BaseClient::ConnectToServer(ci, dwCookie, now, szPassword, bStandalonePrivate);

    // close the "connecting" popup
    if (!GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);
    GetWindow()->RestoreCursor();

    if (!m_fm.IsConnected()) 
    {
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Failed to connect to the server.");
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        g_bQuickstart = false;
    }
    else
    {
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Logging in...", NULL, false);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        m_bDisconnected = false;
    }
    return hr;
}


HRESULT WinTrekClient::ConnectToLobby(BaseClient::ConnectInfo * pci)
{
    HRESULT hr = E_FAIL;

    if (m_fmLobby.IsConnected())
        return S_OK; // maybe specify return code indicating already connected?
    
    // if we get a NULL pci, that means we're gonna let BaseClient take care of it (relogin using cached credentials

    // The actual connect happens in BaseClient
    hr = BaseClient::ConnectToLobby(pci);

    // close the "connecting" popup
    if (!GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);
    GetWindow()->RestoreCursor();

    if (!m_fmLobby.IsConnected()) 
    {
		bool bPos = true;
		if (g_bQuickstart) {
			g_bQuickstart = false;	
			bPos = false;
			GetWindow()->screen(ScreenIDIntroScreen);  //imago 7/4/09 this will make users able
														//  to retry and see the MOTD for outage info
		}
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Failed to connect to the lobby.");
        Point point(c_PopupX, c_PopupY);
        Rect rect(point, point);
		(bPos) ? GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, rect, false) : 
			GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

    }
    else
    {
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Logging into lobby...", NULL, false);
        Point point(c_PopupX, c_PopupY);
        Rect rect(point, point);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, rect, false);
        // waiting for logonack from lobby
    }
    
    return hr;
}


HRESULT WinTrekClient::ConnectToClub(BaseClient::ConnectInfo * pci)
{
    HRESULT hr = E_FAIL;

    if (m_fmClub.IsConnected())
        return S_OK; // maybe specify return code indicating already connected?
    
    // if we get a NULL pci, that means we're gonna let BaseClient take care of it (relogin using cached credentials
    if (pci)
        pci->strServer = GetCfgInfo().strClub;

    // The actual connect happens in BaseClient
    hr = BaseClient::ConnectToClub(pci);

    // close the "connecting" popup
    if (!GetWindow()->GetPopupContainer()->IsEmpty())
        GetWindow()->GetPopupContainer()->ClosePopup(NULL);
    GetWindow()->RestoreCursor();

    if (!m_fmClub.IsConnected()) 
    {
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Failed to connect to the Allegiance Zone.");
        Point point(c_PopupX, c_PopupY);
        Rect rect(point, point);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, rect, false);
        g_bQuickstart = false;
    }
    else
    {
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Logging into the Allegiance Zone...", NULL, false);
        Point point(c_PopupX, c_PopupY);
        Rect rect(point, point);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, rect, false);
        // waiting for logonack from lobby
    }
    
    return hr;
}


void WinTrekClient::OnQuitMission(QuitSideReason reason, const char* szMessageParam)
{
    m_sideidLastWinner = NA;
    m_bWonLastGame = false;
    m_bLostLastGame = false;
    m_nNumEndgamePlayers = 0;
    m_nNumEndgameSides = 0;

    if (m_fm.IsConnected ())
    {
        if (Training::GetTrainingMissionID () == Training::c_TM_7_Live)
        {
            Training::EndMission ();
            GetWindow()->screen(ScreenIDTrainScreen);
        }
        else
            GetWindow()->screen(trekClient.GetIsLobbied() ? ScreenIDGameScreen : ScreenIDIntroScreen);

        ZString strMessage;

        switch (reason)
        {
        case QSR_LeaderBooted:
            strMessage = "You have been booted by your commander! You can rejoin the game by logging in with a different ASGS callsign."; // TurkeyXIII - #239 
            break;

        case QSR_OwnerBooted:
            strMessage = "You have been lobby booted by the mission owner!  You can rejoin the game by logging in with a different ASGS callsign."; // pkk - #239 - conform with wiki information
            break;

        case QSR_AdminBooted:
            strMessage = "You have been booted by a server administrator!";
            break;

        case QSR_ServerShutdown:
            strMessage = "The game has been shut down by an administrator.";
            break;

        case QSR_SquadChange:
        case QSR_SideDestroyed:
        case QSR_TeamSizeLimits:
            assert(false); // shouldn't get booted off the mission for these
            break;

        case QSR_Quit:
            break;

        case QSR_LinkDead:
            // message box created by OnSessionLost
            break;

        case QSR_DuplicateRemoteLogon:
            strMessage = "Someone used your zone account to log into another game.";
            break;

        case QSR_DuplicateLocalLogon:
            strMessage = "Someone used your zone account to log into the game you were playing.";
            break;

        case QSR_DuplicateCDKey:
            assert(szMessageParam);
            strMessage = ZString(szMessageParam ? szMessageParam : "someone") 
                + " used your CD Key to log into a game!";
            break;

			// BT - STEAM
		case QSR_BannedBySteam:
			assert(szMessageParam);
			strMessage = ZString(szMessageParam);
			break;

        case QSR_SwitchingSides:
        case QSR_RandomizeSides:
            assert(false); // shouldn't get booted off the mission for this
            break;
        }

        if (!strMessage.IsEmpty())
        {
            if (trekClient.GetIsLobbied())
            {
                m_strDisconnectReason = strMessage;
            }
            else
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox(strMessage);

                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
        }
    }

    BaseClient::OnQuitMission(reason, szMessageParam);
}

void WinTrekClient::SetGameoverInfo(FMD_S_GAME_OVER* pfmGameOver)
{
    m_sideidLastWinner = pfmGameOver->iSideWinner;
	m_bWonLastGame = (pfmGameOver->iSideWinner == GetSideID() || GetSide()->AlliedSides(GetCore()->GetSide(pfmGameOver->iSideWinner),GetSide())); //#ALLY (Imago) 7/8/09
    m_bLostLastGame = (!m_bWonLastGame && (GetSideID() != SIDE_TEAMLOBBY) && pfmGameOver->iSideWinner != NA) || (pfmGameOver->iSideWinner != NA && GetSide()->GetAllies() != NA && !GetSide()->AlliedSides(GetCore()->GetSide(pfmGameOver->iSideWinner),GetSide()));
    m_strGameOverMessage = FM_VAR_REF(pfmGameOver, szGameoverMessage);
    m_nNumEndgamePlayers = 0;
    m_nNumEndgameSides = pfmGameOver->nNumSides;

    if (m_vplayerEndgameInfo)
        delete []m_vplayerEndgameInfo;
    m_vplayerEndgameInfo = NULL;
    memcpy(m_vsideEndgameInfo, pfmGameOver->rgSides, sizeof(SideEndgameInfo) * c_cSidesMax);
    m_bEndgameEjectPods = pfmGameOver->bEjectPods;
    m_bGameCounted = true; // assume the game counted until we are told otherwise
    m_bScoresCounted = MyMission()->GetMissionParams().bScoresCount; 

    trekClient.GetClientEventSource()->OnGameoverStats();
    trekClient.GetClientEventSource()->OnGameoverPlayers();
}

void WinTrekClient::AddGameoverPlayers(PlayerEndgameInfo* vEndgamePlayerInfo, int nCount)
{
    PlayerEndgameInfo* vnewInfo = new PlayerEndgameInfo[m_nNumEndgamePlayers + nCount];
    
    if (m_nNumEndgamePlayers > 0)
        memcpy(vnewInfo, m_vplayerEndgameInfo, sizeof(PlayerEndgameInfo) * m_nNumEndgamePlayers);
    memcpy(vnewInfo + m_nNumEndgamePlayers, vEndgamePlayerInfo, sizeof(PlayerEndgameInfo) * nCount);

    if (m_vplayerEndgameInfo)
        delete []m_vplayerEndgameInfo;

    m_nNumEndgamePlayers += nCount;
    m_vplayerEndgameInfo = vnewInfo;

    // see if we're in the endgame stats...
    for (int iPlayerIndex = m_nNumEndgamePlayers - nCount; iPlayerIndex < nCount; iPlayerIndex++)
    {
        // if these are our stats...
        if (strcmp(m_vplayerEndgameInfo[iPlayerIndex].characterName, GetShip()->GetName()) == 0)
        {
            // check to see if the game counted.
            m_bGameCounted = m_vplayerEndgameInfo[iPlayerIndex].scoring.GetGameCounted();
            break;
        }
    }

    trekClient.GetClientEventSource()->OnGameoverPlayers();
}

ZString WinTrekClient::GetGameoverMessage()
{
    return m_strGameOverMessage;
};

PlayerEndgameInfo* WinTrekClient::GetEndgamePlayerInfo(int nIndex)
{
    assert(nIndex < m_nNumEndgamePlayers);
    return &(m_vplayerEndgameInfo[nIndex]);
};

int WinTrekClient::GetNumEndgamePlayers()
{
    return m_nNumEndgamePlayers;
};

SideEndgameInfo* WinTrekClient::GetSideEndgameInfo(SideID sideId)
{
    assert(sideId >= 0 && sideId < m_nNumEndgameSides);

    return &(m_vsideEndgameInfo[sideId]);
}

int WinTrekClient::GetNumEndgameSides()
{
    return m_nNumEndgameSides;
}

Color WinTrekClient::GetEndgameSideColor(SideID sideId)
{
    if (sideId < 0)
        return 0.75 * Color::White();

    assert(sideId >= 0 && sideId < m_nNumEndgameSides);

    return m_vsideEndgameInfo[sideId].color;
};

CivID WinTrekClient::GetEndgameSideCiv(SideID sideId)
{
    if (sideId == SIDE_TEAMLOBBY)
        return NA;

    assert(sideId >= 0 && sideId < m_nNumEndgameSides);

    return m_vsideEndgameInfo[sideId].civID;
};

int WinTrekClient::GetGrooveLevel()
{
    int nCurrentGrooveLevel = 0;
    bool bEnemiesSighted = false;
    bool bEnemiesInRange = false;
    bool bEnemiesInRangeShootingAtMe = false;
    float fMaximumRange = 0;
    bool bFiring = false;

    IhullTypeIGC*   pht = GetShip()->GetSourceShip()->GetBaseHullType();
    if (pht)
    {
        // figure out whether we are firing and what our maximum range is
        Mount maxFixedWeapons = pht->GetMaxWeapons();
        for (Mount mount = 0; mount < maxFixedWeapons; mount++) 
        {
            const IweaponIGC* pweapon;
            CastTo(pweapon, GetShip()->GetMountedPart(ET_Weapon, mount));

            if (pweapon) 
            {
                if (pweapon->fActive())
                    bFiring = true;

                IprojectileTypeIGC* ppt = pweapon->GetProjectileType();
                fMaximumRange = std::max(fMaximumRange, ppt->GetSpeed() * pweapon->GetLifespan());
            }
        }

        // check for missiles (for firing and maximum range)
        const ImagazineIGC* pmagazine;
        CastTo(pmagazine, GetShip()->GetSourceShip()->GetMountedPart(ET_Magazine, 0));

        if (pmagazine) 
        {
            if (pmagazine->fActive() && (GetShip()->GetSourceShip()->GetStateM() & missileFireIGC))
                bFiring = true;

            ImissileTypeIGC* pmt = pmagazine->GetMissileType();
            fMaximumRange = std::max(fMaximumRange, 
                pmt->GetLifespan()*(pmt->GetInitialSpeed()+0.5f*pmt->GetLifespan()*pmt->GetAcceleration()));
        }
    }

    
    // look for all visible enemies in the player's sector
    if (GetCluster())
    {
        for (ShipLinkIGC*    psl = GetCluster()->GetShips()->first();
             (psl != NULL);
             psl = psl->next())
        {
            IshipIGC* pship = psl->data();

            if ( ( (pship->GetSide() != GetSide()) && !pship->GetSide()->AlliedSides(pship->GetSide(),GetSide()) ) //#ALLY - imago 7/3/09 7/8/09
                && pship->SeenBySide(GetSide()) )
            {
                bEnemiesSighted = true;
                
                if (GetShip()->GetSourceShip()->GetCluster())
                {
                    float c_fFudgeFactor = 2.0f; // extend the ranges a little

                    bEnemiesInRange = 
                        (GetShip()->GetSourceShip()->GetPosition()
                            - pship->GetSourceShip()->GetPosition()).LengthSquared()
                            < fMaximumRange * fMaximumRange * c_fFudgeFactor;

                    bEnemiesInRangeShootingAtMe = bEnemiesInRange
                        && (pship->GetCommandTarget(c_cmdCurrent) == GetShip()->GetSourceShip())
                        && (pship->GetStateM() & (missileFireIGC | chaffFireIGC | weaponsMaskIGC));
                }
            }               
        }
    }


    //
    // Groove level 1 polling triggers:
    //

    // if we see enemies or enemies see us, be afraid
    if (bEnemiesSighted || MyPlayerInfo()->GetShipStatus().GetDetected())
    {
        m_nGrooveLevel = std::max(m_nGrooveLevel, 1);
        m_vtimeGrooveDrops[1] = Time::Now() + c_fGrooveLevelDuration;
    }

    // look for a cluster with a threatened builder, or station
    for (ClusterLinkIGC* cLink = GetCore()->GetClusters()->first(); cLink != NULL; cLink = cLink->next())
    {
        IclusterIGC* pCluster = cLink->data();

        ClusterWarning warn = 
            GetClusterWarning(pCluster->GetClusterSite()->GetClusterAssetMask(), 
                trekClient.MyMission()->GetMissionParams().bInvulnerableStations);

        switch (warn)
        {
        case c_cwMinerThreatened:
        case c_cwBuilderThreatened:
        case c_cwStationThreatened:
            m_nGrooveLevel = std::max(m_nGrooveLevel, 1);
            m_vtimeGrooveDrops[1] = Time::Now() + c_fGrooveLevelDuration;
            break;
        }
    }


    //
    // Groove level 2 polling triggers:
    //

    // if the client tries to fire while an enemy is within range or if an 
    // enemy is firing on the client, raise the grove level.
    if (bEnemiesInRange && bFiring 
        || bEnemiesInRangeShootingAtMe)
    {
        m_nGrooveLevel = std::max(m_nGrooveLevel, 2);
        m_vtimeGrooveDrops[2] = Time::Now() + c_fGrooveLevelDuration;
    }

    // drop the current groove level until we find one that matches
    while (Time::Now() > m_vtimeGrooveDrops[m_nGrooveLevel] 
        && m_nGrooveLevel > 0)
    {
        --m_nGrooveLevel;
    }

    return m_nGrooveLevel;
}

void WinTrekClient::StartLockDown(const ZString& strReason, LockdownCriteria criteria)
{
    GetWindow()->StartLockDown(strReason);
    BaseClient::StartLockDown(strReason, criteria);
}

void WinTrekClient::EndLockDown(LockdownCriteria criteria)
{
    BaseClient::EndLockDown(criteria);
    GetWindow()->EndLockDown();
}

WinTrekClient trekClient;
