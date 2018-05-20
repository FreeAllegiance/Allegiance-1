/// <reference path="include/IDL.d.ts" /> 
/// <reference path="include/imago.js" />  



// Settings
var GameName = "Perpetual Bot DM";
var ServerName = "AEast2";
var ServerAddr = "10.99.96.234";
var KillGoal = 99;
var DebugSpam = true;

//var CivSelection = "Rixian,Rixian,Iron Coalition,Dreghklar,Belters,Rixan,Gigacorp,Bios,Ga'Taraan,Technoflux,Draconium";  //blank for Random
var CivSelection = "Iron Coalition,Dreghklar,Belters,Rixan,Gigacorp,Bios,Ga'Taraan,Technoflux,Draconium";  //blank for Random
var ShipSelection = "Interceptor";
UpdatesPerSecond = 30;

/** Event Callback
 * @param {string} strText The radius of the circle.
 * @param {IAGCShip} objShip The radius of the circle.
 * @return {void}  
 */
function OnReceiveChat(strText, objShip) {
    Pig.Host.Trace("Coooooooooooooooooooooooooooooooooooooooooooool beeeeeeeeeeeeaaaaaaaaaaaanzzzzzzzzzzzzzzzzzzzzzzzzzzz\n");

    var x = CType.IAGCShip(objShip);
    
    if (strText == "#check") {
        Pig.Game.SendChat("Check baby check! HullTypeCount = " + Pig.HullTypes.Count + ", objShip.Name: " + objShip.Name);
        
        switch (PigState) {
            case Enums.PigState.PigState_WaitingForMission:
                Game.SendChat("Waiting for mission!");
                break;

            default:
                Game.SendChat("Couldn't read state... :(");
                break;
        }
        
        if (PigState == Enums.PigState.PigState_Flying)
            Game.SendChat(PigState + "==" + Enums.PigState.PigState_Flying);
        else
            Game.SendChat(PigState + "!=" + Enums.PigState.PigState_Flying);
    }

    if (PigStateName != PigState_Flying)
        return;

    if (objShip.Team != Ship.Team)
        return;
}



// deep space!
function OnStateFlying(eStatePrevious) {
    DisplayStateTransition(eStatePrevious);
    KillAllTimers();
    // I think Ship.Thrust might be making the bomber fly up when it gets to the base.. Weird...??!!

    //Delay("Ship.Thrust(ThrustUp,ThrustForward)");
    //Delay("Ship.Boost(true)");

    CreateTimer(1, "UpdateTargetTimer()", -1, "UpdateTargetTimer");
}

function CheckBoost(targetDistance) {
    if (targetDistance > 600) {
        Delay("Ship.Boost(true)");
    } else {
        Delay("Ship.Boost(false)");
    }
}
function CheckFireMissile(targetDistance) {
    // Missiles need to be done through the same firing mechnisim as shooting.
    // This pattern does not work with autopilot.
    return;

    if (targetDistance) {
        var missileLock = Ship.MissileLock;
        var missileRange = Ship.MissileRange;
        Trace("Missile Lock: " + missileLock + " Range: " + missileRange + " Target Range: " + targetDistance + " \n");

        // TODO: Missile lock isn't reporting a value > 0; Does this mean there is no target, or is the property just bad.
        //if (Ship.MissileLock > 0.2){ 
        //  Ship.FireMissile(true);
        //}
        if (targetDistance < Ship.MissileRange) {
            // Breaking autopilot
            Ship.FireMissile(true);
        } else {
            //Ship.FireMissile(false);
        }

        if (targetDistance < 600) {
            Ship.DropMine(true);
        } else {
            //Ship.DropMine(false);
        }
    }
}

// Does not have any side effects. Does read from Ship.Sector.Ships and Ship.Sector.Stations
// returns {stationTarget: undefined, shipTarget: undefined};
function FindTarget() {
    var shiplist = Ship.Sector.Ships;
    var stationlist = Ship.Sector.Stations;

    // TODO: Prefer to target a bomber over an Interceptor
    // TODO: Prefer to target a scout over a bomber if its near the bomber
    var nearEnemyIdx = FindNearestEnemy(shiplist);
    var nearEnemyStationIdx = FindNearestEnemyStation(stationlist);

    var foundTarget = { stationTarget: undefined, shipTarget: undefined };
    if (nearEnemyStationIdx > -1 && Ship.BaseHullType != null && Ship.BaseHullType.HasCapability(512)) {
        foundTarget = { stationTarget: stationlist(nearEnemyStationIdx) };
    } else if (nearEnemyIdx > -1) {
        foundTarget = { shipTarget: shiplist(nearEnemyIdx) };
    }
    return foundTarget;
}

