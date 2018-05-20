/*-------------------------------------------------------------------------
 * FSShip.cpp
 * 
 * Implementation of CFSShip and its derivatives
 * 
 * Owner: 
 * 
 * Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#include "pch.h"


// Statics
CFSShip * CFSShip::m_rgpfsShip[c_cShipsMax] = {0};
short     CFSShip::m_shipidNext = 0;
int       CFSShip::m_cShips = 0;
int       CFSPlayer::m_latencyMax = 0;
int       CFSPlayer::m_latencyTotal = 0;
int       CFSPlayer::m_cUpdates = 0;
char      CFSDrone::m_rgfsDrone[c_cDronesMax * sizeof(CFSDrone)];
int       CFSDrone::m_ifsdNext = 0;
int       CFSDrone::m_cDrones = 0;

/******************************************************************
                        CFSShip
******************************************************************/
CFSShip::CFSShip(TRef<IshipIGC> pShip, bool fIsPlayer)
:
    m_plrscore(fIsPlayer)
{
  // FYI: overloaded new allocator in inherited classes is required to zero-init everything, even in retail.
  assert(pShip);
  assert(m_cShips < c_cShipsMax);
  ImissionIGC*  pmission = pShip->GetMission();
  m_pfsMission = (CFSMission *)(pmission->GetPrivateData());
  while (m_rgpfsShip[m_shipidNext]) // find a shipid for them
  {
    m_shipidNext++;
    if (m_shipidNext == c_cShipsMax - 1)
      m_shipidNext = 0;
  }
  pShip->SetObjectID(m_shipidNext);
  m_pShip         = pShip;
  m_fIsPlayer     = fIsPlayer;
  m_money = 0;
  m_rgpfsShip[m_shipidNext] = this;
  m_pShip->SetPrivateData((DWORD) this);
  m_bHasUpdate = false;

  m_warpState = warpReady;
  debugf("New ship (%s): Name=%s, ShipID=%d, PilotType=%d\n", fIsPlayer ? "player" : "drone",
      pShip->GetName(), pShip->GetObjectID(), pShip->GetPilotType());

  
} 

CFSShip::~CFSShip()
{
  //NYI hack ... ppso will need to be saved for players so they can rejoin and/or get their scores recorded when the game ends

  debugf("Destroying %s, id=%d\n", GetName(), GetShipID());

  AnnounceExit(NULL, SDR_LOGGEDOFF);
  m_cShips--;
  m_rgpfsShip[GetShipID()] = NULL;
  m_pShip->Terminate();
}


void CFSShip::HitWarp(IwarpIGC * pwarp)
{
    //Ignore jumps that happen too closely together
    if (m_warpState == warpReady)
    {
		// Andon - Added check for aleph mass limits
		if (m_pShip->GetMass() <= pwarp->MassLimit() || !IsPlayer() && pwarp->MassLimit() > 0 || pwarp->MassLimit() < 0)
		{
			if (IsPlayer())
			{
				m_warpState = warpNoUpdate;
			}

			IwarpIGC *    pwarpDest    = pwarp->GetDestination();
			assert (pwarpDest);
			IclusterIGC * pclusterDest = pwarpDest->GetCluster();

			ShipStatusWarped(pwarp);

			Orientation alephOrientation = pwarpDest->GetOrientation();
			const Vector&   v = m_pShip->GetVelocity();
			float           speed2 = v.LengthSquared();
			float           speed  = float(sqrt(speed2));
			if (speed2 > 0)
			{
			  float           error;
			  {
				  //How close is the ship coming to the center of the warp?
				  Vector          dp = pwarp->GetPosition() - m_pShip->GetPosition();

				  float   t = (dp * v) / speed2;
				  float   d = (dp - t * v).LengthSquared();
				  float   r = pwarp->GetRadius();

				  error = (d / (r*r)) + 0.125f;      //Error ranges from 0.125 to 1.125
				  // yp: to prevent 'spin of death' in massive ships.
				  // This works and is explained in that the more massive the ship the less effect going through the aleph should have
				  // on its rotational velocity. The massive amount of inertia should decrease changes in rotational velocity.
				  if(m_pShip->GetMass() > 300.0f)
				  {
					error = error * (300.0f / m_pShip->GetMass()); // the greater the mass is above 750 the less error will be applied.
				  }
				  // yp end
			  }

			  alephOrientation.Pitch(random(-error, error));
			  alephOrientation.Yaw(random(-error, error));

			  m_pShip->SetCurrentTurnRate(c_axisRoll,
										  m_pShip->GetCurrentTurnRate(c_axisRoll) +
										  random(pi * 0.5f * error, pi * 1.5f * error));  //Must be less than 2.0 * pi
			}
			m_pShip->SetOrientation(alephOrientation);
			const Vector&   backward = alephOrientation.GetBackward();

			speed = -(speed + pwarp->GetMission()->GetFloatConstant(c_fcidExitWarpSpeed));
			m_pShip->SetVelocity(backward * speed);

			m_pShip->SetPosition(pwarpDest->GetPosition() +
								 (alephOrientation.GetUp() * random(2.0f, 5.0f)) +
								 (alephOrientation.GetRight() * random(2.0f, 5.0f)) -
								 (m_pShip->GetRadius() + 5.0f) * backward);

			GetIGCShip()->SetCluster(pclusterDest);
		}
    }
}


/*
void CFSShip::Dock(IstationIGC * pstation)
{
  assert(pstation); // can't use this to undock
}
*/

void CFSShip::SetSide(CFSMission * pfsMission, IsideIGC * pside)
{
  // they can't hop between mission without first going to lobby
  assert (IMPLIES(pfsMission, !m_pfsMission) || pfsMission == m_pfsMission); 
  assert (IMPLIES(pside, pfsMission));
  m_pfsMission = pfsMission;
  GetIGCShip()->SetMission(m_pfsMission ? m_pfsMission->GetIGCMission() : g.trekCore);
  GetIGCShip()->SetSide(pside);
  if (pside && (pside->GetObjectID() >= 0))
  {
      GetIGCShip()->SetBaseHullType(pside->GetCivilization()->GetLifepod());
  }
}


void CFSShip::Reset(bool bFull)
{    
    GetIGCShip()->Reset(bFull);
    ShipStatusExit();
}


/*-------------------------------------------------------------------------
 * Launch
 *-------------------------------------------------------------------------
 * Purpose:
 *    Send ship into space from station
 */
void CFSShip::Launch(IstationIGC* pstation)
{
  assert (pstation);
  assert (m_pShip->GetBaseHullType());

  pstation->Launch(m_pShip);

  // let our side know we've left
  ShipStatusLaunched();
}


/*-------------------------------------------------------------------------
 * AnnounceExit
 *-------------------------------------------------------------------------
 * Purpose:
 *    Somebody left. Let everybody know.
 * 
 * Returns:
 *    nothing
 */
