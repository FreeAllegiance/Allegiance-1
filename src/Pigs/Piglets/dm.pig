<pigbehavior language="JScript">
//imagotrigger@gmail.com 10/14 imago.pig originally from manuel.pig and mark.pig Microsoft

<script src="include\common.js"/>
<script src="include\imago.js"/>
<script src="include\AutoStartCustomGame.js"/>
<script>
<![CDATA[

// Settings


var GameName = "Perpetual Bot DM";
var ServerName = "zonegamewest";
var ServerAddr = "168.62.196.51";
var KillGoal = 99;
var DebugSpam = true;

/*
var GameName = "Perpetual Bot DM";
var ServerName = "azbuildslave";
var ServerAddr = "191.239.1.217";
var KillGoal = 35;
var DebugSpam = false;
*/


/*
var GameName = "Bot Testing";
var ServerName = "Imago-PC";
var ServerAddr = "192.168.2.2";
var KillGoal = 15;
var DebugSpam = true;
*/

var CivSelection = "Iron Coalition,Dreghklar";  //blank for Random
var ShipSelection = "Hvy Interceptor";
var ShootSkill = 0.05;
var TurnSkill = 0.05;
var GotoSkill = 0.05;
UpdatesPerSecond = 30;

function OnReceiveChat(strText, objShip) {
	if (PigStateName != PigState_Flying)
		return;
	if (objShip.Team != Ship.Team)
		return;
	
	//bot pod -> "I need a pickup!" (only if actually needed)
		//see common.js NeedPickup()

	//TODO	
		//determine if i am closer to the pod than the station
			//and not engaged /w an enemy
				//bot rescue <- "I got you, hold still!"
					//Ship.Goto(objShip.Name,true) works?
						//have some kind of array to handle more than one bot picking up more than one pod at a time
		
		//Did i set HailedForRescue?
			//bod pod -> "Thanks"
				//Ship.Goto(objShip.Name,true) works?
}

// deep space!
function OnStateFlying(eStatePrevious) {
	DisplayStateTransition(eStatePrevious);
	KillTimers();
	var x;
	var shiplist = Ship.Sector.Ships;
	var stationlist = Ship.Sector.Stations;
	var e = new Enumerator (shiplist);
	for(; !e.atEnd(); e.moveNext()) {
		x = e.item();
		if (x.Name == Name) {
			MyShip = x; //this should equal Ship
		} else {
			if (x.Team != MyShip.Team) {
				CurrentTarget = x;
			}
		}
	}
	var f = new Enumerator (stationlist);
	for(; !f.atEnd(); f.moveNext()) {
		x = f.item();
		if (x.Name == "Garrison" && x.Team == MyShip.Team) {
			MyGarrison = x;
		}
		if (x.Name == "Garrison" && x.Team != MyShip.Team) {
			EnemyGarrison = x;
		}	    
	}
	if (CurrentTarget) {
		if (DebugSpam) Game.SendChat("My initial target is: "+CurrentTarget.Name);
		Attack(CurrentTarget.Name);
		CreateTimer(1, "UpdateTargetTimer()", -1, "UpdateTargetTimer");
	} else {
		CreateTimer(3, "FindTargetTimer()", -1, "FindTargetTimer");
	}
	CreateTimer(3, "RearmTimer()", -1, "RearmTimer");
	
	//TODO Rescue
		//See OnReceiveChat()
	
	//TODO Missiles
		//Ship.FireMissile(true);
			//this existing function was obviously just for testing (just sets button state)
				//NYI enhance to FireMissile(true, objShip.Name, ShootSkill)
			
	//TODO Chase
		//Boost after if target is getting away
	
	//TODO Mines
		//Ship.DropMine(true); should work fine
			//determine when to do this!  NYI IsFollowing(objShip.Name)
			
	//TODO Countermeasures
		//NYI OnIncomingMissile() Ship.ECM(true);
		
	//TODO Camp
		//Modify GotoStationID, if it's an enemy station, find the nearest exit garage and send that vector along to GotoPlan/Waypoint
			//if a teamate is already camping this door, goto the other one
		
	//TODO Formate
		//Regroup if outmatched
			//compare count of friends engaged vs enemies
}
function FindTargetTimer() {
	if (Ship.BaseHullType.HasCapability(4)) {
		NeedPickup();
		KillTimers();
		return;
	} 
	var objShips = Ship.Sector.Ships;
	var iShip = FindNearestEnemy(objShips);
	if (iShip != -1) CurrentTarget = objShips(iShip);
	if (CurrentTarget) {
		if (DebugSpam) Game.SendChat("My new initial target is: "+CurrentTarget.Name);
		Attack(CurrentTarget.Name);
		Ship.Boost(true);
		Timer.Kill();
		CreateTimer(1, "UpdateTargetTimer()", -1, "UpdateTargetTimer");
	}	
}
function UpdateTargetTimer() {
	if (Ship.BaseHullType.HasCapability(4)) {
		NeedPickup();
		KillTimers();
		return;
	}
	var objShips = Ship.Sector.Ships;
	var newtarget;
	var iShip = FindNearestEnemy(objShips);
	if (iShip != -1) {
		newtarget = objShips(iShip);
		if (CurrentTarget && CurrentTarget.BaseHullType.HasCapability(4)) { //c_habmLifepod
			if (newtarget != CurrentTarget) {
				CurrentTarget = newtarget;
				if (DebugSpam) Game.SendChat("My updated target (not a pod) is: "+CurrentTarget.Name);
				Attack(CurrentTarget.Name);
				return;
			}
		}
		if (CurrentTarget && !IsTargetValid(objShips,CurrentTarget)) {
			if (newtarget != CurrentTarget) {
				CurrentTarget = newtarget;
				if (DebugSpam) Game.SendChat("My updated target (valid) is: "+CurrentTarget.Name);
				Attack(CurrentTarget.Name);
				return;
			}
		}
		if (CurrentTarget && Range2Ship(CurrentTarget) > 2250000) { //1500m ... 562500 = 750m  //TODO skillz
			if (newtarget != CurrentTarget) {
				CurrentTarget = newtarget;
				if (DebugSpam) Game.SendChat("My updated target (distance) is: "+CurrentTarget.Name);
				Attack(CurrentTarget.Name);	
				return;
			}
			Ship.Boost(true);
			IsTargetClose = false;
		} else {
			Ship.Boost(false);
			IsTargetClose = true;
			ReachedEnemy = true;
		}
	} else {
		CurrentTarget = null;
		IsTargetClose = false;
		if (Ship.Fraction < 0.75) {
			KillTimers();
			if (DebugSpam) Game.SendChat("No targets, repairing");	
			Ship.GotoStationID(MyGarrison.ObjectID);								
			Ship.Boost(true);
		} else {
			KillTimers();
			if (DebugSpam) Game.SendChat("No targets, camping");	
			Ship.GotoStationID(EnemyGarrison.ObjectID);			
			Ship.Boost(true);
			CreateTimer(3, "FindTargetTimer()", -1, "FindTargetTimer");
		}
	}
}
function RearmTimer() {
	if (Ship.BaseHullType.HasCapability(4)) {
		NeedPickup();
		KillTimers();	
		return;
	}
	if (Ship.Fraction < 0.15) {
		AboutToDie = true;
		if (DebugSpam) Game.SendChat("Critical damage! heading home NOW");			
		Ship.GotoStationID(MyGarrison.ObjectID);
		Ship.Boost(true);	
		KillTimers();
		return;
	}
	if (!Ship.Ammo) {
		if (DebugSpam) Game.SendChat("No ammo! heading home NOW");
		Ship.GotoStationID(MyGarrison.ObjectID);		
		Ship.Boost(true);	
		KillTimers();		
		return;		
	}
	if (!IsTargetClose && Ship.Fraction < 0.30) {
		AboutToDie = true;
		if (DebugSpam) Game.SendChat("Critical damage! heading home");
		Ship.GotoStationID(MyGarrison.ObjectID);
		Ship.Boost(true);
		KillTimers();			
		return;
	}
	if (!IsTargetClose && Ship.Ammo < Ship.HullType.MaxAmmo * 0.10) {
		if (DebugSpam) Game.SendChat("Low ammo! heading home");	
		Ship.GotoStationID(MyGarrison.ObjectID);
		Ship.Boost(true);	
		KillTimers();	
		return;
	}
	if (ReachedEnemy && !IsTargetClose && Ship.Fuel < Ship.HullType.MaxFuel * 0.10) {	
		if (DebugSpam) Game.SendChat("Bingo fuel! heading home");	
		Ship.GotoStationID(MyGarrison.ObjectID);
		Ship.Boost(true);
		KillTimers();		
		return;
	}
}

function OnShipDamaged(objShip, objModel, fAmount, fLeakage, objV1, objV2) {
	if ((!objShip) || (!objModel))
		return;
	if (objModel.ObjectType != 0) //AGC_Ship
		return;	
	if ((AboutToDie) || (!Ship.Ammo))
		return;
	if (objModel.Team != Ship.Team) {
		var tnow = (new Date).getTime();
		var delta =  tnow - LastDamage;
		LastDamage = tnow;		
		if (delta > 2000) {
			var newtarget;
			var objShips = Ship.Sector.Ships;
			var iShip = FindNearestEnemy(objShips);
			if (iShip != -1) newtarget = objShips(iShip);
			if ((!CurrentTarget) || (newtarget && newtarget.ObjectID == objModel.ObjectID && newtarget.ObjectID != CurrentTarget.ObjectID)) {
				CurrentTarget = newtarget;
				if (DebugSpam) Game.SendChat("My new pesky target is: "+CurrentTarget.Name);
				KillTimers();
				CreateTimer(1, "UpdateTargetTimer()", -1, "UpdateTargetTimer");
				CreateTimer(3, "RearmTimer()", -1, "RearmTimer");
			}
		}
	}
}

function OnShipKilled(objShip, objModel, fAmount, objV1, objV2) {
	if ((!objShip) || (!Ship) || (!objModel))
		return;
	if (objModel.ObjectID == Ship.ObjectID) {
		var tnow = (new Date).getTime();
		var delta =  tnow - LastKill;
		LastKill = tnow;		
		if (delta > 500)	
			if (DebugSpam) Game.SendChat("Yesss.. pwned you!");
	}
	if (objShip.ObjectID == Ship.ObjectID) {
		var bht = objShip.BaseHullType;
		var tnow = (new Date).getTime();
		var delta =  tnow - LastDeath;
		LastDeath = tnow;
		if (delta > 2000) {
			ShootSkill += 0.050;
			TurnSkill += 0.050;
			GotoSkill += 0.050;
			if (DebugSpam) Game.SendChat("Nooo.. skills increased!");
		}
	}
}

]]>
</script>
</pigbehavior>

