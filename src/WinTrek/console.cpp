#include "pch.h"
#include "consoledata.h"
#include "trekctrls.h"
#include "loadout.h"
#include "trekmdl.h"
#include "training.h"
#include "badwords.h"

#include <button.h>
#include <controls.h>

const int   c_iRecipientOther   = -1;
const int   c_iRecipientMe      =  0;
const int   c_iRecipientSector  =  1;
//const int   c_iRecipientAllies  =  2;		//ALLY imago 7/4/09 NYI
const int   c_iRecipientTeam    =  2;
const int   c_iRecipientAll     =  3;
const int   c_iRecipientWing0   =  4;       //with subsequent wings following

const int c_badMatch = 100000;


const float c_minRectangleSize = 5.0f;
static bool inline  LegalRectangle(const Point& p1, const Point& p2)
{
    return (fabs(p1.X() - p2.X()) > c_minRectangleSize) ||
           (fabs(p1.Y() - p2.Y()) > c_minRectangleSize);
}


//////////////////////////////////////////////////////////////////////////////
//
// ConsoleData
//
//////////////////////////////////////////////////////////////////////////////

const int   c_maskMouseLeftDown     = 0x01;
const int   c_maskMouseRightDown    = 0x02;

//////////////////////////////////////////////////////////////////////////////
//
// PickImage
//
//////////////////////////////////////////////////////////////////////////////

class ConsoleDataImpl;

class ConsolePickImage : public Image 
{
    private:
        MouseResult                  m_mouseresult;
        TRef<ConsoleDataImpl>        m_pconsoleData;
        TRef<Viewport>               m_pviewport;

    public:
        ConsolePickImage(Viewport*         pviewport,
                  ConsoleDataImpl*  pconsoleData
        );
        void        SetHitTest(MouseResult  mr);
        MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured);
        MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown);
        void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside);
        void MouseEnter(IInputProvider* pprovider, const Point& point);
        void MouseLeave(IInputProvider* pprovider);
};

struct ChatSubject
{
    ChatTarget             m_ctRecipient;
    ObjectID               m_oidRecipient;
    TRef<IbaseIGC>         m_pbaseRecipient;

    ShipListIGC            m_shipsSubject;

    ChatSubject(ChatTarget  ct)
    :
        m_ctRecipient(ct),
        m_oidRecipient(NA),
        m_pbaseRecipient(NULL)
    {
    }

    ~ChatSubject(void)
    {
        PurgeSubjects();
    }

    void    PurgeSubjects(void)
    {
        ShipLinkIGC*    psl;
        while (psl = m_shipsSubject.first())    //Intentional =
        {
            psl->data()->Release();
            delete psl;
        }
    }

    void AddSubject(IshipIGC*   pship)
    {
        if (!m_shipsSubject.find(pship))
        {
            pship->AddRef();

            //Hack so that the client's ship is always at the front of the list
            //(so that it will correctly echo the chat in the chat history)
            if (pship == trekClient.GetShip())
                m_shipsSubject.first(pship);
            else
                m_shipsSubject.last(pship);
        }
    }

    void DeleteSubject(IshipIGC*    pship)
    {
        ShipLinkIGC*    psl = m_shipsSubject.find(pship);
        if (psl)
        {
            pship->Release();
            delete psl;
        }
    }

    void ToggleSubject(IshipIGC*    pship)
    {
        ShipLinkIGC*    psl = m_shipsSubject.find(pship);
        if (psl)
        {
            //Do not allow the last person to be toggled out of the group
            if (m_shipsSubject.n() > 1)
            {
                pship->Release();
                delete psl;
            }
        }
        else
        {
            pship->AddRef();
            if (pship == trekClient.GetShip())
                m_shipsSubject.first(pship);
            else
                m_shipsSubject.last(pship);
        }

        if (m_shipsSubject.n() == 1)
        {
            m_ctRecipient = CHAT_INDIVIDUAL;
            m_pbaseRecipient = m_shipsSubject.first()->data();
            m_oidRecipient = m_pbaseRecipient->GetObjectID();
        }
        else
        {
            assert (m_shipsSubject.n() != 0);

            m_ctRecipient = CHAT_GROUP;
            m_pbaseRecipient = NULL;
            m_oidRecipient = NA;
        }
    }
    bool LegalCommand(CommandID cid)
    {
        bool    bLegal;

        ShipLinkIGC*    psl;

        for (psl = m_shipsSubject.first(); 
             ((psl != NULL) && !psl->data()->LegalCommand(cid)); psl = psl->next())
        {
        }

        bLegal = psl != NULL;

        return bLegal;
    }

    bool LegalCommand(CommandID cid, ImodelIGC*    pmodelTarget)
    {
        bool    bLegal;

        ShipLinkIGC*    psl;

        for (psl = m_shipsSubject.first(); 
             ((psl != NULL) && !psl->data()->LegalCommand(cid, pmodelTarget)); psl = psl->next())
        {
        }

        bLegal = psl != NULL;

        return bLegal;
    }

    bool ContainsLayer(void)
    {
        bool    bLegal;

        ShipLinkIGC*    psl;

        for (psl = m_shipsSubject.first(); 
             ((psl != NULL) && (psl->data()->GetPilotType() != c_ptLayer)); psl = psl->next())
        {
        }

        bLegal = psl != NULL;

        return bLegal;
    }
    bool ContainsOnlyBuilders(void)
    {
        bool    bLegal;

        ShipLinkIGC*    psl;

        psl = m_shipsSubject.first();

        if (psl == NULL)
            return false;

        while ((psl != NULL) && (psl->data()->GetPilotType() == c_ptBuilder))
        {
            psl = psl->next();
        }

        bLegal = psl == NULL;

        return bLegal;
    }

    void Reset(bool bStart = false)
    {
        if (m_ctRecipient == CHAT_GROUP)
        {
            ShipLinkIGC*    pslNext;
            for (ShipLinkIGC*   psl = m_shipsSubject.first(); (psl != NULL); psl = pslNext)
            {
                pslNext = psl->next();
                if (psl->data()->GetMission() == NULL)
                    delete psl;
            }

            if (m_shipsSubject.n() == 1)
            {
                m_ctRecipient = CHAT_INDIVIDUAL;
                m_pbaseRecipient = m_shipsSubject.first()->data();
                m_oidRecipient = m_pbaseRecipient->GetObjectID();
            }
            else if (m_shipsSubject.n() == 0)
            {
                m_ctRecipient = CHAT_TEAM;
                m_pbaseRecipient = NULL;
                m_oidRecipient = NA;
            }
        }
        else
            SetRecipient(m_ctRecipient, m_oidRecipient, m_pbaseRecipient);

        if (bStart && (m_ctRecipient == CHAT_INDIVIDUAL))
        {
            IshipIGC*   pship = (IshipIGC*)((IbaseIGC*)m_pbaseRecipient);
            assert (pship);

            if ((pship == trekClient.GetShip()) ||
                (pship->GetPilotType() < c_ptPlayer))
            {
                //Chats to a drone or myself convert to the team
                SetRecipient(CHAT_TEAM, NA, NULL);
            }
        }
    }

    void SetRecipient(ChatTarget            ct,
                      ObjectID              oidRecipient,
                      IbaseIGC*             pbaseRecipient,
                      const ShipListIGC*    pships = NULL)
    {
        PurgeSubjects();

        if ((ct != CHAT_NOSELECTION) && (ct != CHAT_ADMIN))
        {
            if (ct == CHAT_GROUP)
            {
                for (ShipLinkIGC*   psl = pships->first(); (psl != NULL); psl = psl->next())
                {
                    if (psl->data()->GetMission())
                        AddSubject(psl->data());
                }

                if (m_shipsSubject.n() == 1)
                {
                    ct = CHAT_INDIVIDUAL;
                    pbaseRecipient = m_shipsSubject.first()->data();
                    oidRecipient = pbaseRecipient->GetObjectID();
                }
                else if (m_shipsSubject.n() == 0)
                {
                    ct = CHAT_TEAM;
                    pbaseRecipient = NULL;
                    oidRecipient = NA;
                }
            }
            else if (ct == CHAT_INDIVIDUAL)
            {
                if (pbaseRecipient->GetMission() != NULL)
                    AddSubject((IshipIGC*)pbaseRecipient);
                else
                {
                    ct = CHAT_TEAM;
                    pbaseRecipient = NULL;
                    oidRecipient = NA;
                }
            }
            else if (ct == CHAT_SHIP)
            {
                pbaseRecipient = NULL;
                oidRecipient = NA;

                IshipIGC*   pship = trekClient.GetShip()->GetSourceShip();
                const ShipListIGC*  pships = pship->GetChildShips();
                if (pships->n() == 0)
                {
                    ct = CHAT_TEAM;
                }
                else
                {
                    AddSubject(pship);
                    for (ShipLinkIGC*   psl = pships->first(); (psl != NULL); psl = psl->next())
                        AddSubject(psl->data());
                }
            }

            if ((ct != CHAT_GROUP) && (ct != CHAT_INDIVIDUAL) && (ct != CHAT_SHIP))
            {
                //Mark all legal recipients of this chat with an appropriate bit
                IsideIGC*           psideFriendly = trekClient.GetShip()->GetSide();
                WingID              wid;
                const ShipListIGC*  pships = psideFriendly->GetShips();
				//OutputDebugString("In SetRecipient() switch ct="+ZString(ct)+"\n");
                switch (ct)
                {
					case CHAT_ALLIES: //imago 7/4/09 ALLY
                    case CHAT_TEAM:
                    {
                        pbaseRecipient = psideFriendly;
                        assert (pbaseRecipient);
                        oidRecipient = pbaseRecipient->GetObjectID();
                    }
                    break;
                 
                    case CHAT_FRIENDLY_SECTOR:
                    {
                        if (!pbaseRecipient)
                        {
                            assert(oidRecipient == NA);
                            pbaseRecipient = trekClient.GetChatCluster();
                            assert (pbaseRecipient);
                            if (!pbaseRecipient)
                            {
                                ct = CHAT_TEAM;
                                oidRecipient = NA;
                            }
                            else
                                oidRecipient = pbaseRecipient->GetObjectID();
                        }
                        else
                        {
                            assert(oidRecipient == pbaseRecipient->GetObjectID());
                        }
                    }
                    break;

                    case CHAT_WING:
                    {
                        wid = trekClient.GetShip()->GetWingID();
                        if (oidRecipient == wid)
                            oidRecipient = NA;
                        else if (oidRecipient != NA)
                            wid = oidRecipient;
                    }
                    break;

                    case CHAT_EVERYONE:
                    {
                        pships = trekClient.m_pCoreIGC->GetShips();
                    }
                    break;

                    default:
                        assert (false);
                }

                for (ShipLinkIGC*   psl = pships->first(); (psl != NULL); psl = psl->next())
                {
                    IshipIGC*   pship = psl->data();
                    bool    bSubject = false;

                    switch (ct)
                    {
						case CHAT_ALLIES: //Imago 7/4/09 ALLY
                        case CHAT_EVERYONE:
                        {
                            bSubject = true;
                        }
                        break;

                        case CHAT_ADMIN:
                        case CHAT_LEADERS:
                        {
                            bSubject = false;   //NYI
                        }
                        break;

                        case CHAT_TEAM:
                        {
                            assert (pship->GetSide() == pbaseRecipient);
                            bSubject = true;
                        }
                        break;

                        case CHAT_FRIENDLY_SECTOR:
                        {
                            assert (pship->GetSide() == psideFriendly);

                            PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());
                            // XXX is this right?
                            if (ppi)
                                bSubject = (ppi->LastSeenSector() == oidRecipient);
                        }
                        break;

                        case CHAT_WING:
                        {
                            assert (pship->GetSide() == psideFriendly);

                            bSubject = (pship->GetWingID() == wid);
                        }
                        break;

                        default:
                        {
                            assert (false);
                        }
                    }

                    if (bSubject)
                        AddSubject(pship);
                }
            }
        }

        m_ctRecipient = ct;
        m_oidRecipient = oidRecipient;
        m_pbaseRecipient = pbaseRecipient;
    }

    ChatSubject&    operator = (const ChatSubject& chs)
    {
        SetRecipient(chs.m_ctRecipient, chs.m_oidRecipient, chs.m_pbaseRecipient, &chs.m_shipsSubject);

        return *this;
    }
};

