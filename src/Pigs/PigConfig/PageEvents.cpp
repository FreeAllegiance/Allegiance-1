/////////////////////////////////////////////////////////////////////////////
// PageEvents.cpp : Implementation of the CPageEvents class.
//

#include "pch.h"
#include "PageEvents.h"
#include "PigConfigSheet.h"


/////////////////////////////////////////////////////////////////////////////
// Globals

CAGCModule _AGCModule;


/////////////////////////////////////////////////////////////////////////////
// CPageEvents property page


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CPageEvents, CPropertyPage)
  //{{AFX_MSG_MAP(CPageEvents)
  ON_NOTIFY(TVN_DELETEITEM, IDC_EVENTS, OnDeleteItemEvents)
  ON_NOTIFY(TVN_KEYDOWN, IDC_EVENTS, OnKeyDownEvents)
  ON_NOTIFY(TVN_SELCHANGED, IDC_EVENTS, OnSelChangedEvents)
  ON_BN_CLICKED(IDC_BY_GROUP, OnByGroup)
  ON_BN_CLICKED(IDC_FLAT_LIST, OnFlatList)
  ON_NOTIFY(LVN_DELETEITEM, IDC_EVENTS_LIST, OnDeleteItemEventsList)
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_EVENTS_LIST, OnColumnClickEventsList)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPageEvents::CPageEvents(CPigConfigSheet* pSheet) :
  CPropertyPage(CPageEvents::IDD),
  m_pSheet(pSheet),
  m_bstrEvent(L"Event"),
  m_bstrEventGroup(L"EventGroup"),
  m_bstrName(L"Name"),
  m_bstrDisplayName(L"DisplayName"),
  m_bstrSeverity(L"Severity"),
  m_bstrID(L"id"),
  m_bstrLowerBound(L"LowerBound"),
  m_bstrUpperBound(L"UpperBound"),
  m_bstrDescription(L"Description"),
  m_bstrLogAsNTEvent(L"LogAsNTEvent"),
  m_bstrLogAsDBEvent(L"LogAsDBEvent"),
  m_bstrCanChangeLogAsNTEvent(L"CanChangeLogAsNTEvent"),
  m_bstrCanChangeLogAsDBEvent(L"CanChangeLogAsDBEvent"),
  m_lParamSelected(NULL),
  m_bLoggerObjectIsAlive(false),
  m_nSortDirection(1),
  m_nSortColumn(1)
{
  //{{AFX_DATA_INIT(CPageEvents)
  //}}AFX_DATA_INIT
}

CPageEvents::~CPageEvents()
{
}


/////////////////////////////////////////////////////////////////////////////
// Attributes

