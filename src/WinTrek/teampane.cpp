#include "pch.h"
#include "training.h"
#include "badwords.h"
//#include "imagetransform.h"

//////////////////////////////////////////////////////////////////////////////
//
// TeamPane
//
//////////////////////////////////////////////////////////////////////////////

static void DrawAutoDonateIcon(PlayerInfo*          pplayer,
                               Surface*             psurface)
{
    if (pplayer->IsHuman())
    {
        bool    bLeader = pplayer->IsTeamLeader();
    
        // draw the donate to icon, if appropriate
        IshipIGC*   pshipAutoDonateTo = trekClient.GetShip()->GetAutoDonate();
        IshipIGC*   pshipAutoDonateFrom = pplayer->GetShip()->GetAutoDonate();

        const char* pszDonateIcon;

        if (pshipAutoDonateTo == pplayer->GetShip())
        {
            pszDonateIcon = bLeader ? "icondonatetocombmp" : "icondonatetobmp";
        }
        else
        {
            if (pshipAutoDonateFrom)
            {
                if ((pshipAutoDonateFrom == trekClient.GetShip()) ||
                    (pshipAutoDonateFrom == pshipAutoDonateTo))
                {
                    pszDonateIcon = bLeader ? "icondonatefromcombmp" : "icondonatefrombmp";
                }
                else
                {
                    pszDonateIcon = bLeader ? "icondonateyoucombmp" : "icondonateyoubmp";
                }
            }
            else
            {
                pszDonateIcon = bLeader ? "icondonateselfcombmp" : "icondonateselfbmp";
            }
        }

        TRef<Image> pimageDonateTo = GetModeler()->LoadImage(pszDonateIcon, true);
    
        WinPoint pntDonateTo = WinPoint(0, 0);
        psurface->BitBlt(pntDonateTo, pimageDonateTo->GetSurface());
    }
}

