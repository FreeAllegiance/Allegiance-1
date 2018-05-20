/* Messages.cpp
   Interface between WinTrek and FedSrv
   Owner: 
*/

#include "pch.h"
#include "messagecore.h"

//  <NKM> 25-Aug-2004
// For debug purposes....
#define GETSORC  const char* sOrC = m_pDirectPlayClient == 0 ? "S" : "C"
#define DPNGETLOCALHOSTADDRESSES_COMBINED               0x0001


const MsgClsPrio FedMessaging::c_mcpDefault = 1000;
static GUID g_guidApplication;

// Ok, this is kinda hacky, but it's too late to make wide-effect changes. The problem is that while in the OnAppMessage
// callback, they may delete the connection from which the message came. Then there may be another FEDMEESSAGE packed in 
// the dplay message. In order to process any FEDMESSAGE, we need the pfsPlayer, which is no longer valid if we've deleted
// the connection, so we need to set a flag when a connection is deleted, so we know not to process any more FEDMESSAGEs
// that might be part of the same packet. Note that this does not apply to other messages in different packets (dplay 
// messages), because in that case the private data for the player will be NULL, and we won't get the pfsPlayer
static bool g_fConnectionDeleted = false;

void DumpMemory()
{
  _CrtDumpMemoryLeaks();
}

template< class T > class SafeReleaser
{
public:
  SafeReleaser( T* t = 0 )
    : m_toRelease( t )
  {}

  ~SafeReleaser()
  {
    if ( m_toRelease )
      m_toRelease->Release();
  }


private:
  SafeReleaser( const SafeReleaser&  );
  SafeReleaser& operator = ( const SafeReleaser& );

  T* m_toRelease;
};

static const char* g_dpMsgids[] = {
  "unknown",
  "DPN_MSGID_ADD_PLAYER_TO_GROUP",
  "DPN_MSGID_APPLICATION_DESC",
  "DPN_MSGID_ASYNC_OP_COMPLETE",
  "DPN_MSGID_CLIENT_INFO",
  "DPN_MSGID_CONNECT_COMPLETE",
  "DPN_MSGID_CREATE_GROUP",
  "DPN_MSGID_CREATE_PLAYER",
  "DPN_MSGID_DESTROY_GROUP",
  "DPN_MSGID_DESTROY_PLAYER",
  "DPN_MSGID_ENUM_HOSTS_QUERY",
  "DPN_MSGID_ENUM_HOSTS_RESPONSE",
  "DPN_MSGID_GROUP_INFO",
  "DPN_MSGID_HOST_MIGRATE",
  "DPN_MSGID_INDICATE_CONNECT",
  "DPN_MSGID_INDICATED_CONNECT_ABORTED",
  "DPN_MSGID_PEER_INFO",
  "DPN_MSGID_RECEIVE",
  "DPN_MSGID_REMOVE_PLAYER_FROM_GROUP",
  "DPN_MSGID_RETURN_BUFFER",
  "DPN_MSGID_SEND_COMPLETE",
  "DPN_MSGID_SERVER_INFO",
  "DPN_MSGID_TERMINATE_SESSION",
  "DPN_MSGID_CREATE_THREAD",
  "DPN_MSGID_DESTROY_THREAD"
};


static const char* getMessageString( DWORD id )
{
  DWORD myMsgid = id & 0xFF;
  if ( myMsgid > 0x18 )
    myMsgid = 0;

  return g_dpMsgids[myMsgid];
}

//  <NKM> 08-Aug-2004
// Added
//-----------------------------------------------------------------------------
// Name: DXUtil_ConvertAnsiStringToWideCch()
// Desc: This is a UNICODE conversion utility to convert a CHAR string into a
//       WCHAR string.
//       cchDestChar is the size in TCHARs of wstrDestination.  Be careful not to
//       pass in sizeof(strDest)
//-----------------------------------------------------------------------------
static WCHAR* constChar2Wchar( const char* from )
{
  if( from == 0 )
    return 0;

  //  <NKM> 07-Aug-2004
  // first get the count
  //  CP_ACP - ansi code page
  //  next 0 is the flags for character options
  // from = char*  and -1 means the whole lot
  // *to is the output and 0 means count how much I need!
  WCHAR* to = 0;
  int n = MultiByteToWideChar( CP_ACP, 0, from, -1, to, 0 );

  if( n == 0 )
    return 0;

  //  <NKM> 07-Aug-2004 - allocate buffer and do the actual call
  to = new WCHAR[n];
  MultiByteToWideChar( CP_ACP, 0, from, -1, to, n );

  return to;
}



//  <NKM> 08-Aug-2004
// Added
//-----------------------------------------------------------------------------
// Name: DXUtil_ConvertWideStringToAnsi()
// Desc: This is a UNICODE conversion utility to convert a WCHAR string into a
//       CHAR string.
//       cchDestChar is the size in TCHARs of strDestination
//-----------------------------------------------------------------------------
static char* wChar2ConstChar( WCHAR* from )
{
  if( from == 0 )
    return 0;

  //  <NKM> 07-Aug-2004
  // first get the count
  //  CP_ACP - ansi code page
  //  next 0 is the flags for character options
  // from = char*  and -1 means the whole lot
  // *to is the output and 0 means count how much I need!
  // 0's on the end are coz we don't used default chars
  char* to = 0;
  int n = WideCharToMultiByte( CP_ACP, 0, from, -1, to, 0, 0, 0 );
  if( n == 0 )
    return 0;


  to = new char[n];
  WideCharToMultiByte( CP_ACP, 0, from, -1, to, n, 0, 0 );

  return to;
}


/*-------------------------------------------------------------------------
 * VerifyMessages
 *-------------------------------------------------------------------------
  Purpose:
    Make sure that our output buffer adheres to the expected format of a 
    string of valid FEDMESSAGEs.
  
  Side Effects:
    Important: The default recipient no longer remains the default if 
               SendMessages is called with a non-NULL precip
 */
void VerifyMessage(LPBYTE pb, CB cbTotal)
{
#ifndef NO_MSG_CRC
  // <sigh> we can't do this as long as we have the hack to support non-crc clients, since we munge the message length on ack from logon to lobby
#else  
  int cMsgs = 0;
  CB cbRunning = 0;
  while (cbRunning < cbTotal)
  {
    FEDMESSAGE * pfm = (FEDMESSAGE *) pb;
    assert(pfm->cbmsg > 0);
    assert(pfm->fmid > 0);
    cbRunning += pfm->cbmsg;
    pb += pfm->cbmsg;
    cMsgs++;
  }
  assert(cbRunning == cbTotal);
#endif
}



//  <NKM> 10-Aug-2004
// Changed to EnumHosts and made a member function
void FedMessaging::EnumHostsCallback ( const DPNMSG_ENUM_HOSTS_RESPONSE& resp )
{
#ifdef DEBUG  
  char szBuff[128];
  const DPN_APPLICATION_DESC* appDesc = resp.pApplicationDescription;
  wsprintf(szBuff, "Found a session: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} for application {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n", 
          appDesc->guidInstance.Data1,    appDesc->guidInstance.Data2,
          appDesc->guidInstance.Data3,    appDesc->guidInstance.Data4[0],
          appDesc->guidInstance.Data4[1], appDesc->guidInstance.Data4[2],
          appDesc->guidInstance.Data4[3], appDesc->guidInstance.Data4[4],
          appDesc->guidInstance.Data4[5], appDesc->guidInstance.Data4[6],
          appDesc->guidInstance.Data4[7],
          appDesc->guidApplication.Data1,    appDesc->guidApplication.Data2,
          appDesc->guidApplication.Data3,    appDesc->guidApplication.Data4[0],
          appDesc->guidApplication.Data4[1], appDesc->guidApplication.Data4[2],
          appDesc->guidApplication.Data4[3], appDesc->guidApplication.Data4[4],
          appDesc->guidApplication.Data4[5], appDesc->guidApplication.Data4[6],
          appDesc->guidApplication.Data4[7]);
  debugf(szBuff);
#endif  
  if ( IsEqualGUID( resp.pApplicationDescription->guidApplication, g_guidApplication ) )
  {
    FMSessionDesc fmSession( resp.pApplicationDescription );

    m_guidInstance = resp.pApplicationDescription->guidInstance;

    if ( m_pHostAddress != 0 )
      m_pHostAddress->Release();

    if ( m_pDeviceAddress != 0)
      m_pDeviceAddress->Release();

    ZSucceeded( resp.pAddressSender->QueryInterface( IID_IDirectPlay8Address,
                                                (LPVOID*) &m_pHostAddress ) );

    ZSucceeded( resp.pAddressDevice->QueryInterface( IID_IDirectPlay8Address,
                                                (LPVOID*) &m_pDeviceAddress ) );	// mdvalley: get device address

    if ( m_fSessionCallback )
      m_pfmSite->OnSessionFound( this, &fmSession );
  }
}


//  <NKM> 09-Aug-2004
/*-------------------------------------------------------------------------
 * DPlayMsgHandler
 *-------------------------------------------------------------------------

  Receives messages from Dplay and forwards them to the right FedMessaging
*/

HRESULT WINAPI DPlayMsgHandler( PVOID pvUserContext,
                                DWORD dwMessageId,
                                PVOID pMsgBuffer )
{
  return ((FedMessaging*) pvUserContext)->MsgHandler( dwMessageId, pMsgBuffer );
}



FedMessaging::FedMessaging(IFedMessagingSite * pfmSite) :
  m_pDirectPlayClient( 0 ),
  m_pDirectPlayServer( 0 ),
  m_cMsgsOdometer(0),
  m_cBytesOdometer(0),
  m_pbFMNext(m_rgbbuffOutPacket),
  m_pfmSite(pfmSite),
  m_fConnected(false),
  m_precipDefault(NULL),
  m_fmGuaranteedDefault(FM_GUARANTEED),
  m_pcnxnServer(NULL),
  m_pgrpEveryone(NULL),
  m_pcnxnMe(NULL),
  m_fSecondaryOut(false),
  m_pbFMNextT(m_rgbbuffSecondaryOutPacket),
  m_guidInstance(GUID_NULL),
  m_guidApplication(GUID_NULL),
  m_timeMsgLast(Time::Now()),
  m_pHostAddress(NULL), // KGJV pigs - was uninitialized
  m_pDeviceAddress(NULL) // KGJV pigs - was uninitialized
{
  assert (pfmSite);
  InitializeCriticalSection( &m_csMsgList );
}


