/*-------------------------------------------------------------------------
  allegdb.h
  
  Allegiance database layer (the OLE DB one, as opposed to the ODBC one, which also exists)
  
  Owner: 
  
  Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#ifndef _ALLEGDB_H_
#define _ALLEGDB_H_

#include <oledb.h>
#include <sqloledb.h>
#include <atldbcli.h>
#include <..\test\TCLib\TCThread.h>
#include <mutex>

class CSQLCore;
class CSQLQuery;
class CSQLThread;
class CSQLQueueThread;

typedef Mlist_utl<CSQLQuery*> MListQueries;
typedef Mlink_utl<CSQLQuery*> MLinkQueries;
typedef Slist_utl<CSQLQuery*> SListQueries;
typedef Slink_utl<CSQLQuery*> SLinkQueries;

extern unsigned CALLBACK SQLThreadProc(void * pvsqlThread);
extern unsigned CALLBACK SQLQueueProc(void * pvSQLQueueThread);



class ISQLSite2 : public IObject
{
public:  
  virtual void OnSQLErrorRecord(SSERRORINFO * perror, OLECHAR * postrError) {};
  virtual void OnOLEDBErrorRecord(BSTR bstrDescription, GUID guid, DWORD dwHelpContext, 
                                  BSTR bstrHelpFile,    BSTR bstrSource ) {};
};


/*-------------------------------------------------------------------------
 * CSQLCore
 *-------------------------------------------------------------------------
      Keeper of everything sql. Maintains two sql euery queues, one for 
      queries that require a response, and one for queries that don't
 */
class CSQLCore
{
public:
  CSQLCore(ISQLSite2 * psqlsite) :
    m_cSilentThreads(0),
    m_cNotifyThreads(0),
    m_hKillSQLEvent(0),
    m_pthdQueue(NULL),
    m_hQueryNotify(NULL),
    m_hQuerySilent(NULL),
    m_nThreadIDNotify(0),
    m_psqlsite(psqlsite)
  {}

  ~CSQLCore();

  HRESULT Init(LPCOLESTR strSQLConfig, DWORD nThreadIDNotify, DWORD cSilentThreads, DWORD cNotifyThreads);

  void PostQuery(CSQLQuery * pquery);

  DWORD GetCountSilentThreads()
  {
    return m_cSilentThreads;
  }

  DWORD GetCountNotifyThreads()
  {
    return m_cNotifyThreads;
  }

  const CDataSource & GetDataSource()
  {
    return m_connection;
  }

  HANDLE GetNotifySemaphore()
  {
    return m_hQueryNotify;
  }

  HANDLE GetSilentSemaphore()
  {
    return m_hQuerySilent;
  }

  CSQLQuery * GetNotifyQuery()
  {
    m_listQueriesNotify.lock();
    MLinkQueries * pl = m_listQueriesNotify.first();
    CSQLQuery * pquery = pl->data();
    pl->unlink();
    m_listQueriesNotify.unlock();
    return pquery;
  }

  CSQLQuery * GetSilentQuery()
  {
    m_listQueriesSilent.lock();
    MLinkQueries * pl = m_listQueriesSilent.first();
    CSQLQuery * pquery = pl->data();
    pl->unlink();
    m_listQueriesSilent.unlock();
    return pquery;
  }

  const DWORD GetNotifyThreadID()
  {
    return m_nThreadIDNotify;
  }