class TeamPane :
public IItemEvent::Sink,
public EventTargetContainer<TeamPane>,
public TrekClientEventSink
{
protected:
    TRef<ButtonPane>         m_pbuttonBoot;
    TRef<ButtonPane>         m_pbuttonDonate;
    TRef<ButtonPane>         m_pbuttonAutoDonate;
    TRef<ButtonPane>         m_pbuttonStopDonate;
    TRef<ButtonPane>         m_pbuttonExpand;
    TRef<ButtonPane>         m_pbuttonTakeMeTo;
    TRef<ButtonPane>         m_pbuttonAccept;
    TRef<ButtonPane>         m_pbuttoncomboTeam;
    TRef<ButtonPane>         m_pbuttonClose;
    TRef<ButtonBarPane>      m_pbuttonbarPlayers;
    TRef<ListPane>           m_plistPanePlayers;
    TRef<ListPane>           m_plistPaneTeams;
    TRef<ListPane>           m_plistPaneSelTeam;
    TRef<ListPane>           m_plistPaneIcon;
    TRef<IItemEvent::Source> m_peventPlayers;
    TRef<IItemEvent::Source> m_peventTeams;
    TRef<IItemEvent::Sink>   m_psinkPlayers;
    TRef<IItemEvent::Sink>   m_psinkTeams;
    SideID                   m_sideCurrent;
    TRef<ComboPane>          m_pcomboTeams;
    TRef<ComboPane>          m_pcomboWing;
    TeamPane*                m_pTeamPaneTwin;
    //MissionInfo*           m_pMission;
    
    
    
    
    TVector<int>             m_viColumns;
    
    enum 
    {
        sortLeader,
        sortStatus,
        sortName,
        sortShip,
        sortSector,
        sortCredits,
        sortObjective,
        sortRank,
        sortDeaths,
        sortWing,
        sortKills,
        sortTurrets
    } m_sortPlayers;
    
public:
    class PlayerPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        
    public:
        
        PlayerPainter(const TVector<int>& viColumns)
            : m_viColumns(viColumns) {};
        
        int GetXSize()
        {
            return 160;
        }
        
        int GetYSize()
        {
            return 28;
        }
        
        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitemArg));
            ZString     pname;
            if (pplayer == NULL || trekClient.MyMission() == NULL)
                return;
            
            // draw the selection bar
            
            if (bSelected) {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(1, 0, 0)
                    );
            }
            
            //
            // draw the icons
            //
            DrawAutoDonateIcon(pplayer, psurface);
            
            // draw the name
            
            if (pplayer->LastSeenState() == c_ssDocked)
            {
                psurface->DrawString(
                    TrekResources::SmallBoldFont(),
                    Color(0.617f, 0.0f, 0.156f),
                    WinPoint(17, 2),
                    pplayer->CharacterName()
                    );
            }
            else
            {
                psurface->DrawString(
                    TrekResources::SmallFont(),
                    Color::White(),
                    WinPoint(17, 2),
                    pplayer->CharacterName()
                    );
            }
            
            // Set the font to white if we know this info for sure, grey if we 
            // are guessing based on what we have seen earlier.
            
            Color color = 
                (pplayer->StatusIsCurrent() || pplayer->SideID() == trekClient.GetSideID())
                ? Color::White() : 0.75f * Color::White();
            
            
            // draw the ship
            
            //psurface->DrawString(
            //    WinPoint(140, 8),
            //    ShipName(pplayer)
            //);
            //TRef<Image> pimageShipIcon;
            
            //if (IsAllyOrHumanOrSpotted(pplayer)){
            //  ZString piconname = ZString(pplayer->GetShip()->GetBaseHullType()->GetIconName()).ToLower();
            //  ZString str = piconname + ZString("bmp");
            //  pimageShipIcon = GetModeler()->LoadImage(str, true);
            //  psurface->BitBlt(WinPoint(140, 8), pimageShipIcon->GetSurface());
            //}
            
            // draw the sector
            
            //psurface->DrawString(
            //    WinPoint(m_viColumns[2] + 2, 0),
            //    SectorName(pplayer->LastSeenSector())
            //);
            
            // draw their money  
			IsideIGC* pside = pplayer->GetShip()->GetSide();
            if (pplayer->IsHuman() && 
				( pplayer->SideID() == trekClient.GetSideID() || //Imago 7/6/09 ALLYTD VISIBLITY, show allies $$$; That data isn't being sent, should it?
				pside->AlliedSides(pside,trekClient.GetSide()) ))
            {
				//Imago #7 7/10
				if (pplayer->IsTeamLeader() || !trekClient.MyPlayerInfo()->IsTeamLeader()) {
					char cbTemp[256];
					wsprintf(cbTemp, "%d", pplayer->GetMoney());
					psurface->DrawString(TrekResources::SmallFont(),color,WinPoint(19, 14),ZString("$: ") + cbTemp);
				} else {
					if ((GetWindow()->GetOverlayFlags() & ofTeam) && (pplayer->LastSeenState() == c_ssDocked || pplayer->LastSeenState() == NULL)) {
						char cbTemp[256];
						DWORD dDelta = Time::Now().clock() - trekClient.GetCore()->GetIgcSite()->ClientTimeFromServerTime(pplayer->LastStateChange()).clock();
						int iSecs = (dDelta != 0) ? dDelta / 1000 : 0;
						if ((iSecs != 0) && iSecs < 60) {
							wsprintf(cbTemp, "Docked for %isec",iSecs);
							psurface->DrawString(TrekResources::SmallFont(),color,WinPoint(19, 14),cbTemp);
						} else if ((iSecs != 0)) {
							int iMins = iSecs / 60;
							iSecs = iSecs % 60;
							wsprintf(cbTemp, "Docked for %imin %isec",iMins,iSecs);
							psurface->DrawString(TrekResources::SmallFont(),color,WinPoint(19, 14),cbTemp);
						}
					} else {
						char cbTemp[256];
						wsprintf(cbTemp, "%d", pplayer->GetMoney());
						psurface->DrawString(TrekResources::SmallFont(),color,WinPoint(19, 14),ZString("$: ") + cbTemp);
					}
				}
				// #7
            }
            
            // draw the deaths
            
            if (pplayer->MissionDeaths())
                psurface->DrawString(TrekResources::SmallFont(), color, WinPoint(143, 2), ZString(pplayer->MissionDeaths()));
            else
                psurface->DrawString(TrekResources::SmallFont(), color, WinPoint(143, 2), ZString("0"));
            // draw the kills
            
            if (pplayer->MissionKills())
                psurface->DrawString(TrekResources::SmallFont(), color, WinPoint(120, 2), ZString(pplayer->MissionKills()));
            else
                psurface->DrawString(TrekResources::SmallFont(), color, WinPoint(120, 2), ZString("0"));
        }
    };
    
    class TeamPainter : public ItemPainter
    {
        MissionInfo* m_pMission;
        
        TRef<Image> m_pimageArrow;
        TRef<Image> m_pimageTab;
		TRef<Image> m_pimageAllies[c_cAlliancesMax]; //Imago ALLYTD 7/9/09 come up with something better than a colored number
        
        int m_nWidth;
        int m_nHeight;
        
        bool m_bSingle;
        
        
    public:
        TeamPainter(MissionInfo* pMission, int nWidth, int nHeight, bool bSingle) : 
          m_pMission(pMission),
              m_nWidth(nWidth), 
              m_nHeight(nHeight),
              m_bSingle(bSingle)
          {
              
              m_pimageArrow = GetModeler()->LoadImage("arrowbmp", true);
              
          };
          
          int GetXSize()
          {
              return m_nWidth;
          }
          
          int GetYSize()
          { 
              return m_nHeight;
          }
          
          void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
          {
              SideInfo* pitem = (SideInfo*)pitemArg;
              
              Color color;
              IsideIGC* pside = NULL;
              
              if (trekClient.GetCore())
                  pside = trekClient.GetCore()->GetSide(pitem->GetSideID());
              int pnumberside = pitem->GetSideID();
              

			  // yp - Your_Persona Team total rank teampane patch may 24 2007  // modified by aem
			  // Add up the sum of all the players ranks.
			  // modified to add IsHuman check, aem and mmf
              const ShipListIGC* mp_ships = pside->GetShips();
              int iRankSum = 0;
              for (const ShipLinkIGC* lShip = mp_ships->first(); lShip; lShip = lShip->next())
              {
                   IshipIGC* pship = lShip->data();
				   if (pship) {
						PlayerInfo* pPlayer = (PlayerInfo*)pship->GetPrivateData();
						if (pPlayer) {
                            if (pPlayer->IsHuman()) 
						        iRankSum += pPlayer->GetPersistScore(NA).GetRank(); // mmf this is just for display purposes
						}
				   }
              }
			  // end yp

              switch (pnumberside)
              {
              case 0:
                  m_pimageTab = GetModeler()->LoadImage("btnteamyellowbmp", true);
                  break;

              case 1:
                  m_pimageTab = GetModeler()->LoadImage("btnteambluebmp", true);
                  break;

              case 2:
                  m_pimageTab = GetModeler()->LoadImage("btnteampurplebmp", true);
                  break;

              case 3:
                  m_pimageTab = GetModeler()->LoadImage("btnteamgreenbmp", true);
                  break;

              case 4:
                  m_pimageTab = GetModeler()->LoadImage("btnteamredbmp", true);
                  break;

              case 5:
                  m_pimageTab = GetModeler()->LoadImage("btnteamwhitebmp", true);
                  break;
                  //
                  // WLP 2005 - added default for NOAT lobby team display
                  //
              default:                                                                // WLP 2005 - view lobby
                  m_pimageTab = GetModeler()->LoadImage("btnteamlobbybmp", true);     // WLP 2005 - view lobby
              }

              /*if (!trekClient.m_fm.IsConnected()) {
                  m_pimageTab = GetModeler()->LoadImage("btnteambmp", true);
              }*/

              if (m_pimageTab)
			  {
                  psurface->BitBlt(WinPoint(0, 0), m_pimageTab->GetSurface());
                  /*if (!trekClient.m_fm.IsConnected()) {
                      Color c = pside->GetColor();
                      if (c.R() + c.G() + c.B() > 2.0f)
                          c = Color(c.R()*0.7f, c.G()*0.7f, c.B()*0.7f);
                      debugf("%s using color %f, %f, %f instead of %f, %f, %f\n", pside->GetName(), c.R(), c.G(), c.B(), pside->GetColor().R(), pside->GetColor().G(), pside->GetColor().B());

                      m_pimageTab = ImageTransform::Multiply(m_pimageTab, new ColorValue(c));

                      WinPoint offset = psurface->GetOffset();
                      auto context = psurface->GetContext();
                      context->SetYAxisInversion(false);
                      context->Translate(Point(offset.X(), offset.Y()));
                      m_pimageTab->Render(context);
                      psurface->ReleaseContext(context);
                  }*/
              	if (!m_bSingle){
                	if (bSelected) 
                  	{
                      // draw the selected tab arrow
                      psurface->BitBlt(WinPoint(157,0), m_pimageArrow->GetSurface());  //AEM 7.21.07 Expanded to X of 160 from 130 
                  	}
              	}            
              WinRect rectClipOld = psurface->GetClipRect();
              psurface->SetClipRect(WinRect(WinPoint(1, 0), WinPoint(105, 20))); // clip name to fit in column // yp: changed from 105 to 90 //AEM to 130

              // draw the team name
			  ZString name;
			  if ( pitem->GetSideID()== SIDE_TEAMLOBBY ) 
			  {
					name="Not On A Team";
			  }
			  else
			  {
					name=CensorBadWords (m_pMission->SideName(pitem->GetSideID()));
			  }
			  // #ALLY - temporary ui: prefix team name with alliance group number
			  char allies = m_pMission->SideAllies(pitem->GetSideID());
			  if (allies!=NA) { //imago added ally  color 7/9/09
				  Color AllianceColors[3] = { Color::Green(), Color::Orange(), Color::Red() };
				  psurface->FillRect(WinRect(WinPoint(0, 0), WinPoint(9, 20)), AllianceColors[pside->GetAllies()]*0.75);
				  name = ZString((int)allies+1)+" " + name;
			  }
			  //
              psurface->DrawString(
                  TrekResources::SmallFont(),
                  Color::White(),
                  WinPoint(1, 2),
                  name
                  );
              psurface->RestoreClipRect(rectClipOld);
              
                           //
			  // WLP - don't show this for lobby - so I added the IF test
			  //
			  if ( pitem->GetSideID()!= SIDE_TEAMLOBBY )
			  {
               // draw number of human players on team
               psurface->DrawString(
                  TrekResources::SmallFont(),
                  Color::White(),
                  WinPoint(115, 2),
                  ZString(m_pMission->SideNumPlayers(pitem->GetSideID())) +				  
				  ZString("|") + ZString(iRankSum) // yp: added team total rank to team name display.  
												   // AEM 7.21.07 changed format from (#players)[rank] to #players|rank to free up room for a few more characters
                  );
			  } 
			  }
        }
     };
    
    class TeamPanelPainter : public ItemPainter
    {
        MissionInfo* m_pMission;
        
        TRef<Image> m_pimageIconPanel;
        
        int m_nWidth;
        int m_nHeight;
        
        
    public:
        TeamPanelPainter(MissionInfo* pMission, int nWidth, int nHeight) : 
          m_nWidth(nWidth), 
              m_nHeight(nHeight),
              m_pMission(pMission)
          {
              
          };
          
          int GetXSize()
          {
              return m_nWidth;
          }
          
          int GetYSize()
          { 
              return m_nHeight;
          }
          
          void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
          {
              SideInfo* pitem = (SideInfo*)pitemArg;
              
              IsideIGC* pside = NULL;
              
              if (trekClient.GetCore())
                  pside = trekClient.GetCore()->GetSide(pitem->GetSideID());
              int pnumberside = pitem->GetSideID();
              
              switch (pnumberside)
              {
              case 0:
                  m_pimageIconPanel = GetModeler()->LoadImage("iconteamyellowbmp", true);
                  break;
                  
              case 1:
                  m_pimageIconPanel = GetModeler()->LoadImage("iconteambluebmp", true);
                  break;
                  
              case 2:
                  m_pimageIconPanel = GetModeler()->LoadImage("iconteampurplebmp", true);
                  break;
                  
              case 3:
                  m_pimageIconPanel = GetModeler()->LoadImage("iconteamgreenbmp", true);
                  break;
                  
              case 4:
                  m_pimageIconPanel = GetModeler()->LoadImage("iconteamredbmp", true);
                  break;
                  
              case 5:
                  m_pimageIconPanel = GetModeler()->LoadImage("iconteamwhitebmp", true);
                  break;
              }
              
              psurface->BitBlt(WinPoint(0,0), m_pimageIconPanel->GetSurface());
              
              IcivilizationIGC* pciv = pside ? pside->GetCivilization() : NULL;
              ZString str;
              
              str = ZString("icon") + ZString(pciv->GetIconName()).ToLower() + ZString("bmp");
              TRef<Image> pimageCiv = GetModeler()->LoadImage(str, true);
              
              psurface->BitBlt(WinPoint(2, 2), pimageCiv->GetSurface());
          }
    };
    
    // filter out enemy drones which we have not seen
    static bool IsAllyOrHumanOrSpotted(ItemID pitem)
    {
        PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem));
        
        if (pplayer == NULL || trekClient.MyMission() == NULL)
            return false;
        
        return (pplayer->IsHuman() 
            || pplayer->LastSeenShipType() != NA 
            || pplayer->SideID() == trekClient.GetSideID() || (trekClient.GetSide()->AlliedSides(trekClient.GetSide(),pplayer->GetShip()->GetSide()) && trekClient.MyMission()->GetMissionParams().bAllowAlliedViz));  //Imago ALLY Visibility 7/6/09 7/11/09
    }

    static Color GetSideUIColor(SideID sideID)
    {
        switch (sideID)
        {
        case 0:
            return Color(152/256.0f, 108/256.0f, 15/256.0f);

        case 1:
            return Color(11/256.0f, 26/256.0f, 119/256.0f);

        case 2:
            return Color(130/256.0f, 15/256.0f, 103/256.0f);

        case 3:
            return Color(27/256.0f, 143/256.0f, 16/256.0f);

        case 4:
            return Color(146/256.0f, 13/256.0f, 18/256.0f);

        case 5:
            return Color(57/256.0f, 207/256.0f, 132/256.0f);

        default:
            // WLP 2005 - allow viewing NOAT lobby team - re-use case 5
           // assert(false);
           // return Color(0.5, 0.5, 0.5);

           return Color(131/256.0f, 131/256.0f, 140/256.0f); // yp your_persona march 14 2006 : NOATcolor patch change color from teal to NOAT grey.
        }
    }
    
    public:
        TeamPane(Modeler* pmodeler, const ZString& strPrefix)
            : m_peventPlayers(NULL), m_sortPlayers(sortLeader), m_pTeamPaneTwin(NULL)
        {
            TRef<IObject> pobjColumns;
            
            // a team pane is meaningless without a team
            if (trekClient.MyMission() == NULL)
                return;
            
            // Load the members from MDL
            
            TRef<INameSpace> pns = pmodeler->GetNameSpace("teampane");
            
            CastTo(m_pbuttoncomboTeam,  pns->FindMember(strPrefix + "ComboButtonPane"));
            CastTo(m_pcomboTeams,       pns->FindMember(strPrefix + "TeamComboPane"));
            CastTo(m_pcomboWing,        pns->FindMember(strPrefix + "WingComboPane"));
            CastTo(m_pbuttonBoot,       pns->FindMember(strPrefix + "BootButtonPane"  ));
            CastTo(m_pbuttonDonate,     pns->FindMember(strPrefix + "DonateButtonPane"));
            CastTo(m_pbuttonAutoDonate, pns->FindMember(strPrefix + "AutoDonateButtonPane"));
            CastTo(m_pbuttonStopDonate, pns->FindMember(strPrefix + "StopDonateButtonPane"));
            CastTo(m_pbuttonExpand,     pns->FindMember(strPrefix + "ExpandButtonPane"));
            CastTo(m_pbuttonTakeMeTo,   pns->FindMember(strPrefix + "TakeMeToButtonPane"));
            CastTo(m_pbuttonAccept,     pns->FindMember(strPrefix + "AcceptButtonPane"));
            CastTo(m_pbuttonClose,      pns->FindMember(strPrefix + "CloseButtonPane"));
            CastTo(m_plistPanePlayers,  (Pane*)pns->FindMember(strPrefix + "PlayerListPane"));
            CastTo(pobjColumns,         pns->FindMember(strPrefix + "PlayerColumns"));
            CastTo(m_pbuttonbarPlayers, pns->FindMember(strPrefix + "PlayerListHeader"));
            CastTo(m_plistPaneTeams,    (Pane*)pns->FindMember(strPrefix + "TeamListPane"));
            CastTo(m_plistPaneSelTeam,  (Pane*)pns->FindMember(strPrefix + "SelTeamListPane"));
            CastTo(m_plistPaneIcon,     (Pane*)pns->FindMember(strPrefix + "IconListPane"));
            
            //
            // Buttons
            //
            
			// mdvalley: Lots of '&TeamPane::' in this file. Client is rigged to crash if this button is used on mdvalley.
            if (m_pbuttonBoot)
                AddEventTarget(&TeamPane::OnButtonBoot,   m_pbuttonBoot->GetEventSource());
            
            if (m_pbuttonDonate)
            {
                m_pbuttonDonate->SetRepeat(0.1f, 0.5f);
                AddEventTarget(&TeamPane::OnButtonDonate, m_pbuttonDonate->GetEventSource());
				AddEventTarget(&TeamPane::OnRightButtonDonate, m_pbuttonDonate->GetRightEventSource());
            }
            
            if (m_pbuttonAutoDonate)
                AddEventTarget(&TeamPane::OnButtonAutoDonate, m_pbuttonAutoDonate->GetEventSource());
            
            if (m_pbuttonStopDonate)
                AddEventTarget(&TeamPane::OnButtonStopDonate, m_pbuttonStopDonate->GetEventSource()); // KGJV #110
            
            if (m_pbuttonAccept)
                AddEventTarget(&TeamPane::OnButtonAccept, m_pbuttonAccept->GetEventSource());
            
            if (m_pbuttonTakeMeTo)
                AddEventTarget(&TeamPane::OnButtonTakeMeTo, m_pbuttonTakeMeTo->GetEventSource());
            
            AddEventTarget(&TeamPane::OnButtonBack,  m_pbuttonClose->GetEventSource());
            AddEventTarget(&TeamPane::OnButtonBar,    m_pbuttonbarPlayers->GetEventSource());
            
            if (m_pbuttonExpand)
                AddEventTarget(&TeamPane::OnButtonSize, m_pbuttonExpand->GetEventSource());
            
            //
            // The player list
            //
            
            ParseIntVector(pobjColumns, m_viColumns);
            m_plistPanePlayers->SetItemPainter(new PlayerPainter(m_viColumns));
            m_peventPlayers = m_plistPanePlayers->GetSelectionEventSource();
            if (m_psinkPlayers)
                m_peventPlayers->RemoveSink(m_psinkPlayers);
            m_peventPlayers->AddSink(m_psinkPlayers = new IItemEvent::Delegate(this));
            
            //Xynth #48 8/2010
			AddEventTarget(&TeamPane::OnPlayerRightClicked, m_plistPanePlayers->GetSingleRightClickEventSource());
            
            
            
            //
            // Wing combo
            //
            
            for (WingID wingID = 0; wingID < c_widMax; wingID++)
            {
                m_pcomboWing->AddItem(
                    c_pszWingName[wingID],
                    wingID                
                    );
            }
            m_pcomboWing->SetSelection(trekClient.GetShip()->GetWingID());
            AddEventTarget(&TeamPane::OnWingCombo, m_pcomboWing->GetEventSource());
            
            //
            // Team combo
            //
                   //
            // Team combo
            //
            // WLP 2005 - modified this to show lobby
            //
            //  lobby is the last selection on menu
            //
            // for (SideID sideID = 0; sideID < trekClient.MyMission()->NumSides(); sideID++)
            for (SideID sideID = 0; sideID <= trekClient.MyMission()->NumSides(); sideID++)
            {
                Color color = GetSideUIColor(sideID);  //  use normal order here
                //
                // WLP 2005 - added else code to show lobby as last team selection
                //
                if ( sideID < trekClient.MyMission()->NumSides() )
				 {                        // WLP - the playing teams
                  TRef<IMenuItem> pitem =
                   m_pcomboTeams->AddItem(
                   CensorBadWords (trekClient.MyMission()->SideName(sideID)) + ZString("  (") + ZString(trekClient.MyMission()->SideNumPlayers(sideID)) + ZString(")"),
                   sideID,color);
				  pitem->SetColors(color,Color(1, 1, 1),color * 1.5f,Color(1, 1, 1));
				 }
			     else  // WLP - show the lobby tab
				 {
					 color=GetSideUIColor(-1);
				 TRef<IMenuItem> pitem =
                    m_pcomboTeams->AddItem(
               	    ZString("Not On A Team"), // WLP - just show the name
                    SIDE_TEAMLOBBY,color);  // WLP modded sideID to newSideID to allow lobby
				 pitem->SetColors(color,Color(1, 1, 1),color * 1.5f,Color(1, 1, 1));
				 }
            }
            
            AddEventTarget(&TeamPane::OnTeamCombo, m_pcomboTeams->GetEventSource());
            
            m_pcomboTeams->SetSelection(trekClient.GetSideID());
            OnTeamCombo(trekClient.GetSideID());
            OnSelectPlayer(NULL);
    }
    
    ~TeamPane()
    {
        if (m_peventPlayers)
        {
            m_peventPlayers->RemoveSink(m_psinkPlayers);
        }
    }
    
    void SetTwin(TeamPane* pTeamPaneTwin)
    {
        m_pTeamPaneTwin = pTeamPaneTwin;
    }
    
    //////////////////////////////////////////////////////////////////////////////
    //
    // Event handlers
    //
    //////////////////////////////////////////////////////////////////////////////
    
    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        if (pevent == m_peventPlayers) {
            OnSelectPlayer(pitem);
        } else if (pevent = m_peventTeams) {
            OnSelTeam(pitem);
        }
        return true;
    }
    
    void OnShipStatusChange(PlayerInfo* pplayer)
    {
        // this might have been someone buying a ship with turrets.
        OnSelectPlayer(m_plistPanePlayers->GetSelection());
    }
    
    void OnMoneyChange(PlayerInfo* pPlayerInfo)
    {
        if ((pPlayerInfo == trekClient.MyPlayerInfo()) 
            || (pPlayerInfo == m_plistPanePlayers->GetSelection()))
        {
            OnSelectPlayer(m_plistPanePlayers->GetSelection());
        }
    }


    int NumPlayersOnSide(IsideIGC* pside)
    {
        int nPlayers = 0;

        for (ShipLinkIGC* psl = pside->GetShips()->first(); 
            (psl != NULL); psl = psl->next())
        {
            PlayerInfo* pplayer = (PlayerInfo*)(psl->data()->GetPrivateData());
            
            if (pplayer->IsHuman())
                nPlayers++;
        }

        return nPlayers;
    }


    int MaxTeamSize()
    {
        const MissionParams& missionparams = trekClient.MyMission()->GetMissionParams();
        int nMaxTeamSize = missionparams.nMaxPlayersPerTeam;

        if (missionparams.iMaxImbalance != 0x7fff)
        {
            for (SideLinkIGC* psidel = trekClient.GetCore()->GetSides()->first(); 
                (psidel != NULL); psidel = psidel->next())
            {
                if (psidel->data()->GetActiveF())
                {
                    int nPlayers = NumPlayersOnSide(psidel->data());

                    if (nPlayers > 0 && nPlayers + missionparams.iMaxImbalance < nMaxTeamSize)
                        nMaxTeamSize = nPlayers + missionparams.iMaxImbalance;
                }
            }
        }

        return nMaxTeamSize;
    }


    bool CanAcceptJoiners()
    {
        return NumPlayersOnSide(trekClient.GetSide()) < MaxTeamSize();
    }


	//KGJV #110 - changed logic for donate
    bool OnSelectPlayer(ItemID pitem)
    {
        bool bEnableDonate = false;
        
        bool bEnableStopDonate = false;
        bool bHideStopDonate = true;
        
        bool bEnableBoot = false; 
        bool bHideBoot = true;
        
        bool bEnableAutoDonate = false;
        bool bHideAutoDonate = false;
        
        bool bEnableTakeMeTo = false;
        bool bHideTakeMeTo = false;
        
        bool bEnableAccept = false;
        bool bHideAccept = true;
        
        if (pitem != NULL)
		{
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem));
            if ( m_sideCurrent == trekClient.GetSideID())	// my side
            {
                bHideBoot = !trekClient.GetPlayerInfo()->IsTeamLeader();
                
                if (pplayer->SideID() == SIDE_TEAMLOBBY) // my side is NOAT
                {
                    if (trekClient.MyMission()->FindRequest(trekClient.GetSideID(), pplayer->ShipID()))
                    {
                        bHideTakeMeTo = true;
                        bHideAccept = false;
                        bEnableAccept = CanAcceptJoiners();
                        bEnableBoot = true;
                    }
                }
                else if (pplayer != trekClient.GetPlayerInfo()) // selected player is not me
                {
                    IshipIGC* pship = pplayer->GetShip();
                    
                    if (pplayer->IsHuman())
                    {
						// psd = to whom he is donating
                        IshipIGC*   psd = pship->GetAutoDonate();

						if (trekClient.GetPlayerInfo()->IsTeamLeader()) // i'm team leader
						{
							bHideStopDonate = false;
							bEnableStopDonate = true; /*((psd == NULL) ||
												 (psd == trekClient.GetShip())) &&
												(pship != trekClient.GetShip()->GetAutoDonate());*/
							bEnableAutoDonate = true;
						}
						else // i'm not team leader
						{
							bool bTargetIsLeader = pplayer->IsTeamLeader();
							// i'm not donating and selected is leader
							bEnableAutoDonate = (trekClient.GetShip()->GetAutoDonate() == NULL && bTargetIsLeader);
							bEnableStopDonate = false;
						}

                        bEnableDonate = (trekClient.GetMoney() > 0);
                    }
                    else
                    {
                        bEnableAutoDonate = false;
                        bEnableDonate = false;
                    }
                    
                    IshipIGC*   pshipDonatingTo = pship->GetAutoDonate();
                    
                    if (trekClient.GetPlayerInfo()->IsTeamLeader())
                    {
                        bEnableBoot = pplayer->IsHuman() 
                            && !trekClient.MyMission()->GetMissionParams().bLockTeamSettings;
                    }
                    
                    bEnableTakeMeTo = true;
                }
                else // selected player is me
                {
                    bHideAutoDonate = false;
					bHideStopDonate = true;
                    bEnableStopDonate = false; //trekClient.GetShip()->GetAutoDonate() != NULL;
                }
			}
            else
            {
                bEnableTakeMeTo = true;
            }
            /*
            bEnableTakeMeTo = (
            (trekClient.GetShip()->GetStation() != NULL
            && pplayer->GetShipStatus().GetSectorID() != NA)
            || (trekClient.GetShip()->GetCluster() != NULL 
            && pplayer->GetShipStatus().GetSectorID() == trekClient.GetCluster()->GetObjectID())
            );
            */

			// or allied side Imago ALLY 7/6/09 7/8/09 ## not drones
			if (trekClient.GetSide()->AlliedSides(trekClient.GetSide(),pplayer->GetShip()->GetSide()) && pplayer->IsHuman())  {
				bEnableDonate = (trekClient.GetMoney() > 0);
				
			}
        }
        
        if (m_pbuttonBoot)
        {
            m_pbuttonBoot->SetHidden(bHideBoot);
            m_pbuttonBoot->SetEnabled(bEnableBoot);
        }
        
        if (m_pbuttonTakeMeTo)
        {
            m_pbuttonTakeMeTo->SetHidden(bHideTakeMeTo);
            m_pbuttonTakeMeTo->SetEnabled(bEnableTakeMeTo);
        }
        
        if (m_pbuttonAccept)
        {
            m_pbuttonAccept->SetHidden(bHideAccept);
            m_pbuttonAccept->SetEnabled(bEnableAccept);
        }
        
        if (! Training::IsTraining ())
        {
            if (m_pbuttonDonate)
                m_pbuttonDonate->SetEnabled(bEnableDonate);
            
            if (m_pbuttonAutoDonate)
            {
                m_pbuttonAutoDonate->SetHidden(bHideAutoDonate);
                m_pbuttonAutoDonate->SetEnabled(bEnableAutoDonate);
            }
            
            if (m_pbuttonStopDonate)
            {
                m_pbuttonStopDonate->SetHidden(bHideStopDonate);
                m_pbuttonStopDonate->SetEnabled(bEnableStopDonate);
            }
        }
        m_pbuttonClose->SetEnabled(true);
        
        if (m_pTeamPaneTwin)
            m_pTeamPaneTwin->m_plistPanePlayers->SetSelection(pitem);
        
        return true;
    }

	//Xynth #48 8/2010 Add rightclick menu
	bool OnPlayerRightClicked()
	{
		ItemID pitem = m_plistPanePlayers->GetSelection();

        if (pitem != NULL && trekClient.GetPlayerInfo())
        {
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem));
			GetWindow()->ShowPlayerPaneContextMenu(pplayer);            
        }		
        return true;
	}
    
    bool OnSelTeam(ItemID pitem)
    {
        if (pitem == NULL)
        {
        // WLP 2005 - show LOBBY
        m_plistPaneTeams->SetSelection(trekClient.MyMission()->GetSideInfo(SIDE_TEAMLOBBY));
        }
        else
        {
            SideInfo* sideinfo = (SideInfo*)pitem;
            
            m_sideCurrent   = sideinfo->GetSideID();
            IsideIGC* pside = trekClient.GetCore()->GetSide(m_sideCurrent);
            
            OnTeamCombo(m_sideCurrent);
            m_plistPaneSelTeam->SetList(
                new SingletonList(trekClient.MyMission()->GetSideInfo(m_sideCurrent))
                );
            m_plistPaneIcon->SetList(
                new SingletonList(trekClient.MyMission()->GetSideInfo(m_sideCurrent))
                );
            
        }
        
        return true;
    }
    
    void OnAddPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        if (m_plistPaneSelTeam)
            m_plistPaneSelTeam->ForceRefresh();
        OnSelectPlayer(m_plistPanePlayers->GetSelection());
    }
    
    void OnDelPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
    {
        if (m_plistPaneSelTeam)
            m_plistPaneSelTeam->ForceRefresh();
        OnSelectPlayer(m_plistPanePlayers->GetSelection());
    }
    
    bool OnTeamCombo(int index)
    {
        m_sideCurrent = (SideID)index;
        
        //TRef<List> plistPlayers = trekClient.MyMission()->GetSideInfo(m_sideCurrent)->GetMemberList();
        TRef<List> plistPlayers;
        if (trekClient.GetPlayerInfo()->IsTeamLeader())
        {
            plistPlayers = new ConcatinatedList(
                trekClient.MyMission()->GetSideInfo(m_sideCurrent)->GetMemberList(),
                trekClient.MyMission()->GetSideInfo(m_sideCurrent)->GetRequestList()
                );
        }
        else
            plistPlayers = trekClient.MyMission()->GetSideInfo(m_sideCurrent)->GetMemberList();
        
        plistPlayers = new FilteredList<bool (*)(ItemID)>(plistPlayers, IsAllyOrHumanOrSpotted);
        
        switch (m_sortPlayers)
        {   
        case sortLeader:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, LeaderCompare);
            break;
            
        case sortStatus:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerRankCompare);
            break;
            
        case sortName:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerNameCompare);
            break;
            
        case sortShip:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerShipCompare);
            break;
            
        case sortSector:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerSectorCompare);
            break;
            
        case sortCredits:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerMoneyCompare);
            break;
            
        case sortObjective:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerObjectiveCompare);
            break;
            
        case sortRank:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerRankCompare);
            break;
            
        case sortWing:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerWingCompare);
            break;
            
        case sortDeaths:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerDeathsCompare);
            break;
            
        case sortKills:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerKillsCompare);
            break;
            
        case sortTurrets:
            plistPlayers = new SortedList<ItemIDCompareFunction>(plistPlayers, PlayerTurretsCompare);
            break;
        }
        
        m_plistPanePlayers->SetList(plistPlayers);
        
        bool    bEnemy = m_sideCurrent != trekClient.GetSideID();
        
        OnSelectPlayer(m_plistPanePlayers->GetSelection());
        
        
        if (m_pTeamPaneTwin && m_pTeamPaneTwin->m_pcomboTeams->GetSelection() != index)
        {
            m_pTeamPaneTwin->m_pcomboTeams->SetSelection(index);
        }
        
        return true;
    }
    
    bool OnWingCombo(int index)
    {
		trekClient.SaveWingAssignment(index); // kolie 6/10
        trekClient.SetWing(index);
        return true;                
    }
    
    class BootSink : public IIntegerEventSink {
    public:
        ShipID  m_shipID;

        BootSink(ShipID shipID) :
            m_shipID(shipID)
        {
        }

        bool OnEvent(IIntegerEventSource* pevent, int value)
        {
			if (value == IDOK) {
				SideID  sideID = trekClient.GetSideID();
				trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
				if (trekClient.MyMission()->FindRequest(trekClient.GetSideID(), m_shipID))
				{
					BEGIN_PFM_CREATE(trekClient.m_fm, pfmPosAck, C, POSITIONACK)
						END_PFM_CREATE
						pfmPosAck->shipID = m_shipID;
					pfmPosAck->fAccepted = false;
					pfmPosAck->iSide = sideID;
				}
				else
				{
					// if defections are allowed, boot them to the lobby side.
					if (trekClient.MyMission()->GetMissionParams().bAllowDefections)
					{
						BEGIN_PFM_CREATE(trekClient.m_fm, pfmQuitSide, CS, QUIT_SIDE)
							END_PFM_CREATE
							pfmQuitSide->shipID = m_shipID;
						pfmQuitSide->reason = QSR_LeaderBooted;
					}
					else
					{
						BEGIN_PFM_CREATE(trekClient.m_fm, pfmQuitMission, CS, QUIT_MISSION)
							END_PFM_CREATE
							pfmQuitMission->shipID = m_shipID;
						pfmQuitMission->reason = QSR_LeaderBooted;
					}
				}
			} // end OK was hit
            return false;
        }
    };

    bool OnButtonBoot()
    {
        ShipID  shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());

		//Imago sunk the boot into the message box above  7/22/08
		TRef<IMessageBox> m_pmessageBox;
		PlayerInfo* pplayer = trekClient.FindPlayer(shipID);
		ZString strMsg = "Boot " + ZString(pplayer->CharacterName()) + "?";
		if (m_pmessageBox == NULL) {
			m_pmessageBox = CreateMessageBox(strMsg, NULL, true, true);
			m_pmessageBox->GetEventSource()->AddSink(new BootSink(shipID));
			GetWindow()->GetPopupContainer()->OpenPopup(m_pmessageBox, false);
		}

        return true;
    }
    
    void OnPlayerStatusChange(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        if (trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
        {
            OnTeamCombo(m_sideCurrent);
            OnSelectPlayer(m_plistPanePlayers->GetSelection());
        }
    }
    
    bool OnButtonDonate()
    {
        if (m_plistPanePlayers->GetSelection() != NULL)
        {
            ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
            
            if (shipID != trekClient.GetShipID() && trekClient.GetMoney() > 0)
            {

				//imago 7/10/09
				if (trekClient.FindPlayer(shipID)->GetShip()->GetSide() != trekClient.GetSide())
					trekClient.PostText(false, "You gave %s $%d. You now have $%d.",
                    	  trekClient.FindPlayer(shipID)->CharacterName(), 
						  (trekClient.GetMoney() >= 100) ? 100 : trekClient.GetMoney(), 
						  (trekClient.GetMoney() >= 100) ? (trekClient.GetMoney() - 100) : 0);

                trekClient.DonateMoney(trekClient.FindPlayer(shipID), 
                    (trekClient.GetMoney() >= 100) ? 100 : trekClient.GetMoney());

            }                    
        }
        
        return true;
    }

	//Imago 7/6/09 large donations /w right click
    bool OnRightButtonDonate()
    {
        if (m_plistPanePlayers->GetSelection() != NULL)
        {
            ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
            
            if (shipID != trekClient.GetShipID() && trekClient.GetMoney() > 0)
            {

				//hack the sfx in, see NYI in trekmdl.cpp
				trekClient.PlaySoundEffect(positiveButtonClickSound);

				//imago 7/10/09
				if (trekClient.FindPlayer(shipID)->GetShip()->GetSide() != trekClient.GetSide())
					trekClient.PostText(false, "You gave %s $%d. You now have $%d.",
                    	 trekClient.FindPlayer(shipID)->CharacterName(), 
						  (trekClient.GetMoney() >= 500) ? 500 : trekClient.GetMoney(), 
						  (trekClient.GetMoney() >= 500) ? (trekClient.GetMoney() - 500) : 0);

				//Imago 7/6/09 large donations /w right click
                trekClient.DonateMoney(trekClient.FindPlayer(shipID), 
                    (trekClient.GetMoney() >= 500) ? 500 : trekClient.GetMoney());


            }                    
        }
        
        return true;
    }

	// KGJV #110
    bool OnButtonStopDonate()
    {
		if (!trekClient.MyPlayerInfo()->IsTeamLeader()) return true;// only leader can toggle someone to stop donating
        if ((m_plistPanePlayers->GetSelection() != NULL) && (trekClient.m_fm.IsConnected ()))
        {
	        ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
            //m_pbuttonAutoDonate->SetEnabled(false);
            //m_pbuttonStopDonate->SetEnabled(false);
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmAutoDonate, C, AUTODONATE)
            END_PFM_CREATE
            pfmAutoDonate->sidDonateTo = shipID; 
            pfmAutoDonate->amount = -1; // indicate to toggle stop donating for sidDonateTo
		}
		return true;
	}
	// KGJV #110 - logic change - does only "team deposit" now
    bool OnButtonAutoDonate()
    {
        ShipID  shipID;
        if ((m_plistPanePlayers->GetSelection() != NULL) && (trekClient.m_fm.IsConnected ()))
        {
            shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
			if (shipID == trekClient.GetShipID()) return true; // skip stop donating
			PlayerInfo* pplayer = trekClient.FindPlayer(shipID);

            m_pbuttonAutoDonate->SetEnabled(false);
            m_pbuttonStopDonate->SetEnabled(false);

            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
			trekClient.SetWing(trekClient.GetSavedWingAssignment()); //Imago 6/10 #91
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmAutoDonate, C, AUTODONATE)
                END_PFM_CREATE
                
                //if (shipID == trekClient.GetShipID())
                //{
                //    pfmAutoDonate->sidDonateTo = NA;
                //    pfmAutoDonate->amount = 0;
                //}
                //else
                {
                    pfmAutoDonate->sidDonateTo = shipID; 
                    pfmAutoDonate->amount = trekClient.GetMoney();
                    trekClient.SetMoney(0);
                }
        }
        
        return true;
    }
    
    bool OnButtonTakeMeTo()
    {
        if (m_plistPanePlayers->GetSelection() != NULL)
        {
            GetWindow()->TurnOffOverlayFlags(ofExpandedTeam | ofTeam);

            ShipID      shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
            PlayerInfo* pplayer = trekClient.FindPlayer(shipID);
            
            assert(pplayer);
            //
            // WLP 2005 - removed next line to allow lobby
            //
            // assert (pplayer->SideID() != SIDE_TEAMLOBBY);
            {
                if (Training::IsTraining())
                {
                    if (Training::CommandViewEnabled())
                    {
                        ImissionIGC*    pCore = trekClient.GetCore();
                        IshipIGC*       pShip = pCore->GetShip(shipID); //pplayer->GetShip() should be the same
                        IclusterIGC*    pCluster = pShip->GetCluster();
                        const Vector*   ppos;
                        if (!pCluster)
                        {
                            IstationIGC*    pStation = pShip->GetStation();
                            if (pStation) {
                                pCluster = pStation->GetCluster();
                                assert(pCluster);
                                debugf("Ship is in station %s in %s.\n", pStation->GetName(), pCluster->GetName());
                                ppos = pStation && pStation->SeenBySide(trekClient.GetSide()) ? &(pStation->GetPosition()) : NULL;
                            }
                            else {
                                debugf("Failed to get ship \"%s\" cluster OR station!\n", pShip->GetName());
                                ppos = NULL;
                            }
                        }
                        else
                            ppos = pShip && pShip->SeenBySide(trekClient.GetSide()) ? &(pShip->GetPosition()) : NULL;

                        IclusterIGC*    pClusterShip = trekClient.GetShip()->GetCluster();
                        if ((pClusterShip == NULL) || (pClusterShip == pCluster)) { //check if in station or in the same sector
                            if (GetWindow()->GetViewMode() != TrekWindow::vmCommand)
                                GetWindow()->SetViewMode(TrekWindow::vmCommand);
				
                            if (pClusterShip == NULL) { //player in base
                                //trekClient.RequestViewCluster (pCluster, pplayer->GetShip()); //fails for ships in stations
                                trekClient.SetViewCluster(pCluster, ppos);
                            }
                            else
                            {
                                assert(pClusterShip == pCluster);
                                GetWindow()->PositionCommandView(ppos, 2.0f);
                            }
                        }
                    }
                }
                else if (pplayer->GetShipStatus().GetSectorID() != NA)
                {
                    IclusterIGC*    pCluster = trekClient.GetCore()->GetCluster(pplayer->GetShipStatus().GetSectorID());
                    //GetWindow()->SetTarget(pplayer->GetShip(), c_cidDefault);
                    
                    IclusterIGC*    pClusterShip = trekClient.GetShip()->GetCluster();
                    if ((pClusterShip == NULL) || (pClusterShip == pCluster))
                    {
                        if (GetWindow()->GetViewMode() != TrekWindow::vmCommand)
                            GetWindow()->SetViewMode(TrekWindow::vmCommand);

                        if (pClusterShip == NULL) //player in base
                            trekClient.RequestViewCluster(pCluster, pplayer->GetShip());
                        else
                        {
                            assert (pClusterShip == pCluster);
                            GetWindow()->PositionCommandView(pplayer->GetShip()->SeenBySide(trekClient.GetSide())
                                                             ? &(pplayer->GetShip()->GetPosition())
                                                             : NULL,
                                                             2.0f);
                        }
                    }
                }

                GetWindow()->GetConsoleImage()->GetConsoleData()->PickShip(pplayer->GetShip()); //Select the ship
            }
        }
        
        return true;
    }
    bool OnButtonAccept()
    {
        if (m_plistPanePlayers->GetSelection() != NULL)
        {
            ShipID      shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
            PlayerInfo* pplayer = trekClient.FindPlayer(shipID);
            
            assert(pplayer);
            
            if (pplayer->SideID() == SIDE_TEAMLOBBY)
            {
                trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
                BEGIN_PFM_CREATE(trekClient.m_fm, pfmPosAck, C, POSITIONACK)
                    END_PFM_CREATE
                    pfmPosAck->shipID = shipID;
                pfmPosAck->fAccepted = true;
                pfmPosAck->iSide = trekClient.GetSideID();
            }

            m_pbuttonAccept->SetEnabled(false);
        }
        
        return true;
    }
    
    bool OnButtonBack()
    {
        if (GetWindow()->GetOverlayFlags() & ofTeam)
            GetWindow()->TurnOffOverlayFlags(ofTeam);
        else
            GetWindow()->TurnOffOverlayFlags(ofExpandedTeam);
        
        return true;
    }
    
    
    bool OnButtonSize()
    {
        if (GetWindow()->GetOverlayFlags() & ofTeam)
        {
            GetWindow()->TurnOffOverlayFlags(ofTeam);
            GetWindow()->TurnOnOverlayFlags(ofExpandedTeam);
        }
        else
        {
            GetWindow()->TurnOnOverlayFlags(ofTeam);
            GetWindow()->TurnOffOverlayFlags(ofExpandedTeam);
        }
        
        return true;
    }
    
    bool OnButtonBar(int iButton)
    {
        OnButtonBarImpl(iButton);
        if (m_pTeamPaneTwin)
            m_pTeamPaneTwin->OnButtonBarImpl(iButton);
        
        return true;
    }
    
    bool OnButtonBarImpl(int iButton)
    {
        // do radio-button behavior
        
        for (int i = 0; i < m_viColumns.GetCount(); i++) {
            if (i != iButton) {
                m_pbuttonbarPlayers->SetChecked(i, false);
            }
        }
        
        if (iButton < m_viColumns.GetCount()) {
            m_pbuttonbarPlayers->SetChecked(iButton, true);
        }
        
        switch (iButton) {
            //case 0:
            //    m_sortPlayers = sortStatus;
            //    break;
            
        case 0:
            m_sortPlayers = sortName;
            break;
            
        case 3:
            m_sortPlayers = sortSector;
            break;
            
        case 4:
            m_sortPlayers = sortShip;
            break;
            
        case 2:
            m_sortPlayers = sortKills;
            break;
            
        case 6:
            m_sortPlayers = sortCredits;
            break;
            
        case 7:
            m_sortPlayers = sortObjective;
            break;
            
        case 1:
            m_sortPlayers = sortRank;
            break;
            
        case 5:
            m_sortPlayers = sortWing;
            break;
            
            //case 8:
            //    m_sortPlayers = sortDeaths;
            //    break;
            
            
            
            //case 2:
            //    m_sortPlayers = sortTurrets;
            //    break;
        }
        
        OnTeamCombo(m_sideCurrent);
        
        return true;
    }
    
    static bool SideCompare(ItemID pitem1, ItemID pitem2)
    {
        SideInfo* sideInfo1 = (SideInfo*)pitem1;
        SideInfo* sideInfo2 = (SideInfo*)pitem2;
        
        return sideInfo1->GetSideID() > sideInfo2->GetSideID();
    }
    
    static bool LeaderCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        if (pplayer2->IsTeamLeader() != pplayer2->IsTeamLeader())
            return pplayer2->IsTeamLeader();
        else 
            return pplayer2->IsHuman() && !pplayer1->IsHuman();
    }
    
    static bool PlayerStatusCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return pplayer1->LastSeenState() > pplayer2->LastSeenState();
    }
    
    static bool PlayerNameCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        if (pplayer1->IsHuman() && pplayer2->IsHuman())
            return _stricmp(pplayer1->CharacterName(), pplayer2->CharacterName()) > 0;
        else if (pplayer1->IsHuman())
            return false;
        else if (pplayer2->IsHuman())
            return true;
        else
            return _stricmp(pplayer1->CharacterName(), pplayer2->CharacterName()) > 0;
    }
    
    static bool PlayerShipCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return _stricmp(ShipName(pplayer1), ShipName(pplayer2)) > 0;
    }
    
    static bool PlayerSectorCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return _stricmp(SectorName(pplayer1->LastSeenSector()), SectorName(pplayer2->LastSeenSector())) > 0;
    }
    
    static bool PlayerMoneyCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return pplayer1->GetMoney() > pplayer2->GetMoney();
    }
    
    static bool PlayerObjectiveCompare(ItemID pitem1, ItemID pitem2)
    {
        IshipIGC* pship1 = trekClient.GetCore()->GetShip(IntItemIDWrapper<ShipID>(pitem1));
        IshipIGC* pship2 = trekClient.GetCore()->GetShip(IntItemIDWrapper<ShipID>(pitem2));
        
        return _stricmp(CurrentCommandText(pship1), CurrentCommandText(pship2)) > 0;
    }

    static bool PlayerRankCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return pplayer1->Rank() < pplayer2->Rank();
    }
    
    static bool PlayerWingCompare(ItemID pitem1, ItemID pitem2)
    {
        IshipIGC* pship1 = trekClient.GetCore()->GetShip(IntItemIDWrapper<ShipID>(pitem1));
        IshipIGC* pship2 = trekClient.GetCore()->GetShip(IntItemIDWrapper<ShipID>(pitem2));
        
        return pship1->GetWingID() > pship2->GetWingID();
    }
    
    static bool PlayerDeathsCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return pplayer1->MissionDeaths() > pplayer2->MissionDeaths();
    }
    
    static bool PlayerKillsCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return pplayer1->MissionKills() > pplayer2->MissionKills();
    }
    
    static bool PlayerTurretsCompare(ItemID pitem1, ItemID pitem2)
    {
        IshipIGC* pship1 = trekClient.GetCore()->GetShip(IntItemIDWrapper<ShipID>(pitem1));
        IshipIGC* pship2 = trekClient.GetCore()->GetShip(IntItemIDWrapper<ShipID>(pitem2));
        
        return NumTurrets(pship1) > NumTurrets(pship2);
    }
    
    static ZString SectorName(SectorID id)
    {
        IclusterIGC* psector = trekClient.GetCore()->GetCluster(id);
        
        if (psector == NULL)
            return "unknown";
        else
            return psector->GetName();
    }
    
    
    static ZString ShipName(PlayerInfo* pplayer)
    {
        ZString strShip;
        
        /*
        if (pplayer->LastSeenState() == c_ssObserver)
        {
        PlayerInfo* pplayerParent = trekClient.FindPlayer(pplayer->LastSeenParent());
        
          if (pplayerParent)
          strShip = ZString("Observer of ") + pplayerParent->CharacterName();
          else
          strShip = "Observer";
          }
          else if (pplayer->LastSeenState() == c_ssTurret)
          {
          PlayerInfo* pplayerParent = trekClient.FindPlayer(pplayer->LastSeenParent());
          
            if (pplayerParent)
            strShip = ZString("Turret of ") + pplayerParent->CharacterName();
            else
            strShip = "Turret";
            }
        */
        if (pplayer->LastSeenState() != c_ssObserver && pplayer->LastSeenState() != c_ssTurret)
        {
            IhullTypeIGC* phull = trekClient.GetCore()->GetHullType(pplayer->LastSeenShipType());
            
            if (phull == NULL)
                strShip = "<none>";
            else
                strShip = phull->GetName();
        }
        
        return strShip;
    }
    
    static int MannedTurrets(IshipIGC* pshipParent)
    {
        // loop through all of the ships on that side and count the ones 
        // that are turrets of this.
        int cMannedTurrets = 0;
        
        const ShipListIGC* shipList = pshipParent->GetSide()->GetShips();
        for (const ShipLinkIGC* lShip = shipList->first(); lShip; lShip = lShip->next())
        {
            IshipIGC* pship = lShip->data();
            PlayerInfo* pplayer = (PlayerInfo*)pship->GetPrivateData();
            
            if (pplayer->LastSeenState() == c_ssTurret)                
            {
                PlayerInfo* pplayerParent = trekClient.FindPlayer(pplayer->LastSeenParent());

                if (pplayerParent && pplayerParent->GetShip() == pshipParent)
                    cMannedTurrets++;
            }
        }
        
        return cMannedTurrets;
    }
    
    static int NumTurrets(IshipIGC* pship)
    {
        PlayerInfo* pPlayer = (PlayerInfo*)pship->GetPrivateData();
        const IhullTypeIGC* pHullType = trekClient.GetCore()->GetHullType(pPlayer->LastSeenShipType());
        
        // if we have a real ship...
        if (pHullType)
            return pHullType->GetMaxWeapons() - pHullType->GetMaxFixedWeapons();
        else
            return 0;
    }
    
    static ZString CurrentCommandText(IshipIGC* pship)
    {
        CommandID   cid = pship->GetCommandID(c_cmdAccepted);
        ImodelIGC*  pmodelTarget = pship->GetCommandTarget(c_cmdAccepted);
        
        if (!pship->LegalCommand(cid, pmodelTarget))
        {
            return "";
        }
        else if (pmodelTarget)
        {
            return c_cdAllCommands[cid].szVerb + ZString(" ") 
                + GetModelName(pmodelTarget);
        }
        else
        {
            return c_cdAllCommands[cid].szVerb;
        }
    }
    
    static ZString CurrentCommandIcon(IshipIGC* pship)
    {
        CommandID   cid = pship->GetCommandID(c_cmdAccepted);
        ZString  m_pstringIcon;
        
        switch (cid)
        {
            
        case 1:
            m_pstringIcon = ZString("ackillbmp");
            break;
            
        case 2:
            m_pstringIcon = ZString("accptbmp");
            break;
            
        case 3:
            m_pstringIcon = ZString("acdefendbmp");
            break;
            
        case 4:
            m_pstringIcon = ZString("acpickupbmp");
            break;
            
        case 5:
            m_pstringIcon = ZString("acgotobmp");
            break;
            
        case 6:
            m_pstringIcon = ZString("acrepairbmp");
            break;
            
        case 7:
            m_pstringIcon = ZString("acjoinbmp");
            break;
            
        case 8:
            m_pstringIcon = ZString("acminebmp");
            break;
            
        case 9:
            m_pstringIcon = ZString("acbuildbmp");
            break;
            
        default:
            m_pstringIcon = ZString("none");
            break;
            
        }
        return m_pstringIcon;
    }
    
    
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<IObject> CreateTeamPane(Modeler* pmodeler)
{
    return (IItemEvent::Sink*)new TeamPane(pmodeler, "teamPane");
}