FedMessaging::~FedMessaging()
{
  Shutdown();
  DeleteCriticalSection( &m_csMsgList );
}


/*-------------------------------------------------------------------------
 * SetDefaultRecipient
 *-------------------------------------------------------------------------
  Purpose:
      Set default recipient, to be used if the out buffer overflows, or 
      NULL recipient is specified in SendMessages

      Important: The default recipient no longer remains the default if 
                 SendMessages is called with a NULL precip
 */
void FedMessaging::SetDefaultRecipient(CFMRecipient * precip, FMGuaranteed fmg)
{
  assert(0 == CbUsedSpaceInOutbox()); // should call this BEFORE you queue any messages
  m_precipDefault = precip;
  m_fmGuaranteedDefault = fmg;
}


/*-------------------------------------------------------------------------
 * GetDefaultRecipient
 *-------------------------------------------------------------------------
  Purpose:
      Get default recipient, and optionally other details about default recipient. 
      See SetDefaultRecipient

  Parameters:
      pfGuaranteedDefault:  output parameter to get guaranteed flag, or NULL if don't care
 */
CFMRecipient * FedMessaging::GetDefaultRecipient(FMGuaranteed * pfmg)
{
  if (pfmg)
    *pfmg = m_fmGuaranteedDefault;
  return m_precipDefault;
}


/*-------------------------------------------------------------------------
 * PFedMsgCreate
 *-------------------------------------------------------------------------
 * Purpose:
 *     Generate a FedMsg of specified type, and construct all variable-length parms
 * 
 * Parameters:
 *     pfm - Where to create the message (non-NULL). If NULL, memory is allocated for message
 *           Ignored if fQueueMsg==true
 * Returns:
 *     pointer to newly created message
 * 
 * Side Effects:
 *     Queues up the message if fQueueMsg is set, otherwise allocates new buffer for msg
 */
void * FedMessaging::PFedMsgCreate(bool fQueueMsg, BYTE * pbFMBuff, FEDMSGID fmid, CB cbfm, ...)
{
  va_list vl;
  BYTE * pBlobSrc, * pBlobDst; 
  BYTE * pbFM;

  assert (IsConnected());

StartOver: // if we overrun our buffer, need to start over after flushing it
  va_start(vl, cbfm); // make sure we have the last required parm

  if (fQueueMsg && (cbfm > CbFreeSpaceInOutbox()))
    SendToDefault(FM_FLUSH);
    
  //figure out where to compose this message
  pbFM =  fQueueMsg ? m_pbFMNext :
         (pbFMBuff  ? pbFMBuff : m_rgbbuffAlloc);

  ((FEDMESSAGE*)pbFM)->fmid = fmid;

  pBlobDst = pbFM + cbfm; // where we start putting the blobs
  IB * pib = (IB*)(pbFM + sizeof(FEDMESSAGE)); // relies on ib/cb pairs starting immediately after fm
  CB * pcb;
  CB cbBlob;

  while (FM_END_VAR_PARMS != (pBlobSrc = va_arg(vl, BYTE *)))
  {
    pcb = pib + 1;
    *pib = pBlobDst - pbFM;
    *pcb = 0;
    BYTE * pbLastNonSpace; // we trim the trailing whitespace too as a free added bonus
    cbBlob = va_arg(vl, CB);
    if (CB_ZTS == cbBlob)
    {
      if (pBlobSrc) // we have something for this parm (as opposed to an intentional NULL)
      {
        if (*pBlobSrc) // pointer to non-NULL string
        {
          int cbLen = lstrlen((char *)(pBlobSrc));
          pbLastNonSpace = pBlobSrc + cbLen - 1;
          while (cbLen > 0 && ' ' == *pbLastNonSpace)
          {
            pbLastNonSpace--;
            cbLen--;
          }

          if (fQueueMsg && (pBlobDst + cbLen > BuffOut() + GetBuffOutSize()))
          {
            // Need to send what was cued up, then move the current message (portion so far) to the head of the cue
            SendToDefault(FM_FLUSH);
            goto StartOver;
          }

          CopyMemory(pBlobDst, pBlobSrc, cbLen);
          *(pBlobDst + cbLen++) = 0;
          pBlobDst += cbLen;
          *pcb = cbLen;
        }
      }
    }
    else // length supplied
    {
      if (fQueueMsg && (pBlobDst + cbBlob > BuffOut() + GetBuffOutSize()))
      {
        // Need to send what was cued up, then move the current message (portion so far) to the head of the cue
        SendToDefault(FM_FLUSH);
        goto StartOver;
      }
      // if non-NULL pointer, then copy, otherwise, just reserve the space
      if (pBlobSrc)
        CopyMemory(pBlobDst, pBlobSrc, cbBlob);
      pBlobDst += cbBlob;
      *pcb = cbBlob;
    }
    
    pib += 2; // advance to next ib/cb pair
  } // while more blobs left
  va_end(vl);
  
  ((FEDMESSAGE *)pbFM)->cbmsg = pBlobDst - pbFM;
  if (fQueueMsg || !pbFMBuff) // if we're copying to pre-alloced pfm, then we're done
  {
    if (fQueueMsg)
    {
      m_pbFMNext = pBlobDst;
      // check for overrun buffer--relies on unsigned CB, otherwise check for free space >= 0
      assert(CbFreeSpaceInOutbox() <= GetBuffOutSize()); 
    }
    else // need to allocate space
    {
      // allocate a new blob to return the message in
      assert (((FEDMESSAGE *)pbFM)->cbmsg <= sizeof(m_rgbbuffAlloc));
      BYTE * pbMsg = (BYTE *) GlobalAllocPtr(GMEM_MOVEABLE, ((FEDMESSAGE *)pbFM)->cbmsg);
      CopyMemory(pbMsg, pbFM, ((FEDMESSAGE *)pbFM)->cbmsg);
      pbFM = pbMsg;
    }
  }
  assert(((FEDMESSAGE*)pbFM)->cbmsg > 0 && ((FEDMESSAGE*)pbFM)->fmid > 0);
  return pbFM;
}


/*-------------------------------------------------------------------------
 * PfmGetNext
 *-------------------------------------------------------------------------
 * Purpose:
 *    Get message after some provided message
 * 
 * Parameters:
 *    a message
 */
FEDMESSAGE * FedMessaging::PfmGetNext(FEDMESSAGE * pfm)
{
  BYTE * pfmNext = (BYTE*)pfm + pfm->cbmsg;

  //  <NKM> 30-Aug-2004
  // Changed the next assert from
  //  assert(pfmNext >= BuffIn()  && (BYTE*) pfm < (BuffIn()) + sizeof(m_rgbbuffInPacket));
  // as the 2nd half is now bogus as we don't have a fixed size input buffer no more :)
  assert( pfmNext >= BuffIn() );

  if (pfmNext - BuffIn() >= (int) PacketSize()) // no more messages in packet
  {
   //WLP = added connection test for disconnect from server interruption
   if (m_fConnected) // WLP - only if we have a connection
    assert((BYTE*)pfmNext == BuffIn() + PacketSize()); // should be exactly equal

	return NULL;
  }
  // Assert(!IsBadReadPointer(pfmNext, pfmNext->fm.cbfm))
  return (FEDMESSAGE *)pfmNext;
}


/*-------------------------------------------------------------------------
 * QueueExistingMsg
 *-------------------------------------------------------------------------
 * Purpose:
 *    Take a message that already exists, and queue it up for the outgoing packet
 * 
 * Parameters:
 *    pfm: the pre-existing, properly setup message
 */
void FedMessaging::QueueExistingMsg(const FEDMESSAGE * pfm)
{
  assert(m_fConnected);
  assert(pfm->cbmsg > 0);
  assert(pfm->fmid > 0);
  if (CbFreeSpaceInOutbox() < pfm->cbmsg) // overrun buffer--send it if we have a default recipient
  {
    assert(m_precipDefault);
    SendToDefault(FM_FLUSH);
  }
  CopyMemory(m_pbFMNext, pfm, pfm->cbmsg);
  m_pbFMNext += pfm->cbmsg;
  assert(CbFreeSpaceInOutbox() <= GetBuffOutSize()); 
}


/*-------------------------------------------------------------------------
 * GenericSend
 *-------------------------------------------------------------------------
 * Purpose:
 *    All messages go through this. Not used by FedMessaging users directly. Use SendMessages or ForwardMessage.
 */
HRESULT FedMessaging::GenericSend(CFMRecipient * precip, const void * pv, CB cb, FMGuaranteed fmg)
{
  DWORD dwFlags, dwTimeout;
  HRESULT hr;

  //
  // Give guaranteed messages a higher priority, and longer ttl
  //
  bool fGuaranteed = FM_GUARANTEED == fmg;
  static DWORD s_guarenteedFlags = DPNSEND_GUARANTEED | DPNSEND_PRIORITY_HIGH | DPNSEND_COALESCE;
  static DWORD s_normalFlags = DPNSEND_NOCOMPLETE | DPNSEND_NONSEQUENTIAL;

  dwFlags = fGuaranteed ? s_guarenteedFlags : s_normalFlags;

  dwTimeout = (fGuaranteed ? 0 : 500); // guaranteed messages will NEVER time out

// #define DUMPMSGS mmf commented this out, enable if we want dplay8 message debugging again
#ifdef DUMPMSGS
  GETSORC;
  debugf("*** (FM=%8x %s) Sending message to %8x, %8x flags = 0x%x, "
         "Timeout = %4d, length = %4d, thread = 0x%x\n", this, sOrC, precip->GetID(), this, dwFlags,
         dwTimeout, cb, GetCurrentThreadId());
#endif

#ifndef NO_MSG_CRC
  // add the crc to the end to minimize possible conflict of assumptions
  // We have to use our own buffer, so as to not overrun any input buffers ** :-( ** 
  char crcbuff[sizeof(m_rgbbuffOutPacket)];
  CopyMemory(crcbuff, pv, cb);
  int crc = MemoryCRC(crcbuff, cb);
  *(int*)(crcbuff + cb) = crc;
  pv = crcbuff;
  cb += sizeof(crc);
#endif

  DPN_BUFFER_DESC sendBufDesc;
  sendBufDesc.dwBufferSize = cb;
  sendBufDesc.pBufferData = (BYTE*) pv;
  DPNHANDLE handle;

  static CTempTimer tt("in dplay Send", 0.05f);
  tt.Start();
  if ( m_pDirectPlayClient )
  {
    hr = m_pDirectPlayClient->Send( &sendBufDesc, 1, dwTimeout, this, &handle, dwFlags );
  }
  else
  {
    hr = m_pDirectPlayServer->SendTo( precip->GetID(), &sendBufDesc, 1, dwTimeout, this, &handle, dwFlags );

  }

  tt.Stop("dpidTo=%8x, guaranteed=%d, hr=%x, 1st message (fmid)=%u", precip->GetID(), fGuaranteed, hr,
          ((FEDMESSAGE*)pv)->fmid);

  m_pfmSite->OnMessageSent(this, precip, pv, cb, fmg);

#ifdef DEBUG
  if ( ! ( hr == DPNSUCCESS_PENDING || hr == S_OK ) )
  {
    debugf("Return code of Send was 0x%x to player with dpid %8x.\n",
           hr, precip->GetID());
  }
#endif // DEBUG

  return hr;
}