  void DumpErrorInfo(IUnknown * punk, IID iid, bool * pfRetry)
  {
    std::lock_guard<std::mutex> lock(m_cs);

	CDBErrorInfo errorInfo;
    ULONG        cRecords;
    HRESULT      hr;
    ULONG        iError; 
    CComBSTR     bstrDesc, bstrHelpFile, bstrSource;
    GUID         guid;
    DWORD        dwHelpContext;
    USES_CONVERSION;
    TRef<ISQLServerErrorInfo> psqlError;
    IMalloc * pMalloc;

    if (pfRetry)
      *pfRetry = false;
    
    hr = CoGetMalloc(1, &pMalloc);
    ZSucceeded(hr);
    LCID lcLocale = GetSystemDefaultLCID();
    hr = errorInfo.GetErrorRecords(punk, iid, &cRecords);

    if (SUCCEEDED(hr) && errorInfo.m_spErrorInfo)
    {
      for (iError = 0; iError < cRecords; iError++)
      {
        hr = errorInfo.GetAllErrorInfo(iError, lcLocale, &bstrDesc, &bstrSource, &guid, &dwHelpContext, &bstrHelpFile);
        ZSucceeded(hr);
        
        hr = errorInfo.GetCustomErrorObject(iError, IID_ISQLServerErrorInfo, (IUnknown**) &psqlError);
        SSERRORINFO * pssError = NULL;
        OLECHAR * postrError = NULL;
        if (SUCCEEDED(hr)) // sql specific info--in which case we don't care about the ole db error, which just contains less info
        {
          hr = psqlError->GetErrorInfo(&pssError, &postrError);
          if (SUCCEEDED(hr) && pssError && postrError) // I've seen these not get set--don't know why
          {
            m_psqlsite->OnSQLErrorRecord(pssError, postrError);
            if (1205 == pssError->lNative ||  // deadlocked transaction
                7312 == pssError->lNative)   // timeout
            {
              assert (pfRetry); // they better be prepared to retry
              *pfRetry = true;
            }
          }
        }

        if (FAILED(hr) || (!pssError && !postrError))
        {
          m_psqlsite->OnOLEDBErrorRecord(bstrDesc, guid, dwHelpContext, bstrHelpFile, bstrSource);
        }
          
        bstrSource.Empty();
        bstrDesc.Empty();
        bstrHelpFile.Empty();
        if (pssError)
          pMalloc->Free(pssError);
        if (postrError)
          pMalloc->Free(postrError);
      }
    }
    else
    {
      GUID guid;
      ZeroMemory(&guid, sizeof(guid));
      
      m_psqlsite->OnOLEDBErrorRecord(L"errorInfo.GetErrorRecords failed, which is bad (and unexpected).", guid, 0, L"", L"");
      Sleep(1000); // give the debug output thread a chance to write the error to the file
      *(DWORD*)0 = 0; // we can't assert from any thread, so let's just hard break
    }

    if (!(pfRetry && *pfRetry)) // let's take a look at any database errors we get, since we don't handle them gracefully
    {
      Sleep(1000); // give the debug output thread a chance to write the error to the file
      *(DWORD*)0 = 0; // we can't assert from any thread, so let's just hard break
      debugf("!!DBERR: retry not attempted !!!!\n");
    }

    return;
  }
    



private:
  friend class CSQLQueueThread;
  void AddQuery(CSQLQuery * pquery);
  
  HANDLE        m_hKillSQLEvent;
  HANDLE        m_hQueryNotify; // signaled when there's a query available that requires notification
  HANDLE        m_hQuerySilent; // signaled when there's a query available that DOESN'T require notification
  CSQLQueueThread * m_pthdQueue;
  CSQLThread ** m_pargSilentThreads;
  CSQLThread ** m_pargNotifyThreads;
  DWORD         m_cSilentThreads;
  DWORD         m_cNotifyThreads;
  ZString       m_strSQLConfig;
  CDataSource   m_connection;
  MListQueries  m_listQueriesNotify; // queries that need a reply
  MListQueries  m_listQueriesSilent; // queries that are send and forget
  DWORD         m_nThreadIDNotify; // thread to send completion notification to
  std::mutex m_cs;
  TRef<ISQLSite2> m_psqlsite;
};


/*-------------------------------------------------------------------------
 * CSQLQuery
 *-------------------------------------------------------------------------
      Base class that all queries must derive from

      These are instantiated in two different way for two different purposes.
      The user can create these, fill in the data, and post CSQLCore::PostQuery() them.
      They are also created by the CSQLThread's to cache the prepared version in each
      thread for performance reasons. The instance the users creates is NEVER the actual 
      instance used to execute the query. It's always a copy.
 */
class CSQLQuery 
{
public:
  CSQLQuery() 
  {}
  virtual ~CSQLQuery() {};

// Mandatory Overrides
  virtual HRESULT     OnExecute() = 0;
  virtual HRESULT     OnPrepare(const CSession & session) = 0;
  virtual CSQLQuery * Copy(CSQLQuery * pqueryDest) = 0; // pqueryDest is either existing obj to copy into, or NULL for creation of new object
  virtual REFGUID     GetGuid() = 0;
  virtual TCHAR *     GetStrQuery() = 0;
  virtual void        DataReady()  = 0; // this happens in the main thread
  virtual bool        GetNotify() = 0;
  virtual IUnknown *  GetIUnknown() = 0;
  virtual bool GetCallbackOnMainThread() const = 0;
};