class ConsoleDataImpl : 
    public ConsoleData, 
    EventTargetContainer<ConsoleDataImpl>,
    TrekClientEventSink
{
friend class    ConsoleImageImpl;

private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Data Members
    //
    //////////////////////////////////////////////////////////////////////////////

    // name space for data binding

    TRef<INameSpace>    m_pns;
    TRef<INameSpace>    m_pnsDisplays;
    TRef<EventSourceImpl> m_peventSourceEveryFrame;

    // console mode

    TRef<ModifiableNumber> m_pnumberOverlayFlags;

    // data about me

    TRef<ModelData>  m_pmodelDataMe;

    // data about the station which I am on

    TRef<ModelData>  m_pmodelDataMyStation;

    // data for my current target

    TRef<ModelData>  m_pmodelDataTarget;

    // data for my current objective
    //TRef<ModifiableString>  m_pstringObjective;

    //TRef<ModelData>         m_pmodelDataObjectiveTarget;
    //TRef<ModelData>         m_pmodelDataObjectiveIssuer;

    // data for my current queued objective

    //TRef<ModifiableString>  m_pstringQueuedObjective;
    //TRef<ModelData>         m_pmodelDataQueuedObjectiveTarget;
    //TRef<ModelData>         m_pmodelDataQueuedObjectiveIssuer;
    TRef<ModifiableNumber>  m_pnumberAutopilotOn;
    TRef<ModifiableNumber>  m_pnumberDetected;
    TRef<ModifiableNumber>  m_pnumberExperience;
    TRef<ModifiableNumber>  m_pnumberTargetExperience;
    TRef<ModifiableNumber>  m_pnumberLivesOn;
    TRef<ModifiableNumber>  m_pnumberKillsOn;
    TRef<ModifiableNumber>  m_pnumberTargetInRangeFlag;
    TRef<ModifiableNumber>  m_pnumberGotFlag;
    TRef<ModifiableNumber>  m_pnumberGotArtifact;
    TRef<ModifiableNumber>  m_pnumberSignature;
    

    // data for composed chat/cmd

    enum ComposeState
    {
        c_csNotComposing        = 0,
        c_csComposeMouseCommand = 1,
        c_csComposeChat         = 2,
        c_csComposeCommand      = 3,
        c_csComposeShell        = 4,
        c_csComposeQuick        = 5
    };
    
    ComposeState           m_csComposeState;

    bool                   m_bRecipientVisible;

    ChatSubject            m_chsCommand;
    ChatSubject            m_chsChat;
    ChatSubject            m_chsQuick;

    ChatSubject*           m_pchsCurrent;

    CommandID              m_cidVerb;
    TRef<ImodelIGC>        m_pmodelTarget;

    ZString                m_strTypedText;

    bool                   m_bMouseIn;

    TRef<ModifiableNumber> m_pnumberComposeState;
    TRef<ModifiableNumber> m_pnumberCommandID;
    TRef<ModifiableString> m_pstringChatRecipient;
    TRef<ModifiableString> m_pstringChatText;

    TRef<ModifiableNumber> m_pnumberBoardState;

    Vector                 m_positionStart;
    Vector                 m_positionStop;

    /*
    ZString                m_strRecipient;
    ZString                m_strMsg;
    ZString                m_strVerb;
    CommandID              m_idVerb;
    ObjectTypeMask         m_otmVerbLegal;
    ZString                m_strObject;
    ZString                m_strComposedChat;
    TRef<ModifiableString> m_pstringDisplayedChat;
    TRef<ModifiableString> m_pstringDisplayedObject;
    TRef<ModifiableNumber> m_pnumberCommandMask;
    TRef<ModelData>        m_pmodelDataObject;
    */

    // data for radar mode
    TRef<ModifiableNumber> m_pnumberRadarMode;

    // data for my funds
    TRef<ModifiableNumber> m_pnumberMoney;

    // data for sector name
    TRef<ModifiableString> m_pstringSectorName;

    // data for the accepted order
    TRef<ModifiableString> m_pstringAcceptedOrder;
    TRef<ModifiableString> m_pstringQueuedOrder;

    // data for help tips
    TRef<ModifiableString> m_pstringTipText0; // yp - Your_Persona Bug10 (3 lines for status text) patch
	TRef<ModifiableString> m_pstringTipText1;  // yp 
	TRef<ModifiableString> m_pstringTipText2;  // end yp 
    TRef<ModifiableString> m_pstringCriticalTipText0;
    TRef<ModifiableString> m_pstringCriticalTipText1;
    TRef<ModifiableString> m_pstringCriticalTipText2;
    TRef<ModifiableString> m_pstringCriticalTipText3;
    Time                   m_timeLastTipSet0;  // yp - Your_Persona Bug10 (3 lines for status text) patch
	Time                   m_timeLastTipSet1;   // yp 
	Time                   m_timeLastTipSet2;   // end yp 
    Time                   m_timeLastCriticalTipSet0;
    Time                   m_timeLastCriticalTipSet1;
    Time                   m_timeLastCriticalTipSet2;
    Time                   m_timeLastCriticalTipSet3;
    bool                   m_fTargetChanged;
    float                  m_rPreviousMissileLock;

    // data for the current vote
    TRef<ModifiableString> m_pstringCurrentVote;
    TRef<ModifiableNumber> m_pnumberHasVote;

    Point                  m_pointMouseStart;
    Point                  m_pointMouseStop;
    int                    m_maskMouseState;

    TRef<ModifiableColorValue> m_pcolorTargetSide;

    OverlayMask                  m_omRecipientsVisible;
    TRef<ButtonBarPane>          m_pbuttonbarRecipients;
    TRef<ButtonBarPane>          m_pbuttonbarVerbs;
    
    TRef<ComboPane>              m_pcomboWing;

    // Lives remaining
    TRef<ModifiableNumber> m_pnumberLives;

    // Number of Kills
    TRef<ModifiableNumber> m_pnumberKills;


public:

    ConsoleDataImpl(Viewport*   pviewport, const char* pszFileName) :
        m_csComposeState(c_csNotComposing),
        m_rPreviousMissileLock(0.0f),
        m_fTargetChanged(false),
        m_timeLastTipSet0(Time::Now()),  // yp - Your_Persona Bug10 (3 lines for status text) patch
		m_timeLastTipSet1(Time::Now()),   // yp 
		m_timeLastTipSet2(Time::Now()),   // end yp 
        m_timeLastCriticalTipSet0(Time::Now()),
        m_timeLastCriticalTipSet1(Time::Now()),
        m_timeLastCriticalTipSet2(Time::Now()),
        m_timeLastCriticalTipSet3(Time::Now()),
        m_maskMouseState(0),
        m_bMouseIn(false),
        m_chsQuick(CHAT_NOSELECTION),
        m_chsCommand(CHAT_NOSELECTION),
        m_chsChat(CHAT_TEAM),
        m_pchsCurrent(&m_chsCommand)
    {
        // create a namespace

        m_pns =
            GetModeler()->CreateNameSpace(
                "console",
                GetModeler()->GetNameSpace("gamepanes")
            );

        m_pns->AddMember("OnEveryFrame", m_peventSourceEveryFrame = new EventSourceImpl());
        
        // export all of our factories
        ModelData::ExportAccessors(m_pns);
        PartWrapper::ExportAccessors(m_pns);

        //console mode
        m_pns->AddMember("OverlayFlags", m_pnumberOverlayFlags = new ModifiableNumber(0.0f));

        // data about me
        m_pns->AddMember("Me", m_pmodelDataMe = new ModelData());
        m_pns->AddMember("OnMeChange", m_pmodelDataMe->GetOnChangeEventSource());

        // data about my current station
        m_pns->AddMember("MyStation", m_pmodelDataMyStation = new ModelData());

        // data for my current target
        m_pns->AddMember("Target", m_pmodelDataTarget = new ModelData(NULL));
        m_pns->AddMember("OnTargetChange", m_pmodelDataTarget->GetOnChangeEventSource());
        m_pns->AddMember("targetSideColor", m_pcolorTargetSide = new ModifiableColorValue(Color::White()));

        // data for my current objective
        //m_pns->AddMember("Objective", m_pstringObjective = new ModifiableString(""));
        //m_pns->AddMember("ObjectiveTarget", m_pmodelDataObjectiveTarget = new ModelData());
        //m_pns->AddMember("OnObjectiveTargetChange", m_pmodelDataObjectiveTarget->GetOnChangeEventSource());
        //m_pns->AddMember("ObjectiveIssuer", m_pmodelDataObjectiveIssuer = new ModelData());
        //m_pns->AddMember("OnObjectiveIssuerChange", m_pmodelDataObjectiveIssuer->GetOnChangeEventSource());
        m_pns->AddMember("AutopilotOn", m_pnumberAutopilotOn = new ModifiableNumber(0));
        m_pns->AddMember("LivesOn", m_pnumberLivesOn = new ModifiableNumber(0));
        m_pns->AddMember("KillsOn", m_pnumberKillsOn = new ModifiableNumber(0));
        m_pns->AddMember("BoardState", m_pnumberBoardState = new ModifiableNumber(0));

        m_pns->AddMember("Detected", m_pnumberDetected = new ModifiableNumber(0));
        m_pns->AddMember("Experience", m_pnumberExperience = new ModifiableNumber(0));
        m_pns->AddMember("TargetExperience", m_pnumberTargetExperience = new ModifiableNumber(0));

        m_pns->AddMember("TargetInRangeFlag", m_pnumberTargetInRangeFlag = new ModifiableNumber(0));
        m_pns->AddMember("GotFlag", m_pnumberGotFlag = new ModifiableNumber(0));
        m_pns->AddMember("GotArtifact", m_pnumberGotArtifact = new ModifiableNumber(0));

        m_pns->AddMember("Signature", m_pnumberSignature = new ModifiableNumber(0));

        // data for my current queued objective
        //m_pns->AddMember("QueuedObjective", m_pstringQueuedObjective = new ModifiableString(""));
        //m_pns->AddMember("QueuedObjectiveTarget", m_pmodelDataQueuedObjectiveTarget = new ModelData());
        //m_pns->AddMember("OnQueuedObjectiveTargetChange", m_pmodelDataQueuedObjectiveTarget->GetOnChangeEventSource());
        //m_pns->AddMember("QueuedObjectiveIssuer", m_pmodelDataQueuedObjectiveIssuer = new ModelData());
        //m_pns->AddMember("OnQueuedObjectiveIssuerChange", m_pmodelDataQueuedObjectiveIssuer->GetOnChangeEventSource());

        // data for composed chat/cmd
        m_pns->AddMember("ComposeState", m_pnumberComposeState = new ModifiableNumber(c_csNotComposing));
        m_pns->AddMember("CommandVerb", m_pnumberCommandID = new ModifiableNumber(c_csNotComposing));
        m_pns->AddMember("ChatRecipient", m_pstringChatRecipient = new ModifiableString(""));
        m_pns->AddMember("ChatText", m_pstringChatText = new ModifiableString(""));

        // data for my radar mode 
        m_pns->AddMember("RadarMode",           m_pnumberRadarMode =  new ModifiableNumber(0));

        // data for my funds
        m_pns->AddMember("Money",  m_pnumberMoney =  new ModifiableNumber(0));

        // data for help line tips
        m_pns->AddMember("SectorName", m_pstringSectorName = new ModifiableString(""));

        m_pns->AddMember("AcceptedOrder", m_pstringAcceptedOrder = new ModifiableString(""));
        m_pns->AddMember("QueuedOrder", m_pstringQueuedOrder = new ModifiableString(""));

        // data for help line tips
        m_pns->AddMember("TipText", m_pstringTipText0 = new ModifiableString(""));  // yp - Your_Persona Bug10 (3 lines for status text) patch
		m_pns->AddMember("TipText1", m_pstringTipText1 = new ModifiableString(""));  // yp 
		m_pns->AddMember("TipText2", m_pstringTipText2 = new ModifiableString(""));  // end yp 
        m_pns->AddMember("CriticalTipText0", m_pstringCriticalTipText0 = new ModifiableString(""));
        m_pns->AddMember("CriticalTipText1", m_pstringCriticalTipText1 = new ModifiableString(""));
        m_pns->AddMember("CriticalTipText2", m_pstringCriticalTipText2 = new ModifiableString(""));
        m_pns->AddMember("CriticalTipText3", m_pstringCriticalTipText3 = new ModifiableString(""));

        // data for the current vote
        m_pns->AddMember("CurrentVote", m_pstringCurrentVote = new ModifiableString(""));
        m_pns->AddMember("HasVote", m_pnumberHasVote = new ModifiableNumber(0));

        //lives remaining

        m_pns->AddMember("Lives",  m_pnumberLives =  new ModifiableNumber(0));

        //number of kills

        m_pns->AddMember("Kills",  m_pnumberKills =  new ModifiableNumber(0));

        // Verb & recipient button bars
        m_pnsDisplays = GetModeler()->GetNameSpace(pszFileName);

        m_omRecipientsVisible = OverlayMask(m_pnsDisplays->FindNumber("RecipientsVisibleOM", 0.0f));

        CastTo(m_pbuttonbarRecipients, m_pnsDisplays->FindMember("RecipientsBB"));
		// mdvalley: Pointers and class.
        AddEventTarget(&ConsoleDataImpl::OnRecipientButton, m_pbuttonbarRecipients->GetEventSource());

        CastTo(m_pbuttonbarVerbs, m_pnsDisplays->FindMember("VerbsBB"));
        AddEventTarget(&ConsoleDataImpl::OnVerbButton, m_pbuttonbarVerbs->GetEventSource());

        CastTo(m_pcomboWing, m_pnsDisplays->FindMember("WingCombo"));

        for (WingID wingID = 0; wingID < c_widMax; wingID++)
        {
            m_pcomboWing->AddItem(c_pszWingName[wingID],
                                  wingID);
        }
       
        AddEventTarget(&ConsoleDataImpl::OnWingCombo, m_pcomboWing->GetEventSource());

    }

    ~ConsoleDataImpl()
    {
        GetModeler()->UnloadNameSpace("console");
        GetModeler()->UnloadNameSpace("teampane");
        GetModeler()->UnloadNameSpace("teleportpane");
    }

    const TRef<INameSpace>& GetNameSpace(void) const
    {
        return m_pnsDisplays;
    }

    void Update(Time now)
    {
        IshipIGC*   pshipParent = trekClient.GetShip()->GetParentShip();

        m_pmodelDataMe->SetModel(trekClient.GetShip()->GetSourceShip());
        m_pmodelDataMyStation->SetModel(trekClient.GetShip()->GetStation());

        //m_pmodelDataObjectiveTarget->SetModel(trekClient.GetShip()->GetCommandTarget(c_cmdAccepted));
        //m_pmodelDataQueuedObjectiveTarget->SetModel(trekClient.GetShip()->GetCommandTarget(c_cmdQueued));

        //m_pstringObjective->SetValue(trekClient.GetShip()->GetCommandVerb(c_cmdAccepted));
        m_peventSourceEveryFrame->Trigger();
        m_pnumberRadarMode->SetValue((float)GetWindow()->GetRadarMode());
        m_pnumberMoney->SetValue((float)trekClient.GetMoney());
        m_pnumberAutopilotOn->SetValue(trekClient.autoPilot() ? 1.0f : 0.0f);
        
        
        

        
        
        

        if (!pshipParent)
            m_pnumberBoardState->SetValue(0);
        else if (trekClient.GetShip()->GetTurretID() == NA)
            m_pnumberBoardState->SetValue(1);
        else
            m_pnumberBoardState->SetValue(2);

        {
            PlayerInfo* ppi = (PlayerInfo*)(trekClient.GetShip()->GetPrivateData());
            m_pnumberDetected->SetValue((ppi && !ppi->GetDetected()) ? 0.0f : 1.0f);
            m_pnumberLives->SetValue(trekClient.m_pCoreIGC->GetMissionParams()->iLives - ppi->MissionDeaths());
            m_pnumberKills->SetValue(ppi->MissionKills());
            if ((trekClient.m_pCoreIGC->GetMissionParams()->iLives - ppi->MissionDeaths()) >= 10)
                m_pnumberLivesOn->SetValue(0.0f);
            else
                m_pnumberLivesOn->SetValue(1.0f);
            if (trekClient.m_pCoreIGC->GetMissionParams()->IsDeathMatchGame())
                m_pnumberKillsOn->SetValue(1.0f);
            else
                m_pnumberKillsOn->SetValue(0.0f);
        }

        // update my experience.  
        {
            float   experience = trekClient.GetShip()->GetExperienceMultiplier();

            m_pnumberExperience->SetValue(100.0f * (experience - 1.0f));
        }

        // update my target's experience
        {
            ImodelIGC* pmodelTarget = m_pmodelDataTarget->GetModel();
            if (pmodelTarget && pmodelTarget->GetObjectType() == OT_ship)
            {
                m_pnumberTargetExperience->SetValue(
                    100.0f * (((IshipIGC*)pmodelTarget)->GetExperienceMultiplier() - 1.0f));
            }
            else
            {
                m_pnumberTargetExperience->SetValue(0);
            }
        }

        {
            IshipIGC*   pship = trekClient.GetShip()->GetSourceShip();

            //pship->GetFlag() -2 = artifact, -1 = nothing, x = flag of side x
            switch (pship->GetFlag())
            {
                case SIDE_TEAMLOBBY:
                    m_pnumberGotFlag->SetValue(0.0f);
                    m_pnumberGotArtifact->SetValue(1.0f);
                break;
            
                case NA:
                    m_pnumberGotFlag->SetValue(0.0f);
                    m_pnumberGotArtifact->SetValue(0.0f);
                break;
                default:
                    m_pnumberGotFlag->SetValue(1.0f);
                    m_pnumberGotArtifact->SetValue(0.0f);
            }

            m_pnumberSignature->SetValue(100.0f * pship->GetSignature() /
                                         pship->GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaSignature)); 
        }
        
        {
            IclusterIGC*    pcluster = trekClient.GetCluster();
            if (pcluster)
                m_pstringSectorName->SetValue(pcluster->GetName());
        }

        {
            static char bfr[100];
            CommandID   cid = trekClient.GetShip()->GetCommandID(c_cmdAccepted);
            if (cid != c_cidNone)
            {
                strcpy(bfr, c_cdAllCommands[cid].szVerb);
                ImodelIGC*  pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdAccepted);
                if (pmodel)
                {
                    int l = strlen(bfr);
                    bfr[l] = ' ';
                    strcpy(bfr + l + 1, GetModelName(pmodel));
                }
            }
            else
                bfr[0] = '\0';
            m_pstringAcceptedOrder->SetValue(bfr);
        }
        {
            static char bfr[100];
            CommandID   cid = trekClient.GetShip()->GetCommandID(c_cmdQueued);
            if (cid != c_cidNone)
            {
                strcpy(bfr, c_cdAllCommands[cid].szVerb);
                ImodelIGC*  pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdQueued);
                if (pmodel)
                {
                    int l = strlen(bfr);
                    bfr[l] = ' ';
                    strcpy(bfr + l + 1, GetModelName(pmodel));
                }
            }
            else
                bfr[0] = '\0';
            m_pstringQueuedOrder->SetValue(bfr);
        }

        // turn off tip text if it is expired
        float   fTipTextDuration = Training::IsTraining () ? 30.0f : 7.5f;
        if (now - m_timeLastCriticalTipSet0 >= fTipTextDuration)
        {
            m_pstringCriticalTipText0->SetValue("");
            m_timeLastCriticalTipSet0 = now + 3600.0f;
        }
        if (now - m_timeLastCriticalTipSet1 >= fTipTextDuration)
        {
            m_pstringCriticalTipText1->SetValue("");
            m_timeLastCriticalTipSet1 = now + 3600.0f;
        }
        if (now - m_timeLastCriticalTipSet2 >= fTipTextDuration)
        {
            m_pstringCriticalTipText2->SetValue("");
            m_timeLastCriticalTipSet2 = now + 3600.0f;
        }
        if (now - m_timeLastCriticalTipSet3 >= fTipTextDuration)
        {
            m_pstringCriticalTipText3->SetValue("");
            m_timeLastCriticalTipSet3 = now + 3600.0f;
        }

        float rMissileLock = 0.0f;

        ImagazineIGC* magazine = (ImagazineIGC*)(trekClient.GetShip()->GetMountedPart(ET_Magazine, 0));
        if (magazine)
            rMissileLock = magazine->GetLock();
        
        if (rMissileLock > 0.0f && rMissileLock < 1.0f && 
                !(m_rPreviousMissileLock > 0.0f && m_rPreviousMissileLock < 1.0f))
            trekClient.PostText(false, "Partial missile lock - center the target on screen to improve the lock.");
        else if (rMissileLock == 1.0f && m_rPreviousMissileLock != 1.0f)
            trekClient.PostText(false, "%s", (const char*)(ZString("Missile lock acquired - press the right mouse button to fire a ")
                + magazine->GetMissileType()->GetName() + " missile at the target."));
        else if (rMissileLock == 0.0f && m_rPreviousMissileLock != 0.0f && !m_fTargetChanged)
            trekClient.PostText(false, "Missile lock lost.");

        m_rPreviousMissileLock = rMissileLock;

        m_fTargetChanged = false;

        BallotInfo* pballotinfo = trekClient.GetCurrentBallot();
        m_pnumberHasVote->SetValue(pballotinfo ? 1.0f : 0.0f);
        m_pstringCurrentVote->SetValue(pballotinfo ? ZString(pballotinfo->GetBallotText()) : ZString());

        /*
        if (m_bRecipientVisible)
        {
            int nShipsPerWing[c_widMax];
            for (int i = 0; (i < c_widMax); i++)
                nShipsPerWing[i] = 0;

            for (ShipLinkIGC*   psl = trekClient.GetSide()->GetShips()->first(); (psl != NULL); psl = psl->next())
            {
                WingID  wid = psl->data()->GetWingID();
                if (wid >= 0)
                {
                    assert (wid < c_widMax);
                    nShipsPerWing[wid]++;
                }
            }

            for (int j = 0; (j < c_widMax); j++)
            {
                if (nShipsPerWing[j] == 0)
                {
                    //Delete wing if it exists
                }
                else
                {
                    
                    m_pcomboWing->AddItem(c_pszWingName[j], j);
                }
            }

            m_pcomboWing->SetSelection(trekClient.GetShip()->GetWingID());
        }
        */
    }

    void SetInRange(bool bInRange)
    {
        m_pnumberTargetInRangeFlag->SetValue(bInRange ? 1.0f : 0.0f);
    }

    void SetTarget(ImodelIGC* pmodel)
    {
        if (m_pmodelDataTarget->GetModel() != pmodel)
        {
            m_fTargetChanged = true;
            m_pmodelDataTarget->SetModel(pmodel);
        }
    }

    void SetOverlayFlags(OverlayMask om)
    {
        m_pnumberOverlayFlags->SetValue((float)om);

        bool    bNew = (om & m_omRecipientsVisible) != 0;

        if (bNew != m_bRecipientVisible)
        {
            m_chsCommand.SetRecipient(CHAT_INDIVIDUAL, trekClient.GetShipID(), trekClient.GetShip());
            AdjustVerbs(&m_chsCommand);
        }

        m_bRecipientVisible = bNew;
    }

    OverlayMask GetOverlayFlags(void) const
    {
        return (OverlayMask)(m_pnumberOverlayFlags->GetValue());
    }

    void OnSwitchViewMode()
    {
        GetWindow()->SetCursor(AWF_CURSOR_DEFAULT);
        m_maskMouseState = 0;

        if (m_csComposeState == c_csComposeMouseCommand)
            QuitComposing();
    }

    void SetTipText(const ZString& strHelp)
    {
		m_timeLastTipSet2 = m_timeLastTipSet1;  // yp - Your_Persona Bug10 (3 lines for status text) patch
		m_timeLastTipSet1 = m_timeLastTipSet0;  // yp 
        m_timeLastTipSet0 = Time::Now();       // yp 

		m_pstringTipText2->SetValue(m_pstringTipText1->GetValue());   // yp 
		m_pstringTipText1->SetValue(m_pstringTipText0->GetValue());   // yp      
        m_pstringTipText0->SetValue(strHelp);        // end yp 
    }

    void SetCriticalTipText(const ZString& strHelp)
    {
        if (!Training::IsTraining())
        {
            // scroll the tip text and the help
            m_pstringCriticalTipText3->SetValue(m_pstringCriticalTipText2->GetValue());
            m_pstringCriticalTipText2->SetValue(m_pstringCriticalTipText1->GetValue());
            m_pstringCriticalTipText1->SetValue(m_pstringCriticalTipText0->GetValue());

            m_timeLastCriticalTipSet3 = m_timeLastCriticalTipSet2;
            m_timeLastCriticalTipSet2 = m_timeLastCriticalTipSet1;
            m_timeLastCriticalTipSet1 = m_timeLastCriticalTipSet0;
        }

        m_pstringCriticalTipText0->SetValue(strHelp);        
        m_timeLastCriticalTipSet0 = Time::Now();

        // HACK: handle the case where the text will wrap by adding a blank 
        // critical tip after it (a hack because of the max of 2 lines, 
        // hardcoded font, and hardcoded width)
        static IEngineFont* pfont = GetModeler()->GetNameSpace("font")->FindFont("medBoldVerdana");
        if (pfont->GetMaxTextLength(strHelp, 620, true) < strHelp.GetLength())
            SetCriticalTipText("");
    }

    void SetComposeState(ComposeState cs)
    {
        {
            IclusterIGC*    pcluster = trekClient.GetCluster();
            if (pcluster)
            {
                //Clear subject masks for all ships and legal objects
                for (ModelLinkIGC*   pml = pcluster->GetPickableModels()->first(); (pml != NULL); pml = pml->next())
                {
                    ImodelIGC*   pmodel = pml->data();
                    ThingSite*  pts = pmodel->GetThingSite();
                    if (pts)
                    {
                        pts->SetMask(0);
                    }
                }
            }
        }
        m_csComposeState = cs;
        m_pnumberComposeState->SetValue((float)(int)cs);
    }

    void SetVerb(CommandID  cidVerb)
    {
        m_cidVerb = cidVerb;
        m_pnumberCommandID->SetValue((float)(int)cidVerb);
    }

    ComposeState GetComposeState()
    {
        return m_csComposeState;
    }

    void OnMoneyChange(PlayerInfo* pPlayerInfo)
    {
    }

    void OnDelPlayer(MissionInfo *pMissionDef, SideID sideID, PlayerInfo *pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
    {
        if (!Training::IsTraining())
        {
            if (pPlayerInfo == trekClient.GetPlayerInfo())
            {
                // go back to the team screen, at least for the moment
                GetWindow()->screen(ScreenIDTeamScreen);
            }
            else if (m_csComposeState != c_csNotComposing)
            {
                //NYI ... need to handle the case where the person receiving a composed chat
                //goes away..
            }
        }
    }

    bool OnBackButton()
    {
        if (!Training::IsTraining () && !trekClient.GetShip()->IsGhost())
            GetWindow()->SetViewMode(trekClient.flyingF() ? TrekWindow::vmCombat : TrekWindow::vmHangar);

        return true;
    }

    bool OnRecipientButton(int nColumn)
    {
        if (m_csComposeState != c_csComposeShell)
        {
            ChatTarget  ct;
            ObjectID    oid;
            IbaseIGC*   pbase;

            switch (nColumn)
            {
                case c_iRecipientMe:
                    ct = CHAT_INDIVIDUAL;
                    pbase = trekClient.GetShip();
                    oid = NA;
					PickShip(trekClient.GetShip()); //Imago #151
                break;

                case c_iRecipientTeam:
                    ct = CHAT_TEAM;
                    pbase = NULL;
                    oid = NA;
					PickShip(trekClient.GetShip()); //Imago #151
                break;

                case c_iRecipientSector:
                    ct = CHAT_FRIENDLY_SECTOR;
                    pbase = NULL;
                    oid = NA;
					PickShip(trekClient.GetShip()); //Imago #151
                break;
/*
                case c_iRecipientAllies: //ALLY 7/4/09 imago NYI
                    ct = CHAT_ALLIES;
                    pbase = NULL;
                    oid = NA;
                break;
*/
                case c_iRecipientAll:
                    ct = CHAT_EVERYONE;
                    pbase = NULL;
                    oid = NA;
					PickShip(trekClient.GetShip()); //Imago #151
                break;

                default:
                    ct = CHAT_WING;
                    pbase = NULL;
                    oid = nColumn - c_iRecipientWing0;
					PickShip(trekClient.GetShip()); //Imago #151
            }

            if (m_csComposeState == c_csNotComposing)
                StartComposing(c_csComposeChat, &m_chsChat);

            m_pchsCurrent->SetRecipient(ct, oid, pbase);

            UpdateComposedChat();
        }

        return true;
    }

    bool OnWingCombo(int index)
    {
        OnRecipientButton(c_iRecipientWing0 + index);
        return true;                
    }
    /*
    bool OnWingButton0()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton1()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton2()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton3()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton4()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton5()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton6()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton7()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton8()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    bool OnWingButton9()
    {
        OnRecipientButton(c_iRecipientWing0);

        return true;
    }
    */

    bool OnVerbButton(int nColumn)
    {
        if (m_csComposeState != c_csComposeShell)
        {
            trekClient.PlaySoundEffect(chatKeySound);

            if (m_csComposeState == c_csNotComposing)
                StartComposing(c_csComposeCommand, &m_chsCommand);
            else
                SetComposeState(c_csComposeCommand);

            SetVerb(CommandID(nColumn + c_cidAttack));

            UpdateComposedChat();
        }
        return true;
    }

    void UpdateComposedChat()
    {
        AdjustVerbs(m_pchsCurrent);

        if (m_csComposeState != c_csNotComposing)
        {
            GetWindow()->SetCommandAsteroid(0);

            //Fill in the typed text
            if (m_csComposeState != c_csComposeMouseCommand)
            {
                ZString displayedChat = m_strTypedText + "|";

                if (m_csComposeState == c_csComposeCommand)
                {
                    if (m_pmodelTarget)
                    {
                        //Fill in the rest of the chat text with the remainder of the picked name
                        char    bfr[c_cbName];
                        int lenText = m_strTypedText.GetLength();
                        strcpy(bfr, (m_pmodelTarget == trekClient.GetShip()) &&
                                    (lenText <= 2) &&
                                    (_strnicmp(m_strTypedText, "me", lenText) == 0)
                                    ? "me"
                                    : GetModelName(m_pmodelTarget));

                        int lenName = strlen(bfr);

                        if (lenText < lenName)
                        {
                            //Convert the trailing characters to upper case
                            for (int i = lenText; (i < lenName); i++)
                                bfr[i] = toupper(bfr[i]);

                            displayedChat += &bfr[lenText];
                        }                            
                    }
                }

                m_pstringChatText->SetValue(displayedChat);
            }

            if (m_csComposeState != c_csComposeShell)
            {
                //Fill in the recipient
                static const char*  pszEveryone = "All";
				static const char*  pszAllies = "Allies";
                static const char*  pszLeaders = "Leaders";
                static const char*  pszShip  = "ship";
                static const char*  pszGroup = "Group";
                static const char*  pszAdmin = "Admin";
                static const char*  pszNone = "None";

                const char* pszRecipient;
                switch (m_pchsCurrent->m_ctRecipient)
                {
					//OutputDebugString("In UpdateComposedChat() switch m_pchsCurrent->m_ctRecipient="+ZString(m_pchsCurrent->m_ctRecipient)+"\n");
                    case CHAT_NOSELECTION:
                        pszRecipient = pszNone;
                    break;          
                    case CHAT_EVERYONE:
                        pszRecipient = pszEveryone;
                    break;
                    case CHAT_GROUP:
                        pszRecipient = pszGroup;

                        {
                            AsteroidAbilityBitMask  aabm = c_aabmBuildable;
                            for (ShipLinkIGC*   psl = m_pchsCurrent->m_shipsSubject.first(); (psl != NULL); psl = psl->next())
                            {
                                IshipIGC*   pship = psl->data();
                                if (pship->GetPilotType() == c_ptBuilder)
                                {
                                    aabm |= pship->GetOrdersABM();
                                }
                            }
                            GetWindow()->SetCommandAsteroid(aabm);
                        }
                    break;
                

                    case CHAT_LEADERS:
                        pszRecipient = pszLeaders;
                    break;

                    case CHAT_SHIP:
                        pszRecipient = pszShip;
                    break;

                    case CHAT_TEAM:
                    {
                        IsideIGC*   pside = m_pchsCurrent->m_pbaseRecipient == NULL
                                            ? trekClient.GetShip()->GetSide()
                                            : (IsideIGC*)((IbaseIGC*)m_pchsCurrent->m_pbaseRecipient);

                        pszRecipient = pside->GetName();
                    }
                    break;

                    case CHAT_INDIVIDUAL:
                    {
                        assert (m_pchsCurrent->m_pbaseRecipient != NULL);

                        IshipIGC*   pship = (IshipIGC*)((IbaseIGC*)m_pchsCurrent->m_pbaseRecipient);
                        pszRecipient = pship->GetName();

                        if (pship->GetPilotType() == c_ptBuilder)
                        {
                            GetWindow()->SetCommandAsteroid(pship->GetOrdersABM());
                        }
                    }
                    break;
    
                    case CHAT_ADMIN:
                    {
                        pszRecipient = pszAdmin;
                    }
                    break;

                    case CHAT_WING:
                    {
						// BT - 9/17 - Prevent crashes in the trainging missions when the pilot is not on any specific wing. 
						if (trekClient.GetShip()->GetWingID() == NA && m_pchsCurrent->m_oidRecipient == NA)
							pszRecipient = c_pszWingName[0];
						else
							pszRecipient = c_pszWingName[m_pchsCurrent->m_oidRecipient == NA
                                                     ? trekClient.GetShip()->GetWingID()
                                                     : m_pchsCurrent->m_oidRecipient];
                    }
                    break;

					case CHAT_ALLIES: //imago 7/4/09 ALLY
						pszRecipient = pszAllies;
					break;   

                    case CHAT_ALL_SECTOR:
                    {
                        //NYI Need to distunguish between all and friendly
                    }

                    case CHAT_FRIENDLY_SECTOR:
                    {
                        IclusterIGC*    pcluster;
                        if (m_pchsCurrent->m_pbaseRecipient == NULL)
                        {
                            pcluster = trekClient.GetChatCluster();
                            assert (pcluster != NULL);
                            if (pcluster)
                                pszRecipient = pcluster->GetName();
                            else
                                pszRecipient = "<unknown>";
                        }
                        else
                            pszRecipient = ((IclusterIGC*)((IbaseIGC*)m_pchsCurrent->m_pbaseRecipient))->GetName();

                    }  
                }
                m_pstringChatRecipient->SetValue(CensorBadWords (pszRecipient));
            }
        }
    }

    void OnAddPlayer(MissionInfo *pMissionDef, SideID sideID, PlayerInfo *pPlayerInfo)
    {
    }

    void SendChatInternal(const char* pszText,
                          int         idSonicChat,
                          CommandID   cid,
                          ImodelIGC*  pmodel)
    {
        m_pchsCurrent->Reset();
        if ((m_pchsCurrent->m_shipsSubject.n() != 0) || (m_pchsCurrent->m_ctRecipient == CHAT_ADMIN))
        {
            ObjectType  otTarget = NA;
            ObjectID    oidTarget = NA;

            if (pmodel)
            {
                if (pmodel->GetMission())
                {
                    otTarget  = pmodel->GetObjectType();
                    oidTarget = pmodel->GetObjectID();
                }
                else
                    pmodel = NULL;      //Model has been terminated
            }

            if (m_pchsCurrent->m_ctRecipient == CHAT_GROUP)
            {
                ChatTarget ct = CHAT_GROUP;
                for (ShipLinkIGC*   psl = m_pchsCurrent->m_shipsSubject.first(); (psl != NULL); psl = psl->next())
                {
                    IshipIGC* pship = psl->data();

                    trekClient.SendChat(
                        trekClient.GetShip(),
                        ct, 
                        pship->GetObjectID(),
                        idSonicChat, 
                        pszText,
                        cid,
                        otTarget, 
                        oidTarget, 
                        pmodel
                    );

                    ct = CHAT_GROUP_NOECHO;
                }
            }
            else
            {
                trekClient.SendChat(
                    trekClient.GetShip(),
                    m_pchsCurrent->m_ctRecipient,
                    m_pchsCurrent->m_oidRecipient,
                    idSonicChat,
                    pszText,
                    cid,
                    otTarget, 
                    oidTarget, 
                    pmodel
                );
            }
        }
    }
    void StartQuickChat(ChatTarget  ctRecipient,
                        ObjectID    oidRecipient,
                        IbaseIGC*   pbaseRecipient)
    {
        if (m_csComposeState == c_csNotComposing)
        {
            if (ctRecipient != CHAT_NOSELECTION)
            {
                m_chsQuick.SetRecipient(ctRecipient, oidRecipient, pbaseRecipient);
                StartComposing(c_csComposeQuick, &m_chsQuick);
            }
            else
                StartComposing(c_csComposeQuick, &m_chsChat);
        }
        else
        {
            assert (m_pchsCurrent);
            if (ctRecipient != CHAT_NOSELECTION)
                m_pchsCurrent->SetRecipient(ctRecipient, oidRecipient, pbaseRecipient);

            SetComposeState(c_csComposeQuick);
        }

        UpdateComposedChat();
    }

    void SendQuickChat(int idSonicChat, int ttMask, CommandID cid, AbilityBitMask aabm)
    {
        //m_pchsCurrent = &m_chsChat;

        if (cid == c_cidMyAccepted)
            cid = trekClient.GetShip()->GetCommandID(c_cmdAccepted);

        //
        // figure out the target
        //
        ImodelIGC* pmodel;
        if (cid != c_cidNone)
        {
            assert (ttMask);

            switch (ttMask)
            {
                case c_ttMyAccepted:
                case c_ttMyQueued:
                case c_ttMyTarget:
                    assert (char(c_ttMyAccepted) == c_cmdAccepted);
                    assert (char(c_ttMyQueued) == c_cmdQueued);
                    assert (char(c_ttMyTarget) == c_cmdCurrent);

                    pmodel = trekClient.GetShip()->GetCommandTarget(char(ttMask));
                break;

                case c_ttWorstEnemy:
                {
                    DamageTrack*        pdt = trekClient.GetShip()->GetDamageTrack();
                    assert (pdt);
                    DamageBucketLink* pdbl = pdt->GetDamageBuckets()->first();
                    pmodel = pdbl ? pdbl->data()->model() : NULL;
                }
                break;

                case c_ttMe:
                    pmodel = trekClient.GetShip();
                break;

                default:
                    pmodel = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);

                    if ((!pmodel) ||
                        (pmodel == trekClient.GetShip()) ||
                        (!FindableModel(pmodel, trekClient.GetShip()->GetSide(), ttMask, aabm)) ||
                        (!trekClient.GetShip()->CanSee(pmodel)))
                    {
                        pmodel = FindTarget(trekClient.GetShip(), ttMask, NULL, NULL, NULL, NULL, aabm);
                    }
            }

            if (pmodel == NULL)
                cid = c_cidNone;
        }
        else
            pmodel = NULL;

        //Send the chat
        SendChatInternal(NULL,
                         idSonicChat,
                         cid,
                         pmodel);

        QuitComposing();
    }

    void SendChat()
    {
        if (m_csComposeState == c_csComposeChat)
        {
            assert (m_strTypedText.GetLength() != 0);

            SendChatInternal(m_strTypedText,
                             NA,
                             c_cidNone,
                             NULL);
        }
        else if (m_csComposeState == c_csComposeShell)
        {
            trekClient.ParseShellCommand(m_strTypedText);
        }
        else
        {
            SendChatInternal(
                NULL,
                NA,
                m_cidVerb,
                m_pmodelTarget
            );
        }
        
        QuitComposing();
    }
    void    AdjustVerbs(ChatSubject*    pchs)
    {
        if (m_bRecipientVisible)
        {
            for (CommandID i = c_cidAttack; (i < c_cidMax); i++)
                m_pbuttonbarVerbs->SetEnabled(i - c_cidAttack, pchs->LegalCommand(i));
        }
    }

    void QuitComposing()
    {
        // clean up the composing process        
        SetComposeState(c_csNotComposing);
        SetChatObject(NULL);

        // default to the command target
        m_pchsCurrent = &m_chsCommand;
        AdjustVerbs(&m_chsCommand);

        GetWindow()->SetCommandAsteroid(0);
    }

    void StartComposing(ComposeState  cs, ChatSubject*    pchsTarget)
    {
        assert (cs != c_csNotComposing);
        assert (m_csComposeState == c_csNotComposing);

        // set the current chat target
        pchsTarget->Reset(cs == c_csComposeChat);
        m_pchsCurrent = pchsTarget;

        SetChatObject(NULL);
        m_strTypedText.SetEmpty();

        // set the compose state
        SetComposeState(cs);
    }

    IshipIGC*   PickShip(IclusterIGC*   pcluster, const Point&   pointPick)
    {
        IshipIGC*   pshipMin = NULL;
        float       distanceMin = FLT_MAX;

        Rect    rectImage = GetWindow()->GetViewport()->GetViewRect()->GetValue();
        Point   point = pointPick - rectImage.Center();

        for (ShipLinkIGC*   psl = pcluster->GetShips()->first(); (psl != NULL); psl = psl->next())
        {
            IshipIGC*   pship = psl->data();
            if (pship->GetVisibleF())
            {
                ThingSite*  psite = pship->GetThingSite();
                if (psite)
                {
                    const Point&    pxy = psite->GetScreenPosition();
                    float           r = psite->GetScreenRadius();

                    if ((pxy.X() + r >= point.X()) && (pxy.X() - r <= point.X()) &&
                        (pxy.Y() + r >= point.Y()) && (pxy.Y() - r <= point.Y()))
                    {
                        float   dx = pxy.X() - point.X();
                        float   dy = pxy.Y() - point.Y();
                        float   d = dx*dx + dy*dy;

                        if (d <= distanceMin)
                        {
                            pshipMin = pship;
                            distanceMin = d;
                        }
                    }
                }
            }                
        }

        return pshipMin;
    }

    void    UpdatePick3D()
    {
        m_positionStop.x = m_positionStart.x;
        m_positionStop.y = m_positionStart.y;

        Camera* pcamera = GetWindow()->GetCamera();
        const Orientation&  o = pcamera->GetOrientation();
        const Vector&       b = o.GetBackward();
        if ((b.z > 0.9f) || (b.z < -0.9f))
        {
            //Too steep a view to making picking in 3D meaningful
            m_positionStop.z = 0.0f;
        }
        else
        {
            Rect    rectImage = GetWindow()->GetViewport()->GetViewRect()->GetValue();
            float   halfwidth = (0.5f * rectImage.XSize());

            const Vector&   positionCamera = pcamera->GetPosition();

            {
                Point   pointStop = m_pointMouseStop - rectImage.Center();
                Vector  directionStop = pcamera->TransformImageToDirection(pointStop / halfwidth);

                float   px = m_positionStart.x - positionCamera.x;
                float   py = m_positionStart.y - positionCamera.y;

                m_positionStop.z = (px * directionStop.x * directionStop.z + 
                                    py * directionStop.y * directionStop.z) /
                                   (directionStop.x * directionStop.x + directionStop.y * directionStop.y) + positionCamera.z;
            }
        }
    }

    void    PickCluster(IclusterIGC* pcluster, int button)
    {
        if (m_csComposeState == c_csNotComposing)
        {
            StartComposing(c_csComposeChat, &m_chsChat);

            m_pchsCurrent->SetRecipient(CHAT_FRIENDLY_SECTOR, pcluster->GetObjectID(), pcluster);

            UpdateComposedChat();
        }
        else
        {
            if (m_csComposeState != c_csComposeCommand)
                m_csComposeState = c_csComposeCommand;

            if ((m_pchsCurrent->m_ctRecipient == CHAT_INDIVIDUAL) && m_pchsCurrent->m_pbaseRecipient->GetMission())
            {
                //For chats to living individuals, treat a single builder as a special case
                IshipIGC*   pship = (IshipIGC*)(IbaseIGC*)(m_pchsCurrent->m_pbaseRecipient);

                if (pship->GetPilotType() == c_ptBuilder)
                {
                    AbilityBitMask abmOrders = pship->GetOrdersABM();
                    if (abmOrders != c_aabmBuildable)
                    {
                        ImodelIGC*  pmodel = FindTarget(pship,
                                                        c_ttNeutral | c_ttAsteroid | c_ttLeastTargeted,
                                                        NULL, pcluster, &(Vector::GetZero()), NULL,
                                                        abmOrders);

                        if (pmodel)
                        {
                            SetVerb(c_cidBuild);
                            SetChatObject(pmodel);
                            SendChat();
                            return;
                        }
                    }
                }
            }

            SetVerb(c_cidGoto);

            // create a cluster buoy in that sector
            DataBuoyIGC db;

            db.position = Vector(0, 0, 0);
            db.clusterID = pcluster->GetObjectID();
            db.type = c_buoyCluster;

            IbuoyIGC*   b = (IbuoyIGC*)(trekClient.m_pCoreIGC->CreateObject(trekClient.m_now, OT_buoy, &db, sizeof(db)));
            assert (b);

            SetChatObject(b);

            SendChat();
            b->Release();
        }
    }

    void PickShip(IshipIGC* pship)
    {
        assert(pship);

        if (m_csComposeState == c_csNotComposing)
        {
            //Wasn't composing ... start composing to the individual
            StartComposing(c_csComposeMouseCommand, &m_chsCommand);
        }

        m_pchsCurrent->SetRecipient(CHAT_INDIVIDUAL, pship->GetObjectID(), pship);

        UpdateComposedChat();
    }

    bool IsComposingCommand()
    {
        return (m_csComposeState == c_csComposeMouseCommand || m_csComposeState == c_csComposeCommand);
    }

    IbaseIGC*  GetPickedObject (void)
    {
        return m_pchsCurrent->m_pbaseRecipient;
    }

    /* NYI ... twizzle implementation once exact usage is defined
    void    PickModel(ImodelIGC* pmodel, int button)
    {
        assert(pmodel);

        if ((m_csComposeState == c_csComposeCommand) ||
            ((m_csComposeState == c_csComposeChat) && button == 1))
        {
            if (m_csComposeState != c_csComposeCommand)
            {
                SetVerb(c_cidGoto);
                m_csComposeState = c_csComposeCommand;
            }

            // make this the object of the command
            SetChatObject(pmodel);
            SendChat();
        }
        else 
        {
            // if the model is a ship, make it the subject of a chat
            if (pmodel->GetObjectType() == OT_ship)
            {
                if (m_csComposeState == c_csNotComposing)
                    StartComposing(c_csComposeCommand);

                m_pchsCurrent->SetRecipient(CHAT_INDIVIDUAL, pmodel->GetObjectID(), pmodel);
                UpdateComposedChat();
            }
        }
    }
    */

    ImodelIGC*  ResolvePick(IclusterIGC* pcluster, const Point& pointPick, int button)
    {

        CommandID       cid =  (m_csComposeState == c_csComposeCommand) ? m_cidVerb : c_cidDefault;
        ChatSubject*    pchs = (m_csComposeState == c_csNotComposing) ? &m_chsCommand : m_pchsCurrent;

        Rect    rectImage = GetWindow()->GetViewport()->GetViewRect()->GetValue();
        Point   point = pointPick - rectImage.Center();

        //Loop over all pickable objects in the sector
        ImodelIGC*  pmodelMin = NULL;
        float       distanceMin = FLT_MAX;

        for (ModelLinkIGC*   pml = pcluster->GetPickableModels()->first(); (pml != NULL); pml = pml->next())
        {
            ImodelIGC*   pmodel = pml->data();
            if (pmodel->GetVisibleF())
            {
                bool    bLegal;
                if (button != 1)
                    bLegal = (pmodel->GetObjectType() == OT_ship) &&
                             (pmodel->GetSide() == trekClient.GetSide());
                else
                    bLegal = false;

                if ((button != 0) && (!bLegal))
                    bLegal = pchs->LegalCommand(cid, pmodel);

                if (bLegal)
                {
                    ThingSite*  psite = pmodel->GetThingSite();
                    if (psite)
                    {
                        const Point&    pxy = psite->GetScreenPosition();
                        float           r = psite->GetScreenRadius();

                        if ((pxy.X() + r >= point.X()) && (pxy.X() - r <= point.X()) &&
                            (pxy.Y() + r >= point.Y()) && (pxy.Y() - r <= point.Y()))
                        {
                            float   dx = pxy.X() - point.X();
                            float   dy = pxy.Y() - point.Y();
                            float   d = dx*dx + dy*dy;

                            if (d <= distanceMin)
                            {
                                pmodelMin = pmodel;
                                distanceMin = d;
                            }
                        }
                    }
                }
            }
        }

        return pmodelMin;
    }

    void    PickObject(IclusterIGC* pcluster, const Point& pointPick, int button)
    {
        ImodelIGC*  pmodelMin = ResolvePick(pcluster, pointPick, button);
		
		//AEM - We don't want to go to an object the mouse cursor happens to be over if we are picking
		//      a coordinate above or below the grid (6/10/07)
		bool goToObject = true;
		//		Check to see if the mouse moved, allowing some room for error when you are moving the mouse around fast to select a object target 
		if ( ( ( m_pointMouseStart.X() - m_pointMouseStop.X() ) * ( m_pointMouseStart.X() - m_pointMouseStop.X() ) )+
			( ( m_pointMouseStart.Y() - m_pointMouseStop.Y() ) * ( m_pointMouseStart.Y() - m_pointMouseStop.Y() ) ) > 100 ) //Imago Reduced 6/10 #33
			goToObject = false;

        if (pmodelMin && goToObject) //was if (pmodelMin)
        {
            if (m_csComposeState != c_csComposeCommand)
            {
                if (m_csComposeState == c_csNotComposing)
                    StartComposing(c_csComposeMouseCommand, &m_chsCommand);
                else
                    m_csComposeState = c_csComposeCommand;

                SetVerb(c_cidDefault);
            }

            SetChatObject(pmodelMin);
            SendChat();
        }
        else if (m_bRecipientVisible)
        {
            if ((m_csComposeState != c_csComposeCommand) ||
                (m_cidVerb == c_cidGoto) ||
                ((m_cidVerb == c_cidBuild) && m_pchsCurrent->ContainsLayer()))
            {
                //Pick an arbitrary point in space
                if (m_csComposeState != c_csComposeCommand)
                {
                    if (m_csComposeState == c_csNotComposing)
                        StartComposing(c_csComposeMouseCommand, &m_chsCommand);
                    else
                        m_csComposeState = c_csComposeCommand;

                    SetVerb(c_cidDefault);
                }
                                                                
                DataBuoyIGC db;
                UpdatePick3D();

                db.position = m_positionStop;
                db.clusterID = trekClient.GetCluster()->GetObjectID();
                db.type = c_buoyWaypoint;

                IbuoyIGC*   b = (IbuoyIGC*)(trekClient.m_pCoreIGC->CreateObject(trekClient.m_now, OT_buoy, &db, sizeof(db)));
                assert (b);

                SetChatObject(b);

                SendChat();
                b->Release();
            }
        }
        else if (button == 0)
        {
            QuitComposing();
        }
    }                

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {     
        //Imago 8/15/09
        if (button > 1) {
            TRef<TrekInput> pinput = GetWindow()->GetInput();
            TRef<Boolean> pboolDown = new Boolean(bDown);
            switch(button) {
                case 2:
                    pinput->SetMouseTrekKey(pinput->OnWheelClick(),pboolDown);
                    break;
                case 3:
                    pinput->SetMouseTrekKey(pinput->OnXButton1(),pboolDown);
                    break;
                case 4:
                    pinput->SetMouseTrekKey(pinput->OnXButton2(),pboolDown);
                    break;
                case 5:
                    pinput->SetMouseTrekKey(pinput->OnXButton3(),pboolDown);
                    break;
                case 6:
                    pinput->SetMouseTrekKey(pinput->OnXButton4(),pboolDown);
                    break;
                case 7:
                    pinput->SetMouseTrekKey(pinput->OnXButton5(),pboolDown);
                    break;
                case 8:
                    if (bDown) {
                        if (GetWindow()->CommandCamera(GetWindow()->GetCameraMode()) || !GetWindow()->NoCameraControl(GetWindow()->GetCameraMode()))
                            pinput->GetInputSite()->OnTrekKey((pinput->OnWheelDown() == TK_ZoomIn) ? TK_ZoomIn : TK_ZoomOut);
                        else if (GetWindow()->GetCameraMode() == TrekWindow::cmCockpit && trekClient.GetShip()->GetTurretID() != NA)
                            pinput->GetInputSite()->OnTrekKey((pinput->OnWheelDown() == TK_ThrottleDown) ? TK_ThrottleDown : TK_ThrottleUp);
                        else
                            pinput->GetInputSite()->OnTrekKey(pinput->OnWheelDown());
                    }
                    break;
                case 9:
                    if (bDown) {
                        if (GetWindow()->CommandCamera(GetWindow()->GetCameraMode()) || !GetWindow()->NoCameraControl(GetWindow()->GetCameraMode()))
                            pinput->GetInputSite()->OnTrekKey((pinput->OnWheelUp() == TK_ZoomOut) ? TK_ZoomOut : TK_ZoomIn);
                        else if (GetWindow()->GetCameraMode() == TrekWindow::cmCockpit && trekClient.GetShip()->GetTurretID() != NA)
                            pinput->GetInputSite()->OnTrekKey((pinput->OnWheelUp() == TK_ThrottleUp) ? TK_ThrottleUp : TK_ThrottleDown);
                        else
                            pinput->GetInputSite()->OnTrekKey(pinput->OnWheelUp());
                    }
                    break;

                default:
                    break;
            }

            return MouseResultRelease();
        }

        MouseResult rc = MouseResultHit();

        IclusterIGC*    pcluster = trekClient.GetCluster();
        if (pcluster && (m_csComposeState != c_csComposeShell))
        {
            if (bDown)
            {
                rc = MouseResultCapture();

                m_pointMouseStart = m_pointMouseStop = point;
                {
                    Camera* pcamera = GetWindow()->GetCamera();

                    Rect    rectImage = GetWindow()->GetViewport()->GetViewRect()->GetValue();
                    float   halfwidth = (0.5f * rectImage.XSize());

                    const Vector&   positionCamera = pcamera->GetPosition();

                    {
                        Point   pointStart = m_pointMouseStart - rectImage.Center();
                        Vector  directionStart = pcamera->TransformImageToDirection(pointStart / halfwidth);

                        //(pc - t * dp).z == 0, solve for t
                        float   t = positionCamera.z / directionStart.z;

                        m_positionStart = positionCamera - t * directionStart;
                    }
                }

                if (button == 1)
                    m_maskMouseState |= c_maskMouseRightDown;
                else if (button == 0)
                    m_maskMouseState |= c_maskMouseLeftDown;
            }
            else if ((m_maskMouseState & (c_maskMouseLeftDown << button)) != 0)
            {
                if (button == 1)
                {
                    m_maskMouseState &= ~c_maskMouseRightDown;

                    PickObject(pcluster, point, 1);
                }
                else if (button == 0)
                {
                    m_maskMouseState &= ~c_maskMouseLeftDown;

                    if (m_csComposeState == c_csComposeCommand)
                        PickObject(pcluster, point, 0);
                    else if (LegalRectangle(m_pointMouseStart, m_pointMouseStop))
                    {
                        ShipListIGC     ships;

                        float   xMin;
                        float   xMax;
                        if (m_pointMouseStart.X() < m_pointMouseStop.X())
                        {
                            xMin = m_pointMouseStart.X();
                            xMax = m_pointMouseStop.X();
                        }
                        else
                        {
                            xMin = m_pointMouseStop.X();
                            xMax = m_pointMouseStart.X();
                        }

                        float   yMin;
                        float   yMax;
                        if (m_pointMouseStart.Y() < m_pointMouseStop.Y())
                        {
                            yMin = m_pointMouseStart.Y();
                            yMax = m_pointMouseStop.Y();
                        }
                        else
                        {
                            yMin = m_pointMouseStop.Y();
                            yMax = m_pointMouseStart.Y();
                        }

                        Rect    rectImage = GetWindow()->GetViewport()->GetViewRect()->GetValue();
                        Point   pointCenter = rectImage.Center();

                        xMin -= pointCenter.X();
                        xMax -= pointCenter.X();
                        yMin -= pointCenter.Y();
                        yMax -= pointCenter.Y();

                        //Find all friendly ships in the selection rectangle
                        IsideIGC*   pside = trekClient.GetShip()->GetSide();
                        for (ShipLinkIGC*   psl = pcluster->GetShips()->first(); (psl != NULL); psl = psl->next())
                        {
                            IshipIGC*   pship = psl->data();
                            if ( (pship->GetSide() == pside || pside->AlliedSides(pside,pship->GetSide())) && pship->GetVisibleF() ) //ALLY - imago 7/3/09
                            {
                                ThingSite*  psite = pship->GetThingSite();
                                if (psite)
                                {
                                    const Point&    pxy = psite->GetScreenPosition();
                                    float           r = psite->GetScreenRadius();

                                    if ((pxy.X() + r >= xMin) && (pxy.X() - r <= xMax) &&
                                        (pxy.Y() + r >= yMin) && (pxy.Y() - r <= yMax))
                                    {
                                        ships.last(pship);
                                    }
                                }
                            }                
                        }

                        if (ships.n() != 0)
                        {
                            if (m_csComposeState == c_csNotComposing)
                                StartComposing(c_csComposeMouseCommand, &m_chsCommand);

                            m_pchsCurrent->SetRecipient(CHAT_GROUP, NA, NULL, &ships);
                            UpdateComposedChat();
                        }
                        else
                        {
                            m_pchsCurrent->SetRecipient(CHAT_NOSELECTION, NA, NULL);
                            QuitComposing();
                        }
                    }
                    else
                    {
                        //Picked a single point
                        IshipIGC*   pship = PickShip(pcluster, point);
                        if (pship)
                        {
                            //Got something ... what do we do with it?
                            if (m_csComposeState == c_csNotComposing)
                            {
                                //Wasn't composing ... start composing to the individual
                                StartComposing(c_csComposeMouseCommand, &m_chsCommand);
                                m_pchsCurrent->SetRecipient(CHAT_INDIVIDUAL, pship->GetObjectID(), pship);
                            }
                            else if (GetKeyState(VK_SHIFT) & 0x8000)
                            {
                                //shift click ... add/remove to the list
                                m_pchsCurrent->ToggleSubject(pship);
                            }
                            else
                            {
                                //normal click reset to the new recipient
                                m_pchsCurrent->SetRecipient(CHAT_INDIVIDUAL, pship->GetObjectID(), pship);
                            }
                            UpdateComposedChat();
                        }
                        else
                        {
                            //m_pchsCurrent->SetRecipient(CHAT_NOSELECTION, NA, NULL);      //NYI why are we doing this?
                            QuitComposing();
                        }
                    }
                }

                if (!m_maskMouseState)
                    rc = MouseResultRelease();
            }
        }

        return rc;
    }

    void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside)
    {
        m_pointMouseStop = point;
    }

    void MouseEnter(IInputProvider* pprovider, const Point& point)
    {
        m_bMouseIn = true;
    }

    void MouseLeave(IInputProvider* pprovider)
    {
        GetWindow()->SetCursor(AWF_CURSOR_DEFAULT);
        m_bMouseIn = false;
    }

    void RenderDropLine(Context* pcontext)
    {
        if ((((m_csComposeState != c_csComposeCommand) && (m_maskMouseState & c_maskMouseRightDown)) ||
             ((m_csComposeState == c_csComposeCommand) && m_maskMouseState &&
              ((m_cidVerb == c_cidGoto) || ((m_cidVerb == c_cidBuild) && m_pchsCurrent->ContainsLayer())))) &&
            LegalRectangle(m_pointMouseStart, m_pointMouseStop))
        {
            UpdatePick3D();

            TVector<VertexL>    vertices;
            TVector<WORD>       indices;

            const Color& color = m_positionStop.Z() > 0.0 ? CommandGeo::s_colorDropLineUp : CommandGeo::s_colorDropLineDown;

            indices.PushEnd(0);
            vertices.PushEnd(VertexL(m_positionStart.X(),
                                     m_positionStart.Y(),
                                     m_positionStart.Z(),
                                     color.R(),
                                     color.G(),
                                     color.B(),
                                     1,
                                     0,
                                     0));
            indices.PushEnd(1);
            vertices.PushEnd(VertexL(m_positionStop.X(),
                                     m_positionStop.Y(),
                                     m_positionStop.Z(),
                                     color.R(),
                                     color.G(),
                                     color.B(),
                                     1,
                                     0,
                                     0));

            pcontext->DrawLines(vertices, indices);
        }
    }
    bool DrawSelectionBox(void) const
    {
        return (m_maskMouseState & c_maskMouseLeftDown) && (m_csComposeState != c_csComposeCommand) &&
                LegalRectangle(m_pointMouseStart, m_pointMouseStop);
    }

    void Render(Context* pcontext)
    {
        if (m_bMouseIn)
        {
            IclusterIGC*    pcluster = trekClient.GetCluster();
            if (pcluster)
            {
                if (DrawSelectionBox())
                {
                    pcontext->DrawRectangle(Rect(m_pointMouseStart, m_pointMouseStop), Color::White());
                }

                //Is the mouse over a legitimate target
                const char* pszCursor = AWF_CURSOR_DEFAULT;

                {
                    ImodelIGC*  pmodelPick = ResolvePick(pcluster, m_pointMouseStop, -1);
                    if (pmodelPick)
                    {
                        IsideIGC*   psidePick = pmodelPick->GetSide();
                        bool        bFriendly = ( (psidePick == trekClient.GetSide()) || (psidePick->AlliedSides(psidePick,trekClient.GetSide())) ); //ALLY - imaog 7/3/09

                        if (bFriendly && (pmodelPick->GetObjectType() == OT_ship))
                        {
                            //Over a friendly ship: always show the select cursor
                            pszCursor = AWF_CURSOR_SELECT;
                        }
                        else
                        {
                            CommandID   cid;

                            if (m_csComposeState == c_csComposeCommand)
                            {
                                //Composing a command ... always show the selected order if legal
                                assert (m_pchsCurrent->LegalCommand(m_cidVerb, pmodelPick));
                                cid = m_cidVerb;
                            }
                            else if (m_pchsCurrent->m_ctRecipient == CHAT_INDIVIDUAL)
                            {
                                //Pick an appropriate default based on the recipient and the object
                                IshipIGC*   pship = (IshipIGC*)(IbaseIGC*)(m_pchsCurrent->m_pbaseRecipient);

                                cid = pship->GetDefaultOrder(pmodelPick);
                            }
                            else
                                cid = c_cidDefault;

                            if (cid >= c_cidDefault)
                                pszCursor = c_cdAllCommands[cid].szVerb;
                            else
                                pszCursor = AWF_CURSOR_DEFAULT;
                        }

                        // show object name
                        TRef<IEngineFont> pfont = TrekResources::SmallFont();
                        float   w = (float)(pfont->GetTextExtent(GetModelName(pmodelPick)).X());
                        Point   offset = Point(-w / 2, -8 - pfont->GetHeight());
                        pcontext->DrawString(pfont, (psidePick ? psidePick->GetColor() : Color::White()), m_pointMouseStop + offset, ZString(GetModelName(pmodelPick)));
                    }
                }

                GetWindow()->SetCursor(pszCursor);
            }
        }
    }

    bool IsTakingKeystrokes()
    {
        return (m_csComposeState > c_csComposeMouseCommand);
    }

    const ShipListIGC*  GetSubjects(void) const
    {
        return (m_csComposeState != c_csNotComposing) &&
               (m_csComposeState != c_csComposeShell)
               ? &(m_pchsCurrent->m_shipsSubject)
               : NULL;
    }

    bool OnChar(const KeyState& ks)
    {
        if (m_csComposeState > c_csComposeMouseCommand)
        {
            trekClient.PlaySoundEffect(chatKeySound);

            if (ks.bControl && (m_csComposeState != c_csComposeCommand) && m_strTypedText.IsEmpty())
            {
                //Lets assume the 1st char of all commands are unique
                for (CommandID  cid = c_cidAttack; (cid < c_cidMax); cid++)
                {
                    if (ks.vk == (c_cdAllCommands[cid].szVerb[0] - 96))
                    {
                        SetComposeState(c_csComposeCommand);
                        SetVerb(cid);

                        SetChatObject(NULL);

                        UpdateComposedChat();

                        break;
                    }
                }
            }
            else
            {
                switch (ks.vk)
                {
                    case VK_RETURN:
                        if (m_strTypedText.IsEmpty())
                        {
                            QuitComposing();
                        }
                        else
                        {
                            SendChat();
                        }
                    break;

                    case VK_ESCAPE:  //NYI Imago save the typed text to a buffer as noted 7/4/09
                        if (m_csComposeState == c_csComposeCommand)
                        {
                            m_strTypedText.SetEmpty();
                            SetComposeState(c_csComposeChat);
                            UpdateComposedChat();
                        }
                        else
                            QuitComposing();
                    break;

                    case VK_BACK:
                        OnBackspace();
                    break;

                    case VK_TAB:
                        OnTab();
                    break;

                    case '~':
                    case '`':
                        QuitComposing();
                        GetWindow()->OpenMainQuickChatMenu();
                    break;


                    case '!':
                    {
                        if ((m_csComposeState == c_csComposeChat) && m_strTypedText.IsEmpty())
                        {
                            m_strTypedText = ZString("!");
                            SetComposeState(c_csComposeShell);
                            UpdateComposedChat();
                            break;
                        }
                    }
                    //intentional no break
                    default:
                        OnPrintable(ks.vk);
                        UpdateComposedChat();
                    break;
                }
            }
        }

        return true;
    }
    
    void StartChat()
    {
        trekClient.PlaySoundEffect(chatKeySound);
        if (m_csComposeState == c_csNotComposing)
            StartComposing(c_csComposeChat, &m_chsChat);
        else
            SetComposeState(c_csComposeChat);

        UpdateComposedChat();
    }

    int MatchName(const char*   pszText, const char* pszName)
    {
        assert (*pszText != '\0');

        while (true)
        {
            if (*pszText == '\0')
            {
                //Ran out of typed text ... return the number of unmatched characters
                //in pszName
                return strlen(pszName);
            }
            else if (tolower(*pszText) != tolower(*pszName))
            {
                //No match
                return c_badMatch;
            }

            pszText++;
            pszName++;
        }
    }

    void    SetChatObject(ImodelIGC*    pmodel)
    {
        if (m_pmodelTarget && (m_pmodelTarget->GetObjectType() == OT_buoy))
            ((IbuoyIGC*)(ImodelIGC*)m_pmodelTarget)->ReleaseConsumer();

        m_pmodelTarget = pmodel;

        if (pmodel && (pmodel->GetObjectType() == OT_buoy))
            ((IbuoyIGC*)pmodel)->AddConsumer();
    }

    void MatchTarget(void)
    {
        SetChatObject(NULL);

        const char* pszText = m_strTypedText;
        if (*pszText == '\0')
            return;

        ImodelIGC*  pmodelBest;
        int         scoreBest = c_badMatch;  //Any nice big number

        //Find an acceptable thing to target
        //First ... try all ships in the universe
        {
            for (ShipLinkIGC*   psl = trekClient.m_pCoreIGC->GetShips()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IshipIGC*   pship = psl->data();
                if (m_pchsCurrent->LegalCommand(m_cidVerb, pship))
                {
					if (pship->GetSide() == trekClient.GetSide() ||
						  pship->SeenBySide(trekClient.GetSide()))
					{ //Xynth #209 8/2010 make sure I should know about this
						int score = MatchName(pszText, pship->GetName());
						if (score < scoreBest)
						{
							pmodelBest = pship;
							scoreBest = score;
						}
					}
                }
            }
        }

        //Second ... try all known stations
        if (scoreBest > 0)
        {
            for (StationLinkIGC*   psl = trekClient.m_pCoreIGC->GetStations()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IstationIGC*   pstation = psl->data();
                if (m_pchsCurrent->LegalCommand(m_cidVerb, pstation))
                {
					if (pstation->GetSide() == trekClient.GetSide() ||
						  pstation->SeenBySide(trekClient.GetSide()))
					{ //Xynth #209 8/2010 make sure I should know about this
						int score = MatchName(pszText, pstation->GetName());
						if (score < scoreBest)
						{
							pmodelBest = pstation;
							scoreBest = score;
						}
					}
                }
            }

            if (scoreBest > 0)
            {
                //Third ... try anything in the cluster
                IclusterIGC*    pcluster = trekClient.GetCluster();
                if (pcluster)
                {
                    for (ModelLinkIGC*   pml = pcluster->GetPickableModels()->first();
                         (pml != NULL);
                         pml = pml->next())
                    {
                        ImodelIGC*   pmodel = pml->data();
                        if (m_pchsCurrent->LegalCommand(m_cidVerb, pmodel))
                        {
                            if (pmodel->GetSide() == trekClient.GetSide() ||
								pmodel->SeenBySide(trekClient.GetSide()))
							{ //Xynth #209 8/2010 make sure I should know about this
								int score = MatchName(pszText, GetModelName(pmodel));
								if (score < scoreBest)
								{
									pmodelBest = pmodel;
									scoreBest = score;
								}
							}
                        }
                    }
                }

                if (scoreBest > 0)
                {
                    //Fourth ... try me
                    if (m_pchsCurrent->LegalCommand(m_cidVerb, trekClient.GetShip()))
                    {
                        int score = MatchName(pszText, "me");
                        if (score < scoreBest)
                        {
                            pmodelBest = trekClient.GetShip();
                            scoreBest = score;
                        }
                    }

                    if (scoreBest > 0)
                    {
                        //Fifth ... try all clusters
                        IclusterIGC*    pcluster = NULL;
                        for (ClusterLinkIGC*   pcl = trekClient.m_pCoreIGC->GetClusters()->first();
                             (pcl != NULL);
                             pcl = pcl->next())
                        {
                            IclusterIGC*   pc = pcl->data();
                            int score = MatchName(pszText, pc->GetName());
                            if (score < scoreBest)
                            {
                                pcluster = pc;
                                scoreBest = score;
                            }
                        }

                        if (pcluster)
                        {
                            // create a cluster buoy in that sector
                            DataBuoyIGC db;

                            db.position = Vector(0, 0, 0);
                            db.clusterID = pcluster->GetObjectID();
                            db.type = c_buoyCluster;

                            pmodelBest = (IbuoyIGC*)(trekClient.m_pCoreIGC->CreateObject(trekClient.m_now, OT_buoy, &db, sizeof(db)));
                            assert (pmodelBest);

                            pmodelBest->Release();
                        }
                    }
                }
            }
        }

        if (scoreBest != c_badMatch)
            SetChatObject(pmodelBest);
    }

    void OnBackspace()
    {
        assert (m_csComposeState > c_csComposeMouseCommand);

        if (m_strTypedText.GetLength() != 0)
        {
            if ((m_csComposeState == c_csComposeShell) && (m_strTypedText.GetLength() == 1))
            {
                m_strTypedText.SetEmpty();
                SetComposeState(c_csComposeChat);
            }
            else
            {
                m_strTypedText = m_strTypedText.LeftOf(1);
                if (m_csComposeState == c_csComposeCommand)
                    MatchTarget();
            }
            UpdateComposedChat();                
        }
        else if (m_csComposeState == c_csComposeCommand)
        {
            SetComposeState(c_csComposeChat);
            SetChatObject(NULL);
        }
    }

    void CycleChatTarget(void)
    {


        if (m_csComposeState != c_csNotComposing)
        {

			//Imago 7/4/09 ALLY
			//OutputDebugString("In CycleChatTarget() switch m_pchsCurrent->m_ctRecipient="+ZString(m_pchsCurrent->m_ctRecipient)+"\n");
            if (trekClient.GetSide()->GetAllies() == NA) {
				switch (m_pchsCurrent->m_ctRecipient)
	            {
	                case CHAT_WING:
	                {
	                    if (m_csComposeState != c_csComposeChat)
	                    {
	                        m_pchsCurrent->SetRecipient(CHAT_INDIVIDUAL, NA, trekClient.GetShip());
	                        break;
	                    }
	                }
	                //no break ... cant compose a chat to yourself so fall through to 

	                case CHAT_INDIVIDUAL:
	                {
	                    if (trekClient.GetShip()->GetSourceShip()->GetChildShips()->n() != 0)
	                    {
	                        m_pchsCurrent->SetRecipient(CHAT_SHIP, NA, NULL);
	                        break;
	                    }
	                }
	                //no break ... alone in ship

					case CHAT_SHIP:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_FRIENDLY_SECTOR, NA, NULL);
	                    if (m_pchsCurrent->m_shipsSubject.n() != 0)
	                        break;
	                }
					
					//no break ... no one in the sector, so skip over it.

	                case CHAT_FRIENDLY_SECTOR:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_TEAM, NA, NULL);
	                }
	                break;
	   
	                case CHAT_TEAM:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_EVERYONE, NA, NULL);
	                }
	                break;      

	                default:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_WING, NA, NULL);
	                }
	            }
			} else { //no ALLY
				switch (m_pchsCurrent->m_ctRecipient)
	            {
					case CHAT_ALLIES:
					{
						m_pchsCurrent->SetRecipient(CHAT_WING, NA, NULL);
					}
					break;
	                case CHAT_WING:
	                {
	                    if (m_csComposeState != c_csComposeChat)
	                    {
	                        m_pchsCurrent->SetRecipient(CHAT_INDIVIDUAL, NA, trekClient.GetShip());
	                        break;
	                    }
	                }
	                //no break ... cant compose a chat to yourself so fall through to 

	                case CHAT_INDIVIDUAL:
	                {
	                    if (trekClient.GetShip()->GetSourceShip()->GetChildShips()->n() != 0)
	                    {
	                        m_pchsCurrent->SetRecipient(CHAT_SHIP, NA, NULL);
	                        break;
	                    }
	                }
	                //no break ... alone in ship

					case CHAT_SHIP:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_FRIENDLY_SECTOR, NA, NULL);
	                    if (m_pchsCurrent->m_shipsSubject.n() != 0)
	                        break;
	                }
					
					//no break ... no one in the sector, so skip over it.

	                case CHAT_FRIENDLY_SECTOR:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_TEAM, NA, NULL);
	                }
	                break;
	   
	                case CHAT_TEAM:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_EVERYONE, NA, NULL);
	                }
	                break;      

	                default:
	                {
	                    m_pchsCurrent->SetRecipient(CHAT_ALLIES, NA, NULL);
	                }
	            }
			}
        }
    }

    void OnTab()
    {
        assert (m_csComposeState > c_csComposeMouseCommand);
        const char* pcc = m_strTypedText;

        if ((*pcc == '\0') || (*pcc == ' '))
        {
            CycleChatTarget();
        }
        else
        {
            //Try to match the typed text against a recipient
            if (MatchName(pcc, "admin") == 0)
            {
                SetChatObject(NULL);
                m_strTypedText.SetEmpty();
                m_pchsCurrent->SetRecipient(CHAT_ADMIN, NA, NULL);
            }
            else
            {
                int         scoreBest = c_badMatch;
				//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
                ChatTarget    ct = CHAT_NOSELECTION;
                ObjectID      oidRecipient;
                IbaseIGC*     pbaseRecipient;

                //Try to match against a wing
                {
                    for (WingID   i = 0;
                         (i < c_widMax);
                         i++)
                    {
                        int score = MatchName(pcc, c_pszWingName[i]);
                        if (score < scoreBest)
                        {
                            scoreBest = score;
							//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");

                            ct = CHAT_WING;
                            oidRecipient = i;
                            pbaseRecipient = NULL;
                        }
                    }
                }

                if (scoreBest > 0)
                {
                    for (ClusterLinkIGC*   pcl = trekClient.m_pCoreIGC->GetClusters()->first();
                         (pcl != NULL);
                         pcl = pcl->next())
                    {
                        IclusterIGC*   pcluster = pcl->data();

                        int score = MatchName(pcc, pcluster->GetName());
                        if (score < scoreBest)
                        {
                            scoreBest = score;
							//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
                            ct = CHAT_FRIENDLY_SECTOR;
                            oidRecipient = pcluster->GetObjectID();
                            pbaseRecipient = pcluster;
                        }
                    }

                    if (scoreBest > 0)
                    {
                        //Try to match against your team
                        IsideIGC*   pside = trekClient.GetSide();

                        int score = MatchName(pcc, pside->GetName());
                        if (score < scoreBest)
                        {
                            scoreBest = score;
							//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
                            ct = CHAT_TEAM;
                            oidRecipient = pside->GetObjectID();
                            pbaseRecipient = pside;
                        }

                        if (scoreBest > 0)
                        {
                            int score = MatchName(pcc, "me");
                            if (score < scoreBest)
                            {
                                scoreBest = score;
								//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
                                ct = CHAT_INDIVIDUAL;
                                oidRecipient = trekClient.GetShipID();
                                pbaseRecipient = trekClient.GetShip();
                            }
                            if (scoreBest > 0)
                            {
                                int score = MatchName(pcc, "all");
								ZString allstr = ZString(pcc).RightOf("all");
                                if (score < scoreBest)
                                {
                                    scoreBest = score;
									//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
                                    ct = CHAT_EVERYONE;
                                    oidRecipient = NA;
                                    pbaseRecipient = NULL;
                                }

                                if (scoreBest > 0)
                                {
                                    if (trekClient.GetShip()->GetSourceShip()->GetChildShips()->n() != 0)
                                    {
                                        int score = MatchName(pcc, "ship");
                                        if (score < scoreBest)
                                        {
                                            scoreBest = score;
											//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
                                            ct = CHAT_SHIP;
                                            oidRecipient = NA;
                                            pbaseRecipient = NULL;
                                        }
                                    }
		                            if (scoreBest > 0)  //imago ALLY 7/4/09
		                            {
										if (trekClient.GetSide()->GetAllies() != NA) {
		                               		int score = MatchName(pcc, "allies");
			                                if (score < scoreBest)
			                                {
			                                    scoreBest = score;
												//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
			                                    ct = CHAT_ALLIES;
                           						oidRecipient = pside->GetObjectID();
                            					pbaseRecipient = pside;
			                                }
										}
	                                    if (scoreBest > 0)
										{
											if (trekClient.GetSide()->GetAllies() != NA) //Imago 7/6/09 ALLY
											{ //allied way, try to match our side AND allied sides
		                                        for (SideLinkIGC*  palliedside = trekClient.GetCore()->GetSides()->first();
		                                             (palliedside != NULL);
		                                             palliedside = palliedside->next())
		                                        {
		                                            IsideIGC*   pside = palliedside->data();
													if (pside->AlliedSides(pside,trekClient.GetSide()) 
														|| pside->GetObjectID() == trekClient.GetSideID()) {

				                                        for (ShipLinkIGC*   psl = pside->GetShips()->first();
				                                             (psl != NULL);
				                                             psl = psl->next())
				                                        {
															IshipIGC*   pship = psl->data();
				                                            int score = MatchName(pcc, pship->GetName());
				                                            if (score < scoreBest)
				                                            {
				                                                scoreBest = score;
																//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
				                                                ct = CHAT_INDIVIDUAL;
				                                                oidRecipient = pship->GetObjectID();
				                                                pbaseRecipient = pship;
				                                            }
														}
													}
		                                        }
											} else { //non-allied way
	                                        	//Try to match against an individual on our side
	                                        	IsideIGC*   pside = trekClient.GetSide();
		                                        for (ShipLinkIGC*   psl = pside->GetShips()->first();
		                                             (psl != NULL);
		                                             psl = psl->next())
		                                        {
		                                            IshipIGC*   pship = psl->data();

		                                            int score = MatchName(pcc, pship->GetName());
		                                            if (score < scoreBest)
		                                            {
		                                                scoreBest = score;
														//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
		                                                ct = CHAT_INDIVIDUAL;
		                                                oidRecipient = pship->GetObjectID();
		                                                pbaseRecipient = pship;
		                                            }
		                                        }
											}

	                                        // try to match against everyone else
	                                        if (scoreBest > 0)
	                                        {
	                                            IsideIGC* psideMine = trekClient.GetSide();

	                                            for (ShipLinkIGC*   psl = trekClient.m_pCoreIGC->GetShips()->first();
	                                                 (psl != NULL);
	                                                 psl = psl->next())
	                                            {
	                                                IshipIGC*   pship = psl->data();

													// WLP 2005 - Only talk to enemy pilots - not cons
													// I added the pilot check to enforce this
													//
	                                                if ((pship->GetSide() != psideMine)&& (pship->GetPilotType()== c_ptPlayer ))
	                                                {
	                                                    int score = MatchName(pcc, pship->GetName());
	                                                    if ((ct == CHAT_INDIVIDUAL)
	                                                        ? (score == 0)
	                                                        : (score < scoreBest))
	                                                    {
	                                                        scoreBest = score;
															//OutputDebugString("In OnTab() scoreBest= "+ZString(scoreBest)+"\n");
	                                                        ct = CHAT_INDIVIDUAL;
	                                                        oidRecipient = pship->GetObjectID();
	                                                        pbaseRecipient = pship;
	                                                    }
	                                                }
	                                            }
											}

	                                        if (scoreBest > 0)
	                                        {
	                                            //Did not have a perfect match ... try a verb
	                                            //Try to match against one of the verbs (but don't match default)
	                                            for (CommandID  cid = c_cidAttack; (cid < c_cidMax); cid++)
	                                            {
	                                                if (MatchName(pcc, c_cdAllCommands[cid].szVerb) == 0)
	                                                {
	                                                    SetComposeState(c_csComposeCommand);
	                                                    SetVerb(cid);
														//OutputDebugString("In OnTab() Calling SetVerb() and returning scoreBest= "+ZString(scoreBest)+"\n");
	                                                    SetChatObject(NULL);
	                                                    m_strTypedText.SetEmpty();
	                                                    UpdateComposedChat();

	                                                    return;
	                                                }
	                                            }
	                                        }
	                                    }
	                                }
	                            }
	                        }
	                    }
					}
				}

                if (scoreBest != c_badMatch)
                {
                    SetChatObject(NULL);
                    m_strTypedText.SetEmpty();
                    m_pchsCurrent->SetRecipient(ct, oidRecipient, pbaseRecipient);
					//OutputDebugString("In OnTab() Calling SetRecipient() scoreBest= "+ZString(scoreBest)+"\n");
                }
                else
                {
                    CycleChatTarget();
					//OutputDebugString("In OnTab() Calling CylceTarget() scoreBest= "+ZString(scoreBest)+"\n");
                }
            }
        }
        UpdateComposedChat();
		//OutputDebugString("UpdateCompsedChat returning OnTab()\n");
    }

    void OnPrintable(char ch)
    {
        assert (m_csComposeState > c_csComposeMouseCommand);
		
		// yp your_persona march 24 2005: constrain chat messages to always fit in the buffer size 255
		if (m_strTypedText.GetLength() < 255)
		{
			m_strTypedText += ZString(ch, 1);
		}   

        if (m_csComposeState == c_csComposeCommand)
        {
            MatchTarget();
        }
    }
};