/*-------------------------------------------------------------------------
 * SendToDefault
 *-------------------------------------------------------------------------
  Purpose:
      Send messages to default recipient

  Returns:
      Same as SendMessages

 */
int FedMessaging::SendToDefault(FMFlush fmf)
{
  return SendMessages(NULL, m_fmGuaranteedDefault, fmf);
}


/*-------------------------------------------------------------------------
 * SendMessages
 *-------------------------------------------------------------------------
  Purpose:
    Send queued-up messages
  
  Parameters:
    pvContext:  This is passed back to us when dp tell us it couldn't 
        deliver a message that we were tracking. FedSrv uses this as a pFSCon,
        and the client doesn't use it (yet).
    fflush: Whether you want to flush the queued message buffer
  
  Returns:
    bytes sent
  
  Side Effects:
    Important: The default recipient no longer remains the default if 
               SendMessages is called with a non-NULL precip
 */
int FedMessaging::SendMessages(CFMRecipient * precip, FMGuaranteed fmg, FMFlush fmf)
{
  assert(m_fConnected);
  // if they don't supply a recip, then they better have previously supplied one
  assert (precip || m_precipDefault);
  if (precip)
  {
    m_precipDefault = NULL;
  }
  else
  {
    precip = m_precipDefault;
    fmg    = m_fmGuaranteedDefault;
  }
  HRESULT hr;
  hr = S_OK;
  int cbToSend = CbUsedSpaceInOutbox();
  assert(cbToSend <= GetBuffOutSize());

  if (cbToSend > 0)
  {
    VerifyMessage(BuffOut(), cbToSend);
    hr = GenericSend(precip, BuffOut(), cbToSend, fmg);

    m_cMsgsOdometer++;
    m_cBytesOdometer += cbToSend;

    if (FM_FLUSH == fmf)
      m_pbFMNext = BuffOut();
  }
  return cbToSend;
}


/*-------------------------------------------------------------------------
 * CheckOdometer
 *-------------------------------------------------------------------------
 * Purpose:
 *    Tells you:
 *        How long it's been, 
 *        How many messages have been sent, and
 *        How many bytes have been sent
 *    since you've last called CheckOdometer. So obviously the first time you 
 *    call it you can ignore the return values
 * 
 * Parameters:    The return values
 * 
 * Side Effects:  Resets the odometer
 *
 * Returns:       Current time
 */
Time FedMessaging::CheckOdometer(float& flDTime, int& cMsgsOdometer, int& cBytesOdometer)
{
  Time timeNow    = Time::Now ();
  flDTime         = timeNow - m_timeOdometerStart;
  cMsgsOdometer   = m_cMsgsOdometer;
  cBytesOdometer  = m_cBytesOdometer;

  m_timeOdometerStart = timeNow;
  m_cMsgsOdometer     = 0;
  m_cBytesOdometer    = 0;
  return timeNow;
}


/*-------------------------------------------------------------------------
 * HandleSysMsg
 *-------------------------------------------------------------------------
 * Purpose:
 *    Handle system message
 * 
 * Parameters:
 *    System messages are those that dplay send us. They're handled here
 * 
 * Side Effects:
 *    Various
 */
HRESULT FedMessaging::OnSysMessage( const DPlayMsg& msg )
{
  // The body of each case is there so you can set a breakpoint and examine
  // the contents of the message received.

  static CTempTimer timerSysMsg("spent in OnSysMessage()", .01f);
  timerSysMsg.Start();
  Time timeNow = Time::Now();

  m_pfmSite->OnSysMessage(this); // just for tracking
    
  GETSORC;
  // debugf("** (FM=%8x %s) OnSysMessage, type %8x %s \n", this, sOrC, msg.dwType, getMessageString(msg.dwType) );

  if( msg.dwType == DPN_MSGID_CREATE_PLAYER )
  {
    static CTempTimer tt( "handling DPN_MSGID_CREATE_PLAYER", .01f);
    tt.Start();
    DPNMSG_CREATE_PLAYER* lp = (DPNMSG_CREATE_PLAYER*) msg.pData;

    if ( lp->pvPlayerContext == (void*) 1 )
    {
      //  <NKM> 18-Aug-2004
      // This is the server player
      m_pcnxnMe = new CFMConnection( this, "<Server connection>", lp->dpnidPlayer );
      tt.Stop();
      return DPN_OK;
    }

    // Only get this if I'm a server
    assert( m_pDirectPlayServer != 0 );

    //  <NKM> 23-Aug-2004
    // Do the DPLAY bizare API thing...
    DPN_PLAYER_INFO* pPlayerInfo = 0;
    DWORD dwSize = 0;

    HRESULT hr = m_pDirectPlayServer->GetClientInfo( lp->dpnidPlayer, pPlayerInfo,
                                          &dwSize, 0);

    //IMAGO - REVIEW 7/5/09 sanitize pPlayerInfo
	/*
		HRESULT: 0x80158420 (2148893728)
		Name: Unknown
		Description: n/a
		Severity code: Failed
		Facility Code: FACILITY_DPLAY (21)
		Error Code: 0x8420 (33824)
	*/
	assert( hr == DPNERR_BUFFERTOOSMALL );

    pPlayerInfo = (DPN_PLAYER_INFO*) new BYTE[dwSize];
    ZeroMemory( pPlayerInfo, dwSize );
    pPlayerInfo->dwSize = sizeof( DPN_PLAYER_INFO );

    hr = m_pDirectPlayServer->GetClientInfo( lp->dpnidPlayer, pPlayerInfo,
                                             &dwSize, 0 );

    if ( hr != DPN_OK )
    {
        debugf( "m_pDirectPlayServer->GetClientInfo failed\n");
        return hr;
    }

    char* name = wChar2ConstChar( pPlayerInfo->pwszName );

    // debugf("(FM=%8x %s) Create Player for %s (%u)\n", this, sOrC, name, lp->dpnidPlayer );

    CFMConnection * pcnxn = CreateConnection( name, lp->dpnidPlayer );
   
	//removed below becasue connections can reset here 
	//then GetIPAddress breaks the lobby -Imago

	// char szRemoteAddress[64];
   // GetIPAddress( *pcnxn, szRemoteAddress );
   // debugf(" ip=%s\n", szRemoteAddress );

    delete[] name;
    delete[] pPlayerInfo;
    tt.Stop();
  }
  else if( msg.dwType == DPN_MSGID_CREATE_GROUP )
  {
    // Moved to asynchronus handle
  }
  else if ( msg.dwType == DPN_MSGID_DESTROY_PLAYER )
  {
    static CTempTimer tt("handling DPN_MSGID_DESTROY_PLAYER", .01f);
    tt.Start();

    DPNMSG_DESTROY_PLAYER* lp = (DPNMSG_DESTROY_PLAYER *) msg.pData;

    CFMConnection * pcnxn = GetConnectionFromDpid( lp->dpnidPlayer );

    //debugf("(FM=%8x %s) DESTROY Player for %s (%u)\n", this, sOrC, pcnxn ? pcnxn->GetName() : "<unknown>" , lp->dpnidPlayer );

    // if your player dies, or the server dies, you're SOL--not much point in
    // waiting until dplay gets around to telling the session is dead,
    // since sometimes they don't anyway.
    if ( pcnxn && ( m_pcnxnMe == pcnxn || m_pcnxnServer == pcnxn) )
    {
      m_pfmSite->OnSessionLost(this);
      Shutdown();
    }
    else if (pcnxn)
    {
      DeleteConnection(*pcnxn);
    }

    tt.Stop();
  }
  else if ( msg.dwType == DPN_MSGID_SEND_COMPLETE )
  {
    static CTempTimer tt("handling DPN_MSGID_SEND_COMPLETE", .01f);
    tt.Start();

    DPNMSG_SEND_COMPLETE* lp = (DPNMSG_SEND_COMPLETE *) msg.pData;

    CFMRecipient * prcp = NULL;
    DPID to = 0;

    //  <NKM> 22-Aug-2004
    // If this is an Async op we have a handle to, then remove it
    HandleIDMap::iterator it = m_handleMap.find( lp->hAsyncOp );
    if ( it != m_handleMap.end() )
    {
      to = it->second;
      m_handleMap.erase( it );
      prcp = GetGroupFromDpid( to );
    }

    if ( !prcp )
    {
      CFMConnection * pcnxn = GetConnectionFromDpid( to );
      if ( pcnxn )
      {
        prcp = pcnxn;
        if ( DPN_OK == lp->hResultCode )
          pcnxn->SetLastComplete( timeNow.clock() );
      }
    }

    if ( DPN_OK != lp->hResultCode )
    {
      // debugf("(FM=%8x %s) SEND FAILED: Message was not delivered to %s(%u), hr=0x%x, SendTime=%d.\n",
      //       this, sOrC, prcp ? prcp->GetName() : "<not found>", to, lp->hResultCode, lp->dwSendTime );

      if ( prcp )
        m_pfmSite->OnMessageNAK( this, lp->dwSendTime, prcp );
    }

    tt.Stop();
  }
  else if ( msg.dwType == DPN_MSGID_ADD_PLAYER_TO_GROUP )
  {
    DPNMSG_ADD_PLAYER_TO_GROUP* lp = (DPNMSG_ADD_PLAYER_TO_GROUP*) msg.pData;

    CFMGroup * pgrp = GetGroupFromDpid( lp->dpnidGroup );
    CFMConnection * pcnxn = GetConnectionFromDpid(lp->dpnidPlayer);
    if ( pgrp )
      pgrp->PlayerAdded( pcnxn );
    //debugf("(FM=%8x %s) ADD Player to Group: player %s(%u) to group %s(%u)\n",
    //       this, sOrC,
    //       pcnxn ? pcnxn->GetName() : "<gone>", lp->dpnidPlayer,
    //       pgrp ? pgrp->GetName() : "<gone>", lp->dpnidGroup);
  }
  else if ( msg.dwType == DPN_MSGID_REMOVE_PLAYER_FROM_GROUP )
  {
    DPNMSG_REMOVE_PLAYER_FROM_GROUP* lp = (DPNMSG_REMOVE_PLAYER_FROM_GROUP*) msg.pData;

    CFMGroup * pgrp = GetGroupFromDpid( lp->dpnidGroup );
    CFMConnection * pcnxn = GetConnectionFromDpid(lp->dpnidPlayer);
    if ( pgrp )
      pgrp->PlayerDeleted( pcnxn );
    //debugf("(FM=%8x %s) REMOVE Player from Group: player %s(%u) to group %s(%u)\n",
    //       this, sOrC,
    //       pcnxn ? pcnxn->GetName() : "<gone>", lp->dpnidPlayer,
    //       pgrp ? pgrp->GetName() : "<gone>", lp->dpnidGroup);
  }
  else if( msg.dwType == DPN_MSGID_TERMINATE_SESSION )
  {
    m_pfmSite->OnSessionLost(this);
  }

  timerSysMsg.Stop();

  return(S_OK);
}