/*-------------------------------------------------------------------------
 * CSQLThread
 *-------------------------------------------------------------------------
      Waits for posted queries to appear in one of the two query queues,
      depending on which type of thread this is. Each thread will ONLY
      pull either queries that do, or do not require notification. 
      The derived query thread is responsible for actually executing the
      query and getting data. A completion message is posted to the main
      thread to synchronize access to database results.
 */
class CSQLThread 
{
public:
  CSQLThread(HANDLE hEventDie, bool fNotify, CSQLCore * psql) :
    m_hEventDie(hEventDie),
    m_fNotify(fNotify),
    m_psql(psql)
  {
    m_pThread = TCThread::BeginMsgThread(SQLThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    // TODO: if that fails?
    m_pThread->ResumeThread();
  }

  CSQLCore * GetSQLCore()
  {
    return m_psql;
  }

  HRESULT ServiceQuery(CSQLQuery * pquery);

  bool GetNotify()
  {
    return m_fNotify;
  }

  HRESULT Open()
  {
    HRESULT hr = m_session.Open(m_psql->GetDataSource());
    if (FAILED(hr))
      m_psql->DumpErrorInfo(m_session.m_spOpenRowset, IID_IOpenRowset, NULL);

    return hr;
  }

  HANDLE GetEventDie()
  {
    return m_hEventDie;
  }

  DWORD GetThreadID()
  {
    return m_pThread->m_nThreadID;
  }

private:
  TCThread *  m_pThread;
  HANDLE      m_hEventDie;
  CSession    m_session;
  bool        m_fNotify; // whether this thread services queries that require notification
  SListQueries m_listQueries; // these are the UNIQUE queries (by guid) that this thread has seen (and prepared) before, added on demand
  CSQLCore *  m_psql;
};


/*-------------------------------------------------------------------------
 * CSQLQueueThread
 *-------------------------------------------------------------------------
      Manages taking posted and putting them in the right queue
 */
class CSQLQueueThread
{
public: 
  CSQLQueueThread(HANDLE hEventDie, CSQLCore * psql) :
    m_hEventDie(hEventDie),
    m_psql(psql)
  {
    m_pThread = TCThread::BeginMsgThread(SQLQueueProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    // TODO: if that fails?
    m_pThread->ResumeThread();
  }

  void AddQuery(CSQLQuery * pquery)
  {
    m_psql->AddQuery(pquery);
  }


  HANDLE GetEventDie()
  {
    return m_hEventDie;
  }

  CSQLCore * GetSQLCore()
  {
    return m_psql;
  }

  DWORD GetThreadID()
  {
    return m_pThread->m_nThreadID;
  }

private:
  TCThread *  m_pThread;
  HANDLE      m_hEventDie;
  CSQLCore *  m_psql;
};


#define WM_SQL (WM_APP + 0x2000) // somewhat random, hopefully safe, range
#define wm_sql_addquery  WM_SQL     // Sent from outside in. LPARAM: CSQLQuery*, WPARAM: unused
#define wm_sql_querydone WM_SQL + 1 // Sent inside out.      LPARAM: CSQLQuery*, WPARAM: unused 

/*-------------------------------------------------------------------------
 * CQuery
 *-------------------------------------------------------------------------
      Templatized on the data structure (with column and/or parameter maps)
      used to shuttle data to/from the database
 */
template <class TQueryData, bool TfResultSet>
class CQuery : public CSQLQuery
{
public:
  CQuery(void (pfDataReady)(CQuery*)) : 
    m_pargQueryData(NULL),
    m_cRows(0),
    m_cRowsAlloc(0),
    m_fCallbackOnMainThread(true)
  {
    m_pfDataReady = pfDataReady;
    if (GUID_NULL == s_guid)
      CoCreateGuid(&s_guid);
  }

  ~CQuery()
  {
    ClearRows();
  }
  
  virtual REFGUID GetGuid()
  {
    return s_guid;
  }

  virtual void DataReady() // this happens in the main thread
  {
    if (m_pfDataReady)
      (*m_pfDataReady)(this);
    delete this; 
  }  