bool CPageEvents::IsDisplayable()
{
  // Determine if a Pigs Session was successfully created
  if (GetSheet()->GetSession() == NULL)
    return false;

  // Attempt to create AGCEventIDRanges objects
  if (FAILED(m_spRangesNT.CreateInstance("AGC.EventIDRanges")))
    return false;
  if (FAILED(m_spRangesDB.CreateInstance("AGC.EventIDRanges")))
    return false;

  // Attempt to create an instance of the MSXML component
  if (FAILED(m_spXMLDoc.CreateInstance(L"Microsoft.XMLDOM")))
    return false;

  // Create the AGCEventLogger (either live or static)
  return SUCCEEDED(GetEventLogger());
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

void CPageEvents::OnOK() 
{
  // Save any changes made to the currently-selected node's attributes
  UpdateNodeFromItem(m_lParamSelected);

  // Inspect the <Event> nodes of the document
  if (NULL != m_spXMLDoc)
  {
    // Get all of the <Event> nodes in the document
    IXMLDOMNodeListPtr spNodeList;
    VERIFY(SUCCEEDED(m_spXMLDoc->getElementsByTagName(m_bstrEvent,
      &spNodeList)));

    // Process each node
    IXMLDOMNodePtr spNode;
    do  
    {
      // Get the next node of the child list
      VERIFY(SUCCEEDED(spNodeList->nextNode(&spNode)));
      if (NULL != spNode)
      {
        // Query for the IXMLDOMElement interface
        IXMLDOMElementPtr spElement(spNode);
        ASSERT(NULL != spElement);

        // Get the event id attribute
        CComVariant varEventID;
        spElement->getAttribute(m_bstrID, &varEventID);
        VERIFY(SUCCEEDED(varEventID.ChangeType(VT_I4)));
        AGCEventID idEventBegin = (AGCEventID)(V_UI4(&varEventID));
        AGCEventID idEventEnd = (AGCEventID)(idEventBegin + 1);

        // Get the LogAsNTEvent attribute
        IXMLDOMAttributePtr spAttrNT;
        if (S_OK == spElement->getAttributeNode(m_bstrLogAsNTEvent, &spAttrNT))
        {
          CComVariant varLog2NT;
          spAttrNT->get_value(&varLog2NT);
          VERIFY(SUCCEEDED(varLog2NT.ChangeType(VT_BOOL)));

          // Add this event id to the range, if it should be logged
          if (V_BOOL(&varLog2NT))
            m_spRangesNT->AddByValues(idEventBegin, idEventEnd);
        }

        // Get the LogAsDBEvent attribute
        IXMLDOMAttributePtr spAttrDB;
        if (S_OK == spElement->getAttributeNode(m_bstrLogAsDBEvent, &spAttrDB))
        {
          CComVariant varLog2DB;
          spAttrDB->get_value(&varLog2DB);
          VERIFY(SUCCEEDED(varLog2DB.ChangeType(VT_BOOL)));

          // Add this event id to the range, if it should be logged
          if (V_BOOL(&varLog2DB))
            m_spRangesDB->AddByValues(idEventBegin, idEventEnd);
        }
      }
    } while (NULL != spNode);

    // Set the enabled ranges of the event logger object
    VERIFY(SUCCEEDED(m_spEventLogger->put_EnabledNTEvents(m_spRangesNT)));
    VERIFY(SUCCEEDED(m_spEventLogger->put_EnabledDBEvents(m_spRangesDB)));
  }

  // Perform default processing
  CPropertyPage::OnOK();
}

void CPageEvents::DoDataExchange(CDataExchange* pDX)
{
  // Perform default processing
  CPropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(CPageEvents)
  DDX_Control(pDX, IDC_EVENTS_LIST, m_listEvents);
  DDX_Control(pDX, IDC_LOG2DB, m_btnLogToDB);
  DDX_Control(pDX, IDC_LOG2NT, m_btnLogToNT);
  DDX_Control(pDX, IDC_DESCRIPTION, m_staticDescription);
  DDX_Control(pDX, IDC_EVENT_ID, m_staticID);
  DDX_Control(pDX, IDC_EVENT_TYPE, m_staticType);
  DDX_Control(pDX, IDC_EVENTS, m_tree);
  //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

HRESULT CPageEvents::GetEventLogger()
{
  return GetSheet()->GetSession()->get_EventLog(&m_spEventLogger);
}

void CPageEvents::InitEventTree()
{
  // Load the image list
  if (!m_images.GetSafeHandle())
    m_images.Create(IDB_IMAGES, 16, 0, RGB(0, 255, 0));
  m_tree.SetImageList(&m_images, TVSIL_NORMAL);

  // Clear the tree's current contents, if any
  m_tree.DeleteAllItems();
}

void CPageEvents::InitEventList()
{
  // Set the control's extended styles
  m_listEvents.SetExtendedStyle(LVS_EX_FULLROWSELECT | 0x4000);

  // Load the image list
  if (!m_images.GetSafeHandle())
    m_images.Create(IDB_IMAGES, 16, 0, RGB(0, 255, 0));
  m_listEvents.SetImageList(&m_images, LVSIL_SMALL);

  // Clear the list's current contents, if any
  m_listEvents.DeleteAllItems();

  // TODO: Delete all columns, if any

  // Insert columns
  CString strColumn;
  LVCOLUMN lvcol = {LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT};

  // Insert Type column
  lvcol.iSubItem = 0;
  lvcol.fmt    = LVCFMT_LEFT | LVCFMT_IMAGE;
  strColumn.LoadString(IDS_COLUMN_TYPE);
  lvcol.pszText  = const_cast<LPTSTR>((LPCSTR)strColumn);
  m_listEvents.InsertColumn(lvcol.iSubItem, &lvcol);
  m_cxMaxType = m_listEvents.GetStringWidth(strColumn + "    + ");
  
  // Insert EventID column
  lvcol.iSubItem = 1;
  lvcol.fmt    = LVCFMT_RIGHT;
  strColumn.LoadString(IDS_COLUMN_EVENTID);
  lvcol.pszText  = const_cast<LPTSTR>((LPCSTR)strColumn);
  m_listEvents.InsertColumn(lvcol.iSubItem, &lvcol);
  m_cxMaxID = m_listEvents.GetStringWidth(strColumn + "    + ");
  
  // Insert Name column
  lvcol.iSubItem = 2;
  lvcol.fmt    = LVCFMT_LEFT;
  strColumn.LoadString(IDS_COLUMN_NAME);
  lvcol.pszText  = const_cast<LPTSTR>((LPCSTR)strColumn);
  m_listEvents.InsertColumn(lvcol.iSubItem, &lvcol);
}

HRESULT CPageEvents::PopulateTreeAndList()
{
  // Get the XML text of the logger's current status
  CComBSTR bstrEventList;
  RETURN_FAILED(m_spEventLogger->get_EventList(&bstrEventList));

  // Load the XML text into the XMLDOM object
  VARIANT_BOOL bSucceeded;
  RETURN_FAILED(m_spXMLDoc->loadXML(bstrEventList, &bSucceeded));
  if (!bSucceeded)
    return E_FAIL;

  // Get the root element
  IXMLDOMElementPtr spRoot;
  RETURN_FAILED(m_spXMLDoc->get_documentElement(&spRoot));
  #ifdef _DEBUG
  {
    // Ensure that the root element tag is <AGCEvents>
    CComBSTR bstrRoot;
    ASSERT(SUCCEEDED(spRoot->get_tagName(&bstrRoot)));
    ASSERT(0 == wcscmp(bstrRoot, L"AGCEvents"));
  }
  #endif // _DEBUG

  // Recursively process the children of each group
  RETURN_FAILED(AddXMLNodeToTree(spRoot, TVI_ROOT));

  // Caculate the column widths
  m_cxMaxType += 16 + 2; // Adjust for icon and padding
  int cxScroll = GetSystemMetrics(SM_CXVSCROLL);

  // Get the width of the list control's client area
  CRect rect;
  m_listEvents.GetClientRect(rect);

  // Compute the Name column to fit the maximum width
  int cxTotal = m_cxMaxType + m_cxMaxID + cxScroll;
  int cxName = rect.Width() - cxTotal;

  // Set the column widths
  LVCOLUMN lvcol = {LVCF_WIDTH};
  lvcol.cx = m_cxMaxType;
  m_listEvents.SetColumn(0, &lvcol);
  lvcol.cx = m_cxMaxID;
  m_listEvents.SetColumn(1, &lvcol);
  lvcol.cx = cxName;
  m_listEvents.SetColumn(2, &lvcol);

  // Indicate success
  return S_OK;
}


HRESULT CPageEvents::AddXMLNodeToTree(IXMLDOMNode* pNode, HTREEITEM hParent)
{
  // Get the list of child nodes
  IXMLDOMNodeListPtr spChildren;
  RETURN_FAILED(pNode->get_childNodes(&spChildren));

  // Process each child node
  IXMLDOMNodePtr spChild;
  do  
  {
    // Get the next node of the child list
    RETURN_FAILED(spChildren->nextNode(&spChild));
    if (NULL != spChild)
    {
      // Get the child node's tagname
      int       iImage;
      CString     strType;
      CString     strID;
      bool        bIsGroup, bIsEvent;
      CComBSTR      bstrText;
      IXMLDOMElementPtr spElement(spChild);
      if (NULL != spElement)
      {
        CComBSTR bstrTagName;
        RETURN_FAILED(spElement->get_tagName(&bstrTagName));
        if (bstrTagName.Length())
        {
          // Accept the Event and EventGroup tag names
          if (0 == wcscmp(bstrTagName, m_bstrEvent))
          {
            bIsGroup = false;
            bIsEvent = true;
          }
          else if (0 == wcscmp(bstrTagName, m_bstrEventGroup))
          {
            bIsGroup = true;
            bIsEvent = false;
          }

          // Get the display attributes if this is a group or event node
          if (bIsGroup || bIsEvent)
          {
            // Get the type of the element
            CComBSTR bstrSeverity;
            GetElementSeverity(spElement, &bstrSeverity);
            iImage = ImageFromSeverity(bstrSeverity);
            strType = TypeFromSeverity(bstrSeverity);

            // Get the id of the element
            CComBSTR bstrID;
            GetElementID(spElement, &bstrID);
            strID = bstrID;

            // Get the name of the element
            RETURN_FAILED(GetElementDisplayName(spElement, &bstrText));
          }
        }
      }

      // Add the node to the tree and list controls
      if (bstrText.Length())
      {
        // Typecast the element pointer as an LPARAM
        IXMLDOMElement* pElement = spElement.Detach();
        LPARAM lParam = reinterpret_cast<LPARAM>(pElement);

        // Insert the element into the tree
        USES_CONVERSION;
        LPCTSTR pszText = OLE2CT(bstrText);
        UINT mask  = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        HTREEITEM hItem = m_tree.InsertItem(mask, pszText, iImage, iImage,
          0, 0, lParam, hParent, TVI_LAST);

        // Insert the element into the list, if its not an <EventGroup>
        if (!bIsGroup)
        {
          int iItem = m_listEvents.GetItemCount();
          iItem = m_listEvents.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
            iItem, strType, 0, 0, iImage, lParam);
          if (-1 != iItem)
          {
            // Keep a reference on the element pointer in the LPARAM
            pElement->AddRef();

            // Keep track of maximum widths
            int cx = m_listEvents.GetStringWidth(strType + "  ");
            m_cxMaxType = std::max(m_cxMaxType, cx);
            cx = m_listEvents.GetStringWidth(strID + "    ");
            m_cxMaxID = std::max(m_cxMaxID, cx);

            // Set the subitems
            m_listEvents.SetItem(iItem, 1, LVIF_TEXT, strID, 0, 0, 0, 0);
            m_listEvents.SetItem(iItem, 2, LVIF_TEXT, pszText, 0, 0, 0, 0);
          }
        }

        // Recurse into node, if it's a group
        if (bIsGroup)
        {
          RETURN_FAILED(AddXMLNodeToTree(spChild, hItem));
        }
      }
    }
  } while (NULL != spChild);

  // Indicate success
  return S_OK;
}

HRESULT CPageEvents::GetElementSeverity(IXMLDOMElement* pElem, BSTR* pbstr)
{
  // Get the Severity attribute
  CComVariant var;
  pElem->getAttribute(m_bstrSeverity, &var);
  var.ChangeType(VT_BSTR);
  *pbstr = V_BSTR(&var);
  V_VT(&var) = VT_EMPTY;
  return S_OK;
}

HRESULT CPageEvents::GetElementDisplayName(IXMLDOMElement* pElem, BSTR* pbstr)
{
  // Get the DisplayName or Name attribute, in that order
  CComVariant var;
  if (FAILED(pElem->getAttribute(m_bstrDisplayName, &var)) ||
    FAILED(var.ChangeType(VT_BSTR))            ||
    !V_BSTR(&var)                    ||
    !SysStringLen(V_BSTR(&var)))
  {
    RETURN_FAILED(pElem->getAttribute(m_bstrName, &var));
    RETURN_FAILED(var.ChangeType(VT_BSTR));
  }
  *pbstr = V_BSTR(&var);
  V_VT(&var) = VT_EMPTY;
  return S_OK;
}

HRESULT CPageEvents::GetElementID(IXMLDOMElement* pElem, BSTR* pbstr)
{
  // Get the id or LowerBound attribute, in that order
  CComVariant var;
  if (FAILED(pElem->getAttribute(m_bstrID, &var)) ||
    FAILED(var.ChangeType(VT_I4))       ||
    FAILED(var.ChangeType(VT_BSTR)))
  {
    RETURN_FAILED(pElem->getAttribute(m_bstrLowerBound, &var));
    RETURN_FAILED(var.ChangeType(VT_I4));
    RETURN_FAILED(var.ChangeType(VT_BSTR));
  }
  *pbstr = V_BSTR(&var);
  V_VT(&var) = VT_EMPTY;
  return S_OK;
}

HRESULT CPageEvents::GetElementID(IXMLDOMElement* pElem, int* pID)
{
  // Get the id or LowerBound attribute, in that order
  CComVariant var;
  if (FAILED(pElem->getAttribute(m_bstrID, &var)) ||
    FAILED(var.ChangeType(VT_I4)))
  {
    RETURN_FAILED(pElem->getAttribute(m_bstrLowerBound, &var));
    RETURN_FAILED(var.ChangeType(VT_I4));
  }
  *pID = V_I4(&var);
  return S_OK;
}

HRESULT CPageEvents::UpdateNodeFromItem(LPARAM lNode)
{
  // Typecast the tree item's param as an XML element pointer
  IXMLDOMElement* pElement = reinterpret_cast<IXMLDOMElement*>(lNode);
  if (!pElement)
    return S_FALSE;

  // Determine if the node is a group element or an event
  CComBSTR bstrTagName;
  VERIFY(SUCCEEDED(pElement->get_tagName(&bstrTagName)));
  bool bIsEvent = 0 == wcscmp(bstrTagName, m_bstrEvent);
  bool bIsGroup = !bIsEvent && 0 == wcscmp(bstrTagName, m_bstrEventGroup);
  ASSERT(bIsEvent || bIsGroup);
  if (bIsEvent)
  {
    // Set the LogAsNTEvent attribute
    if (GetDlgItem(IDC_LOG2NT)->IsWindowEnabled())
    {
      CComVariant varLog2NT(!!IsDlgButtonChecked(IDC_LOG2NT));
      pElement->setAttribute(m_bstrLogAsNTEvent, varLog2NT);
    }

    // Set the LogAsDBEvent attribute
    if (GetDlgItem(IDC_LOG2DB)->IsWindowEnabled())
    {
      CComVariant varLog2DB(!!IsDlgButtonChecked(IDC_LOG2DB));
      pElement->setAttribute(m_bstrLogAsDBEvent, varLog2DB);
    }
  }
  else
  {
    // TODO: Work-out how to best support indeterminate checkbox state
  }

  // Indicate success
  return S_OK;
}

HRESULT CPageEvents::UpdateItemFromNode(LPARAM lNode)
{
  USES_CONVERSION;

  // Typecast the tree item's param as an XML element pointer
  IXMLDOMElement* pElement = reinterpret_cast<IXMLDOMElement*>(lNode);
  if (!pElement)
    return S_FALSE;

  // Use to enable/disable the check boxes
  bool bEnableLog2NT = false;
  bool bEnableLog2DB = false;

  // Determine if the node is a group element or an event
  CComBSTR bstrTagName;
  VERIFY(SUCCEEDED(pElement->get_tagName(&bstrTagName)));
  bool bIsEvent = 0 == wcscmp(bstrTagName, m_bstrEvent);
  bool bIsGroup = !bIsEvent && 0 == wcscmp(bstrTagName, m_bstrEventGroup);
  ASSERT(bIsEvent || bIsGroup);
  if (bIsEvent)
  {
    // Get the Severity attribute
    CComVariant varSeverity;
    pElement->getAttribute(m_bstrSeverity, &varSeverity);
    VERIFY(SUCCEEDED(varSeverity.ChangeType(VT_BSTR)));
    SetDlgItemText(IDC_EVENT_TYPE, TypeFromSeverity(V_BSTR(&varSeverity)));

    // Get the event id attribute
    CComVariant varEventID;
    pElement->getAttribute(m_bstrID, &varEventID);
    VERIFY(SUCCEEDED(varEventID.ChangeType(VT_I4)));
    SetDlgItemInt(IDC_EVENT_ID, V_I4(&varEventID));

    // Get the CanChangeLogAsNTEvent attribute
    IXMLDOMAttributePtr spAttrNT;
    if (S_OK == pElement->getAttributeNode(m_bstrCanChangeLogAsNTEvent, &spAttrNT))
    {
      CComVariant varCanChange;
      spAttrNT->get_value(&varCanChange);
      VERIFY(SUCCEEDED(varCanChange.ChangeType(VT_BOOL)));
      bEnableLog2NT = !!V_BOOL(&varCanChange);
    }
    else
    {
      // When not specified, the default is true
      bEnableLog2NT = true;
    }

    // Get the LogAsNTEvent attribute
    CComVariant varLog2NT;
    pElement->getAttribute(m_bstrLogAsNTEvent, &varLog2NT);
    VERIFY(SUCCEEDED(varLog2NT.ChangeType(VT_BOOL)));
    CheckDlgButton(IDC_LOG2NT, V_BOOL(&varLog2NT));

    // Get the CanChangeLogAsDBEvent attribute
    IXMLDOMAttributePtr spAttrDB;
    if (S_OK == pElement->getAttributeNode(m_bstrCanChangeLogAsDBEvent, &spAttrDB))
    {
      CComVariant varCanChange;
      spAttrDB->get_value(&varCanChange);
      VERIFY(SUCCEEDED(varCanChange.ChangeType(VT_BOOL)));
      bEnableLog2DB = !!V_BOOL(&varCanChange);
    }
    else
    {
      // When not specified, the default is true
      bEnableLog2DB = true;
    }

    // Get the LogAsDBEvent attribute
    CComVariant varLog2DB;
    pElement->getAttribute(m_bstrLogAsDBEvent, &varLog2DB);
    VERIFY(SUCCEEDED(varLog2DB.ChangeType(VT_BOOL)));
    CheckDlgButton(IDC_LOG2DB, V_BOOL(&varLog2DB));
  }
  else
  {
    // Always show the type as "Group"
    CString strGroup;
    strGroup.LoadString(IDS_TYPE_GROUP);
    SetDlgItemText(IDC_EVENT_TYPE, strGroup);

    // Get the group LowerBound attribute
    CComVariant varLowerBound;
    pElement->getAttribute(m_bstrLowerBound, &varLowerBound);
    VERIFY(SUCCEEDED(varLowerBound.ChangeType(VT_UI4)));

    // Get the group UpperBound attribute
    CComVariant varUpperBound;
    pElement->getAttribute(m_bstrUpperBound, &varUpperBound);
    VERIFY(SUCCEEDED(varUpperBound.ChangeType(VT_UI4)));

    // Format the group's range into a string
    TCHAR szID[_MAX_PATH];
    wsprintf(szID, TEXT("%u - %u"), V_UI4(&varLowerBound),
      V_UI4(&varUpperBound));
    SetDlgItemText(IDC_EVENT_ID, szID);

    // TODO: Work-out how to best support indeterminate checkbox state
    CheckDlgButton(IDC_LOG2NT, false);
    CheckDlgButton(IDC_LOG2DB, false);
  }

  // Get the event/group Description attribute
  CComVariant varDescription;
  pElement->getAttribute(m_bstrDescription, &varDescription);
  VERIFY(SUCCEEDED(varDescription.ChangeType(VT_BSTR)));
  SetDlgItemText(IDC_DESCRIPTION, OLE2CT(V_BSTR(&varDescription)));

  // Enable/disable checkboxes
  GetDlgItem(IDC_LOG2NT)->EnableWindow(bEnableLog2NT);
  GetDlgItem(IDC_LOG2DB)->EnableWindow(bEnableLog2DB);

  // Uncheck checkboxes that are disabled
  if (!bEnableLog2NT)
    CheckDlgButton(IDC_LOG2NT, false);
  if (!bEnableLog2DB)
    CheckDlgButton(IDC_LOG2DB, false);

  // Indicate success
  return S_OK;
}

int CPageEvents::ImageFromSeverity(BSTR bstrSeverity)
{
  if (!bstrSeverity || L'\0' == bstrSeverity[0])
    return 0;
  switch (bstrSeverity[0])
  {
    case L'E':
    case L'e':
      return 3;
    case L'W':
    case L'w':
      return 2;
    case L'I':
    case L'i':
    default:
      return 1;
  }
}

CString CPageEvents::TypeFromSeverity(BSTR bstrSeverity)
{
  UINT idRes;
  if (!bstrSeverity || L'\0' == bstrSeverity[0])
  {
    idRes = IDS_TYPE_GROUP;
  }
  else
  {
    switch (bstrSeverity[0])
    {
      case L'E':
      case L'e':
        idRes = IDS_TYPE_ERROR;
        break;
      case L'W':
      case L'w':
        idRes = IDS_TYPE_WARNING;
        break;
      case L'I':
      case L'i':
        idRes = IDS_TYPE_INFORMATION;
        break;
      case L'S':
      case L's':
      default:
        idRes = IDS_TYPE_SUCCESS;
        break;
    }
  }

  CString str;
  str.LoadString(idRes);
  return str;
}

void CPageEvents::ToggleCheckBox(UINT id)
{
  CWnd* pwndCheckBox = GetDlgItem(id);
  if (!pwndCheckBox->IsWindowEnabled())
    MessageBeep(static_cast<DWORD>(-1));
  else
  {
    bool bChecked = BST_CHECKED == IsDlgButtonChecked(id);
    CheckDlgButton(id, bChecked ? BST_UNCHECKED : BST_CHECKED);
  }
}

PFNLVCOMPARE CPageEvents::SortProcFromColumn(int iSubItem)
{
  switch (m_nSortColumn)
  {
    case 0 :         return CompareType;
    case 1 :         return CompareID  ;
    case 2 :         return CompareName;
    default:  ASSERT(false); return      NULL;
  }
}

int CALLBACK CPageEvents::CompareType(LPARAM lp1, LPARAM lp2, LPARAM lpThis)
{
  // Typecast the sort parameters
  IXMLDOMElement* pElem1 = reinterpret_cast<IXMLDOMElement*>(lp1);
  IXMLDOMElement* pElem2 = reinterpret_cast<IXMLDOMElement*>(lp2);
  CPageEvents*  pThis  = reinterpret_cast<CPageEvents*>(lpThis);

  // Get the type image index for element 1
  CComBSTR bstrSeverity1;
  pThis->GetElementSeverity(pElem1, &bstrSeverity1);
  int iImage1 = pThis->ImageFromSeverity(bstrSeverity1);

  // Get the type image index for element 2
  CComBSTR bstrSeverity2;
  pThis->GetElementSeverity(pElem2, &bstrSeverity2);
  int iImage2 = pThis->ImageFromSeverity(bstrSeverity2);

  // Compute the difference, return if not equal
  int nDiff = iImage1 - iImage2;
  return nDiff ? pThis->m_nSortDirection * nDiff
               : CompareID(lp1, lp2, lpThis);
}

int CALLBACK CPageEvents::CompareID(LPARAM lp1, LPARAM lp2, LPARAM lpThis)
{
  // Typecast the sort parameters
  IXMLDOMElement* pElem1 = reinterpret_cast<IXMLDOMElement*>(lp1);
  IXMLDOMElement* pElem2 = reinterpret_cast<IXMLDOMElement*>(lp2);
  CPageEvents*  pThis  = reinterpret_cast<CPageEvents*>(lpThis);

  // Get the id for element 1
  int id1;
  VERIFY(SUCCEEDED(pThis->GetElementID(pElem1, &id1)));

  // Get the id for element 2
  int id2;
  VERIFY(SUCCEEDED(pThis->GetElementID(pElem2, &id2)));

  // Return the difference
  return pThis->m_nSortDirection * (id1 - id2);
}

int CALLBACK CPageEvents::CompareName(LPARAM lp1, LPARAM lp2, LPARAM lpThis)
{
  // Typecast the sort parameters
  IXMLDOMElement* pElem1 = reinterpret_cast<IXMLDOMElement*>(lp1);
  IXMLDOMElement* pElem2 = reinterpret_cast<IXMLDOMElement*>(lp2);
  CPageEvents*  pThis  = reinterpret_cast<CPageEvents*>(lpThis);

  // Get the name for element 1
  CComBSTR bstrName1;
  VERIFY(SUCCEEDED(pThis->GetElementDisplayName(pElem1, &bstrName1)));

  // Get the name for element 2
  CComBSTR bstrName2;
  VERIFY(SUCCEEDED(pThis->GetElementDisplayName(pElem2, &bstrName2)));

  // Return the case-insensitive comparison
  int nDiff = _wcsicmp(bstrName1, bstrName2);
  return nDiff ? pThis->m_nSortDirection * nDiff
               : CompareID(lp1, lp2, lpThis);
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CPageEvents::OnInitDialog() 
{
  // Perform default processing
  CPropertyPage::OnInitDialog();

  // Get the instance of the AGCEventLogger on which we will operate
  if (NULL != m_spEventLogger && NULL != m_spXMLDoc)
  {
    // Initialize the tree and list controls
    InitEventTree();
    InitEventList();

    // Populate the tree and list controls with the event hierarchy
    PopulateTreeAndList();
  }
  else
  {
    // FORNOW: Disable the whole window
    EnableWindow(false);
  }

  // Select the Tree control view
  CheckDlgButton(IDC_BY_GROUP, BST_CHECKED);
  OnByGroup();

  // Return true to set the focus to the first tabstop control
  return true;
}


void CPageEvents::OnDeleteItemEvents(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Typecast the specified notification structure
  NM_TREEVIEW* pTV = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);

  // Typecast the tree item's param as an XML element pointer
  IXMLDOMElement* pElement =
    reinterpret_cast<IXMLDOMElement*>(pTV->itemOld.lParam);

  // Release the XML element pointer associated with the tree item
  if (pElement)
    pElement->Release();

  // Clear the result
  *pResult = 0;
}

void CPageEvents::OnKeyDownEvents(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Typecast the specified notification structure
  TV_KEYDOWN* pTVKD = reinterpret_cast<TV_KEYDOWN*>(pNMHDR);

  // Check for alphanum key
  if (isalnum(pTVKD->wVKey))
  {
    // Check for check-box accelerator keys
    if ('e' == pTVKD->wVKey || 'E' == pTVKD->wVKey)
      ToggleCheckBox(IDC_LOG2NT);
    if ('d' == pTVKD->wVKey || 'D' == pTVKD->wVKey)
      ToggleCheckBox(IDC_LOG2DB);

    // Exclude the key from further processing
    *pResult = true;
    return;
  }
  
  // Clear the result
  *pResult = 0;
}

void CPageEvents::OnSelChangedEvents(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Typecast the specified notification structure
  NM_TREEVIEW* pNMTV = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);

  // Save any changes made to the previously-selected node's attributes
  UpdateNodeFromItem(pNMTV->itemOld.lParam);

  // Update the controls from the newly-selected node
  m_lParamSelected = pNMTV->itemNew.lParam;
  UpdateItemFromNode(pNMTV->itemNew.lParam);
  
  // Clear the result
  *pResult = 0;
}


