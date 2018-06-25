#pragma once

#include "WinTrek.H"

class CallsignSquad {

private:
    ZString m_tag;
    std::vector<ZString> m_availableTokens;
    static ZString CleanupSquadTag(ZString tag);
public:
    CallsignSquad(ZString tag, std::vector<ZString> vAvailableTokens);

    const std::vector<ZString>& GetAvailableOfficerTokens() const;

    ZString GetCleanedTag() const;
};

std::shared_ptr<CallsignSquad> CreateSquadFromSteam(CSteamID squadSteamClanId, CSteamID userSteamId);

class CallsignHandler {
private:
    TRef<GameConfigurationWrapper> m_pconfiguration;
    std::vector<std::shared_ptr<CallsignSquad>> m_squads;

    static ZString CleanupCallsign(ZString callsign);

public:
    CallsignHandler(const TRef<GameConfigurationWrapper>& pconfiguration, const std::vector<std::shared_ptr<CallsignSquad>>& squads);

    std::vector<std::shared_ptr<CallsignSquad>> GetAvailableSquads();

    std::shared_ptr<CallsignSquad> GetSquadForTag(ZString tag);

    TRef<StringValue> GetCleanedFullCallsign();

    static bool IsValidCallsign(ZString callsign);
};

std::shared_ptr<CallsignHandler> CreateCallsignHandlerFromSteam(const TRef<GameConfigurationWrapper>& pconfiguration);