var PREVIOUS_TARGET = {};
function UpdateTargetTimer() {

    // Check lifepod
    if (Ship.BaseHullType != null && Ship.BaseHullType.HasCapability(4)) {
        NeedPickup();
        KillTimers();
        return;
    }

    // Check for the latest ship or station targets
    var found = FindTarget();

    var someTarget = found.shipTarget || found.stationTarget
    if (someTarget && someTarget.Position != null && Position != null) {
        var targetDistance = Range2Ship(someTarget);

        CheckFireMissile(targetDistance);
        CheckBoost(targetDistance);

        if (found.shipTarget != PREVIOUS_TARGET.shipTarget) {
            Ship.AttackShip(found.shipTarget);
        }
        if (found.stationTarget != PREVIOUS_TARGET.stationTarget) {
            Ship.AttackStation(found.stationTarget);
        }


        CheckRunAway(targetDistance);
    } else {
        CheckRunAway();
    }
}


function CheckRunAway(targetDistance) {
    targetDistance = (targetDistance || 10000);
    var TargetIsClose = targetDistance < 1400

    if (Ship.BaseHullType != null && Ship.BaseHullType.HasCapability(4)) {
        NeedPickup();
        KillTimers();
        return;
    }
    var goHome = false;


    if (!Ship.Ammo) {
        if (DebugSpam) Game.SendChat("No ammo! heading home NOW");
        goHome = true;
    }

    if (TargetIsClose && Ship.Fraction < 0.30) {
        if (DebugSpam) Game.SendChat("Critical damage! heading home");
        goHome = true;
    }

    if (!TargetIsClose && Ship.Ammo < Ship.HullType.MaxAmmo * 0.10) {
        if (DebugSpam) Game.SendChat("Low ammo! heading home");
        goHome = true;
    }

    if (!TargetIsClose && Ship.Fuel < Ship.HullType.MaxFuel * 0.10) {
        if (DebugSpam) Game.SendChat("Bingo fuel! heading home");
        goHome = true;
    }

    if (goHome) {
        var stationlist = Ship.Sector.Stations;
        var mystationIdx = FindNearestFriendlyStation(stationlist);
        Delay("Ship.GotoStationID(" + stationlist(mystationIdx).ObjectID + ")");
        Delay("Ship.Boost(true)");
        KillTimers();
    }
}

var canChatTaunt = true;
function OnShipDamaged(objShip, objModel, fAmount, fLeakage, objV1, objV2) {

    return;
}

function OnShipKilled(objShip, objModel, fAmount, objV1, objV2) {
    if ((!objShip) || (!Ship) || (!objModel))
        return;
    if (objModel.ObjectID == Ship.ObjectID) {
        if (canChatTaunt == true) {
            canChatTaunt = false;
            // here we can pick from a chat from anything approperate

            if (DebugSpam) {
                // var taunt = RandomTaunt();
                //Game.SendChat(taunt.text,-1, taunt.id);
            }
            CreateTimer(5, "canChatTaunt=true;", 1, "canChatReset");
        }
    }
    if (objShip.ObjectID == Ship.ObjectID) {
        var bht = objShip.BaseHullType;
        var tnow = (new Date).getTime();
        var delta = tnow - LastDeath;
        LastDeath = tnow;
        if (delta > 2000) {
            var moreSkill = 0.95 * ShootSkill + 0.049;	
            ShootSkill += moreSkill;
            TurnSkill += moreSkill;
            GotoSkill += moreSkill;

            if (ShootSkill > 1.0) ShootSkill = 1.0;
            if (TurnSkill > 1.0) TurnSkill = 1.0;
            if (GotoSkill > 1.0) GotoSkill = 1.0;

            if (DebugSpam) {
                // var yell = RandomYell();
                // Game.SendChat(yell.text,-1, yell.id);
            }
        }
    }
}