void CPageEvents::OnByGroup() 
{
  // Do nothing if tree view is already visible
  if (GetDlgItem(IDC_EVENTS)->IsWindowVisible())
    return;

  // TODO: Add your control notification handler code here

  // TODO: Select the currently selected list item in the tree
  
  // Hide the list control and show the tree control
  GetDlgItem(IDC_EVENTS_LIST)->ShowWindow(SW_HIDE);
  GetDlgItem(IDC_EVENTS   )->ShowWindow(SW_SHOW);
}

void CPageEvents::OnFlatList() 
{
  // Do nothing if list view is already visible
  if (GetDlgItem(IDC_EVENTS_LIST)->IsWindowVisible())
    return;

  // Save any changes made to the currently-selected node's attributes
  UpdateNodeFromItem(m_lParamSelected);

  // TODO: Select the currently selected tree item in the list
  
  // Hide the tree control and show the list control
  GetDlgItem(IDC_EVENTS   )->ShowWindow(SW_HIDE);
  GetDlgItem(IDC_EVENTS_LIST)->ShowWindow(SW_SHOW);
}

void CPageEvents::OnDeleteItemEventsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Typecast the specified notification structure
  NM_LISTVIEW* pLV = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);
  
  // Typecast the item's param as an XML element pointer
  IXMLDOMElement* pElement =
    reinterpret_cast<IXMLDOMElement*>(pLV->lParam);

  // Release the XML element pointer associated with the item
  if (pElement)
    pElement->Release();

  // Clear the result
  *pResult = 0;
}

void CPageEvents::OnColumnClickEventsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Typecast the specified notification structure
  NM_LISTVIEW* pLV = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);

  // Reverse the sort order if column is the same as current sort column
  if (pLV->iSubItem == m_nSortColumn)
    m_nSortDirection *= -1;
  else
    m_nSortDirection = 1;

  // Save the new sort order
  m_nSortColumn = pLV->iSubItem;      

  // Sort the items in the list
  m_listEvents.SortItems(SortProcFromColumn(m_nSortColumn),
    reinterpret_cast<DWORD>(this));

  // Clear the result
  *pResult = 0;
}