void CFSShip::AnnounceExit(IclusterIGC* pclusterOld, ShipDeleteReason sdr)
{
  if ((SDR_DOCKED == sdr || SDR_LEFTSECTOR == sdr) && (m_pShip->GetParentShip() != NULL))
    return;

  BEGIN_PFM_CREATE(g.fm, pfmShipDelete, S, SHIP_DELETE)
  END_PFM_CREATE
  pfmShipDelete->shipID = GetShipID();
  pfmShipDelete->sdr = sdr;
  CFMRecipient * prcp = NULL;
  if (SDR_DOCKED == sdr || SDR_LEFTSECTOR == sdr)
  {
    assert (pclusterOld);
    prcp = GetGroupSectorFlying(pclusterOld);
  }
  else
  {
    ImissionIGC*    pm = GetIGCShip()->GetMission();
    
    if (pm && pm->GetPrivateData())
    {
      CFSMission * pfsMission = (CFSMission *)(pm->GetPrivateData());
      prcp = pfsMission->GetGroupMission();
    }
  }

  if (prcp)
    g.fm.SendMessages(prcp, FM_GUARANTEED, FM_FLUSH);
  else
    g.fm.PurgeOutBox();
}

void CFSPlayer::ForceLoadoutChange(void)
{
    assert(0 == g.fm.CbUsedSpaceInOutbox());

    QueueLoadoutChange(true);
    g.fm.SendMessages(GetPlayer()->GetConnection(), FM_GUARANTEED, FM_FLUSH);
}


void CFSShip::ShipStatusSpotted(IsideIGC* pside)
{
    SideID  sideID = pside->GetObjectID();

    SectorID    sectorID;
    {
        IclusterIGC*    pcluster = GetIGCShip()->GetCluster();
        if (pcluster == NULL)
        {
            IstationIGC*    pstation = GetIGCShip()->GetStation();
            assert (pstation);
            pcluster = pstation->GetCluster();
        }

        sectorID = pcluster->GetObjectID();
    }

    ShipStatus* pss = &m_rgShipStatus[sideID];
	pss->SetStateTime(g.timeNow.clock());
    pss->SetState(c_ssFlying);
    pss->SetParentID(NA);
    pss->SetHullID(GetIGCShip()->GetHullType()->GetObjectID());
    pss->SetSectorID(sectorID);

    //Flag that we have been detected as well
    IsideIGC*   mySide = GetSide();
    SideID  mySideID = mySide->GetObjectID();
	if (mySide != pside && !mySide->AlliedSides(mySide, pside)) //#ALLY -was: mySide != pside (Imago fixed 7/8/09)
        m_rgShipStatus[mySideID].SetDetected(true);

    //Adjust the ship status for all of the children as well
    {
        for (ShipLinkIGC*   psl = GetIGCShip()->GetChildShips()->first();
             (psl != NULL);
             psl = psl->next())
        {
            IshipIGC*   pship = psl->data();
            CFSShip*    pfsship = ((CFSShip*)(pship->GetPrivateData()));
            ShipStatus* pss = pfsship->GetShipStatus(sideID);

            pss->SetState((pship->GetTurretID() == NA) ? c_ssObserver : c_ssTurret); 
            pss->SetHullID(NA);
            pss->SetSectorID(sectorID);
            pss->SetParentID(GetShipID());

			if (mySide != pside && !mySide->AlliedSides(mySide, pside)) //#ALLY -was != (Imago fixed 7/8/09)
                pfsship->GetShipStatus(mySideID)->SetDetected(true);
        }
    }
}
void          CFSShip::ShipStatusHidden(IsideIGC* pside)
{
    SideID  sideID = pside->GetObjectID();
    m_rgShipStatus[sideID].SetUnknown();

    IsideIGC*   mySide = GetSide();
    SideID      mySideID = mySide->GetObjectID();

    //Does anyone see us?
    bool    detectedF = false;
    {
        CFSMission* pmission = this->GetMission();
        if (pmission)  
        {
            for (SideLinkIGC*   psl = pmission->GetIGCMission()->GetSides()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
				if (psl->data() != mySide && !mySide->AlliedSides(psl->data(), mySide)) //#ALLY -was: != (Imago fixed 7/8/09)
                {
                    ShipStatus* pss = GetShipStatus(psl->data()->GetObjectID());
                    if (!pss->GetUnknown())
                    {
                        detectedF = true;
                        break;
                    }
                }
            }
        }
    }

    m_rgShipStatus[mySideID].SetDetected(detectedF);

    //Adjust the ship status for all of the children as well
    {
        for (ShipLinkIGC*   psl = GetIGCShip()->GetChildShips()->first();
             (psl != NULL);
             psl = psl->next())
        {
            IshipIGC*   pship = psl->data();
            CFSShip*    pfsShip = ((CFSShip*)(pship->GetPrivateData()));
            ShipStatus* pss = pfsShip->GetShipStatus(sideID);

            pss->SetUnknown();

            pfsShip->GetShipStatus(mySideID)->SetDetected(detectedF);
        }
    }
}
void          CFSShip::ShipStatusHullChange(IhullTypeIGC*    pht)
{
    IsideIGC*   psideMe = GetIGCShip()->GetSide();
    HullID  hid = pht->GetObjectID();

    for (SideLinkIGC*   psl = GetMission()->GetIGCMission()->GetSides()->first();
         (psl != NULL);
         psl = psl->next())
    {
        IsideIGC*   pside = psl->data();
        if ((psideMe == pside || psideMe->AlliedSides(psideMe,pside)) || GetIGCShip()->SeenBySide(pside)) // #ALLY Imago 7/23/09 VISIBILITY?
        {
            m_rgShipStatus[pside->GetObjectID()].SetHullID(hid);
        }
    } 
}

void          CFSShip::ShipStatusDocked(IstationIGC*   pstation)
{
    StationID   stationID = pstation->GetObjectID();
    SectorID    sectorID = pstation->GetCluster()->GetObjectID();
    HullID      hullID;
    {
        IhullTypeIGC*   pht = GetIGCShip()->GetBaseHullType();
        hullID = pht ? pht->GetObjectID() : NA;
    }
    IsideIGC*   psideMe = GetIGCShip()->GetSide();

    for (SideLinkIGC*   psl = GetMission()->GetIGCMission()->GetSides()->first();
         (psl != NULL);
         psl = psl->next())
    {
        IsideIGC*   pside = psl->data();
        if ((pside == psideMe || pside->AlliedSides(pside,psideMe)) || // #ALLY Imago 7/8/09 VISIBILITY?
            (GetIGCShip()->SeenBySide(pside) && pstation->SeenBySide(pside)))
        {
            SideID      sideID = pside->GetObjectID();
            ShipStatus* pss = &(m_rgShipStatus[sideID]);
			pss->SetStateTime(g.timeNow.clock());
            pss->SetState(c_ssDocked);
            pss->SetParentID(NA);
            pss->SetStationID(stationID);
            pss->SetSectorID(sectorID);
            pss->SetHullID(hullID);

            //Adjust the ship status for all of the children as well
            {
                for (ShipLinkIGC*   psl = GetIGCShip()->GetChildShips()->first();
                     (psl != NULL);
                     psl = psl->next())
                {
                    IshipIGC*   pship = psl->data();
                    ShipStatus* pss = ((CFSShip*)(pship->GetPrivateData()))->GetShipStatus(sideID);

                    assert ((pship->GetTurretID() == NA)
                            ? (pss->GetState() == c_ssObserver)
                            : (pss->GetState() == c_ssTurret));
                    assert (pss->GetParentID() == GetIGCShip()->GetObjectID());
                    assert (pss->GetHullID() == NA);

                    pss->SetStationID(stationID);
                    pss->SetSectorID(sectorID);

                }
            }
        }
    } 
}