//////////////////////////////////////////////////////////////////////////////
//
// DisplayImage
//
//////////////////////////////////////////////////////////////////////////////

class DisplayImage : public WrapImage, public IEventSink
{
protected:
    TRef<TransformImage>                m_pTransformImage;
    TRef<AnimatedTranslateTransform2>   m_pTransform;
    TRef<IEventSink>                    m_peventSinkOffScreen;

    bool                                m_bVisible;     
    bool                                m_bUndetectable;
    OverlayMask                         m_omVisible;
    
protected:
    Number* GetTime() 
    { 
        return Number::Cast(GetChild(1)); 
    }

public:
    DisplayImage(
        Number*      ptime,
        RectValue*   prect,
        Image*       pimage,
        int          side,
        const Point& ptOffScreen,
        const Point& ptOnScreen,
        float        dtimeSlide,
        OverlayMask  omVisible,
        bool         bUndetectable
    ) :
        WrapImage(Image::GetEmpty(), ptime),
        m_bVisible(false),
        m_omVisible(omVisible),
        m_bUndetectable(bUndetectable)
    {
        m_peventSinkOffScreen = IEventSink::CreateDelegate(this);

        m_pTransformImage = new TransformImage(
            pimage,
            m_pTransform =
                AnimatedTranslateTransform2::Create(
                    ptime,
                    prect,
                    side,
                    ptOffScreen,
                    ptOnScreen,
                    dtimeSlide
                )
        );

        m_pTransform->AddSink(m_peventSinkOffScreen);
    }

