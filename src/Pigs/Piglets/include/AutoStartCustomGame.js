//imagotrigger@gmail.com

var MissionParams;

function AutoStartGame(objParams) {
	if (PigState_WaitingForMission != PigState)
		return;
	if (objParams)
		MissionParams = objParams;
	if (Properties.Exists("AutoStartGame"))
		Timer.Kill();
	CreateTimer(1.0, "AutoStartGame_Tick()", -1, "AutoStartGameTimer");
}

function AutoStartGame_Tick() {
	for (var it = new Enumerator(Game.Teams); !it.atEnd(); it.moveNext())
		if (it.item().Ships.Count < MissionParams.MinTeamPlayers)
	  		return;
	Timer.Kill();
	Game.SendChat("Good luck and have fun.",1296); //voEveryoneReadySound
	CreateTimer(10.0, "AutoStartGameDelay()", -1, "AutoStartGameDelay");
}

function AutoStartGameDelay() {
	GameController = IsMissionOwner();
	if (GameController && "object" != typeof(Properties("ChatStartGameTimer")) && "object" != typeof(Properties("StartGameTimer"))) {
		for (var it = new Enumerator(Game.Teams); !it.atEnd(); it.moveNext()) {
			if (it.item().Ships.Count < MissionParams.MinTeamPlayers) {
				Game.SendChat("Aborting launch...",1301); //voJustASecSound
				Timer.Kill();
				CreateTimer(1.0, "AutoStartGame_Tick()", -1, "AutoStartGameTimer");
				return;
			}
		}
		Timer.Kill();
		Trace("StartCustomGame....\n");
		StartCustomGame(MissionParams);
		CreateTimer(20.0, "AutoStartGame()", -1, "AutoStartGame");
	}
	Timer.Kill();
}