void          CFSShip::ShipStatusRecalculate(void)
{
    IsideIGC*       psideMe = GetIGCShip()->GetSide();
    IclusterIGC*    pcluster = GetIGCShip()->GetCluster();
    if (pcluster)
    {
        for (SideLinkIGC*   psl = GetMission()->GetIGCMission()->GetSides()->first();
             (psl != NULL);
             psl = psl->next())
        {
            IsideIGC*   pside = psl->data();
            SideID      sid = pside->GetObjectID();
            if ((pside == psideMe || pside->AlliedSides(pside,psideMe)) || (m_rgShipStatus[sid].GetState() >= c_ssFlying)) //ALLY IMAGO 7/23/09
                ShipStatusSpotted(pside);
        }
    }
    else
    {
        ShipStatusSpotted(psideMe);
    }
}

void          CFSShip::ShipStatusLaunched(void)
{
    SideID   sideID = GetIGCShip()->GetSide()->GetObjectID();
	m_rgShipStatus[sideID].SetStateTime(g.timeNow.clock());
    m_rgShipStatus[sideID].SetState(c_ssFlying);
    m_rgShipStatus[sideID].SetParentID(NA);
    
    assert (m_rgShipStatus[sideID].GetSectorID() == GetIGCShip()->GetCluster()->GetObjectID());
}

void          CFSShip::ShipStatusStart(IstationIGC*   pstation)
{
    SideID   sideID = GetIGCShip()->GetSide()->GetObjectID();
	m_rgShipStatus[sideID].SetStateTime(g.timeNow.clock());
    m_rgShipStatus[sideID].SetState(pstation ? c_ssDocked : c_ssDead);
    m_rgShipStatus[sideID].SetParentID(NA);
    m_rgShipStatus[sideID].SetStationID(pstation ? pstation->GetObjectID() : NA);
    m_rgShipStatus[sideID].SetSectorID(pstation ? pstation->GetCluster()->GetObjectID() : NA);
    m_rgShipStatus[sideID].SetHullID(GetIGCShip()->GetHullType()->GetObjectID());
}

void          CFSShip::ShipStatusExit(void)
{
    for (SideID i = 0; (i < c_cSidesMax); i++)
    {
		m_rgShipStatus[i].SetStateTime(g.timeNow.clock());
        m_rgShipStatus[i].SetState(c_ssDead);
        m_rgShipStatus[i].SetParentID(NA);
        m_rgShipStatus[i].SetHullID(NA);
        m_rgShipStatus[i].SetSectorID(NA);
        m_rgShipStatus[i].SetStationID(NA);
        m_rgShipStatus[i].SetUnknown();
    }
}
void          CFSShip::ShipStatusRestart(IstationIGC*   pstation)
{
    IsideIGC*   psideShip = GetIGCShip()->GetSide();

    for (SideLinkIGC*   psl = GetMission()->GetIGCMission()->GetSides()->first();
         (psl != NULL);
         psl = psl->next())
    {
        IsideIGC*   pside = psl->data();
        if ((pside == psideShip || pside->AlliedSides(pside,psideShip)) || (GetIGCShip()->SeenBySide(pside))) //ALLY imago 7/23/09
        {
            ShipStatus* pss = &(m_rgShipStatus[pside->GetObjectID()]);
			pss->SetStateTime(g.timeNow.clock());
            pss->SetState(c_ssDead);
            pss->SetParentID(NA);
            pss->SetHullID(NA);
            pss->SetSectorID(NA);
        }
    } 
}
void          CFSShip::ShipStatusWarped(IwarpIGC*   pwarp)
{
    SectorID   sectorID = pwarp->GetDestination()->GetCluster()->GetObjectID();
    for (SideLinkIGC*   psl = GetMission()->GetIGCMission()->GetSides()->first();
         (psl != NULL);
         psl = psl->next())
    {
        IsideIGC*   pside = psl->data();
        if ((GetIGCShip()->SeenBySide(pside)) && (pwarp->SeenBySide(pside)))
        {
            SideID      sideID = pside->GetObjectID();
            m_rgShipStatus[sideID].SetSectorID(sectorID);

            //Adjust the ship status for all of the children as well
            {
                for (ShipLinkIGC*   psl = GetIGCShip()->GetChildShips()->first();
                     (psl != NULL);
                     psl = psl->next())
                {
                    IshipIGC*   pship = psl->data();
                    ShipStatus* pss = ((CFSShip*)(pship->GetPrivateData()))->GetShipStatus(sideID);

                    pss->SetSectorID(sectorID);
                }
            }
        }
    } 
}

//This should only be called from the ChangeCluster() callback.
void CFSShip::SetCluster(IclusterIGC * pcluster, bool   bViewOnly)
{
    if (pcluster)
    {
        IshipIGC* pshipParent = m_pShip->GetParentShip();

        if ((pshipParent == NULL) && !bViewOnly)
        {
            //Everyone already in the sector now knows about the player's ship
            //(assuming the player is not in a turret)
            QueueLoadoutChange();

            BEGIN_PFM_CREATE(g.fm, pfmSSU, S, SINGLE_SHIP_UPDATE)
            END_PFM_CREATE

            m_pShip->ExportShipUpdate(&(pfmSSU->shipupdate));

            {
                ImodelIGC*  pmodelTarget = m_pShip->GetCommandTarget(c_cmdCurrent);
                if (pmodelTarget)
                {
                    pfmSSU->otTarget = pmodelTarget->GetObjectType();
                    pfmSSU->oidTarget = pmodelTarget->GetObjectID();
                }
                else
                {
                    pfmSSU->otTarget = NA;
                    pfmSSU->oidTarget = NA;
                }                    
            }

            pfmSSU->bIsRipcording = m_pShip->fRipcordActive();

            g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH);

            SetDeviation(0.0f);
        }

        // if this is a drone that has arrived in a sector to which it was told
        // to go, clear its objective.
        if (!IsPlayer())
        {
            for (Command i = 0; i < c_cmdMax; i++)
            {
                ImodelIGC* ptarget = m_pShip->GetCommandTarget(i);
                
                // if this command has a cluster buoy in this cluster...
                if (ptarget && ptarget->GetObjectType() == OT_buoy 
                    && ((IbuoyIGC*)ptarget)->GetBuoyType() == c_buoyCluster
                    && ((IbuoyIGC*)ptarget)->GetCluster() == pcluster)
                {
                    // clear the command
                    m_pShip->SetCommand(i, NULL, c_cidNone);
                }
            }
        }
    }
}