template< class T > T* allocAndCopyStruct( const T& data )
{
  return new T(data); //Fix memory leak -Imago 8/2/09
}

//  <NKM> 09-Aug-2004
// DPlay handler - copy and store messages in list for later processing
HRESULT FedMessaging::MsgHandler( DWORD dwMessageId, PVOID pMsgBuffer )
{
  // For most messages we make a copy and store it.
  DPlayMsg msg;
  msg.dwType = dwMessageId;
  msg.pData = 0;

  // Default to OK
  HRESULT result = DPN_OK;

  const char* sOrC = m_pDirectPlayClient == 0 ? "S" : "C";

  // debugf("** (FM=%8x %s) RECV DPlay message, type %8x %s \n", this, sOrC, dwMessageId, getMessageString( dwMessageId ) );

  switch( dwMessageId )
  {
  case DPN_MSGID_CREATE_PLAYER:
    msg.pData = allocAndCopyStruct( *((DPNMSG_CREATE_PLAYER*) pMsgBuffer) );
    break;
  case DPN_MSGID_CREATE_GROUP:
    msg.pData = allocAndCopyStruct( *((DPNMSG_CREATE_GROUP*) pMsgBuffer) );

    // Moved this here.  Groups are create in a synchronus op so this is safe,
    // And I need to set the dpid here
    {
      DPNMSG_CREATE_GROUP* lp = (DPNMSG_CREATE_GROUP*) msg.pData;
      CFMGroup* grp = (CFMGroup*) lp->pvGroupContext;
      grp->m_dpid = lp->dpnidGroup;
    }
    break;
  case DPN_MSGID_DESTROY_PLAYER:
    msg.pData = allocAndCopyStruct( *((DPNMSG_DESTROY_PLAYER*) pMsgBuffer) );
    break;
//  case DPN_MSGID_DESTROY_GROUP:
//    msg.pData = allocAndCopyStruct( *((DPNMSG_DESTROY_GROUP*) pMsgBuffer) );
//    break;
  case DPN_MSGID_SEND_COMPLETE:
    msg.pData = allocAndCopyStruct( *((DPNMSG_SEND_COMPLETE*) pMsgBuffer) );
    break;
  case DPN_MSGID_ADD_PLAYER_TO_GROUP:
    msg.pData = allocAndCopyStruct( *((DPNMSG_ADD_PLAYER_TO_GROUP*) pMsgBuffer) );
    break;
  case DPN_MSGID_REMOVE_PLAYER_FROM_GROUP:
    msg.pData = allocAndCopyStruct( *((DPNMSG_REMOVE_PLAYER_FROM_GROUP*) pMsgBuffer) );
    break;
  case DPN_MSGID_TERMINATE_SESSION:
    msg.pData = allocAndCopyStruct( *((DPNMSG_TERMINATE_SESSION*) pMsgBuffer) );
    break;

  case DPN_MSGID_RECEIVE:
    //  <NKM> 09-Aug-2004
    // This is different coz we have to have to return SUCESS_PENDING so that DPlay
    // doesn't free the message buffer;
    msg.pData = allocAndCopyStruct( *((DPNMSG_RECEIVE*) pMsgBuffer ) );
    result = DPNSUCCESS_PENDING;
    break;

  case DPN_MSGID_ENUM_HOSTS_RESPONSE:
    //  <NKM> 10-Aug-2004
    //  Also special as we deal with this in-line
    EnumHostsCallback( *((DPNMSG_ENUM_HOSTS_RESPONSE*) pMsgBuffer) );
    return DPN_OK;

  default:
    //  <NKM> 09-Aug-2004
    // Don't care about others for now... log and return
    // debugf("***** (FM=%8x %s) Unhandled DPlay message, type %8x *****\n", this, sOrC, dwMessageId);
    return DPN_OK;
  }

  // Now enter critical section and save message to buffer
  EnterCriticalSection( &m_csMsgList );
  m_msgList.push_back( msg );
  LeaveCriticalSection( &m_csMsgList );

  return result;
}

/*-------------------------------------------------------------------------
 * ReceiveMessages
 *-------------------------------------------------------------------------
 * Purpose:
 *    Handles all inbound messages and dispatches them appropriately
 */
HRESULT FedMessaging::ReceiveMessages()
{
  // loop to read all messages in queue
  while ( IsConnected() )
  {
    DPlayMsg msg;

    // Now enter critical section and pop message from buffer
    // if we have one!
    int msgCount;
    EnterCriticalSection( &m_csMsgList );
    msgCount = m_msgList.size();
    if ( msgCount > 0 )
    {
      msg = m_msgList.front();
      m_msgList.pop_front();
    }
    LeaveCriticalSection( &m_csMsgList );

    GETSORC;
    // If no message break out
    if ( msgCount != 0 )
    {
      // If it's not a recieve message then we pass it off
      if ( msg.dwType != DPN_MSGID_RECEIVE )
      {
        OnSysMessage( msg );
      }
      else
      {
        DPNMSG_RECEIVE* p_dpMsg = (DPNMSG_RECEIVE*) msg.pData;

        CFMConnection * pcnxnFrom = GetConnectionFromDpid( p_dpMsg->dpnidSender );

        if ( pcnxnFrom != 0 ) // if we can't resolve the player, then it's into the bit bucket with this message
        {
          m_rgbbuffInPacket = p_dpMsg->pReceiveData;
          FEDMESSAGE* pfm = (FEDMESSAGE *) p_dpMsg->pReceiveData;
          m_dwcbPacket = p_dpMsg->dwReceiveDataSize;

          int cMsgs = 0;
#ifndef NO_MSG_CRC
          int crc = MemoryCRC(m_rgbbuffInPacket, PacketSize());
          if (crc == *(int*)(m_rgbbuffInPacket + PacketSize()))
          {
#endif            
            static CTempTimer tt("spent looping through message queue", .1f);
            tt.Start();
            g_fConnectionDeleted = false;

            CB cb = pfm->cbmsg;
            FEDMSGID id = pfm->fmid;

            while (pfm && !g_fConnectionDeleted)
            {
              if(pfm->fmid > 0 && pfm->cbmsg >= sizeof(FEDMESSAGE) && pfm->cbmsg <= PacketSize())
              {
               // debugf( "(FM=%8x %s) Msg from %s(%8x) cbmsg=%d, fmid=%d, total packet size=%d\n",
               //         this, sOrC,
               //         pcnxnFrom ? pcnxnFrom->GetName() : "<unknown>", p_dpMsg->dpnidSender,
               //        cb, id, m_dwcbPacket );

                m_pfmSite->OnAppMessage(this, *pcnxnFrom, pfm);
                pfm = PfmGetNext(pfm);
                cMsgs++;
              }
              else
              {
                // ACK! Hacker?
                debugf("HACKER?? message from %s(%8x).\ncbmsg=%d, fmid=%d, total packet size=%d\n",
                       pcnxnFrom ? pcnxnFrom->GetName() : "<unknown>", p_dpMsg->dpnidSender,
                       cb, id, m_dwcbPacket );

#ifndef NO_MSG_CRC
                // we'll just call it a bad crc, even though it's not quite the same thing
                m_pfmSite->OnBadCRC(this, *pcnxnFrom, m_rgbbuffInPacket, m_dwcbPacket);
#endif
              }
            }
#ifndef NO_MSG_CRC          
          }
          else
          {
            // ACK! Failed CRC. WTF??? ...<sigh> but it happens
            m_pfmSite->OnBadCRC(this, *pcnxnFrom, m_rgbbuffInPacket, m_dwcbPacket);
          }
#endif          
        }
        else
        {
          //debugf("(FM=%8x %s) Warning: Ignoring message from dpid %u who doesn't have an associated CFMConnection\n",
          //       this, sOrC, p_dpMsg->dpnidSender );
        }

        if ( m_pDirectPlayClient != 0 )
        {
          m_pDirectPlayClient->ReturnBuffer( p_dpMsg->hBufferHandle, 0 );
        }
        else
        {
        //  WLP - added test for valid obj - DPLAY* - exit game error
        //  m_pDirectPlayServer->ReturnBuffer( p_dpMsg->hBufferHandle, 0 );
         if (m_pDirectPlayServer)
			 m_pDirectPlayServer->ReturnBuffer( p_dpMsg->hBufferHandle, 0 );
        }
        m_timeMsgLast = Time::Now();
      }

      delete msg.pData;
    }
    else
    {
      break;  // Carry on :)
    }
  }

  return DPN_OK;
}


