/////////////////////////////////////////////////////////////////////////////
// PigMissionParams.h : Declaration of the CPigMissionParams class

#ifndef __PigMissionParams_h__
#define __PigMissionParams_h__

#include "SrcInc.h"
#include <AGC.h>
#include <PigsLib.h>
#include <Allegiance.h>
#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// CPigMissionParams
class ATL_NO_VTABLE CPigMissionParams : 
  public IDispatchImpl<IPigMissionParams, &IID_IPigMissionParams, &LIBID_PigsLib>,
  public ISupportErrorInfo,
  public IPigMissionParamsPrivate,
  public TCPersistStreamInitImpl<CPigMissionParams>,
  public TCPersistPropertyBagImpl<CPigMissionParams>,
  public TCComPropertyClass<CPigMissionParams>,
  public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPigMissionParams, &CLSID_PigMissionParams>
{
// Declarations
public:
  DECLARE_REGISTRY_RESOURCEID(IDR_PigMissionParams)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  DECLARE_GET_CONTROLLING_UNKNOWN()

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPigMissionParams)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPigMissionParams)
    COM_INTERFACE_ENTRY(IPigMissionParams)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPigMissionParamsPrivate)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersistStream, IPersistStreamInit)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_AUTOAGGREGATE(IID_IMarshal, m_punkMBV.p,
      CLSID_TCMarshalByValue)
  END_COM_MAP()

// Property Map
public:
  BEGIN_PROP_MAP(CPigMissionParams)
    PROP_ENTRY("TeamCount"     , dispid_TeamCount     , CLSID_NULL)
    PROP_ENTRY("MaxTeamPlayers", dispid_MaxTeamPlayers, CLSID_NULL)
    PROP_ENTRY("MinTeamPlayers", dispid_MinTeamPlayers, CLSID_NULL)
    PROP_ENTRY("MapType"       , dispid_MapType       , CLSID_NULL)
	//Imago 10/14
	PROP_ENTRY("GameName"       , dispid_GameName     , CLSID_NULL)
	PROP_ENTRY("CoreName"       , dispid_CoreName     , CLSID_NULL)
  END_PROP_MAP()

// Construction / Destruction
public:
  CPigMissionParams();
  HRESULT FinalConstruct();

// ISupportErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// IPigMissionParams Interface Methods
public:
  STDMETHODIMP Validate();
  STDMETHODIMP put_TeamCount(short nTeamCount);
  STDMETHODIMP get_TeamCount(short* pnTeamCount);
  STDMETHODIMP put_MaxTeamPlayers(short nMaxTeamPlayers);
  STDMETHODIMP get_MaxTeamPlayers(short* pnMaxTeamPlayers);
  STDMETHODIMP put_MinTeamPlayers(short nMinTeamPlayers);
  STDMETHODIMP get_MinTeamPlayers(short* pnMinTeamPlayers);
  STDMETHODIMP put_MapType(PigMapType eMapType);
  STDMETHODIMP get_MapType(PigMapType* peMapType);
  //imago 10/14
  STDMETHODIMP put_TeamKills(short nGoalTemaKills);
  STDMETHODIMP get_TeamKills(short* nGoalTemaKills);
  STDMETHODIMP put_GameName(BSTR bstrGameName);
  STDMETHODIMP get_GameName(BSTR* bstrGameName);
  STDMETHODIMP put_CoreName(BSTR bstrCoreName);
  STDMETHODIMP get_CoreName(BSTR* bstrCoreName);
  STDMETHODIMP put_KillBonus(short KBlevel);
  STDMETHODIMP get_KillBonus(short* KBlevel);
  STDMETHODIMP put_Defections(VARIANT_BOOL Defections);
  STDMETHODIMP get_Defections(VARIANT_BOOL* Defections);
  STDMETHODIMP put_Miners(short Miners);
  STDMETHODIMP get_Miners(short* Miners);
  STDMETHODIMP put_Developments(VARIANT_BOOL Developments);
  STDMETHODIMP get_Developments(VARIANT_BOOL* Developments);
  STDMETHODIMP put_Conquest(short Conquest);
  STDMETHODIMP get_Conquest(short* Conquest);
  STDMETHODIMP put_Flags(short Flags);
  STDMETHODIMP get_Flags(short* Flags);
  STDMETHODIMP put_Artifacts(short Artifacts);
  STDMETHODIMP get_Artifacts(short* Artifacts);
  STDMETHODIMP put_Pods(VARIANT_BOOL Pods);
  STDMETHODIMP get_Pods(VARIANT_BOOL* Pods);
  STDMETHODIMP put_Experimental(VARIANT_BOOL Experimental);
  STDMETHODIMP get_Experimental(VARIANT_BOOL* Experimental);
// IPigMissionParamsPrivate Interface Methods
public:
  STDMETHODIMP GetData(IStream** ppstm);

// IPersistStreamInit Interface Methods
public:
  STDMETHODIMP InitNew();

// Types
protected:
  typedef TCObjectLock<CPigMissionParams> XLock;

// Data Members
public:
  CComPtr<IUnknown> m_punkMBV;
protected:
  MissionParams     m_mp;
};


/////////////////////////////////////////////////////////////////////////////

#endif //__PigMissionParams_h__
