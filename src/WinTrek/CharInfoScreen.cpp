#include "pch.h"

#include <button.h>
#include <controls.h>

class CharInfoScreen;
extern CharInfoScreen * g_pCharInfoScreen; // messy but avoids .h file
extern bool    g_bDisableZoneClub;
extern bool    g_bDisableNewCivs;


class IZoneMedal;


typedef TListListWrapper<TRef<IZoneMedal> >    XZoneMedals;


// #define NO_CLUB_SERVER_CONNECTION 1 // comment out before checkin

/////////////////////////////////////////////////////////////////////////////////////////////////

class CCharacterStats
{
public:

    CCharacterStats() :
        m_bInfoReceived(false),
        m_nPromotionAt(0),
        m_fScore(0.0f),
        m_nCombatRating(0),
        m_cBaseKills(0),
        m_cBaseCaptures(0),
        m_cKills(0),
        m_cDeaths(0),
        m_cWins(0),
        m_cLosses(0),
        m_nAchievementPoints(0),
        m_nRank(NA),
        m_cMinutesPlayed(0)
    {
    }

    bool m_bInfoReceived;
    // stats
    float m_fScore;
    int m_nPromotionAt;
    int m_nCombatRating;
    int m_nAchievementPoints;
    int m_cBaseKills;
    int m_cBaseCaptures;
    int m_cKills;
    int m_cDeaths;
    int m_cWins;
    int m_cLosses;
    int m_cMinutesPlayed;
    int m_nRank;
    ZString m_strLastPlayed;
    XZoneMedals m_medals;
};


/////////////////////////////////////////////////////////////////////////////////////////////////

class IZoneMedal :
    public IObject,
    public IMedalBase
{
public:
    virtual TRef<Image>     GetImage()          = 0;
};


class CZoneMedalImpl :
    public IZoneMedal
{

public:

    /////////////////////////////////////////////////////////////////////////

    CZoneMedalImpl () :
        m_fRanking(1),
        m_pImage(NULL)
    {
         m_szName[0] = 0;
    }

    /////////////////////////////////////////////////////////////////////////

    void    SetName(const char *szName)
    {
        strncpy(m_szName, szName, sizeof(m_szName)-1);
    }

    /////////////////////////////////////////////////////////////////////////

    char *  GetName() 
    {
        return m_szName;
    }

    /////////////////////////////////////////////////////////////////////////

    void    SetImageFileName(const char * szFileName)
    {
        m_pImage = GetModeler()->LoadImage(szFileName, true);
    }

    /////////////////////////////////////////////////////////////////////////

    TRef<Image> GetImage() 
    {
        return m_pImage;
    }

    /////////////////////////////////////////////////////////////////////////

    int GetRanking()
    {
        return m_fRanking;
    }

    /////////////////////////////////////////////////////////////////////////

    void SetRanking(int nRanking)
    {
        m_fRanking = nRanking;
    }

    /////////////////////////////////////////////////////////////////////////

    void SetDescription(const char * szDescription)
    {
        strncpy(m_szDescription, szDescription, sizeof(m_szDescription));
    }

    /////////////////////////////////////////////////////////////////////////

    char * GetDescription()
    {
        return m_szDescription;
    }

    /////////////////////////////////////////////////////////////////////////

private:

    char            m_szName[256];
    char            m_szDescription[512];
    int             m_fRanking;
    TRef<Image>     m_pImage;
};


TRef<IZoneMedal> CreateZoneMedal()
{
    return new CZoneMedalImpl;
}







//////////////////////////////////////////////////////////////////////////////
//
// Character Info Screen
//
//////////////////////////////////////////////////////////////////////////////

class CharInfoScreen :
    public Screen,