/*-------------------------------------------------------------------------
 * ConnectToDPAddress
 *-------------------------------------------------------------------------
 * Purpose:
 *    Connect to a machine, given a dplay address
 */
HRESULT FedMessaging::ConnectToDPAddress(LPVOID pvAddress)
{
  assert(0);
  return DPN_OK;
//   HRESULT hr = m_pDirectPlay->InitializeConnection(pvAddress, 0);
//   if (FAILED(hr))
//   {
//     if (DPERR_ALREADYINITIALIZED == hr)
//       hr = S_OK;
//     else
//       m_pfmSite->OnMessageBox(this, "Failed to connect to server. Please check network connection.",
//                             "Allegiance", MB_OK);
//   }
//   return hr;
}


/*-------------------------------------------------------------------------
 * Connect
 *-------------------------------------------------------------------------
 * Purpose:
 *    Connect to a host machine. Could be self.
 * 
 * Parameters:
 *    string address of host
 */
HRESULT FedMessaging::Connect(const char * szAddress)
{
    assert(0);
  return DPN_OK;
//   assert(m_pDirectPlay);
//   assert(!m_fConnected);
//   void * pvAddress = NULL;
//   DWORD dwAddressSize;
//   HRESULT hr = E_FAIL;
//   // Find out how big the address buffer needs to be--intentional fail 1st time
//   hr = m_pDirectPlayLobby->CreateAddress(DPSPGUID_TCPIP, DPAID_INet, szAddress,
//           lstrlen(szAddress) + 1, pvAddress, &dwAddressSize);
//   assert(DPERR_BUFFERTOOSMALL == hr);
    
//   pvAddress = new char[dwAddressSize];
//   ZSucceeded(m_pDirectPlayLobby->CreateAddress(DPSPGUID_TCPIP, DPAID_INet, szAddress,
//           lstrlen(szAddress) + 1, pvAddress, &dwAddressSize));
//   hr = ConnectToDPAddress(pvAddress);
//   delete [] pvAddress;
  
//   return hr;
}


/*-------------------------------------------------------------------------
 * KillSvr
 *-------------------------------------------------------------------------
   Purpose:
      Kill dplaysvr

   Returns:
      Whether it was sucessfully killed (or if it wasn't running in the first place)
   
   Side Effects:
      Any running dplay server apps will be killed. This should never be 
      true if any other dplay server app is running
 */
bool FedMessaging::KillSvr()
{
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si); 

  Shutdown(); // can't be doing this with an open session
  
  BOOL fCreated = CreateProcess(NULL, "KillSvr.exe", NULL, NULL, FALSE, 
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

  if (fCreated)
  {
    // Now we need to make sure it finishes before we go start using dplaysvr again
    // WARNING: We could hang if killsvr never comes back. Maybe just abort if it doesn't come back in a few seconds?
    DWORD exitcode = STILL_ACTIVE;
    
    int i = 0;
    const int cRetries = 50; // let's give it 50 retries
    while (STILL_ACTIVE == exitcode && i > cRetries) 
    {
      GetExitCodeProcess(pi.hProcess, &exitcode);
      Sleep(100);
      i++;
    }
    if (i > cRetries)
      fCreated = false;
    else
      debugf("Slept (100ms) %d times  waiting for killsvr\n", i);
    Sleep(1000); // wait another second to make sure it's really gone
  }
  else
  {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL 
                  );
    m_pfmSite->OnMessageBox(this, (LPCTSTR)lpMsgBuf, "Couldn't run killsvr.exe.", MB_OK | MB_ICONINFORMATION);
    LocalFree( lpMsgBuf );
  }
 
  return !!fCreated;
}


/*-------------------------------------------------------------------------
 * InitDPlay
 *-------------------------------------------------------------------------
 * Purpose:
 *    Must be called before you can do anything with dplay
 * 
 * Side Effects:
 *    Creates a dplay object m_pDirectPlay, that must later be closed, 
 *        released, and set to NULL.
 */
HRESULT FedMessaging::InitDPlayClient()
{
  HRESULT hr = E_FAIL;
  assert( !m_fConnected);
  assert( !m_pDirectPlayClient );
  assert( !m_pDirectPlayServer );

  // Create an IDirectPlay interface
  m_pfmSite->OnPreCreate(this);


  if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Client, NULL,
                                     CLSCTX_INPROC_SERVER, IID_IDirectPlay8Client,
                                     (LPVOID*) &m_pDirectPlayClient ) ) )
  {
    m_pfmSite->OnMessageBox(this, "Failed to Create DirectPlayClient in DirectX 8/9", "Allegiance", MB_OK);
    return hr;
  }

  //  <NKM> 08-Aug-2004
  // AT some point we can set flags to DPNINITIALIZE_DISABLEPARAMVAL - but for BETA I
  // I sugegst we leave it at this
  //  const DWORD dwInitFlags = DPNINITIALIZE_DISABLEPARAMVAL;
  const DWORD dwInitFlags = 0;

  if( FAILED( hr = m_pDirectPlayClient->Initialize( this, DPlayMsgHandler, dwInitFlags ) ) )
  {
    m_pfmSite->OnMessageBox(this, "Failed to initialize DirectX 8/9", "Allegiance", MB_OK);
    return hr;
  }

  //  <NKM> 09-Aug-2004
  // No longer check version as that is somewhat implicit
  // in the interfaces - and as Dplay 8 is the last version of Dplay.... :)

  return S_OK;
}


/*-------------------------------------------------------------------------
 * InitDPlayServer
 *-------------------------------------------------------------------------
 * Purpose:
 *    Must be called before you can Host a session
 *
 */
HRESULT FedMessaging::InitDPlayServer()
{
  HRESULT hr = E_FAIL;
  assert( !m_fConnected);
  assert( !m_pDirectPlayClient );
  assert( !m_pDirectPlayServer );

  // Create an IDirectPlay interface
  m_pfmSite->OnPreCreate(this);

  ZDebugOutput("CoCreateInstance( CLSID_DirectPlay8Server ... )\n");
  if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Server, NULL,
                                     CLSCTX_INPROC_SERVER, IID_IDirectPlay8Server,
                                     (LPVOID*) &m_pDirectPlayServer ) ) )
  {
	ZDebugOutput("Failed to Create DirectPlaySever in DirectX 8/9 hr="+ZString(hr)+"\n");
    m_pfmSite->OnMessageBox(this, "Failed to Create DirectPlaySever in DirectX 8/9", "Allegiance", MB_OK);
    return hr;
  }

  //  <NKM> 08-Aug-2004
  // AT some point we can set flags to DPNINITIALIZE_DISABLEPARAMVAL - but for BETA I
  // I sugegst we leave it at this
  //  const DWORD dwInitFlags = DPNINITIALIZE_DISABLEPARAMVAL;
  const DWORD dwInitFlags = 0;
  ZDebugOutput("m_pDirectPlayServer->Initialize( ... )\n");
  if( FAILED( hr = m_pDirectPlayServer->Initialize( this, DPlayMsgHandler, dwInitFlags ) ) )
  {
	ZDebugOutput("Failed to initialize DirectX 8/9 hr="+ZString(hr)+"\n");
    m_pfmSite->OnMessageBox(this, "Failed to initialize DirectX 8/9", "Allegiance", MB_OK);
    return hr;
  }

  //  <NKM> 09-Aug-2004
  // No longer check version as that is somewhat implicit
  // in the interfaces - and as Dplay 8 is the last version of Dplay.... :)

  return S_OK;
}


/*-------------------------------------------------------------------------
 * HostSession
 *-------------------------------------------------------------------------
 * Purpose:
 *    Start a session and be the host (server)
 * 
 * Parameters:
 *    guidApplication: Clients will only find this session if they ask for the same application
 *    fKeepAlive:      Should dplay detect timeouts and kill players on dead connections?
 *    hEventServer:    Event to be signaled when a message comes to the server
 *    fKillSvr:        Whether to kill the dplaysvr process. 
 * 
 * Returns:
 *    
 * 
 * Side Effects:
 *    
 */
