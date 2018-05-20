/////////////////////////////////////////////////////////////////////////////
// PigBehaviorScriptType.cpp : Implementation of the CPigBehaviorScriptType
// class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCLib\ExcepInfo.h>
#include <..\TCAtl\ObjectMap.h>
#include <AGC.h>

#include "PigBehaviorScriptType.h"
#include "PigBehaviorScript.h"



/////////////////////////////////////////////////////////////////////////////
// CPigBehaviorScriptType

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPigBehaviorScriptType)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigBehaviorScriptType::CPigBehaviorScriptType() :
  m_dwAttributes(0),
  m_clsidEngine(CLSID_NULL),
  m_bAppearsValid(false),
  m_spAs(NULL),
  m_spAse(NULL),
  m_spAsp(NULL)
{
  m_ftModified.dwLowDateTime = m_ftModified.dwHighDateTime = 0;
}

HRESULT CPigBehaviorScriptType::FinalConstruct()
{
  // Indicate success
  return S_OK;
}

void CPigBehaviorScriptType::FinalRelease()
{
}

HRESULT CPigBehaviorScriptType::Load(const WIN32_FIND_DATA* pffd,
  tstring strFileName)
{
  XLock lock(this);

  // Save the file information
  m_dwAttributes = pffd->dwFileAttributes;
  m_ftModified = pffd->ftLastWriteTime;
  m_strFileName = strFileName;

  // Parse the specified script file
  HRESULT hr = ParseScriptFile(NULL);

  // Save the IErrorInfo, if any
  IErrorInfoPtr spei;
  HRESULT hrEI = GetErrorInfo(0, &spei);

  // Set the m_bAppearsValid flag according the result of parsing the script
  if (!(m_bAppearsValid = SUCCEEDED(hr)))
    Unload();

  // Release the scripting engine
  ShutdownScriptingEngine();

  // If there was an error, restore it
  SetErrorInfo(0, spei);

  // Return the last result
  return hr;
}