    ~DisplayImage(void)
    {
        m_pTransform->RemoveSink(m_peventSinkOffScreen);
    }

    void SetOverlayFlags(OverlayMask om)
    {
        Show( (om & m_omVisible) != 0);
    }

    void Show(bool bVisible)
    {
        if (m_bVisible != bVisible)
        {
            if (bVisible)
            {
                SetImage(m_pTransformImage);
                m_pTransform->MoveEnd();
            }
            else
            {
                //NYI need ome way to tell the image that it no longer needs to be updated
                //(e.g. animate a static image moving off screen).
                m_pTransform->MoveStart();
            }

            m_bVisible = bVisible;
        }
    }

    bool OnEvent(IEventSource* pevent)
    {
        SetImage(Image::GetEmpty());
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IMouseInput methods
    //
    //////////////////////////////////////////////////////////////////////////////

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    {
        MouseResult result = WrapImage::HitTest(pprovider, point, bCaptured);

        if (!m_bVisible || m_bUndetectable) {
            result.Clear(MouseResultHit());
        }

        return result;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// ConsolePickImage
//
//////////////////////////////////////////////////////////////////////////////

ConsolePickImage::ConsolePickImage(Viewport*         pviewport,
                     ConsoleDataImpl*  pconsoleData
                    )
:
    Image(),
    m_pviewport(pviewport),
    m_pconsoleData(pconsoleData),
    m_mouseresult(MouseResultHit())
{
    assert (m_pconsoleData);
}

void        ConsolePickImage::SetHitTest(MouseResult   mr)
{
    m_mouseresult = mr;
}

MouseResult ConsolePickImage::HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
{
    return m_mouseresult; 
}

MouseResult ConsolePickImage::Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
{
    /*
    const Rect& rect = m_pviewport->GetViewRect()->GetValue();

    Vector direction =
        m_pviewport->GetCamera()->TransformImageToDirection(
            (point - rect.Center()) * (2.0f / rect.XSize())
        );
    */

    return m_pconsoleData->Button(pprovider, point, button, bCaptured, bInside, bDown);
}

void ConsolePickImage::MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside)
{
    m_pconsoleData->MouseMove(pprovider, point, bCaptured, bInside);
}

void ConsolePickImage::MouseEnter(IInputProvider* pprovider, const Point& point)
{
    m_pconsoleData->MouseEnter(pprovider, point);
}

void ConsolePickImage::MouseLeave(IInputProvider* pprovider)
{
    m_pconsoleData->MouseLeave(pprovider);
}

//////////////////////////////////////////////////////////////////////////////
//
// ConsoleImage
//
//////////////////////////////////////////////////////////////////////////////

class ConsoleImageImpl : public ConsoleImage 
{
private:
    TRef<ConsoleDataImpl>        m_pconsoleData;
    TVector<TRef<DisplayImage> > m_vpdisplayImages;
    TRef<GroupImage>             m_pgroupDisplays;
    TRef<IObject>                m_pobjectTeamPane;
    TRef<IObject>                m_pobjectExpandedTeamPane;
    TRef<IObject>                m_pobjectTeleportPane;
    TRef<GameStateContainer>     m_pgsc;
    TRef<ComboPane>              m_pcomboCluster;
    TRef<ButtonPane>             m_pbuttonBack;
    TRef<ConsolePickImage>              m_pickimage;

    char                         m_szFileName[c_cbFileName];
    

public:
    ConsoleImageImpl(Engine* pengine, Viewport* pviewport) :
        ConsoleImage(pengine, pviewport)
    {
        m_szFileName[0] = '\0';
        SetDisplayMDL("dialog");
    }

    ~ConsoleImageImpl(void)
    {
        GetWindow()->SetChatListPane(NULL);
    }

    void StartChat()
    {
        m_pconsoleData->StartChat();
    }
    void StartQuickChat(ChatTarget  ctRecipient,
                        ObjectID    oidRecipient,
                        IbaseIGC*   pbaseRecipient)
    {
        m_pconsoleData->StartQuickChat(ctRecipient, oidRecipient, pbaseRecipient);
    }
    void SendQuickChat(int idSonicChat, int ttMask, CommandID cid, AbilityBitMask aabm)
    {
        m_pconsoleData->SendQuickChat(idSonicChat, ttMask, cid, aabm);
    }
    void    QuitComposing(void)
    {
        m_pconsoleData->QuitComposing();
    }

    void RenderDropLine(Context* pcontext)
    {
        if (m_pconsoleData)
            m_pconsoleData->RenderDropLine(pcontext);
    }

    bool DrawSelectionBox(void) const
    {
        return m_pconsoleData ? m_pconsoleData->DrawSelectionBox() : false;
    }

    void Render(Context* pcontext)
    {
        pcontext->PushState();
        ConsoleImage::Render(pcontext);
        pcontext->PopState();

        if (m_pconsoleData)
            m_pconsoleData->Render(pcontext);
    }
    void Update(Time now)
    {
        m_pconsoleData->Update(now);
    }

    void SetOverlayFlags(OverlayMask om)
    {
        if (GetOverlayFlags() != om)
        {
            trekClient.PlaySoundEffect(paneSlideSound);

            switch (GetWindow()->GetViewMode())
            {
            case TrekWindow::vmHangar:
            case TrekWindow::vmLoadout:
            case TrekWindow::vmOverride:
                m_pickimage->SetHitTest(MouseResult());
                break;
                
            default:
                m_pickimage->SetHitTest(MouseResultHit());
                break;
            }
        }

        for (short i = 0; i < m_vpdisplayImages.GetCount(); i++)
            m_vpdisplayImages[i]->SetOverlayFlags(om);
        m_pconsoleData->SetOverlayFlags(om);
    }

    OverlayMask GetOverlayFlags(void) const
    {
        return m_pconsoleData ? m_pconsoleData->GetOverlayFlags() : 0;
    }

    void OnSwitchViewMode()
    {
        m_pconsoleData->OnSwitchViewMode();
    }

    ConsoleData* GetConsoleData()
    {
        return m_pconsoleData;
    }


    bool IsComposing()
    {
        return m_pconsoleData->IsTakingKeystrokes();
    }
    
    bool OnChar(const KeyState& ks)
    {
        return m_pconsoleData->OnChar(ks);
    }

    const ShipListIGC*  GetSubjects(void) const
    {
        return m_pconsoleData->GetSubjects();
    }

    void                CycleChatTarget(void)
    {
        m_pconsoleData->CycleChatTarget();
        m_pconsoleData->UpdateComposedChat();
    }

    void                SetInRange(bool bInRange)
    {
        m_pconsoleData->SetInRange(bInRange);
    }

    void SetDisplayMDL(const char* pszFileName)
    {
        if (strcmp(pszFileName, m_szFileName) == 0)
            return;

        assert (strlen(pszFileName) < c_cbFileName);
        strcpy(m_szFileName, pszFileName);

        OverlayMask om = GetOverlayFlags();

        m_pconsoleData            = NULL;
        m_vpdisplayImages         = NULL;
        m_pgroupDisplays          = NULL;
        m_pobjectTeamPane         = NULL;
        m_pobjectExpandedTeamPane = NULL;
        m_pobjectTeleportPane     = NULL;
        m_pcomboCluster           = NULL;
        m_pbuttonBack             = NULL;
        m_pickimage               = NULL;

        m_pgroupDisplays = new GroupImage();

		// BUILD_DX9
		GetModeler()->SetColorKeyHint( true );

		
		m_pconsoleData   = new ConsoleDataImpl(this->GetViewport(), pszFileName);

        TRef<INameSpace> pnsDisplays = m_pconsoleData->GetNameSpace();

        //
        // Get Game The State Container
        //

        CastTo(m_pgsc, pnsDisplays->FindMember("gameStateContainer"));

        //
        // load dialog.mdl which contains the hud dialog descriptions
        //

        // back button

        CastTo(m_pbuttonBack, pnsDisplays->FindMember("backButtonPane"));
// VS.Net 2003 port: error C2465 here.
#if _MSC_VER >= 1310
		m_pconsoleData->AddEventTarget(&ConsoleDataImpl::OnBackButton, m_pbuttonBack->GetEventSource());
#else
        m_pconsoleData->AddEventTarget(m_pconsoleData->OnBackButton, m_pbuttonBack->GetEventSource());
#endif
        
        //
        // Add all of the loaded displays
        //

        IObjectList* plistDisplays; CastTo(plistDisplays, pnsDisplays->FindMember("dialogs"));

        plistDisplays->GetFirst();
        while (plistDisplays->GetCurrent() != NULL)
        {
            TRef<IObjectPair> ppair; CastTo(ppair, plistDisplays->GetCurrent());

            TRef<Image>      pimage;               CastTo(pimage, (Value*)ppair->GetNth(0));
            int              side                 =         (int)GetNumber(ppair->GetNth(1));
            TRef<PointValue> ppointOffScreen      =       PointValue::Cast(ppair->GetNth(2));
            TRef<PointValue> ppointOnScreen       =       PointValue::Cast(ppair->GetNth(3));
            float            dtimeSlide           =              GetNumber(ppair->GetNth(4));
            OverlayMask      cmVisible            = (OverlayMask)GetNumber(ppair->GetNth(5));
            TRef<Boolean>    pbooleanUndetectable =          Boolean::Cast(ppair->GetLastNth(6));

            CreateDisplayImage(
                pimage,
                side,
                ppointOffScreen->GetValue(),
                ppointOnScreen->GetValue(),
                dtimeSlide,
                cmVisible,
                pbooleanUndetectable->GetValue()
            );

            plistDisplays->GetNext();
        }

        //
        // Hook up the team pane
        //

        m_pobjectTeleportPane = CreateTeleportPane(GetModeler());
        m_pobjectTeamPane = CreateTeamPane(GetModeler());
        m_pobjectExpandedTeamPane = CreateExpandedTeamPane(GetModeler(), m_pobjectTeamPane);

        SetImage(m_pgroupDisplays);

        SetOverlayFlags(om);

        //Needs to go last so that it only captures mouse clicks missed by anything else
        m_pickimage = new ConsolePickImage(this->GetViewport(), m_pconsoleData);
        m_pgroupDisplays->AddImage(m_pickimage);

        //
        // Tell the TrekWindow about the chat pane
        //

        ChatListPane* pchatListPane;
        CastTo(pchatListPane, (Pane*)pnsDisplays->FindMember("chatListPane"));

        GetWindow()->SetChatListPane(pchatListPane);

        // unload the namespace so that the displays will go away when we are 
        // finished with them.

        GetModeler()->UnloadNameSpace(pszFileName);
    }

private:

    TRef<GameStateContainer> GetGameStateContainer()
    {
        return m_pgsc;
    }

    TRef<Value> CreateDisplayImage(
              Image*      pimage,
              int         side,
        const Point&      ptOffScreen,
        const Point&      ptOnScreen,
              float       dtimeSlide,
              OverlayMask omVisible,
              bool        bUndetectable
    ) {
        TRef<DisplayImage> pdisplayImage =
            new DisplayImage(
                GetWindow()->GetTime(),
                GetWindow()->GetScreenRectValue(),
                pimage,
                side,
                ptOffScreen,
                ptOnScreen,
                dtimeSlide,
                omVisible,
                bUndetectable
            );

        m_vpdisplayImages.PushEnd(pdisplayImage);
        m_pgroupDisplays->AddImage(pdisplayImage);
        return pdisplayImage;
    }
};

TRef<ConsoleImage> ConsoleImage::Create(Engine* pengine, Viewport* pviewport)
{
    return new ConsoleImageImpl(pengine, pviewport);
}

void AddChildren(Pane* ppaneParent, ObjectStack& stack)
{
    TRef<IObjectList> plist; CastTo(plist, (IObject*)stack.Pop());

    plist->GetFirst();

    while (plist->GetCurrent() != NULL) {
        IObjectPair*      ppair;       CastTo(ppair,       plist->GetCurrent());
        TRef<Pane>        ppaneChild;  CastTo(ppaneChild,  ppair->GetFirst()  );
        TRef<PointValue>  ppointChild; CastTo(ppointChild, ppair->GetSecond() );

        ppaneParent->InsertAtBottom(ppaneChild);
        ppaneChild->SetOffset(
            WinPoint(
                (int)ppointChild->GetValue().X(),
                (int)ppointChild->GetValue().Y()
            )
        );

        plist->GetNext();
    }
}
