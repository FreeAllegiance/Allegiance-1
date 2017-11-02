#ifndef _screens_h_
#define _screens_h_

//////////////////////////////////////////////////////////////////////////////
//
// Screen
//
//////////////////////////////////////////////////////////////////////////////

class Screen : public IObject {
public:
    virtual Image*   GetImage() { return NULL; }
    virtual Pane*    GetPane()  { return NULL; }
    virtual WinPoint GetSize()  { return WinPoint(800, 600); }
    virtual void     OnFrame()  { } //ADDED Sleep to preserve CPU cycles when not in-game 12/23/07 -- Orion 
										//Imago removed the Sleep(1), it added 1 ms/f, all the time! 7/13/09
											//See the new, more agressive sleepers at the ends of DoTrekUpdate and and DoIdle
};

//////////////////////////////////////////////////////////////////////////////
//
// Screen Constructors
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateTeamScreen(Modeler* pmodeler);
TRef<Screen> CreateGameScreen(Modeler* pmodeler);
TRef<Screen> CreateIntroScreen(Modeler* pmodeler);
TRef<Screen> CreateNewGameScreen(Modeler* pmodeler);
TRef<Screen> CreateGameOverScreen(Modeler* pmodeler);
TRef<Screen> CreateLeaderBoardScreen(Modeler* pmodeler, ZString strCharacter);
TRef<Screen> CreateZoneEventsScreen(Modeler* pmodeler);
TRef<Screen> CreateZoneClubScreen(Modeler* pmodeler, Number * ptime);
TRef<Screen> CreateSquadsScreen(Modeler* pmodeler, const char * szNameDudeX, int idZoneDudeX, const char * szSquad);
TRef<Screen> CreateCharInfoScreen(Modeler* pmodeler, int idZone); // if id is NA, current player is used
TRef<Screen> CreateHangarScreen(Modeler* pmodeler, const ZString& strNamespace);
TRef<Screen> CreateTrainingScreen(Modeler* pmodeler);
TRef<Screen> CreateTrainingSlideshow (Modeler* pmodeler, const ZString& strNamespace, int iMissionIndex);
TRef<Screen> CreatePostTrainingSlideshow (Modeler* pmodeler, const ZString& strNamespace);
TRef<Screen> CreateGameStartingScreen(Modeler* pmodeler);

// BUILD_DX9
// Taken out of D3D9 build for now, movies not supported at the moment.  //Imago use DDVideo
// TRef<Screen> CreateVideoScreen(Modeler* pmodeler, bool bIntroOnly);
// BUILD_DX9

TRef<IPopup> CreateMissionParametersPopup(Modeler* pmodeler);

extern bool g_bQuickstart;
extern bool g_bReloaded;   // as in after AutoUpdate
extern int  g_civStart;
extern bool bStartTraining;

extern char g_szCharName[c_cbName];
extern char g_szCharPW[c_cbName];

void LaunchFromHangar();

#endif
