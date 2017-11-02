#include "pch.h"

#include <inttypes.h> 

// BT - STEAM

#include "CallsignTagInfo.h"



CallsignTagInfo::CallsignTagInfo(ZString callsignTag, uint64 steamGroupID, int index, bool isOfficer) :
	m_callsignTag(callsignTag),
	m_steamGroupID(steamGroupID),
	m_index(index),
	m_isOfficer(isOfficer)
{
	m_callsignTag = FixupCallsignTag(m_callsignTag);
}

//CallsignTagInfo::CallsignTagInfo(CSteamID steamGroupID)
//{
//	
//}

CallsignTagInfo::CallsignTagInfo()
{
	m_callsignTag = "";
	m_steamGroupID = 0;
	m_index = -1;

	LoadFromRegistry();
}

ZString CallsignTagInfo::FixupCallsignTag(ZString callsignTag)
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

ZString CallsignTagInfo::Render(ZString callsign)
{
	ZString returnValue = callsign;

	if (m_isOfficer == true)
		returnValue = m_callsignToken + returnValue;

	if (m_callsignTag.GetLength() > 0)
		returnValue = returnValue + ZString("@") + m_callsignTag;

	if (returnValue.GetLength() > 24)
		returnValue = returnValue.Left(24);

	return returnValue;
}


void CallsignTagInfo::LoadFromRegistry()
{
	char szSteamClanID[120];
	szSteamClanID[0] = '\0';
	DWORD cbClanID = sizeof(szSteamClanID);

	char szSteamOfficerToken[5];
	szSteamOfficerToken[0] = '\0';
	DWORD cbSteamOfficerToken = sizeof(szSteamOfficerToken);

	HKEY hKey;
	DWORD dwType;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ, &hKey))
	{
		RegQueryValueEx(hKey, "SteamClanID", NULL, &dwType, (BYTE *)&szSteamClanID, &cbClanID);
		RegQueryValueEx(hKey, "SteamOfficerToken", NULL, &dwType, (BYTE *)&szSteamOfficerToken, &cbSteamOfficerToken);
		RegCloseKey(hKey);
	}
#ifdef STEAM_APP_ID
	CSteamID targetGroupID(strtoull(szSteamClanID, NULL, NULL));

	CSteamID currentUser = SteamUser()->GetSteamID();

	int clanCount = SteamFriends()->GetClanCount();
	for (int i = 0; i < clanCount; i++)
	{
		if (targetGroupID == SteamFriends()->GetClanByIndex(i))
		{
			m_steamGroupID = targetGroupID.ConvertToUint64();
			break;
		}
	}

	UpdateStringValues(szSteamOfficerToken);
#endif
}

void CallsignTagInfo::SaveToRegistry()
{
	char steamGroupID[64];
	sprintf(steamGroupID, "%" PRIu64, m_steamGroupID);

	char token[5];
	sprintf(token, m_callsignToken);

	HKEY hKey;
	DWORD dwType;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ | KEY_WRITE, &hKey))
	{
		RegSetValueEx(hKey, "SteamClanID", 0, REG_SZ, (BYTE * )steamGroupID, sizeof(steamGroupID));
		RegSetValueEx(hKey, "SteamOfficerToken", 0, REG_SZ, (BYTE *) token, sizeof(token));

		RegCloseKey(hKey);
	}

	UpdateStringValues(m_callsignToken);
}

void CallsignTagInfo::UpdateStringValues(ZString selectedToken)
{
	m_isOfficer = false;
	//m_callsignTag = "";
	//m_index = 0;

	CSteamID currentUser = SteamUser()->GetSteamID();

	m_callsignTag = FixupCallsignTag(SteamFriends()->GetClanTag(m_steamGroupID));

	// If the user is on the officer's list, then they are an officer.
	int officerCount = SteamFriends()->GetClanOfficerCount(m_steamGroupID);
	for (int j = 0; j < officerCount; j++)
	{
		if (SteamFriends()->GetClanOfficerByIndex(m_steamGroupID, j) == currentUser)
		{
			m_isOfficer = true;
			break;
		}
	}

	// If the user is the group owner, then they are an officer.
	if (currentUser == SteamFriends()->GetClanOwner(m_steamGroupID))
	{
		m_isOfficer = true;
	}

	ZString groupTag = FixupCallsignTag(SteamFriends()->GetClanTag(m_steamGroupID));
	if (groupTag == "Dev" || groupTag == "Alleg")
	{
		m_isOfficer = true;
	}

	if (m_isOfficer == true)
	{
		ZString tokens = GetAvailableTokens();
		if (tokens.Find(selectedToken) >= 0)
			m_callsignToken = selectedToken;
		else
			m_callsignToken = tokens.Middle(tokens.GetLength() - 1, 1);
	}
}

ZString CallsignTagInfo::GetAvailableTokens()
{
	if (m_callsignTag == "Alleg" && m_isOfficer == false)
	{
		return "?";
	}
	else if (m_callsignTag == "Dev")
	{
		return "+";
	}
	else if (m_isOfficer == true) // Steam doesn't let us determine a Squad Leader vs an ASL.
	{
		return "*^";
	}
	else
	{
		return "";
	}
}

void CallsignTagInfo::SetSteamGroupID(uint64 steamGroupID, ZString callsignTag)
{
	m_steamGroupID = steamGroupID;
	m_callsignTag = "";
	m_callsignToken = "";
	SaveToRegistry();
}

void CallsignTagInfo::SetToken(ZString token)
{
	m_callsignToken = token;
	SaveToRegistry();
}