//    public LogonSite, 
//    public TrekClientEventSink,
    public EventTargetContainer<CharInfoScreen>,
    public TEvent<ItemID>::Sink
{
private:
    TRef<Modeler>     m_pmodeler;
    TRef<Pane>        m_ppane;

    TRef<IKeyboardInput>        m_pkeyboardInputOldFocus;

    // buttons
    TRef<ButtonPane>  m_pbuttonLeaderboard;
    TRef<ButtonPane>  m_pbuttonSquads;
	TRef<ButtonPane>  m_pbuttonSquads2;
    TRef<ButtonPane>  m_pbuttonZoneClub;
    TRef<ButtonPane>  m_pbuttonEdit;
    TRef<ButtonPane>  m_pbuttonApply; // apply edit changes
    TRef<ButtonPane>   m_pbuttonWeb;

	// header
    TRef<EditPane>    m_peditPaneCharDescription; // when in edit mode, this is the description
    TRef<ModifiableString> m_pstringCharDescription; // when in read-only mode this is the description
    TRef<EditPane>    m_peditPaneCharName;
    TRef<ModifiableString> m_pstringMemberSince;
    TRef<ComboPane>   m_pcomboCiv;

    // stats
    TRef<ModifiableString> m_pstringRank;
    TRef<ModifiableString> m_pstringBaseCaptures;
    TRef<ModifiableString> m_pstringScore;
    TRef<ModifiableString> m_pstringBaseKills;
    TRef<ModifiableString> m_pstringKills;
    TRef<ModifiableString> m_pstringDeaths;
    TRef<ModifiableString> m_pstringWins;
    TRef<ModifiableString> m_pstringLosses;
    TRef<ModifiableString> m_pstringPromotionAt;
    TRef<ModifiableString> m_pstringMinutesPlayed;
    TRef<ModifiableString> m_pstringLastPlayed;
    TRef<ModifiableString> m_pstringCombatRating;
	

//    TRef<ComboPane>        m_pcomboSquads;
    TRef<StringListPane>   m_plistPaneSquads;



    // medals
    TRef<ListPane>         m_plistPaneMedals;
    TRef<TEvent<ItemID>::Sink> m_psinkMedals;
    TRef<IItemEvent::Source>   m_peventMedals;
    TRef<ModifiableString> m_pstringMedalName;
    TRef<ModifiableString> m_pstringMedalDescription;
    XZoneMedals *          m_plistMedals;   
    WinPoint               m_pointMaxMedalSize;
    int*                   m_pCivIds;
    CCharacterStats *      m_pStats;
    int                    m_cCivs;
    int                    m_nID;   // Zone Club ID

    //watermark
    TRef<WrapImage>      m_pwrapImageCiv;


public:

    /////////////////////////////////////////////////////////////////////////

    CharInfoScreen(Modeler* pmodeler, int idZone) :
        m_nID(idZone),
        m_pCivIds(NULL),
        m_pStats(NULL),
        m_pmodeler(pmodeler)
    {

        g_pCharInfoScreen = this;

        m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();

#ifndef NO_CLUB_SERVER_CONNECTION 

        // Ask server for data
        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmGeneral, C, REQ_CHARACTER_INFO)
        END_PFM_CREATE
        pfmGeneral->nMemberID = idZone;

        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmSquadListReq, C, SQUAD_NEXT_PAGE_DUDEX)
        END_PFM_CREATE
        pfmSquadListReq->nMemberID = idZone;

        trekClient.SendClubMessages();