  virtual HRESULT OnExecute()
  {
    ClearRows();
    HRESULT hr = m_cmd.Open();
    if (FAILED(hr)) 
      return hr;
    
    if (TfResultSet)
    {
      // ok, because sql is the way it is, we can't know how many rows we're gonna get until we get 'em,
      // so it's either traverse (and copy) all the rows twice, or allocate as you go. I choose the latter.
      while (S_OK == (hr = m_cmd.MoveNext())) // end of rows is also success
      {
        m_cRows++;
        if (m_cRows > m_cRowsAlloc)
        {
          m_cRowsAlloc = m_cRows * 2;
          if (m_pargQueryData)
            m_pargQueryData = (TQueryData *) HeapReAlloc(GetProcessHeap(), 0, m_pargQueryData, sizeof(TQueryData) * m_cRowsAlloc);
          else
            m_pargQueryData = (TQueryData *) HeapAlloc(GetProcessHeap(), 0, sizeof(TQueryData) * m_cRowsAlloc);
        }
        m_pargQueryData[m_cRows - 1] = *GetData();
      }
    }

    if (SUCCEEDED(hr))
      m_cmd.Close();
    return hr;
  }

  virtual HRESULT OnPrepare(const CSession & session)
  {
    HRESULT hr = m_cmd.Create(session, GetStrQuery(), DBGUID_DBSQL);
    if (SUCCEEDED(hr))
      hr = m_cmd.Prepare(100000); // what's the right number for "a lot"?

    return hr;
  }
  
  virtual CSQLQuery * Copy(CSQLQuery * pqueryDest)  // pqueryDest is either existing obj to copy into, or NULL for creation of new object
  {
    if (!pqueryDest)
      pqueryDest = new CQuery(m_pfDataReady);

    REFGUID guid1 = pqueryDest->GetGuid();
    REFGUID guid2 = GetGuid();
    
    assert(guid1 == guid2); // we know it's the right type, so we can upcast
    // and then make a memory copy of just the user contents
    // NOT the size of the whole m_cmd, since we can't stomp on atl data members--atl also doesn't support copying accessors
    CQuery * pqueryDestT = static_cast<CQuery *>(pqueryDest);
    CopyMemory(&pqueryDestT->m_cmd, &m_cmd, sizeof(TQueryData));
	debugf("CSQLQuery->Copy: size = %d\n",sizeof(TQueryData));
    pqueryDestT->m_cRowsAlloc = m_cRowsAlloc; 
    pqueryDestT->m_pargQueryData = m_pargQueryData; 
    pqueryDestT->m_cRows = m_cRows;
    pqueryDestT->m_fCallbackOnMainThread = m_fCallbackOnMainThread;
    // only the copy owns the data now.
    m_pargQueryData = NULL;
    m_cRows = 0;
    m_cRowsAlloc = 0;
    m_fCallbackOnMainThread = true;

    return pqueryDest;
  }

  TQueryData * GetData()
  {
    return &m_cmd;
  }

  TQueryData * GetOutputRows(int * pcRows)
  {
    if (pcRows)
      *pcRows = (int) m_cRows;
    return m_pargQueryData;
  }

  virtual TCHAR * GetStrQuery()
  {
    return m_cmd.GetStrQuery();
  }

  void ClearRows()
  {
    assert(IFF(m_pargQueryData, m_cRowsAlloc > 0));
    if (m_pargQueryData)
    {
      HeapFree(GetProcessHeap(), 0, m_pargQueryData);
      m_pargQueryData = NULL;
      m_cRows = 0;
      m_cRowsAlloc = 0;
    }
  }

  bool GetNotify()
  {
    return !!m_pfDataReady;
  }

  IUnknown * GetIUnknown()
  {
    return m_cmd.m_spCommand;
  }

  void SetCallbackOnMainThread(bool fCallbackOnMainThread)
  {
    m_fCallbackOnMainThread = fCallbackOnMainThread;
  }
  virtual bool GetCallbackOnMainThread() const
  {
    return m_fCallbackOnMainThread;
  }
  
  CCommand<CAccessor<TQueryData> >  m_cmd;
  TQueryData *  m_pargQueryData;
  ULONG         m_cRows;
  ULONG         m_cRowsAlloc;
  void (*m_pfDataReady)(CQuery* pquery);
  bool m_fCallbackOnMainThread;

public: // just so the macro can easily set this  
  static GUID   s_guid;
};


// ------------- MACROS USED FOR CREATING QUERIES -----------------

// The queries get memmove'd, so NO objects that require special construction or external storage!
// N = Name of query class, typically CQ...
// R = Whether query generates any rowsets to be scanned 
// Q = Query command text
#define BEGIN_QUERY(N, R, Q) \
struct N##Data \
{ \
  TCHAR * GetStrQuery() \
  { \
    return TEXT(Q); \
  } 

#define END_QUERY(N, R) \
}; \
typedef class CQuery<N##Data, R> N;

  

#endif 