void CPigBehaviorScriptType::Unload()
{
  // Shutdown the scripting engine
  ShutdownScriptingEngine();

  // Reset the CLSID of the scripting engine
  m_clsidEngine = CLSID_NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Operations

void CPigBehaviorScriptType::AddBehavior(DWORD dwGITCookie)
{
  XLock lock(this);

  // Ensure that the specified behavior is not already in the collection
  assert(m_Behaviors.end() == m_Behaviors.find(dwGITCookie));

  // Ensure that the specified cookie is in the GIT
  IPigBehaviorPtr spBehavior;
  assert(SUCCEEDED(GetEngine().GetInterfaceFromGlobal(dwGITCookie,
    IID_IPigBehavior, (void**)&spBehavior)));

  // AddRef ourself
  AddRef();

  // Add the cookie to the collection
  m_Behaviors.insert(dwGITCookie);
}

void CPigBehaviorScriptType::RemoveBehavior(DWORD dwGITCookie)
{
  // Find the specified behavior cookie in the collection
  XLock lock(this);
  XBehaviorIt it = m_Behaviors.find(dwGITCookie);
  assert(m_Behaviors.end() != it);

  // Remove the instance from our collection
  m_Behaviors.erase(it);

  // Release ourself
  Release();
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

HRESULT CPigBehaviorScriptType::ParseScriptFile(BSTR bstrFileName,
  ITCStrings** ppScripts)
{
  // Only validate if ppScripts is non-NULL
  bool bValidate = !ppScripts;

  // Reset the collection of InvokeCommand strings when validating
  if (bValidate)
    m_spInvokeCommands = NULL;

  // Create an XML document instance
  IXMLDocumentPtr spXMLDoc;
  RETURN_FAILED(spXMLDoc.CreateInstance(CLSID_XMLDocument));

  // Open the specified document
  CComBSTR bstrTemp;
  if (!bstrFileName)
  {
    RETURN_FAILED(get_FilePathName(&bstrTemp));
    bstrFileName = bstrTemp;
  }
  RETURN_FAILED(spXMLDoc->put_URL(bstrFileName));

  // Set the full script file path name to the base class
  USES_CONVERSION;
  IActiveScriptSiteImplBase::SetScriptFileName(tstring(OLE2CT(bstrFileName)));

  // Get the root element
  IXMLElementPtr spRoot;
  RETURN_FAILED(spXMLDoc->get_root(&spRoot));

  // Ensure that the root element is a <pigbehavior> element
  if (bValidate)
  {
    CComBSTR bstrTag;
    RETURN_FAILED(spRoot->get_tagName(&bstrTag));
    if (0 != _wcsicmp(bstrTag, L"pigbehavior"))
      return Error(IDS_E_FILEFMT_BEHAVIOR, IID_IPigBehaviorScriptType);
  }

  // Process the language attribute of the root element
  RETURN_FAILED(ProcessRootAttributes(spRoot, bValidate));

  // Get the collection of child elements
  IXMLElementCollectionPtr spElements;
  RETURN_FAILED(spRoot->get_children(&spElements));

  // Search for the <registration> element
  if (NULL != spElements)
  {
    IDispatchPtr spDispatch;
    if (SUCCEEDED(spElements->item(_variant_t(L"registration"), _variant_t(),
      &spDispatch)))
    {
      IXMLElementPtr spReg(spDispatch);
      IXMLElementCollectionPtr spRegs(spDispatch);
      if (bValidate && NULL != spRegs)
        return Error(IDS_E_FILEFMT_MULTIREGS, IID_IPigBehaviorScriptType);

      // Process the <registration> element
      if (bValidate && NULL != spReg)
      {
        // Get the collection of child elements
        IXMLElementCollectionPtr spRegElements;
        RETURN_FAILED(spReg->get_children(&spRegElements));

        // Get the collection of <InvokeCommand> elements, if any
        RETURN_FAILED(spRegElements->item(_variant_t(L"invokecommand"),
          _variant_t(), &spDispatch));
        IXMLElementPtr spCmd(spDispatch);
        IXMLElementCollectionPtr spCmds(spDispatch);
        if (NULL != spCmd)
        {
          RETURN_FAILED(AddInvokeCommand(spCmd));
        }
        else if (NULL != spCmds)
        {
          long cElements;
          RETURN_FAILED(spCmds->get_length(&cElements));
          for (_variant_t i(0L), v; V_I4(&i) < cElements; ++V_I4(&i))
          {
            IDispatchPtr spCmdDisp;
            RETURN_FAILED(spCmds->item(i, v, &spCmdDisp));
            RETURN_FAILED(AddInvokeCommand(IXMLElementPtr(spCmdDisp)));
          }
        }
      }
    }

    // Create the collection of script strings
    ITCStringsPtr spScriptStrings;
    RETURN_FAILED(spScriptStrings.CreateInstance(CLSID_TCStrings));

    // Search for <script> elements
    RETURN_FAILED(spElements->item(_variant_t(L"script"), _variant_t(),
      &spDispatch));
    IXMLElementPtr spScript(spDispatch);
    IXMLElementCollectionPtr spScripts(spDispatch);
    if (NULL != spScript)
    {
      RETURN_FAILED(ProcessScriptElement(spScript, spScriptStrings,
        bValidate));
    }
    else if (NULL != spScripts)
    {
      long cElements;
      RETURN_FAILED(spScripts->get_length(&cElements));
      for (_variant_t i(0L), v; V_I4(&i) < cElements; ++V_I4(&i))
      {
        IDispatchPtr spScriptDisp;
        RETURN_FAILED(spScripts->item(i, v, &spScriptDisp));
        RETURN_FAILED(ProcessScriptElement(IXMLElementPtr(spScriptDisp),
          spScriptStrings, bValidate));
      }
    }
    else if (bValidate)
      return Error(IDS_E_FILEFMT_NOSCRIPTS, IID_IPigBehaviorScriptType);

    // Iterate thru collection of script strings, add them to the script engine
    if (bValidate)
    {
      long cScriptsStrings;
      RETURN_FAILED(spScriptStrings->get_Count(&cScriptsStrings));
      for (_variant_t i(0L); V_I4(&i) < cScriptsStrings; ++V_I4(&i))
      {
        CComBSTR bstrScript;
        RETURN_FAILED(spScriptStrings->get_Item(&i, &bstrScript));
        TCEXCEPINFO ei;
        HRESULT hr = m_spAsp->ParseScriptText(bstrScript, NULL, NULL, NULL,
          0, 0, 0L, NULL, &ei);
        if (FAILED(hr))
        {
          Unload();
          _SVERIFYE(ei.SetErrorInfo());
          return hr;
        }
      }

      // Begin executing the script
      RETURN_FAILED(m_spAs->SetScriptState(SCRIPTSTATE_CONNECTED));

      // Get the scripting engine's IDispatch
      IDispatchPtr spd;
      RETURN_FAILED(m_spAs->GetScriptDispatch(NULL, &spd));

      // Resolve the interface methods to script method DISPID's
      RETURN_FAILED(m_Methods.ResolveMethods(GetObjectCLSID(), spd, this));
    }

    // Detach the collection of script strings to the [out] parameter
    if (ppScripts)
      *ppScripts = spScriptStrings.Detach();
  }
  else if (bValidate)
    return Error(IDS_E_FILEFMT_NOSCRIPTS, IID_IPigBehaviorScriptType);

  // Indicate success
  return S_OK;
}

HRESULT CPigBehaviorScriptType::ProcessRootAttributes(IXMLElement* pRoot,
  bool bValidate)
{
  // Only process this if we are validating
  if (bValidate)
  {
    // Get the "language" attribute of the XML element
    CComVariant var;
    RETURN_FAILED(pRoot->getAttribute(L"language", &var));
    RETURN_FAILED(var.ChangeType(VT_BSTR));

    // Validate that the "language" attribute is not empty
    if (!V_BSTR(&var) || !SysStringLen(V_BSTR(&var)))
      return Error(IDS_E_FILEFMT_SCRIPTLANG, IID_IPigBehaviorScriptType);

    // Create the object to ensure that it's a supported scripting engine
    if (FAILED(CreateScriptingEngine(V_BSTR(&var))) || NULL == m_spAsp)
      return Error(IDS_E_FILEFMT_SCRIPTLANG, IID_IPigBehaviorScriptType);

    // Use the file name as the behavior name
    TCHAR szName[_MAX_FNAME];
    _tsplitpath(m_strFileName.c_str(), NULL, NULL, szName, NULL);
    m_strName = szName;

    // Add the script name as an invoke command
    CComBSTR bstrName;
    _SVERIFYE(get_Name(&bstrName));
    RETURN_FAILED(AddInvokeCommand(bstrName));

    // TODO: Get the description of the behavior

    // Get the optional "basebehavior" attribute of the XML element
    if (SUCCEEDED(pRoot->getAttribute(L"basebehavior", &var))
      && SUCCEEDED(var.ChangeType(VT_BSTR)) && BSTRLen(V_BSTR(&var)))
    {
      USES_CONVERSION;
      m_strBaseBehavior = OLE2CT(V_BSTR(&var));
    }
  }

  // Indicate success
  return S_OK;  
}

HRESULT CPigBehaviorScriptType::AddInvokeCommand(IXMLElement* pCmd)
{
  // Get the text of the XML element
  CComBSTR bstr;
  RETURN_FAILED(pCmd->get_text(&bstr));

  // Add the <InvokeCommand> string to the collection
  return AddInvokeCommand(bstr);
}

HRESULT CPigBehaviorScriptType::AddInvokeCommand(BSTR bstr)
{
  // Validate the <InvokeCommand> element text
  UINT cch = SysStringLen(bstr);
  if (!cch || wcscspn(bstr, L" \r\n\t") < cch)
    return Error(IDS_E_FILEFMT_INVOKECMD, IID_IPigBehaviorScriptType);

  // Add the <InvokeCommand> string to the collection
  if (NULL == m_spInvokeCommands)
    RETURN_FAILED(m_spInvokeCommands.CreateInstance(CLSID_TCStrings));
  RETURN_FAILED(m_spInvokeCommands->Add(bstr));

  // Indicate success
  return S_OK;
}

HRESULT CPigBehaviorScriptType::ProcessScriptElement(IXMLElement* pScr,
  ITCStrings* pScripts, bool bValidate)
{
  CComVariant var;
  CComBSTR bstr;

  // Get the "src" attribute of the XML element, if any
  CComBSTR bstrSrc;
  if SUCCEEDED(pScr->getAttribute(_bstr_t(L"src"), &var))
  {
    RETURN_FAILED(var.ChangeType(VT_BSTR));
    bstrSrc = V_BSTR(&var);
    if (bstrSrc.Length())
    {
      // Ensure that the text of the script is nothing more than whitespace
      if (bValidate)
      {
        // Get the text of the element
        CComBSTR bstrText;
        RETURN_FAILED(pScr->get_text(&bstrText));

        // Ensure that the text of the script is nothing more than whitespace
        if (bstrText.Length() && bstrText.Length() != wcsspn(bstrText, L" \r\n\t"))
          return Error(IDS_E_FILEFMT_SCRIPTSRCNOTEMPTY, IID_IPigBehaviorScriptType);
      }

      // Ensure that we have an absolute URL
      if (S_FALSE == IsValidURL(NULL, bstrSrc, 0))
      {
        // Get the script directory
        CComBSTR bstrScriptDir;
        RETURN_FAILED(GetEngine().get_ScriptDir(&bstrScriptDir));

        // Create an absolute URL from the relative one specified
        OLECHAR szCombined[_MAX_PATH * 2];
        DWORD cchCombined = sizeofArray(szCombined);
        RETURN_FAILED(CoInternetCombineUrl(bstrScriptDir, bstrSrc, 0,
          szCombined, cchCombined, &cchCombined, 0));

        // Save the absolute URL into the original BSTR
        bstrSrc = szCombined;
      }

      // Open the specified script file
	  printf("Loading script include: %S \n", bstrSrc);
      USES_CONVERSION;
      IStreamPtr spstm;
      RETURN_FAILED(URLOpenBlockingStream(NULL, OLE2CT(bstrSrc), &spstm, 0, NULL));

      // Get the size of the stream
      const LARGE_INTEGER nPosZero = {0, 0};
      ULARGE_INTEGER cbFileLarge;
      _SVERIFYE(spstm->Seek(nPosZero, STREAM_SEEK_END, &cbFileLarge));
      assert(0 == cbFileLarge.HighPart);
      DWORD cbFile = cbFileLarge.LowPart;

      // Create a buffer to contain the stream and its conversion to UNICODE
      int cchTotal = cbFile * (sizeof(char) + sizeof(WCHAR));
      TCArrayPtr<char*> pcFile = new char[cchTotal];
      if (pcFile.IsNull())
        return E_OUTOFMEMORY;

      // Read the stream into the memory block
      _SVERIFYE(spstm->Seek(nPosZero, STREAM_SEEK_SET, NULL));
      RETURN_FAILED(spstm->Read(pcFile, cbFile, NULL));

      // Convert the memory to UNICODE
      LPWSTR pwcFile = LPWSTR(pcFile + cbFile);
      int cch = MultiByteToWideChar(CP_ACP, 0, pcFile, cbFile, pwcFile, cbFile);

      // Create a BSTR to contain the file text
      CComBSTR bstrFileText(cch, pwcFile);

      // Add the text to the collection of scripts
      RETURN_FAILED(pScripts->Add(bstrFileText));
    }
  }

  // If "src" attribute of the XML element was empty, save the element text
  if (!bstrSrc.Length())
  {
    // Get the collection of child elements
    IXMLElementCollectionPtr spElements;
    RETURN_FAILED(pScr->get_children(&spElements));

    // Get the count of child elements
    long cElements;
    RETURN_FAILED(spElements->get_length(&cElements));

    // Find the first element of text
    IXMLElementPtr spText;
    for (_variant_t i(0L), v; V_I4(&i) < cElements; ++V_I4(&i))
    {
      IDispatchPtr spDispatch;
      RETURN_FAILED(spElements->item(i, v, &spDispatch));
      IXMLElementPtr spItem(spDispatch);
      if (NULL != spItem)
      {
        long eType;
        RETURN_FAILED(spItem->get_type(&eType));
        if (XMLELEMTYPE_TEXT == eType)
        {
          spText = spItem;
          break;
        }
      }
    }

    // Get the full text of the element
    if (NULL != spText)
    {
      // Get the text of the element
      CComBSTR bstrText;
      RETURN_FAILED(spText->get_text(&bstrText));

      // Add the text to the collection of scripts
      RETURN_FAILED(pScripts->Add(bstrText));
    }
  }

  // Indicate success
  return S_OK;
}

HRESULT CPigBehaviorScriptType::CreateScriptingEngine(BSTR bstrProgID)
{
  // Find the CLSID of the specified language
  if (FAILED(CLSIDFromProgID(bstrProgID, &m_clsidEngine)))
    return Error(IDS_E_FILEFMT_SCRIPTLANG, IID_IPigBehaviorScriptType);

  // Create the specified object
  assert(NULL == m_spAs && NULL == m_spAsp);
  //if (FAILED(m_spAsp->CreateInstance(m_clsidEngine)) || NULL == m_spAsp)
  //  return Error(IDS_E_FILEFMT_SCRIPTLANG, IID_IPigBehaviorScriptType);
  //m_spAs = m_spAsp;
  //assert(NULL != m_spAs);
  if (FAILED(CoCreateInstance(m_clsidEngine, 0, CLSCTX_ALL, 
                 IID_IActiveScript, 
                 (void **)&m_spAs)) || NULL == m_spAs)

		return Error(IDS_E_FILEFMT_SCRIPTLANG, IID_IPigBehaviorScriptType);
  //m_spAs = m_spAsp;
  m_spAs->QueryInterface(IID_IActiveScriptParse, (void **)&m_spAsp);
  assert(NULL != m_spAsp);

  // Set the script site
  RETURN_FAILED(m_spAs->SetScriptSite(this));

  // Initialize the script parse
  RETURN_FAILED(m_spAsp->InitNew());

  // Note: We don't add our named objects since we don't run the script from
  // this class. We only attempt to parse/validate the script.

  // Add our TypeLib
  RETURN_FAILED(m_spAs->AddTypeLib(LIBID_PigsLib, 1, 0, 0));

  // Indicate success
  return S_OK;
}

void CPigBehaviorScriptType::ShutdownScriptingEngine()
{
  // Shutdown the scripting engine
  if (NULL != m_spAsp)
  {
    m_spAsp = NULL;
  }
  if (NULL != m_spAs)
  {
    m_spAs->Close();
    m_spAs = NULL;
  }
}

void CPigBehaviorScriptType::TriggerBaseError(AGCEventID idEvent)
{
  // Trigger an event
  USES_CONVERSION;
  _AGCModule.TriggerEvent(NULL, idEvent, GetName().c_str(), -1, -1, -1, 4,
    "FileName"      , VT_LPWSTR, T2CW(GetScriptFileName().c_str()),
    "BaseBehavior"  , VT_LPWSTR, T2CW(GetBaseBehaviorName().c_str()),
    "Line"          , VT_I4    , 0,
    "Column"        , VT_I4    , 0);
}


/////////////////////////////////////////////////////////////////////////////
// IPigBehaviorType Interface Methods

STDMETHODIMP CPigBehaviorScriptType::get_InvokeCommands(ITCStrings** ppStrings)
{
  CLEAROUT(ppStrings, (ITCStrings*)NULL);
  XLock lock(this);

  if (NULL == m_spInvokeCommands)
    RETURN_FAILED(m_spInvokeCommands.CreateInstance(CLSID_TCStrings));
  (*ppStrings = m_spInvokeCommands)->AddRef();
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_Name(BSTR* pbstrName)
{
  CLEAROUT(pbstrName, (BSTR)NULL);
  XLock lock(this);
  *pbstrName = CComBSTR(m_strName.c_str()).Detach();
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_Description(BSTR* pbstrDescription)
{
  // Initialize the [out] parameters
  CLEAROUT(pbstrDescription, (BSTR)NULL);

  // TODO: Get a description
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_Behaviors(IPigBehaviors** ppBehaviors)
{
  // TODO: Implement this
  return E_NOTIMPL;
}

STDMETHODIMP CPigBehaviorScriptType::get_IsScript(VARIANT_BOOL* pbIsScript)
{
  CLEAROUT(pbIsScript, VARIANT_TRUE);
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_BaseBehaviorType(
  IPigBehaviorType** ppBaseType)
{
  // Initialize the [out] parameter
  CLEAROUT(ppBaseType, (IPigBehaviorType*)NULL);

  // Do nothing if there is not a base behavior name
  XLock lock(this);
  if (m_strBaseBehavior.empty())
    return S_OK;

  // Get a pointer to the base behavior type
  CPigBehaviorScriptType* pType =
    GetEngine().GetBehaviorType(CComBSTR(m_strBaseBehavior.c_str()));
  assert(pType);
  return pType->QueryInterface(IID_IPigBehaviorType, (void**)ppBaseType);
}

STDMETHODIMP CPigBehaviorScriptType::put_AppearsValid(
  VARIANT_BOOL bAppearsValid)
{
  XLock lock(this);
  m_bAppearsValid = !!bAppearsValid;
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_AppearsValid(
  VARIANT_BOOL* pbAppearsValid)
{
  XLock lock(this);
  CLEAROUT(pbAppearsValid, VARBOOL(m_bAppearsValid));
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IPigBehaviorScriptType Interface Methods

STDMETHODIMP CPigBehaviorScriptType::get_FilePathName(BSTR* pbstrFilePathName)
{
  CLEAROUT(pbstrFilePathName, (BSTR)NULL);

  // Get the script directory and file name strings
  CComBSTR bstr;
  RETURN_FAILED(GetEngine().get_ScriptDir(&bstr));

  // Create the fully-qualified path name of the script file
  OLECHAR szPathName[_MAX_PATH];
  USES_CONVERSION;
  _wmakepath(szPathName, NULL, bstr, T2COLE(m_strFileName.c_str()), NULL);

  // Create and return a BSTR
  *pbstrFilePathName = CComBSTR(szPathName).Detach();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_FileName(BSTR* pbstrFileName)
{
  CLEAROUT(pbstrFileName, (BSTR)NULL);
  XLock lock(this);
  *pbstrFileName = CComBSTR(m_strFileName.c_str()).Detach();
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_FileModifiedTime(DATE* pdate)
{
  CLEAROUT(pdate, (DATE)0.0);
  XLock lock(this);
  return FileTimeToVariantTime(&m_ftModified, pdate) ?
    S_OK : HRESULT_FROM_WIN32(GetLastError());
}

STDMETHODIMP CPigBehaviorScriptType::get_FileAttributes(
  FileAttributes* peFileAttributes)
{
  XLock lock(this);
  CLEAROUT(peFileAttributes, (FileAttributes)m_dwAttributes);
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_FileText(BSTR* pbstrFileText)
{
  // Initialize the [out] parameters
  CLEAROUT(pbstrFileText, (BSTR)NULL);

  // Get the fully-qualified path name of the script file
  CComBSTR bstrPathName;
  RETURN_FAILED(get_FilePathName(&bstrPathName));

  // Open the file
  USES_CONVERSION;
  TCHandle hFile = CreateFile(OLE2CT(bstrPathName), GENERIC_READ,
    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE == hFile)
    return HRESULT_FROM_WIN32(GetLastError());

  // Get the size of the file
  DWORD cbFile = GetFileSize(hFile, NULL);

  // Create a buffer to contain the file and its conversion to UNICODE
  int cchTotal = cbFile * (sizeof(char) + sizeof(WCHAR));
  TCArrayPtr<char*> pcFile = new char[cchTotal];
  if (pcFile.IsNull())
    return E_OUTOFMEMORY;

  // Read the file into the memory block
  DWORD cbRead = 0;
  if (!ReadFile(hFile, pcFile, cbFile, &cbRead, NULL))
    return HRESULT_FROM_WIN32(GetLastError());

  // Convert the memory to UNICODE
  LPWSTR pwcFile = LPWSTR(pcFile + cbFile);
  int cch = MultiByteToWideChar(CP_ACP, 0, pcFile, cbFile, pwcFile, cbFile);

  // Create a BSTR to contain the file text
  *pbstrFileText = SysAllocStringLen(pwcFile, cch);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_ScriptText(ITCStrings** ppScriptStrings)
{
  CLEAROUT(ppScriptStrings, (ITCStrings*)NULL);
  return ParseScriptFile(NULL, ppScriptStrings);
}

STDMETHODIMP CPigBehaviorScriptType::get_ScriptEngineProgID(BSTR* pbstrProgID)
{
  // Initialize the [out] parameters
  CLEAROUT(pbstrProgID, (BSTR)NULL);

  // Convert the scripting engine's CLSID to a ProgID string
  XLock lock(this);
  LPOLESTR pszProgID;
  RETURN_FAILED(ProgIDFromCLSID(m_clsidEngine, &pszProgID));

  // Create a BSTR from the ProgID string and release the string
  *pbstrProgID = SysAllocString(pszProgID);
  CoTaskMemFree(pszProgID);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigBehaviorScriptType::get_ScriptEngineCLSID(BSTR* pbstrCLSID)
{
  // Initialize the [out] parameters
  CLEAROUT(pbstrCLSID, (BSTR)NULL);

  // Create a BSTR from the scripting engine's CLSID
  XLock lock(this);
  *pbstrCLSID = BSTRFromGUID(m_clsidEngine).copy();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigBehaviorScriptType::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigBehaviorScriptType,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IActiveScriptSite Interface Methods

STDMETHODIMP CPigBehaviorScriptType::GetItemInfo(LPCOLESTR pstrName,
  DWORD dwReturnMask, IUnknown** ppunkItem, ITypeInfo** ppti)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(ppunkItem,  (IUnknown*)NULL);
  CLEAROUT_ALLOW_NULL(ppti     , (ITypeInfo*)NULL);

  // Note: We don't add our named objects since we don't run the script from
  // this class. We only attempt to parse/validate the script.
  return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP CPigBehaviorScriptType::OnScriptError(IActiveScriptError* pScriptError)
{
	EXCEPINFO info;
	BSTR line;
	DWORD sourceContext;
	ULONG lineNumber;
	LONG charPosition;
	pScriptError->GetExceptionInfo(&info);
	pScriptError->GetSourceLineText(&line);
	pScriptError->GetSourcePosition(&sourceContext, &lineNumber, &charPosition);

	printf("Script Error!\n");
	printf("Exception Info: %S\n", info.bstrDescription);
	//printf("Source Line Text: %S\n", line);
	printf("Source Position: %ld, Line: %u, charPosition: %ld\n", sourceContext, lineNumber, charPosition);

  // Save error information
  m_spAse = pScriptError;

  // Indicate that the script is invalid
  m_bAppearsValid = false;

  // Perform default processing
  return IActiveScriptSiteImplBase::OnScriptError(pScriptError);
}

