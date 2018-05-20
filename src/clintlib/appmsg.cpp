#include "pch.h"
#include    <limits.h>

//
// Helper function for handling ship updates.
//

//
// Big function to handle default messaging.
// Three possible return codes:
//  S_OK    message handled
//  S_FALSE message not handled
//  E_FAIL  error, abort
HRESULT BaseClient::HandleMsg(FEDMESSAGE* pfm,
                        Time lastUpdate, Time now)
{
    HRESULT hr = S_OK;  //handled by default

#ifdef DUMPMSGS
    debugf("Received message type %d of length %d\n", pfm->fmid, pfm->cbmsg);
#endif // DUMPMSGS

    // keep track of the last server message, but only count unreliable 
    // messages while the client is in flight.
    if (!m_ship || !IsInGame() || GetCluster() == NULL || pfm->fmid == FM_CS_PING 
        || pfm->fmid == FM_S_HEAVY_SHIPS_UPDATE || pfm->fmid == FM_S_LIGHT_SHIPS_UPDATE)
    {
        m_timeLastServerMessage = now;
    }


    switch(pfm->fmid)
    {
        case FM_S_LOGONACK:
        {
            CASTPFM(pfmLogonAck, S, LOGONACK, pfm);
            if (pfmLogonAck->fValidated)
            {
                debugf("I am ship %s, shipid=%d\n", FM_VAR_REF(pfmLogonAck, CharName_OR_FailureReason), pfmLogonAck->shipID);
                lstrcpy(m_szCharName, FM_VAR_REF(pfmLogonAck, CharName_OR_FailureReason)); // remember who we are
                m_fLoggedOn = true;
                SetCookie(pfmLogonAck->cookie);

                // set the client-server time offset properly
                {
                    m_cUnansweredPings = 0;

                    DWORD newLag = 100; // reasonable default value

                    //Try a new way of doing the correction
                    DWORD newOffset = pfmLogonAck->timeServer.clock() - now.clock() + newLag;

                    m_serverOffsetValidF = true;
                    m_serverLag = newLag;
                    m_serverOffset = newOffset;

                    m_timeLastPing = now;
                    m_timeLastPingServer = pfmLogonAck->timeServer;
                }
            }

            //                                
            // If we need to download art files, then
            // don't ack logon until done
            //

            char * szFailureReason = pfmLogonAck->fValidated ? NULL : FM_VAR_REF(pfmLogonAck, CharName_OR_FailureReason);

            OnLogonAck(pfmLogonAck->fValidated, pfmLogonAck->fRetry, szFailureReason);
        }
        break;

        case FM_S_URLROOT:
        {
            CASTPFM(pfmUrlRoot, S, URLROOT, pfm);
            UTL::SetUrlRoot(FM_VAR_REF(pfmUrlRoot, UrlRoot));
        }
        break;

        case FM_S_EXPORT:
        {
            if (IsWaitingForGameRestart())
                break;

            CASTPFM(pfmExport, S, EXPORT, pfm);

            IObject* u = m_pCoreIGC->CreateObject(lastUpdate,
                                        pfmExport->objecttype,
                                        FM_VAR_REF(pfmExport, exportData),
                                        pfmExport->cbexportData);


            if (u)
                u->Release();
            else
            {
                //Station exports are allowed to "fail" because they may simply update an existing station.
                //Ships are allowed to fail because sometimes an existing ship will be updated.
				// mmf commented out assert as it seems to trigger when TP is destroyed for being too close to asteroid
				//     that returns S_FALSE
                // assert ((pfmExport->objecttype == OT_station) || (pfmExport->objecttype == OT_ship));
            }
            Sleep(0);
        }
        break;

        case FM_S_POSTER:
        {
            // I think this message is no longer used.
            assert(false);
            /*
            CASTPFM(pfmPoster, S, POSTER, pfm);

            IclusterIGC * cluster =
                                m_pCoreIGC->GetCluster(pfmPoster->sectorID);
            if (cluster)
            {
                float   cosLatr = cos(pfmPoster->latitude);
                Vector  position(cosLatr * cos(pfmPoster->longitude), 
                                 cosLatr * sin(pfmPoster->longitude), 
                                 sin(pfmPoster->latitude));

                cluster->GetClusterSite()->AddPoster(
                                            pfmPoster->textureName, 
                                            position,
                                            pfmPoster->radius);
            }
            */
        }
        break;

        case FM_S_RELAUNCH_SHIP:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmRelaunch, S, RELAUNCH_SHIP, pfm);

            IshipIGC*   pShip = m_pCoreIGC->GetShip(pfmRelaunch->shipID);
            if (pShip)
            {
                IshipIGC*   pshipParent = m_pCoreIGC->GetShip(pfmRelaunch->carrierID);
                assert (pshipParent);

                pShip->SetParentShip(NULL);

                pShip->ProcessShipLoadout(pfmRelaunch->cbloadout, (const ShipLoadout*)(FM_VAR_REF(pfmRelaunch, loadout)), true);
                pShip->SetAmmo(SHRT_MAX);
                pShip->SetFuel(FLT_MAX);
                pShip->SetEnergy(pShip->GetHullType()->GetMaxEnergy());

                pShip->SetPosition(pfmRelaunch->position);
                pShip->SetVelocity(pfmRelaunch->velocity);
                {
                    Orientation o;
                    pfmRelaunch->orientation.Export(&o);
                    pShip->SetOrientation(o);
                }

                if (pShip == m_ship)
                {
                    SetCookie(pfmRelaunch->cookie);
                    PlayNotificationSound(salRepairedAtCarrierSound, GetShip());
                    OverrideCamera(pshipParent);
                }
            }
        }
        break;

        case FM_S_EJECT:
        {
            if (!IsInGame())
                break;

            IclusterIGC*    pcluster = GetCluster();
            if (pcluster)
            {
                CASTPFM(pfmEject, S, EJECT, pfm);

                IshipIGC*   pShip = m_pCoreIGC->GetShip(pfmEject->shipID);
                if (pShip)
                {
                    if (pShip->GetParentShip())
                    {
                        pShip->SetParentShip(NULL);
                        m_pClientEventSource->OnBoardShip(pShip, NULL);
                    }
                    else
                    {
                        //Ship 'died' .
                        assert (pShip->GetChildShips()->n() == 0);
                        {
                            const PartListIGC*  plist = pShip->GetParts();
                            PartLinkIGC*        plink;
                            while (plink = plist->first())  //Not ==
                                plink->data()->Terminate();
                        }

                        //and fuel and ammo
                        pShip->SetAmmo(0);
                        pShip->SetFuel(0.0f);
                    }
                    {
                        {
                            for (MissileLinkIGC*  pml = pcluster->GetMissiles()->first();
                                 (pml != NULL);
                                 pml = pml->next())
                            {
                                ImissileIGC*    pmissile = pml->data();
                                if (pmissile->GetTarget() == pShip)
                                    pmissile->SetTarget(NULL);
                            }
                        }

                        //Eject the player
                        pShip->SetBaseHullType(pShip->GetSide()->GetCivilization()->GetLifepod());

                        //Put a spin on the ship as it leaves the bad guys.
                        //Note ... this needs to match the code in fedsrv.cpp
                        pShip->SetCurrentTurnRate(c_axisRoll, pi * 2.0f);

                        pShip->SetPosition(pfmEject->position);
                        pShip->SetVelocity(pfmEject->velocity);
                        {
                            Orientation o(pfmEject->forward);
                            pShip->SetOrientation(o);
                        }
                    }

                    if (pShip == m_ship)
                    {
                        SetCookie(pfmEject->cookie);
                        ImodelIGC*  pNearestBase = FindTarget (pShip, c_ttStation | c_ttFriendly | c_ttAnyCluster | c_ttProbe, pShip->GetCommandTarget (c_cmdCurrent), pcluster, &pfmEject->position, NULL, c_sabmRescue | c_sabmRescueAny | c_sabmLand);
                        pShip->SetCommand (c_cmdCurrent, pNearestBase, c_cidGoto);
                        SetAutoPilot (true);
                        bInitTrekJoyStick = true;
                        PlayNotificationSound(salAutopilotEngageSound, GetShip());
                    }
                }
            }
        }
        break;

        case FM_S_DESTROY_TREASURE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmTreasure, S, DESTROY_TREASURE, pfm);

            {
                IclusterIGC* c = m_pCoreIGC->GetCluster(pfmTreasure->sectorID);
                assert (c);

                ItreasureIGC* treasure = c->GetTreasure(pfmTreasure->treasureID);
                if (treasure)
                    treasure->Terminate();
            }
        }
        break;

        case FM_S_ACQUIRE_TREASURE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmTreasure, S, ACQUIRE_TREASURE, pfm);

            switch (pfmTreasure->treasureCode)
            {
                case c_tcPart:
                    {
                        SetMessageType(BaseClient::c_mtGuaranteed);
                        BEGIN_PFM_CREATE(m_fm, pfmAck, C, TREASURE_ACK)
                        END_PFM_CREATE

                        pfmAck->mountID = m_ship->HitTreasure(c_tcPart, pfmTreasure->treasureID, pfmTreasure->amount);
                    }
                break;

                case c_tcPowerup:
                    PlaySoundEffect(pickUpPowerupSound, GetShip());
                break;

                case c_tcDevelopment:
                    PlaySoundEffect(pickUpDevelopmentSound, GetShip());
                break;

                case c_tcCash:
                {
                    PlaySoundEffect(pickUpCashSound, GetShip());

                    IshipIGC*   pshipDonate = m_ship->GetAutoDonate();
                    if (pshipDonate)
                        PostText(false, "You donated your reward of $%d to %s.", pfmTreasure->amount, pshipDonate->GetName());
                    else
                        PostText(false, "You received a reward of $%d.", pfmTreasure->amount);

                }
                break;

                case c_tcFlag:
                    PlaySoundEffect(pickUpDevelopmentSound, GetShip());     //NYI pickUpFlagSound
                break;
            }
        }
        break;

        case FM_S_PLAYER_RESCUED:
        {
            CASTPFM(pfmPlayerRescued, S, PLAYER_RESCUED, pfm);
            ShipID  sid = GetShipID();
            if (pfmPlayerRescued->shipIDRescuer == sid)
            {
                // I just picked someone up.
                PlayerInfo* pplayerRescuee = FindPlayer(pfmPlayerRescued->shipIDRescuee);
                
                if (!pplayerRescuee)
                {
                    assert(false);
                    break;
                }

                PostText(false, "You rescued %s.", pplayerRescuee->CharacterName());
                PlaySoundEffect(rescuePlayerSound, GetShip());
            }
            else if (pfmPlayerRescued->shipIDRescuee == sid)
            {
                // I was just teleported back to base.
                PlayerInfo* pplayerRescuer = FindPlayer(pfmPlayerRescued->shipIDRescuer);
                
                if (!pplayerRescuer)
                {
                    assert(false);
                    break;
                }

                PostText(true, "%s rescued you.", pplayerRescuer->CharacterName());
                PlaySoundEffect(jumpSound);
            }
        }
        break;

        case FM_S_BALLOT:
        {
            CASTPFM(pfmBallot, S, BALLOT, pfm);

            // if I'm not the one who called this vote...
            if (!(pfmBallot->otInitiator == OT_ship && pfmBallot->oidInitiator == GetShipID())
              && !(pfmBallot->otInitiator == OT_side && pfmBallot->oidInitiator == GetSideID()))
            {
				// KGJV #110
				// if bHideToLeader and i'm the team leader then auto vote no
				if (pfmBallot->bHideToLeader &&	MyPlayerInfo()->IsTeamLeader())
				{
					// auto vote no
					SetMessageType(c_mtGuaranteed);
					BEGIN_PFM_CREATE(m_fm, pfmVote, C, VOTE)
					END_PFM_CREATE
					pfmVote->ballotID = pfmBallot->ballotID;
					pfmVote->bAgree = false;
				}
				else
				{
	                // then propose the issue.
					m_listBallots.PushEnd(BallotInfo(
						(char*)(FM_VAR_REF(pfmBallot, BallotText)) + ZString("Press [Y] to vote yes, [N] to vote no."), 
						pfmBallot->ballotID, 
						ClientTimeFromServerTime(pfmBallot->timeExpiration)
						));
				}
		
            }
        }
        break;

        case FM_S_CANCEL_BALLOT:
        {
            CASTPFM(pfmCancelBallot, S, CANCEL_BALLOT, pfm);

            // destroy any ballot in the queue with this ID
            BallotList::Iterator iterBallot(m_listBallots);

            while (!iterBallot.End())
            {
                if (iterBallot.Value().GetBallotID() == pfmCancelBallot->ballotID)
                    iterBallot.Remove();
                else
                    iterBallot.Next();
            }
        }
        break;

        case FM_CS_PING:
        {
            CASTPFM(pfmPing, CS, PING, pfm);
            assert (now >= pfmPing->timeClient);

            //
            // Assume lag evenly split here to there & there to here.
            //
            DWORD newLag = (now.clock() - pfmPing->timeClient.clock()) >> 1;
            if (newLag < 1000)
            {
                m_cUnansweredPings = 0;

                //Try a new way of doing the correction
                //DWORD newOffset = pfmPing->timeServer.clock() - pfmPing->timeClient.clock();
                DWORD newOffset = pfmPing->timeServer.clock() - now.clock() + newLag;

                if (m_serverOffsetValidF)
                {
                    float currentSync = float(now.clock() - m_timeLastPing.clock()) /
                             float(pfmPing->timeServer.clock() - m_timeLastPingServer.clock());
                    m_sync = m_sync * 3/4 + currentSync/4;

                    if (newLag > m_serverLag)
                    {
                        //new lag is worse ... heavily weight the old lag
                        m_serverOffset += ((int)(newOffset - m_serverOffset)) >> 3;
                        m_serverLag = (m_serverLag * 7 + newLag) >> 3;
                    }
                    else
                    {
                        //new lag is better ... average the old & new
                        m_serverOffset += ((int)(newOffset - m_serverOffset)) >> 1;
                        m_serverLag = (m_serverLag + newLag) >> 1;
                    }
                }
                else
                {
                    m_serverOffsetValidF = true;
                    m_serverLag = newLag;
                    m_serverOffset = newOffset;
                }

                m_timeLastPing = now;
                m_timeLastPingServer = pfmPing->timeServer;
            }
        }
        break;

        case FM_S_VIEW_CLUSTER:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmViewCluster, S, VIEW_CLUSTER, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmViewCluster->clusterID);
            assert (pcluster);
            SetViewCluster(pcluster, pfmViewCluster->bUsePosition ? &(pfmViewCluster->position) : NULL);
        }
        break;

        case FM_S_SHIP_DELETE:
        {
            CASTPFM(pfmShipDelete, S, SHIP_DELETE, pfm);

            //
            // Which ship went away ...
            //
            if (pfmShipDelete->shipID != GetShipID())
            {
                IshipIGC* ship = m_pCoreIGC->GetShip(pfmShipDelete->shipID);
                if (ship)
                {
                    //
                    // Found a ship ... why'd it go?
                    //
                    switch (pfmShipDelete->sdr)
                    {
                        case SDR_TERMINATE:
                        case SDR_KILLED:
                        {
                            ship->SetCluster(NULL);
                        }
                        break;

                        default:
                        {
                            //
                            // The ship is no longer visible to the player,
                            // "cache" it by moving it the null sector.
                            // Ignore for your parent ship since this is handled
                            // by getting a docked or set cluster message for
                            // the parent.
                            if (m_ship->GetParentShip() != ship)
                                ship->SetCluster(NULL);
                        }
                    }
                }

                if (pfmShipDelete->sdr == SDR_LOGGEDOFF)
                {
                    //The player is gone ... nuke their player info (if it exists)
                    PlayerLink* l = FindPlayerLink(pfmShipDelete->shipID);

                    if (l)
                    {
                        PlayerInfo* pPlayerInfo = &(l->data());
                        assert (pPlayerInfo);

                        debugf("Logging off ship for %s, ID=%d\n", 
                            pPlayerInfo->CharacterName(), pPlayerInfo->ShipID());

                        RemovePlayerFromSide(pPlayerInfo, QSR_Quit);
                        RemovePlayerFromMission(pPlayerInfo, QSR_Quit);
                    }
                }
            }
        }
        break;

        case FM_S_STATION_DESTROYED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmStation, S, STATION_DESTROYED, pfm);

            IstationIGC * station = m_pCoreIGC->GetStation(
                                                pfmStation->stationID);
            if (station)
            {
                station->GetCluster()->GetClusterSite()->AddExplosion(station,
                                                                      station->GetStationType()->HasCapability(c_sabmFlag)
                                                                      ? c_etLargeStation
                                                                      : c_etSmallStation);
                if (pfmStation->launcher != NA)
                {
                    IshipIGC * pship = GetCore()->GetShip(pfmStation->launcher);
                    PostText(true, START_COLOR_STRING "%s" END_COLOR_STRING " destroyed " START_COLOR_STRING "%s's %s" END_COLOR_STRING " in %s.", 
                        (PCC) ConvertColorToString (pship ? pship->GetSide ()->GetColor () : Color::White ()),
                        (pship ? pship->GetName() : "Unknown ship"), 
                        (PCC) ConvertColorToString (station->GetSide ()->GetColor ()),
                        station->GetSide()->GetName(), 
                        station->GetName(), 
                        station->GetCluster()->GetName()
                        );
                }
                else
                    PostText(true, START_COLOR_STRING "%s's %s" END_COLOR_STRING " in %s was destroyed.", 
                        (PCC) ConvertColorToString (station->GetSide()->GetColor ()),
                        station->GetSide()->GetName(), 
                        station->GetName(), 
                        station->GetCluster()->GetName());

                if (station->GetSide() == GetSide() || GetSide()->AlliedSides(GetSide(),station->GetSide())) //AllY Imago/Rock 7/27/09 
                    PlaySoundEffect(station->GetStationType()->GetDestroyedSound());
                else
                    PlaySoundEffect(station->GetStationType()->GetEnemyDestroyedSound());

                station->Terminate();
            }
        }
        break;

        case FM_S_STATIONS_UPDATE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmStation, S, STATIONS_UPDATE, pfm);

            IclusterIGC*    pcluster = GetChatCluster();
            if (pcluster)
            {
                for (int i = int(pfmStation->cbrgStationStates /
                                 sizeof(StationState)) - 1; (i >= 0); i--)
                {
                    StationState*   pss = ((StationState*)(FM_VAR_REF(pfmStation, rgStationStates))) + i;

                    IstationIGC * station = pcluster->GetStation(pss->stationID);
                    if (station)
                    {
                        station->SetFraction(pss->bpHullFraction);
                        station->SetShieldFraction(pss->bpShieldFraction);
                    }
                }
            }
        }
        break;

        case FM_S_PROBES_UPDATE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmProbe, S, PROBES_UPDATE, pfm);

            IclusterIGC*    pcluster = GetCluster();
            if (pcluster)
            {
                for (int i = int(pfmProbe->cbrgProbeStates /
                                 sizeof(ProbeState)) - 1; (i >= 0); i--)
                {
                    ProbeState*   pps = ((ProbeState*)(FM_VAR_REF(pfmProbe, rgProbeStates))) + i;

                    IprobeIGC * probe = pcluster->GetProbe(pps->probeID);
                    if (probe)
                        probe->SetFraction(pps->bpFraction);
                }
            }
        }
        break;

        case FM_S_ASTEROIDS_UPDATE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmAsteroids, S, ASTEROIDS_UPDATE, pfm);

            IclusterIGC*    pcluster = GetCluster();
            if (pcluster)
            {
                for (int i = int(pfmAsteroids->cbrgAsteroidStates /
                                 sizeof(AsteroidState)) - 1; (i >= 0); i--)
                {
                    AsteroidState*   pas = ((AsteroidState*)(FM_VAR_REF(pfmAsteroids, rgAsteroidStates))) + i;

                    IasteroidIGC* asteroid = pcluster->GetAsteroid(pas->asteroidID);
                    if (asteroid)
                    {
                        Orientation o;
                        pas->co.Export(&o);
                        asteroid->SetOrientation(o);
                        asteroid->SetOre(float(pas->ore));

                        asteroid->SetFraction(pas->bpFraction);
                    }
                }
            }
        }
        break;

        case FM_S_STATION_CAPTURE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmStationCapture, S, STATION_CAPTURE, pfm);

            IstationIGC * station = m_pCoreIGC->GetStation(pfmStationCapture->stationID);
            if (station)
            {
                TRef<BucketStatusArray> prgStatus;

                /* NYI: Used to be ... is the new code still valid?
                if ((pfmStationCapture->iSide != GetSide()->GetObjectID()) && 
                    m_mapBucketStatusArray.Find(pfmStationCapture->stationID, prgStatus))
                */

                if ((pfmStationCapture->sidOld == GetSide()->GetObjectID()) && 
                    m_mapBucketStatusArray.Find(pfmStationCapture->stationID, prgStatus))
                {
                    // we lost the station... purge the buckets
                    m_mapBucketStatusArray.Remove(pfmStationCapture->stationID);
                }

                m_pClientEventSource->OnStationCaptured(pfmStationCapture->stationID, pfmStationCapture->sidNew);

                IsideIGC*   psideNew = m_pCoreIGC->GetSide(pfmStationCapture->sidNew);

                PostText(true, START_COLOR_STRING "%s" END_COLOR_STRING " captured " START_COLOR_STRING "%s's %s" END_COLOR_STRING " in %s.", 
                    (PCC) ConvertColorToString (GetCore()->GetShip(pfmStationCapture->shipIDCredit)->GetSide ()->GetColor ()),
                    GetCore()->GetShip(pfmStationCapture->shipIDCredit)->GetName(), 
                    (PCC) ConvertColorToString (station->GetSide()->GetColor ()),
                    station->GetSide()->GetName(), 
                    station->GetName(), 
                    station->GetCluster()->GetName()
                    );

                if (pfmStationCapture->sidOld == GetSideID() || GetSide()->AlliedSides(GetCore()->GetSide(pfmStationCapture->sidOld),GetSide()))  //ALLY Rock/Imago 7/27/09
                    PlaySoundEffect(station->GetStationType()->GetCapturedSound());
                else if (pfmStationCapture->sidNew == GetSideID() || GetSide()->AlliedSides(GetCore()->GetSide(pfmStationCapture->sidNew),GetSide())) //ALLY Rock/Imago 7/27/09
                    PlaySoundEffect(station->GetStationType()->GetEnemyCapturedSound());

                station->SetSide(psideNew);
            }
        }
        break;

        case FM_S_LIGHT_SHIPS_UPDATE:
        {
            if (GetCluster())
            {
                CASTPFM(pfmLight, S, LIGHT_SHIPS_UPDATE, pfm);
                ShipID  myShipID = m_ship->GetObjectID();

                ShipID*                 pitu = (ShipID*)(FM_VAR_REF(pfmLight, rgInactiveTurretUpdates));
                if (pitu)
                {
                    ShipID*  pituMax = (ShipID*)((char*)pitu + pfmLight->cbrgInactiveTurretUpdates);

                    //Process the inactive turret updates ..
                    while (pitu < pituMax)
                    {
                        ShipID  shipID = *(pitu++);
                        if (shipID != myShipID)
                        {
                            IshipIGC*   pship = m_pCoreIGC->GetShip(shipID);

                            //The following checks shouldn't be needed ... but since these messages
                            //can arrive in any order, paranoia isn't bad
                            if (pship && pship->GetParentShip() && pship->GetCluster())
                                pship->SetStateM(0);        //Stop shooting
                        }
                    }
                }

                ServerLightShipUpdate*  plsu = (ServerLightShipUpdate*)(FM_VAR_REF(pfmLight, rgLightShipUpdates));
                if (plsu)
                {
                    ServerLightShipUpdate*  plsuMax = (ServerLightShipUpdate*)((char*)plsu + pfmLight->cbrgLightShipUpdates);

                    //Process the light ship updates
                    while (plsu < plsuMax)
                    {
                        ServerLightShipUpdate&  lsu = *(plsu++);

                        //Never get updates for yourself
                        assert (lsu.shipID != myShipID);

                        IshipIGC*   pship = m_pCoreIGC->GetShip(lsu.shipID);
                        if (pship && (pship->GetParentShip() == NULL) && pship->GetCluster())
                        {
                            pship->ProcessShipUpdate(lsu);
                        }
                    }
                }
            }
        }
        break;

        case FM_S_HEAVY_SHIPS_UPDATE:
        {
            if (GetCluster())
            {
                CASTPFM(pfmHeavy, S, HEAVY_SHIPS_UPDATE, pfm);

                if (m_pmodelServerTarget && m_pmodelServerTarget->GetCluster() && (pfmHeavy->bpTargetHull.GetChar() != 255))
                {
                    switch(m_pmodelServerTarget->GetObjectType())
                    {
                        case OT_ship:
                        {
                            IshipIGC*   pship = ((IshipIGC*)(ImodelIGC*)m_pmodelServerTarget)->GetSourceShip();

                            pship->SetFraction(pfmHeavy->bpTargetHull);

                            IshieldIGC* pshield = (IshieldIGC*)(pship->GetMountedPart(ET_Shield, 0));
                            if (pshield)
                                pshield->SetFraction(pfmHeavy->bpTargetShield);                            
                        }
                        break;

                        case OT_station:
                        {
                            IstationIGC*   pstation = (IstationIGC*)(ImodelIGC*)m_pmodelServerTarget;

                            pstation->SetFraction(pfmHeavy->bpTargetHull);
                            pstation->SetShieldFraction(pfmHeavy->bpTargetShield);
                        }
                        break;

                        case OT_probe:
                        case OT_asteroid:
                        case OT_missile:
                        {
                            ((IdamageIGC*)(ImodelIGC*)m_pmodelServerTarget)->SetFraction(pfmHeavy->bpTargetHull);
                        }
                    }
                }

                ServerActiveTurretUpdate*   patu = (ServerActiveTurretUpdate*)(FM_VAR_REF(pfmHeavy, rgActiveTurretUpdates));
                if (patu)
                {
                    ServerActiveTurretUpdate*      patuMax = (ServerActiveTurretUpdate*)((char*)patu + pfmHeavy->cbrgActiveTurretUpdates);

                    //Process the active turret updates ..
                    while (patu < patuMax)
                    {
                        ServerActiveTurretUpdate&  atu= *(patu++);
                        assert (atu.shipID != m_ship->GetObjectID());

                        IshipIGC*   pship = m_pCoreIGC->GetShip(atu.shipID);

                        //The following checks shouldn't be needed ... but since these messages
                        //can arrive in any order, paranoia isn't bad
                        if (pship && pship->GetParentShip() && pship->GetCluster())
                        {
                            pship->ProcessShipUpdate(pfmHeavy->timeReference, atu);
                        }
                    }
                }

                ServerHeavyShipUpdate*      phsu = (ServerHeavyShipUpdate*)(FM_VAR_REF(pfmHeavy, rgHeavyShipUpdates));
                if (phsu)
                {
                    ServerHeavyShipUpdate*      phsuMax = (ServerHeavyShipUpdate*)((char*)phsu + pfmHeavy->cbrgHeavyShipUpdates);

                    //Process the heavy ship updates
                    while (phsu < phsuMax)
                    {
                        ServerHeavyShipUpdate&  hsu = *(phsu++);

                        //Never get updates for yourself
                        assert (hsu.shipID != m_ship->GetObjectID());

                        IshipIGC*   pship = m_pCoreIGC->GetShip(hsu.shipID);
                        if (pship && (pship->GetParentShip() == NULL) && pship->GetCluster())
                        {
                            pship->ProcessShipUpdate(pfmHeavy->timeReference, pfmHeavy->positionReference, hsu);
                        }
                    }
                }

                m_ship->GetSourceShip()->ProcessFractions(pfmHeavy->fractions);
            }
        }
        break;

        case FM_S_PROMOTE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmPromote, S, PROMOTE, pfm);

            IshipIGC*   pship = m_pCoreIGC->GetShip(pfmPromote->shipidPromoted);
            assert (pship);

            IshipIGC*   pshipParent = pship->GetParentShip();
            if (pshipParent)
            {
                pship->Promote();

                m_pClientEventSource->OnBoardShip(pship, NULL);
                m_pClientEventSource->OnBoardShip(pshipParent, pship);
                
                if (pshipParent == m_ship)
                    PostText(true, "You have been demoted to a turret gunner");
                else if (pship == m_ship)
                    PostText(true, "You have been promoted to pilot");
            }
        }
        break;

        case FM_S_WARP_BOMB:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmWarpBomb, S, WARP_BOMB, pfm);
            IwarpIGC*   pwarp = m_pCoreIGC->GetWarp(pfmWarpBomb->warpidBombed);
            if (pwarp)
            {
                ImissileTypeIGC*    pmt = (ImissileTypeIGC*)(m_pCoreIGC->GetExpendableType(pfmWarpBomb->expendableidMissile));
                assert (pmt);
                assert (pmt->GetObjectType() == OT_missileType);

                pwarp->AddBomb(ClientTimeFromServerTime(pfmWarpBomb->timeExplosion), pmt);

                IclusterIGC*    pclusterMe = GetCluster();
                IclusterIGC*    pc1 = pwarp->GetCluster();
                IclusterIGC*    pc2 = pwarp->GetDestination()->GetCluster();
                if ((pclusterMe == pc1) || (pclusterMe == pc2))
                    PostText(true, "Aleph to %s destabilized\n", (pclusterMe == pc1) ? pc2->GetName() : pc1->GetName());

                /*
                DamageTypeID    dtid = pmt->GetDamageType();
                float           p    = pmt->GetPower();
                float           r    = pmt->GetBlastRadius();

                IclusterIGC*    pcluster = pwarp->GetCluster();
                pcluster->CreateExplosion(dtid,
                                          p,
                                          r,
                                          c_etBigShip,
                                          pcluster->GetLastUpdate(),
                                          pwarp->GetPosition(),
                                          NULL);

                pwarp = pwarp->GetDestination();
                pcluster = pwarp->GetCluster();
                pcluster->CreateExplosion(dtid,
                                          p,
                                          r,
                                          c_etBigShip,
                                          pcluster->GetLastUpdate(),
                                          pwarp->GetPosition(),
                                          NULL);
                */
            }
        }
        break;

        case FM_S_RIPCORD_ACTIVATE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmRipcordActivate, S, RIPCORD_ACTIVATE, pfm);

            IshipIGC*   pshipSource = m_ship->GetSourceShip();
            if (pfmRipcordActivate->shipidRipcord == pshipSource->GetObjectID())
            {
                //We are ripcording
                ImodelIGC*      pmodelRipcord = m_pCoreIGC->GetModel(pfmRipcordActivate->otRipcord,
                                                                     pfmRipcordActivate->oidRipcord);

                IclusterIGC*    pclusterDesired = m_pCoreIGC->GetCluster(pfmRipcordActivate->sidRipcord);
                assert (pclusterDesired);

                assert (pmodelRipcord);
                IclusterIGC*    pclusterRipcord = pmodelRipcord->GetCluster();
                if (pclusterRipcord == NULL)
                {
                    assert (pmodelRipcord->GetObjectType() == OT_ship);

                    PlayerInfo* ppi = (PlayerInfo*)(((IshipIGC*)pmodelRipcord)->GetPrivateData());
                    			
					assert (ppi->StatusIsCurrent());
					pclusterRipcord = m_pCoreIGC->GetCluster(ppi->LastSeenSector());
   	                assert (pclusterRipcord); 
                }

                const char*     name = pclusterRipcord->GetName();

                char    bfr[100];
                if (pclusterRipcord != pclusterDesired)
                    sprintf(bfr, "Ripcording to %s, which is closest to %s",
                            name, pclusterDesired->GetName());
                else
                    sprintf(bfr, "Ripcording to %s", name);

                PostText(true, bfr);

                if (pmodelRipcord != pshipSource->GetRipcordModel())
                {
                    pshipSource->SetRipcordModel(pmodelRipcord);
                    pshipSource->ResetRipcordTimeLeft();
                }

                // set up the ripcord effect
                pshipSource->GetThingSite ()->SetTimeUntilRipcord (pshipSource->GetRipcordTimeLeft ());
            }
            else
            {
                //Someone else is ripcording ... just set the model cause we
                //really don't care
                IshipIGC*   pship = m_pCoreIGC->GetShip(pfmRipcordActivate->shipidRipcord);

                //We need a valid model that will stick around at least as long as the
                //ship. Hmmm ... lets see.
                pship->SetRipcordModel(pship);

                // set up the ripcord effect
                pship->ResetRipcordTimeLeft ();
                pship->GetThingSite ()->SetTimeUntilRipcord (pship->GetRipcordTimeLeft ());
            }
        }
        break;
        case FM_S_RIPCORD_DENIED:
        {
            if (!IsInGame())
                break;

            PlayNotificationSound(salNoRipcordSound, m_ship);
        }
        break;
        case FM_S_RIPCORD_ABORTED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmRipcordAborted, S, RIPCORD_ABORTED, pfm);
            IshipIGC*   pship = m_pCoreIGC->GetShip(pfmRipcordAborted->shipidRipcord);
            assert (pship);
            pship->SetRipcordModel(NULL);

            if (pship == m_ship->GetSourceShip())
                PlayNotificationSound(salRipcordAbortedSound, pship);

            // clear the ripcord effect
            pship->GetThingSite ()->SetTimeUntilRipcord (-1.0f);
        }
        break;

        case FM_S_SET_CLUSTER:
        {
            CASTPFM(pfmSetCluster, S, SET_CLUSTER, pfm);

            if (IsLockedDown())
                EndLockDown(lockdownLoadout | lockdownTeleporting);

            // cancel any pending disembarks (it's now a bad idea to step out of the airlock)
            m_bLaunchAfterDisembark = false;
            m_sidBoardAfterDisembark = NA;
            m_sidTeleportAfterDisembark = NA;

            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmSetCluster->sectorID);
            assert (pcluster);

            //The ship update is either for us or for our parent ship
            IshipIGC*   pship = m_ship->GetSourceShip();

            //Pretend the server sends a ship delete message for everything the player could see
            {
                //We could use the old cluster ... but modifying the contents of a list
                const ShipListIGC*  ships = m_pCoreIGC->GetShips();
                assert (ships);

                for (ShipLinkIGC*   l = ships->first();
                     (l != NULL);
                     l = l->next())
                {
                    IshipIGC*  s = l->data();
                    if ((s != pship) && (s != m_ship))
                        s->SetCluster(NULL);
                }
            }

            {
                //Reset the ship's trail
                assert (pship->GetThingSite());
                pship->GetThingSite()->SetTrailColor(GetShip()->GetSide()->GetColor());

                IclusterIGC*    pclusterOld;
                Orientation     orientationOld;
                if (pship != m_ship)
                {
                    pclusterOld = NULL;
                    orientationOld = pship->GetOrientation();
                }

                Time    time = pcluster->GetLastUpdate();
                pship->SetLastUpdate(time);

                pfmSetCluster->shipupdate.time = ClientTimeFromServerTime(pfmSetCluster->shipupdate.time);
                pship->ProcessShipUpdate(pfmSetCluster->shipupdate);

                if (m_ship->GetStation() != NULL)
                {
                    assert (m_ship->GetCluster() == NULL);
					m_ship->SetLastTimeLaunched(Time::Now());
                    m_ship->SetStation(NULL);           //This will call IIgcSite::ChangeStation()
                }
                else
                {
                    pclusterOld = m_ship->GetCluster();
                    if (pclusterOld != NULL)
                    {
                        //Hack ... play the sound effect for an aleph since the player
                        //did not undock.
                        PlaySoundEffect(jumpSound);
                    }
                }
                assert (m_ship->GetStation() == NULL);


                if (pship == m_ship)
                {
                    //Set the cookie
                    SetCookie(pfmSetCluster->cookie);
                }
                else
                {
                    //Adjust our orientation
                    Mount   tid = m_ship->GetTurretID();
                    if (tid != NA)
                    {
                        if (pclusterOld == NULL)
                        {
                            Orientation oTurret = pship->GetHullType()->GetWeaponOrientation(tid) *
                                                  pship->GetOrientation();
                            m_ship->SetOrientation(oTurret);
                        }
                        else
                        {
                            //Preserve the old orientation relative to the ship
                            m_ship->SetOrientation(pship->GetOrientation().TimesInverse(orientationOld) * m_ship->GetOrientation());
                        }
                    }
                }
                pship->SetCluster(pcluster);
            }
        }
        break;

        case FM_S_DOCKED:
        {
            CASTPFM(pfmDocked, S, DOCKED, pfm);

            //Pretend the server sends a ship delete message for everything the player could see
            {
                //We could use the old cluster ... but modifying the contents of a list
                const ShipListIGC*  ships = m_pCoreIGC->GetShips();
                assert (ships);

                for (ShipLinkIGC*   l = ships->first();
                     (l != NULL);
                     l = l->next())
                {
                    IshipIGC*  s = l->data();
                    if (s != m_ship)
                        s->SetCluster(NULL);
                }
            }
			m_ship->SetLastTimeDocked(Time::Now());
            m_ship->SetStation(m_pCoreIGC->GetStation(pfmDocked->stationID));
            assert (m_ship->GetCluster() == NULL);
        }
        break;

        case FM_S_SINGLE_SHIP_UPDATE:
        {
            if (IsWaitingForGameRestart())
                break;

            IclusterIGC*    pcluster = GetCluster();
            if (pcluster)
            {
                CASTPFM(pfmSSU, S, SINGLE_SHIP_UPDATE, pfm);

                //Ignore single ship updates for ourself
                if (m_ship->GetObjectID() != pfmSSU->shipupdate.shipID)
                {
                    //Ignore single shup updates for our parent (this data comes via SetCluster).
                    IshipIGC*   pshipParent = m_ship->GetParentShip();
                    if ((pshipParent == NULL) ||
                        (pshipParent->GetObjectID() != pfmSSU->shipupdate.shipID))
                    {
                        IshipIGC*   ship = m_pCoreIGC->GetShip(pfmSSU->shipupdate.shipID);
                        assert (ship);
                        assert (ship->GetBaseHullType());

                        //Never get single ship updates for passengers
                        assert (ship->GetParentShip() == NULL);

                        //Reset the ship's trail
                        assert (ship->GetThingSite());
                        ship->GetThingSite()->SetTrailColor(ship->GetSide()->GetColor());

                        //Force the updates to be processed, even if it is out of sync with the local time
                        Time    time = pcluster->GetLastUpdate();
                        ship->SetLastUpdate(time);

                        pfmSSU->shipupdate.time = ClientTimeFromServerTime(pfmSSU->shipupdate.time);
                        ship->ProcessShipUpdate(pfmSSU->shipupdate);

                        ship->SetRipcordModel(pfmSSU->bIsRipcording ? ship : NULL); //Just has to be a valid pointer

                        {
                            ImodelIGC*  pmodel = m_pCoreIGC->GetModel(pfmSSU->otTarget, pfmSSU->oidTarget);
                            ship->SetCommand(c_cmdCurrent, pmodel, c_cidNone);
                        }

                        if (ship->GetCluster() == NULL)
                        {
                            // The ship has moved to the same cluster as the player
                            debugf("Moving %s/%d to %s\n",
                                ship->GetName(), ship->GetObjectID(),
                                pcluster->GetName());

                            ship->SetCluster(pcluster);
                            ship->SetLastUpdate(time);
                        }
                        else
                            assert (ship->GetCluster() == pcluster);
                    }
                }
            }
        }
        break;

        case FM_S_SHIP_RESET:
        {
            CASTPFM(pfmShipReset, S, SHIP_RESET, pfm);

            if (m_ship->GetCluster() && (m_ship->GetParentShip() == NULL))
            {
                pfmShipReset->shipupdate.time = ClientTimeFromServerTime(pfmShipReset->shipupdate.time);
                m_ship->ProcessShipUpdate(pfmShipReset->shipupdate, false);
                SetCookie(pfmShipReset->cookie);
            }
        }
        break;

        case FM_S_AUTODONATE:
        {
            CASTPFM(pfmAutoDonate, S, AUTODONATE, pfm);

            IshipIGC*   pshipBy = m_ship->GetSide()->GetShip(pfmAutoDonate->sidDonateBy);
            assert (pshipBy);
            IshipIGC*   pshipTo = pfmAutoDonate->sidDonateTo == NA
                                  ? NULL
                                  : m_ship->GetSide()->GetShip(pfmAutoDonate->sidDonateTo);

            IshipIGC*   pshipOld = pshipBy->GetAutoDonate();
            pshipBy->SetAutoDonate(pshipTo);

            if ((m_pCoreIGC->GetMissionStage() == STAGE_STARTED) && (pshipOld != pshipTo))
            {
                if (pshipBy == GetShip())
                {
                    if (pshipTo)
                    {
                        char    bfr[100];
                        sprintf(bfr, "You have started donating your income to %s", pshipTo->GetName());
                        PostPlainText(true, bfr);
                    }
                    else
                    {
                        PostText(true, "You have stopped donating your income to %s", pshipOld->GetName());
                    }
                }
                else if (pshipTo == GetShip())
                {
                    char    bfr[100];
                    sprintf(bfr, "%s is now donating to you", pshipBy->GetName());

                    PostPlainText(true, bfr);

                    // count donors, and play the designated investor sound if we've gone from 0 to 1
                    int nNumDonors = 0;
                    for (ShipLinkIGC*   psl = GetSide()->GetShips()->first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        if (psl->data()->GetAutoDonate() == GetShip())
                            nNumDonors++;
                    }

                    if (nNumDonors == 1)
                        PlaySoundEffect(investorSound);
                }
                else if (pshipOld == GetShip())
                {
                    char    bfr[100];
                    sprintf(bfr, "%s has stopped donating to you", pshipBy->GetName());
                    PostPlainText(true, bfr);
                }
            }

            if (pfmAutoDonate->amount != 0)
            {
                if (pshipTo)
                {
                    //An autodonation to another player ... increase the target's money
                    PlayerInfo* ppiTo = (PlayerInfo*)(pshipTo->GetPrivateData());
                    ppiTo->SetMoney(ppiTo->GetMoney() + pfmAutoDonate->amount);
                    m_pClientEventSource->OnMoneyChange(ppiTo);

                    if (pshipBy != m_ship) // KGJV mutiny-note: dont change this without changing MutinyBallot::OnPassed server side
                    {
                        //Someone other than me autodonated ... subtract their money
                        PlayerInfo* ppiBy = (PlayerInfo*)(pshipBy->GetPrivateData());
                        ppiBy->SetMoney(ppiBy->GetMoney() - pfmAutoDonate->amount);
                        m_pClientEventSource->OnMoneyChange(ppiBy);
                    }
                }
                else if (pshipBy == m_ship)
                {
                    //We made a request and it was denied ... get a refund.
                    SetMoney(GetMoney() + pfmAutoDonate->amount);
                    m_pClientEventSource->OnMoneyChange(m_pPlayerInfo);
                }
            }

            SideInfo* psideinfo = m_pMissionInfo->GetSideInfo(GetSideID());
            if (psideinfo)
            {
                psideinfo->GetMembers().GetSink()();
            }
        }
        break;

        case FM_S_PAYDAY:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmPayday, S, PAYDAY, pfm);

            Money   moneyReceived = 0;

            for (ShipLinkIGC*   psl = GetSide()->GetShips()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IshipIGC*   pship = psl->data();
                if (pship->GetPilotType() >= c_ptPlayer)
                {
                    IshipIGC*   pshipDonate = pship->GetAutoDonate();
                    if (pshipDonate)
                    {
                        if (pshipDonate == m_ship)
                            moneyReceived += pfmPayday->dMoney;
                    }
                    else
                        pshipDonate = pship;

                    PlayerInfo* ppi = (PlayerInfo*)(pshipDonate->GetPrivateData());
                    ppi->SetMoney(ppi->GetMoney() + pfmPayday->dMoney);
                    m_pClientEventSource->OnMoneyChange(ppi);
                }
            }
            m_pMissionInfo->GetSideInfo(GetSide()->GetObjectID())->GetMembers().GetSink()();

            PlaySoundEffect(paydaySound);
            {
                IshipIGC*   pshipDonate = m_ship->GetAutoDonate();
                if (pshipDonate)
                    PostText(false, "You donated your payday of $%d to %s.", pfmPayday->dMoney, pshipDonate->GetName());
                else if (moneyReceived == 0)
                    PostText(false, "You received a payday of $%d.", pfmPayday->dMoney);
                else
                    PostText(false, "You received $%d in pay and donations.", pfmPayday->dMoney + moneyReceived);
            }
        }
        break;

        case FM_S_MONEY_CHANGE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmMoney, S, MONEY_CHANGE, pfm);

            ShipID  sid = GetShipID();
            if (sid != pfmMoney->sidFrom)
			{
                IshipIGC*  pshipTo   = m_pCoreIGC->GetShip(pfmMoney->sidTo);
                assert (pshipTo);

                {
                    PlayerInfo* ppiTo = (PlayerInfo*)(pshipTo->GetPrivateData());
                    ppiTo->SetMoney(ppiTo->GetMoney() + pfmMoney->dMoney);
                    m_pClientEventSource->OnMoneyChange(ppiTo);
                }

                if ((pfmMoney->sidTo != pfmMoney->sidFrom) && (pfmMoney->sidFrom != NA))
                {
                    //This was a donation from one player to another
                    IshipIGC*  pshipFrom   = m_pCoreIGC->GetShip(pfmMoney->sidFrom);
                    assert (pshipFrom);

                    {
                        PlayerInfo* ppiFrom = (PlayerInfo*)(pshipFrom->GetPrivateData());
                        ppiFrom->SetMoney(ppiFrom->GetMoney() - pfmMoney->dMoney);
                        m_pClientEventSource->OnMoneyChange(ppiFrom);
						
						if (pfmMoney->sidTo == sid) {
                            PostText(false, "%s gave you $%d. You now have $%d.",
                                     ppiFrom->CharacterName(), pfmMoney->dMoney, MyPlayerInfo()->GetMoney());
							
						}
                    }
                }
				m_pMissionInfo->GetSideInfo(GetSide()->GetObjectID())->GetMembers().GetSink()();
			}
        }
        break;
        case FM_S_SET_MONEY:
        {
            CASTPFM(pfmMoney, S, SET_MONEY, pfm);

            IshipIGC*  pship   = m_pCoreIGC->GetShip(pfmMoney->shipID);
            assert (pship);

            {
                PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());

                ppi->SetMoney(pfmMoney->money);
                m_pClientEventSource->OnMoneyChange(ppi);
            }

            m_pMissionInfo->GetSideInfo(GetSide()->GetObjectID())->GetMembers().GetSink()();
        }
        break;
        case FM_S_ADD_PART:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmAddPart, S, ADD_PART, pfm);
            if (pfmAddPart->shipID != m_ship->GetObjectID())
            {
                IshipIGC*   pShip = m_pCoreIGC->GetShip(pfmAddPart->shipID);
                assert (pShip);
                assert (pShip->GetParentShip() == NULL);
                assert (pShip->GetBaseHullType());

                IpartTypeIGC* ppt = m_pCoreIGC->GetPartType(pfmAddPart->newPartData.partID);
                assert (ppt);

                IpartIGC* ppart = pShip->GetMountedPart(ppt->GetEquipmentType(), pfmAddPart->newPartData.mountID);
                if (ppart)
                {
                    assert (ppart->GetPartType() == ppt);
                    ppart->SetAmount(ppart->GetAmount() + pfmAddPart->newPartData.amount);
                }
                else
                    pShip->CreateAndAddPart(ppt, pfmAddPart->newPartData.mountID, pfmAddPart->newPartData.amount);
            }
        }
        break;
        case FM_CS_DROP_PART:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmDropPart, CS, DROP_PART, pfm);
            if (pfmDropPart->shipID != m_ship->GetObjectID())
            {
                IshipIGC*   pShip = m_pCoreIGC->GetShip(pfmDropPart->shipID);
                assert (pShip);
                assert (pShip->GetParentShip() == NULL);
                assert (pShip->GetBaseHullType());

                IpartIGC*   ppart = pShip->GetMountedPart(pfmDropPart->et, pfmDropPart->mount);
                assert (ppart);
                ppart->Terminate();
            }
        }
        break;
        case FM_CS_SWAP_PART:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmSwapPart, CS, SWAP_PART, pfm);
            if (pfmSwapPart->shipID != m_ship->GetObjectID())
            {
                IshipIGC*   pShip = m_pCoreIGC->GetShip(pfmSwapPart->shipID);
                assert (pShip);
                assert (pShip->GetParentShip() == NULL);
                assert (pShip->GetBaseHullType());
                assert (pfmSwapPart->mountNew >= -c_maxCargo);

                IpartIGC*   ppart = pShip->GetMountedPart(pfmSwapPart->etOld, pfmSwapPart->mountOld);
                assert (ppart);

                IpartIGC*   ppartNew = pShip->GetMountedPart(pfmSwapPart->etOld, pfmSwapPart->mountNew);
                if (ppartNew)
                {
                    ppart->SetMountID(c_mountNA);
                    ppartNew->SetMountID(pfmSwapPart->mountOld);
                }

                ppart->SetMountID(pfmSwapPart->mountNew);
            }
        }
        break;
        case FM_CS_RELOAD:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmReload, CS, RELOAD, pfm);

            if (pfmReload->shipID != m_ship->GetObjectID())
            {
                IshipIGC*   pship = m_pCoreIGC->GetShip(pfmReload->shipID);
                assert (pship);

                ReloadData*   prlNext = (ReloadData*)FM_VAR_REF(pfmReload, rgReloads);
                int           nReloads = pfmReload->cbrgReloads / sizeof(ReloadData);
                ReloadData*   prlStop = prlNext + nReloads;

                while (prlNext < prlStop)
                {
                    assert (prlNext->mount < 0);
                    assert (prlNext->mount >= -c_maxCargo);
                    IpartIGC*     ppart = pship->GetMountedPart(NA, prlNext->mount);
                    ObjectType    type = ppart->GetObjectType();

                    if (type == OT_pack)
                    {
                        IpackIGC* ppack = (IpackIGC*)ppart;
                        PackType  packtype = ppack->GetPackType();
                        short     amount = ppack->GetAmount();

                        if (prlNext->amountTransfered == NA)
                        {
                            OnReload(ppart, true);
                            ppack->Terminate();
                        }
                        else
                        {
                            OnReload(ppart, false);
                            assert (prlNext->amountTransfered > 0);
                            assert (prlNext->amountTransfered < amount);
                            ppack->SetAmount(amount - prlNext->amountTransfered);
                            amount = prlNext->amountTransfered;
                        }

                        if (packtype == c_packAmmo)
                        {
                            pship->SetAmmo(pship->GetAmmo() + amount);

                            //disable all mounted weapons that use ammo
                            Mount   maxWeapons = pship->GetHullType()->GetMaxWeapons();
                            for (Mount i = 0; (i < maxWeapons); i++)
                            {
                                IweaponIGC* pw = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
                                if (pw && (pw->GetAmmoPerShot() != 0))
                                    pw->SetMountedFraction(0.0f);
                            }
                        }
                        else
                        {
                            assert (packtype == c_packFuel);
                            pship->SetFuel(pship->GetFuel() + float(amount));

                            IpartIGC* pa = pship->GetMountedPart(ET_Afterburner, 0);
                            if (pa)
                                pa->SetMountedFraction(0.0f);
                        }
                    }
                    else
                    {
                        assert (IlauncherIGC::IsLauncher(type));

                        IlauncherIGC* plauncher = (IlauncherIGC*)ppart;
                        IlauncherIGC* plauncherMounted = (IlauncherIGC*)(pship->GetMountedPart(plauncher->GetEquipmentType(), 0));

                        if (prlNext->amountTransfered == NA)
                        {
                            OnReload(ppart, true);
                            assert ((plauncherMounted == NULL) || (plauncherMounted->GetAmount() == 0));
                            if (plauncherMounted)
                                plauncherMounted->Terminate();

                            plauncher->SetMountID(0);
                        }
                        else
                        {
                            assert (prlNext->amountTransfered <= plauncher->GetAmount());
                            assert (plauncherMounted);

                            short amount = plauncher->GetAmount();
                            if (amount == prlNext->amountTransfered)
                            {
                                OnReload(ppart, true);
                                plauncher->Terminate();
                            }
                            else
                            {
                                OnReload(ppart, false);
                                plauncher->SetAmount(amount - prlNext->amountTransfered);
                            }

                            plauncherMounted->SetAmount(plauncherMounted->GetAmount() + prlNext->amountTransfered);
                            plauncherMounted->SetMountedFraction(0.0f);
                            plauncherMounted->ResetTimeLoaded();
                        }
                    }

                    prlNext++;
                }
            }
        }
        break;
        case FM_CS_FIRE_MISSILE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmFireMissile, CS, FIRE_MISSILE, pfm);

            DataMissileIGC  dm;
            dm.pLauncher = m_pCoreIGC->GetShip(pfmFireMissile->launcherID);
            if (dm.pLauncher)
            {
                dm.pmissiletype = (ImissileTypeIGC*)(m_pCoreIGC->GetExpendableType(pfmFireMissile->missiletypeID));
                dm.pTarget = m_pCoreIGC->GetModel(pfmFireMissile->targetType, pfmFireMissile->targetID);
                dm.pCluster = m_pCoreIGC->GetCluster(pfmFireMissile->clusterID);
                dm.lock = pfmFireMissile->lock;

                Time                timeFired = ClientTimeFromServerTime(pfmFireMissile->timeFired);
                int                 iNumMissiles = pfmFireMissile->cbmissileLaunchData / sizeof(MissileLaunchData);
                MissileLaunchData*  pMissileLaunchData = (MissileLaunchData*) (FM_VAR_REF(pfmFireMissile, missileLaunchData));

                for (int i = 0; i < iNumMissiles; i++)
                {
                    dm.position     = pMissileLaunchData[i].vecPosition;
                    dm.velocity     = pMissileLaunchData[i].vecVelocity;
                    dm.forward      = pMissileLaunchData[i].vecForward;
                    dm.missileID    = pMissileLaunchData[i].missileID;
                    dm.bDud         = pfmFireMissile->bDud;
                    ImissileIGC*    m = (ImissileIGC*)(m_pCoreIGC->CreateObject(timeFired, OT_missile, &dm, sizeof(dm)));

                    if (m)
                        m->Release();
                }

                //The player immediately adjusts their own count on launching the missile so don't update
                //their count.
                if (pfmFireMissile->launcherID != m_ship->GetObjectID())
                {
                    ImagazineIGC*   pmagazine = (ImagazineIGC*)(dm.pLauncher->GetMountedPart(ET_Magazine, 0));
                    if (pmagazine)
                    {
                        short   amount = pmagazine->GetAmount() - iNumMissiles;
                        assert (amount >= 0);
                        pmagazine->SetAmount(amount);
                    }
                }
            }
        }
        break;

        case FM_S_FIRE_EXPENDABLE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmFireExpendable, S, FIRE_EXPENDABLE, pfm);

            //Ignore messages that you fired a mine since you already decremented the mine count
            if (pfmFireExpendable->launcherID != m_ship->GetObjectID())
            {
                IshipIGC*   pshipLauncher = m_pCoreIGC->GetShip(pfmFireExpendable->launcherID);
                if (pshipLauncher)
                {
                    IdispenserIGC*   pdispenser = (IdispenserIGC*)(pshipLauncher->GetMountedPart(pfmFireExpendable->equipmentType, 0));
                    if (pdispenser)
                    {
                        short   amount = pdispenser->GetAmount() - 1;
                        assert (amount >= 0);
                        pdispenser->SetAmount(amount);
                    }
                }
            }
        }
        break;

        case FM_S_CREATE_CHAFF:
        {
            CASTPFM(pfmChaff, S, CREATE_CHAFF, pfm);

            DataChaffIGC    dc;

            if (GetCluster())
            {
                dc.time0 = pfmChaff->time0;
                dc.p0    = pfmChaff->p0;
                dc.v0    = pfmChaff->v0;
                dc.pchafftype = (IchaffTypeIGC*)(m_pCoreIGC->GetExpendableType(pfmChaff->etid));
                assert (dc.pchafftype->GetObjectType() == OT_chaffType);
                dc.pcluster = GetCluster();

                IchaffIGC*  c = (IchaffIGC*)(m_pCoreIGC->CreateObject(lastUpdate, OT_chaff, &dc, sizeof(dc)));
                assert (c);

                m_pchaffLastCreated = c;

                c->Release();
            }
        }
        break;

        case FM_S_MISSILE_SPOOFED:
        {
            CASTPFM(pfmMissileSpoofed, S, MISSILE_SPOOFED, pfm);

            // need to make sure we have a cluster because of a race condition 
            // when changing view clusters.
            if (GetCluster())
            {
                ImissileIGC*    pmissile = GetCluster()->GetMissile(pfmMissileSpoofed->missileID);
                if (pmissile)
                {
                    assert (m_pchaffLastCreated);
                    pmissile->SetTarget(m_pchaffLastCreated);
                }
            }
        }
        break;


        case FM_S_END_SPOOFING:
        {
            m_pchaffLastCreated = NULL;
        }
        break;

        case FM_S_ASTEROID_DESTROYED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmAD, S, ASTEROID_DESTROYED, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmAD->clusterID);
            assert (pcluster);
            IasteroidIGC*   pasteroid = pcluster->GetAsteroid(pfmAD->asteroidID);
            if (pasteroid)
            {
                if (pfmAD->explodeF)
                    pasteroid->GetCluster()->GetClusterSite()->AddExplosion(pasteroid, c_etAsteroid);
                else
                {
                    //Pre-terminate the building effect so that terminating the asteroid does not terminate the effect
                    //If the server is really lagged (30 seconds or more), the building effect will terminate locally
                    //before we get the message. This isn't good, but we shouldn't crash.
                    if (pasteroid->GetBuildingEffect())
                        pasteroid->SetBuildingEffect(NULL);
                }
                pasteroid->Terminate();
            }
        }
        break;

        case FM_S_ASTEROID_DRAINED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmAD, S, ASTEROID_DRAINED, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmAD->clusterID);
            assert (pcluster);
            IasteroidIGC*   pasteroid = pcluster->GetAsteroid(pfmAD->asteroidID);
            if (pasteroid)
            {
                pasteroid->SetOre(0.0f);
            }
        }
        break;
		case FM_S_ASTEROID_MINED:  //Xynth #132 7/2010
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmAD, S, ASTEROID_MINED, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmAD->clusterID);
            assert (pcluster);
            IasteroidIGC*   pasteroid = pcluster->GetAsteroid(pfmAD->asteroidID);
            if (pasteroid)
            {
				pasteroid->SetOreWithFraction(pfmAD->bpOreFraction, true);  //Xynth #163 7/2010
            }
        }
        break;  //End Xynth #132

		case FM_CS_HIGHLIGHT_CLUSTER:  //Xynth #208 8/2010
		{
			if (!IsInGame())
                break;

			CASTPFM(pfmAD, CS, HIGHLIGHT_CLUSTER, pfm);
			IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmAD->clusterID);
			pcluster->SetHighlight(pfmAD->highlight);
			if (pfmAD->highlight) PlaySoundEffect(newCommandMsgSound); //play sound on activate
		}
		break;

        case FM_S_PROBE_DESTROYED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmPD, S, PROBE_DESTROYED, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmPD->clusterID);
            assert (pcluster);
            IprobeIGC*   pprobe = pcluster->GetProbe(pfmPD->probeID);
            if (pprobe)
                pprobe->Terminate();
        }
        break;

        case FM_S_BUILDINGEFFECT_DESTROYED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmBED, S, BUILDINGEFFECT_DESTROYED, pfm);
            IasteroidIGC*    pasteroid = m_pCoreIGC->GetAsteroid(pfmBED->asteroidID);
            if (pasteroid)
            {
                IbuildingEffectIGC* pbe = pasteroid->GetBuildingEffect();
                if (pbe)
                    pbe->Terminate();
            }
        }
        break;
        case FM_S_MISSILE_DESTROYED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmMD, S, MISSILE_DESTROYED, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmMD->clusterID);
            assert (pcluster);
            ImissileIGC*   pmissile = pcluster->GetMissile(pfmMD->missileID);
            if (pmissile)
            {
                if (pfmMD->position != Vector::GetZero())
                    pmissile->Explode(pfmMD->position);
                pmissile->Terminate();
            }
        }
        break;

        case FM_S_MINE_DESTROYED:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmMD, S, MINE_DESTROYED, pfm);
            IclusterIGC*    pcluster = m_pCoreIGC->GetCluster(pfmMD->clusterID);
            assert (pcluster);
            ImineIGC*   pmine = pcluster->GetMine(pfmMD->mineID);
            if (pmine)
                pmine->Terminate();
        }
        break;

        case FM_CS_ORDER_CHANGE:
        {
            if (!IsInGame())
                break;

            SideInfo* psideinfo = m_pMissionInfo->GetSideInfo(GetSideID());
            if (psideinfo)
            {
                CASTPFM(pfmOC, CS, ORDER_CHANGE, pfm);

                IshipIGC*   pship = m_pCoreIGC->GetShip(pfmOC->shipID);
                if (pship)
                {
                    ImodelIGC*  pmodel = m_pCoreIGC->GetModel(pfmOC->objectType, pfmOC->objectID);

                    if (pship != m_ship) {
                        pship->SetCommand(pfmOC->command, pmodel, pfmOC->commandID);

                        // Post notification for miners
                        if (pfmOC->commandID == c_cidMine) {
                            PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());
                            if (ppi && pmodel && ppi->LastSeenSector() != pmodel->GetCluster()->GetObjectID())
                                PostText(false, "%s: Going to mine %s in %s.", pship->GetName(), pmodel->GetName(), pmodel->GetCluster()->GetName());
                        }
                    }
                    else if (pfmOC->command == c_cmdCurrent)
                        m_pmodelServerTarget = pmodel;

                    psideinfo->GetMembers().GetSink()();
                }
            }
        }
        break;

        case FM_S_TELEPORT_ACK:
        {
            if (!IsInGame())
                break;

            if (IsLockedDown())
                EndLockDown(lockdownTeleporting);

            CASTPFM(pfmAck, S, TELEPORT_ACK, pfm);

            if (pfmAck->stationID != NA)
            {
                IstationIGC*    pstation = m_pCoreIGC->GetStation(pfmAck->stationID);
                assert (pstation);
                m_ship->SetStation(pstation);
                if (pfmAck->bNewHull)
                {
                    //If the ship had children, it doesn't now
                    {
                        const ShipListIGC*  pshipsChildren = m_ship->GetChildShips();

                        ShipLinkIGC*    psl;
                        while (psl = pshipsChildren->first())   //intentional assignment
                        {
                            IshipIGC*   pshipChild = psl->data();
                            pshipChild->SetParentShip(NULL);
                            m_pClientEventSource->OnBoardShip(pshipChild, NULL);
                        }
                    }

                    const PartListIGC*  pparts = m_ship->GetParts();
                    PartLinkIGC*        ppl;
                    while (ppl = pparts->first())       //Intentional
                        ppl->data()->Terminate();

                    ReplaceLoadout(pstation);
                }

                PlaySoundEffect(personalJumpSound);
            }
        }
        break;

        case FM_S_BOARD_NACK:
        {
            if (!IsInGame())
                break;
            
            if (IsLockedDown())
                EndLockDown(lockdownTeleporting);

            CASTPFM(pfmBoardNack, S, BOARD_NACK, pfm);

            IshipIGC* pshipRequestedParent = m_pCoreIGC->GetShip(pfmBoardNack->sidRequestedParent);
            m_pClientEventSource->OnBoardFailed(pshipRequestedParent);
        }
        break;

        case FM_S_LEAVE_SHIP:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmLeave, S, LEAVE_SHIP, pfm);

            IshipIGC*   pshipChild = m_pCoreIGC->GetShip(pfmLeave->sidChild);
            assert (pshipChild);

            pshipChild->SetParentShip(NULL);
            m_pClientEventSource->OnBoardShip(pshipChild, NULL);

            if (pshipChild == m_ship)
            {
                assert (pshipChild->GetBaseHullType() == NULL);
                assert (pshipChild->GetParts()->n() == 0);

                if (IsLockedDown())
                    EndLockDown(lockdownTeleporting);

                pshipChild->SetBaseHullType(pshipChild->GetSide()->GetCivilization()->GetLifepod());

                ReplaceLoadout(m_ship->GetStation(), m_bLaunchAfterDisembark);
                m_bLaunchAfterDisembark = false;
            }
            else if (pshipChild->GetParentShip() == m_ship)
                PostText(true, "%s has left your ship.", pshipChild->GetName());
        }
        break;

        case FM_S_ENTER_LIFEPOD:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmEnter, S, ENTER_LIFEPOD, pfm);

            /*
            IshipIGC*   pship = m_pCoreIGC->GetShip(pfmEnter->shipID);
            assert (pship);
            */

            m_ship->SetParentShip(NULL);
            m_pClientEventSource->OnBoardShip(m_ship, NULL);

            //If the ship had children, it doesn't now
            {
                const ShipListIGC*  pshipsChildren = m_ship->GetChildShips();

                ShipLinkIGC*    psl;
                while (psl = pshipsChildren->first())   //intentional assignment
                {
                    IshipIGC*   pshipChild = psl->data();
                    pshipChild->SetParentShip(NULL);
                    m_pClientEventSource->OnBoardShip(pshipChild, NULL);
                }
            }

            {
                const PartListIGC*  pparts = m_ship->GetParts();
                PartLinkIGC*        ppl;
                while (ppl = pparts->first())       //Intentional
                    ppl->data()->Terminate();
            }

            m_ship->SetBaseHullType(m_ship->GetSide()->GetCivilization()->GetLifepod());
        }
        break;

        case FM_CS_LOGOFF:
        {
            m_fLoggedOn = false;
            Disconnect(); // we could already be calling from Disconnect, if the user Alt-F4ed, but this is handled
            break;
        }
        
        case FM_S_LOADOUT_CHANGE:
        {
            if (IsWaitingForGameRestart())
                break;

            CASTPFM(pfmLC, S, LOADOUT_CHANGE, pfm);

            IshipIGC*   pship = pfmLC->sidShip == NA
                                ? m_ship
                                : m_pCoreIGC->GetShip(pfmLC->sidShip);
            assert (pship);
            //debugf("Loadout change for %s/%d\n", pship->GetName(), pfmLC->sidShip);

		

            //If the ship was a passenger, now it is not
            pship->SetParentShip(NULL);
            m_pClientEventSource->OnBoardShip(pship, NULL);

            //If the ship had children, it doesn't now
            {
                const ShipListIGC*  pshipsChildren = pship->GetChildShips();

                ShipLinkIGC*    psl;
                while (psl = pshipsChildren->first())   //intentional assignment
                {
                    IshipIGC*   pshipChild = psl->data();
                    pshipChild->SetParentShip(NULL);
                    m_pClientEventSource->OnBoardShip(pshipChild, NULL);
                }
            }

            //Ignore most part changes for ourselves  ALLY Imago docking lifepods at allied bases hack 7/13/09 & 12/09
            if (pfmLC->sidShip != GetShipID())
            {
                if (pship->GetBaseHullType() && pship->GetBaseHullType()->HasCapability(c_habmLifepod) && pship->GetStation()) {
                    RestoreLoadout(pship->GetStation());
                } else {
                    pship->ProcessShipLoadout(pfmLC->cbloadout,
                                          (const ShipLoadout*)(FM_VAR_REF(pfmLC, loadout)), (pship->GetCluster() == NULL));
                }
            }

            //But always process the passenger data (even for ourself)
            {
                const PassengerData*  ppassengersLC = (const PassengerData*)(FM_VAR_REF(pfmLC, rgPassengers));
                const PassengerData*  pstop = (const PassengerData*)(((char*)ppassengersLC) + pfmLC->cbrgPassengers);
                for (const PassengerData* ppd = ppassengersLC; (ppd < pstop); ppd++)
                {
                    IshipIGC*   pshipChild = m_pCoreIGC->GetShip(ppd->shipID);
                    assert (pshipChild);

                    if (pshipChild == m_ship)
                    {
                        SetAutoPilot(false);

                        if ((ppd->turretID != NA) && (m_ship->GetCluster() != NULL) && (pshipChild->GetTurretID() != ppd->turretID))
                        {
                            Orientation oTurret = pship->GetHullType()->GetWeaponOrientation(ppd->turretID) *
                                                  pship->GetOrientation();
                            pshipChild->SetOrientation(oTurret);
                        }
                    }
                    else
                    {
                        assert (pshipChild->GetAutopilot() == false);
                    }

                    pshipChild->SetParentShip(pship);

                    assert (pshipChild->GetBaseHullType() == NULL);
                    assert (pshipChild->GetParts()->n() == 0);

                    if (pship == m_ship && m_ship->GetCluster() == NULL)
                    {
                        if (ppd->turretID != NA)
                            PostText(true, "%s has boarded your ship as a turret.", pshipChild->GetName());
                        else
                            PostText(true, "%s has boarded your ship as an observer.", pshipChild->GetName());
                        
                        PlaySoundEffect(boardSound);
                    }
                    pshipChild->SetTurretID(ppd->turretID);
                    m_pClientEventSource->OnBoardShip(pshipChild, pship);

                    if (pshipChild == m_ship && IsLockedDown())
                        EndLockDown(lockdownTeleporting);
                }
            }

            //We were just out of a ship while at a station
            assert ((m_ship->GetParentShip() != NULL) || (m_ship->GetBaseHullType() != NULL));
        }
        break;

        /* NYI
        case FM_S_ACTIVE_TURRET_UPDATE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmATU, S, ACTIVE_TURRET_UPDATE, pfm);

            if (m_ship->GetObjectID() != pfmATU->shipID)
            {
                Time    time = ClientTimeFromServerTime(pfmATU->timeUpdate);

                IshipIGC*   ship = m_pCoreIGC->GetShip(pfmATU->shipID);
                assert (ship);
                if (ship->GetTurretID() != NA)
                {
                    assert (ship->GetParentShip());

                    ship->ProcessShipUpdate(time, pfmATU->atu);
                }
            }
        }
        break;

        case FM_S_INACTIVE_TURRET_UPDATE:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmITU, S, INACTIVE_TURRET_UPDATE, pfm);

            if (m_ship->GetObjectID() != pfmITU->shipID)
            {
                IshipIGC*   ship = m_pCoreIGC->GetShip(pfmITU->shipID);
                assert (ship);
                assert (ship->GetParentShip());

                ship->SetStateM(lastUpdate, 0);
            }
        }
        break;
        */

        case FM_CS_SET_WINGID:
        {
            CASTPFM(pfmSW, CS, SET_WINGID, pfm);

            IshipIGC*   pship = m_ship->GetSide()->GetShip(pfmSW->shipID);
            //assert (pship);
			if (!pship) {
				break; //Imago 6/10
			}

            if ((pship != m_ship) || pfmSW->bCommanded)
            {
                pship->SetWingID(pfmSW->wingID);
                if (pfmSW->bCommanded && (pship == m_ship))
                    PostText(true, "You have been assigned to wing %s", c_pszWingName[pfmSW->wingID]);
            }

            m_pMissionInfo->GetSideInfo(pship->GetSide()->GetObjectID())->GetMembers().GetSink()();
        }
        break;

        case FM_S_JOINED_MISSION:
        {
            CASTPFM(pfmJoinedMission, S, JOINED_MISSION, pfm);

            assert(m_pMissionInfo != NULL);

            // make sure this was what we intended to join if we were joining from the lobby
            assert(m_dwCookieToJoin == pfmJoinedMission->dwCookie || !m_fmLobby.IsConnected());

            debugf("I am ship %d\n", pfmJoinedMission->shipID);

            // Make sure we have the right static core
            if (lstrcmp(m_pMissionInfo->GetIGCStaticFile(), m_szIGCStaticFile) != 0
                && !ResetStaticData(m_pMissionInfo->GetIGCStaticFile(), &m_pCoreIGC, now, GetIsZoneClub()))
            {
                // we have the wrong file - let's bail.
                Disconnect();
                
                // not logged on yet, so we have to do this part manually
                delete m_pMissionInfo;
                m_pMissionInfo = NULL;

#ifdef STEAMSECURE
				// BT - STEAM - If the IGC file is invalid, trigger a reverify of the user's content.
				SteamApps()->MarkContentCorrupt(false);
#endif

				// BT - STEAM
                OnLogonAck(false, false, "The client and server core files are out of sync. Requesting a file reverification from Steam. Please check the Steam app "
				"for the download status on any updates, and try re-launching the game in a few minutes. If the problem persists, please un-install and re-install again, "
				"and feel free to reach out to the help line on Discord or in the forums.");
            }
            else
            {
                // update myself
                PlayerInfo* pPlayerInfo = m_pPlayerInfo;

                if (!pPlayerInfo)
                {
                    PlayerLink* l = new PlayerLink;
                    m_listPlayers.last(l);
                    pPlayerInfo = &(l->data());
                }
                pPlayerInfo->SetMission(m_pCoreIGC);

                FMD_S_PLAYERINFO fmPlayerInfo;
                memset(&fmPlayerInfo, 0, sizeof(fmPlayerInfo));
                strcpy(fmPlayerInfo.CharacterName, "<error>");
                fmPlayerInfo.shipID = pfmJoinedMission->shipID;
                fmPlayerInfo.iSide = NA;
                pPlayerInfo->Set(&fmPlayerInfo);
                SetPlayerInfo(pPlayerInfo);

                assert (m_pPlayerInfo == pPlayerInfo);
                pPlayerInfo->SetShip(m_ship);

                ZAssert(m_pPlayerInfo);
                if (m_pPlayerInfo->ShipID() != pfmJoinedMission->shipID)
                    SetPlayerInfo(FindPlayer(pfmJoinedMission->shipID));

                // we will be sent the list of players right after this message
                m_pMissionInfo->PurgePlayers();
            }

            break;
        }

        case FM_S_PLAYERINFO:
        {
            if (NULL == m_pCoreIGC->GetSide(SIDE_TEAMLOBBY))
            {
                assert(false);
                OnSessionLost("Unexpected message received.", &m_fm);
            }

            CASTPFM(pfmPlayerInfo, S, PLAYERINFO, pfm);
            PlayerLink* l = FindPlayerLink(pfmPlayerInfo->shipID);
            debugf("Got Player Info for %s\n", pfmPlayerInfo->CharacterName);
            // start with the player not on a side
            SideID sideID = pfmPlayerInfo->iSide;
            bool bReady = pfmPlayerInfo->fReady;

            pfmPlayerInfo->iSide = NA;

            if (l != NULL)
                l->data().Set(pfmPlayerInfo);
            else
                {
                l = new PlayerLink;
                l->data().Set(pfmPlayerInfo);
                m_listPlayers.last(l);
                }
            ZAssert(l);
            
            PlayerInfo* pPlayerInfo = &(l->data());
            pPlayerInfo->SetMission(m_pCoreIGC);

            {
                DataShipIGC ds;
                ds.hullID = NA;
                ds.shipID = pfmPlayerInfo->shipID;
                ds.nParts = 0;
                ds.sideID = NA;
                //ds.wingID = pfmPlayerInfo->wingID;
                ds.nDeaths = pfmPlayerInfo->nDeaths;
                ds.nEjections = pfmPlayerInfo->nEjections;
                ds.nKills = pfmPlayerInfo->nKills;

                ds.pilotType = pfmPlayerInfo->pilotType;
                ds.abmOrders = pfmPlayerInfo->abmOrders;
                ds.baseObjectID = pfmPlayerInfo->baseObjectID;

                strcpy(ds.name, pfmPlayerInfo->CharacterName);

                IshipIGC*   pship = (IshipIGC*)(m_pCoreIGC->CreateObject(m_lastSend, OT_ship, &ds, sizeof(ds)));
                if (pship != NULL)
                {
                    //Slightly bad form: releasing pship here and releasing it below but ships
                    //are never deleted on their first release (since they added to the side and
                    //mission lists)
                    pship->Release();
                }
                else
                {
                    //The ship must already exist ... get it for the player info
                    //NYI does this happen for anything other than the player's ship?
                    pship = m_pCoreIGC->GetShip(pfmPlayerInfo->shipID);
                    assert (pship);
                }
                pPlayerInfo->SetShip(pship);
                pship->SetExperience(pfmPlayerInfo->fExperience);
            }
            
            // add this player to their team
            AddPlayerToMission(pPlayerInfo);

            if (sideID != SIDE_TEAMLOBBY)
            {
                AddPlayerToSide(pPlayerInfo, sideID);
                // pPlayerInfo->SetReady(bReady); Imago commented out so afk not reset
            }

            debugf("PlayerInfo for %s, ship=%d, side=%d\n", pPlayerInfo->CharacterName(),
                pPlayerInfo->ShipID(), pPlayerInfo->SideID());
            break;
        }

        case FM_CS_PLAYER_READY:
        {
            ZAssert(m_pMissionInfo);
            CASTPFM(pfmPlayerReady, CS, PLAYER_READY, pfm);
            PlayerInfo* pPlayerInfo = FindPlayer(pfmPlayerReady->shipID);
            if (pPlayerInfo)
                {
                pPlayerInfo->Update(pfmPlayerReady);
                if (pPlayerInfo->IsHuman())
                    m_pClientEventSource->OnPlayerStatusChange(m_pMissionInfo, pPlayerInfo->SideID(), pPlayerInfo);

                // mark the player list as having changed
                m_pMissionInfo->GetSideInfo(pPlayerInfo->SideID())->GetMembers().GetSink()();
                }
            break;
        }

        case FM_S_MISSION_STAGE:
        {
            CASTPFM(pfmMissionStage, S, MISSION_STAGE, pfm);

            m_pMissionInfo->SetStage(pfmMissionStage->stage);

            switch (pfmMissionStage->stage)
            {
                case STAGE_NOTSTARTED:
                {
                    m_pMissionInfo->SetInProgress(false);
                    m_pMissionInfo->SetCountdownStarted(false);
                    m_bWaitingForGameRestart = false;
                    m_pClientEventSource->OnMissionEnded(m_pMissionInfo);
                    break;
                }
                case STAGE_STARTING:
                {
                    m_pMissionInfo->SetCountdownStarted(true);
                    m_pClientEventSource->OnMissionCountdown(m_pMissionInfo);
                    break;
                }
                case STAGE_STARTED:
                {
                    m_pMissionInfo->SetCountdownStarted(false);
                    m_pMissionInfo->SetInProgress(true);

                    assert (memcmp(m_pCoreIGC->GetMissionParams(),
                                   &m_pMissionInfo->GetMissionParams(),
                                   sizeof(m_pMissionInfo->GetMissionParams())) == 0);

                    m_pCoreIGC->SetMissionStage(STAGE_STARTED);
                    m_pClientEventSource->OnMissionStarted(m_pMissionInfo);

                    break;
                }
                case STAGE_OVER:
                {
                    m_pMissionInfo->SetInProgress(false);
                    m_bWaitingForGameRestart = false;
                    m_pClientEventSource->OnMissionEnded(m_pMissionInfo);
                    break;
                }
                case STAGE_TERMINATE:
                {
                    RemovePlayerFromSide(m_pPlayerInfo, QSR_Quit);
                    RemovePlayerFromMission(m_pPlayerInfo, QSR_Quit);
                    break;
                }
                default:
                {
                    assert(false);
                }
            }
            m_mapMissions.GetSink()();
            break;
        }

        case FM_S_CREATE_BUCKETS:
        {
            m_pCoreIGC->SetMissionParams(&m_pMissionInfo->GetMissionParams());

            IsideIGC*   pside = GetSide();
            assert (pside);
            assert (pside->GetObjectID() != SIDE_TEAMLOBBY);
            {
                //Hack copy side attributes over to lobby side
                m_pCoreIGC->GetSide(SIDE_TEAMLOBBY)->SetGlobalAttributeSet(pside->GetGlobalAttributeSet());

                CASTPFM(pfmCB, S, CREATE_BUCKETS, pfm);

                pside->SetDevelopmentTechs(pfmCB->ttbmDevelopments);
                pside->SetInitialTechs(pfmCB->ttbmInitial);
                pside->CreateBuckets();
            }
        }
        break;

        case FM_S_GAME_STATE:
        {
            CASTPFM(pfmGS, S, GAME_STATE, pfm);

            SideID  sid = 0;
            for (SideLinkIGC*   l = m_pCoreIGC->GetSides()->first(); (l != NULL); l = l->next())
            {
                assert (sid == l->data()->GetObjectID());
                l->data()->SetConquestPercent(pfmGS->conquest[sid]);
                l->data()->SetTerritoryCount(pfmGS->territory[sid]);
                l->data()->SetFlags(pfmGS->nFlags[sid]);
                l->data()->SetArtifacts(pfmGS->nArtifacts[sid++]);
            }
        }
        break;

        case FM_S_MISSIONDEF:
        {
            CASTPFM(pfmMissionDef, S, MISSIONDEF, pfm);

            pfmMissionDef->misparms.timeStart = ClientTimeFromServerTime(pfmMissionDef->misparms.timeStart);

            if (m_pMissionInfo == NULL)
            {
                m_pMissionInfo = new MissionInfo(pfmMissionDef->dwCookie);
                m_pMissionInfo->Update(pfmMissionDef);
            }
            else
            {
                m_pMissionInfo->Update(pfmMissionDef);
                m_pCoreIGC->SetMissionParams(&pfmMissionDef->misparms);
                m_pCoreIGC->UpdateSides(Time::Now(), &(m_pMissionInfo->GetMissionParams()), pfmMissionDef->rgszName);
				m_pCoreIGC->UpdateAllies(pfmMissionDef->rgfAllies); //#ALLY
            }

            m_pClientEventSource->OnAddMission(m_pMissionInfo);
            break;
        }

        case FM_S_SIDE_TECH_CHANGE:
        {
            CASTPFM(pfmSTC, S, SIDE_TECH_CHANGE, pfm);
            IsideIGC* pSide = m_pCoreIGC->GetSide(pfmSTC->sideID);
            assert (pSide);

            pSide->SetDevelopmentTechs(pfmSTC->ttbmDevelopments);
        }
        break;

        case FM_S_SIDE_ATTRIBUTE_CHANGE:
        {
            CASTPFM(pfmSAC, S, SIDE_ATTRIBUTE_CHANGE, pfm);
            IsideIGC* pSide = m_pCoreIGC->GetSide(pfmSAC->sideID);
            assert (pSide);

            pSide->SetGlobalAttributeSet(pfmSAC->gasAttributes);

            // HACK: copy the global attributes to the lobby side for loadout
            if (pSide == GetSide())
                GetCore()->GetSide(SIDE_TEAMLOBBY)->SetGlobalAttributeSet(pfmSAC->gasAttributes);
        }
        break;

        case FM_S_BUCKET_STATUS:
        {
            CASTPFM(pfmBucketStatus, S, BUCKET_STATUS, pfm);

            IsideIGC*   pside = m_pCoreIGC->GetSide(pfmBucketStatus->sideID);
            assert (pside);
            IbucketIGC* b = pside->GetBucket(pfmBucketStatus->iBucket);

            if (b)
                b->SetTimeAndMoney(pfmBucketStatus->timeTotal,
                                   pfmBucketStatus->moneyTotal);

            break;
        }

        case FM_S_POSITIONREQ:  // sent to team leader to request a pos
        {
            CASTPFM(pfmPosReq, S, POSITIONREQ, pfm);
            PlayerInfo* ppi = FindPlayer(pfmPosReq->shipID);

            m_pMissionInfo->AddRequest(pfmPosReq->iSide, pfmPosReq->shipID);
            m_pClientEventSource->OnAddRequest(m_pMissionInfo, pfmPosReq->iSide, ppi);

            if (MyPlayerInfo()->IsTeamLeader() && (pfmPosReq->iSide == GetSideID()))
                PostText(true, "%s wishes to join your team", ppi->CharacterName());

            PlaySoundEffect(salPlayerWaitingSound);

            break;
        }

        case FM_CS_DELPOSITIONREQ:
        {
            CASTPFM(pfmDelPositionReq, CS, DELPOSITIONREQ, pfm);

            PlayerInfo* pPlayerInfo = FindPlayer(pfmDelPositionReq->shipID);

            if (pPlayerInfo && pfmDelPositionReq->iSide < m_pMissionInfo->GetMissionParams().nTeams
                && m_pMissionInfo->FindRequest(pfmDelPositionReq->iSide, pfmDelPositionReq->shipID))
            {
                m_pMissionInfo->RemoveRequest(pfmDelPositionReq->iSide, pfmDelPositionReq->shipID);
            }
            m_pClientEventSource->OnDelRequest(m_pMissionInfo, pfmDelPositionReq->iSide, pPlayerInfo, pfmDelPositionReq->reason);
            break;
        }

        case FM_CS_QUIT_MISSION:
        {
            if (m_pMissionInfo)
            {
                // someone is quiting...
                CASTPFM(pfmQuitMission, CS, QUIT_MISSION, pfm);
                PlayerInfo* pPlayerInfo = FindPlayer(pfmQuitMission->shipID);

                if (!pPlayerInfo || pPlayerInfo->SideID() == NA)
                {
                    assert(false);
                    break;
                }

                if (pPlayerInfo->SideID() != SIDE_TEAMLOBBY)
                {
                    assert(false);
                    RemovePlayerFromSide(pPlayerInfo, QSR_Quit);
                }

                SideID sideIDOld = pPlayerInfo->SideID();
                RemovePlayerFromMission(pPlayerInfo, pfmQuitMission->reason, FM_VAR_REF(pfmQuitMission, szMessageParam));

                // review: use a new message
                m_pClientEventSource->OnDelRequest(m_pMissionInfo, sideIDOld, pPlayerInfo, DPR_Canceled);
            }
            break;
        }

        case FM_CS_CHANGE_TEAM_CIV:
        {
            CASTPFM(pfmChangeCiv, CS, CHANGE_TEAM_CIV, pfm);
            //m_pMissionInfo->SetSideCivID(pfmChangeCiv->iSide, pfmChangeCiv->civID);

            m_pCoreIGC->GetSide(pfmChangeCiv->iSide)->SetCivilization(m_pCoreIGC->GetCivilization(pfmChangeCiv->civID));
            m_pClientEventSource->OnTeamCivChange(m_pMissionInfo, pfmChangeCiv->iSide, pfmChangeCiv->civID);
			m_pCoreIGC->GetSide(pfmChangeCiv->iSide)->SetRandomCivilization(pfmChangeCiv->random);
            break;
        }

        case FM_CS_SET_TEAM_INFO:
        {
            CASTPFM(pfmSetTeamInfo, CS, SET_TEAM_INFO, pfm);
            IsideIGC* pside = m_pCoreIGC->GetSide(pfmSetTeamInfo->sideID);
            assert(pside);
            pside->SetSquadID(pfmSetTeamInfo->squadID);
            pside->SetName(pfmSetTeamInfo->SideName);
            m_pMissionInfo->SetSideName(pfmSetTeamInfo->sideID, pfmSetTeamInfo->SideName);
            m_pClientEventSource->OnTeamNameChange(m_pMissionInfo, pfmSetTeamInfo->sideID);
            break;
        }

		// #ALLY
		case FM_S_CHANGE_ALLIANCES:
		{
			// update our local missioninfo, IGC sides and call the sink
            CASTPFM(pfmChangeAlliances, S, CHANGE_ALLIANCES, pfm);
			for (SideID i = 0; i < c_cSidesMax ; i++)
			{
				// missioninfo
				m_pMissionInfo->SetSideAllies(i,pfmChangeAlliances->Allies[i]);

				// IGC
				IsideIGC* pside = m_pCoreIGC->GetSide(i);
				if (pside)
					pside->SetAllies(pfmChangeAlliances->Allies[i]);
			}
            m_pClientEventSource->OnTeamAlliancesChange(m_pMissionInfo);
			break;
		}

        case FM_S_TEAM_READY:
        {
            CASTPFM(pfmTeamReady, S, TEAM_READY, pfm);
            m_pMissionInfo->SetSideReady(pfmTeamReady->iSide, pfmTeamReady->fReady);
            m_pClientEventSource->OnTeamReadyChange(m_pMissionInfo, pfmTeamReady->iSide, pfmTeamReady->fReady);
            break;
        }

        case FM_CS_SIDE_INACTIVE: //KGJV #62 - changed to CS
        {
            CASTPFM(pfmSideInactive, CS, SIDE_INACTIVE, pfm); // KGJV #62 - changed to CS
            debugf("Side inactive, side=%d\n", pfmSideInactive->sideID);
            m_pMissionInfo->SetSideActive(pfmSideInactive->sideID, pfmSideInactive->bActive); // KGJV #62 was false
			// update the AET bit
			if (pfmSideInactive->bChangeAET)
				m_pMissionInfo->SetAllowEmptyTeams(pfmSideInactive->bAET);
				
            m_pClientEventSource->OnTeamInactive(m_pMissionInfo, pfmSideInactive->sideID);
            m_mapMissions.GetSink()();
            break;
        }

        case FM_CS_FORCE_TEAM_READY:
        {
            CASTPFM(pfmTeamForceReady, CS, FORCE_TEAM_READY, pfm);
            m_pMissionInfo->SetSideForceReady(pfmTeamForceReady->iSide, pfmTeamForceReady->fForceReady);
            m_pClientEventSource->OnTeamForceReadyChange(m_pMissionInfo, pfmTeamForceReady->iSide, pfmTeamForceReady->fForceReady);
        }
        break;

        case FM_CS_AUTO_ACCEPT:
        {
            CASTPFM(pfmAutoAccept, CS, AUTO_ACCEPT, pfm);
            m_pMissionInfo->SetSideAutoAccept(pfmAutoAccept->iSide, pfmAutoAccept->fAutoAccept);
            m_pClientEventSource->OnTeamAutoAcceptChange(m_pMissionInfo, pfmAutoAccept->iSide, pfmAutoAccept->fAutoAccept);
            m_mapMissions.GetSink()();
        }
        break;

        case FM_CS_LOCK_LOBBY:
        {
            CASTPFM(pfmLockLobby, CS, LOCK_LOBBY, pfm);
            m_pMissionInfo->SetLockLobby(pfmLockLobby->fLock);
            m_pClientEventSource->OnLockLobby(pfmLockLobby->fLock);
            m_mapMissions.GetSink()();
        }
        break;
        
        case FM_CS_LOCK_SIDES:
        {
            CASTPFM(pfmLockSides, CS, LOCK_SIDES, pfm);
            m_pMissionInfo->SetLockSides(pfmLockSides->fLock);
            m_pClientEventSource->OnLockSides(pfmLockSides->fLock);
            m_mapMissions.GetSink()();
        }
        break;
        
        case FM_CS_REQUEST_MONEY:
        {
            CASTPFM(pfmRequest, CS, REQUEST_MONEY, pfm);
            IshipIGC*   pship = m_ship->GetSide()->GetShip(pfmRequest->shipidRequest);
            if (pship)
            {
                GetMoneyRequest(pship, pfmRequest->amount, pfmRequest->hidFor);
            }
        }
        break;

        case FM_CS_CHATMESSAGE:
        {
            CASTPFM(pfmChat, CS, CHATMESSAGE, pfm);
            //Ignore chats we sent ... we will have already handled them when sent
            if (pfmChat->cd.sidSender != m_ship->GetObjectID())
            {
                ReceiveChat(m_pCoreIGC->GetShip(pfmChat->cd.sidSender),
                            pfmChat->cd.chatTarget,
                            pfmChat->cd.oidRecipient,
                            pfmChat->cd.voiceOver,
                            FM_VAR_REF(pfmChat, Message),
                            pfmChat->cd.commandID,
                            pfmChat->otTarget,
                            pfmChat->oidTarget,
                            NULL,
                            pfmChat->cd.bObjectModel);
            }
            break;
        }

        case FM_CS_CHATBUOY:
        {
            if (!IsInGame())
                break;

            CASTPFM(pfmBuoy, CS, CHATBUOY, pfm);

            if (pfmBuoy->cd.sidSender != m_ship->GetObjectID())
            {
                //Create a buoy for this chat message
                IbaseIGC*   b = m_pCoreIGC->CreateObject(lastUpdate, OT_buoy, &(pfmBuoy->db), sizeof(pfmBuoy->db));
                assert (b);

                ((IbuoyIGC*)b)->AddConsumer();

                ReceiveChat(m_pCoreIGC->GetShip(pfmBuoy->cd.sidSender),
                            pfmBuoy->cd.chatTarget,
                            pfmBuoy->cd.oidRecipient,
                            pfmBuoy->cd.voiceOver,
                            FM_VAR_REF(pfmBuoy, Message),
                            pfmBuoy->cd.commandID,
                            OT_buoy,
                            b->GetObjectID(),
                            (ImodelIGC*)b,
                            pfmBuoy->cd.bObjectModel);

                ((IbuoyIGC*)b)->ReleaseConsumer();
                b->Release();
            }
        }
        break;

        case FM_CS_MISSIONPARAMS:
        {
            assert(false);
            CASTPFM(pfmMissionParams, CS, MISSIONPARAMS, pfm);
            pfmMissionParams->missionparams.timeStart = ClientTimeFromServerTime(pfmMissionParams->missionparams.timeStart);

            m_pCoreIGC->SetMissionParams(&(pfmMissionParams->missionparams));
        }
        break;

        case FM_S_SET_START_TIME:
        {
            CASTPFM(pfmStartTime, S, SET_START_TIME, pfm);
            
            ZAssert(MyMission());
            Time timeStart = ClientTimeFromServerTime(pfmStartTime->timeStart);
            MyMission()->UpdateStartTime(timeStart);
            m_pCoreIGC->SetStartTime(timeStart);
        }
        break;

        case FM_S_SHIP_STATUS:
        {
            if (IsWaitingForGameRestart())
                break;

            CASTPFM(pfmShipStatus, S, SHIP_STATUS, pfm);
            PlayerInfo* pPlayerInfo = FindPlayer(pfmShipStatus->shipID);

            if (pPlayerInfo && m_pMissionInfo)
            {
                if ((pfmShipStatus->status.GetSectorID() != NA) &&
                    (pfmShipStatus->status.GetSectorID() != pPlayerInfo->LastSeenSector()))
                {
                    ShipWarped(pPlayerInfo->GetShip(),
                               pPlayerInfo->LastSeenSector(),
                               pfmShipStatus->status.GetSectorID());
                }

                // update the last seen info
                pPlayerInfo->SetShipStatus(pfmShipStatus->status);

                // update any lists using this info
                m_pMissionInfo->GetSideInfo(pPlayerInfo->SideID())->GetMembers().GetSink()();
                m_pClientEventSource->OnShipStatusChange(pPlayerInfo);

                if ((pfmShipStatus->status.GetState() == c_ssTurret) !=
                    (pPlayerInfo->LastSeenState()     == c_ssTurret))
                {
                    m_pClientEventSource->OnTurretStateChanging(pfmShipStatus->status.GetState() == c_ssTurret);    
                }
            }
            break;
        }

        case FM_S_KILL_SHIP:
        {
            CASTPFM(pfmKillShip, S, KILL_SHIP, pfm);
            PlayerInfo* pPlayerInfo = FindPlayer(pfmKillShip->shipID);
            ZAssert(pPlayerInfo);

            ImodelIGC*  pmodelKiller = m_pCoreIGC->GetModel(pfmKillShip->typeCredit, pfmKillShip->idCredit);
            PlayerInfo* pLauncherInfo = ((pfmKillShip->typeCredit == OT_ship) && pmodelKiller)
                                        ? (PlayerInfo*)(((IshipIGC*)pmodelKiller)->GetPrivateData())
                                        : NULL;

            IsideIGC*   pside = GetSide();
            SideID      sideID = pside->GetObjectID();
            ShipID      shipID = GetShipID();

            IshipIGC*   pship = pPlayerInfo->GetShip();
            assert (pship);
            TargetKilled(pship);

            if (pfmKillShip->bDeathCredit)
            {
                pPlayerInfo->AddDeath();
                pPlayerInfo->GetShip()->SetExperience(0.0f);
            }
            else
                pPlayerInfo->AddEjection();

            MyMission()->GetSideInfo(pPlayerInfo->SideID())->GetMembers().GetSink()();

            const char* pszVictim = pPlayerInfo->CharacterName();
            bool bConstructorOrMiner;
            {
                PilotType pt = pship->GetPilotType();
                bConstructorOrMiner = (pt == c_ptBuilder) ||
                                      (pt == c_ptMiner) ||
                                      (pt == c_ptCarrier);
            }

            if (!pLauncherInfo)
            {
                if (pfmKillShip->bKillCredit)
                {
                    assert (pfmKillShip->typeCredit != NA);

                    IsideIGC*   pside = pmodelKiller ? pmodelKiller->GetSide() : m_pCoreIGC->GetSide(pfmKillShip->sideidKiller);
                    assert (pside);
                    pside->AddKill();
                }

                if (pfmKillShip->shipID == shipID)
                    PostText(true, pfmKillShip->bDeathCredit ? "You were killed" : "You ejected");
                else if (bConstructorOrMiner)
                    PostText(pPlayerInfo->SideID() == GetSideID(), START_COLOR_STRING "%s" END_COLOR_STRING " was killed", (PCC) ConvertColorToString (m_pCoreIGC->GetSide(pPlayerInfo->SideID())->GetColor ()), pszVictim);
                else 
                    PostText(false, pfmKillShip->bDeathCredit ? START_COLOR_STRING "%s" END_COLOR_STRING " was killed" : START_COLOR_STRING "%s" END_COLOR_STRING " ejected", (PCC) ConvertColorToString (m_pCoreIGC->GetSide(pPlayerInfo->SideID())->GetColor ()), pszVictim);
            }
            else
            {
                const char* pszLauncher = pLauncherInfo->CharacterName();

                if (pfmKillShip->bKillCredit)
                {
                    pLauncherInfo->AddKill();
                    if (pLauncherInfo->GetShip()->GetSide() != pship->GetSide() && !pship->GetSide()->AlliedSides(pLauncherInfo->GetShip()->GetSide(),pship->GetSide())) //ALLY 7/27/09 rock imago
                        pLauncherInfo->GetShip()->AddExperience();
                }

                MyMission()->GetSideInfo(pLauncherInfo->SideID())->GetMembers().GetSink()();

                // if we can see the victim or if the perpatrator or victim
                // is on our side...
                if (pfmKillShip->shipID == shipID)
                    PostText(true,
                             pfmKillShip->bDeathCredit
                             ? "You were killed by " START_COLOR_STRING "%s" END_COLOR_STRING
                             : "You were forced to eject by " START_COLOR_STRING "%s" END_COLOR_STRING,
                             (PCC) ConvertColorToString (pLauncherInfo->GetShip()->GetSide()->GetColor ()), pszLauncher);
                else if (pmodelKiller == m_ship)
                    PostText(true,
                             pfmKillShip->bDeathCredit
                             ? "You killed " START_COLOR_STRING "%s"  END_COLOR_STRING
                             : "You forced " START_COLOR_STRING "%s"  END_COLOR_STRING " to eject",
                             (PCC) ConvertColorToString (pPlayerInfo->GetShip()->GetSide()->GetColor ()), pszVictim);
                else
                {
                    if (bConstructorOrMiner && pPlayerInfo->SideID() != GetSideID())
                        PostText(bConstructorOrMiner,
                                 pfmKillShip->bDeathCredit
                                 ? (START_COLOR_STRING "%s %s" END_COLOR_STRING " was killed by %s") 
                                 : (START_COLOR_STRING "%s %s" END_COLOR_STRING " was forced to eject by %s"),
                                 (PCC) ConvertColorToString (pPlayerInfo->GetShip()->GetSide()->GetColor ()),
                                 pPlayerInfo->GetShip()->GetSide()->GetName(),
                                 pszVictim, pszLauncher);
                    else
                        PostText(bConstructorOrMiner,
                                 pfmKillShip->bDeathCredit
                                 ? "%s was killed by %s"
                                 : "%s was forced to eject by %s", pszVictim, pszLauncher);
                }
            }

            IclusterIGC * pCluster = pship->GetCluster();
            if (NULL != pCluster)
            {
                IhullTypeIGC*   pht = pship->GetBaseHullType();
                if (pht)
                    pCluster->GetClusterSite()->AddExplosion(pship,
                                                             pht->HasCapability(c_habmFighter)
                                                             ? c_etSmallShip
                                                             : c_etBigShip);
            }

            // for drones, play the drone destruction sound
            if (!pPlayerInfo->IsHuman())
            {
                if (sideID == pPlayerInfo->SideID()) //ALLYTD too much spam?  rock imago
                {
                    SoundID destroyedSound;

                    switch (pship->GetPilotType())
                    {
                        case c_ptCarrier:       //NYI need carrier destroyed sound
                            destroyedSound = carrierDestroyedSound;
                            break;

                        case c_ptMiner:
                            destroyedSound = minerDestroyedSound;
                            break;

                        case c_ptBuilder:
                            destroyedSound = 
                                ((IstationTypeIGC*)(IbaseIGC*)(pship->GetBaseData()))
                                    ->GetConstructorDestroyedSound();
                            break;

                        case c_ptLayer:
                            if (pship->GetBaseData()->GetObjectType() == OT_mineType)
                                destroyedSound = minefieldLayerDestroyedSound;
                            else
                                destroyedSound = towerLayerDestroyedSound;
                            break;

                        default:
                            assert(false);
                        case c_ptWingman:
                            destroyedSound = towerLayerDestroyedSound;
                            break;
                    }

                    PlaySoundEffect(destroyedSound);
                }
            }
            else if ((pship == m_ship) && pfmKillShip->bDeathCredit)
            {
                //The player died ... set up the gloat camera
                EjectPlayer(m_pCoreIGC->GetModel(pfmKillShip->typeCredit, pfmKillShip->idCredit));
            }

            ChangeGameState();
        }
        break;

        case FM_S_JOIN_SIDE:
        {
            CASTPFM(pfmJoinSide, S, JOIN_SIDE, pfm);

            PlayerInfo* pplayer = FindPlayer(pfmJoinSide->shipID);
            AddPlayerToSide(pplayer, pfmJoinSide->sideID);
        }
        break;

        /*
        case FM_S_SET_EXPERIENCE:
        {
            CASTPFM(pfmSet, S, SET_EXPERIENCE, pfm);

            IshipIGC*   pship = m_pCoreIGC->GetShip(pfmSet->shipID);
            assert (pship);
            pship->SetExperience(pfmSet->fExperience);
        }
        break;
        */

        case FM_CS_SET_TEAM_LEADER:
        {
            CASTPFM(pfmSetTeamLeader, CS, SET_TEAM_LEADER, pfm);

            PlayerInfo* pplayer = FindPlayer(pfmSetTeamLeader->shipID);

            // demote the previous team leader (if any)
            if (MyMission()->SideLeaderShipID(pplayer->SideID()) != NA)
            {
                PlayerInfo* pplayerOld = FindPlayer(MyMission()->SideLeaderShipID(pplayer->SideID()));
                pplayerOld->SetTeamLeader(false);
                pplayerOld->SetMissionOwner(false);
                m_pClientEventSource->OnPlayerStatusChange(MyMission(), pplayerOld->SideID(), pplayerOld);
            }

            pplayer->SetTeamLeader(true);
            pplayer->SetMissionOwner(MyMission()->MissionOwnerSideID() == pplayer->SideID());

            MyMission()->SetSideLeader(pplayer);

            if ((m_pCoreIGC->GetMissionStage() == STAGE_STARTED) &&
                pplayer->SideID() == MyPlayerInfo()->SideID())
            {
                //Leader on our team has switched
                if (pplayer == MyPlayerInfo())
                    PostText(true, "You have become the team leader");
                else
                {
                    char    bfr[100];
                    sprintf(bfr, "%s has become the team leader", pplayer->CharacterName());
                    PostPlainText(true, bfr);
                }
            }

            if (pplayer == MyPlayerInfo())
                PlaySoundEffect(commanderSound);

            m_pClientEventSource->OnPlayerStatusChange(MyMission(), pplayer->SideID(), pplayer);
        }
        break;

        case FM_CS_SET_MISSION_OWNER:
        {
            CASTPFM(pfmSetMissionOwner, CS, SET_MISSION_OWNER, pfm);

            // demote the previous mission owner (if any)
            if (MyMission()->MissionOwnerSideID() != NA)
            {
                PlayerInfo* pplayerOld = FindPlayer(MyMission()->MissionOwnerShipID());
                pplayerOld->SetMissionOwner(false);
                m_pClientEventSource->OnPlayerStatusChange(MyMission(), pplayerOld->SideID(), pplayerOld);
            }

            PlayerInfo* pplayer = FindPlayer(pfmSetMissionOwner->shipID);
            ZAssert(pplayer->IsTeamLeader());
            pplayer->SetMissionOwner(true);

            MyMission()->SetSideLeader(pplayer);

            m_pClientEventSource->OnPlayerStatusChange(MyMission(), pplayer->SideID(), pplayer);
        }
        break;

        case FM_CS_QUIT_SIDE:
        {
            if (m_pMissionInfo)
            {
                CASTPFM(pfmQuitSide, CS, QUIT_SIDE, pfm);

                PlayerInfo* pplayer = FindPlayer(pfmQuitSide->shipID);
                RemovePlayerFromSide(pplayer, pfmQuitSide->reason, FM_VAR_REF(pfmQuitSide, szMessageParam));
            }
        }
        break;

        case FM_S_SQUAD_INFO:
        case FM_S_SQUAD_DETAILS:
        case FM_S_SQUAD_DETAILS_PLAYER:
        case FM_S_SQUAD_LOG_INFO:
        case FM_S_SQUAD_DELETED:
        case FM_S_SQUAD_TEXT_MESSAGE:
        case FM_S_SQUAD_CREATE_ACK:
        {
            ForwardSquadMessage(pfm); // forward message to squads screen
        }
        break;

        case FM_S_CHARACTER_INFO_GENERAL:
        case FM_S_CHARACTER_INFO_BY_CIV:
        case FM_S_CHARACTER_INFO_MEDAL:
        {
            ForwardCharInfoMessage(pfm); // forward message to character info screen
        }
        break;

        case FM_S_SQUAD_INFO_DUDEX:
        {
            // forward messages that go to either of these screens
            ForwardSquadMessage(pfm); 
            ForwardCharInfoMessage(pfm); 
        }
        break;

        case FM_S_LEADER_BOARD_QUERY_FAIL:
        case FM_S_LEADER_BOARD_LIST:
        {
            ForwardLeaderBoardMessage(pfm); // forward message to leader board screen
        }
        break;

        case FM_S_ENTER_GAME:
        {
            ClearLoadout ();
            LoadCustomLoadoutFile (); //AaronMoore 1/10
            OnEnterGame();
        }
        break;

        case FM_S_GAME_OVER:
        {
            m_bInGame = false;
            m_bWaitingForGameRestart = true;
            m_strBriefingText.SetEmpty();
            m_bGenerateCivBriefing = false;

            CASTPFM(pfmGameOver, S, GAME_OVER, pfm);

            if (IsLockedDown())
                EndLockDown(lockdownDonating | lockdownLoadout | lockdownTeleporting);

            SetViewCluster(NULL);
            GetShip()->SetCluster(NULL);
            
            // set all of the players to unready with 0 money
            for (PlayerLink*    ppl = GetPlayerList()->first();
                (ppl != NULL);
                ppl = ppl->next())
            {
                // review: since we are trying this silly away from keyboard thing,
                // set everyone as ready (= not away from keyboard)
				// Imago commented out the below line becasue of what the devs wrote on the above line =)
                // ppl->data().SetReady(true); Imago commented out so afk not reset
                ppl->data().Reset(true);
            }

            // set all of the sides to active and not forced ready
            for (SideID sid = 0; sid < MyMission()->NumSides(); sid++)
            {
                MyMission()->SetSideActive(sid, true);
                GetCore()->GetSide(sid)->SetActiveF(true);
                MyMission()->SetSideForceReady(sid, false);
                MyMission()->SetSideReady(sid, true);
            }

            // nuke any pending votes
            m_listBallots.SetEmpty();

            for (SideLinkIGC*   psl = GetCore()->GetSides()->first(); (psl != NULL); psl = psl->next())
                psl->data()->Reset();
            
            GetCore()->ResetMission();
        }
        break;

        case FM_S_GAME_OVER_PLAYERS:
        {
            CASTPFM(pfmGameOverPlayers, S, GAME_OVER_PLAYERS, pfm);

            // update the scores of all of the players
            int cPlayerEndgameInfo = pfmGameOverPlayers->cbrgPlayerInfo / sizeof(PlayerEndgameInfo);
            PlayerEndgameInfo* vPlayerEndgameInfo 
                = (PlayerEndgameInfo*)FM_VAR_REF(pfmGameOverPlayers, rgPlayerInfo);

            for (int i = 0; i < cPlayerEndgameInfo; i++)
            {
                PlayerInfo* pplayer = FindPlayer(vPlayerEndgameInfo[i].characterName);

                if (pplayer)
                    pplayer->UpdateScore(vPlayerEndgameInfo[i].stats);
            }
        }
        break;

        case FM_S_GAIN_FLAG:
        {
            if (IsWaitingForGameRestart())
                break;

            CASTPFM(pfmGain, S, GAIN_FLAG, pfm);
            IshipIGC*   pship = m_pCoreIGC->GetShip(pfmGain->shipidRecipient);
            assert (pship);
			if (pfmGain->bIsTreasureDocked) // KGJV #118 - extended for docked tech
			{
				if (pship->GetSide() != GetSide()) break;
				IpartTypeIGC *ppartType = GetCore()->GetPartType(pfmGain->parttypeidDocked);
				if (ppartType)
					PostText(true, "%s has secured %s",
						pship->GetName(),
						ppartType->GetName()
					);
			}
			else
			{ // normal flag capture code
				assert ((pship->GetFlag() == NA) || (pfmGain->sideidFlag == NA));

				pship->SetFlag(pfmGain->sideidFlag);

				if (pfmGain->sideidFlag != NA)
				{
					if (pship->GetSide() == GetSide())
					{
						if (pfmGain->sideidFlag != SIDE_TEAMLOBBY)
						{
							PostText(true, "%s has stolen " START_COLOR_STRING "%s's" END_COLOR_STRING " flag.", 
								pship->GetName(), 
								(PCC) ConvertColorToString (GetCore()->GetSide(pfmGain->sideidFlag)->GetColor ()),
								GetCore()->GetSide(pfmGain->sideidFlag)->GetName()
								);

							PlaySoundEffect(enemyFlagLostSound);
						}
						else
						{
							PostText(true, "%s has found an artifact.", 
								pship->GetName());

							PlaySoundEffect(artifactFoundSound);
						}
					}
					else if (pfmGain->sideidFlag > 0 && pfmGain->sideidFlag == GetSideID())
					{
						ZString color = ConvertColorToString (pship->GetSide()->GetColor ());
						PostText(true, START_COLOR_STRING "%s" END_COLOR_STRING " of " START_COLOR_STRING "%s" END_COLOR_STRING " has stolen our flag.", 
							(PCC) color,
							pship->GetName(), 
							(PCC) color,
							pship->GetSide()->GetName()
							);

						PlaySoundEffect(flagLostSound);
					}
				}
			}// KGJV #118 - else bIsTreasureDocked
        }
        break;

        case FM_S_BUY_LOADOUT_ACK:
        {
            CASTPFM(pfmBuyLoadoutAck, S, BUY_LOADOUT_ACK, pfm);

            if (!pfmBuyLoadoutAck->fLaunch && IsLockedDown())
                EndLockDown(lockdownLoadout);

            m_ship->ProcessShipLoadout(pfmBuyLoadoutAck->cbloadout, (const ShipLoadout*)(FM_VAR_REF(pfmBuyLoadoutAck, loadout)), true);

            SaveLoadout();

            m_pClientEventSource->OnPurchaseCompleted(pfmBuyLoadoutAck->fBoughtEverything);

            // if they wanted to board a new ship after disembarking, we are 
            // finally at a safe place to do so.  
            if (m_sidBoardAfterDisembark != NA) 
            {
                IshipIGC* pshipNewParent = FindPlayer(m_sidBoardAfterDisembark)->GetShip();

                if (pshipNewParent)
                {
                    BoardShip(pshipNewParent);
                }

                m_sidBoardAfterDisembark = NA;
            }
            // otherwise, if they wanted to teleport we can do that now too.
            else if (m_sidTeleportAfterDisembark != NA)
            {
                IstationIGC* pstation = GetCore()->GetStation(m_sidTeleportAfterDisembark);

                if (pstation != GetShip()->GetStation())
                {
                    SetMessageType(BaseClient::c_mtGuaranteed);
                    BEGIN_PFM_CREATE(m_fm, pfmDocked, C, DOCKED)
                    END_PFM_CREATE

                    pfmDocked->stationID = m_sidTeleportAfterDisembark;

                    StartLockDown("Teleporting to " + ZString(pstation->GetName()) + "....", lockdownTeleporting);
                }

                m_sidTeleportAfterDisembark = NA;
            }
        }
        break;

        case FM_S_OBJECT_SPOTTED: //ALLY imago 7/12/09 					
        {							//we can safely assume if an object is spotted by somone not on our team it's an ally 7/13/09
            if (!IsInGame())
                break;

            CASTPFM(pfmObjectSpotted, S, OBJECT_SPOTTED, pfm);

            ZString strSpotterName;
			ZString strAllies;
			Color AllianceColors[3] = { Color::Green(), Color::Orange(), Color::Red() };
			IsideIGC* myside = GetSide();

            switch (pfmObjectSpotted->otSpotter)
            {
            case OT_station:
				if (GetCore()->GetStation(pfmObjectSpotted->oidSpotter)->GetSide() != myside) {
					

 					strAllies = "\x81 " + ConvertColorToString(AllianceColors[myside->GetAllies()]*0.75) + "Allied" + END_COLOR_STRING + " " ;
				} else {
					strAllies = "Your "; 
				}
                strSpotterName = strAllies 
                    + GetCore()->GetStation(pfmObjectSpotted->oidSpotter)->GetName()
                    + " has";
                break;

            case OT_probe:
				if (GetCore()->GetProbe(pfmObjectSpotted->oidSpotter)->GetSide() != myside) {
					
 					strAllies = "\x81 " + ConvertColorToString(AllianceColors[myside->GetAllies()]*0.75) + "ally's" + END_COLOR_STRING+" " ;
				} else {
					strAllies = "team's"; 
				}
                strSpotterName = "One of your "+ZString(strAllies)+" probes has";
                break;

            case OT_ship:
				if (GetCore()->GetShip(pfmObjectSpotted->oidSpotter)->GetSide() != myside) {
					
 					strAllies = "\x81 " + ConvertColorToString(AllianceColors[myside->GetAllies()]*0.75) + " (Ally)" + END_COLOR_STRING +" has" ;
				} else {
					strAllies = " has"; 
				}

                if (pfmObjectSpotted->oidSpotter == GetShipID())
                    strSpotterName = "You've";
                else
                    strSpotterName = GetCore()->GetShip(pfmObjectSpotted->oidSpotter)->GetName() + ZString(strAllies);
                break;

            default:
                assert(false);
                strSpotterName = "<bug> has";
                break;
            }

            switch (pfmObjectSpotted->otObject)
            {
            case OT_station:
                {
                    IstationIGC* pstation = GetCore()->GetStation(pfmObjectSpotted->oidObject);
					IsideIGC* myside = GetSide();
                    assert(pstation);
					if (myside->AlliedSides(myside,pstation->GetSide())) { //ALLY imago for when VIS is OFF
						PostText(false, strSpotterName + " discovered an allied " + pstation->GetName() + " in sector " + pstation->GetCluster()->GetName()); //#ALLY imago changed enemy to friendly if allied 7/3/09
					} else {
                    	PostText(GetShip()->GetWingID() == 0, strSpotterName + " discovered an enemy " + pstation->GetName() + " in sector " + pstation->GetCluster()->GetName()); 
					}
                }
                break;

            case OT_asteroid:
                {
                    IasteroidIGC* pasteroid = GetCore()->GetAsteroid(pfmObjectSpotted->oidObject);
                    assert(pasteroid);
                    PostText(GetShip()->GetWingID() == 0, strSpotterName + " discovered a " + IasteroidIGC::GetTypeName(pasteroid->GetCapabilities()) + " asteroid in sector " + pasteroid->GetCluster()->GetName());
                }
                break;

            case OT_warp:
                {
                    IwarpIGC* paleph = GetCore()->GetWarp(pfmObjectSpotted->oidObject);
                    assert(paleph);
                    PostText(GetShip()->GetWingID() == 0, strSpotterName + " discovered an aleph from sector " + paleph->GetCluster()->GetName() + " to sector " + paleph->GetDestination()->GetCluster()->GetName());
                }
                break;

            default:
                assert(false);
                break;
            }
        }
        break;

        case FM_S_DEV_COMPLETED:
        {
            CASTPFM(pfmDevelopmentCompleted, S, DEV_COMPLETED, pfm);

            IdevelopmentIGC* pdevelopment = GetCore()->GetDevelopment(pfmDevelopmentCompleted->devId);
            assert(pdevelopment);

            PostText(true, "%s research completed", pdevelopment->GetName());
            PlaySoundEffect(pdevelopment->GetCompletionSound());
        }
        break;

        case FM_S_SET_BRIEFING_TEXT:
        {
            CASTPFM(pfmSetBriefingText, S, SET_BRIEFING_TEXT, pfm);
            m_strBriefingText = FM_VAR_REF(pfmSetBriefingText, text);
            m_bGenerateCivBriefing = pfmSetBriefingText->fGenerateCivBriefing;
        }
        break;

        case FM_LS_SQUAD_MEMBERSHIPS:
        {
            CASTPFM(pfmSquadMemberships, LS, SQUAD_MEMBERSHIPS, pfm);
            SquadMembership* vSquadMemberships 
                = (SquadMembership*)FM_VAR_REF(pfmSquadMemberships, squadMemberships);
            m_squadmemberships.SetEmpty();
            for (int iSquad = 0; iSquad < pfmSquadMemberships->cSquadMemberships; iSquad++)
            {
                m_squadmemberships.PushEnd(vSquadMemberships[iSquad]);
            }
            SaveSquadMemberships(m_szCharName);
            m_mapMissions.GetSink()();
        }
        break;

        case FM_S_STATIC_MAP_INFO:
        {
            CASTPFM(pfmStaticMapInfo, S, STATIC_MAP_INFO, pfm);

            // copy the new static map info into our internal table        
            if (m_vStaticMapInfo)
                delete [] m_vStaticMapInfo;
            m_cStaticMapInfo = pfmStaticMapInfo->cbmaps / sizeof(StaticMapInfo);
            
            if (m_cStaticMapInfo > 0)
            {
                m_vStaticMapInfo = new StaticMapInfo[m_cStaticMapInfo];
                memcpy(m_vStaticMapInfo, FM_VAR_REF(pfmStaticMapInfo, maps), m_cStaticMapInfo * sizeof(StaticMapInfo));
            }
            else
                m_vStaticMapInfo = NULL;
        }
        break;

        case FM_S_RANK_INFO:
        {
            CASTPFM(pfmRankInfo, S, RANK_INFO, pfm);

            // copy the new rank info into our internal table        
            if (m_vRankInfo)
                delete [] m_vRankInfo;
            m_cRankInfo = pfmRankInfo->cRanks;
            m_vRankInfo = new RankInfo[m_cRankInfo];
            memcpy(m_vRankInfo, FM_VAR_REF(pfmRankInfo, ranks), m_cRankInfo * sizeof(RankInfo));
        }
        break;

        case FM_S_CLUB_RANK_INFO:
        {
            CASTPFM(pfmRankInfo, S, CLUB_RANK_INFO, pfm);

            // copy the new rank info into our internal table
            if (m_vRankInfo)
                delete [] m_vRankInfo;
            m_cRankInfo = pfmRankInfo->cRanks;
            m_vRankInfo = new RankInfo[m_cRankInfo];
            memcpy(m_vRankInfo, FM_VAR_REF(pfmRankInfo, ranks), m_cRankInfo * sizeof(RankInfo));
        }
        break;

        // ************ Lobby Messages *************

        case FM_L_AUTO_UPDATE_INFO: 
        {
            if (m_pAutoDownload == NULL)
                m_pAutoDownload = CreateAutoDownload();

            IAutoUpdateSink * pAutoUpdateSink = OnBeginAutoUpdate(NULL, false); //no relog #111
            assert(pAutoUpdateSink);

            CASTPFM(pfmServerInfo, L, AUTO_UPDATE_INFO, pfm);

            m_pAutoDownload->SetFTPSite(FM_VAR_REF(pfmServerInfo, FTPSite), 
                                        FM_VAR_REF(pfmServerInfo, FTPInitialDirectory), 
                                        FM_VAR_REF(pfmServerInfo, FTPAccount), 
                                        FM_VAR_REF(pfmServerInfo, FTPPassword));

            m_pAutoDownload->SetOfficialFileListAttributes(pfmServerInfo->crcFileList, 
                                                           pfmServerInfo->nFileListSize);

            // Disconnect during download
            DisconnectLobby(); 

            m_pAutoDownload->SetArtPath(GetArtPath());

            //
            // Let's do it!
            //
			m_pAutoDownload->BeginUpdate(pAutoUpdateSink, true); //#111 force check
            // m_pAutoDownload could be NULL at this point, if the autodownload system decided
            // not to do a download after all.  This can happen if there is an error or if
            // the client was already up-to-date.
        }
        break;

        case FM_L_CREATE_MISSION_ACK:
        {
            CASTPFM(pfmCreateMissionAck, L, CREATE_MISSION_ACK, pfm);
            // we remember it, so that if we're told to join a mission, we can be sure it's for the one we want to join,
            m_dwCookieToJoin = pfmCreateMissionAck->dwCookie;
            m_strPasswordToJoin[0] = '\0';
        }
        break;

        case FM_LS_LOBBYMISSIONINFO:
        {
            if (!LoggedOnToLobby()) 
                break;

            CASTPFM(pfmLobbyMissionInfo, LS, LOBBYMISSIONINFO, pfm);

            // The time offset may be off by as much as the maximum server 
            // latency, but that's OK for just displaying a timer.  Let's 
            // make sure we never assume a starting time in the future, however.
            pfmLobbyMissionInfo->dwStartTime += m_lobbyServerOffset;
            if (now - Time(pfmLobbyMissionInfo->dwStartTime) < 0 && !pfmLobbyMissionInfo->fCountdownStarted)
              pfmLobbyMissionInfo->dwStartTime = now.clock();

            MissionInfo * pMissionInfo = GetLobbyMission(pfmLobbyMissionInfo->dwCookie);
            assert(pMissionInfo);
            pMissionInfo->Update(pfmLobbyMissionInfo);
            m_mapMissions.GetSink()();
            m_pClientEventSource->OnAddMission(pMissionInfo);
            break;
        }

        case FM_L_JOIN_MISSION:
        {
            CASTPFM(pfmJoinMission, L, JOIN_MISSION, pfm);
            if (m_dwCookieToJoin = pfmJoinMission->dwCookie) // this is our number--join up
            {
                m_ci.strServer = pfmJoinMission->szServer;
				m_ci.dwPort = pfmJoinMission->dwPort;			// mdvalley: save the port number
                m_ci.guidSession = GUID_NULL;
                ConnectToServer(m_ci, pfmJoinMission->dwCookie, now, m_strPasswordToJoin, false);
            }
            else
            {
                OnLogonAck(false, false, "<bug>: Received a spurious join request.");
            }
            break;
        }

        case FM_L_CREATE_MISSION_NACK:
        {
          OnLogonAck(false, false, "There are no available servers on which to create a game.");
        }
        break;

        case FM_L_JOIN_GAME_NACK:
        {
          OnLogonAck(false, false, "There is no more room on this server.");
        }
        break;

        case FM_LS_MISSION_GONE:
        {
            CASTPFM(pfmMissionGone, LS, MISSION_GONE, pfm);
            MissionInfo * pMission = GetLobbyMission(pfmMissionGone->dwCookie);
 
            if (m_pMissionInfo != NULL && pMission->GetCookie() == MyMission()->GetCookie())
            {
                if (GetSideID() != NA)
                {
                    if (GetSideID() != SIDE_TEAMLOBBY)
                        RemovePlayerFromSide(m_pPlayerInfo, QSR_Quit);
                    RemovePlayerFromMission(m_pPlayerInfo, QSR_Quit);
                }
                else
                {
                    delete m_pMissionInfo;
                    m_pMissionInfo = NULL;
                }
            }

            m_pClientEventSource->OnDelMission(pMission);
            m_mapMissions.Remove(pfmMissionGone->dwCookie);
            delete pMission;
        }
        break;

        case FM_L_LOGON_ACK:
        {
            CASTPFM(pfmLogonAck, L, LOGON_ACK, pfm);
            m_fLoggedOnToLobby = true;
            m_lobbyServerOffset = pfmLogonAck->dwTimeOffset;
            OnLogonLobbyAck(true, false, NULL);
        }
        break;

        case FM_L_LOGON_NACK:
        {
            CASTPFM(pfmLogonNack, L, LOGON_NACK, pfm);
            OnLogonLobbyAck(false, pfmLogonNack->fRetry, FM_VAR_REF(pfmLogonNack, Reason));
            assert(m_fLoggedOnToLobby == false);
            m_szLobbyCharName[0] = '\0';
        }
        break;

        case FM_S_LOGON_CLUB_ACK:
        {
            CASTPFM(pfmLogonAck, S, LOGON_CLUB_ACK, pfm);
            SetZoneClubID(pfmLogonAck->nMemberID);
            m_fLoggedOnToClub = true;
            OnLogonClubAck(true, false, NULL);
        }
        break;

        case FM_S_LOGON_CLUB_NACK:
        {
            CASTPFM(pfmLogonNack, S, LOGON_CLUB_NACK, pfm);
            OnLogonClubAck(false, pfmLogonNack->fRetry, FM_VAR_REF(pfmLogonNack, Reason));
            assert(m_fLoggedOnToClub == false);
            m_szClubCharName[0] = '\0';
        }
        break;

        case FM_L_FOUND_PLAYER:
        {
            CASTPFM(pfmFoundPlayer, L, FOUND_PLAYER, pfm);

            MissionInfo* pMissionInfo;

            if (pfmFoundPlayer->dwCookie == NA)
                pMissionInfo = NULL;
            else
                pMissionInfo = GetLobbyMission(pfmFoundPlayer->dwCookie);

            m_pClientEventSource->OnFoundPlayer(pMissionInfo);
        }
        break;

		// KGJV #114
		case FM_L_SERVERS_LIST:
		{
			CASTPFM(pfmServerList, L, SERVERS_LIST, pfm);
			m_pClientEventSource->OnServersList(
				pfmServerList->cCores,
				FM_VAR_REF(pfmServerList,Cores),
				pfmServerList->cServers,
				FM_VAR_REF(pfmServerList,Servers)
				);
		}
		break;

        default:
        {
            // Note: we can't assert here because there are some messages 
            // which are only handled in TrekWindowImpl::HandleMessage. TrekIGC.cpp
            // already asserts that one of the two handles the message, however.  
            //ZError("unknown message");
            hr = S_FALSE;
        }
    }

    return (hr);
}
