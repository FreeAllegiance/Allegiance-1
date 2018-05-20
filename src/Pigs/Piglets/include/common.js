//imagotrigger@gmail.com 10/14 originally from Matthew Lee, 1999 Microsoft

// Constants
var c_habmLifepod 			= 0x04;
var c_habmCaptureThreat 	= 0x08;
var c_habmFighter 			= 0x80;
var c_habmThreatToStation 	= 0x200;



function DisplayStateTransition(eStatePrevious) {
    Trace("State changed from " + StateName(eStatePrevious) + " to " + PigStateName + "\n");
}

function CanCalculatePosition(targetPosition)
{
	return typeof Position !== 'undefined' && targetPosition != null && typeof targetPosition.Position !== 'undefined';
}

function ShouldBeTargeted(potentialTarget)
{
	if(!potentialTarget)
		return false;
	
	if(typeof potentialTarget === 'undefined')
		return false;
	
	if(!potentialTarget.BaseHullType)
		return false;
	
	if(potentialTarget.BaseHullType == null)
		return false;
	
	return potentialTarget.BaseHullType != null && (potentialTarget.BaseHullType + '').toLowerCase().indexOf("pod") == -1;
}

function NeedPickup() {
	if (HailedForRescue)
		return;
	var nearestFriend;
	var objShips = Ship.Sector.Ships;
	var iShip = FindNearestFriend(objShips);
	if (iShip != -1) nearestFriend = objShips(iShip);
	if (nearestFriend && CanCalculatePosition(nearestFriend) && CanCalculatePosition(MyGarrison)) {
		var range = Range2Ship(nearestFriend);
		var range2 = Range2Ship(MyGarrison);
		Trace("In NeedPickup()...ship "+range+" vs station "+range2+"\n");
		if (range < range2) {
			Ship.Team.SendChat("I need a pickup!",-1,1219); //voNeedPickupSound
			HailedForRescue = true;
		}
	}
}

function KillTimers() {
	if ("object" == typeof(Properties("UpdateTargetTimer")))
		Properties("UpdateTargetTimer").Kill();
	if ("object" == typeof(Properties("FindTargetTimer")))
		Properties("FindTargetTimer").Kill();
	if ("object" == typeof(Properties("RearmTimer")))
        Properties("RearmTimer").Kill();
    if ("object" == typeof (Properties("CheckFireMissileTimer")))
        Properties("CheckFireMissileTimer").Kill();
}


function KillAllTimers() {
    KillTimers();
    if ("object" == typeof (Properties("CheckFireMissileTimer")))
        Properties("CheckFireMissileTimer").Kill();
    KillTimer("onStateDockedTimeElapsedTimer");
}

function KillTimer(timerName) {
    if ("object" == typeof (Properties(timerName)))
        Properties(timerName).Kill();
}

function Delay(action) {
    KillTimer(action);
    Properties(action) = undefined;
    CreateTimer(0.1, action, 1, action);
}

function dump(arr,level) {
	var dumped_text = "";
	if(!level) level = 0;
	var level_padding = "";
	for(var j=0;j<level+1;j++) level_padding += "    ";
	if(typeof(arr) == 'object') {
		for(var item in arr) {
			var value = arr[item];
			if(typeof(value) == 'object') {
				dumped_text += level_padding + "'" + item + "' ...\n";
				dumped_text += dump(value,level+1);
			} else {
				dumped_text += level_padding + "'" + item + "' => \"" + value + "\"\n";
			}
		}
	} else {
		dumped_text = "===>"+arr+"<===("+typeof(arr)+")";
	}
	return dumped_text;
}

function plural(nCount, strNonPlural, strPlural) {
  if (nCount == 1 || nCount == -1)
    return strNonPlural;
  else
    return strPlural;
}

function SelectBestHull (hullCollection, strFirstChoice, strSecondChoice) {
  var FirstHull = -1, SecondHull = -1, DefaultHull = -1;
  var e = new Enumerator (hullCollection)
	for (var i=0; !e.atEnd(); e.moveNext(), i++) {
        var hull = e.item();
        
        if (hull.Name.search(strFirstChoice) != -1)
                FirstHull = i;

        if (hull.Name.search(strSecondChoice) != -1)
            SecondHull = i;		

        if (hull.Name.search("Scout") != -1)
            DefaultHull = i;		

    }
	if (FirstHull != -1) {
    return FirstHull;
	} else if (SecondHull != -1) {
    return SecondHull;
	} else {
    return DefaultHull;
  }
}

function Range2Ship(agcShip) {
	
	//Trace("My Position: " + Position + "\n");
	//Trace("agcShip.Position: " + agcShip.Position + "\n");
	
    return (Position.Subtract(agcShip.Position).Length);
}

function FindNearestEnemy(shipCollection) {
	var Nearest = -1, Dist = Number.MAX_VALUE;
	var e = new Enumerator (shipCollection);
	for (var i=0; !e.atEnd(); e.moveNext(), i++) {
		var ship = e.item();
		
		// if (ship && ship.Team != Ship.Team)
		// {
			// Trace("ship = " + ship + "\n");
			// Trace("ship.BaseHullType = " + ship.BaseHullType + "\n");
			
			// if(ship.BaseHullType != null && typeof ship.BaseHullType.GetObjectType === "function")
				// Trace("ship.BaseHullType.GetObjectType = " + ship.BaseHullType.GetObjectType() + "\n");
			
			// if(ship.BaseHullType != null && typeof ship.BaseHullType.HasCapability !== "function")
				// Trace("typeof ship.BaseHullType.HasCapability = **** Not Present *****!\n");
			// else
				// Trace("typeof ship.BaseHullType.HasCapability = Ok\n");
			
			// Trace("CanCalculatePosition(ship) = " + CanCalculatePosition(ship) + "\n");
			// Trace("ShouldBeTargeted(ship) = " + ShouldBeTargeted(ship) + "\n");
		// }
		
        if (ship && ship.Team != Ship.Team && Ship.CanSee(ship) && CanCalculatePosition(ship) && ShouldBeTargeted(ship) ) { //c_habmLifepod
			var range = Range2Ship(ship);
			if (range < Dist) {
				Dist = range;
				Nearest = i;
			}
		}
	}
	return Nearest;
}

