/////////////////////////////////////////////////////////////////////////////
// PigMissionParams.cpp : Implementation of the CPigMissionParams class.

#include "pch.h"
#include <TCLib.h>
#include "PigMissionParams.h"


/////////////////////////////////////////////////////////////////////////////
// Begin: Allegiance Initialization                                        //
                                                                           //
HKEY g_dwArtHive = HKEY_LOCAL_MACHINE;                                     //
char g_szArtReg[] =                                                        //
  "SOFTWARE\\Classes\\AppID\\{F132B4E3-C6EF-11D2-85C9-00C04F68DEB0}";      //
                                                                           //
                                                                           //
// End: Allegiance Initialization                                          //
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CPigMissionParams

TC_OBJECT_EXTERN_IMPL(CPigMissionParams)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigMissionParams::CPigMissionParams()
{
}

HRESULT CPigMissionParams::FinalConstruct()
{
  // Initialize the data
  InitNew();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigMissionParams::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigMissionParams,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IPigMissionParams Interface Methods

STDMETHODIMP CPigMissionParams::Validate()
{
  XLock lock(this);
  if (2 > m_mp.nTeams || m_mp.nTeams > 6)
    return Error(IDS_E_TEAMCOUNT, IID_IPigMissionParams);
  if (1 > m_mp.nMaxPlayersPerTeam || m_mp.nMaxPlayersPerTeam > 100) //imago 10/14 max and min were switched around!
    return Error(IDS_E_MAXTEAMPLAYERS, IID_IPigMissionParams);
  if (1 > m_mp.nMinPlayersPerTeam || m_mp.nMinPlayersPerTeam > 50)
    return Error(IDS_E_MINTEAMPLAYERS, IID_IPigMissionParams);
  if (m_mp.nMinPlayersPerTeam > m_mp.nMaxPlayersPerTeam)
    return Error(IDS_E_MINMAXREVERSED, IID_IPigMissionParams);
  return S_OK;
}

STDMETHODIMP CPigMissionParams::put_TeamCount(short nTeamCount)
{
  return TCComPropertyPut(this, m_mp.nTeams, nTeamCount, dispid_TeamCount);
}

STDMETHODIMP CPigMissionParams::get_TeamCount(short* pnTeamCount)
{
  return TCComPropertyGet(this, pnTeamCount, m_mp.nTeams);
}

STDMETHODIMP CPigMissionParams::put_MaxTeamPlayers(short nMaxTeamPlayers)
{
  return TCComPropertyPut(this, m_mp.nMaxPlayersPerTeam, nMaxTeamPlayers, dispid_TeamCount);
}

STDMETHODIMP CPigMissionParams::get_MaxTeamPlayers(short* pnMaxTeamPlayers)
{
  return TCComPropertyGet(this, pnMaxTeamPlayers, m_mp.nMaxPlayersPerTeam);
}

STDMETHODIMP CPigMissionParams::put_MinTeamPlayers(short nMinTeamPlayers)
{
  return TCComPropertyPut(this, m_mp.nMinPlayersPerTeam, nMinTeamPlayers, dispid_TeamCount);
}

STDMETHODIMP CPigMissionParams::get_MinTeamPlayers(short* pnMinTeamPlayers)
{
  return TCComPropertyGet(this, pnMinTeamPlayers, m_mp.nMinPlayersPerTeam);
}

STDMETHODIMP CPigMissionParams::put_MapType(PigMapType eMapType)
{
  return TCComPropertyPut(this, m_mp.mmMapType, eMapType, dispid_TeamCount);
}

STDMETHODIMP CPigMissionParams::get_MapType(PigMapType* peMapType)
{
  return TCComPropertyGet(this, peMapType, (PigMapType&)m_mp.mmMapType);
}


//imago 10/14
STDMETHODIMP CPigMissionParams::put_TeamKills(short nGoalTeamKills)
{
  return TCComPropertyPut(this, m_mp.nGoalTeamKills, nGoalTeamKills, dispid_TeamKills);
}
STDMETHODIMP CPigMissionParams::get_TeamKills(short* nGoalTeamKills)
{
  return TCComPropertyGet(this, nGoalTeamKills, m_mp.nGoalTeamKills);
}

STDMETHODIMP CPigMissionParams::put_GameName(BSTR bstrGameName)
{
	XLock lock(this);
	USES_CONVERSION;
	Strcpy(m_mp.strGameName,OLE2A(bstrGameName));
	return S_OK;
}
STDMETHODIMP CPigMissionParams::get_GameName(BSTR* bstrGameName)
{
  
  XLock lock(this);
  USES_CONVERSION;
  CLEAROUT(bstrGameName, A2OLE(m_mp.strGameName));
  return S_OK;
}

STDMETHODIMP CPigMissionParams::put_CoreName(BSTR bstrCoreName)
{
	XLock lock(this);
	USES_CONVERSION;
	Strcpy(m_mp.szIGCStaticFile,OLE2A(bstrCoreName));
	return S_OK;
}
STDMETHODIMP CPigMissionParams::get_CoreName(BSTR* bstrCoreName)
{
  XLock lock(this);
  USES_CONVERSION;
  CLEAROUT(bstrCoreName, A2OLE(m_mp.szIGCStaticFile));
  return S_OK;
}

STDMETHODIMP CPigMissionParams::put_KillBonus(short KBlevel)
{
  return TCComPropertyPut(this, m_mp.KBlevel, KBlevel, dispid_KillBonus);
}
STDMETHODIMP CPigMissionParams::get_KillBonus(short* KBlevel)
{
  return TCComPropertyGet(this, KBlevel, m_mp.KBlevel);
}

STDMETHODIMP CPigMissionParams::put_Defections(VARIANT_BOOL Defections)
{
	XLock lock(this);
	USES_CONVERSION;
	m_mp.bAllowDefections = Defections;
	return S_OK;
}
STDMETHODIMP CPigMissionParams::get_Defections(VARIANT_BOOL* Defections)
{
  XLock lock(this);
  USES_CONVERSION;
  CLEAROUT(Defections, VARBOOL(m_mp.bAllowDefections));
  return S_OK;
}

STDMETHODIMP CPigMissionParams::put_Miners(short Miners)
{
  return TCComPropertyPut(this, m_mp.nInitialMinersPerTeam, Miners, dispid_Miners);
}
STDMETHODIMP CPigMissionParams::get_Miners(short* Miners)
{
  return TCComPropertyGet(this, Miners, m_mp.nInitialMinersPerTeam);
}

STDMETHODIMP CPigMissionParams::put_Developments(VARIANT_BOOL Developments)
{
	XLock lock(this);
	USES_CONVERSION;
	m_mp.bAllowDevelopments = Developments;
	return S_OK;
}
STDMETHODIMP CPigMissionParams::get_Developments(VARIANT_BOOL* Developments)
{
  XLock lock(this);
  USES_CONVERSION;
  CLEAROUT(Developments, VARBOOL(m_mp.bAllowDevelopments));
  return S_OK;
}

STDMETHODIMP CPigMissionParams::put_Conquest(short Conquest)
{
  return TCComPropertyPut(this, m_mp.iGoalConquestPercentage, Conquest, dispid_Conquest);
}
STDMETHODIMP CPigMissionParams::get_Conquest(short* Conquest)
{
  return TCComPropertyGet(this, Conquest, m_mp.iGoalConquestPercentage);
}

STDMETHODIMP CPigMissionParams::put_Flags(short Flags)
{
  return TCComPropertyPut(this, m_mp.nGoalFlagsCount, Flags, dispid_Flags);
}
STDMETHODIMP CPigMissionParams::get_Flags(short* Flags)
{
  return TCComPropertyGet(this, Flags, m_mp.nGoalFlagsCount);
}

STDMETHODIMP CPigMissionParams::put_Artifacts(short Artifacts)
{
  return TCComPropertyPut(this, m_mp.nGoalArtifactsCount, Artifacts, dispid_Artifacts);
}
STDMETHODIMP CPigMissionParams::get_Artifacts(short* Artifacts)
{
  return TCComPropertyGet(this, Artifacts, m_mp.nGoalArtifactsCount);
}

STDMETHODIMP CPigMissionParams::put_Pods(VARIANT_BOOL Pods)
{
	XLock lock(this);
	USES_CONVERSION;
	m_mp.bEjectPods = Pods;
	return S_OK;
}
STDMETHODIMP CPigMissionParams::get_Pods(VARIANT_BOOL* Pods)
{
  XLock lock(this);
  USES_CONVERSION;
  CLEAROUT(Pods, (VARIANT_BOOL)m_mp.bEjectPods);
  return S_OK;
}

STDMETHODIMP CPigMissionParams::put_Experimental(VARIANT_BOOL Experimental)
{
	XLock lock(this);
	USES_CONVERSION;
	m_mp.bExperimental = Experimental;
	return S_OK;
}
STDMETHODIMP CPigMissionParams::get_Experimental(VARIANT_BOOL* Experimental)
{
  XLock lock(this);
  USES_CONVERSION;
  CLEAROUT(Experimental, VARBOOL(m_mp.bExperimental));
  return S_OK;
}
/////////////////////////////////////////////////////////////////////////////
// IPigMissionParamsPrivate Interface Methods

STDMETHODIMP CPigMissionParams::GetData(IStream** ppstm)
{
  // Initialize the [out] parameter
  CLEAROUT(ppstm, (IStream*)NULL);

  // Create a stream on global
  IStreamPtr spstm;
  RETURN_FAILED(CreateStreamOnHGlobal(NULL, true, &spstm));

  // Write the size of the data structure to the stream
  UINT cb = sizeof(m_mp);
  RETURN_FAILED(spstm->Write(&cb, sizeof(cb), NULL));

  // Write the data structure to the stream
  RETURN_FAILED(spstm->Write(&m_mp, cb, NULL));

  // Detach the interface to the [out] parameter
  *ppstm = spstm.Detach();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IPersistStreamInit Interface Methods

STDMETHODIMP CPigMissionParams::InitNew()
{
  XLock lock(this);
  ZeroMemory(&m_mp, sizeof(m_mp));

  // Reset the structure
  m_mp.Reset();

  // Initialize the Game name
  strcpy(m_mp.strGameName, "Pig Mission");
  strcpy(m_mp.szIGCStaticFile, "PCore014"); //Imago 10/14

  // Perform default processing
  return TCPersistStreamInitImplBase::InitNew();
}