#include "pch.h"
#include "SteamClans.h"

// BT - STEAM
#ifdef STEAM_APP_ID

SteamClans::SteamClans()
{
	int addedItemCount = 0;

	CSteamID currentUser;
	if (SteamUser() != nullptr)
		currentUser = SteamUser()->GetSteamID();

	int nGroups = SteamFriends()->GetClanCount();
	int nGroupsFound = 0;
	for (int i = 0; i < nGroups && nGroupsFound < 10; ++i)
	{
		CSteamID groupSteamID = SteamFriends()->GetClanByIndex(i);
		ZString szGroupName = SteamFriends()->GetClanName(groupSteamID);
		ZString szGroupTag = SteamFriends()->GetClanTag(groupSteamID);

		// Don't do the expensive officer call unless there is actually a tag associated with the group.
		if(szGroupTag.GetLength() == 0)
			continue;

		nGroupsFound++;

		m_officerDataReceived = false;
		SteamAPICall_t handle = SteamFriends()->RequestClanOfficerList(groupSteamID);
		m_SteamCallResultClanOfficerListResponse.Set(handle, this, &SteamClans::OnClanOfficerListResponse);

		for (int i = 0; i < 30 && m_officerDataReceived == false; i++)
		{
			SteamAPI_RunCallbacks();
			Sleep(100);
		}

		if (m_officerDataReceived == true)
		{

			int nOfficers = SteamFriends()->GetClanOfficerCount(groupSteamID);
			bool isOfficer = false;
			for (int j = 0; j < nOfficers; j++)
			{
				if (currentUser == SteamFriends()->GetClanOfficerByIndex(groupSteamID, j))
				{
					isOfficer = true;
					break;
				}
			}

			CallsignTagInfo callsignInfo(szGroupTag, groupSteamID.ConvertToUint64(), addedItemCount, isOfficer);

			if (callsignInfo.m_callsignTag.GetLength() > 0 && m_availableCallsignTags.Find(callsignInfo) < 0)
			{
				m_availableCallsignTags.PushEnd(callsignInfo);
				addedItemCount++;
			}
		}
	}
}

void SteamClans::OnClanOfficerListResponse(ClanOfficerListResponse_t *pCallback, bool bIOFailure)
{
	m_officerDataReceived = true;
}

TVector<CallsignTagInfo, DefaultEquals, DefaultCompare> * SteamClans::GetAvailableCallsignTags()
{
	return &m_availableCallsignTags;
}

#endif