#endif

    //
    // Exports
    //

        TRef<INameSpace> pnsData = pmodeler->CreateNameSpace("charinfoscreendata");

        // medal exports
        pnsData->AddMember("medalNameStr",       (Value*)(m_pstringMedalName = new ModifiableString("No medals earned yet.")));
        pnsData->AddMember("medalDescriptionStr",(Value*)(m_pstringMedalDescription = new ModifiableString("")));

        // stat exports
        char *szDefault = "";
        pnsData->AddMember("memberSinceStr",(Value*)(m_pstringMemberSince = new ModifiableString("")));
        pnsData->AddMember("descriptionStr",(Value*)(m_pstringCharDescription = new ModifiableString("")));
        pnsData->AddMember("rankStr",(Value*)(m_pstringRank = new ModifiableString(szDefault)));
        pnsData->AddMember("baseCapturesStr",(Value*)(m_pstringBaseCaptures = new ModifiableString(szDefault)));
        pnsData->AddMember("scoreStr",(Value*)(m_pstringScore = new ModifiableString(szDefault)));
        pnsData->AddMember("baseKillsStr",(Value*)(m_pstringBaseKills = new ModifiableString(szDefault)));
        pnsData->AddMember("killsStr",(Value*)(m_pstringKills = new ModifiableString(szDefault)));
        pnsData->AddMember("deathsStr",(Value*)(m_pstringDeaths = new ModifiableString(szDefault)));
        pnsData->AddMember("winsStr",(Value*)(m_pstringWins = new ModifiableString(szDefault)));
        pnsData->AddMember("lossesStr",(Value*)(m_pstringLosses = new ModifiableString(szDefault)));
        pnsData->AddMember("promotionAtStr",(Value*)(m_pstringPromotionAt = new ModifiableString(szDefault)));
        pnsData->AddMember("minutesPlayedStr",(Value*)(m_pstringMinutesPlayed = new ModifiableString(szDefault)));
        pnsData->AddMember("lastPlayedStr",(Value*)(m_pstringLastPlayed = new ModifiableString(szDefault)));
        pnsData->AddMember("combatRatingStr",(Value*)(m_pstringCombatRating = new ModifiableString(szDefault)));

        // watermark
        pnsData->AddMember("civImage", (Value*)(m_pwrapImageCiv = new WrapImage(Image::GetEmpty())));

    //
    // Imports
    //

        TRef<INameSpace> pns = pmodeler->GetNameSpace("charinfoscreen");
        CastTo(m_ppane, pns->FindMember("screen"));

        //
        // Normal Buttons
        //
        CastTo(m_pbuttonLeaderboard, pns->FindMember("leaderboardButtonPane"));
        CastTo(m_pbuttonSquads, pns->FindMember("squadsButtonPane"));
        CastTo(m_pbuttonSquads2, pns->FindMember("squads2ButtonPane"));
        CastTo(m_pbuttonZoneClub, pns->FindMember("zoneclubButtonPane"));
		// mdvalley: Pointers and class.
        AddEventTarget(&CharInfoScreen::OnButtonLeaderboard, m_pbuttonLeaderboard->GetEventSource());
        AddEventTarget(&CharInfoScreen::OnButtonSquads, m_pbuttonSquads->GetEventSource());
        AddEventTarget(&CharInfoScreen::OnButtonSquads, m_pbuttonSquads2->GetEventSource());
        AddEventTarget(&CharInfoScreen::OnButtonZoneClub, m_pbuttonZoneClub->GetEventSource());
        CastTo(m_pbuttonApply, pns->FindMember("applyButtonPane"));
        AddEventTarget(&CharInfoScreen::OnButtonApply, m_pbuttonApply->GetEventSource());
        m_pbuttonSquads->SetEnabled(!g_bDisableZoneClub);
        m_pbuttonSquads2->SetEnabled(!g_bDisableZoneClub);
        m_pbuttonApply->SetEnabled(false);
        m_pbuttonApply->SetHidden(true);

        CastTo(m_pbuttonWeb, pns->FindMember("webButtonPane"));
        AddEventTarget(&CharInfoScreen::OnButtonZoneWeb, m_pbuttonWeb->GetEventSource());

        //
        // Medals Window
        //
        TRef<PointValue> ppointTemp;
        CastTo(ppointTemp, pns->FindMember("maxMedalBitmapSize"));

        m_pointMaxMedalSize.SetX((int)ppointTemp->GetValue().X());
        m_pointMaxMedalSize.SetY((int)ppointTemp->GetValue().Y());

        CastTo(m_plistPaneMedals, (Pane*)pns->FindMember("medalsListPane"     ));
        m_peventMedals = m_plistPaneMedals->GetSelectionEventSource();
        m_peventMedals->AddSink(m_psinkMedals = new IItemEvent::Delegate(this));
        m_plistPaneMedals->SetItemPainter(new MedalsItemPainter(this));

        //
        //  headers stuff -- description with edit button
        //
        CastTo(m_pbuttonEdit, pns->FindMember("editButtonPane"));
        AddEventTarget(&CharInfoScreen::OnButtonEdit, m_pbuttonEdit->GetEventSource());

        if (trekClient.GetZoneClubID() != idZone)
           m_pbuttonEdit->SetEnabled(false);

        CastTo(m_peditPaneCharDescription, (Pane*)pns->FindMember("descriptionEditPane"));
        m_peditPaneCharDescription->SetMaxLength(c_cbDescriptionDB);
        m_peditPaneCharDescription->SetHidden(true);
        AddEventTarget(&CharInfoScreen::OnDescriptionClick, m_peditPaneCharDescription->GetClickEvent());

        m_peditPaneCharDescription->SetString(szDefault);
        m_peditPaneCharDescription->SetReadOnly();

        CastTo(m_peditPaneCharName, (Pane*)pns->FindMember("characterNameEditPane"));
        m_peditPaneCharName->SetMaxLength(c_cbNameDB);
        AddEventTarget(&CharInfoScreen::OnNameClick, m_peditPaneCharName->GetClickEvent());
      
        m_peditPaneCharName->SetString(szDefault);
        m_peditPaneCharName->SetReadOnly();

        CastTo(m_pcomboCiv, pns->FindMember("civComboPane"));
        m_pcomboCiv->SetEnabled(true);
        AddEventTarget(&CharInfoScreen::OnCivChange, m_pcomboCiv->GetEventSource());

        int cNames = FillCombo(pns, m_pcomboCiv, "CivNames");
        m_cCivs = cNames;
        m_pCivIds = new int[cNames];
        m_pStats = new CCharacterStats[cNames];
        int cInts = FillIntegerArray(pns, m_pCivIds, "CivIDs");
        assert(cInts == cNames);

        OnCivChange(); // init civImage

        //
        // Stats
        //
        CastTo(m_plistPaneSquads,(Pane*)pns->FindMember("squadsListPane"));
        AddEventTarget(&CharInfoScreen::OnSquadDoubleClick, m_plistPaneSquads->GetDoubleClickEventSource());

        pmodeler->UnloadNameSpace(pns);