HRESULT FedMessaging::HostSession( GUID guidApplication, bool fKeepAlive, HANDLE hEventServer, bool fProtocol, DWORD dwPort )	// mdvalley: (optional) dwPort forces enumeration to that port
{
  HRESULT hr = E_FAIL;

  assert(!m_fConnected);
  if (false) // TODO: How do we know when to make sure dplaysvr isn't hosed?
  {
    if (!KillSvr())
    {
      m_pfmSite->OnMessageBox(this, "Failed to successfully run killsvr.exe (or it failed to complete), which kills dplaysvr.exe. If we can't veryify that dplaysvr starts out clean, we're in for trouble, so I'm aborting.",
                            "Allegiance", MB_OK);
      return E_FAIL;
    }
  }
  
  if( m_pDirectPlayServer == 0 )
  {
    hr = InitDPlayServer();
	ZDebugOutput(PCC(ZString("InitDPlayServer() returned ")+ZString(hr)+ZString("\n")));
    if ( FAILED(hr) )
      return hr;
  }
  

  DPN_APPLICATION_DESC   dpnAppDesc;
  IDirectPlay8Address*   pDP8AddressLocal = 0;

  // Create the local device address object
  ZDebugOutput("CoCreateInstance( CLSID_DirectPlay8Address ... )\n");
  if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
                                     CLSCTX_ALL, IID_IDirectPlay8Address,
                                     (LPVOID*) &pDP8AddressLocal ) ) )
  {
	ZDebugOutput("CoCreateInstance( CLSID_DirectPlay8Address ... ) returned "+ZString(hr)+"\n");
    m_pfmSite->OnMessageBox( this, "Failed to create DPlay server local address", "Allegiance", MB_OK );
    return hr;
  }

  SafeReleaser<IDirectPlay8Address> r1( pDP8AddressLocal );

  // Set IP service provider
   ZDebugOutput("pDP8AddressLocal->SetSP( &CLSID_DP8SP_TCPIP )\n");
  if( FAILED( hr = pDP8AddressLocal->SetSP( &CLSID_DP8SP_TCPIP ) ) )
  {
	ZDebugOutput(PCC(ZString("pDP8AddressLocal->SetSP( &CLSID_DP8SP_TCPIP ) returned ")+ZString(hr)+ZString("\n")));
    m_pfmSite->OnMessageBox( this, "Failed to create DPlay server local SP", "Allegiance", MB_OK );
    return hr;
  }

  if( dwPort ) {
	  ZDebugOutput("pDP8AddressLocal->AddComponent(DPNA_KEY_PORT ... )\n");
	  if(FAILED(hr = pDP8AddressLocal->AddComponent(DPNA_KEY_PORT, &dwPort, sizeof(DWORD), DPNA_DATATYPE_DWORD)))
		{
			ZDebugOutput(PCC(ZString("Failed to set DPlay server port - returned ")+ZString(hr)+ZString("\n")));
		    m_pfmSite->OnMessageBox( this, "Failed to set DPlay server port", "Allegiance", MB_OK );
		    return hr;
		}
  }

  ZeroMemory( &dpnAppDesc, sizeof( DPN_APPLICATION_DESC ) );
  dpnAppDesc.dwSize = sizeof( DPN_APPLICATION_DESC );
  dpnAppDesc.dwFlags = DPNSESSION_CLIENT_SERVER;
  dpnAppDesc.guidApplication = guidApplication;
  dpnAppDesc.pwszSessionName = L"FedSrv";
  dpnAppDesc.dwMaxPlayers = 0;

  // Set host player context to non-NULL so we can determine which player indication is
  // the host's.
  hr = m_pDirectPlayServer->Host( &dpnAppDesc, &pDP8AddressLocal, 1, NULL, NULL, (void *) 1, 0  );
  ZDebugOutput(PCC(ZString("m_pDirectPlayServer->Host( &dpnAppDesc ... ) - returned ")+ZString(hr)+ZString("\n")));
  m_guidApplication = guidApplication;

  m_fConnected = true;

  // then create the Everyone group
  ZVerify(m_pgrpEveryone = CreateGroup("Everyone"));

  ResetOutBuffer();

  if ( FAILED(hr) )
    Shutdown();
  
  return hr;
}


/*-------------------------------------------------------------------------
 * Shutdown
 *-------------------------------------------------------------------------
 * Purpose:
 *    Shutdown an open session. It is assumed that logoff, etc. has already been done
 */
void FedMessaging::Shutdown()
{
  static CTempTimer tt("In FedMessaging::Shutdown", 0.1f);
  tt.Start();
  if ( m_pDirectPlayClient )
  {
    m_pDirectPlayClient->Close(0);
    m_pDirectPlayClient->Release();
    m_pDirectPlayClient = NULL;
  }
  if ( m_pDirectPlayServer )
  {
    m_pDirectPlayServer->Close(0);
    m_pDirectPlayServer->Release();
    m_pDirectPlayServer = NULL;
  }

  if( m_pcnxnMe )
  {
    delete m_pcnxnMe;
    m_pcnxnMe = NULL;
  }

  if (m_pcnxnServer)
  {
    delete m_pcnxnServer;
    m_pcnxnServer = NULL;
  }

  ResetOutBuffer();

  // Flush the lists
  m_listCnxns.SetEmpty();
  m_cnxnsMap.clear();
  m_groupMap.clear();

  // Flush message queue
  m_msgList.clear();

  m_guidInstance = GUID_NULL;

  m_fConnected  = false;

  tt.Stop();
}


/*-------------------------------------------------------------------------
 * JoinSession
 *-------------------------------------------------------------------------
 * Purpose:
 *    Become a client in a session
 * 
 * Parameters:
 *    what server, what application, and what instance to join (or NULL for default)
 *    OUT: connection to the sevrer that all messages should be sent to
 */
HRESULT FedMessaging::JoinSession(GUID guidApplication, const char * szServer, const char * szCharName, DWORD dwPort)	// mdvalley: (optional) dwPort forces enumeration on given port
{
  HRESULT hr = E_FAIL;
  Time timeStart = Time::Now();
  m_fSessionCallback = false; // we're going to join the session right here, so we don't want any site methods being called

  hr = InitDPlayClient();
  if ( FAILED(hr) )
    return hr;

  hr = EnumHostsInternal(guidApplication, szServer, dwPort);
  if ( FAILED(hr) )
    return hr;

  // Mdvalley: since Enum is now asyncronous, we have to wait until a proper reply comes back.
  // Sure, it'll freeze for 15 secs if there's no connection, but it did that under dplay4, too.

  int i = 0;
#ifdef _DEBUG 
  while(IsEqualGUID(GUID_NULL, m_guidInstance) && i < 50)   // 5 seconds for debug Imago 7/10 
#else 
  while(IsEqualGUID(GUID_NULL, m_guidInstance) && i < 300)	// 30 second timeout //was 15 seconds 8/1/09 Imago
#endif
  {
     Sleep(100);	// check every 100 ms
     i++;
  }

  if (IsEqualGUID(GUID_NULL, m_guidInstance))
    hr = E_FAIL; // something else?
  else
//    hr = JoinSessionInstance( guidApplication, m_guidInstance, m_pHostAddress, szCharName);
    hr = JoinSessionInstance( guidApplication, m_guidInstance, m_pHostAddress, m_pDeviceAddress, szCharName);

  ResetOutBuffer();

  if (FAILED(hr))
  {
	m_pDirectPlayClient->CancelAsyncOperation(NULL, DPNCANCEL_ENUM);		// Cancel all enumeration requests
    Shutdown();
  }
  
  return hr;
}


/*-------------------------------------------------------------------------
 * FedMessaging.JoinSessionInstance
 *-------------------------------------------------------------------------
  Purpose:
      Join a session that has already been enum'd

  Parameters:
      session instance and name of player to create 

  Side Effects:
      joins the session, creates the named player
 */
HRESULT FedMessaging::JoinSessionInstance( GUID guidApplication, GUID guidInstance, IDirectPlay8Address* addr, IDirectPlay8Address* device, const char * szName)	// mdvalley: added device argument
{
  // WLP 2005 - added init client if needed for DPLAY8
  //
  if ( !m_pDirectPlayClient ) InitDPlayClient(); // WLP - make client when needed
  // WLP - end init code

  assert (m_pDirectPlayClient );

  if ( !m_pDirectPlayClient )
    return E_FAIL;

  DPN_APPLICATION_DESC dpnAppDesc;
  ZeroMemory( &dpnAppDesc, sizeof( DPN_APPLICATION_DESC ) );
  dpnAppDesc.dwSize          = sizeof( DPN_APPLICATION_DESC );
  dpnAppDesc.guidApplication = guidApplication;
  dpnAppDesc.guidInstance    = guidInstance;

  addr->AddRef();
  device->AddRef();
  SafeReleaser<IDirectPlay8Address> r1( addr );

  DPN_PLAYER_INFO playerInfo;
  ZeroMemory( &playerInfo, sizeof( DPN_PLAYER_INFO ) );
  playerInfo.dwSize = sizeof( DPN_PLAYER_INFO );
  playerInfo.dwInfoFlags = DPNINFO_NAME;
  playerInfo.pwszName = constChar2Wchar( szName );
      
  HRESULT hr = m_pDirectPlayClient->SetClientInfo( &playerInfo, 0, 0 /*0 for sync op*/, DPNSETCLIENTINFO_SYNC );

  if (FAILED(hr))
    return hr;

  debugf("DirectPlay: Connecting synchronously");
  hr = m_pDirectPlayClient->Connect( &dpnAppDesc,        // Application description
                                     addr,               // Session host address
                                     device,             // Address of device used to connect to the host (mdvalley: formerly 0)
                                     NULL, NULL,         // Security descriptions & credientials (MBZ in DPlay8)
                                     NULL, 0,            // User data & its size
                                     NULL,               // Asynchronous connection context (returned with DPNMSG_CONNECT_COMPLETE in async handshaking)
                                     NULL,               // Asynchronous connection handle (used to cancel connection process)
                                     DPNOP_SYNC );       // Connect synchronously  //Fix memory leak -Imago 8/2/09

  delete[] playerInfo.pwszName;
  if (FAILED(hr)) {
      debugf("DirectPlay: Connect failed");
      return hr;
  }
  debugf("DirectPlay: Connect success");

  m_guidInstance = guidInstance;
  m_fConnected = true;

  //  <NKM> 19-Aug-2004
  // use 0 dor DPID for now
  m_pcnxnMe = new CFMConnection( this, szName, 0);

  // Set the output connection that the client will send msgs to
  m_pcnxnServer = new CFMConnection(this, "Server", 1);
        
  return hr;
}

// WLP 2005 added this 2 parameter call to update to DPLAY8
        
HRESULT FedMessaging::JoinSessionInstance(GUID guidInstance, const char * szName)
{
//return JoinSessionInstance( GUID_NULL, m_guidInstance, m_pHostAddress, szName);
return JoinSessionInstance( GUID_NULL, m_guidInstance, m_pHostAddress, m_pDeviceAddress, szName);
}
// WLP - End

CFMConnection * FedMessaging::GetConnectionFromDpid(DPID dpid)
{
  CFMConnection * pcnxn = NULL;

  if ( m_pcnxnMe && ( m_pcnxnMe->GetDPID() == dpid ) )
  {
    return m_pcnxnMe;
  }

  if ( m_pcnxnServer && ( m_pcnxnServer->GetDPID() == dpid) )
  {
    return m_pcnxnServer;
  }

  ConnectionMap::iterator it = m_cnxnsMap.find( dpid );
  if ( it != m_cnxnsMap.end() )
    return it->second;

  return 0;
}


CFMGroup * FedMessaging::GetGroupFromDpid(DPID dpid)
{
  GroupMap::iterator it = m_groupMap.find( dpid );
  if ( it != m_groupMap.end() )
    return it->second;

  return 0;
}


