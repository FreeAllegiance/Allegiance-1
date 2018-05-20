<pigbehavior language="JScript" basebehavior="Default">

<script src="include\common.js"></script>
<script src="include\AutoStartGame.js"/>

<script>
<![CDATA[
/////////////////////////////////////////////////////////////////////////////
// Beginning of script
//
// Outside of the right margin, I have shown an alternate syntax for calling
// the properties/methods of the ActivePigs objects. The point is merely to
// demonstrate that several of the ActivePigs objects are in the global
// name space of the script. These objects are:
//
//     This - The currently running behavior object for which this script
//            defines the actions of the pig.
//
//     Pig  - The pig object upon which this behavior is assigned.
//
//     Host - An object that acts as this behavior's entry into the
//            ActivePigs object model. This can also be thought of as an
//            /internal/ session object.
//


var nCorrectionInterval = 2.0;
var fTimerOn = true;
var fHostile = true;
var nThrottle = 100;
var strTarget = "+BackTrak@Dev";
var strOldTarget = "";
var mySector, myShip, myTarget;
var nShieldValue, nHullValue;
var visitedSectors;

/////////////////////////////////////////////////////////////////////////////
// Handles state transition. Joins or creates a mission.
function OnStateMissionList(eStatePrevious)
{
  visitedSectors = new ActiveXObject("Scripting.Dictionary");

  // Attempt to join the current pig mission, if any
  try
  {
    JoinMission("Pig Mission");                                            // Pig.JoinMission
    return;
  }
  catch (e)
  {
  }

  // Attempt to join Grendel's game, if it's there
  try
  {
    JoinMission("Grendel's game");                                            // Pig.JoinMission
    return;
  }
  catch (e)
  {
  }

  // Attempt to join pig007's game, if it's there
  try
  {
    JoinMission("pig007's game");                                            // Pig.JoinMission
    return;
  }
  catch (e)
  {
  }

  // Mission didn't exist, join any other current mission, if any
  try
  {
    JoinMission();                                                         // Pig.JoinMission
    return;
  }
  catch (e)
  {
  }

  // No missions exist, create one and join it
  var objParams = new ActiveXObject("Pigs.MissionParams");
  objParams.TeamCount = 2;
  objParams.Validate();
  CreateMission(objParams);                                                // Pig.CreateMission

  // Automatically start game when the minimum players per team have joined
  AutoStartGame(objParams.MinTeamPlayers);
}


/////////////////////////////////////////////////////////////////////////////
// Handles state transition. Joins a random team.
function OnStateTeamList(eStatePrevious)
{
  // Avoid repeated attempts to join a team
  if (PigState_JoiningTeam != eStatePrevious)
  {
    try
    {
      JoinTeam();                                                          // Pig.JoinTeam
    }
    catch (e)
    {
      Trace("\n" + e.description + " - Attempting to QuitGame\n");         // Host.Trace
      QuitGame();
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Handles state transition. Launches the pig as soon as it becomes docked.
function OnStateDocked(ePrev)
{
  // Kill the flying timer, if one exists
  if ("object" == typeof(Properties("FlyingTimer")))
    Properties("FlyingTimer").Kill();

  try
  {
    // Get the list of hull types
    var objHullTypes = HullTypes;                                        // Pig.HullTypes (should be Pig.Station.HullTypes)
    var cHullTypes = objHullTypes.Count;
    Host.Trace("\nOnStateDocked: cHullTypes = " + cHullTypes + "\n");    // Host.Trace

    // Buy the best hull (or else we'll be in an eject pod)
    var v = SelectBestHull(objHullTypes, "Interceptor", "Scout");
    if (cHullTypes)
      Ship.BuyHull(objHullTypes(v))                                      // Pig.Ship.BuyHull
  }
  catch (e)
  {
    Trace("\n" + e.description + "\n");                                  // Host.Trace
  }

  // As soon as the mission starts or we join a team, we launch
  if (PigState_WaitingForMission == ePrev || PigState_JoiningTeam == ePrev)
  {
    // Launch into space!
    Launch();                                                              // Pig.Launch
  }
  else if (PigState_Flying == ePrev)
  {
	// If we were flying before, we need to schedule the Launch method
	CreateTimer(5.0, "OnDoLaunch();", -1, "LaunchTimer");
  }
}

/////////////////////////////////////////////////////////////////////////////
function WhoAmI()
{
  var shiplist = Game.Ships;
  var x;
  Host.Trace("*********************************************\n");
  var e = new Enumerator (shiplist);
  for(; !e.atEnd(); e.moveNext())
  {
    x = e.item();
    Host.Trace("ship: " + x.Name + "\n");
    if (x.Name == Name)
      myShip = x;
  }
  if (myShip)
    mySector = myShip.Sector;
  Host.Trace("my ship name is " + myShip + "\n");
  Host.Trace("my sector name is " + mySector + "\n");
}

/////////////////////////////////////////////////////////////////////////////
function FindTarget(strTargetName)
{
  if (strTarget == strOldTarget)
    return;
  else
    strOldTarget = strTarget;
  var shiplist = Game.Ships;
  var stationlist = Game.Stations;
  var x, result = null;
  Host.Trace("*********** Find Target **********************************\n");
  var e = new Enumerator (shiplist);
  for(; !e.atEnd(); e.moveNext())
  {
    x = e.item();
    Host.Trace("ship: " + x.Name + "\n");
    if (x.Name == strTargetName)
      result = x;
  }
  if (result != null)
  {
    myTarget = result;
    Host.Trace("target ship found: " + result + myTarget.Position + "\n");
    //Game.SendMsg("target ship found: " + result + "\n");
    Host.Trace("*********** Find Target End **********************************\n");
    return (result);
  }
  Host.Trace("%%%%%%%%%%%%%% ship not found %%%%%%%%%%\n");
  e = new Enumerator (stationlist);
  for(; !e.atEnd(); e.moveNext())
  {
    x = e.item();
    Host.Trace("station: " + x.Name + "\n");
    if (x.Name == strTargetName)
      result = x;
  }
  if (result != null)
  {
    Host.Trace("target station found: " + result + "\n");
    myTarget = result;
    Host.Trace("*********** Find Target End **********************************\n");
    return (result);
  }
  Host.Trace("*********** Find Target End Not Found **********************************\n");
  return (null);
}

/////////////////////////////////////////////////////////////////////////////
// Handles state transition. Currently just outputs debug text.
function OnStateFlying(eStatePrevious)
{
  // Set the ship's throttle to 100%
  Throttle = 100;                                                          // Pig.Ship.Throttle

  // Create a timer to reset the throttle
  var fDuration = 5.0 + (Random() % 10);
  CreateTimer(fDuration, "OnInitialThrottle()", -1, "FlyingTimer");             // This.CreateTimer
}

/////////////////////////////////////////////////////////////////////////////
function OnInitialThrottle()
{

  WhoAmI();
  // Set the ship's throttle to 0%
  //Ship.Throttle = 0;                                                       // Pig.Ship.Throttle

  // Set the timer's interval to 2 seconds
  Timer.Interval = nCorrectionInterval;
  Timer.ExpirationExpression = "CorrectOrbit()";
}


/////////////////////////////////////////////////////////////////////////////
function CorrectOrbit()
{
  try
  {
    Ship.Face(strTarget, "ThrustOrbit();");
    if (fHostile)
      nCorrectionInterval = 1.0;              
    else
      nCorrectionInterval = 2.0;
    FindTarget(strTarget);
  }
  catch (e)
  {
    //Ship.Face("Outpost", "ThrustOrbit();");
    Host.Trace ("$$$$$$$$$$$$$$$$$ bad stuff!! $$$$$$$$$$$$$$$$$$$$$\n");
	Host.Trace (e.number + ", " + e.description + "\n");
    AllStop();
		FireWeapon(false);
		FireMissile(false);
    nCorrectionInterval = 5.0;
  }
  if (fTimerOn)
    Timer.Interval = nCorrectionInterval;
}

function ProperVelocity()
{
  var basevelocity = myTarget.Velocity.Length;
  var mythrottle = 0;
  var mythrust = ThrustForward;
  var range = Range2Ship(myTarget);

  if (range < 62500)
  {
    mythrottle = 20;
    mythrust = ThrustLeft;
    if (fHostile)
      FireWeapon(true);
  }
  else if (range < 202500)
  {
    mythrottle = 60;
    mythrust = ThrustForward;
    if (fHostile)
      FireWeapon(true);
  }
  else
  {
    mythrottle = 100;
    mythrust = ThrustForward;
  }

  if (mythrottle < basevelocity || basevelocity > 90)
  {
    mythrottle = basevelocity;
    mythrust = ThrustForward;
  }

  Throttle = mythrottle;
  Thrust(ThrustForward, mythrust);
}   

/////////////////////////////////////////////////////////////////////////////
function ThrustOrbit()
{
	try
	{

    ProperVelocity();

		// Check for empty Ammo
		if (!Ammo)
			CommitSuicide();
	}
	catch (e)
	{
    Host.Echo("*************** Something's Wrong!!!! ***********************\n");
	}
}



// End of script
/////////////////////////////////////////////////////////////////////////////
]]>
</script>

</pigbehavior>