#ifdef NO_CLUB_SERVER_CONNECTION 

        //
        // TEMP:
        //
        m_plistPaneSquads->GetStringList()->AddItem("Marco's Squad");
        m_plistPaneSquads->GetStringList()->AddItem("Death Squad");
        m_plistPaneSquads->GetStringList()->AddItem("Rainfall");
        m_plistPaneSquads->GetStringList()->AddItem("Dudes");
        m_plistPaneSquads->GetStringList()->AddItem("Nice Guys");
        m_plistPaneSquads->GetStringList()->AddItem("Four Horsemen");

        for (int i = 0; i < cNames; ++i)
        {
            m_pStats[i].m_fScore = 430*i-10;
            m_pStats[i].m_cBaseCaptures = 10-i;
            m_pStats[i].m_cBaseKills = 20-2*i;
            m_pStats[i].m_cKills = 43+i*3;
            m_pStats[i].m_cDeaths = 30-i*3;
            m_pStats[i].m_cWins = i*3;
            m_pStats[i].m_cLosses = i/2;
            m_pStats[i].m_nPromotionAt = 1000+i*200;
            m_pStats[i].m_cMinutesPlayed = 68+22*i;
            m_pStats[i].m_strLastPlayed = "12/22/99";

        }

        TRef<IZoneMedal> pMedal = CreateZoneMedal();
        pMedal->SetName("Purple Heart");
        pMedal->SetDescription("Awarded for getting injured in Mission 3B");
        pMedal->SetImageFileName("iconleaderbmp");
        pMedal->SetRanking(1);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[1].m_medals.PushEnd(pMedal);

        pMedal = CreateZoneMedal();
        pMedal->SetName("Flying Cross");
        pMedal->SetDescription("Awarded for brave flying");
        pMedal->SetImageFileName("iconturretbmp");
        pMedal->SetRanking(2);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[2].m_medals.PushEnd(pMedal);


        pMedal = CreateZoneMedal();
        pMedal->SetName("Good Ownership");
        pMedal->SetDescription("Awarded by teammates for being a good owner");
        pMedal->SetImageFileName("iconflyingbmp");
        pMedal->SetRanking(30);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[1].m_medals.PushEnd(pMedal);


        pMedal = CreateZoneMedal();
        pMedal->SetName("Awesome Destroyer");
        pMedal->SetDescription("Awarded for high kill count");
        pMedal->SetImageFileName("iconownerbmp");
        pMedal->SetRanking(10);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[2].m_medals.PushEnd(pMedal);

        pMedal = CreateZoneMedal();
        pMedal->SetName("Purple Heart");
        pMedal->SetImageFileName("iconleaderbmp");
        pMedal->SetDescription("Awarded for getting injured in Mission 23A");
        pMedal->SetRanking(1);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[1].m_medals.PushEnd(pMedal);

        pMedal = CreateZoneMedal();
        pMedal->SetName("Purple Heart");
        pMedal->SetImageFileName("iconleaderbmp");
        pMedal->SetDescription("Awarded for getting injured in Mission 5C");
        pMedal->SetRanking(1);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[1].m_medals.PushEnd(pMedal);

        pMedal = CreateZoneMedal();
        pMedal->SetName("Purple Heart");
        pMedal->SetImageFileName("iconleaderbmp");
        pMedal->SetDescription("Awarded for getting injured in Mission 3A");
        pMedal->SetRanking(1);
        m_pStats[0].m_medals.PushEnd(pMedal);
        m_pStats[1].m_medals.PushEnd(pMedal);

        RefreshMedalsList();
