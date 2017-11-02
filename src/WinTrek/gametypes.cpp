#include "pch.h"
#include "gametypes.h"

class GameTypeImpl : public GameType
{
    ZString m_strName;
    bool m_bAllowDevelopments;
    bool m_bInvulnerableStations;
    bool m_bEjectPods;
    short m_nLives;
    float m_fGoalTeamMoney;
    short m_iGoalConquestPercentage;
    short m_nGoalFlagsCount;
    short m_nGoalArtifactsCount;
    short m_nGoalTeamKills;
    short m_nGoalTerritory;
    float m_fGameLength;
	bool m_bExperimental; // mmf 10/07 Experimental game type
    MapMakerID m_mmMapType;

public:
    GameTypeImpl(
        const ZString& strName, 
        bool bAllowDevelopments, 
        bool bInvulnerableStations,
        bool bEjectPods,
        short nLives, 
        float fGoalTeamMoney,
        short iGoalConquestPercentage,
        short nGoalFlagsCount,
        short nGoalArtifactsCount,
        short nGoalTeamKills,
        short iGoalTerritory,
        float fGameLength,
	    bool bExperimental, // mmf 10/07 Experimental game type
		MapMakerID mmMapType
        ) :

        m_strName(strName),
        m_bAllowDevelopments(bAllowDevelopments),
        m_bInvulnerableStations(bInvulnerableStations),
        m_bEjectPods(bEjectPods),
        m_nLives(nLives),
        m_fGoalTeamMoney(fGoalTeamMoney),
        m_iGoalConquestPercentage(iGoalConquestPercentage),
        m_nGoalFlagsCount(nGoalFlagsCount),
        m_nGoalArtifactsCount(nGoalArtifactsCount),
        m_nGoalTeamKills(nGoalTeamKills),
        m_nGoalTerritory(iGoalTerritory),
        m_fGameLength(fGameLength),
		m_bExperimental(bExperimental), // mmf 10/07 Experimental game type
        m_mmMapType(mmMapType)
    {
    };

    virtual ZString GetName()
    {
        return m_strName;
    };

    virtual bool IsGameType(const MissionParams& misparams)
    {
        return (misparams.bAllowDevelopments == m_bAllowDevelopments)
            && (misparams.bInvulnerableStations == m_bInvulnerableStations)
            && (misparams.bEjectPods == m_bEjectPods)
            && ((misparams.iLives == c_cUnlimitedLives) == (m_nLives == c_cUnlimitedLives))
            && ((misparams.fGoalTeamMoney == 0) == (m_fGoalTeamMoney == 0))
            && ((misparams.iGoalConquestPercentage == 0) == (m_iGoalConquestPercentage == 0))
            && ((misparams.nGoalFlagsCount == 0) == (m_nGoalFlagsCount == 0))
            && ((misparams.nGoalArtifactsCount == 0) == (m_nGoalArtifactsCount == 0))
            && ((misparams.nGoalTeamKills == 0) == (m_nGoalTeamKills == 0))
            && ((misparams.iGoalTerritoryPercentage == 100) == (m_nGoalTerritory == 100))
            && ((misparams.dtGameLength == 0) == (m_fGameLength == 0))
			&& (misparams.bExperimental == m_bExperimental)  // mmf 11/07 Experimental game type
            //&& (misparams.bScoresCount == trekClient.GetIsZoneClub())
            ;

        // note: allow any map type.
    };

    virtual void Apply(MissionParams& misparams)
    {
        misparams.bAllowDevelopments = m_bAllowDevelopments;
        misparams.bInvulnerableStations = m_bInvulnerableStations;
        misparams.bEjectPods = m_bEjectPods;
        misparams.iLives = m_nLives;
        misparams.fGoalTeamMoney = m_fGoalTeamMoney;
        misparams.iGoalConquestPercentage = m_iGoalConquestPercentage;
        misparams.nGoalFlagsCount = m_nGoalFlagsCount;
        misparams.nGoalArtifactsCount = m_nGoalArtifactsCount;
        misparams.nGoalTeamKills = m_nGoalTeamKills;
        misparams.iGoalTerritoryPercentage = m_nGoalTerritory;
        misparams.dtGameLength = m_fGameLength;
		misparams.bExperimental = m_bExperimental; // mmf 10/07 Experimental game type
		misparams.mmMapType = m_mmMapType;
        //misparams.bScoresCount = misparams.bClubGame;
		misparams.bScoresCount = true;
 }
};

namespace 
{
    bool bGameTypesInitialized = false;
    TList<TRef<GameType> > listGameTypes;

    void VerifyGameTypeInitialization()
    {
        TRef<GameType> pNewGameType;
        if (!bGameTypesInitialized)
        {
			// mmf 10/07 if this list changes see Experimental game type comment in newgamescreen.cpp
            pNewGameType = new GameTypeImpl(
                "Conquest", true, false, true, c_cUnlimitedLives, 0, 100, 0, 0, 0, 100, 0, false, c_mmHiLo
                ); // mmf 10/07 added false experimental param
            listGameTypes.PushEnd(pNewGameType);

            pNewGameType = new GameTypeImpl(
                "Death Match", false, true, false, c_cUnlimitedLives, 0, 0, 0, 0, 20, 100, 0, false, c_mmBrawl
                ); // mmf 10/07 added false experimental param
            listGameTypes.PushEnd(pNewGameType);

            pNewGameType = new GameTypeImpl(
                "Capture the Flag", false, true, true, c_cUnlimitedLives, 0, 0, 10, 0, 0, 100, 0, false, c_mmHiLo
                ); // mmf 10/07 added false experimental param
            listGameTypes.PushEnd(pNewGameType);

            pNewGameType = new GameTypeImpl(
                "Artifacts", true, false, true, c_cUnlimitedLives, 0, 0, 0, 10, 0, 100, 0, false, c_mmPinWheel
                ); // mmf 10/07 added false experimental param
            listGameTypes.PushEnd(pNewGameType);

            pNewGameType = new GameTypeImpl(
                "Territorial", true, false, true, c_cUnlimitedLives, 0, 0, 0, 0, 0, 70, 0, false, c_mmGrid
                ); // mmf 10/07 added false experimental param
            listGameTypes.PushEnd(pNewGameType);

			// mmf 10/07 add new game type for trying things on the live servers 
            pNewGameType = new GameTypeImpl(
                "Experimental", true, false, true, c_cUnlimitedLives, 0, 100, 0, 0, 0, 100, 0, true, c_mmHiLo
                ); // mmf 10/07 added true experimental param
            listGameTypes.PushEnd(pNewGameType);

            bGameTypesInitialized = true;
        }
    }
};


TRef<GameType> GameType::FindType(const MissionParams& misparams)
{
    VerifyGameTypeInitialization();
    TList<TRef<GameType> >::Iterator gameTypesIter(listGameTypes);

    while (!gameTypesIter.End())
    {
        if (gameTypesIter.Value()->IsGameType(misparams))
        {
            return gameTypesIter.Value();
        }
        gameTypesIter.Next();
    }

    return NULL;
}

TList<TRef<GameType> >& GameType::GetGameTypes()
{
    VerifyGameTypeInitialization();
    return listGameTypes;
}