HRESULT FedMessaging::GetIPAddress(CFMConnection & cnxn, char szRemoteAddress[64])
{
  //assert( m_pDirectPlayServer != 0 );

  IDirectPlay8Address* pAddress;

  // WLP 2005 - made this server client and server side
  //

  // imago removed ZSucceeded(  );
  if (m_pDirectPlayServer) {
	  if(FAILED(m_pDirectPlayServer->GetClientAddress( cnxn.GetDPID(), &pAddress, 0 ))) {
		return E_FAIL;
	  }
  }
  // imago removed ZSucceeded(  );
  if (m_pDirectPlayClient) {
	  if(FAILED(m_pDirectPlayClient->GetServerAddress(&pAddress, 0 ))) {
		return E_FAIL;
	  }
  }

  WCHAR add[200];
  DWORD cnt = 200;
  DWORD type;

  HRESULT hr = pAddress->GetComponentByName( DPNA_KEY_HOSTNAME, (void*)add, &cnt, &type );
  if ( FAILED( hr ) )
  {
    strcpy( szRemoteAddress, "0.0.0.0" );
  }
  else
  {
    char* aadd = wChar2ConstChar( add );
    strcpy( szRemoteAddress, aadd );
    delete[] aadd;
  }

  pAddress->Release();
  return S_OK;
}

HRESULT FedMessaging::GetListeningPort(DWORD* dwPort)	// mdvalley: Finds the port the server is listening at
{
	assert(m_pDirectPlayServer);

	if(!m_pDirectPlayServer)
		return E_FAIL;			// needs a better error code, or something.

	IDirectPlay8Address* pAddress;
	HRESULT hr;

	DWORD dwAddySize = 1;

	hr = m_pDirectPlayServer->GetLocalHostAddresses(&pAddress, &dwAddySize, DPNGETLOCALHOSTADDRESSES_COMBINED);
	if (ZFailed(hr)) return hr;

	DWORD dwBuffSize = sizeof(DWORD);
	DWORD dwDataType;

	DWORD dwPortGot;

	hr = pAddress->GetComponentByName(DPNA_KEY_PORT, &dwPortGot, &dwBuffSize, &dwDataType);
	if (ZFailed(hr)) return hr;

	*dwPort = dwPortGot;

	return S_OK;
}

HRESULT FedMessaging::EnumSessions(GUID guidApplication, const char * szServer)
{
// WLP 2005 - added init client if needed
//
if (!m_pDirectPlayClient)
{
HRESULT hr;
hr = InitDPlayClient();
  if ( FAILED(hr) )
    return hr;
}
// WLP 2005 - end of client init

  m_fSessionCallback = true;
  
  return EnumHostsInternal(guidApplication, szServer);
}

HRESULT FedMessaging::EnumHostsInternal(GUID guidApplication, const char * szServer, DWORD dwPort)	// mdvalley: (optional) dwPort forces enumeration to that port
{
// WLP 2005 - added init client if needed
//
if (!m_pDirectPlayClient)
{
HRESULT hr;
hr = InitDPlayClient();
  if ( FAILED(hr) )
    return hr;
}
// WLP 2005 - end of client init

  //  <NKM> 10-Aug-2004
  // Left this in from old method.... well why not :)
  DWORD dwSize = 0;
  Time timeStart = Time::Now();
  static CTempTimer tt("in FedMessaging::EnumSessionsInternal", 1.0f);
  tt.Start();

  // restore the hourglass cursor
  SetCursor(LoadCursor(NULL, IDC_WAIT));

  assert( m_pDirectPlayClient ); // WLP 2005 - uncommented

  DPN_APPLICATION_DESC   dpnAppDesc;
  IDirectPlay8Address*   pDP8AddressHost  = 0;
  IDirectPlay8Address*   pDP8AddressLocal = 0;
  WCHAR*                 wszHostName      = 0;
  HRESULT                hr;

  // Create the local device address object
  if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
                                     CLSCTX_ALL, IID_IDirectPlay8Address,
                                     (LPVOID*) &pDP8AddressLocal ) ) )  //Fix memory leak -Imago 8/2/09
  {
    m_pfmSite->OnMessageBox( this, "Failed to create DPlay client local address", "Allegiance", MB_OK );
    return hr;
  }

  SafeReleaser<IDirectPlay8Address> r1( pDP8AddressLocal );

  // Set IP service provider
  if( FAILED( hr = pDP8AddressLocal->SetSP( &CLSID_DP8SP_TCPIP ) ) )
  {
    m_pfmSite->OnMessageBox( this, "Failed to create DPlay local SP", "Allegiance", MB_OK );
    return hr;
  }
    
  // Create the remote host address object
  if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
                                     CLSCTX_ALL, IID_IDirectPlay8Address,
                                     (LPVOID*) &pDP8AddressHost ) ) )
  {
    m_pfmSite->OnMessageBox( this, "Failed to create DPlay client remote address", "Allegiance", MB_OK );
    return hr;
  }

  SafeReleaser<IDirectPlay8Address> r2( pDP8AddressHost );
  // Set IP service provider
  if( FAILED( hr = pDP8AddressHost->SetSP( &CLSID_DP8SP_TCPIP ) ) )  //Fix memory leak -Imago 8/2/09
  {
    m_pfmSite->OnMessageBox( this, "Failed to create DPlay remote SP", "Allegiance", MB_OK );
    return hr;
  }

  // Set the remote port
  if(dwPort != 6073 && FAILED(hr = pDP8AddressHost->AddComponent(DPNA_KEY_PORT, &dwPort, sizeof(DWORD), DPNA_DATATYPE_DWORD)))  //Fix memory leak -Imago 8/2/09
  {
	  m_pfmSite->OnMessageBox( this, "Failed to set DPlay client remote port", "Allegiance", MB_OK );
	  return hr;
  }


  // Set the remote host name (if provided)
  if( szServer != 0 )
  {
    wszHostName = constChar2Wchar( szServer );

    hr = pDP8AddressHost->AddComponent( DPNA_KEY_HOSTNAME, wszHostName,
                                        (wcslen(wszHostName)+1)*sizeof(WCHAR),
                                        DPNA_DATATYPE_STRING );  //Fix memory leak -Imago 8/2/09

    delete[] wszHostName;

    if( FAILED(hr) )
    {
      m_pfmSite->OnMessageBox( this, "Failed to set DPlay server", "Allegiance", MB_OK );
      return hr;
    }
  }

  ZeroMemory( &dpnAppDesc, sizeof( DPN_APPLICATION_DESC ) );
  dpnAppDesc.dwSize = sizeof( DPN_APPLICATION_DESC );
  dpnAppDesc.guidApplication = guidApplication;

  //  <NKM> 22-Aug-2004
  // Hmm - hacky globals I don't like, but i'm going to perpetuate this one
  // variable for now.....
  // This is how we pass "what we are looking for" to the enum callback.
  g_guidApplication = guidApplication;

  // Enumerate all StressMazeApp hosts running on IP service providers
  // mmf 05/07 implement WLP's LAN fix, use sync for LAN game, async otherwise

  if (IsEqualGUID(FEDSRV_STANDALONE_PRIVATE_GUID, guidApplication)) {
     hr = m_pDirectPlayClient->EnumHosts( &dpnAppDesc, pDP8AddressHost,
                                          pDP8AddressLocal, NULL,
                                          0, 0, 0, 0, NULL,  // 0, 0, 0, 0, NULL,
                                          0 /*MUST use 0 handle for DPNOP_SYNC flag */ , DPNOP_SYNC );
  } else {
  // Mdvalley: Use an asyncronous EnumHosts with infinite retries to make NATs happy.
     DPNHANDLE fillerHandle;	// Use of filler keeps DPlay from saying nasty things.
     hr = m_pDirectPlayClient->EnumHosts( &dpnAppDesc, pDP8AddressHost,
                                          pDP8AddressLocal, NULL,
                                          0, INFINITE, 0, 0, NULL,
                                          &fillerHandle, 0 );  //Fix memory leak -Imago 8/2/09
  }

// WLP - DPLAY8 def = STDMETHOD(EnumHosts)
//  (THIS_ PDPN_APPLICATION_DESC const pApplicationDesc,                     &dpnAppDesc
//  IDirectPlay8Address *const pAddrHost                                     pDP8AddressHost
//  IDirectPlay8Address *const pDeviceInfo,                                  pDP8AddressLocal
//  PVOID const pUserEnumData,                                               NULL
//  const DWORD dwUserEnumDataSize, .........................................0
//  const DWORD dwEnumCount,                                                 0   number of times to repeat
//  const DWORD dwRetryInterval,                                             0   milliseconds between retries
//  const DWORD dwTimeOut,                                                   0   milliseconds to wait for responses
//  PVOID const pvUserContext,                                               NULL
//  DPNHANDLE *const pAsyncHandle,                                           0
//   const DWORD dwFlags) PURE;                                              DPNOP_SYNC
/* WLP - this is from the MSDN

HRESULT EnumHosts(
PDPN_APPLICATION_DESC const pApplicationDesc,
IDirectPlay8Address *const pdpaddrHost,
IDirectPlay8Address *const pdpaddrDeviceInfo,
PVOID const pvUserEnumData,
const DWORD dwUserEnumDataSize,
const DWORD dwEnumCount,
const DWORD dwRetryInterval,
const DWORD dwTimeOut,
PVOID const pvUserContext,
HANDLE *const pAsyncHandle,
const DWORD dwFlags

*/


  if( FAILED(hr) )
  {
      m_pfmSite->OnMessageBox( this, "Failed to set Enumerate Hosts", "Allegiance", MB_OK );
      return hr;
  }
      
  
  return hr;
}  

CFMConnection * FedMessaging::CreateConnection(const char * szName, DPID dpid) // after the physical connection has already been established
{
  static CTempTimer tt("in CreateConnection", .01f);
  tt.Start();
  CFMConnection * pcnxn = new CFMConnection(this, szName, dpid);
  m_listCnxns.PushFront(pcnxn);
  m_cnxnsMap[dpid] = pcnxn;
  m_pgrpEveryone->AddConnection(this, pcnxn); // we'll let dplay own removing them when they disconnect, since they retain lifetime membership
  m_pfmSite->OnNewConnection(this, *pcnxn);
  tt.Stop();
  return pcnxn;
}