#endif
    }

    /////////////////////////////////////////////////////////////////////////

    virtual ~CharInfoScreen()
    {
        if(m_pCivIds)
            delete[] m_pCivIds;

        if(m_pkeyboardInputOldFocus)
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);

        g_pCharInfoScreen = NULL;

        m_peventMedals->RemoveSink(m_psinkMedals);
        m_plistPaneMedals->SetList(new EmptyList); // remove to avoid cyclic destruction

//        m_plistPaneMedals = NULL;

        if (m_pStats)
        {
            int i;
            for (i = 0; i < m_cCivs; ++i)
                m_pStats[i].m_medals.SetEmpty();

            delete[] m_pStats;
        }
    }

    /////////////////////////////////////////////////////////////////////////
	

	/////////////////////////////////////////////////////////////////////////


	void ForwardCharInfoMessage(FEDMESSAGE *pCharInfoMessage)
    {
#ifndef NO_CLUB_SERVER_CONNECTION 
        switch (pCharInfoMessage->fmid) 
        {
            case FM_S_CHARACTER_INFO_GENERAL:
            {
                CASTPFM(pfmCharInfoGeneral, S, CHARACTER_INFO_GENERAL, pCharInfoMessage);

                char * szPlayerName = FM_VAR_REF(pfmCharInfoGeneral, szName);

                if (szPlayerName)
                    m_peditPaneCharName->SetString(szPlayerName);
                else
                    m_peditPaneCharName->SetString(ZString());

                char * szDescription = FM_VAR_REF(pfmCharInfoGeneral, szDescription);

                if (szDescription)
                {
                    m_peditPaneCharDescription->SetString(szDescription);

                    if (m_peditPaneCharDescription->IsHidden())
                      m_pstringCharDescription->SetValue(szDescription);
                }
                else 
                    m_peditPaneCharDescription->SetString(ZString());
            }
            break;

            case FM_S_CHARACTER_INFO_BY_CIV:
            {
                CASTPFM(pfmCharInfoByCiv, S, CHARACTER_INFO_BY_CIV, pCharInfoMessage);

                // find out which index this civid maps to
                int i;
                for (i = 0; i < m_cCivs; ++i)
                    if (m_pCivIds[i] == pfmCharInfoByCiv->civid)
                        break;

                if (i == m_cCivs)
                {
                    //MessageBox(0, "Unknown CivID; CharInfoScreen.mdl is not in sync with AllClub's database.", "Error", 0);
                    //assert(0);
                    break;
                }
                else
                {
                    m_pStats[i].m_bInfoReceived = true;
                    m_pStats[i].m_fScore = pfmCharInfoByCiv->fScore;
                    m_pStats[i].m_cBaseCaptures = pfmCharInfoByCiv->cBaseCaptures;
                    m_pStats[i].m_cBaseKills = pfmCharInfoByCiv->cBaseKills;
                    m_pStats[i].m_cKills = pfmCharInfoByCiv->cKills;
                    m_pStats[i].m_cDeaths = pfmCharInfoByCiv->cDeaths;
                    m_pStats[i].m_cWins = pfmCharInfoByCiv->cWins;
                    m_pStats[i].m_cLosses = pfmCharInfoByCiv->cLosses;
                    m_pStats[i].m_nPromotionAt = pfmCharInfoByCiv->nPromotionAt;
                    m_pStats[i].m_cMinutesPlayed = pfmCharInfoByCiv->cMinutesPlayed;
                    m_pStats[i].m_nRank = pfmCharInfoByCiv->nRank;
                    m_pStats[i].m_nCombatRating = pfmCharInfoByCiv->nRating;
                    char * szLastPlayed = FM_VAR_REF(pfmCharInfoByCiv, szLastPlayed);
                    if (szLastPlayed)
                        m_pStats[i].m_strLastPlayed = szLastPlayed;

                    //
                    // Update screen as needed
                    //
                    if (i == m_pcomboCiv->GetIndex())
                        OnCivChange();
                }
            }
            break;

            case FM_S_CHARACTER_INFO_MEDAL:
            {
                CASTPFM(pMedalInfo, S, CHARACTER_INFO_MEDAL, pCharInfoMessage);

                TRef<IZoneMedal> pMedal = CreateZoneMedal();
                pMedal->SetName(FM_VAR_REF(pMedalInfo, szName));
                pMedal->SetDescription(ZString(FM_VAR_REF(pMedalInfo, szDescription)) 
                    + "\n\n" + ZString(FM_VAR_REF(pMedalInfo, szSpecificInfo)));

                pMedal->SetImageFileName(ZString(FM_VAR_REF(pMedalInfo, szBitmap)) + ZString("bmp"));
                // mdvalley: Gotta be explicit int in 2005
                static int c = 0;
                pMedal->SetRanking(c++);

                // find out which index this civid maps to
                int i;
                for (i = 0; i < m_cCivs; ++i)
                    if (m_pCivIds[i] == pMedalInfo->civid)
                        break;

                if (i == m_cCivs)
                {
                    //MessageBox(0, "Unknown CivID; CharInfoScreen.mdl is not in sync with AllClub's database.", "Error", 0);
                    assert(0);
                    break;
                }
                else
                {
                    m_pStats[i].m_medals.PushEnd(pMedal);
                }

                if (i == m_pcomboCiv->GetIndex())
                    OnCivChange();

                
            }
            break;

            case FM_S_SQUAD_INFO_DUDEX:
            {
                CASTPFM(pfmInfo, S, SQUAD_INFO_DUDEX, pCharInfoMessage);
                char *szName = FM_VAR_REF(pfmInfo, szName);
                m_plistPaneSquads->GetStringList()->AddItem(szName);
           }
            break;
        }
#endif
    }


    /////////////////////////////////////////////////////////////////////////

    void SetCivBitmap(const ZString *strCivName)
    {
        if (strCivName) {
            ZString str = *strCivName;
            str.ReplaceAll(" ", '_');
            str = str.ToLower() + "charinfowatermarkbmp";

            m_pwrapImageCiv->SetImage(m_pmodeler->LoadImage(str, false));
        } else {
            m_pwrapImageCiv->SetImage(m_pmodeler->LoadImage("defaultwatermarkbmp", false));
        }
    }

    /////////////////////////////////////////////////////////////////////////

    int FillCombo(INameSpace* pns, ComboPane* pcombo, const char* szContentName)
    {
        IObjectList* plist; 
        
        CastTo(plist, pns->FindMember(szContentName));

        plist->GetFirst();

        int index = 0;
        while (plist->GetCurrent() != NULL) 
        {
            pcombo->AddItem(GetString(plist->GetCurrent()), index);
            ++index;
            plist->GetNext();
        }
        return index;
    }

    /////////////////////////////////////////////////////////////////////////
    int FillIntegerArray(INameSpace* pns, int * pInts, const char* szContentName)
    {
        IObjectList* plist; 
        
        CastTo(plist, pns->FindMember(szContentName));

        plist->GetFirst();

        int index = 0;
        while (plist->GetCurrent() != NULL) 
        {
            int nID = (int)GetNumber(plist->GetCurrent());

            pInts[index] = nID;
            ++index;
            plist->GetNext();
        }
        return index;
    }

    /////////////////////////////////////////////////////////////////////////

    int GetMedalBitmapMaxWidth()
    {
        return m_pointMaxMedalSize.X();
    }

    /////////////////////////////////////////////////////////////////////////

    int GetMedalBitmapMaxHeight()
    {
        return m_pointMaxMedalSize.Y();
    }

    /////////////////////////////////////////////////////////////////////////

    WinPoint & GetMedalBitmapMaxSize()
    {
        return m_pointMaxMedalSize; 
    }

    /////////////////////////////////////////////////////////////////////////

    Color & GetMedalSelectionBarColor()
    {
        static Color c = Color(1, 0, 0);
        return c; // TODO make export
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnCivChange()
    {
        SetCivBitmap(m_pcomboCiv->GetSelectionString());

        int i = m_pcomboCiv->GetSelection();

        if(m_pStats[i].m_bInfoReceived)
        {
            m_pstringPromotionAt->SetValue(m_pStats[i].m_nPromotionAt);
            m_pstringScore->SetValue(m_pStats[i].m_fScore);
            m_pstringBaseCaptures->SetValue(m_pStats[i].m_cBaseCaptures);
            m_pstringBaseKills->SetValue(m_pStats[i].m_cBaseKills);
            m_pstringKills->SetValue(m_pStats[i].m_cKills);
            m_pstringDeaths->SetValue(m_pStats[i].m_cDeaths);
            m_pstringWins->SetValue(m_pStats[i].m_cWins);
            m_pstringLosses->SetValue(m_pStats[i].m_cLosses);
            m_pstringMinutesPlayed->SetValue(m_pStats[i].m_cMinutesPlayed);
            m_pstringLastPlayed->SetValue(m_pStats[i].m_strLastPlayed);
            m_pstringCombatRating->SetValue(m_pStats[i].m_nCombatRating);
#if defined(NO_CLUB_SERVER_CONNECTION) || defined(MARKCU1)
            m_pstringRank->SetValue("Admiral");
#else
            m_pstringRank->SetValue(trekClient.LookupRankName(m_pStats[i].m_nRank, m_pCivIds[i]));
#endif
        }
        else
        {
            m_pstringPromotionAt->SetValue("");
            m_pstringScore->SetValue("");
            m_pstringBaseCaptures->SetValue("");
            m_pstringBaseKills->SetValue("");
            m_pstringKills->SetValue("");
            m_pstringDeaths->SetValue("");
            m_pstringWins->SetValue("");
            m_pstringLosses->SetValue("");
            m_pstringMinutesPlayed->SetValue("");
            m_pstringLastPlayed->SetValue("");
            m_pstringRank->SetValue("");
			m_pstringCombatRating->SetValue("");
        }

        m_plistMedals = &m_pStats[i].m_medals;
        RefreshMedalsList();

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnSquadDoubleClick()
    {
//        StringListItem * pstr = (StringListItem*)m_plistPaneSquads->GetSelection();
//        OutputDebugString(PCC(pstr->GetString()));
        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnButtonZoneClub()
    {
#ifndef MARKCU1
        GetWindow()->screen(ScreenIDZoneClubScreen);
#endif

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnButtonSquads()
    {
#ifndef MARKCU1
        StringListItem * pstr = (StringListItem*)m_plistPaneSquads->GetSelection();
        if (pstr)
        {
           GetWindow()->SquadScreenForPlayer(PCC(m_peditPaneCharName->GetString()), m_nID, PCC(pstr->GetString()));
        }
        else
        {
           GetWindow()->SquadScreenForPlayer(PCC(m_peditPaneCharName->GetString()), m_nID, NULL);
        }
#endif
        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnButtonLeaderboard()
    {
//        GetWindow()->LeaderBoardScreenForPlayer(m_nID == NA ? trekClient.GetZoneClubID() : m_nID);
#ifndef MARKCU1
        GetWindow()->LeaderBoardScreenForPlayer(m_peditPaneCharName->GetString());
#endif

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

	
    bool OnButtonGames()
    {
       // m_bConnectLobby = true;

        //ConnectToZone(ScreenIDGameScreen);

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnButtonZoneWeb()
    {
        // note: users can also access web page from the introscreen
        GetWindow()->ShowWebPage();
        return true;
    }

	////////////////////////////////////////////////////////////////////////

    bool OnButtonApply()
    {
        m_peditPaneCharDescription->SetReadOnly(true);
        m_peditPaneCharDescription->SetHidden(true);
        m_pstringCharDescription->SetValue(m_peditPaneCharDescription->GetString());

        m_pbuttonApply->SetEnabled(false);
        m_pbuttonApply->SetHidden(true);
        m_pbuttonEdit->SetEnabled(true);
        m_pbuttonEdit->SetHidden(false);

#ifndef NO_CLUB_SERVER_CONNECTION 

        // send update
        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmEdit, C, CHARACTER_INFO_EDIT_DESCRIPTION)
            FM_VAR_PARM(PCC(m_peditPaneCharDescription->GetString()), CB_ZTS)
        END_PFM_CREATE

        trekClient.SendClubMessages();
#endif

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnButtonEdit()
    {
        m_peditPaneCharDescription->SetReadOnly(false);
        m_peditPaneCharDescription->SetHidden(false);
        m_pstringCharDescription->SetValue("");

        m_pbuttonApply->SetEnabled(true);
		    m_pbuttonApply->SetHidden(false);
        m_pbuttonEdit->SetEnabled(false);
		    m_pbuttonEdit->SetHidden(true);

        GetEngineWindow()->SetFocus(m_peditPaneCharDescription);

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnDescriptionClick()
    {
        GetEngineWindow()->SetFocus(m_peditPaneCharDescription);

        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    bool OnNameClick()
    {
        return true;
    }

    /////////////////////////////////////////////////////////////////////////

    static bool MedalRankingCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneMedal* pmedal1 = (IZoneMedal*)pitem1;
        IZoneMedal* pmedal2 = (IZoneMedal*)pitem2;

        return pmedal1->GetRanking() > pmedal2->GetRanking();
    }

    /*-------------------------------------------------------------------------
     * RefreshMedalsList()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Redraw the listbox for medals
     * 
     */
    void RefreshMedalsList()
    {
        List *plist = new ListDelegate(m_plistMedals);

        plist = new SortedList<ItemIDCompareFunction>(plist, MedalRankingCompare);

        m_plistPaneMedals->SetList(plist);

        if (m_plistPaneMedals->GetSelection() == NULL && !m_plistMedals->IsEmpty())
            m_plistPaneMedals->SetSelection(m_plistMedals->GetItem(0));
    }

    /*-------------------------------------------------------------------------
     * SelectMedal()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Call this to select a medal.
     * 
     */
    void SelectMedal(IZoneMedal & medal)
    {
        m_plistPaneMedals->SetSelection(&medal);
    }

    
    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        if (pevent == m_peventMedals) 
        {
            IZoneMedal* pmedal = (IZoneMedal*)pitem;
            OnMedalSelected(pmedal);
        }

        return true;
    }

    /*-------------------------------------------------------------------------
     * OnMedalSelected()
     *-------------------------------------------------------------------------
     * Purpose:
     *   This event is called when a medal is selected.
     * 
     */
    void OnMedalSelected(IZoneMedal* pmedal)
    {
        if (pmedal)
        {
            m_pstringMedalName->SetValue(pmedal->GetName());
            m_pstringMedalDescription->SetValue(pmedal->GetDescription());
        }
        else
        {
            if (m_plistMedals->IsEmpty())
            {
                m_pstringMedalName->SetValue("No medals earned yet.");
                m_pstringMedalDescription->SetValue("");
            }
            else
            {
                m_plistPaneMedals->SetSelection(m_plistMedals->GetItem(0));
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////////
    //
    // MedalsItemPainter - within the Medals listbox
    //
    //////////////////////////////////////////////////////////////////////////////
    class MedalsItemPainter : public ItemPainter
    {
        CharInfoScreen * m_pCharInfoScreen;

    public:

        MedalsItemPainter(CharInfoScreen * pMaker)
            : m_pCharInfoScreen(pMaker)
          {};

        int GetXSize()
        {
            return m_pCharInfoScreen->GetMedalBitmapMaxWidth();
        }

        int GetYSize()
        {
            return m_pCharInfoScreen->GetMedalBitmapMaxHeight();
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            IZoneMedal* pmedal = (IZoneMedal*)pitemArg;
			      TRef<Image> m_pimageSel;
			      m_pimageSel = GetModeler()->LoadImage("medhighlightbmp", true);
			


            if (bSelected)
            {
                //
                // Draw selection bar
                //
			    psurface->BitBlt(WinPoint(0, 0),
				  m_pimageSel->GetSurface());
            }

            
            //
            // Display Proper Icon
            //
            //
            {
                // Center icon
                //
                WinPoint pPointOffset = pmedal->GetImage()->GetSurface()->GetSize();
                pPointOffset = (m_pCharInfoScreen->GetMedalBitmapMaxSize() - pPointOffset) / 2;

                // draw it!
                psurface->BitBlt(pPointOffset, pmedal->GetImage()->GetSurface());
            }
        }
    };                            

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateCharInfoScreen(Modeler* pmodeler, int idZone) // if id is NA, current player is used
{
    return new CharInfoScreen(pmodeler, idZone);
}




CharInfoScreen * g_pCharInfoScreen = NULL;


void ForwardCharInfoMessage(FEDMESSAGE * pCharInfoMessage)
{
    if(g_pCharInfoScreen)
    {
        g_pCharInfoScreen->ForwardCharInfoMessage(pCharInfoMessage);
    }
}

