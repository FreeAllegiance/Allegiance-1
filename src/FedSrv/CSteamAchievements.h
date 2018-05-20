#pragma once

// Note, even if you remove a value from this list, you must NEVER remove it from m_Achievements
// or the Enum to string assignments will not line up, and you will be handing out wrong achievements!
enum EAchievements
{
	BETA_ACHIEVEMENT_1_0 = 0,
	FIRST_MINER_KILL_1_1 = 1,
	FIRST_CON_KILL_1_2 = 2,
	FIRST_FORCE_EJECT_1_3 = 3,
	FIRST_BASE_KILL_1_4 = 4,
	FIRST_BASE_CAP_1_5 = 5,
	RANK_5_1_6 = 6,
	RANK_10_1_7 = 7,
	RECOVER_TECH_1_8 = 8,
	FIRST_PROBE_KILL_1_9 = 9,
	PROBE_SPOT_1_10 = 10,
    NANITE_REPAIR_1_11 = 11,
	GET_RESCUED_1_12 = 12,
	PICKUP_POD_1_13 = 13,
	SPOT_GARRISON_1_14 = 14,
	FORCE_100_EJECTS_1_15 = 15,
	KILL_50_MINERS_1_16 = 16,
	WIN_10_GAMES_1_17 = 17,
	WIN_50_GAMES_1_18 = 18,
	WIN_100_GAMES_1_19 = 19

	
	// Don't forget to update g_nMaximumSteamAchievementCount and m_Achievements!
};

// Keep in sync with m_Stats!
enum EStats
{
	MINER_KILLS = 0,
	CON_KILLS = 1,
	FORCE_EJECT = 2,
	BASE_KILLS = 3,
	BASE_CAPS = 4,
	SUM_SCORE = 5,
	PLAYER_RANK = 6,
	PLAYER_WINS = 7,
	PLAYER_LOSS = 8,
	REPAIR_PERCENT = 9,
	COMM_ELO = 10,
	COMM_GAMES = 11

	// Don't forget to update g_nMaximumSteamStatCount and m_Stats!
};
const int g_nMaximumSteamAchievementCount = 20; // Always keep this in sync with the number of achievments in EAchievements!
const int g_nMaximumSteamStatCount = 12; // Always keep this in sync with the number of stats in EStats!

// BT - STEAM
                            // 0   1    2    3      4  
const int RANK_REQUIREMENTS[51] = { 0, 300, 690, 1197, 1856, 2713, 3827, 5275, 7157, 9604, 12786, 16922, 22298, 29288, 38374, 50186, 65542, 85504, 111455, 145192, 189050, 246065, 320184, 416539,
541801, 704641, 916333, 1191533, 1549293, 2014381, 2618996, 3404994, 4426793, 5755130, 7481970, 9726860, 12645219, 16439084, 21371109, 27782742, 36117865, 46953524, 61039882, 79352146, 103158090,
134105817, 174337862, 226639520, 294631676, 383021479, 497928223 };

class CSteamAchievements
{
private:
	CSteamID	m_steamID;
	char		m_szSteamID[64];
	bool		m_gotRequestStatsResponse;
	bool		m_gotSuccessfulRequestStatsResponse;
	bool		m_gotStatsStoredResponse;
	bool		m_gotSuccessfulStatsStoredResponse;
    bool        m_nanAchievementEarned;

	CCallResult< CSteamAchievements, GSStatsReceived_t > m_UserStatsRequestedCallResult;
	CCallResult< CSteamAchievements, GSStatsStored_t > m_UserStatsStoredCallResult;

	// Keep in sync with EAchievements
	const char * m_Achievements[g_nMaximumSteamAchievementCount] =
	{
		// Never remove an item from this list! (see note above).
		"BETA_ACHIEVEMENT_1_0",
		"FIRST_MINER_KILL_1_1",
		"FIRST_CON_KILL_1_2",
		"FIRST_FORCE_EJECT_1_3",
		"FIRST_BASE_KILL_1_4",
		"FIRST_BASE_CAP_1_5",
		"RANK_5_1_6",
		"RANK_10_1_7",
		"RECOVER_TECH_1_8",
		"FIRST_PROBE_KILL_1_9",
		"PROBE_SPOT_1_10",
        "NANITE_REPAIR_1_11",
		"GET_RESCUED_1_12",
		"PICKUP_POD_1_13",
		"SPOT_GARRISON_1_14",
		"FORCE_100_EJECTS_1_15",
		"KILL_50_MINERS_1_16",
		"WIN_10_GAMES_1_17",
		"WIN_50_GAMES_1_18",
		"WIN_100_GAMES_1_19",
	};

	// Keep in sync with EStats! 
	const char * m_Stats[g_nMaximumSteamStatCount] =
	{
		// Never remove an item from this list! (see note above).
		"MINER_KILLS",
		"CON_KILLS",
		"FORCE_EJECT",
		"BASE_KILLS",
		"BASE_CAPS",
		"SUM_SCORE",
		"PLAYER_RANK",
		"PLAYER_WINS",
		"PLAYER_LOSS",
		"REPAIR_PERCENT",
		"COMM_ELO",
		"COMM_GAMES"
	};

	bool GetAchievement(EAchievements achievement);
	bool SetAchievement(EAchievements achievement);

	bool GetStat(EStats theStat, int * pVal);
	bool SetStat(EStats theStat, int val);

	bool InitiateStatsRequestAndWaitForStatsFromSteamServer();
	bool CheckRank(int currentScore);

	

public:
	CSteamAchievements(CSteamID &steamID);


	void InitiateStatsRequest();

	RankID GetRank();

	bool RemoveAchievement(EAchievements achievement);

	void AwardBetaParticipation();
	void AwardKillAchievement(PilotType pt);
	void AwardBaseKillOrCapture(bool kill);
	void AwardIGCAchievements(AchievementMask am);
	void AwardRecoverTechAchievement();
	void AwardPodPickup();
	void AwardGetRescued();
	
	void AddUserStats(PlayerScoreObject*  ppso, IshipIGC* pIship);
	void UpdateCommanderStats(int opponentELO, bool win);
	int GetCommELO();

	void UpdateLeaderboard(PlayerScoreObject*  ppso);
	
	bool SaveStats();


	// Steam Call Results
	void OnUserStatsStored(GSStatsStored_t *pCallback, bool bIOFailure);
	void OnUserStatsReceived(GSStatsReceived_t *pCallback, bool bIOFailure);
	
};