//  <NKM> 07-Aug-2004
// Moved from header file as I need to convert
// from char* to WCHAR and I've put those functions in this file as well
void FedMessaging::SetSessionDetails(char * szDetails) // tokenized name/value pairs: Name\tValue\nName\tValue...
{
  //  <NKM> 07-Aug-2004
  // Change to use Application Desc from DPlay4 Session Desc structures
  DWORD size = 0;
  assert( m_pDirectPlayServer );
  HRESULT hr = m_pDirectPlayServer->GetApplicationDesc( 0, &size, 0 );
  if( hr != DPNERR_BUFFERTOOSMALL )
  {
    ZSucceeded( hr );
  }

  DPN_APPLICATION_DESC* pAppDesc = (DPN_APPLICATION_DESC*) new BYTE[size];
  ZeroMemory( pAppDesc, size );
  pAppDesc->dwSize = sizeof( DPN_APPLICATION_DESC );

  ZSucceeded( m_pDirectPlayServer->GetApplicationDesc( pAppDesc, &size, 0 ) );


  WCHAR* wDetails = constChar2Wchar( szDetails );
  pAppDesc->pwszSessionName = wDetails;

  m_pDirectPlayServer->SetApplicationDesc( pAppDesc, 0 );

  delete[] wDetails;
  delete[] pAppDesc;
}

DWORD FedMessaging::GetCountConnections()
{
    DWORD size = 0; //sizeof( DPN_APPLICATION_DESC );
    assert( m_pDirectPlayServer );

    // first get size we need
    HRESULT hr2 = m_pDirectPlayServer->GetApplicationDesc( 0, &size, 0 );

    //  <NKM> 23-Aug-2004
    // I love this - we now allocate memory of amount size, but we set
    // the size of the struct to sizeof(DPN_APPLICATION_DESC)
    //  Bizare api....
    DPN_APPLICATION_DESC* pAppDesc = (DPN_APPLICATION_DESC*) new BYTE[size];
    ZeroMemory( pAppDesc, size );
    pAppDesc->dwSize = sizeof( DPN_APPLICATION_DESC );

    HRESULT hr = m_pDirectPlayServer->GetApplicationDesc( pAppDesc, &size, 0 );

    DWORD dwConnections = pAppDesc->dwCurrentPlayers;

    delete[] pAppDesc;
    return dwConnections;
  }

//  <NKM> 08-Aug-2004
// Moved from header
// TODO: change for DX9 structs and calls
HRESULT FedMessaging::GetLinkDetails(CFMConnection * pcnxn, OUT DWORD * pdwHundredbpsG, OUT DWORD * pdwmsLatencyG,
                                     OUT DWORD * pdwHundredbpsU, OUT DWORD * pdwmsLatencyU)
{
//   DPCAPS caps;
//   HRESULT hr = E_FAIL;
//   if ( !pcnxn || !GetDPlay() ) // throw error?
//     return E_FAIL;

//   caps.dwSize = sizeof(caps);
//   hr = GetDPlay()->GetPlayerCaps(pcnxn->GetDPID(), &caps, DPGETCAPS_GUARANTEED);
//   if FAILED(hr)
//              goto Exit;
//   if (pdwHundredbpsG)
//     *pdwHundredbpsG = caps.dwHundredBaud;
//   if (pdwmsLatencyG)
//     *pdwmsLatencyG = caps.dwLatency;

//   hr = GetDPlay()->GetPlayerCaps(pcnxn->GetDPID(), &caps, 0);
//   if FAILED(hr)
//              goto Exit;
//   if (pdwHundredbpsU)
//     *pdwHundredbpsU = caps.dwHundredBaud;
//   if (pdwmsLatencyU)
//     *pdwmsLatencyU = caps.dwLatency;
//  Exit:
//   return hr;
  *pdwHundredbpsG = 0;
  *pdwmsLatencyG = 0;
  *pdwHundredbpsU = 0;
  *pdwmsLatencyU = 0;
  return S_OK;
}



CFMConnection::CFMConnection(FedMessaging * pfm, const char * szName, DPID dpid) : // only FedMessaging::CreateConnection can create these things
    CFMRecipient(szName, dpid),
    m_cAbsentCount(0),
    m_dwPrivate(0)
{
  assert(GetDPID() == dpid);
  // debugf("CFMConnection: pfm=%8x dpid=%8x, name=%s, playerdata(this)=%p\n", pfm, dpid, szName, this );
}


void CFMConnection::Delete(FedMessaging * pfm) // basically the destructor, but with a parameter
{
  //debugf("Deleting connection: pfm=%8x dpid=%8x, name=%s, playerdata=%p, private=%u\n",
  //       pfm, GetDPID(), GetName(), this, m_dwPrivate);
  // Don't want to hear anything more from this player
  static CTempTimer tt("in DestroyPlayer/SetPlayerData", 0.02f);
  tt.Start();
  HRESULT hr = pfm->GetDPlayServer()->DestroyClient(GetDPID(), 0, 0, 0);
  debugf("DestroyPlayer=0x%08x\n", hr);
  tt.Stop();
  delete this;
  g_fConnectionDeleted = true;
}


// **************  CFMGroup  **************

void CFMGroup::AddConnection(FedMessaging * pfm, CFMConnection * pcnxn)
{
  DPNHANDLE hand;
  pfm->GetDPlayServer()->AddPlayerToGroup( GetDPID(), pcnxn->GetDPID(), 0, &hand, 0 );
}


void CFMGroup::DeleteConnection(FedMessaging * pfm, CFMConnection * pcnxn)
{
  // ignore return code, since dplay connection could already be dead
  DPNHANDLE hand;
  pfm->GetDPlayServer()->RemovePlayerFromGroup(GetDPID(), pcnxn->GetDPID(), 0, &hand, 0);
}


CFMGroup::CFMGroup(FedMessaging * pfm, const char * szName)
  : m_cPlayers(0), CFMRecipient(szName, 0) // dplay group hasn't been created yet
{
  DPN_GROUP_INFO dpn;
  ZeroMemory( &dpn, sizeof( DPN_GROUP_INFO ) );
  dpn.dwSize = sizeof( DPN_GROUP_INFO );
  dpn.dwInfoFlags = DPNINFO_NAME;
  dpn.pwszName = constChar2Wchar( szName );
  ZSucceeded( pfm->GetDPlayServer()->CreateGroup( &dpn, this, NULL, NULL, DPNOP_SYNC ) );

  //debugf("CFMGroup: pfm=%8x dpid=%8x, name=%s, playerdata(this)=%p, ", pfm, GetDPID(), szName, this );

  delete[] dpn.pwszName;
}


void CFMGroup::Delete (FedMessaging * pfm) 
{
  DPNHANDLE hand;
  pfm->GetDPlayServer()->DestroyGroup(GetDPID(), 0, 0, DPNOP_SYNC );
  delete this;
}


void CFMGroup::PlayerAdded(CFMConnection * pcnxn)
{
  m_cPlayers++;
}


void CFMGroup::PlayerDeleted(CFMConnection * pcnxn)
{
  m_cPlayers--;
}


/*-------------------------------------------------------------------------
 * FMSessionDesc.FMSessionDesc
 *-------------------------------------------------------------------------
  Purpose:
      Crack the name/value pairs in the dplay session name and populate the members

  Side Effects:
      This is kinda of bad coupling, since the server packs this, and the messaging layer unpacks. 
 */
FMSessionDesc::FMSessionDesc( const DPN_APPLICATION_DESC* appDesc)
{
  char* szName = 0;
  char* szValue = 0;
  m_nNumPlayers = 0;
  m_nMaxPlayers = 1;

  char* sessionName = wChar2ConstChar( appDesc->pwszSessionName );

  szName = strtok( sessionName , "\t");
  while(szName)
  {
    if (!lstrcmp(szName, "GAM"))
    {
      szValue = strtok(NULL, "\n");
      m_strGameName = szValue;
    }
    else if (!lstrcmp(szName, "PLR"))
    {
      szValue = strtok(NULL, "\n");
      m_nNumPlayers = (short)atoi(szValue);
    }
    else if (!lstrcmp(szName, "LIM"))
    {
      szValue = strtok(NULL, "\n");
      m_nMaxPlayers = (short)atoi(szValue);
    }
    // there could be many more
    else // throw away any remainder
    {
      szValue = strtok(NULL, "\n");
    }

    szName = strtok(NULL, "\t");
  }
  
  m_guidInstance = appDesc->guidInstance;

  delete[] sessionName;
}


//  <NKM> 19-Aug-2004
// re-implment these

HRESULT FedMessaging::GetSendQueue(DWORD * pcMsgs, DWORD * pcBytes)
{
  if( m_pDirectPlayClient != 0 )
    return m_pDirectPlayClient->GetSendQueueInfo( pcMsgs, pcBytes, 0 );

  *pcMsgs = 0;
  *pcBytes = 0;
  return S_OK;
}

HRESULT FedMessaging::GetReceiveQueue(DWORD * pcMsgs, DWORD * pcBytes)
{
  // DPlay 8 has no receive queue - we handle them async
  *pcMsgs = 0;
  *pcBytes = 0;
  return S_OK;
}

HRESULT FedMessaging::GetConnectionSendQueue(CFMConnection * pcnxn, DWORD * pcMsgs, DWORD * pcBytes)
{
  if ( m_pDirectPlayServer != 0 )
    return m_pDirectPlayServer->GetSendQueueInfo( pcnxn->GetID(), pcMsgs, pcBytes, 0 );

  *pcMsgs = 0;
  *pcBytes = 0;
  return S_OK;
}

HRESULT FedMessaging::GetConnectionInfo(CFMConnection * pcnxn, DPN_CONNECTION_INFO & ConnectionInfo)
{

  if ( m_pDirectPlayServer != 0 )
	  return m_pDirectPlayServer->GetConnectionInfo(pcnxn->GetID(),&ConnectionInfo,0);

  return S_FALSE;
}
