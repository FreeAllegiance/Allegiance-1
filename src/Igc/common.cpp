#include    "igc.h"
#include    <math.h>

// mmf begin
/*
int _matherr( struct _exception *except )
{
	printf("offending function %s\n",except->name);
	// cause an exception not handled to exit
	// fmod in ThingGeo thows a math exception, skip it
	// hopefully the miner bug is not an fmod

	if ( strcmp( except->name, "fmod" ) == 0 ) return (0);

	(*(int*)0) = 0;

	return (0);

}
*/
// mmf end

const char*       c_pszWingName[c_widMax] =
                        { "command",
                          "attack",
                          "defend",
                          "escort",
                          "search",
                          "alpha",
                          "bravo",
                          "charlie",
                          "delta",
                          "echo",
                          //"foxtrot",
                          //"golf",
                          //"hotel",
                          //"india"//,
                          //"juliet"
                        };

const CommandData   c_cdAllCommands[c_cidMax] =
                    {
                        { "goto",       "acdefaultbmp", "qudefaultbmp" },
                        { "attack",     "ackillbmp",    "qukillbmp" },
                        { "capture",    "accptbmp",     "qucptbmp" },
                        { "defend",     "acdefendbmp",  "qudefendbmp" },
                        { "pickup",     "acpickupbmp",  "qupickupbmp" },
                        { "goto",       "acgotobmp",    "qugotobmp" },
                        { "repair",     "acrepairbmp",  "qurepairbmp" },
                        { "join",       "acjoinbmp",    "qujoinbmp" },
                        { "mine",       "acminebmp",    "quminebmp" },
                        { "build",      "acbuildbmp",   "qubuildbmp" }
                    };

const int   c_ttTypebits[OT_modelEnd+1] =
            { c_ttShip, c_ttStation, c_ttMissile,
              c_ttMine, c_ttProbe, c_ttAsteroid, 0, c_ttWarp, c_ttTreasure, c_ttBuoy, 0, 0};

float    solveForImpact(const Vector&      deltaP,
                        const Vector&      deltaV,
                        float              speed,
                        float              radius,
                        Vector*            direction)
{
    assert (speed >= 0.0f);
    assert (direction);
    float   t;
    float   c = deltaP * deltaP - radius * radius;
    if (c <= 0.0f)
    {
        t = 0.0f;
        *direction = deltaP.Normalize();
    }
    else
    {
        //Solve for the time when the projectile is t * speed + radius units away from the target.
        //  distance(t) = radius + speed * t = |deltaP + t * deltaV|
        //  distance(t)^2 = radius ^2 + 2 * radius * speed * t + speed ^2 * t^2 = deltaP * deltaP + 2 * t * deltaV * deltaP+ t^2 * deltaV * deltaV
        //
        //  0 = deltaP^2 - radius^2 + 2 * t * (deltaV * deltaP - radius * speed) + t^2 * (deltaV^2 - speed^2)
        //
        float   a = deltaV * deltaV - (speed * speed);
        float   b = 2.0f * (deltaV * deltaP - radius * speed);

        float   b24ac = b * b - 4.0f * a * c;

        if ((a == 0.0f) || ((a > 0.0f) && (b > 0.0f)) || (b24ac < 0.0f))
        {
            //No valid solution
            t = (FLT_MAX / 500000000.0f); // mmf podpickup bug added the divisor
			// mmf also note *direction is NOT SET in this case...
        }
        else
        {
			// mmf added check for negative
			// revisit what to set b24ac to in this case
			if (b24ac < 0.0f) {
				debugf("common.cpp b24ac is less than zero about to sqrt it, it to 0.1f\n");
				b24ac = 0.1f;
			}

            //quadratic formula ... we only care about the smallest root > 0
            t = (b + (float)sqrt(b24ac)) / (-2.0f * a);   //(-b-sqrt(b24ac))/(2a)    Only or smallest possible positive root

            if (t > 0.0f)   //should always be the case, but round-off error can cause problems
            {
                //Found a solution: now find the direction in which to shoot:
                *direction = (deltaP + t * deltaV) / (t * speed + radius);  //Should be a unit vector
            }
            else
            {
                t = 0.0f;
                *direction = deltaP.Normalize();
            }
        }
    }

    return t;
}

float    solveForLead(ImodelIGC*        shooter,
                      ImodelIGC*        target,
                      IweaponIGC*       weapon,
                      Vector*           direction,
                      float             skill)
{
    assert (shooter);
    assert (target);
    assert (weapon);
    assert (direction);

    const Vector&       myPosition = shooter->GetPosition();
    const Vector&       myVelocity = shooter->GetVelocity();
    const Orientation&  myOrientation = shooter->GetOrientation();

    const Vector&       hisPosition = target->GetPosition();
    const Vector&       hisVelocity = target->GetVelocity() * skill;

    IprojectileTypeIGC* pt = weapon->GetProjectileType();
    assert (pt);
	float speed = pt->GetSpeed() * (weapon->GetAmmoPerShot() ? shooter->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaSpeedAmmo) : 1);

    return solveForImpact(hisPosition - (myPosition + weapon->GetPosition() * myOrientation),
                          pt->GetAbsoluteF()
                          ? hisVelocity
                          : (hisVelocity - myVelocity),
                          speed, target->GetRadius(),
                          direction);
}


float    turnToFace(const Vector&       deltaTarget,
                    float               dt,
                    IshipIGC*           pship,
                    ControlData*        controls,
                    float               skill)
{
    float   deltaAngle;

    const IhullTypeIGC* pht = pship->GetHullType();

    assert (controls);
    controls->jsValues[c_axisRoll] = 0.0f;      //Ships never try to roll

    const Orientation&  myOrientation = pship->GetOrientation();
    double  cosTurn = myOrientation.CosForward(deltaTarget);

    if (cosTurn <= -0.999)
    {
        //Target is almost exactly behind, just yaw
        //(and assume the time increment is small enough
        //that yawing at max is appropriate).
        controls->jsValues[c_axisYaw] = 1.0f;
        controls->jsValues[c_axisPitch] = 0.0f;
        deltaAngle = pi;
    }
    else
    {
        float   yaw;
        float   pitch;

        if (cosTurn < 0.98)
        {
            //Target is somewhere other than in directly in front
            deltaAngle = (float)acos(cosTurn);

            //Find the vector we'd like to rotate about
            Vector  twist = CrossProduct(myOrientation.GetBackward(), deltaTarget).Normalize();
            yaw = -(twist * myOrientation.GetUp()) * deltaAngle;
            pitch = (twist * myOrientation.GetRight()) * deltaAngle;
		}
        else
        {
            //The target is almost directly in front of us (within 11 degrees or so)
            //which would make getting the twist axis above a little dicey
            //So ... instead ... get the yaw and pitch off of the angles with the right * up
            yaw   = acos(myOrientation.CosRight(deltaTarget)) - 0.5f * pi;
            pitch = acos(myOrientation.CosUp(deltaTarget)) - 0.5f * pi;

			// mmf
			//{
			//	float check = yaw * yaw + pitch * pitch;
			//	if (check != check) debugf("common.cpp yaw * yaw + pitch * pitch is a nan\n");
			//	if (check < 0.0f) debugf("common.cpp yaw * yaw + pitch * pitch is a negative about to sqrt it\n");
			//}

            deltaAngle = (float)sqrt(yaw * yaw + pitch * pitch);
        }

        //Adjust the yaw and pitch by the amount we are going to drift (due to our current
        //turning rates).
        {
            float   tm = pship->GetTorqueMultiplier();

            float   mass = pship->GetMass();
            assert (mass > 0.0f);
            {
                float   yawRate = pship->GetCurrentTurnRate(c_axisYaw);
                yaw -= (float)(skill * fabs(yawRate) * (0.5f * yawRate * mass / (tm * pht->GetTurnTorque(c_axisYaw))));
            }
            {
                float   pitchRate = pship->GetCurrentTurnRate(c_axisPitch);
                pitch -= (float)(skill * fabs(pitchRate) * (0.5f * pitchRate * mass / (tm * pht->GetTurnTorque(c_axisPitch))));
            }
        }

        //How do we want to set the controls so that we will turn to face the desired goal
        {
            float   maxYaw = dt * pht->GetMaxTurnRate(c_axisYaw);
            float   maxPitch = dt * pht->GetMaxTurnRate(c_axisPitch);

            float   y = yaw / maxYaw;
            float   p = pitch / maxPitch;

            float   d2 = (y * y + p * p);
            if (d2 > 1.0f)
            {
                float   f = (float)(1.0 / sqrt(d2));

                controls->jsValues[c_axisYaw] = y * f;
                controls->jsValues[c_axisPitch] = p * f;
            }
            else
            {
                controls->jsValues[c_axisYaw] = y;
                controls->jsValues[c_axisPitch] = p;
            }
        }
    }

    return deltaAngle;
}

bool  FindableModel(ImodelIGC*          m,
                    IsideIGC*           pside,
                    int                 ttMask,
                    AbilityBitMask      abmAbilities,
					int					iAllies)
{
    bool    okF = false;

	//You never target yourself or something marked as hidden
    ObjectType  type = m->GetObjectType();

    if (GetTypebits(type) & ttMask)
    {
        IsideIGC*  pHisSide = m->GetSide();

        int sidebits = (pHisSide == NULL)
                       ? c_ttNeutral
					   : ( (pside == pHisSide) || (IsideIGC::AlliedSides(pside,pHisSide) && iAllies == 1) // #ALLY Imago 7/31/09
                          ? c_ttFriendly
                          : c_ttEnemy );

		if (iAllies == 2)
			sidebits = (IsideIGC::AlliedSides(pside,pHisSide) && pside != pHisSide) //ALLY
				? c_ttFriendly
				: c_ttEnemy;


		bool notsameside = !(pside == pHisSide); // #ALLY

        if ((sidebits & ttMask) ||
            ((type == OT_probe) && ((abmAbilities & c_eabmRescueAny) != 0) &&
             ((IprobeIGC*)m)->GetProbeType()->HasCapability(c_eabmRescueAny)))
        {
            if (abmAbilities != 0)
            {
                switch (type)
                {
                    case OT_ship:
                    {
                        if (((IshipIGC*)m)->GetParentShip() == NULL)
                        {
                            IhullTypeIGC*   pht = ((IshipIGC*)m)->GetBaseHullType();
                            okF = pht && pht->HasCapability(abmAbilities);
                        }
                        else
                            okF = false;
                    }
                    break;

                    case OT_station:
                    {
						//#ALLY : exceptions for c_ttFriendly which include now allies
						//dont match allies if we're looking for a friendly station with one of the abilities below

						if (iAllies == 1) { //imago 7/31/09
							ImissionIGC*         pmission = pside->GetMission();
						    const MissionParams* pmp = pmission->GetMissionParams();

							AbilityBitMask alliesnotallowed = (pmp->bAllowAlliedRip) ?
								c_sabmRestart | c_sabmUnload :
								c_sabmRestart | c_sabmUnload | c_sabmRipcord;  //imago 7/9/09 c_sabmTeleportUnload removed 7/23/09

							if ((ttMask & c_ttFriendly) &&
								(abmAbilities & alliesnotallowed) &&
								notsameside)
								break;
						}
						okF = ((IstationIGC*)m)->GetStationType()->HasCapability(abmAbilities);
                    }
                    break;

                    case OT_asteroid:
                    {
                        okF = ((IasteroidIGC*)m)->HasCapability(abmAbilities);
                    }
                    break;

                    case OT_treasure:
                    {
                        okF = ((ItreasureIGC*)m)->GetTreasureCode() == c_tcFlag;
                    }
                    break;

                    case OT_probe:
                    {
                        okF = ((IprobeIGC*)m)->GetProbeType()->HasCapability(abmAbilities);
                    }
                    break;

                    default:
                        assert (false);
                }
            }
            else
                okF = ((type != OT_ship) || (((IshipIGC*)m)->GetParentShip() == NULL));
        }
    }

    return okF;
}

