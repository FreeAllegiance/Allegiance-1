/////////////////////////////////////////////////////////////////////////////
// DlgGameSelect.cpp : implementation file
//

#include "stdafx.h"
#include "AllSrvUI.h"
#include "AllSrvUISheet.h"
#include "DlgGameSelect.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgGameSelect dialog


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CDlgGameSelect, CDialog)
  //{{AFX_MSG_MAP(CDlgGameSelect)
  ON_NOTIFY(LVN_DELETEITEM, IDC_GAME_LIST, OnDeleteItemGameList)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_GAME_LIST, OnItemChangedGameList)
  ON_NOTIFY(NM_DBLCLK, IDC_GAME_LIST, OnDblClickGameList)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction

CDlgGameSelect::CDlgGameSelect(CAllSrvUISheet* pSheet)
  : CDialog(CDlgGameSelect::IDD, pSheet),
	m_mpSheet(pSheet)
{
  //{{AFX_DATA_INIT(CDlgGameSelect)
  //}}AFX_DATA_INIT
  m_spGame = pSheet->GetGame();
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

void CDlgGameSelect::DoDataExchange(CDataExchange* pDX)
{
  // Perform default processing
  CDialog::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(CDlgGameSelect)
  DDX_Control(pDX, IDC_GAME_LIST, m_listGames);
  //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

void CDlgGameSelect::PopulateGamesList()
{
  CWaitCursor wait;
  LPCSTR pszContext = "retrieving collection of games on server";

  // Clear the contents of the list
  m_listGames.DeleteAllItems();

  // Get the collection of games on the server
  IAdminGamesPtr spGames;
  HRESULT hr = m_mpSheet->GetServer()->get_Games(&spGames);
  if (FAILED(hr))
  {
    m_mpSheet->HandleError(hr, pszContext, false);
    EndDialog(IDCANCEL);
    return;
  }

  // Get the enumerator object from the collection
  IUnknownPtr spEnumUnk;
  if (FAILED(hr = spGames->get__NewEnum(&spEnumUnk)))
  {
	  m_mpSheet->HandleError(hr, pszContext, false);
    EndDialog(IDCANCEL);
    return;
  }
  IEnumVARIANTPtr spEnum(spEnumUnk);
  if (NULL == spEnum)
  {
	  m_mpSheet->HandleError(E_NOINTERFACE, pszContext, false);
    EndDialog(IDCANCEL);
    return;
  }

  // Iterate through each game from the enumerator
  int iIndex = 0;
  CComVariant games[32];
  do
  {
    // Fetch the next block of games from the enumerator
    ULONG cFetched;
    if (FAILED(hr = spEnum->Next(sizeofArray(games), games, &cFetched)))
    {
		m_mpSheet->HandleError(hr, pszContext, false);
      EndDialog(IDCANCEL);
      return;
    }

    // Add each game to the list
    for (ULONG i = 0; i < cFetched; ++i)
    {
      // Convert the VARIANT to IDispatch, if not already
      VARTYPE vt = V_VT(&games[i]);
      if (VT_DISPATCH != vt && VT_UNKNOWN != vt)
      {
		  m_mpSheet->HandleError(DISP_E_TYPEMISMATCH, pszContext, false);
        EndDialog(IDCANCEL);
        return;
      }

      // Ensure that the IDispatch supports IAdminGame
      IAdminGamePtr spGame(V_DISPATCH(&games[i]));
      if (NULL == spGame)
      {
		  m_mpSheet->HandleError(E_NOINTERFACE, pszContext, false);
        EndDialog(IDCANCEL);
        return;
      }

      // Clear the VARIANT in the array
      games[i].Clear();

      // Get the interesting properties of the Game
      CComBSTR bstrName;
      // Game Name
      spGame->get_Name(&bstrName);

      // Add the game to the list
      USES_CONVERSION;
      m_listGames.InsertItem(iIndex, OLE2CT(bstrName));
      m_listGames.SetItemData(iIndex, reinterpret_cast<DWORD>(spGame.Detach()));
      ++iIndex;
    }

  } while (S_FALSE != hr);

}



/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CDlgGameSelect::OnInitDialog() 
{
  // Perform default processing
  CDialog::OnInitDialog();

  // Initialize the ListView control
  m_listGames.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
  m_listGames.InsertColumn(0, TEXT("Name"));
  m_listGames.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

  // TODO: Hook-up to receive game destruction events

  // Populate the list box with the available games
  PopulateGamesList();

  // Select the current game in the list
  LPARAM lp = reinterpret_cast<LPARAM>(static_cast<IAdminGame*>(m_spGame));
  LVFINDINFO lvfi = {LVFI_PARAM, NULL, lp};
  int iSel = m_listGames.FindItem(&lvfi);
  if (iSel > -1)
    m_listGames.SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED);

  // Return true to set the focus to the first tabstop control
  return true;
}

void CDlgGameSelect::OnDeleteItemGameList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Parse the message parameters
  NM_LISTVIEW* pNMLV = (NM_LISTVIEW*)pNMHDR;

  // Reinterpret the item data
  IAdminGame* pGame = reinterpret_cast<IAdminGame*>(pNMLV->lParam);

  // Release the Game interface
  if (pGame)
    pGame->Release();

  // Clear the result 
  *pResult = 0;
}

void CDlgGameSelect::OnItemChangedGameList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Parse the message parameters
  NM_LISTVIEW* pNMLV = (NM_LISTVIEW*)pNMHDR;

  // Save the selected game, if this item is selected
  if (LVIS_SELECTED & pNMLV->uNewState)
    m_spGame = reinterpret_cast<IAdminGame*>(pNMLV->lParam);
  
  // Clear the result 
  *pResult = 0;
}

void CDlgGameSelect::OnDblClickGameList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Select the OK button
  OnOK();
  
  // Clear the result 
  *pResult = 0;
}
