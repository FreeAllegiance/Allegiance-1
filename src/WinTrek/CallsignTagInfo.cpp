#include "pch.h"

#include <inttypes.h> 

// BT - STEAM

#include "CallsignTagInfo.h"

CallsignHandler::CallsignHandler(const TRef<GameConfigurationWrapper>& pconfiguration, const std::vector<std::shared_ptr<CallsignSquad>>& squads) :
    m_pconfiguration(pconfiguration),
    m_squads(squads)
{
}

ZString CallsignHandler::CleanupCallsign(ZString callsign)
{
    std::string s = std::string(callsign);

    s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
        if (c == '_') {
            return false;
        }
        if (isalnum(c) == false) {
            return true;
        }
        return false;
    }), s.end());

    //if it's too small, do something with it
    if (s.size() <= 2) {
        return ZString("Pilot_") + ZString(s.c_str());
    }

    return s.c_str();
}

std::vector<std::shared_ptr<CallsignSquad>> CallsignHandler::GetAvailableSquads()
{
    return m_squads;
}

std::shared_ptr<CallsignSquad> CallsignHandler::GetSquadForTag(ZString tag)
{
    if (tag == "") {
        return nullptr;
    }

    std::vector<ZString> vectorAllowedTokens;
    for (const std::shared_ptr<CallsignSquad>& squad : GetAvailableSquads()) {
        if (squad->GetCleanedTag() == tag) {
            return squad;
        }
    }

    return nullptr;
}

TRef<StringValue> CallsignHandler::GetCleanedFullCallsign()
{
    return new TransformedValue<ZString, ZString, ZString, ZString>([this](ZString strCallsign, ZString strSquad, ZString strToken) {
        ZString strCallsignPart = CallsignHandler::CleanupCallsign(strCallsign);

        ZString strSquadPart = "";
        std::vector<ZString> vectorAllowedTokens;

        std::shared_ptr<CallsignSquad> squad = GetSquadForTag(strSquad);

        if (squad != nullptr) {
            std::vector<ZString> vectorSquadTokens;
            vectorAllowedTokens.insert(vectorAllowedTokens.end(), std::begin(squad->GetAvailableOfficerTokens()), std::end(squad->GetAvailableOfficerTokens()));
            strSquadPart = "@" + strSquad;
        }

        ZString strTokenPart = "";
        if (strToken != "") {
            for (ZString allowedToken : vectorAllowedTokens) {
                if (allowedToken == strToken) {
                    strTokenPart = strToken;
                }
            }
        }

        return strTokenPart + strCallsignPart + strSquadPart;
    }, m_pconfiguration->GetOnlineCharacterName(), m_pconfiguration->GetOnlineSquadTag(), m_pconfiguration->GetOnlineOfficerToken());
}

ZString FixupCallsignTag(ZString callsignTag)
{
    ZString returnValue = callsignTag;

    // It's tradition!
    if (returnValue == "MsAlleg")
        return "Alleg";

    // Give our older members a nice badge of honor.
    if (returnValue == "AllegFAO")
        return "Vet";

    // Allows us to have some group tags that may have already been claimed.
    if (returnValue.Find("@") >= 0)
        returnValue = returnValue.RightOf("@");

    if (returnValue.Find("#") >= 0)
        returnValue = returnValue.RightOf("#");

    if (returnValue.Find("!") >= 0)
        returnValue = returnValue.RightOf("!");

    if (returnValue.Find("&") >= 0)
        returnValue = returnValue.RightOf("&");

    if (returnValue.Find("%") >= 0)
        returnValue = returnValue.RightOf("%");

    if (returnValue.Find("^") >= 0)
        returnValue = returnValue.RightOf("^");

    if (returnValue.Find("*") >= 0)
        returnValue = returnValue.RightOf("*");

    if (returnValue.Find("$") >= 0)
        returnValue = returnValue.RightOf("$");

    if (returnValue == "Alleg" || returnValue == "Vet")
        return "Fail";

    return returnValue;
}

ZString CallsignSquad::CleanupSquadTag(ZString tag)
{
    std::string s = std::string(tag);

    s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
        if (isalnum(c) == false) {
            return true;
        }
        return false;
    }), s.end());

    return ZString(s.c_str());
}

CallsignSquad::CallsignSquad(ZString tag, std::vector<ZString> vAvailableTokens) :
    m_tag(tag),
    m_availableTokens(vAvailableTokens)
{
}

const std::vector<ZString>& CallsignSquad::GetAvailableOfficerTokens() const
{
    return m_availableTokens;
}

ZString CallsignSquad::GetCleanedTag() const
{
    return CallsignSquad::CleanupSquadTag(m_tag);
}

std::shared_ptr<CallsignSquad> CreateSquadFromSteam(CSteamID squadSteamClanId, CSteamID userSteamId)
{
    ZString strSteamTag = FixupCallsignTag(SteamFriends()->GetClanTag(squadSteamClanId));

    bool bIsOfficer = false;

    // If the user is on the officer's list, then they are an officer.
    int officerCount = SteamFriends()->GetClanOfficerCount(squadSteamClanId);
    for (int j = 0; j < officerCount; j++)
    {
        if (SteamFriends()->GetClanOfficerByIndex(squadSteamClanId, j) == userSteamId)
        {
            bIsOfficer = true;
            break;
        }
    }

    // If the user is the group owner, then they are an officer.
    if (userSteamId == SteamFriends()->GetClanOwner(squadSteamClanId))
    {
        bIsOfficer = true;
    }

    if (strSteamTag == "Dev" || strSteamTag == "Alleg")
    {
        bIsOfficer = true;
    }

    std::vector<ZString> tokens = std::vector<ZString>({});
    if (bIsOfficer) {
        tokens.push_back("*");
        tokens.push_back("^");
    }

    ZString token_debug_string = "";
    for (ZString token : tokens) {
        token_debug_string += token;
    }

    debugf("Finding squad tags. Added tag. fixed_tag=%s, tokens=%s", (const char*)strSteamTag, (const char*)token_debug_string);

    return std::make_shared<CallsignSquad>(strSteamTag, tokens);
}

std::shared_ptr<CallsignHandler> CreateCallsignHandlerFromSteam(const TRef<GameConfigurationWrapper>& pconfiguration)
{
    int addedItemCount = 0;

    CSteamID currentUser;
    if (SteamUser() != nullptr)
        currentUser = SteamUser()->GetSteamID();

    int nGroupsFound = SteamFriends()->GetClanCount();
    int nGroups = std::min(20, nGroupsFound); //only do the first 20

    debugf("Finding squad tags. %d groups found, iterating over %d groups (possibly limit reached)", nGroupsFound, nGroups);

    std::vector<std::shared_ptr<CallsignSquad>> vSquads;
    for (int i = 0; i < nGroups; ++i)
    {
        CSteamID groupSteamID = SteamFriends()->GetClanByIndex(i);
        ZString szGroupName = SteamFriends()->GetClanName(groupSteamID);
        ZString szGroupTag = SteamFriends()->GetClanTag(groupSteamID);

        debugf("Finding squad tags. Found name=%s, tag=%s", (const char*)szGroupName, (const char*)szGroupTag);

        // Don't do the expensive officer call unless there is actually a tag associated with the group.
        if (szGroupTag.GetLength() == 0) {
            debugf("Finding squad tags. Tag empty, skipping");
            continue;
        }
        
        vSquads.push_back(CreateSquadFromSteam(groupSteamID, currentUser));
    }

    return std::make_shared<CallsignHandler>(pconfiguration, vSquads);
}