static bool IsFriendlyCluster(IclusterIGC*  pcluster, IsideIGC* pside)
{
    StationLinkIGC* psl = pcluster->GetStations()->first();
    if (psl == NULL)
        return false;                   //No stations == unfriendly

    bool    rc = false;
    do
    {
        IstationIGC*    ps = psl->data();
        if ((!ps->GetStationType()->HasCapability(c_sabmPedestal)) &&
            ps->SeenBySide(pside))
        {
			if ((pside != ps->GetSide()) && !IsideIGC::AlliedSides(pside, ps->GetSide()))		// #ALLY FIXED 7/10/09 imago - was: pside != ps->GetSide(
                return false;               //enemy has a station == unfriendly

            rc = true;
        }

        psl = psl->next();
    }
    while (psl != NULL);
	//It has stations but no enemy stations ... therefore at least one friendly station

	// mmf 10/07 controversial change, enable it with Experimental game type
	ImissionIGC*         pmission = pside->GetMission();  // mmf 10/07 added so we can get at bExperimental game type
    const MissionParams* pmp = pmission->GetMissionParams(); // mmf 10/07

	if (!(pmp->bExperimental)) {
	  return rc; // mmf 10/07 orig code
	}
    // mmf else if Experimental game type fall through to yp's code
	// yp: Improving AI: no reason to check further if we already know its a hostile sector
	if(rc == false)
		return rc;
	// we should also check to see if there is a lot of enemy in the sector.
	// we wouldnt want to go somewhere hostile even if we do have a base there.
	if(pcluster->GetShips() != NULL)
	{
		int friendlyShipCount = 0;

		for (ShipLinkIGC*   psl = pcluster->GetShips()->first(); (psl != NULL); psl = psl->next()) // mmf changed this to pcluste->GetShips from pside
        {
            IshipIGC*   pship = psl->data();
			// If our team knows that ship is there or its one of our ships, then we can count it.
            if (pship->SeenBySide(pside) || pship->GetSide() == pside)
			{
				//if (pside != pship->GetSide()) // if its not our side then we subtract 1 from our count
				if ((pside != pship->GetSide()) && !IsideIGC::AlliedSides(pside,pship->GetSide())) //#ALLY -was: line above IMAGO FIXED LIKE THIS ALL OVER 7/8/09
				{// count hostiles in the system.
					// TODO: Make smarter: Assign differnt ship hulls a differnt amount of points, could also handle drones differntly
					friendlyShipCount--;
				}
				else//, otherwise we increment it.
				{// count friendlys in the system.
					friendlyShipCount++;
				}
			}
        }

		if(friendlyShipCount>=0)// do we have a good chance of being safe?
		{
			rc = true; // to do this...
		}
		else
		{
			rc = false;
		}
	}
    // yp end

    return rc;
}

struct  ClusterPosition
{
    IclusterIGC*    pcluster;
    const Vector*   pposition;
};
typedef Slist_utl<ClusterPosition> CPList;
typedef Slink_utl<ClusterPosition> CPLink;

static bool    UniqueCP(CPList*    cpl, IclusterIGC*   pc)
{
    for (CPLink*    l = cpl->first(); (l != NULL); l = l->next())
    {
        if (l->data().pcluster == pc)
            return false;
    }

    return true;
}
static void    NewCP(CPList*   cpl, IclusterIGC*   pc, const Vector* pposition)
{
    CPLink* l = new CPLink;
    l->data().pcluster = pc;
    l->data().pposition = pposition;

    cpl->last(l);
}

