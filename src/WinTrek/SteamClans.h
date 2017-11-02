#pragma once
#ifdef STEAM_APP_ID

// BT - STEAM

class SteamClans
{
private:
	TVector<CallsignTagInfo, DefaultEquals, DefaultCompare>	m_availableCallsignTags; 
	bool m_officerDataReceived = false;

	void OnClanOfficerListResponse(ClanOfficerListResponse_t *pCallback, bool bIOFailure);
	CCallResult< SteamClans, ClanOfficerListResponse_t > m_SteamCallResultClanOfficerListResponse;

public:
	SteamClans();

	TVector<CallsignTagInfo, DefaultEquals, DefaultCompare> * GetAvailableCallsignTags();
};

#endif