/*-------------------------------------------------------------------------
 * CaptureStation
 *-------------------------------------------------------------------------
 * Purpose:
 *    Handle a station being captured by a ship
 */
void CFSShip::CaptureStation(IstationIGC * pstation)
{
  {
    //Fudge the hitpoints of the station
    //All those guns inside damage the hull
    pstation->SetFraction(pstation->GetFraction() * 0.5f);

    //But the heroic engineer's get the shields up.
    pstation->SetShieldFraction(0.8f); //pstation->GetShieldFraction() + 0.5f);
  }

  {
      GetPlayerScoreObject()->CaptureBase(true);
	  if (IsPlayer())
	  {
		  CSteamAchievements *pSteamAchievements = GetPlayer()->GetSteamAchievements();
		  pSteamAchievements->AwardBaseKillOrCapture(false); //Award Base Capture Achievement
	  }

      for (ShipLinkIGC* psl = GetIGCShip()->GetChildShips()->first();
           (psl != NULL);
           psl = psl->next())
      {
         CFSShip*      ps = (CFSShip*)(psl->data()->GetPrivateData());
         ps->GetPlayerScoreObject()->CaptureBase(false);
      }
  }

  IsideIGC * pside = GetSide();
  pside->AddBaseCapture();

  SideID iSide = pside->GetObjectID();
  IsideIGC* psideOld = pstation->GetSide();

  StationID stationID = pstation->GetObjectID();
  BEGIN_PFM_CREATE(g.fm, pfmStationCapture, S, STATION_CAPTURE)
  END_PFM_CREATE
  pfmStationCapture->stationID = stationID;
  pfmStationCapture->sidOld = psideOld->GetObjectID();
  pfmStationCapture->sidNew = iSide;
  pfmStationCapture->shipIDCredit = GetIGCShip()->GetObjectID();
  g.fm.SendMessages(GetMission()->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  pstation->SetSide(pside);
  // TE: Fire AGCEvent when base is captured
  CFSMission * pfsMission = this->GetMission();
  LPCSTR pszContext = pfsMission->GetIGCMission() ? pfsMission->GetIGCMission()->GetContextName() : NULL;
  _AGCModule.TriggerContextEvent(NULL, EventID_StationChangesSides, pszContext,
    GetName(), GetShipID(), pside->GetUniqueID(), -1, 4,
	"GameID"	 , VT_I4   , pfsMission->GetMissionID(),
	"OldTeam"    , VT_I4   , psideOld->GetUniqueID(),
    "OldTeamName", VT_LPSTR, psideOld->GetName(),
	"StationName", VT_LPSTR, pstation->GetName());
  // TE end

	// yp: Add event for players who were involved in the capture of an enemy base.
    // mmf commented this out for now pending additional testing
#if 0
	 ZString pszPlayerList = ""; // this creates a new ZString object and set its value to "", it's not a pointer to ""
	 if(m_pfsMission->GetIGCMission() && m_pfsMission->GetIGCMission()->GetShips())
	 {
		ShipLinkIGC * pShiplink = m_pfsMission->GetIGCMission()->GetShips()->first();
		while (pShiplink)
		{
			CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
			if (pfsShip && pfsShip->IsPlayer())
			{
				// this logic might need to be tweeked to include the ship that did the capture if its
				if(pfsShip->GetSide()		&& pfsShip->GetSide()->GetObjectID()	== iSide && // if they are on the side that did the Capture. and..
					pfsShip->GetPlayer()->GetIGCShip()->GetObjectID() == GetIGCShip()->GetObjectID() ||	// they are the ship that did the caputure. or
				   pfsShip->GetCluster()	&& pstation->GetCluster()	&& pfsShip->GetCluster()->GetObjectID() == pstation->GetCluster()->GetObjectID()) // they are in this sector
				{
					pszPlayerList = pszPlayerList + ";" + ZString(pfsShip->GetPlayer()->GetName()); // players name
					// The distance the player is from the station that was destroyed.
					if(pfsShip->GetPlayer()->GetIGCShip() )
					{
						pszPlayerList = pszPlayerList +  ":" + ZString( (pstation->GetPosition() - pfsShip->GetPlayer()->GetIGCShip()->GetPosition()).Length()); // the distance
					}
				}
			}
			pShiplink = pShiplink->next();
		}
	 }
	 
	 // Fire AGCEvent listing players in the sector
	// TODO: Might want to add into the event weither or not this was a VICTORY capture.. should we track that as well?
	_AGCModule.TriggerContextEvent(NULL, EventID_StationChangesSides, pszContext,
						GetName(), GetShipID(), pside->GetUniqueID(), -1, 4,
						"GameID"	 , VT_I4   , pfsMission->GetMissionID(),
						"OldTeam"    , VT_I4   , psideOld->GetUniqueID(),
						"OldTeamName", VT_LPSTR, psideOld->GetName(),
						"zPlayerList", VT_LPSTR, pszPlayerList); // pszPlayerList should look like ";player@squad:1500;player2@squad:500"
 // yp:end
#endif



  //Possibly the built themselves to a victory
  IsideIGC*   psideWin = m_pfsMission->CheckForVictoryByStationCapture(pside, psideOld);
  if (psideWin)
  {
      static char szReason[100];     //Make this static so we only need to keep a pointer to it around
      sprintf(szReason, "%s won because %s captured %s", psideWin->GetName(), GetIGCShip()->GetName(), pstation->GetName());
      m_pfsMission->GameOver(psideWin, szReason);
  }
  else if (psideOld->GetActiveF())						//RESET OUR EYE HERE?  IMAGO CAPTURE EYE BUG FIX?  REVIEW 7/23/09
      m_pfsMission->VacateStation(pstation);

  for (SideLinkIGC* l = pside->GetMission()->GetSides()->first(); (l != NULL); l = l->next()) {
      IsideIGC* pcurSide = l->data();
      if (pcurSide == psideOld || IsideIGC::AlliedSides(psideOld, pcurSide)) {
          pcurSide->HandleNewEnemyCluster(pstation->GetCluster());
          pcurSide->UpdateTerritory();
      }
      else if (pcurSide == pside || IsideIGC::AlliedSides(pside, pcurSide)) {
          pcurSide->UpdateTerritory();
      }
  }
}

void CFSShip::QueueLoadoutChange(bool bForce)
{
  const PartListIGC*  pplist = GetIGCShip()->GetParts();
  const ShipListIGC*  pslist = GetIGCShip()->GetChildShips();

  BEGIN_PFM_CREATE(g.fm, pfmLoadoutChange, S, LOADOUT_CHANGE)
      FM_VAR_PARM(NULL, GetIGCShip()->ExportShipLoadout(NULL))
      FM_VAR_PARM(NULL, pslist->n() * sizeof(PassengerData))
  END_PFM_CREATE

  GetIGCShip()->ExportShipLoadout((ShipLoadout*)(FM_VAR_REF(pfmLoadoutChange, loadout)));

  pfmLoadoutChange->sidShip = bForce ? NA : GetIGCShip()->GetObjectID();

  //Queue any children and their positions
  {
      PassengerData*   pd = (PassengerData*)(FM_VAR_REF(pfmLoadoutChange, rgPassengers));
      for (ShipLinkIGC*   psl = pslist->first();
           (psl != NULL);
           psl = psl->next())
      {
          pd->shipID = psl->data()->GetObjectID();
          pd->turretID = psl->data()->GetTurretID();

          pd++;
      }
  }
}


/******************************************************************
                        CFSPlayer
******************************************************************/
//This should only be called from the ChangeCluster() callback.
void CFSPlayer::SetCluster(IclusterIGC* pcluster, bool bViewOnly)
{
  CFSShip::SetCluster(pcluster, bViewOnly);

  if (pcluster)
  {
    SetDPGroup((CFSCluster*)(pcluster->GetPrivateData()), true);

    IshipIGC*   pshipParent = GetIGCShip()->GetParentShip();
    if ((pshipParent == NULL) || bViewOnly)
    {
        ShipID      shipID = GetIGCShip()->GetObjectID();
        assert(0 == g.fm.CbUsedSpaceInOutbox());
        if (!bViewOnly)
        {
            //Move the player to his destination
            BEGIN_PFM_CREATE(g.fm, pfmSetCluster, S, SET_CLUSTER)
            END_PFM_CREATE
            pfmSetCluster->sectorID = pcluster->GetObjectID();

            //Send the position of the parent ship if we are a child, otherwise our position
            IshipIGC*   pshipSource = pshipParent ? pshipParent : GetIGCShip();
            pshipSource->SetStateBits(keyMaskIGC, 0);   //deactivate states for side-thrust, afterburner
            pshipSource->SetStateBits(0x70000, 0);      //deactivate states for missiles, mines, chaff
            pshipSource->ExportShipUpdate(&(pfmSetCluster->shipupdate));
            pfmSetCluster->cookie = NewCookie();
        }

        {
            for (ShipLinkIGC*   pshiplink = pcluster->GetShips()->first(); pshiplink; pshiplink = pshiplink->next())
            {
                IshipIGC * pshipExist = pshiplink->data();
                if ((pshipExist != GetIGCShip()) && (pshipExist != pshipParent))
                {
                  IshipIGC*   pshipExistParent = pshipExist->GetParentShip();

                  //Tell the new player where the existing ship is/was
                  //provided the existing ship is not the child of some other ship
                  //and is not the parent of the new ship
                  if (pshipExistParent == NULL) 
                  {
                    CFSShip * pfsShipExist = (CFSShip *) pshipExist->GetPrivateData();

                    pfsShipExist->QueueLoadoutChange();

                    BEGIN_PFM_CREATE(g.fm, pfmSSU, S, SINGLE_SHIP_UPDATE)
                    END_PFM_CREATE

                    //Always use the ship update based on the server's current view of the universe
                    //(this shouldn't be a lot worse than anything the player sent and it is easier)
                    pshipExist->ExportShipUpdate(&(pfmSSU->shipupdate));

                    {
                        ImodelIGC*  pmodelTarget = pshipExist->GetCommandTarget(c_cmdCurrent);
                        if (pmodelTarget)
                        {
                            pfmSSU->otTarget = pmodelTarget->GetObjectType();
                            pfmSSU->oidTarget = pmodelTarget->GetObjectID();
                        }
                        else
                        {
                            pfmSSU->otTarget = NA;
                            pfmSSU->oidTarget = NA;
                        }                    
                    }
                    pfmSSU->bIsRipcording = pshipExist->fRipcordActive();
                  }
                }
            }
        }

        {
            // Let's build up a list of station updates so we can batch 'em down
            IsideIGC* pside = GetIGCShip()->GetSide();

            {
                const StationListIGC * pstnlist = pcluster->GetStations();
                int nStations = 0;
                {
                    //Count the number of visible stations
                    for (StationLinkIGC* pstnlink = pstnlist->first(); pstnlink; pstnlink = pstnlink->next())
                    {
                        IstationIGC*  pstation = pstnlink->data();
                        if (pstation->SeenBySide(pside))
                            nStations++;
                    }
                }

                if (nStations != 0)
                {
                    // tell the client what happened
                    BEGIN_PFM_CREATE(g.fm, pfmStationsUpdate, S, STATIONS_UPDATE)
                      FM_VAR_PARM(NULL, nStations * sizeof(StationState))
                    END_PFM_CREATE

                    StationState* pss = (StationState*)(FM_VAR_REF(pfmStationsUpdate, rgStationStates));
                    for (StationLinkIGC* pstnlink = pstnlist->first(); pstnlink; pstnlink = pstnlink->next())
                    {
                        IstationIGC * pstation = pstnlink->data();
                        if (pstation->SeenBySide(pside))
                        {
                            pss->stationID            = pstation->GetObjectID();
                            pss->bpHullFraction       = pstation->GetFraction();
                            (pss++)->bpShieldFraction = pstation->GetShieldFraction();
                        }
                    }
                }
            }

            {
                //Let's build up a list of probe updates and batch them on down (only damage & visible probes)
                const ProbeListIGC * pprblist = pcluster->GetProbes();
                int nProbes = 0;
                {
                    for (ProbeLinkIGC* pprblink = pprblist->first(); pprblink; pprblink = pprblink->next())
                    {
                        if (pprblink->data()->SeenBySide(pside))
                            nProbes++;
                    }
                }

                if (nProbes != 0)
                {
                    BEGIN_PFM_CREATE(g.fm, pfmProbesUpdate, S, PROBES_UPDATE)
                      FM_VAR_PARM(NULL, nProbes * sizeof(ProbeState))
                    END_PFM_CREATE

                    ProbeState* pps = (ProbeState*)(FM_VAR_REF(pfmProbesUpdate, rgProbeStates));
                    for (ProbeLinkIGC* pprblink = pprblist->first(); pprblink; pprblink = pprblink->next())
                    {
                        IprobeIGC * pprobe = pprblink->data();

                        if (pprobe->SeenBySide(pside))
                        {
                            pps->probeID        = pprobe->GetObjectID();
                            (pps++)->bpFraction = pprobe->GetFraction();
                        }
                    }
                }
            }

            {
                //Let's build up a list of asteroid updates and batch them on down
                const AsteroidListIGC * pastlist = pcluster->GetAsteroids();
                int nAsteroids = 0;
                {
                    for (AsteroidLinkIGC* pastlink = pastlist->first(); pastlink; pastlink = pastlink->next())
                    {
                        if (pastlink->data()->SeenBySide(pside))
                            nAsteroids++;
                    }
                }

                if (nAsteroids != 0)
                {
                    BEGIN_PFM_CREATE(g.fm, pfmAsteroidsUpdate, S, ASTEROIDS_UPDATE)
                      FM_VAR_PARM(NULL, nAsteroids * sizeof(AsteroidState))
                    END_PFM_CREATE

                    AsteroidState* pas = (AsteroidState*)(FM_VAR_REF(pfmAsteroidsUpdate, rgAsteroidStates));
                    for (AsteroidLinkIGC* pastlink = pastlist->first(); pastlink; pastlink = pastlink->next())
                    {
                        IasteroidIGC * pasteroid = pastlink->data();

                        if (pasteroid->SeenBySide(pside))
                        {
                            pas->asteroidID         = pasteroid->GetObjectID();
                            pas->ore                = short(pasteroid->GetOre());
                            pas->co.Set(pasteroid->GetOrientation());
                            (pas++)->bpFraction     = pasteroid->GetFraction();
                        }
                    }
                }
            }
        }
    
        //Also send the identical message to all of the ship's children]
		if (!bViewOnly) //TheRock 4-1-2010 fixed ships overlaying in f3 while on a turret
        {
            for (ShipLinkIGC*   psl = GetIGCShip()->GetChildShips()->first(); (psl != NULL); psl = psl->next())
            {
                CFSShip*    pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
                assert (pfsShip->IsPlayer());
                pfsShip->GetPlayer()->ResetLastUpdate();
                g.fm.SendMessages(pfsShip->GetPlayer()->GetConnection(), FM_GUARANTEED, FM_DONT_FLUSH);
            }
        }
        g.fm.SendMessages(GetConnection(), FM_GUARANTEED, FM_FLUSH);
    }
  }
  else if (bViewOnly)
  {
    IstationIGC*    pstation = GetIGCShip()->GetStation();
    assert (pstation);

    CFSCluster*     pfsCluster = (CFSCluster*)(pstation->GetCluster()->GetPrivateData());
    SetDPGroup(pfsCluster, false);
  }
  else
    SetDPGroup(NULL, false);
}

CFSPlayer::CFSPlayer(CFMConnection * pcnxn, int characterID, const char * szCDKey,
                      TRef<IshipIGC> pShip, bool fCanCheat, CSteamID &steamID) :
  CFSShip(pShip, true), // parent
  m_fReady(true),
  m_characterId(characterID),
  m_fCanCheat(fCanCheat),
  m_pcnxn(pcnxn),
  m_pgrp(NULL),
  m_ucLastUpdate(c_ucNone),
  m_pfsClusterFlying(NULL),
  m_pclusterLifepod(NULL),
  m_bannedSideMask(0),
  // mdvalley: add last flown for
  m_lastSide(SIDE_TEAMLOBBY),
  m_chatBudget(c_chatBudgetMax),
  m_strCDKey(szCDKey),
  m_pSteamAchievements(steamID) // BT - STEAM
{
	m_pSteamAchievements.InitiateStatsRequest();

  m_pcnxn->SetPrivateData((DWORD) this); // set up two-way link between connection and this
  m_dwStartTime = m_timeUpdate = g.timeNow;

  ShipID    shipID = pShip->GetObjectID();

  LPCSTR pszContext = pShip->GetMission()->GetContextName();

  _AGCModule.TriggerContextEvent(NULL, EventID_LoginServer, pszContext,
    pShip->GetName(), GetConnection()->GetID(), -1, -1, 1,
    "User", VT_I4, GetConnection()->GetID());
}


CFSPlayer::~CFSPlayer()
{
  /*
    People get removed from the groups automatically when the player is destroyed
  ZSucceeded(g.pDirectPlay->DeletePlayerFromGroup(g.dpidEveryone, m_dpid));
  if (NA == GetSide())
    ZSucceeded(g.pDirectPlay->DeletePlayerFromGroup(g.dpidLobby, m_dpid));
  else
    ZSucceeded(g.pDirectPlay->DeletePlayerFromGroup(g.rgdpidSides[GetSide()], m_dpid));

  if (m_dpidGroup)
    ZSucceeded(g.pDirectPlay->DeletePlayerFromGroup(m_dpidGroup, m_dpid));
  // NYI: They might be in sector overview too
  */
  m_pcnxn->SetPrivateData(0); // disconnect two-way link between connection and this

  LPCSTR pszContext = GetIGCShip() ? GetIGCShip()->GetMission()->GetContextName() : NULL;

  _AGCModule.TriggerContextEvent(NULL, EventID_LogoutServer, pszContext,
    GetName(), GetConnection()->GetID(), -1, -1, 1,
    "User", VT_I4, GetConnection()->GetID());

  ShipID shipID = GetShipID();
  if (NULL != g.pServerCounters)
    g.pServerCounters->cPlayersOnline--;

  if (GetMission())
    GetMission()->RemovePlayerFromMission(this, QSR_LinkDead);

  // Get rid of all the PersistPlayerScoreObjects
  PersistPlayerScoreObjectLink * pperplrscoLink = NULL;
  while (pperplrscoLink = m_perplrscoreList.first())
  {
    delete pperplrscoLink->data();
    delete pperplrscoLink;
  }
  
  // Too risky, dplay may block-- g.fm.GetDPlay()->DestroyPlayer(m_dpid);
}


void CFSPlayer::Launch(IstationIGC* pstation)
{
  assert (GetIGCShip()->GetBaseHullType() != NULL);

  SetWarpState();
  CFSShip::Launch(pstation);
}


void CFSPlayer::Dock(IstationIGC * pstation)
{
  IshipIGC * pship = GetIGCShip();

  CFSCluster*       pfsCluster = (CFSCluster*)(pstation->GetCluster()->GetPrivateData());
  SetDPGroup(pfsCluster, false);

  ShipStatus* pss = GetShipStatus(pship->GetSide()->GetObjectID());
  pss->SetStationID(pstation->GetObjectID());
  pss->SetSectorID(pstation->GetCluster()->GetObjectID());

  // Tell them what station they're at
  BEGIN_PFM_CREATE(g.fm, pfmDocked, S, DOCKED)
  END_PFM_CREATE
  pfmDocked->stationID = pstation->GetObjectID();
  g.fm.SendMessages(GetConnection(), FM_GUARANTEED, FM_FLUSH);

  //CFSShip::Dock(pstation);
}


void CFSPlayer::SetSide(CFSMission * pfsMission, IsideIGC * pside)
{
  CFSMission * pfsmOld = GetMission();
  IsideIGC* psideOld = GetSide();
  CFSShip::SetSide(pfsMission, pside);

  // Their persist score now becomes their current score, and other stuff is reset
  if (pside)
  {
    PlayerScoreObject* ppso = GetPlayerScoreObject();
    ppso->SetPersist(GetPersistPlayerScore(pside->GetCivilization()->GetObjectID()));

    memset(m_ptDesiredLoadout, 0, sizeof(m_ptDesiredLoadout));
  }

  // can't guarantee success on group changes--dplay player may be already blown away
  if (pside != psideOld)
  {
    if (pfsmOld)
      g.fm.DeleteConnectionFromGroup(CFSSide::FromIGC(psideOld)->GetGroup(), GetConnection());

    if (pfsMission)
    {
      bool fLobbySide = SIDE_TEAMLOBBY == pside->GetObjectID();
      CFMGroup * pgrp = CFSSide::FromIGC(pside)->GetGroup();
      g.fm.AddConnectionToGroup(pgrp, GetConnection());
      
      if (fLobbySide)
        g.fm.DeleteConnectionFromGroup(pfsMission->GetGroupRealSides(), GetConnection());
      else
        g.fm.AddConnectionToGroup(pfsMission->GetGroupRealSides(), GetConnection());
    }
  }
  else
    assert (pfsmOld == pfsMission);

  if (!pside && m_pgrp)
  {
    g.fm.DeleteConnectionFromGroup(m_pgrp, GetConnection());
    m_pgrp = NULL;
  }


  //
  // If the user just joined or left the game, consider firing an AGC event
  //
  if (pfsmOld != pfsMission) 
  {

    if (pfsmOld) // if leaving a game
    {
        const char * szName = pfsmOld->GetIGCMission() && pfsmOld->GetIGCMission()->GetMissionParams() ? pfsmOld->GetIGCMission()->GetMissionParams()->strGameName : "?" ;
        int id = pfsmOld->GetIGCMission() ? pfsmOld->GetIGCMission()->GetMissionID() : -1;
        long idShip = (AGC_AdminUser << 16) | CAdminUser::DetermineID(this->GetPlayer());

        LPCSTR pszContext = pfsmOld->GetIGCMission() ? pfsmOld->GetIGCMission()->GetContextName() : NULL;

      _AGCModule.TriggerContextEvent(NULL, EventID_LogoutGame, pszContext,
        GetName(), idShip, -1, -1, 2,
        "GameID",   VT_I4,    id,
        "GameName", VT_LPSTR, szName);
    }

    if (pfsMission) // if joining a game
    {
        const char * szName = pfsMission->GetIGCMission() && pfsMission->GetIGCMission()->GetMissionParams() ? pfsMission->GetIGCMission()->GetMissionParams()->strGameName : "?" ;
        int id = pfsMission->GetIGCMission() ? pfsMission->GetIGCMission()->GetMissionID() : -1;
        long idShip = (AGC_AdminUser << 16) | CAdminUser::DetermineID(this->GetPlayer());

        LPCSTR pszContext = pfsMission->GetIGCMission() ? pfsMission->GetIGCMission()->GetContextName() : NULL;

      _AGCModule.TriggerContextEvent(NULL, EventID_LoginGame, pszContext,
        GetName(), idShip, -1, -1, 2,
        "GameID",   VT_I4,    id,
        "GameName", VT_LPSTR, szName);
    }
  }

  //
  // If the user just joined or left the team, consider firing an AGC event
  //
  if (psideOld  != pside) 
  {
	  //Imago #169
	  if ( (psideOld && psideOld->GetObjectID() == SIDE_TEAMLOBBY) || (pside && pside->GetObjectID() == SIDE_TEAMLOBBY))
		 if (pfsMission)
			 pfsMission->SetLobbyIsDirty();

    long idShip = (AGC_AdminUser << 16) | CAdminUser::DetermineID(this->GetPlayer());

    if (psideOld ) // if leaving a side
    {
      LPCSTR pszContext = GetIGCShip()->GetMission() ? GetIGCShip()->GetMission()->GetContextName() : NULL;
	  // TE Modify LeaveTeam AGCEvent to include MissionID, and change UniqueID to ObjectID
      _AGCModule.TriggerContextEvent(NULL, EventID_LeaveTeam, pszContext,
        GetName(), idShip, psideOld->GetUniqueID(), -1, 3, // Changed UniqueID to ObjectID. Modified ParamCount to 3
        "MissionID", VT_I4	, psideOld->GetMission()->GetMissionID(),	// Added line MissionID as param
	   "Team"    , VT_I4   , psideOld->GetObjectID(), // Changed UniqueID to ObjectID
        "TeamName", VT_LPSTR, psideOld->GetName());

	  // old event
      //_AGCModule.TriggerContextEvent(NULL, EventID_LeaveTeam, pszContext,
      //  GetName(), idShip, psideOld->GetUniqueID(), -1, 2,
      //  "Team"    , VT_I4   , psideOld->GetUniqueID(),
      //  "TeamName", VT_LPSTR, psideOld->GetName());
	}

    if (pside) // if joining a side
    {
        // TE Modify JoinTeam AGCEvent to include MissionID, and change UniqueID to ObjectID
		_AGCModule.TriggerEvent(NULL, EventID_JoinTeam, GetName(), idShip,
         pside->GetUniqueID(), -1, 3, // Changed UniqueID to ObjectID. Modified ParamCount to 3
		 "MissionID", VT_I4	, pfsMission->GetMissionID(),		// Added line MissionID
         "Team"    , VT_I4   , pside->GetObjectID(), // Changed UniqueID to ObjectID.
         "TeamName", VT_LPSTR, pside->GetName());
     
		// old event
		//_AGCModule.TriggerEvent(NULL, EventID_JoinTeam, GetName(), idShip,
        //pside->GetUniqueID(), -1, 2,
        //"Team"    , VT_I4   , pside->GetUniqueID(),
        //"TeamName", VT_LPSTR, pside->GetName());
    }
  }
}

void CFSPlayer::SetShipUpdate(const ClientShipUpdate& su)
{
    if (su.time > m_timeUpdate)
    {
        ShipUpdateStatus sus = GetIGCShip()->ProcessShipUpdate(su);
        if (sus == c_susAccepted)
        {
            m_ucLastUpdate = c_ucShipUpdate;

            m_su = su;
            m_timeUpdate = su.time;

            if (m_warpState == warpNoUpdate)
            {
                m_warpState = warpWaiting;
                m_timeNextWarp = g.timeNow + 2.0f;  //Some fudge factor
            }
            else if ((m_warpState == warpWaiting) && (g.timeNow >= m_timeNextWarp))
                m_warpState = warpReady;
        }
        else if (sus == c_susRejected)
        {
            BEGIN_PFM_CREATE(g.fm, pfmSR, S, SHIP_RESET)
            END_PFM_CREATE

            GetIGCShip()->ExportShipUpdate(&(pfmSR->shipupdate));
            pfmSR->cookie = NewCookie();

            g.fm.SendMessages(GetConnection(), FM_GUARANTEED, FM_FLUSH);
        }
    }
}

void CFSPlayer::SetActiveTurretUpdate(const ClientActiveTurretUpdate& atu)
{
    if (atu.time > m_timeUpdate)
    {
        ShipUpdateStatus sus = GetIGCShip()->ProcessShipUpdate(atu);
        if (sus == c_susAccepted)
        {
            m_ucLastUpdate = c_ucActiveTurretUpdate;

            m_atu = atu;
            m_timeUpdate = atu.time;
        }
    }
}
void CFSPlayer::SetInactiveTurretUpdate(void)
{
    m_ucLastUpdate = c_ucInactiveTurretUpdate;
    GetIGCShip()->SetStateM(0);
}

/*-------------------------------------------------------------------------
 * SetDPGroup
 *-------------------------------------------------------------------------
 * Purpose:
 *    Adds the player to a "primary" group within a mission, i.e. either
 *        flying or docked in a sector. Players in missions are always in 
 *        exactly 1 primary group.
 * 
 * Side Effects:
 *    Removes them from the group they were in
 */
void CFSPlayer::SetDPGroup(CFSCluster*  pfsCluster, bool bFlying)
{
  if (m_pfsClusterFlying)
  {
      m_pfsClusterFlying->RemoveFlyingPlayer(this);
  }

  CFMGroup* pgrp;
  if (pfsCluster)
  {
      if (bFlying)
      {
          pgrp = pfsCluster->GetClusterGroups()->pgrpClusterFlying;

          m_pfsClusterFlying = pfsCluster;
          pfsCluster->AddFlyingPlayer(this);
      }
      else
      {
          pgrp = pfsCluster->GetClusterGroups()->pgrpClusterDocked;
          m_pfsClusterFlying = NULL;
      }
  }
  else
  {
      pgrp = NULL;
      m_pfsClusterFlying = NULL;
  }

  if (m_pgrp)
  {
#ifdef DEBUG
    // mmf only log this in debug build
    debugf("Removing %s(%u) from group %s(%u)\n", GetName(), GetConnection()->GetID(), 
            m_pgrp->GetName(), m_pgrp->GetID());
#endif
    g.fm.DeleteConnectionFromGroup(m_pgrp, GetConnection());
  }
  m_pgrp = pgrp;
  if (m_pgrp)
	g.fm.AddConnectionToGroup(m_pgrp, GetConnection());
}


bool CFSPlayer::IsMissionOwner()
{
  return this == GetMission()->GetOwner();
}


void CFSPlayer::SetReady(bool fReady)
{
  m_fReady = fReady;
  GetMission()->PlayerReadyChange(this);
}

void CFSPlayer::SetAutoDonate(CFSPlayer* pplayer, Money amount, bool bSend)
{
  assert (pplayer != this);

  ShipID    sidDonateBy = GetShipID();
  ShipID    sidDonateTo;

  IshipIGC* pship;
  if (pplayer)
  {
      pship = pplayer->GetIGCShip()->GetAutoDonate();
      if (pship == NULL)
          pship = pplayer->GetIGCShip();
      else
      {
          assert (pship->GetAutoDonate() == NULL);

          CFSPlayer*    pplayerNew = ((CFSShip*)(pship->GetPrivateData()))->GetPlayer();

          if (pplayerNew == this)
          {
              //We are trying to autodonate to someone who is already autodonating to us
              //Tell them to stop first (but don't send any messages)
              pplayer->SetAutoDonate(NULL, 0, false);

              //and then set our autodonate to go to them anyhow
              pship = pplayer->GetIGCShip();
          }
          else
          {
              //The person we would like to donate to was already donating to someone else
              //donate to who ever they were donating to.
              pplayer = pplayerNew;
          }
      }

      assert (pship->GetAutoDonate() == NULL);
      assert (pplayer->GetIGCShip() == pship);

      sidDonateTo = pship->GetObjectID();

      if (amount != 0)
      {
          TrapHack(GetMoney() >= amount);
          pplayer->SetMoney(amount + pplayer->GetMoney());
          SetMoney(GetMoney() - amount);
      }
  }
  else
  {
      pship = NULL;
      sidDonateTo = NA;
  }

  GetIGCShip()->SetAutoDonate(pship);

  BEGIN_PFM_CREATE(g.fm, pfmAD, S, AUTODONATE)
  END_PFM_CREATE
  pfmAD->sidDonateBy = sidDonateBy;
  pfmAD->sidDonateTo = sidDonateTo;
  pfmAD->amount = amount;

  if (bSend)
    g.fm.SendMessages(CFSSide::FromIGC(GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);
}

bool CFSPlayer::GetIsMemberOfSquad(SquadID squadID)
{
  if (squadID == NA)
      return true;

  for (SquadMembershipLink* pSquadLink = GetSquadMembershipList()->first();
    pSquadLink != NULL; pSquadLink = pSquadLink->next())
  {
    if (pSquadLink->data()->GetID() == squadID)
      return true;
  }

  return false;
}

bool CFSPlayer::GetCanLeadSquad(SquadID squadID)
{
  if (squadID == NA)
      return true;

  for (SquadMembershipLink* pSquadLink = GetSquadMembershipList()->first();
    pSquadLink != NULL; pSquadLink = pSquadLink->next())
  {
    SquadMembership* psquad = pSquadLink->data();
    if (psquad->GetID() == squadID)
      return psquad->GetIsAssistantLeader() || psquad->GetIsLeader();
  }

  return false;
}

SquadID CFSPlayer::GetPreferredSquadToLead()
{
  SquadMembershipLink* pSquadLink;

  // look for a squad which this player leads...
  for (pSquadLink = GetSquadMembershipList()->first();
    pSquadLink != NULL; pSquadLink = pSquadLink->next())
  {
    SquadMembership* psquad = pSquadLink->data();
    if (psquad->GetIsLeader())
      return psquad->GetID();
  }

  // failing that, look for a squad in which this player is an assistant leader...
  for (pSquadLink = GetSquadMembershipList()->first();
    pSquadLink != NULL; pSquadLink = pSquadLink->next())
  {
    SquadMembership* psquad = pSquadLink->data();
    if (psquad->GetIsAssistantLeader())
      return psquad->GetID();
  }

  return NA;
}

/******************************************************************
                        CFSDrone
******************************************************************/
CFSDrone::CFSDrone(IshipIGC* pship) :
  CFSShip(pship, false)
{
    if (GetMission()->GetStage() == STAGE_STARTED)
    {
      //Hook the drone's side/score information up (since drones live only on a single side for their entire lifespan).
      IsideIGC* pside = pship->GetSide();
      assert (pside);
      CFSSide*  pfsside = CFSSide::FromIGC(pside);
      assert (pfsside);

      GetPlayerScoreObject()->Connect(g.timeNow);
    }
}

CFSDrone::~CFSDrone()
{
}

/*
void CFSDrone::GetHeavyShipUpdate(HeavyShipUpdate * phsu) 
{
  GetIGCShip()->ExportShipUpdate(phsu);
  phsu->SetTimeOffset(g.timeNow, GetIGCShip()->GetLastUpdate());
}


void CFSDrone::GetLightShipUpdate(LightShipUpdate * plsu)
{
  GetIGCShip()->ExportShipUpdate(plsu);
}
*/

void CFSDrone::Launch(IstationIGC* pstation)
{
    CFSShip::Launch(pstation);
}

void CFSDrone::Dock(IstationIGC * pstation)
{
    //Drones always instantly undock
    //CFSShip::Dock(pstation);
}