ImodelIGC*  FindTarget(IshipIGC*           pship,
                       int                 ttMask,
                       ImodelIGC*          pmodelCurrent,
                       IclusterIGC*        pcluster,
                       const Vector*       pposition,
                       const Orientation*  porientation,
                       AbilityBitMask      abmAbilities,
                       int                 maxDistance,
					   int				   iAllies) //Imago 8/1/09
{
    if (!pcluster)
        pcluster = pship->GetCluster();
    if (!pcluster)
        return NULL;

    IsideIGC*   pside = pship ? pship->GetSide() : NULL;

    if (pmodelCurrent && ((pmodelCurrent == pship) ||
                          (!FindableModel(pmodelCurrent, pside, ttMask, abmAbilities,iAllies)) ||
                          (!pship->CanSee(pmodelCurrent))))
        pmodelCurrent = NULL;

    if (((ttMask & (c_ttFront | c_ttNearest)) == 0) && !pmodelCurrent && (pship || pposition))
        ttMask |= c_ttNearest;

    //This is a hack (which works only because the lists are lists of interface pointers)
    //so that, if we are only looking for a single type of target, we only search the list
    //of those types of targets.
    const ModelListIGC*   models = ((ttMask & c_ttAllTypes) == c_ttShip)
                                    ? (const ModelListIGC*)(pcluster->GetShips())
                                    : (((ttMask & c_ttAllTypes) == c_ttStation)
                                       ? (const ModelListIGC*)(pcluster->GetStations())
                                       : (((ttMask & c_ttAllTypes) == c_ttWarp)
                                          ? (const ModelListIGC*)(pcluster->GetWarps())
                                          : ((ttMask & c_ttAllTypes) == c_ttTreasure)
                                            ? (const ModelListIGC*)(pcluster->GetTreasures())
                                            : pcluster->GetPickableModels()));
    assert (models);
	ImodelIGC*      pmodelTarget = NULL;
    if (models->n() != 0)
    {
        int    ttBest = (ttMask & (c_ttFront | c_ttNearest | c_ttLeastTargeted));
        if (ttMask & (c_ttFront | c_ttNearest))
        {
            if (!pposition)
                pposition = &(pship->GetPosition());

            if ((ttMask & c_ttFront) && !porientation)
                porientation = &(pship->GetOrientation());
        }

        ModelLinkIGC*    mLink = models->first();
        if (pmodelCurrent)
        {
            //Find the link (in the model list) that corresponds to the current target
            while ((mLink != NULL) && (mLink->data() != pmodelCurrent))
                mLink = mLink->next();

            // mLink = mLink && (mLink != models->last()) ? mLink->next() : models->first();
            if (ttMask&c_ttPrevious)
                mLink = (mLink && (mLink != models->first())) ? mLink->txen() : models->last();
            else
                mLink = (mLink && (mLink != models->last())) ? mLink->next() : models->first();
        }
        assert (mLink);
        float   capacity;
        if ((abmAbilities & c_aabmMineHe3) != 0)
            capacity = pside->GetMission()->GetFloatConstant(c_fcidCapacityHe3);

        //Search through the entire container list, starting at the link beyond mLink, wrapping around
        //until back at mLink for a ship or station hub in the same sector as the ship.
        int             nTargeting = 0x7fff; //surely we can do better than this
        float           distance = FLT_MAX;

        ModelLinkIGC*   l = mLink;
        do
        {
            ImodelIGC*  m = l->data();

			//You never target yourself or something marked as hidden
            if ((m != pship) && ((!pship) || pship->CanSee(m)) && FindableModel(m, pside, ttMask, abmAbilities,iAllies))
            {
                if (ttBest)
                {
                    int     n = 0;
                    float   d;
                    if (ttMask & (c_ttFront | c_ttNearest))
                    {
                        //intentionally backwards so low cosforwards == in front
                        Vector  dp = (*pposition) - m->GetPosition();
                        d = (ttMask & c_ttFront)
                            ? porientation->CosForward2(dp)
                            : dp.LengthSquared();
                    }

                    bool    bReplace;
                    if (ttMask & c_ttLeastTargeted)
                    {
                        assert ((ttMask & c_ttAllTypes) == c_ttAsteroid);
                        assert (m->GetObjectType() == OT_asteroid);


                        n = 0;
                        {
                            //Count the number of drones on this side and allied sides building or mining this asteroid  (ALLY ROCK FIX)
                            for (ShipLinkIGC*   psl = pside->GetMission()->GetShips()->first(); (psl != NULL); psl = psl->next()) //imago changed mission wide not side wide
                            {
                                IshipIGC*   ps = psl->data();
                                if (ps->GetPilotType() < c_ptPlayer)
                                {
									if ((ps->GetSide() == pside || pside->AlliedSides(pside,ps->GetSide()))) {  //imago added extra if
                                    	if ((ps->GetCommandTarget(c_cmdAccepted) == m) &&
                                        	(ps->GetCommandID(c_cmdAccepted) >= c_cidMine))
                                    	{
                                        	n++;
                                    	}
									}
                                }
                            }
                        }

                        bReplace = ((n < nTargeting) || ((n == nTargeting) && (d < distance)));
                        if (bReplace && ((abmAbilities & c_aabmMineHe3) != 0))
                        {
                            float   ore = ((IasteroidIGC*)m)->GetOre() - float(n) * capacity;

                            if (ore <= capacity * 0.25f)
                                bReplace = false;
                        }
                    }
                    else
                        bReplace = (d < distance);


                    if (bReplace)
                    {
                        nTargeting = n;
                        distance = d;

                        //Don't replace a valid (if less optimal) target with the
                        //current one unless there is no target (pmodelCurrent is
                        //always checked last).
                        if ((m != pmodelCurrent) || (pmodelTarget == NULL))
                            pmodelTarget = m;
                    }
                }
                else
                {
                    //and that is all we need ...
                    pmodelTarget = m;
                    break;
                }
            }

            if (ttMask & c_ttPrevious)
                l = (l == models->first()) ? models->last() : l->txen();
            else
                l = (l == models->last()) ? models->first() : l->next();
        }
        while (l != mLink);
    }

    if ((pmodelTarget) ||
        ((ttMask & c_ttAnyCluster) == 0) ||
        ((ttMask & (c_ttStation | c_ttAsteroid | c_ttTreasure | c_ttWarp)) == 0) ||
        (maxDistance == 0))
        return pmodelTarget;

    IclusterIGC*    pclusterStart = pcluster;
    int             distance = 0;

    {
        //Search adjacent clusters for an appropriate target
        WarpListIGC     warpsOne;
        WarpListIGC     warpsTwo;
        ClusterListIGC  clustersVisited;

        WarpListIGC* pwlOneAway = &warpsOne;
        WarpListIGC* pwlTwoAway = &warpsTwo;

        clustersVisited.first(pcluster);    //We've already visited this cluster

        //None of these bits make any sense when searching a remote cluster
        ttMask &= ~(c_ttAnyCluster | c_ttFront | c_ttShip | c_ttBuoy | c_ttMissile);

        bool    firstMatchF = false;
        do
        {
            assert (pcluster);

            //Push the destinations of the warps in pcluster onto the end the list of
            //warps that are an extra jump away
            {
                for (WarpLinkIGC*   l = pcluster->GetWarps()->first(); (l != NULL); l = l->next())
                {
                    IwarpIGC*   w = l->data();
                    if ((!pship) || pship->CanSee(w))
                    {
                        IwarpIGC*       pwarpDestination = w->GetDestination();
                        if (pwarpDestination)
                        {
                            IclusterIGC*    pclusterOther = pwarpDestination->GetCluster();

                            //Have we visited pclusterOther?
                            if (clustersVisited.find(pclusterOther) == NULL)
                            {
                                //No
                                if (((ttMask & c_ttCowardly) == 0) || IsFriendlyCluster(pclusterOther, pside))
                                    pwlTwoAway->last(pwarpDestination);
                            }
                        }
                    }
                }
            }

            //Find the next cluster to search
            if (pwlOneAway->n() == 0)
            {
                if ((pwlTwoAway->n() == 0) || (distance++ >= maxDistance))
                {
                    //No place left to search
                    break;
                }
                else
                {
                    //No clusters in the current distance bracket ... start on the clusters in the next distance bracket
                    WarpListIGC* pwl = pwlOneAway;
                    pwlOneAway = pwlTwoAway;
                    pwlTwoAway = pwl;
                }
            }

            assert (pwlOneAway->n() > 0);
            WarpLinkIGC*    plink = (ttMask & c_ttPrevious) ? pwlOneAway->last() : pwlOneAway->first();
            IwarpIGC*       pwarp = plink->data();

            delete plink;

            pposition = &(pwarp->GetPosition());

            pcluster = pwarp->GetCluster();
            clustersVisited.first(pcluster);

            pmodelTarget = FindTarget(pship, ttMask, NULL, pcluster, pposition, NULL, abmAbilities);

            //Skip over the existing target once
            if ((pmodelTarget != NULL) &&
                (pmodelTarget == pmodelCurrent) &&
                firstMatchF)
            {
                pmodelTarget = NULL;
                firstMatchF = false;
            }
        }
        while (pmodelTarget == NULL);
    }

    if (!pship || (ttMask & c_ttNoRipcord) ||
        (pship->GetBaseHullType() == NULL) ||
        (pship->GetFlag() != NA))
    {
        return pmodelTarget;
    }

    const IhullTypeIGC* pht = pship->GetHullType();
    HullAbilityBitMask  habmShip = pht->GetCapabilities();
    if (habmShip & c_habmNoRipcord)
        return pmodelTarget;

    float   ripcordSpeed = pht->GetRipcordSpeed();

    //OK ... the ship can rip. See if there is something "closer" via a ripcord.
    //First, make a list of all clusters that contain a ripcord
    CPList  clustersRipcord;
	ImissionIGC*         pmission = pside->GetMission();  // mmf/Imago 7/8/09 ALLY
	const MissionParams* pmp = pmission->GetMissionParams(); //

    {
		if (pmp->bAllowAlliedRip) { //if allied record allowed game ALLY RIPCORD
	        for (StationLinkIGC*    psl = pmission->GetStations()->first(); (psl != NULL); psl = psl->next())  //ALLY: pmission instead of pside
	        {
	            IstationIGC*    ps = psl->data();
	            if (ps->GetStationType()->HasCapability(c_sabmRipcord) &&
					(ps->GetSide()->AlliedSides(pside,ps->GetSide()) || pside->GetObjectID() == ps->GetSide()->GetObjectID()) )  //ALLY imago 7/8/09
	            {
					if (ps->SeenBySide(pside) || pmp->bAllowAlliedViz) { //Imago VISIBILITY RIPCORD 7/10/09 ALLY
	                	IclusterIGC*    pc = ps->GetCluster();
	                	if ((pc != pclusterStart) && UniqueCP(&clustersRipcord, pc) &&
	                    	(((ttMask & c_ttCowardly) == 0) || IsFriendlyCluster(pc, pside)))
	                    	NewCP(&clustersRipcord, ps->GetCluster(), &(ps->GetPosition()));
					}
	            }
	        }
		} else { //regular situation, a no allies ripcord game
	        for (StationLinkIGC*    psl = pside->GetStations()->first(); (psl != NULL); psl = psl->next())
	        {
	            IstationIGC*    ps = psl->data();
	            if (ps->GetStationType()->HasCapability(c_sabmRipcord))
	            {
	                IclusterIGC*    pc = ps->GetCluster();
	                if ((pc != pclusterStart) && UniqueCP(&clustersRipcord, pc) &&
	                    (((ttMask & c_ttCowardly) == 0) || IsFriendlyCluster(pc, pside)))
	                    NewCP(&clustersRipcord, ps->GetCluster(), &(ps->GetPosition()));
	            }
	        }
		}
    }

    if (pship->GetPilotType() >= c_ptPlayer)   //non-players don't tp to probes or ships
    {
        HullAbilityBitMask  habm = (habmShip & c_habmCanLtRipcord)
                                   ? (c_habmIsRipcordTarget | c_habmIsLtRipcordTarget)
                                   : c_habmIsRipcordTarget;

        ImissionIGC*        pmission = pship->GetMission();
        IIgcSite*           pigc = pmission->GetIgcSite();

		if (pmp->bAllowAlliedRip) { //if allied ripcord allowed game
	        for (ShipLinkIGC*    psl = pmission->GetShips()->first(); (psl != NULL); psl = psl->next())  //ALLY 7/8/09 was pside iterator
	        {
	            IshipIGC*    ps = psl->data();
	            if (ps != pship &&
					(ps->GetSide()->AlliedSides(pside,ps->GetSide()) || pside->GetObjectID() == ps->GetSide()->GetObjectID()) )  //ALLY imago 7/8/09
	            {
					if (ps->SeenBySide(pside) || pmp->bAllowAlliedViz) { //Imago VISIBILITY RIPCORD 7/10/09 ALLY
	                	IclusterIGC*    pc = pigc->GetRipcordCluster(ps, habm);
	                	if (pc && (pc != pclusterStart) && UniqueCP(&clustersRipcord, pc) &&
	                    	(((ttMask & c_ttCowardly) == 0) || IsFriendlyCluster(pc, pside)))
	                    	NewCP(&clustersRipcord, pc, NULL);
					}
	            }
	        }
		} else { //normal no allied ripcord
	        for (ShipLinkIGC*    psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
	        {
	            IshipIGC*    ps = psl->data();
	            if (ps != pship && pside->GetObjectID() == ps->GetSide()->GetObjectID())  //ALLY imago 7/8/09
	            {
	                IclusterIGC*    pc = pigc->GetRipcordCluster(ps, habm);
	                if (pc && (pc != pclusterStart) && UniqueCP(&clustersRipcord, pc) &&
	                    (((ttMask & c_ttCowardly) == 0) || IsFriendlyCluster(pc, pside)))
	                    NewCP(&clustersRipcord, pc, NULL);
	            }
	        }
		}


        for (ClusterLinkIGC*    pcl = pmission->GetClusters()->first(); (pcl != NULL); pcl = pcl->next())
        {
            IclusterIGC*    pc = pcl->data();
            if ((pc != pclusterStart) && UniqueCP(&clustersRipcord, pc) &&
                (((ttMask & c_ttCowardly) == 0) || IsFriendlyCluster(pc, pside)))
            {
                for (ProbeLinkIGC*  ppl = pc->GetProbes()->first(); (ppl != NULL); ppl = ppl->next())
                {
                    IprobeIGC*  pprobe = ppl->data();
					if (pmp->bAllowAlliedRip) { //if allied record allowed game
                    	if ((pprobe->GetSide() == pside || pprobe->GetSide()->AlliedSides(pside,pprobe->GetSide())) && pprobe->GetCanRipcord(ripcordSpeed)) //ALLY imago 7/8/09
                    	{
                        	NewCP(&clustersRipcord, pc, &(pprobe->GetPosition()));
                        	break;
						}
					} else {
                    	if ((pprobe->GetSide() == pside) && pprobe->GetCanRipcord(ripcordSpeed)) //NO ripcord imago 7/8/09
                    	{
                        	NewCP(&clustersRipcord, pc, &(pprobe->GetPosition()));
                        	break;
                    	}
					}
                }
            }
        }
    }

    //Now ... check each cluster to see if there is something closer
    ttMask |= c_ttNoRipcord | c_ttAnyCluster;

    distance--;
    int distanceBest = 0x7fffffff;

    for (CPLink*    pcl = clustersRipcord.first(); (pcl != NULL); pcl = pcl->next())
    {
        ImodelIGC*  pmodel = FindTarget(pship, ttMask, NULL, pcl->data().pcluster, pcl->data().pposition, NULL, abmAbilities, distance);
        if (pmodel)
        {
            IclusterIGC*    pc = pmodel->GetCluster();
            assert (pc);
            int d = GetDistance(pship, pclusterStart, pc, distanceBest);
            if (d < distanceBest)
            {
                distanceBest = d;
                pmodelTarget = pmodel;
            }
        }
    }

    return pmodelTarget;
}

struct Path
{
    IwarpIGC*   pwarpStart;
    IwarpIGC*   pwarp;
    float       distance;
};
typedef Slist_utl<Path> PathList;
typedef Slink_utl<Path> PathLink;

IwarpIGC* FindPath(IshipIGC*    pship,
                   IclusterIGC* pclusterTarget,
                   bool         bCowardly)
{
    assert (pship);
    IsideIGC*   pside = pship->GetSide();

    IclusterIGC*    pclusterCurrent = pship->GetCluster();
    assert (pclusterCurrent);

    if (pclusterCurrent == pclusterTarget)
        return NULL;

    const Vector&   positionShip = pship->GetPosition();

    ClusterListIGC  explored;
    PathList        unexplored;

    explored.last(pclusterCurrent);

    {
        //Add the initial warps ... distance from the player to the aleph
        const WarpListIGC*  pwarps = pclusterCurrent->GetWarps();
        for (WarpLinkIGC*   wLink = pwarps->first(); (wLink != NULL); wLink = wLink->next())
        {
            IwarpIGC*   pwarp = wLink->data();
            if (pship->CanSee(pwarp))
            {
                assert (pwarp->GetDestination());
                IclusterIGC*    pclusterDestination = pwarp->GetDestination()->GetCluster();
                if ((!bCowardly) ||
                    (pclusterTarget == pclusterDestination) ||
                    IsFriendlyCluster(pclusterDestination, pside))
                {
                    PathLink*   pl = new PathLink;
                    assert (pl);

                    Path&       path = pl->data();
                    path.distance = (pwarp->GetPosition() - positionShip).LengthSquared();
                    path.pwarpStart = path.pwarp = pwarp;

                    //Keep the list sorted
                    PathLink*   p = unexplored.first();
                    while (true)
                    {
                        if (p == NULL)
                        {
                            //Nothing left ... to the end of the list
                            unexplored.last(pl);
                            break;
                        }
                        else if (path.distance < p->data().distance)
                        {
                            //Insert in front of the existing link (which has a greater distance)
                            p->txen(pl);
                            break;
                        }

                        p = p->next();
                    }
                }
            }
        }

        if (unexplored.n() == 0)
            return NULL;
    }

    while (true)
    {
        PathLink*   plinkClosest = unexplored.first();

        if (!plinkClosest)
            return NULL;        //Never found a path

        const Path& path = plinkClosest->data();
        IwarpIGC*   pwarp = path.pwarp;

        IclusterIGC*    pclusterNext = pwarp->GetDestination()->GetCluster();
        if (pclusterNext == pclusterTarget)
        {
            //Found a path to the target
            return path.pwarpStart;
        }

        explored.last(pclusterNext);

        //Add warps for the warp in the new cluster (that do not lead to a previously explored cluster)
        //Add all of the warps in this cluster to the unexplored list
        for (WarpLinkIGC*   pwl = pclusterNext->GetWarps()->first(); (pwl != NULL); pwl = pwl->next())
        {
            IwarpIGC*   pwarp = pwl->data();

            if (pship->CanSee(pwarp))
            {
                IclusterIGC*    pclusterDestination = pwarp->GetDestination()->GetCluster();

                if ((!bCowardly) ||
                    (pclusterDestination == pclusterTarget) ||
                    IsFriendlyCluster(pclusterDestination, pside))
                {
                    if (explored.find(pclusterDestination) == NULL)
                    {
                        PathLink*   ppl = new PathLink;
                        assert (ppl);

                        Path&       ppathNew = ppl->data();
                        ppathNew.pwarpStart = path.pwarpStart;
                        ppathNew.pwarp = pwarp;

                        unexplored.last(ppl);
                    }
                }
            }
        }

        delete plinkClosest;
    }
}

IwarpIGC* FindPath(IshipIGC*  pShip,
                   ImodelIGC* pTarget,
                   bool       bCowardly)
{
    assert (pShip);
    assert (pTarget);

    IclusterIGC*    pclusterTarget = pTarget->GetMission()->GetIgcSite()->GetCluster(pShip, pTarget);

    return pclusterTarget ? FindPath(pShip, pclusterTarget, bCowardly) : NULL;
}

bool    SearchClusters(ImodelIGC*    pmodel,
                       void*         pdata,
                       bool          (*pfnCluster)(IclusterIGC*  pcluster,
                                                   void*         pdata))
{
    assert (pmodel);

    bool    rc = false; // mmf initialize to false
    IclusterIGC*    pcluster = pmodel->GetCluster();

    if (pcluster)
    {
        if (pfnCluster(pcluster, pdata))
        {
            //That was easy: success on the first cluster checked
            rc = true;
        }
        else
        {
        }
    }
    else
        rc = false;

    return rc;
}

const char* GetModelType(ImodelIGC* pmodel)
{
    switch (pmodel->GetObjectType())
    {
        case OT_ship:
            return ((IshipIGC*)pmodel)->GetHullType()->GetName();
        case OT_projectile:
            return "";
            return "minefield";
        case OT_station:
            return ((IstationIGC*)pmodel)->GetStationType()->GetName();
        case OT_buoy:
            return "";
        case OT_asteroid:
            return IasteroidIGC::GetTypeName(((IasteroidIGC*)pmodel)->GetCapabilities());
        case OT_warp:
            return "aleph";
        case OT_treasure:
        {
            IbuyableIGC*    pb = ((ItreasureIGC*)pmodel)->GetBuyable();
            if (pb)
                return pb->GetName();
            else
            {
                static const char*  szNames[] = {"", "", "Powerup", "", "Cash", ""};
                TreasureCode tc = ((ItreasureIGC*)pmodel)->GetTreasureCode();
                assert ((tc == c_tcPowerup) || (tc == c_tcCash) || (tc == c_tcFlag));
                return szNames[tc];
            }
        }

        case OT_mine:
        case OT_probe:
            return pmodel->GetName() + 1;       //Special hack where probes & mines always have a secondary name

        default:
        {
            assert (pmodel->GetObjectType() == OT_missile);
            return ((ImissileIGC*)pmodel)->GetMissileType()->GetName();
        }
    }
}

const char* GetModelName(ImodelIGC* pmodel)
{
    assert (pmodel);
    const char* n = pmodel->GetName();

    if (n[0] != '\0')
        return n;
    else
    {
        ObjectType  type = pmodel->GetObjectType();
        return ((type == OT_asteroid) || (type == OT_buoy)) ? (n + 1) : GetModelType(pmodel);
    }
}

AmmoState   GetAmmoState(IshipIGC*  pship)
{
    assert (pship);

    //speed hack ... assume that 500 rounds is enough for anything without checking guns
    short   ammo = pship->GetAmmo();
    if (ammo >= 500)
        return c_asFull;

    float   consumption = 0.0f;

    //Go through the ship's mounted weapons and count up the number of bullets it needs/second
    for (Mount i = pship->GetHullType()->GetMaxWeapons() - 1; (i >= 0); i--)
    {
        IweaponIGC* w = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
        if (w)
        {
            short   aps = w->GetAmmoPerShot();
            ammo -= aps;
            consumption += ((float)aps) / w->GetDtBurst();

        }
    }

    return (consumption == 0.0f)
           ? c_asFull
           : ((ammo <= 0)
              ? c_asEmpty
              : (((float)ammo) / consumption) < 10.0f ? c_asLow : c_asFull);
}

static const GotoPositionMask c_gpmKillThrottle = 0x01;
static const GotoPositionMask c_gpmFinished     = 0x02;
static const GotoPositionMask c_gpmPivot        = 0x04;
static const GotoPositionMask c_gpmEnter        = 0x08;
static const GotoPositionMask c_gpmNoDodge      = 0x10;
static const GotoPositionMask c_gpmFast         = 0x40;
static const GotoPositionMask c_gpmDodgeShips   = 0x80;
static const GotoPositionMask c_gpmRoll         = 0x100;

static const float c_fOffsetFudge = 10.0f;

static const char   c_stateSeek = 0;
static const char   c_stateCoast = 1;
static const char   c_statePivot = 2;
static const char   c_stateEnter = 3;

GotoPositionMask Waypoint::DoApproach(IshipIGC*        pship,
                                      const Vector&    myPosition,
                                      const Vector&    itsPosition,
                                      int              nLand,
                                      const Vector*    pCenters,
                                      const Vector*    pDirections,
                                      float            distanceRest,
                                      const Vector&    positionRest,
                                      Vector*          pvectorGoto,
                                      ImodelIGC**      ppmodelSkip,
                                      Vector*          pvectorFacing)
{
    GotoPositionMask    gpm;

    assert (nLand > 0);

    //The best bay is the one where the distance between our rest position and the approach strip is the smallest
    Vector  dpNow = myPosition - itsPosition;
    Vector  dpRest = positionRest - itsPosition;
    float   myRadius = (pship->GetRadius() + m_pmodelTarget->GetRadius() + 10.0f);   //my radius plus a fudge factor

    const Vector* pdirectionBest;
    const Vector* pcenterBest;
    Vector  goalBest;
    float   error2Best = FLT_MAX;
    float   offset2Best;
    int     bayBest;
	ImissionIGC*         pmission = pship->GetMission();  // mmf 10/07 added so we can get at bExperimental game type
    const MissionParams* pmp = pmission->GetMissionParams(); // mmf 10/07

    for (int i = 0; (i < nLand); i++)
    {
        const Vector* pcenter = pCenters + i;
        const Vector* pdirection = pDirections + i;

        //This is where we want to go
        Vector  goal;
        {
            //Find t such that
            //|*pcenter + *pdirection * t| = R

            //(pc  + pd * t)^2 = R^2

            // pc^2 + 2pd pc t + pd^2 t^2 - r^2 = 0

            assert (pdirection->LengthSquared() >= 0.98f);
            assert (pdirection->LengthSquared() <= 1.02f);
            float   b = *pcenter * *pdirection;
            float   c = *pcenter * *pcenter - myRadius * myRadius;
            assert (c < 0.0f);

			// mmf added check for negative
			// revisit what to set t to in this case splat

			float t=0.1f;
			if ((b*b - c) < 0.0f) {
				debugf("common.cpp b*b-c is less than zero about to sqrt it, set t to 0.1f\n");
			} else { t = sqrt(b*b - c) - b; }

            // float   t = sqrt(b*b - c) - b;
            assert (t >= 0.0f);

            goal = *pcenter + *pdirection * t;
        }

        //Look at where both our current position and reset positions are with respect to the line
        Vector  deltaNow = (dpNow - *pcenter);
        float   dotNow = deltaNow * *pdirection;

        Vector  deltaRest = (dpRest - *pcenter);
        float   dotRest = (deltaRest * *pdirection);

        float   error2;
        float   offset2;
        if ((dotNow >= 0.0f) || (dotRest >= myRadius))
        {
            //Ship is infront of the bay ... error is distance between
            //respostion and approach line
            error2  = deltaRest.LengthSquared() - dotRest * dotRest;
            offset2 = deltaNow.LengthSquared()  - dotNow * dotNow;
        }
        else
        {
            //Ship is behind (or not far enough in front) ... error is
            //distance from rest position and approach point
            error2 = (dpRest - goal).LengthSquared();
            offset2 = FLT_MAX;
        }

        if (error2 < error2Best)
        {
            pdirectionBest = pdirection;
            pcenterBest = pcenter;
            goalBest = goal;
            error2Best = error2;
            offset2Best = offset2;
            bayBest = i;
        }
    }

    if (error2Best > 36.0f)
    {
        //Not able to drift into position .. continue normally
        *ppmodelSkip = NULL;
        *pvectorGoto = goalBest + itsPosition;

        float   d2 = (dpNow - goalBest).LengthSquared();
        if ((d2 > distanceRest * distanceRest * 2.0f) ||
            ((dpRest - goalBest).LengthSquared() <= d2 + 1.0f))
        {
            //We are moving in more or less the correct direction
            gpm = ((dpNow - *pcenterBest) * *pdirectionBest >= 0.0f) ? c_gpmDodgeShips : 0;
        }
        else
        {
            //Our rest position is further from the goal than our current position
            *pvectorFacing = *pvectorGoto - myPosition;
            gpm = c_gpmPivot;
        }
    }
    else
    {
        //Our rest position is close to the line ... are we close to the line of approach?
        if (offset2Best > 40.0f)
        {
            //We are not close to the approach line ... coast in
            gpm = c_gpmKillThrottle | c_gpmDodgeShips;

            *ppmodelSkip = NULL;
            *pvectorGoto = goalBest + itsPosition;
        }
        else
        {
            //We are on the line and either not moving or moving along the line
            *ppmodelSkip = m_pmodelTarget;
            *pvectorGoto = *pcenterBest + itsPosition;
            *pvectorFacing = *pvectorGoto - myPosition;

            float rateYaw = pship->GetCurrentTurnRate(c_axisYaw);
            float ratePitch = pship->GetCurrentTurnRate(c_axisPitch);
            float rate2 = rateYaw * rateYaw + ratePitch * ratePitch;
            float rateMax = 0.01f; // mmf 10/07 orig code had this as a static const

			// mmf 10/07 controversial change, enable it with Experimental game type
			if (pmp->bExperimental)
              rateMax = 0.10f; // yp: was 0.01f but we dont need to be going THAT Slow. We want to get in this thing!

            static const float cosMin = 0.999f * 0.999f;
            if ((rate2 < rateMax * rateMax) && (pship->GetOrientation().CosForward2(*pvectorFacing) > cosMin))
            {
                //On track, going the right way and not turning .... charge
                gpm = c_gpmEnter | c_gpmNoDodge;
            }
            else
            {
                //On track and almost stopped ... pivot to face the bay
                // gpm = c_gpmPivot | c_gpmDodgeShips; // original
				// gpm = c_gpmPivot; yp: We just want to pivot and get in there, we dont want to worry about trying to dodge anytying.
				gpm = c_gpmPivot | c_gpmNoDodge; // 04/08 mmf added | c_gpmNoDodge
				                                 // yp's change resulted in cons dodging missiles on pivot which was undesirable
            }
        }
    }

    return gpm;
}

GotoPositionMask Waypoint::GetGotoPosition(IshipIGC*           pship,
                                           float               distanceRest,
                                           const Vector&       positionRest,
                                           Vector*             pvectorGoto,
                                           ImodelIGC**         ppmodelSkip,
                                           Vector*             pvectorFacing)
{
    assert (pship);

    const Vector&       myPosition = pship->GetPosition();

    GotoPositionMask    gpm;

    assert (m_pmodelTarget);
    const Vector&   itsPosition = m_pmodelTarget->GetPosition();

    if (m_objective == Waypoint::c_oGoto)
    {
        Vector  vRest = itsPosition - positionRest;
        float   dRest2 = vRest.LengthSquared();

        Vector  vCenters = itsPosition - myPosition;
        float   dCenters2 = vCenters.LengthSquared();

        float   radius = m_pmodelTarget->GetRadius() + pship->GetRadius();

        //First ... is there a danger of colliding with the object?
        float   radiusRest = radius + distanceRest;
        if (dCenters2 <= radiusRest * radiusRest)
        {
            //Yes there is ... pivot
            gpm = c_gpmPivot;
            *pvectorFacing = vCenters;
            *ppmodelSkip = NULL;
        }
        else
        {
            //No danger of a collision
            *ppmodelSkip = m_pmodelTarget;

            float   offset = radius + c_fOffsetFudge;
            if (dRest2 < offset * offset)
            {
                if (distanceRest < 0.5f)
                {
                    gpm = (c_gpmPivot | c_gpmFinished);
                    *pvectorFacing = vCenters;
                }
                else
                {
                    gpm = c_gpmKillThrottle;
                    *pvectorGoto = itsPosition;
                }
            }
            else
            {
                gpm = 0;
                *pvectorGoto = itsPosition;
            }
        }
    }
    else
    {
        switch (m_pmodelTarget->GetObjectType())
        {
            case OT_asteroid:
            {
                Vector  centers[2];
                Vector  directions[2];

				// yp: get an alignment position that is unique to this ship
				// this will greatly decrease the odds of two constructors tring to build on a asteroid from the same spot
				// could also allow two miners to mine the same asteroid, though that will
				// require a fix somewhere else as well.
				float uniquePosX = ((int)pship->GetObjectID() %10)/10;
				float uniquePosY = ((int)pship->GetObjectID() %9)/9;
				float uniquePosZ = ((int)pship->GetObjectID() %8)/8;
                centers[0].x = uniquePosX;
				centers[0].y = uniquePosY;
				centers[0].z = uniquePosZ;
                centers[1].x = uniquePosX;
				centers[1].y = uniquePosY;
				centers[1].z = uniquePosZ;
				// previously
				//centers[0].x = centers[0].y = centers[0].z = 0.0f;
				//centers[1].x = centers[1].y = centers[1].z = 0.0f;
				// yp end

                const Rotation& r = m_pmodelTarget->GetRotation();
                directions[0] = r.axis();
                directions[1] = -r.axis();

                gpm = DoApproach(pship, myPosition, itsPosition,
                                 2, centers, directions,
                                 distanceRest,
                                 positionRest,
                                 pvectorGoto,
                                 ppmodelSkip,
                                 pvectorFacing);

                if (gpm & c_gpmEnter)
                {
                    gpm |= c_gpmFinished;
                }
            }
            break;


            case OT_ship:
            {
				// mmf pod pickup
                //Trying to pick up a ship ... where will he be when we get there?
                Vector  direction;
                Vector  dp = itsPosition - myPosition;
                Vector  dv = m_pmodelTarget->GetVelocity();

                float   t = solveForImpact(dp, dv,
                                           pship->GetHullType()->GetMaxSpeed(), 0.0f, &direction);


                gpm = c_gpmFast;
                *pvectorGoto = itsPosition + t * dv;
                *ppmodelSkip = m_pmodelTarget;
            }
            break;

            case OT_probe:
            {
                gpm = c_gpmFast;
                *pvectorGoto = itsPosition;
                *ppmodelSkip = m_pmodelTarget;
            }
            break;

            case OT_station:
            {
                //Trying to dock ...
                IhullTypeIGC*   pht = pship->GetBaseHullType();
                assert (pht);
                const IstationTypeIGC*  pst = ((IstationIGC*)m_pmodelTarget)->GetStationType();

                {
                    int nLand = pht->HasCapability(c_habmFighter)
                                ? pst->GetLandSlots()
                                : pst->GetCapLandSlots();

                    if (nLand == 0)
                    {
                        //No landing bays ... power glide in
                        gpm = c_gpmFast;
                        *pvectorGoto = itsPosition;
                        *ppmodelSkip = m_pmodelTarget;
                    }
                    else
                    {
                        assert (nLand > 0);
                        assert (nLand <= c_maxLandSlots);

                        Vector  centers[c_maxLandSlots];
                        Vector  directions[c_maxLandSlots];

                        const Orientation&  itsOrientation = m_pmodelTarget->GetOrientation();
                        for (int i = 0; (i < nLand); i++)
                        {
                            centers[i] = pst->GetLandPosition(i, 0) * itsOrientation;
                            directions[i] = pst->GetLandDirection(i, 0) * itsOrientation;
                        }

                        gpm = DoApproach(pship, myPosition, itsPosition,
                                         nLand, centers, directions,
                                         distanceRest,
                                         positionRest,
                                         pvectorGoto,
                                         ppmodelSkip,
                                         pvectorFacing);

                        if (gpm & c_gpmEnter)
                        {
                            gpm |= c_gpmRoll;
                        }
                    }
                }
            }
            break;

            default:
            {
                gpm = c_gpmFast;
                *pvectorGoto = itsPosition;
                *ppmodelSkip = m_pmodelTarget;
            }
            break;
        }
    }

    return gpm;
}

/*-------------------------------------------------------------------------
 * Function: getDirection
 *-------------------------------------------------------------------------
 * Purpose:
 *	  Get the button control mask to thrust in a given direction
 */
static int  getDirection(const Vector&      dP,
                         const Orientation& orientation)
{
    float   z = dP * orientation.GetBackward();
    float   y = dP * orientation.GetUp();
    float   x = dP * orientation.GetRight();

    double  absX = fabs(x);
    double  absY = fabs(y);
    double  absZ = fabs(z);

    int stateX = (x >= 0.0f ? rightButtonIGC   : leftButtonIGC);
    int stateY = (y >= 0.0f ? upButtonIGC      : downButtonIGC);
    int stateZ = (z <= 0.0f ? forwardButtonIGC : backwardButtonIGC);

    const float c_fT = 2.0f;
    #define GetState(a,b,c)   (abs##a > c_fT * abs##b)                                              \
                              ? state##a                                                            \
                              : (abs##a > c_fT * abs##c)                                            \
                                ? (state##a | state##b)                                             \
                                : (stateX | stateY | stateZ)

    int state;
    if (absX > absY)
    {
        //x > y
        if (absY > absZ)
        {
            //x > y > z
            state = GetState(X, Y, Z);
        }
        else if (absX > absZ)
        {
            // x > z > y
            state = GetState(X, Z, Y);
        }
        else
        {
            //z > x > y
            state = GetState(Z, X, Y);
        }
    }
    else
    {
        //y > x
        if (absX > absZ)
        {
            //y > x > z
            state = GetState(Y, X, Z);
        }
        else if (absY > absZ)
        {
            // y > z > x
            state = GetState(Y, Z, X);
        }
        else
        {
            //z > y > x
            state = GetState(Z, Y, X);
        }
    }

    #undef GetState

    return state;
}

bool    Ignore(IshipIGC*   pship, ImodelIGC* pmodel)
{
    bool    ignore = false;
    IsideIGC*   mySide = pship->GetSide();
    IsideIGC*   hisSide = pmodel->GetSide();

    ObjectType  type = pmodel->GetObjectType();

    if (type == OT_ship)
    {
        IshipIGC*   pshipHim = (IshipIGC*)pmodel;
        IhullTypeIGC*   phtHim = pshipHim->GetBaseHullType();
        if (phtHim == NULL)
            ignore = true;
        else
        {
			if ((mySide == hisSide) || IsideIGC::AlliedSides(mySide, hisSide)) // #ALLY Imago 7/8/09
            {

                if ((pshipHim->GetObjectID() < pship->GetObjectID()) &&                 //he has a lower ship ID
                    (pship->GetPilotType() < c_ptPlayer) &&                             //if we are a drone &
                    (pshipHim->GetPilotType() < c_ptPlayer) &&                          //he a drone that
                    ((pshipHim->GetStateM() & miningMaskIGC) == 0) &&                   //isn't mining
                    (pshipHim->GetRipcordModel() == NULL))                              //isn't ripcording
                {
                    ignore = true;                                                      //ignore him
                }
                else
                {
                    HullAbilityBitMask  habmHim = phtHim->GetCapabilities();
                    HullAbilityBitMask  habmMe  = pship->GetBaseHullType()->GetCapabilities();

                    ignore = ((habmMe & c_habmLifepod) && (habmHim & c_habmRescue))  ||
                             ((habmMe & c_habmRescue)  && (habmHim & c_habmLifepod)) ||
                             ((habmMe & c_habmCarrier) && (habmHim & c_habmLandOnCarrier));
                }
            }
            else if (pship->GetPilotType() < c_ptPlayer)
                ignore = true;                                                          //trucker rules of road: we're heavier
        }
    }
    else if ((type == OT_mine) && ((mySide == hisSide) || IsideIGC::AlliedSides(mySide,hisSide)))   //#ALLY    FIXED IMAGO 7/8/09                    //We can ignore friendly minefields
    {
        ignore = true;
    }

    return ignore;
}

bool    Dodge(IshipIGC*     pship,
              ImodelIGC*    pmodelIgnore,
              int*          pstate,
              bool          bShipsOnly,
              float         tMax)
{
    IclusterIGC*    pcluster = pship->GetCluster();
    assert (pcluster);

    const Vector&       myPosition = pship->GetPosition();
    const Vector&       myVelocity = pship->GetVelocity();
    const Orientation&  myOrientation = pship->GetOrientation();
    float               speed = myVelocity.Length();
    float               myRadius = pship->GetRadius() + 1.0f;
    float               myAcceleration = pship->GetHullType()->GetThrust() / pship->GetMass();

    HitTest*    myHitTest = pship->GetHitTest();

    ImodelIGC*  pmodelCollide = NULL;
    float       tCollide = FLT_MAX;
    for (ModelLinkIGC*  pml = (bShipsOnly
                               ? (ModelLinkIGC*)(pcluster->GetShips()->first())
                               : pcluster->GetPickableModels()->first());
         (pml != NULL); pml = pml->next())
    {
        ImodelIGC*  pmodel = pml->data();
        if ((pmodel != pship) &&
            (pmodel != pmodelIgnore) &&
            (pmodel->GetHitTest()->GetNoHit() != myHitTest))

        {
            //Ignore the appropriate things plus minefields if we are going slowly enough.
            if (!Ignore(pship, pmodel) && ((speed > 50.0f) || (pmodel->GetObjectType() != OT_mine)))
            {
                //Get the distance of closest approach
                const Vector&   hisPosition = pmodel->GetPosition();
                const Vector&   hisVelocity = pmodel->GetVelocity();

                float   r = pmodel->GetRadius() + myRadius;
                Vector  dp = hisPosition - myPosition;
                float   d2 = dp.LengthSquared();

                Vector  direction;
                float   t = (d2 > r * r)
                            ? solveForImpact(dp,
                                             hisVelocity - myVelocity,
                                             0.0f,
                                             r,
                                             &direction)
                            : 0.0f;


                assert (t >= 0.0f);

                if (t < tCollide)
                {
                    if (tMax >= 0.0f
                        ? (t <= tMax)
                        : (t*t <= (4.0f * r / myAcceleration)))
                    {
                        pmodelCollide = pmodel;
                        tCollide = t;
                    }
                }
            }
        }
    }

    if (pmodelCollide)
    {
        // We are going to collide: find our positions at closest approach
        const Vector    dp = myPosition - pmodelCollide->GetPosition();

        Vector  ca;
        if (tCollide == 0.0f)
            ca = dp;            //Already inside the object ... dodge straight away from its center
        else
        {
            const Vector    dv = myVelocity - pmodelCollide->GetVelocity();
            float           ldv2 = dv.LengthSquared();

            ca = ldv2 > 0.1f
                 ? (dp - dv * ((dp * dv) / ldv2))
                 : dp;
            if (ca.LengthSquared() < 0.1f)
            {
                //Aimed straight at the center of the object ... dodge in a nice
                //orthogonal direction to the original displacement
                ca = dp.GetOrthogonalVector();
            }
        }

        // Thrust along the away vector
        *pstate = getDirection(ca, myOrientation);
        return true;
    }

    return false;
}

bool    GotoPlan::Execute(Time  now, float  dt, bool bDodge)
{
    int         stateM;
    ControlData controls;

    bool    bDone = SetControls(dt, bDodge, &controls, &stateM);

    const int   c_maneuverButtons = backwardButtonIGC |
                                    forwardButtonIGC |
                                    leftButtonIGC |
                                    rightButtonIGC |
                                    upButtonIGC |
                                    downButtonIGC |
                                    afterburnerButtonIGC |
                                    coastButtonIGC |
                                    oneWeaponIGC |      //Disable any weapon fire
                                    allWeaponsIGC;

    m_pship->SetStateBits(c_maneuverButtons, stateM);
    m_pship->SetControls(controls);

    return bDone;
}

// mmf this is where autopilot does its work
bool    GotoPlan::SetControls(float  dt, bool bDodge, ControlData*  pcontrols, int* pstate)
{
    bool    bDone;

    *pstate = 0;
    pcontrols->Reset();

    if (m_maskWaypoints != 0)
    {
        assert (m_wpTarget.m_pmodelTarget);

        IclusterIGC*    pcluster = m_pship->GetCluster();
        assert (pcluster);

        if (pcluster != m_pvOldCluster)
        {
            //The ship's cluster changed ... recalculate goals from the original goal
            m_wpWarp.Reset();
            m_maskWaypoints = c_wpTarget;

            //See if the goal was to enter a warp
            if (m_wpTarget.m_pmodelTarget &&
                (m_wpTarget.m_objective == Waypoint::c_oEnter) &&
                (m_wpTarget.m_pmodelTarget->GetObjectType() == OT_warp) &&
                (((IwarpIGC*)(m_wpTarget.m_pmodelTarget))->GetDestination()->GetCluster() == pcluster))
            {
                m_wpTarget.Reset();
                m_maskWaypoints = 0;

                return true;
            }

            m_pvOldCluster = pcluster;
        }

        const Vector&       myPosition = m_pship->GetPosition();
        const Vector&       myVelocity = m_pship->GetVelocity();
        const Orientation&  myOrientation = m_pship->GetOrientation();
        float               speed = myVelocity.Length();
        float               myRadius = m_pship->GetRadius() + 2.0f;

        bDone = false;

        IclusterIGC*    pclusterTarget = m_pship->GetMission()->GetIgcSite()->GetCluster(m_pship, m_wpTarget.m_pmodelTarget);

		//Xynth #24 7/2010 Check to see if target entered our cluster 
		if ((pclusterTarget != m_pvOldClusterTarget)  &&
			(pclusterTarget == pcluster))
		{
			//If so reset the waypoint to start heading toward target
			//instead of the aleph
			m_wpWarp.Reset();
            m_maskWaypoints = c_wpTarget;

			m_pvOldClusterTarget = pclusterTarget;
		}

        //No point in going towards a dead target or a target we can not see
        if (pclusterTarget)
        {
            if (pcluster != pclusterTarget)
            {
                if (((m_maskWaypoints & c_wpWarp) == 0) ||
                    (pclusterTarget != m_pvOldClusterTarget))
                {
                    //The target is in a different cluster from the ship and either we have no
                    //warp waypoint selected or the target changed clusters. Either way ... recalculate
                    //the warp waypoint

                    //Clear the old intermediate waypoints
                    m_wpWarp.Reset();

                    m_pvOldClusterTarget = pclusterTarget;

                    bool        bCoward = (m_pship->GetPilotType() < c_ptCarrier);
                    IwarpIGC*   pwarp = FindPath(m_pship, pclusterTarget, bCoward);
                    if (bCoward && (pwarp == NULL))
                        pwarp = FindPath(m_pship, pclusterTarget, false);

                    if (pwarp)
                    {
                        m_wpWarp.Set(Waypoint::c_oEnter, pwarp);
                        m_maskWaypoints = c_wpTarget | c_wpWarp;
                    }
                    else
                    {
                        //Do not know of a warp to the intended target ... keep the original target around & clear the warp waypoint
                        m_maskWaypoints = c_wpTarget;

                        if (bDodge)
                            Dodge(m_pship, NULL, pstate);
                        return false;
                    }
                }
            }
            else if (m_wpTarget.m_pmodelTarget->GetCluster() == NULL)
            {
                //We are in the target cluster but the target is not here ... probably because it is docked.
                //wait and do nothing (but clear the warp waypoint since we're in the right cluster.
                m_maskWaypoints = c_wpTarget;

                if (bDodge)
                    Dodge(m_pship, NULL, pstate);
                return false;
            }

            const IhullTypeIGC* pht = m_pship->GetHullType();

            double   backMultiplier = pht->GetBackMultiplier();
            double   vMax = pht->GetMaxSpeed() * backMultiplier;
            double   thrustMax = pht->GetThrust() * backMultiplier;
            double   mass = m_pship->GetMass();

            double   k = thrustMax / (mass * vMax);

            //Calculate the ship's rest position if it kills its throttle
            float   distanceRest = 0.0f;
            Vector  positionRest = myPosition;
            if (speed > 0.1f)
            {
                //If we killed our throttle, where would we drift to?
                //From the differential equations:
                //  dV/dT = -k V(t) + A
                //
                //  A = acceleration = thrust / mass
                //  k = drag coefficient = thrustMax / (vMax * mass)
                //
                //  Vterminal = A / k
                //
                //  V(t) = Vterminal + (V(0) - Vterminal) * exp(-k t)
                //In this particular case ... the ship is decelerating,
                //so Vterminal = -vMax

                //Solve V(t) = 0 = Vterminal + (V(0) - Vterminal) * exp(-k t)
                //-Vterminal = (V(0) - Vterminal) exp (-k t) or, where Vmax = -Vterminal & speed = V(0)
                // Vmax = (speed + Vmax) exp (-kt)
                //t = ln(vMax / (v0 + vMax)) / -k
                double   expmkt = vMax / (speed + vMax);
                double   mkt = log(expmkt);

                //S(t) = Integral (0 -> t) (V(t) dt)
                //     = s0 + Vterminal (t - 0) + (speed - Vterminal) * (1/-k) (exp(-kt) - exp(0))
                //     = 0  +
                //            Vmax * -kt / k    +
                //                                (speed + Vmax) * (1.0 - exp(-kt)) / k
                //
                // But ... -kt = ln (Vmax / (speed + Vmax)), so

                // S(t) = Vmax * -kt / k + (speed * Vmax) * (1 - Vmax / (speed + Vmax)) / k
                //      = Vmax * -kt/k + speed/k
                //      = (Vmax * -kt + speed) / k
                distanceRest = float((vMax * mkt + speed) / k);
                positionRest += myVelocity * (distanceRest / speed);
            }

            GotoPositionMask    gpm;
            Vector              positionGoto;
            ImodelIGC*          pmodelSkip;
            Vector              facing;

            assert (m_maskWaypoints & c_wpTarget);
            gpm = ((m_maskWaypoints & c_wpWarp) ? m_wpWarp : m_wpTarget).GetGotoPosition(m_pship,
                                                                                         distanceRest,
                                                                                         positionRest,
                                                                                         &positionGoto,
                                                                                         &pmodelSkip,
                                                                                         &facing);


            //First priority ... dodge
            if (((gpm & c_gpmNoDodge) == 0) && bDodge)
            {
                if (Dodge(m_pship, pmodelSkip, pstate, (gpm & c_gpmDodgeShips) != 0))
                {
                    if ((gpm & (c_gpmPivot | c_gpmEnter)) != 0)
                    {
                        if (facing * facing >= 0.1f)
                            turnToFace(facing, dt, m_pship, pcontrols, m_fSkill);
                    }
                    else
                    {
                        Vector path = positionGoto - myPosition;
                        if (path * path >= 0.1f)
                            turnToFace(path, dt, m_pship, pcontrols, m_fSkill);
                    }
                    return false;
                }
            }

            pcontrols->jsValues[c_axisThrottle] = -1.0f;
            if (((gpm & c_gpmFinished) != 0) && (m_wpTarget.m_pmodelTarget->GetObjectType() != OT_ship))
            {
                m_wpWarp.Reset();
                m_wpTarget.Reset();

                m_maskWaypoints = 0;

                bDone = true;
            }
            else if ((gpm & c_gpmPivot) != 0)
            {
                //Pivot implies kill throttle
                if (facing * facing > 0.1f)
                    turnToFace(facing, dt, m_pship, pcontrols, m_fSkill);
            }
            else if ((gpm & c_gpmKillThrottle) != 0)
            {
                Vector path = positionGoto - myPosition;
                if (path * path >= 0.1f)
                    turnToFace(path, dt, m_pship, pcontrols, m_fSkill);
            }
            else if ((gpm & c_gpmEnter) != 0)
            {
                if (facing * facing > 0.1f)
                    turnToFace(facing, dt, m_pship, pcontrols, m_fSkill);
                if ((gpm & c_gpmRoll) != 0)
                {
					// mmf took this out, compiler says it is never referenced
                    //float   rollMax2 = 1.0f -
                    //                   pcontrols->jsValues[c_axisPitch] * pcontrols->jsValues[c_axisPitch] -
                    //                   pcontrols->jsValues[c_axisYaw] * pcontrols->jsValues[c_axisYaw];

                    //What is the angle we'd like to roll? Is the angle between the ship's Y axis and the global z-axis
                    const Orientation&  o = m_pship->GetOrientation();
                    double              cosRoll = o.GetUp().z;      //Dot product of the up vector with (0,0,1)

                    if ((cosRoll < 0.99) && (cosRoll > -0.99))
                    {
                        float   roll = float((cosRoll >= 0.0) ? -acos(cosRoll) : acos(-cosRoll));

                        if (o.GetRight().z < 0.0f)
                            roll = -roll;

                        float               tm   = m_pship->GetTorqueMultiplier();
                        const IhullTypeIGC* pht  = m_pship->GetHullType();
                        float               mass = m_pship->GetMass();
                        assert (mass > 0.0f);

                        float   rollRate = m_pship->GetCurrentTurnRate(c_axisRoll);
                        roll -= (float)(m_fSkill * fabs(rollRate) * (0.5f * rollRate * mass / (tm * pht->GetTurnTorque(c_axisRoll))));


                        float   maxRoll= dt * pht->GetMaxTurnRate(c_axisRoll);

                        double   r = roll / maxRoll;

                        double   d2 = pcontrols->jsValues[c_axisPitch] * pcontrols->jsValues[c_axisPitch] +
                                      pcontrols->jsValues[c_axisYaw] * pcontrols->jsValues[c_axisYaw];

                        if (d2 + r * r > 1.0)
                        {
							// mmf debug
							// mmf when pitch or yaw is very close to one and the other is close to zero d2
							// is greater than 1 so set it to 1
							if (d2 > 1.0) {
								// debugf("mmf common.cpp d2 > 1.0 about to sqrt a negative number setting d2 to 1\n");
								//debugf("d2=%g, r=%g, js pitch=%f yaw=%f\n",d2,r,pcontrols->jsValues[c_axisPitch],
								//		pcontrols->jsValues[c_axisYaw]);
								d2=1.0;
							}
                            float   f = float(sqrt(1.0 - d2));
                            pcontrols->jsValues[c_axisRoll] = (r > 0.0) ? f : -f;
                        }
                        else
                            pcontrols->jsValues[c_axisRoll] = float(r);
                    }
                }
                pcontrols->jsValues[c_axisThrottle] = 1.0f;
            }
            else
            {
                //We want to go someplace and we're not coasting to a stop (which implies we got a long way to go)
                //see if we need to plot a course around anything in the meantime.
                Vector          path = positionGoto - myPosition;
                float           distance2 = path.LengthSquared();

                const float     divertOffset = 10.0f;

                float           dMax2 = distance2;

                ImodelIGC*      pmodelDivert = NULL;

                for (ModelLinkIGC*  pml = pcluster->GetPickableModels()->first();
                     (pml != NULL);
                     pml = pml->next())
                {
                    ImodelIGC*  pmodel = pml->data();
                    if ((pmodel != m_pship) &&
                        (pmodel != pmodelSkip) &&
                        !Ignore(m_pship, pmodel))
                    {
                        ObjectType  type = pmodel->GetObjectType();
                        if ((type != OT_mine) &&
                            ((type != OT_ship) || (pmodel->GetVelocity().LengthSquared() < 1.0f)))
                        {
                            //Vector to the center of the object
                            const Vector&   itsPosition = pmodel->GetPosition();

                            Vector  dp = itsPosition - myPosition;
                            float   d2 = dp.LengthSquared();
                            if (d2 < dMax2)             //Object is closer that our goal
                            {
                                float   dot = dp * path;
                                if (dot >= 0.0f)            //Object is not behind us
                                {
                                    float   r = myRadius + pmodel->GetRadius() + divertOffset;
                                    float   r2 = r*r;

                                    Vector  closest = myPosition + path * (dot / distance2);
                                    Vector  offset = closest - itsPosition;
                                    float   offsetLength2 = offset.LengthSquared();
                                    if (offsetLength2 < r2)
                                    {
                                        pmodelDivert = pmodel;
                                        dMax2 = d2;
                                    }
                                }
                            }
                        }
                    }
                }

                if (pmodelDivert)
                {
                    //We need to get around this object ...
                    const Vector&   itsPosition = pmodelDivert->GetPosition();

                    Vector  dp = itsPosition - myPosition;
                    float   dot = dp * path;
                    assert (dot >= 0.0f);

                    Vector  closest = myPosition + path * (dot / distance2);
                    Vector  offset = closest - itsPosition;
                    float   offsetLength2 = offset.LengthSquared();

                    //The object extends across our path ... recalculate a new path
                    Vector  cross;
                    if (offsetLength2 >= 0.5f)
                    {
                        Vector  n = CrossProduct(offset, dp);
                        cross = CrossProduct(dp, n);
                    }
                    else
                        cross = dp.GetOrthogonalVector();

                    if (cross.LengthSquared() != 0)
                        cross = cross.Normalize();
                    else
                        cross = Vector::RandomDirection();

                    double  rpf = myRadius + pmodelDivert->GetRadius() + divertOffset;
                    double  rpf2 = rpf*rpf;

                    double  dpLength2 = dp.LengthSquared();
                    double  sinTheta2 = rpf * rpf / dpLength2;

                    if (sinTheta2 < 0.99)
                    {
                        double  cosTheta2 = (1.0 - sinTheta2);

                        //This is a vector tangent to the blocking object
                        Vector  tangent = dp + cross * float(sqrt(rpf2 / cosTheta2));
                        float   tangentLength2 = tangent.LengthSquared();

                        float   dot = dp * tangent;
                        assert (dot > 0.0f);

                        Vector  pca = tangent * (dot / tangentLength2);
                        Vector  radial = (pca - dp).Normalize();

                        path = pca + radial * float(rpf * 0.02);
                    }
                    else
                    {
                        //We are very close to the object ... turn 90
                        path = cross * float(rpf);
                    }
                }

                //Always face where we are going
                if (path * path >= 0.1f)
                {
					float da = turnToFace(path, dt, m_pship, pcontrols, m_fSkill);

                    if (da < pi / 8.0f)
                    {
                        float   fThrottleMax = 1.0f;

                        if (((gpm & c_gpmFast) == 0) &&
                            ((positionGoto - myPosition).LengthSquared() < distanceRest * distanceRest))
                        {
                            //We are close enough to want to slow down
                            fThrottleMax = -1.0f;
                        }
                        else
                        {
                            //Are we headed through a minefield?
                            IsideIGC*   mySide = m_pship->GetSide();

                            for (ModelLinkIGC*  pml = pcluster->GetPickableModels()->first();
                                 (pml != NULL);
                                 pml = pml->next())
                            {
                                ImodelIGC*  pmodel = pml->data();
                                if ((pmodel->GetObjectType() == OT_mine) && ((pmodel->GetSide() != mySide) && !IsideIGC::AlliedSides(pmodel->GetSide(), mySide))) //#ALLY -was : pmodel->GetSide() != mySide  FIXED imago 7/8/09
                                {
                                    //Vector to the center of the object
                                    const Vector&   itsPosition = pmodel->GetPosition();

                                    Vector  dp = itsPosition - myPosition;
                                    float   d2 = dp.LengthSquared();
                                    if (d2 < distance2)             //Object is closer that our goal
                                    {
                                        float   dot = dp * path;
                                        if (dot >= 0.0f)            //Object is not behind us
                                        {
                                            float   r = myRadius + pmodel->GetRadius();

                                            float   rSlow = r + distanceRest;
                                            if (d2 <= rSlow * rSlow)
                                            {
                                                float   r2 = r*r;

                                                Vector  closest = myPosition + path * (dot / distance2);
                                                Vector  offset = closest - itsPosition;
                                                float   offsetLength2 = offset.LengthSquared();
                                                if (offsetLength2 < r2)
                                                {
                                                    fThrottleMax = -0.5;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        pcontrols->jsValues[c_axisThrottle] = fThrottleMax;
                    }
                    else
                        *pstate = getDirection(path - myVelocity * float(1.0 / k), myOrientation);
                }
                else
                {
                    pcontrols->Reset();
                }

            }
        }
    }
    else
        bDone = true;

    return bDone;
}
bool        LineOfSightExist(const IclusterIGC* pcluster,
                             const ImodelIGC*   pmodel1,
                             const ImodelIGC*   pmodel2)
{
    assert (pcluster);
    assert (pmodel1);
    assert (pmodel1->GetObjectType() != OT_asteroid);
    assert (pmodel2);
    assert (pmodel1->GetCluster() == pcluster);
    assert (pmodel2->GetCluster() == pcluster);

    // P1 is the eye,and P2 is the center of the target that we want to know whether or not is visible
    const Vector&   P1 = pmodel1->GetPosition();
    const Vector&   P2 = pmodel2->GetPosition();

    // compute the vector between the two points, get its squared length, the reciprocal of the length, and then normalize it
    Vector          V12 = P2 - P1;
    float           fLengthSquaredV12 = V12.LengthSquared (),
                    fOverLengthV12 = 1.0f / sqrtf (fLengthSquaredV12);
    V12 *= fOverLengthV12;

    // compute the angle subtended by the target from the eye
    float           fVisibleAngle = asinf (pmodel2->GetRadius() * fOverLengthV12);

    for (ModelLinkIGC* pml = ((ModelListIGC*)(pcluster->GetAsteroids()))->first(); (pml != NULL); pml = pml->next())
    {
        ImodelIGC*  pmodel = pml->data();
        if (pmodel2 != pmodel)
        {
            // P3 is the center of the object that might obscure our view of P2
            const Vector&   P3 = pmodel->GetPosition();

            // Compute the vector between the eye and the obstacle, and get its squared length.
            Vector          V13 = P3 - P1;
            float           fLengthSquaredV13 = V13.LengthSquared ();

            // if the obscruing object is closer than the target, then it might really
            // obscure the view of the target
            if (fLengthSquaredV13 < fLengthSquaredV12)
            {
                float   dot = (V12 * V13);
                if (dot > 0.0f)
                {
                    // If the dot product is negative, the obstacle is behind the eye and could not
                    // possibly obscure the view (unless the eye is inside of the obscuring object,
                    // which we hope won't happen).

                    // compute the reciprocal of the length of the vector, and then normalize it
                    float   fOverLengthV13 = 1.0f / sqrtf (fLengthSquaredV13);
                    //V13 *= fOverLengthV13;

                    // Compute the dot product of the two vectors, this is the cosine of
                    // the angle between them.
                    float   fCosineSeparationAngle = dot * fOverLengthV13;

                    //assert (fCosineSeparationAngle > 0.0f);       //This should be true, but it only takes a little round-off to spoil a day
                    {
                        // Get the radius of the obscuring model, and scale it to simulate a dense core
                        // or variable geometry. We then compute the coverage angle of the obscuring
                        // object.
                        float   fRadius3 = pmodel->GetRadius () * 0.5f;
                        float   fCoveredAngle = asinf (fRadius3 * fOverLengthV13);

                        // Compute the separation angle of the line to the target and the line to the
                        // obscuring object. The farthest angle from the obscuring object at which the
                        // target can be oscured is the separation angle plus the visible angle.
                        float   fSeparationAngle = acosf (fCosineSeparationAngle);
                        float   fMaximumSeparationAngle = fSeparationAngle + fVisibleAngle;

                        // If the farthest angle at which the target can be viewed is covered by the obscuring object,
                        // then the target is not visible and we return false.
                        if (fMaximumSeparationAngle < fCoveredAngle)
                            return false;
                    }
                }
            }
        }
    }

    return true;
}


IshipIGC*   CreateDrone(ImissionIGC*     pmission,
                        ShipID           shipID,
                        PilotType        pt,
                        const char*      pszName,
                        HullID           hullID,
                        IsideIGC*        pside,
                        AbilityBitMask   abmOrders,
                        float            shootSkill,
                        float            moveSkill,
                        float            bravery)
{
    // Do IGC initialization:
    DataShipIGC  ds;

    ds.shipID = shipID;
    ds.hullID = hullID;
    ds.sideID = pside->GetObjectID();
    //ds.wingID = 0;
    ds.nKills = 0;
    ds.nDeaths = 0;

    ds.pilotType = pt;
    ds.abmOrders = abmOrders;
    ds.baseObjectID = NA;

    if (pszName)
    {
        assert (strlen(pszName) < c_cbName - 4);
        strcpy(ds.name, pszName);

        //Is the name unique?
        ShipLinkIGC*    psl;
        for (psl = pmission->GetShips()->first();
             ((psl != NULL) && (_stricmp(pszName, psl->data()->GetName()) != 0));
             psl = psl->next())
        {
        }

        if (psl != NULL)
        {
            //Name is not unique ... make it unique
            _itoa(shipID, ds.name + strlen(ds.name), 10);
        }
    }
    else
        ds.name[0] = '\0';

    ds.nParts = 0;

    IshipIGC* ship = (IshipIGC*)(pmission->CreateObject(pmission->GetLastUpdate(),            // Make the IGC ship
                                                        OT_ship, &ds,
                                                        sizeof(DataShipIGC)));

    assert (ship);

    //Try to fill out the drone with the specified parts
    {
        IhullTypeIGC*   pht = ship->GetBaseHullType();
        for (PartTypeLinkIGC*   ptl = pht->GetPreferredPartTypes()->first();
             (ptl != NULL);
             ptl = ptl->next())
        {
            IpartTypeIGC*   ppt = ptl->data();

            //Mount the part anyplace it can be mounted. Ignore price (included in the cost of the drone) & availability
            EquipmentType   et = ppt->GetEquipmentType();
            Mount           mountMax = (et == ET_Weapon)
                                       ? pht->GetMaxWeapons()
                                       : 1;

            for (Mount i = 0; (i < mountMax); i++)
            {
                if ((ship->GetMountedPart(et, i) == NULL) && pht->CanMount(ppt, i))
                    ship->CreateAndAddPart(ppt, i, 0x7fff);
            }
        }
    }
    ship->SetAutopilot(true);
    ship->Release();

    return ship;
}

ClusterWarning  GetClusterWarning(AssetMask am, bool bInvulnerableStations)
{
    ClusterWarning  cw;

    if ((am & c_amEnemyAPC) && (am & c_amStation) && !bInvulnerableStations)
        cw = c_cwStationCaptureThreat;
    else if ((am & (c_amEnemyTeleport | c_amEnemyTeleportShip)) && (am & c_amStation) && !bInvulnerableStations)
        cw = c_cwStationTeleportThreat;
    else if ((am & c_amEnemyBomber) && (am & c_amStation) && !bInvulnerableStations)
        cw = c_cwStationThreatened;
    else if ((am & c_amEnemyAPC) && !bInvulnerableStations)
        cw = c_cwTransportInCluster;
    else if (am & c_amEnemyTeleportShip)
        cw = c_cwTeleportInCluster;
    else if (am & c_amEnemyCapital)
        cw = c_cwCapitalInCluster;
    else if (am & c_amEnemyCarrier)
        cw = c_cwEnemyCarrierInCluster;
    else if ((am & c_amEnemyBomber) && !bInvulnerableStations)
        cw = c_cwBomberInCluster;
    else if ((am & c_amEnemyFighter) && (am & c_amCarrier))
        cw = c_cwCarrierThreatened;
    else if ((am & c_amEnemyFighter) && (am & c_amBuilder))
        cw = c_cwBuilderThreatened;
    else if ((am & c_amEnemyFighter) && (am & c_amMiner))
        cw = c_cwMinerThreatened;
    else if ((am & c_amEnemyFighter) && (am & c_amFighter))
        cw = c_cwCombatInCluster;
    else if (am & c_amEnemyBuilder)
        cw = c_cwEnemyBuilderInCluster;
    else if (am & c_amEnemyMiner)
        cw = c_cwEnemyMinerInCluster;
    else if (am & c_amEnemyFighter)
        cw = c_cwEnemyFighterInCluster;
    else
        cw = c_cwNoThreat;

    return cw;
}

const char*     GetClusterWarningText(ClusterWarning cw)
{
    static const char*  c_pszAlerts[c_cwMax] =
    {
        "No threat",
        "Enemy fighter spotted",
        "Enemy miner spotted",
        "Enemy builder spotted",
        "Conflict",
        "Miner at risk",
        "Builder at risk",
        "Carrier at risk",
        "Enemy bomber spotted",
        "Enemy carrier spotted",
        "Enemy capital ship spotted",
        "Enemy assault ship spotted",
        "Enemy transport spotted",
        "Station at risk",
        "Station at risk by teleport",
        "Station at risk of capture"
    };

    assert(cw >= 0 && cw < c_cwMax);
    return c_pszAlerts[cw];
}

DamageTrack::DamageTrack(DamageTrackSet* pdts)
:
    m_pset(pdts)
{
    assert (pdts);
    pdts->AddTrack(this);
}

DamageTrack::~DamageTrack(void)
{
    Reset();

    assert (m_pset);
    m_pset->DeleteTrack(this);
}

void    DamageTrack::SwitchSlots(void)
{
    DamageBucketLink*   l = m_buckets.first();
    while (l)
    {
        //Prefetch the next pointer
        DamageBucketLink*   lNext = l->next();

        DamageBucket*  db = l->data();
        db->SwitchSlots(m_pset->m_idSlot);

        if (db->totalDamage() < 0.5f)       //Allow a generous amount for roundoff errors
            delete db;                      //Also deletes the link

        l = lNext;
    }

    //We could sort here ... but let's not. Do a lazy sort the next time we take damage
}

void    DamageTrack::ApplyDamage(Time        timeNow,
                                 ImodelIGC*  pmodel,
                                 float       damage)
{
    DamageBucketLink*  l;
    for (l = m_buckets.first();
         ((l != NULL) && (l->data()->model() != pmodel));
         l = l->next())
    {
    }

    DamageBucket*  pBucket;
    if (l == NULL)
    {
        //No bucket existed for the model ... create one
        pBucket = new DamageBucket(this, pmodel);
    }
    else
        pBucket = l->data();

    pBucket->ApplyDamage(timeNow, m_pset->m_idSlot, damage);

    DamageTrack::sort(&m_buckets);
}

void    DamageTrack::Reset(void)
{
    DamageBucketLink*   pdbl;
    while (pdbl = m_buckets.first())    //Intentional
        delete pdbl->data();
}

void DamageTrack::sort(DamageBucketList*  pListBuckets)
{
    if (pListBuckets->n() > 1)
    {
        //Sort the elements of the bucket list which is almost sorted
        DamageBucketLink*   p = pListBuckets->first();
        while (true)
        {
            DamageBucketLink*   next = p->next();
            if (!next)
                break;

            if (p->data()->totalDamage() < next->data()->totalDamage())
            {
                //*p < *next (& therefore either p or next is out of order)
                //Move next forward as much as is needed to re-establish the sort of all
                //of the elements between the start of the list and p
                next->unlink();

                DamageBucketLink*   back = p->txen();
                while ((back) && (back->data()->totalDamage() >= next->data()->totalDamage()))
                {
                    back = back->txen();
                }

                if (back)
                {
                    //back has done more damage than next ... therefore next should be after it.
                    back->next(next);
                }
                else
                {
                    //couldn't find a bucket with a higher damage total than next ...
                    //put next at the front of the list.
                    pListBuckets->first(next);
                }
            }
            else
                p = next;
        }
    }
}

void   PlayerScoreObject::CalculateScore(ImissionIGC*   pmission)
{
    if (m_dtPlayed == 0.0f)
        return;

    float   kMax = m_dtPlayed / (15.0f * 60.0f);    //1.0 / 15 minutes


    m_fScore = float(m_cWarpsSpotted)       * pmission->GetFloatConstant(c_fcidPointsWarp) +
        float(m_cAsteroidsSpotted)           * pmission->GetFloatConstant(c_fcidPointsAsteroid) +
        m_cTechsRecovered                    * pmission->GetFloatConstant(c_fcidPointsTech) +
        (m_cMinerKills * kMax)               * pmission->GetFloatConstant(c_fcidPointsMiner) / (m_cMinerKills + kMax) +
        (m_cBuilderKills * kMax)             * pmission->GetFloatConstant(c_fcidPointsBuilder) / (m_cBuilderKills + kMax) +
        (m_cLayerKills * kMax)               * pmission->GetFloatConstant(c_fcidPointsLayer) / (m_cLayerKills + kMax) +
        (m_cCarrierKills * kMax)             * pmission->GetFloatConstant(c_fcidPointsCarrier) / (m_cCarrierKills + kMax) +
        m_cPlayerKills                       * pmission->GetFloatConstant(c_fcidPointsPlayer) +
        (m_cBaseKills * kMax)                * pmission->GetFloatConstant(c_fcidPointsBaseKill) / (m_cBaseKills + kMax) +
        (m_cBaseCaptures * kMax)             * pmission->GetFloatConstant(c_fcidPointsBaseCapture) / (m_cBaseCaptures + kMax) +
        float(m_cRescues)                    * pmission->GetFloatConstant(c_fcidPointsRescues) +
        float(m_cArtifacts)                  * pmission->GetFloatConstant(c_fcidPointsArtifacts) +
        float(m_cFlags)                      * pmission->GetFloatConstant(c_fcidPointsFlags) +
        float(m_cProbeSpot)                  * 10 +
        float(m_cRepair)                     * 10;

    if (m_bWin)
        m_fScore *= 2.0f;

	m_fScore *= m_rankRatio;
}

float  PlayerScoreObject::GetScore(void) const
{
    return m_fScore;
}
void   PlayerScoreObject::SetScore(float fNewScore)
{
    m_fScore = fNewScore;
}

void    PlayerScoreObject::AdjustCombatRating(ImissionIGC*          pmission,
                                              PlayerScoreObject*    ppsoKiller,
                                              PlayerScoreObject*    ppsoKillee)
{

    float   k = (pmission->GetFloatConstant(c_fcidRatingAdd) +
                 ppsoKillee->m_fCombatRating - ppsoKiller->m_fCombatRating) /
                pmission->GetFloatConstant(c_fcidRatingDivide);

    ppsoKiller->m_fCombatRating = ppsoKiller->m_fCombatRating * (1.0f - k) + k;
    ppsoKillee->m_fCombatRating = ppsoKillee->m_fCombatRating * (1.0f - k);
}


int GetDistance(IshipIGC*     pship,
                IclusterIGC*  pcluster,
                IclusterIGC*  pclusterStop,
                int           maxDistance)
{
    assert (pcluster != pclusterStop);

    int distance = 1;

    //Search adjacent clusters for an appropriate target
    WarpListIGC     warpsOne;
    WarpListIGC     warpsTwo;
    ClusterListIGC  clustersVisited;

    WarpListIGC* pwlOneAway = &warpsOne;
    WarpListIGC* pwlTwoAway = &warpsTwo;

    while (true)
    {
        assert (pcluster);
        clustersVisited.first(pcluster);    //We've already visited this cluster

        //Push the destinations of the warps in pcluster onto the end the list of
        //warps that are an extra jump away
        {
            for (WarpLinkIGC*   l = pcluster->GetWarps()->first(); (l != NULL); l = l->next())
            {
                IwarpIGC*   w = l->data();
                if (pship->CanSee(w))
                {
                    IwarpIGC*   pwarpDestination = w->GetDestination();
                    if (pwarpDestination)
                    {
                        IclusterIGC*    pclusterOther = pwarpDestination->GetCluster();

                        if (pclusterOther == pclusterStop)
                            return distance;
                        else if (clustersVisited.find(pclusterOther) == NULL)
                            pwlTwoAway->last(pwarpDestination);
                    }
                }
            }
        }

        //Find the next cluster to search
        if (pwlOneAway->n() == 0)
        {
            if ((pwlTwoAway->n() == 0) || (distance++ >= maxDistance))
                return 0x7fffffff;

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

static void    GetAsteroidName(const char* pszPrefix,
                               AsteroidID  id,
                               char*       bfr)
{
    const int   c_iMultiplier = 5237;
    const int   c_iModulo     = 8713;
    const int   c_iOffset     = 1093;
    const int   c_iPlus       = 1024;

    int l = strlen(pszPrefix);
    if (l == 0)
        l = 2;      //Allow for asteroids with no default name
    memcpy(bfr, pszPrefix, l);
    _itoa(c_iPlus + ((id + c_iOffset) * c_iMultiplier) % c_iModulo,
          &bfr[l], 10);
}

static Vector   RandomPosition(const ModelListIGC*  models, float   rThing, float radius, float lens)
{
    const float maxStationRadius = 550.0f;

    if (rThing < maxStationRadius)
        rThing = maxStationRadius;

    Vector          position;

    ModelLinkIGC*   pmlink;
    do
    {
        //Pick a random position for the asteroid
        //in a squashed disk
        position = Vector::RandomDirection();

        position *= radius * pow(random(0.0f, 1.0f), 1.0f/3.0f);
        position.z *= lens;

        //Verify that it is not close to any other model in the cluster
        for (pmlink = models->first(); (pmlink != NULL); pmlink = pmlink->next())
        {
            ImodelIGC*  pm = pmlink->data();
            float       r = pm->GetRadius();
            if (r < maxStationRadius)
                r = maxStationRadius;
            r += rThing;

            if ((pm->GetPosition() - position).LengthSquared() < 2.0f * r * r)
            {
                //Too close ... try again
                radius *= 1.02f;
                break;
            }
        }
    }
    while (pmlink);

    return position;
}

void    CreateAsteroid(ImissionIGC*         pmission,
                       IclusterIGC*         pcluster,
                       int                  type,
                       float                amountHe3)
{
    DataAsteroidIGC da;

    da.asteroidDef = IasteroidIGC::GetTypeDefaults(type);

    static const Vector xAxis(1.0, 0.0, 0.0);
    static const Vector zAxis(0.0, 0.0, 1.0);

    da.clusterID = pcluster->GetObjectID();

    da.forward = Vector::RandomDirection();
    da.up = CrossProduct(da.forward, xAxis);
    if (da.up.LengthSquared() <= 0.1f)
        da.up = CrossProduct(da.forward, zAxis);
    assert(da.up.LengthSquared() > 0.1f);
    da.up.SetNormalize();

    da.asteroidDef.radius = (short) randomInt(da.asteroidDef.radius, 2*da.asteroidDef.radius);
    da.signature = ((float) da.asteroidDef.radius) / 100.0f;

    da.fraction = 1.0f;

    da.rotation.axis(da.forward);
    da.rotation.angle(0.2f / da.signature);

    if (da.asteroidDef.ore != 0.0f)
        da.asteroidDef.ore *= amountHe3;
    da.asteroidDef.oreMax = da.asteroidDef.ore;

    da.asteroidDef.asteroidID = pmission->GenerateNewAsteroidID();

    GetAsteroidName(IasteroidIGC::GetTypePrefix(type),
                    da.asteroidDef.asteroidID, da.name);

    da.position = RandomPosition(pcluster->GetModels(), da.asteroidDef.radius,
                                 pmission->GetFloatConstant(c_fcidRadiusUniverse) * 0.75f,
                                 pmission->GetFloatConstant(c_fcidLensMultiplier));


    IObject * o = pmission->CreateObject(pmission->GetLastUpdate(),
                                         OT_asteroid,
                                         &da,
                                         sizeof(da));
    assert (o);
    o->Release();
}

static IstationIGC* CreatePedestalAndFlag(ImissionIGC*     pmission,
                                          SectorID         clusterID,
                                          SideID           sideID,
                                          const Vector&    position,
                                          float            dz)
{
    Time            now = pmission->GetLastUpdate();

    DataStationIGC ds;

    ds.clusterID = clusterID;
    ds.position = position;
    ds.position.z += dz * c_fPedestalOffset;

    ds.forward.x = ds.forward.y = 0.0f;
    ds.forward.z = dz;
    ds.up.x = 1.0f;
    ds.up.y = ds.up.z = 0.0f;

    ds.rotation.axis(ds.forward);
    ds.rotation.angle(0.0f);

    ds.bpHull = 1.0f;
    ds.bpShield = 0.0f;

    ds.sideID = sideID;
    ds.stationID = pmission->GenerateNewStationID();

    {
        IstationTypeIGC*    ppedestal = pmission->GetStationTypes()->last()->data();
        assert (ppedestal->HasCapability(c_sabmPedestal));

        ds.stationTypeID = ppedestal->GetObjectID();
        strcpy(ds.name, ppedestal->GetName());
    }

    IstationIGC*    pstation = (IstationIGC*)(pmission->CreateObject(now,
                                                                     OT_station,
                                                                     &ds,
                                                                     sizeof(ds)));
    assert(pstation);

    DataTreasureIGC dt;
    dt.treasureCode = c_tcFlag;
    dt.treasureID = sideID;
    dt.amount = 0;
    dt.clusterID = clusterID;
    dt.lifespan = 10.0f * 24.0f * 3600.0f;

    dt.objectID = pmission->GenerateNewTreasureID();

    dt.p0 = ds.position;
    dt.p0.z += (c_fFlagOffset + pstation->GetRadius()) * dz;
    dt.v0 = Vector::GetZero();
    dt.time0 = now;

    ItreasureIGC* t = (ItreasureIGC *)(pmission->CreateObject(now, OT_treasure,
                                                              &dt, sizeof(dt)));

    assert (t);
    t->Release();

    //Bad form here ... but we know that the station's release count will not go to zero
    pstation->Release();

    return pstation;
}

void        PopulateCluster(ImissionIGC*            pmission,
                            const MissionParams*    pmp,
                            IclusterIGC*            pcluster,
                            float                   amountHe3,
                            bool                    bAsteroids,
                            bool                    bTreasures,
                            short                   nMineableAsteroidsMultiplier,
                            short                   nSpecialAsteroidsMultiplier,
                            short                   nAsteroidsMultiplier)
{
    Time                    now = pmission->GetLastUpdate();
    SectorID                clusterID = pcluster->GetObjectID();
    const StationListIGC*   pstations = pcluster->GetStations();

    bool        bHomeSector = pcluster->GetHomeSector();

    if (pmp->IsFlagsGame())
    {
        for (StationLinkIGC*    psl = pstations->last(); (psl != NULL); psl = psl->txen())
        {
            IstationIGC*    pstation = psl->data();
            IsideIGC*       pside = pstation->GetSide();

            if (pstation->GetBaseStationType()->HasCapability(c_sabmStart))
            {
                SideID          sideID   = pside->GetObjectID();
                const Vector&   position = pstation->GetPosition();

                CreatePedestalAndFlag(pmission, clusterID,
                                      sideID,
                                      position, 1.0f);

                CreatePedestalAndFlag(pmission, clusterID,
                                      sideID,
                                      position, -1.0f);

            }
        }
    }

    {
        //Make all stations invisible to all sides but their own
        const SideListIGC*      psides = pmission->GetSides();
        for (StationLinkIGC*    psl = pstations->first(); (psl != NULL); psl = psl->next())
        {
            IstationIGC*    pstation = psl->data();
            IsideIGC*       pside = pstation->GetSide();

            for (SideLinkIGC*   p = psides->first(); (p != NULL); p = p->next())
            {
                if (p->data() != pside)  //Imago ALLY VISIBILITY 7/11/09 reverted
                    pstation->SetSideVisibility(p->data(),
                                                false);
            }
        }
    }

    if (bAsteroids)
    {
        {
            short   nMineableAsteroids;
            if (nMineableAsteroidsMultiplier >= 0)
            {
                nMineableAsteroids = bHomeSector
                                     ? pmp->nPlayerSectorMineableAsteroids
                                     : pmp->nNeutralSectorMineableAsteroids;
                nMineableAsteroids *= nMineableAsteroidsMultiplier;
            }
            else
                nMineableAsteroids = -nMineableAsteroidsMultiplier;

            for (short i = 0; (i < nMineableAsteroids); i++)
            {
                CreateAsteroid(pmission, pcluster, IasteroidIGC::GetRandomType(c_aabmMineHe3), amountHe3);
            }
        }

        {
            short   nSpecialAsteroids;
            if (nSpecialAsteroidsMultiplier >= 0)
            {
                nSpecialAsteroids = bHomeSector
                                     ? pmp->nPlayerSectorSpecialAsteroids
                                     : pmp->nNeutralSectorSpecialAsteroids;
                nSpecialAsteroids *= nSpecialAsteroidsMultiplier;
            }
            else
                nSpecialAsteroids = -nSpecialAsteroidsMultiplier;

            int offset = IasteroidIGC::NumberSpecialAsteroids(pmp);
            if (offset != 1)
                offset = randomInt(1, offset);
            for (short i = 0; (i < nSpecialAsteroids); i++)
            {
                CreateAsteroid(pmission, pcluster, IasteroidIGC::GetSpecialAsterioid(pmp, offset++), 0.0f);
            }
        }

        {
            short   nAsteroids;
            if (nAsteroidsMultiplier >= 0)
            {
                nAsteroids = bHomeSector
                                     ? pmp->nPlayerSectorAsteroids
                                     : pmp->nNeutralSectorAsteroids;
                nAsteroids *= nAsteroidsMultiplier;
            }
            else
                nAsteroids = -nAsteroidsMultiplier;

            for (short i = 0; (i < nAsteroids); i++)
            {
                CreateAsteroid(pmission, pcluster, IasteroidIGC::GetRandomType(c_aabmBuildable), 0.0f);
            }
        }
    }

    if (bTreasures)
    {
        short   nTreasures;
        short   tsi;
        if (bHomeSector)
        {
            nTreasures = pmp->nPlayerSectorTreasures;
            tsi = pmp->tsiPlayerStart;
        }
        else
        {
            if (pmp->IsArtifactsGame())
            {
                pmission->GenerateTreasure(now,
                                           pcluster,
                                           -2);   //Special hack for flags
            }

            nTreasures = pmp->nNeutralSectorTreasures;
            tsi = pmp->tsiNeutralStart;
        }

        for (short i = 0; (i < nTreasures); i++)
        {
            pmission->GenerateTreasure(now,
                                       pcluster,
                                       tsi);
        }
    }
}