//////////////////////////////////////////////////////////////////////////////
//
// Expanded Team Pane
//
//////////////////////////////////////////////////////////////////////////////

class ExpandedTeamPane : public TeamPane
{
    TRef<ButtonPane>         m_pbuttonCollapse;
    
    class ExpandedPlayerPainter : public TeamPane::PlayerPainter
    {
        const TVector<int>& m_viColumns;
        
    public:
        
        ExpandedPlayerPainter(const TVector<int>& viColumns)
            : m_viColumns(viColumns), PlayerPainter(viColumns) {};
        
        int GetXSize()
        {
            return m_viColumns[7] - 15;
        }
        
        int GetYSize()
        {
            return 15;
        }
        

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitemArg));
            
            if (pplayer == NULL || trekClient.MyMission() == NULL)
                return;
            
            IshipIGC* pship = trekClient.GetCore()->GetShip(pplayer->ShipID());
            
            // draw the selection bar
            
            if (bSelected) {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(1, 0, 0)
                    );
            }
            
            //PlayerPainter::Paint(pitemArg, psurface, bSelected, bFocus);
            
            // draw the objective
            
            
            
            //psurface->SetTextColor(Color::White());
            //psurface->SetFont(TrekResources::SmallFont());
            
            //psurface->DrawString(WinPoint(m_viColumns[4] + 2, 0), 
            //    CurrentCommandText(pship));
            
            TRef<Image> pimageObj;
            
            if (CurrentCommandIcon(pship) != "none")
            {
                pimageObj = GetModeler()->LoadImage(CurrentCommandIcon(pship), true);
            }
            
            if (pimageObj){
                psurface->BitBlt(
                    WinPoint(m_viColumns[6] + 2, -1),
                    pimageObj->GetSurface()
                    );
            }

            DrawAutoDonateIcon(pplayer, psurface);
            
            // draw the name
            
            TRef<IEngineFont> pfont;
            Color             color;
            
            if (pplayer->LastSeenState() == c_ssDocked)
            {
                pfont = TrekResources::SmallBoldFont();
                color = Color(0.617f, 0.0f, 0.156f);
            }
            else
            {
                pfont = TrekResources::SmallFont();
                color = Color::White();
            }
			// yp: show player name with rank attached if is human  // cut by aem
            ZString pzsPlayerDisplayName = ZString(pplayer->CharacterName());
            //if (pplayer->IsHuman()) // this is a human, attach rank to name
                //pzsPlayerDisplayName += ZString("[")+ ZString( pplayer->GetPersistScore(NA).GetRank())+ZString("]");
			// yp end // end aem
            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(0, 0), WinPoint(120, 20))); // clip name to fit in column
            psurface->DrawString(
                pfont, color,   
                WinPoint(17, 2),
                pzsPlayerDisplayName
                );
            psurface->RestoreClipRect(rectClipOld);
            
            
			// draw the player's Wing
			if (pplayer->SideID() == trekClient.GetSideID())
			{
				// BT - 7/15 - When the team screen is shown, miners have -1 for the wingID, which causes alleg to crash.
				int wingID = pship->GetWingID();
				if (wingID != NA)
					psurface->DrawString(pfont, color, WinPoint(m_viColumns[4] + 2, 1), c_pszWingName[wingID]);
			}

			// draw the rank: AEM 7.21.07
            
			if (pplayer->IsHuman())
			{
				short checkRank(pplayer->GetPersistScore(NA).GetRank());
				ZString strRankF6 = checkRank;
				if (checkRank<10)
					strRankF6 = " " + strRankF6;
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[0] + 2, 1), strRankF6);
			}
			else
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[0] + 2, 1), ZString(""));

            // draw the deaths
            
            if (pplayer->MissionDeaths())
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[1] + 27, 1), pplayer->MissionDeaths());
            else
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[1] + 27, 1), ZString("0"));
            
            // draw the kills
            
            if (pplayer->MissionKills())
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[1] + 2, 1), pplayer->MissionKills());
            else
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[1] + 2, 1), ZString("0"));
            
            // draw the sector location or turret/observer location if aboard a ship
            ZString strLocation;
            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[2] + 2, 0), WinPoint(m_viColumns[3], GetYSize()))); // clip status to fit in column

            if (pplayer->LastSeenState() == c_ssObserver || pplayer->LastSeenState() == c_ssTurret )
            {
                PlayerInfo* pplayerParent = trekClient.FindPlayer(pplayer->LastSeenParent());

                if (pplayerParent)
                    strLocation = ZString("aboard ") + pplayerParent->CharacterName();
                else
                    strLocation = SectorName(pplayer->LastSeenSector());
            }
            else 
            {
                strLocation = SectorName(pplayer->LastSeenSector());
            }
            psurface->DrawString(pfont, color, WinPoint(m_viColumns[2] + 2, 0), strLocation);

            psurface->RestoreClipRect(rectClipOld);
            
            const IhullTypeIGC* pHullType = trekClient.GetCore()->GetHullType(pplayer->LastSeenShipType());
            ZString piconname;
            TRef<Image> pimageicon;
            if (pplayer->LastSeenState() == c_ssObserver)
            {
                pimageicon = GetModeler()->LoadImage(AFW_TEAM_OBSERVER_ICON, true);
            }
            else if (pplayer->LastSeenState() == c_ssTurret)
            {
                pimageicon = GetModeler()->LoadImage(AFW_TEAM_TURRET_ICON, true);
            }
            else if (!pHullType)
            {
                // we should only see this when the hull type is NA
                assert(pplayer->LastSeenShipType() == NA);
                
                pimageicon = GetModeler()->LoadImage("unknownshipbmp", true);
            }
            else if (pHullType->GetIconName())
            {
                piconname = pHullType->GetIconName() + ZString("bmp");
                pimageicon = GetModeler()->LoadImage(piconname, true);
            }
            else
            {
                pimageicon = GetModeler()->LoadImage("ejectpodbmp", true);
            }
            
            if (pimageicon){
                psurface->BitBlt(
                    WinPoint(m_viColumns[3] + 10, -1),
                    pimageicon->GetSurface()
                    );
            }
            
            //draw the money
            if (pplayer->IsHuman() && pplayer->SideID() == trekClient.GetSideID())
            {
                char cbTemp[256];
                wsprintf(cbTemp, "%d", pplayer->GetMoney());
                psurface->DrawString(
                    pfont,
                    color,
                    WinPoint(m_viColumns[5] + 2, 1),
                    cbTemp
                    );
            }
            
            
            
            
        }
    };
    