function FindNearestFriend(shipCollection) {
	var Nearest = -1, Dist = Number.MAX_VALUE;
	var e = new Enumerator (shipCollection);
	for (var i=0; !e.atEnd(); e.moveNext(), i++) {
		var ship = e.item();
		if (ship && ship.Team == Ship.Team && CanCalculatePosition(ship) && ShouldBeTargeted(ship)) { 
			var range = Range2Ship(ship);
			if (range < Dist) {
				Dist = range;
				Nearest = i;
			}
		}
	}
	return Nearest;
}

function FindNearestFriendlyStation(stationCollection) {
    var Nearest = -1, Dist = Number.MAX_VALUE;
    var e = new Enumerator(stationCollection);

    for (var i =0; !e.atEnd(); e.moveNext(), i++) {
        x = e.item();
        if (x.Team == Ship.Team && CanCalculatePosition(x)) {
            var range = Range2Ship(x);
            if (range < Dist) {
                Dist = range;
                Nearest = i;
            }
        }
    }
    return Nearest
}
function FindNearestEnemyStation(stationCollection) {
    var Nearest = -1, Dist = Number.MAX_VALUE;
    var e = new Enumerator(stationCollection);

    for (var i = 0; !e.atEnd(); e.moveNext(), i++) {
        x = e.item();
        if (x.Team != Ship.Team && CanCalculatePosition(x)) {
            var range = Range2Ship(x);
            if (range < Dist) {
                Dist = range;
                Nearest = i;
            }
        }
    }
    return Nearest
}

function IsTargetValid(shipCollection,objTarget) {
	var e = new Enumerator (shipCollection);
	for (; !e.atEnd(); e.moveNext()) {
		var ship = e.item();
		if (ship && ship.Team != Ship.Team && ShouldBeTargeted(ship) && ship.ObjectID == objTarget.ObjectID)
			return true;
	}
	return false;
}

function DoRipCord() {
  AllStop();
  FireMissile(false);
  FireWeapon(false);

  Ship.RipCord(true);
  CreateTimer(15, "EndRipCord()", 1, "RipcordTimer");
}
function EndRipCord() {
  Ship.RipCord(false);
  Timer.Kill();
}

var QUICKCHATBYCODE = {
    // These might not be correct.
    // They are out of quickchat.mdl !!! See sounds.h for alternative IDs
    "`ty": { "id": 115, "text": "Yessssss!" },
    "`tv": { "id": 113, "text": "You'll be sorry you did that" },
    "`tt": { "id": 114, "text": "You died with dignity." },
    "`tr": { "id": 112, "text": "Ripcord... Last resort for the desperate." },
    "`tq": { "id": 111, "text": "Payload delivered!" },
    "`tp": { "id": 185, "text": "You want a piece of me?" },
    "`to": { "id": 184, "text": "Oooh ... sorry about that one." },
    "`tn": { "id": 109, "text": "You have no honor." },
    "`tl": { "id": 110, "text": "Like lambs to the slaughter." },
    "`tf": { "id": 107, "text": "How did that feel?" },
    "`te": { "id": 106, "text": "Easier than shooting fish in a barrel." },
    "`td": { "id": 105, "text": "Death becomes you." },
    "`tc": { "id": 104, "text": "Come back and fight!" },
    "`ta": { "id": 102, "text": "Almost had you." },
    "`t1": { "id": 108, "text": "You're good but not that good" },

    "`9": { "id": 119, "text": "I'll be on your turret in a few seconds." },
    "`8": { "id": 118, "text": "I need turret gunners NOW!" },
    "`7": { "id": 117, "text": "Donate your money to the investor!" },
    "`6": { "id": 11, "text": "I need help!" },
    "`5": { "id": 10, "text": "Attack my target!" },
    "`4": { "id": 47, "text": "Yee-haaaaaw!" },
    "`3": { "id": 116, "text": "Shhhooot!" },
    "`2": { "id": 16, "text": "Negative!" },
    "`1": { "id": 15, "text": "Affirmative!" },
    "`0": { "id": 120, "text": "Oops" },
    "`yx": { "id": 48, "text": "Doah." },
    "`yq": { "id": 230, "text": "Ooooaaaah!" },
    "`yn": { "id": 50, "text": "Nooooo!" },
    "`ye": { "id": 231, "text": "Aaaaaaahhh!" },
    "`ya": { "id": 208, "text": "Deeg" }
};
var TAUNTS = [
    "`ty",
    "`tv",
    "`tt",
    "`tr",
    "`tq",
    "`tp",
    "`to",
    "`tn",
    "`tl",
    "`tf",
    "`te",
    "`td",
    "`tc",
    "`ta",
    "`t1"
];
var YELL = [
    "`yx",
    "`yq",
    "`yn",
    "`ye",
    "`ya"
];
function RandomTaunt() {
    var fRand = Random() % TAUNTS.length;
    return QUICKCHATBYCODE[TAUNTS[fRand]];
}
function RandomYell() {
    var fRand = Random() % YELL.length;
    return QUICKCHATBYCODE[YELL[fRand]];
}