public:
    ExpandedTeamPane(Modeler* pmodeler, ZString strPrefix)
        
        : TeamPane(pmodeler, strPrefix)
    {
        // a team pane is meaningless without a team
        if (trekClient.MyMission() == NULL)
            return;
        
        TRef<INameSpace> pns = pmodeler->GetNameSpace("teampane");
        
        CastTo(m_pbuttonCollapse,     pns->FindMember(strPrefix + "CollapseButtonPane"));
        
        AddEventTarget(&TeamPane::OnButtonSize, m_pbuttonCollapse->GetEventSource());
        
        
        m_plistPanePlayers->SetItemPainter(new ExpandedPlayerPainter(m_viColumns));
        //
        //  The Team Tab list pane
        //
        
        m_peventTeams = m_plistPaneTeams->GetSelectionEventSource();
        m_peventTeams->AddSink(m_psinkTeams = new IItemEvent::Delegate(this));
        m_plistPaneTeams->SetList(
              // WLP 2005 - tacked TEAM LOBBY onto the list of team names 
                new ConcatinatedList(
			    new SortedList<ItemIDCompareFunction>(trekClient.MyMission()->GetSideList(), SideCompare),
                new SingletonList(trekClient.MyMission()->GetSideInfo(SIDE_TEAMLOBBY))
				));
            //  WLP
        m_plistPaneTeams->UpdateLayout();
        m_plistPaneTeams->SetItemPainter(new TeamPainter(
            trekClient.MyMission(), 
            m_plistPaneTeams->XSize(),
            m_plistPaneTeams->YSize() / (c_cSidesMax + 1),
            false
            ));
        
        m_plistPaneTeams->SetSelection(trekClient.MyMission()->GetSideInfo(trekClient.GetSideID()));
        
        m_plistPaneSelTeam->SetList(
            new SingletonList(trekClient.MyMission()->GetSideInfo(trekClient.GetSideID()))
            );
        m_plistPaneSelTeam->UpdateLayout();
        m_plistPaneSelTeam->SetItemPainter(new TeamPainter(
            trekClient.MyMission(),
            m_plistPaneTeams->XSize(),
            m_plistPaneTeams->YSize(),
            true
            ));
        
        m_plistPaneIcon->SetList(
            new SingletonList(trekClient.MyMission()->GetSideInfo(trekClient.GetSideID()))
            );
        m_plistPaneIcon->UpdateLayout();
        m_plistPaneIcon->SetItemPainter(new TeamPanelPainter(
            trekClient.MyMission(),
            m_plistPaneIcon->XSize(),
            m_plistPaneIcon->YSize()
            ));
    }
};


//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<IObject> CreateExpandedTeamPane(Modeler* pmodeler, IObject* pNormalTeamPane)
{   
    ExpandedTeamPane* pExpandedTeamPane = new ExpandedTeamPane(pmodeler, "eTeamPane");
    
    
    if (pNormalTeamPane)
    {
        TeamPane* pteam; CastTo(pteam, (Pane*)pNormalTeamPane);
        
        pteam->SetTwin(pExpandedTeamPane);
        pExpandedTeamPane->SetTwin(pteam);
    }
    
    return (IItemEvent::Sink*)pExpandedTeamPane;
}
