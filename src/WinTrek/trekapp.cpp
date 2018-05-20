#include "pch.h"
#include <objbase.h>
#include <malloc.h>

// BT - STEAM
#ifdef STEAM_APP_ID
# include "atlenc.h"
# include <inttypes.h>
#endif


//////////////////////////////////////////////////////////////////////////////
//
// Include the main function
//
//////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "regkey.h"

#include <fstream>

// BUILD_DX9
#include "VideoSettingsDX9.h"
// BUILD_DX9

extern bool g_bEnableSound = true;
extern bool bStartTraining   = false;
extern bool g_bCheckFiles;
extern bool g_fZoneAuth;
bool    g_bSkipAutoUpdate = false;
bool    g_bDownloadZoneMessage = true;
bool    g_bDisableZoneClub = false;
bool    g_bDisableNewCivs = true;
bool    g_bQuickstart = false;
bool    g_bReloaded = false;
int     g_civStart    = -1;
bool    g_bDownloadNewConfig = true;
bool    g_bAskForCDKey = 
//#ifdef USEAUTH // We don't bother with CD keys anymore.
//  true;
//#else
  false;
//#endif
// wlp 2006 - added askforcallsign - don't ask if passed in on commandline
bool g_bAskForCallSign = true ; // wlp 2006
//////////////////////////////////////////////////////////////////////////////
//
// Trek Application Implementation
//
//////////////////////////////////////////////////////////////////////////////
#ifdef _ALLEGIANCE_PROD_
#define GAME_REG_KEY   "Software\\Microsoft\\Microsoft Games\\Allegiance\\1.0"
#else
#define GAME_REG_KEY        "Software\\Microsoft\\Microsoft Games\\Allegiance\\1.1"
#endif

typedef DWORD (*EBUPROC) (LPCTSTR lpRegKeyLocation, LPCTSTR lpEULAFileName, LPCSTR lpWarrantyFileName, BOOL fCheckForFirstRun);





//
// EULA related files should be in the artwork folder so that they may be autoupdated
//

// yp your_persona march 25 2006 : Remove EULA.dll dependency patch
//
//HRESULT FirstRunEula(PathString strArtPath)
//{
//    TCHAR   szEULA[MAX_PATH];
//    if (UTL::getFile("eula", ".rtf", szEULA, false, false) != S_OK)
//        return false;
//
//    // don't use += operator cause it's buggy with PathString
//    strArtPath = strArtPath + "EBUEula.dll";
//
//    HINSTANCE hMod = LoadLibrary(PCC(strArtPath));
//    if (NULL == hMod)       // can't attach to DLL
//    {
//        // this time, search path
//        hMod = LoadLibrary("EBUEula.dll");
//        if (NULL == hMod)       // can't attach to DLL
//          return E_FAIL;
//    }
//
//    EBUPROC pfnEBUEula = (EBUPROC) GetProcAddress(hMod, "EBUEula");
//    if (NULL == pfnEBUEula)     // can't find entry point
//    {
//        FreeLibrary(hMod);
//        return E_FAIL;
//    }
//
//    /*
//    TCHAR   szWarranty[MAX_PATH];
//    LoadString(GetModuleHandle(), STR_EULAFILENAME, szEULA, sizeof(szEULA));
//    LoadString(GetModuleHandle(), STR_WARRANTYNAME, szWarranty, sizeof(szWarranty));
//
//    //
//    //This call enables both EULA and warranty accepting/viewing/printing.  If your
//    //game doesn't ship with a WARRANTY file, specifiy NULL instead of szWarranty…
//    //The code below, for instance, works with both OEM and retail builds…
//    //
//    TCHAR *pszWarrantyParam = 0xFFFFFFFF != GetFileAttributes(szWarranty) ? szWarranty : NULL;
//    */
//    bool fAllowGameToRun = pfnEBUEula(GAME_REG_KEY, szEULA, NULL, TRUE) != 0;
//
//    FreeLibrary(hMod);
//
//    return (fAllowGameToRun ? S_OK : S_FALSE);
//}

//
// Check to make sure that they are running DX 7 Dsound or better
//
bool CheckDSoundVersion()
{
    // Get version information from the application 
    
    HMODULE hmodDSound = GetModuleHandle("dsound");
    if (NULL == hmodDSound) // why isn't the dll loaded???
      return false;

    char  szDSoundPath[MAX_PATH];
    GetModuleFileName(hmodDSound, szDSoundPath, sizeof(szDSoundPath)); 

    DWORD dwTemp;
    DWORD dwVerInfoSize = GetFileVersionInfoSize(szDSoundPath, &dwTemp);
    if (0 == dwVerInfoSize)
      return false;

    void *pvVerInfo = _alloca(dwVerInfoSize);
    GetFileVersionInfo(szDSoundPath, NULL, dwVerInfoSize, pvVerInfo);

    VS_FIXEDFILEINFO *lpvsFixedFileInfo = NULL;
    unsigned uTemp;
    if (!VerQueryValue(pvVerInfo, "\\", (LPVOID*) &lpvsFixedFileInfo, &uTemp))
      return false;


    // check the version info

    WORD ver1 = HIWORD(lpvsFixedFileInfo->dwFileVersionMS);
    WORD ver2 = LOWORD(lpvsFixedFileInfo->dwFileVersionMS);
    WORD ver3 = HIWORD(lpvsFixedFileInfo->dwFileVersionLS);
    WORD ver4 = LOWORD(lpvsFixedFileInfo->dwFileVersionLS);

    if (ver1 > 5) // assume all major new versions work
      return true;

    if (ver1 == 5) // NT -- WARNING -- if future versions of DX go outside 4.??? and spill into NT versions, we're in trouble
      return ver2 > 0 || ver3 >= 2113; // ver3 == NT build #
    else if (ver1 == 4) // Stand-alone DX runtimes
    {
      if (ver2 > 7) // new major dx versions (DX8+)
        return true;
      else if (ver2 == 7) // DX7
        return ver3 > 0 || ver4 >= 700;
    }

    return false;
}

//
// Check to make sure that they are running DX 7 Dsound or better
//
bool CheckFreeMemory()
{
    const int nAppSize = 40 * 1024 * 1024;

    // first, try a non-invasive check to see if we would fit in the current
    // swap file.  
    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);

    if (ms.dwAvailVirtual == -1 || ms.dwAvailPhys == -1 
        || ms.dwAvailVirtual + ms.dwAvailPhys >= nAppSize)
    {
        return true;
    }

    // if that fails, try allocating a big chunk of memory to try to force the 
    // swap file to grow.
    void *pv = VirtualAlloc(NULL, nAppSize, MEM_COMMIT, PAGE_NOACCESS);

    if (pv)
    {
        VirtualFree(pv, 0, MEM_RELEASE);
        return true;
    }
    else
        return false;
}

ZString ReadAuthPipe()
{
	const int LENGTH = 64;
	HANDLE hWrite;
	DWORD nDataLength;
	DWORD nWritten;
	DWORD *nRead;
	char buffer[LENGTH];
	char memoryLocation[LENGTH];

	buffer[0] = '\0';

	sprintf(memoryLocation, "%ld", (long)&buffer);
	nDataLength = strlen(memoryLocation) + 1;

	debugf("sending memory location: %s\r\n", memoryLocation);

	hWrite = CreateFile(_T("\\\\.\\pipe\\allegqueue"), 
		FILE_GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);

	if(WriteFile(hWrite, memoryLocation, nDataLength, &nWritten, NULL) == false || nDataLength != nWritten)
		debugf("Couldn't write memory address to named pipe for key relay.\r\n");

	CloseHandle(hWrite);

	for(int i = 0; i < 100 && strlen(buffer) == 0; i++)
		Sleep(100);

	if(strlen(buffer) == 0)
		debugf("Remote process didn't deliver key to memory location within 10 seconds.\r\n"); 
	
	debugf("received key length: %ld, %s\r\n", strlen(buffer), buffer);

	return ZString(buffer);
};



class TrekAppImpl : public EffectApp {
public:
    TrekAppImpl()
    {
        AddRef();
    }

	// BT - STEAM
	ZString CleanUpSteamName(ZString &personaName)
	{
		char tempBuffer[25];
		Strncpy(tempBuffer, personaName, 24);

		tempBuffer[24] = '\0';

		int underscoreCount = 0;

		for (int i = 0; i < strlen(tempBuffer); i++)
		{
			//  if the character is not within an allowed range, then replace it out. 
			if (!(		 tempBuffer[i] == '_' 
					||	(tempBuffer[i] >= '0' && tempBuffer[i] <= '9') 
					||	(tempBuffer[i] >= 'A' && tempBuffer[i] <= 'Z') 
					||	(tempBuffer[i] >= 'a' && tempBuffer[i] <= 'z'))
				)
			{
				switch (tempBuffer[i])
				{
				/*case ' ':
					tempBuffer[i] = '_';
					break;
				case '!':
					tempBuffer[i] = 'l';
					break;

				case '@':
					tempBuffer[i] = 'a';
					break;

				case '#':
					tempBuffer[i] = 'H';
					break;

				case '$':
					tempBuffer[i] = 'S';
					break;

				case '%':
					tempBuffer[i] = 'X';
					break;

				case '&':
					tempBuffer[i] = 'n';
					break;*/

				default:
					tempBuffer[i] = '_';
					underscoreCount++;
					break;
				}
			}
		}

		// Prevent an all _ character name. (Thanks Door!) 
		if (underscoreCount == strlen(tempBuffer))
			strcpy(tempBuffer, "");

		return ZString(tempBuffer);
	}

    HRESULT Initialize(const ZString& strCommandLine)
    {
        _controlfp(_PC_53, _MCW_PC);
#ifdef STEAM_APP_ID
		// BT - STEAM
#ifndef _DEBUG
		if (IsDebuggerPresent() == false)
		{
			if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID) == true)
				::exit(-1);
		}
#endif

		bool steamInitResult = SteamAPI_Init();
		if (steamInitResult == false)
		{
			// If you are debugging locally, then you need to put steam_appid.txt with 700480 in it next to the Allegaince EXE that you are debugging.
			::MessageBoxA(NULL, "Steam Client is not running. Please launch Steam and try again.", "Error", MB_ICONERROR | MB_OK);
			::exit(-1);
		}
#endif
        //
        // Make sure reloader finished correctly--this must be first before any other files are opened
        //
        //
        {
          // Make sure the current path is where Allegiance.exe is for the AutoUpdate: 
          // For Download -AND- for Reloader.exe -AND- Loading FileList which happens 
          // when client logs onto lobby
          //
          char    path[MAX_PATH + 16];
          ::GetModuleFileName(NULL, path, MAX_PATH);
          char*   p = strrchr(path, '\\');
          if (!p)
              p = path;
          else
              p++;

          *p = 0; // erase filename
          ::SetCurrentDirectory(path);
        }


        //
        // Check for sufficient free memory
        //

        if (!CheckFreeMemory())
        {
            if (MessageBox(NULL, 
                "You are low on free memory and/or hard drive space.  "
                "You may experience problems running Allegiance.  Run anyway?", 
                "Allegiance",
                MB_ICONERROR | MB_YESNO
                ) != IDYES)
            {
               ::ExitProcess(0);
               return S_FALSE;
            }
        }

        //
        // Check the DSound version
        //

        if (!CheckDSoundVersion())
        {
            MessageBox(NULL, 
                "Allegiance requires DirectX 7 or higher, which was not detected.  "
                    "Please re-run setup and choose to install DirectX 7.", 
                "Allegiance",
                MB_ICONERROR | MB_OK
                );

            return E_FAIL;
        }


        HRESULT hr = CoInitialize(NULL);

        if (FAILED(hr))
            return hr;

        // Fix success HRESULT
        hr = S_OK;

// BUILD_DX9
		// For the D3D build, move this to after the window has been created, as we need a valid HWND to create the device.
//		EffectApp::Initialize(strCommandLine);
// BUILD_DX9

        //
        // get the artpath
        //
        PathString pathStr = GetConfiguration()->GetStringValue(
            "Data.Path",
            GetConfiguration()->GetStringValue(
                "ArtPath",
                GetExecutablePath() + "\\Artwork" 
            )
        ).c_str();

        bool bLogFrameData = GetConfiguration()->GetBoolValue(
            "Debug.LogFrameData",
            GetConfiguration()->GetBoolValue(
                "LogFrameData",
                ""
            )
        );

        std::string pathLogFrameData = GetConfiguration()->GetStringValue(
            "Debug.LogFrameDataPath",
            GetConfiguration()->GetStringValue(
                "LogFrameDataPath",
                ""
            )
        );

        if (bLogFrameData && pathLogFrameData.length() > 0)
        {
            ZString strFile = pathLogFrameData.c_str();
            g_pzfFrameDump = new ZWriteFile(strFile);
            // check for a valid file handle
            if (g_pzfFrameDump->IsValid())
            {
                // dump out the header row of data
                g_pzfFrameDump->Write(
                    "Mspf,"
                    "Fps,"
                    "Warps,"
                    "Ships,"
                    "Projectiles,"
                    "Asteroids,"
                    "Stations,"
                    "Treasures,"
                    "Missiles,"
                    "SectorWarps,"
                    "SectorShips,"
                    "SectorProjectiles,"
                    "SectorAsteroids,"
                    "SectorStations,"
                    "SectorTreasures,"
                    "SectorMissiles,"
                    "Triangles,"
                    "DrawStringCalls,"
                    "Chars"
                    "\n"
                );
            }
            else
            {
                // pop up error message box
                MessageBox(NULL, "Framerate log file location is invalid.\n"
                    "Use CliConfig to set the framerate log file location to "
                    "a valid location.",
                    "Framerate Logging Error", MB_ICONEXCLAMATION | MB_OK);
            }
        }
		
		//Imago 8/16/09
		ZVersionInfo vi;
		debugf("Running %s %s\nArtpath: %s\nCommand line: %s\n", (PCC) vi.GetInternalName(), 
			(PCC) vi.GetStringValue("FileVersion"),(PCC) pathStr, (PCC) strCommandLine);

 		UTL::SetArtPath(pathStr);

        //
        // Initialize the runtime
        //

        srand((unsigned)timeGetTime());

        //
        // Parse the command line
        //

        bool bLogonDialog     = true;
        bool bStartOffline    = false;
        bool bMovies          = true;
        bool bSingleInstance  = true;
		bool bStartFullscreen = true;


        ZString strMap;
		ZString strAdapter; int iUseAdapter = 0;

        PCC pcc = strCommandLine;
        CommandLineToken token(pcc, strCommandLine.GetLength());

        while (token.MoreTokens()) {
            ZString str;

            if (token.IsMinus(str)) 
            {
                if (str == "training") {
                    bStartTraining = true;
                    bLogonDialog = false;
                    bMovies = false;
                } else if (str == "nooutput") {
                    //todo, change logging based on this
                } else if (str == "quickstart") {
					//Imago dont quickstart if no saved name 7/21/09
					if (trekClient.GetSavedCharacterName().GetLength())
                    	g_bQuickstart = true;
                    float civStart;
                    if (token.IsNumber(civStart)) 
                        g_civStart = (int)civStart;
                } else if (str == "nocfgdl")  {
                    g_bDownloadNewConfig = false;
                } else if (str == "checkfiles") {
                    g_bCheckFiles = true;
                } else if (str == "noautoupdate") {
                    g_bSkipAutoUpdate = true;
                } else if (str == "nosound") {
                    g_bEnableSound = false;
                } else if (str == "nomessage")  {
                    g_bDownloadZoneMessage = false;
                } else if (str == "nomovies")  {
                    bMovies = false;
                } else if (str == "zone")  {
                    g_bDisableZoneClub = false;
                } else if (str == "newcivs")  {
                    g_bDisableNewCivs = false;
                } else if (str == "map")  {
                    if (token.IsString(strMap)) 
                    {
                        bStartOffline = true;
                        bLogonDialog = false;
                    }
                } else if(str == "reloaded") {
                    //
                    // reloaded after an auto-update
                    // 
                    g_bReloaded = true;

                    // TODO: make two types of reloaded: one for internet, one for zone
                    // that way we know how to log on.  Right now there it just relogs on
                    // using internet connect
                } else if (str == "multi") { 
                    bSingleInstance = false;
                } else if (str == "noauth") {
                    g_fZoneAuth = false;
                } else if (str == "cdkey") {
                    g_bAskForCDKey = true;
                // wlp 2006 - added debug option to turn on debug output
				} else if (str == "debug") {
                    //todo, change logging based on this
                } else if (str.Left(9) == "callsign=") { // wlp - 2006, added new ASGS token
                    trekClient.SaveCharacterName(str.RightOf(9)) ; // Use CdKey for ASGS callsign storage
                    g_bAskForCallSign = false ; // wlp callsign was entered on commandline
                } else if (str == "windowed") {  //imago sucked these in here to accommidate the way we now create the D3DDevice
	                bStartFullscreen = false;
	            } else if (str == "fullscreen") {
	                bStartFullscreen = true;
				} else if (str == "adapter") { //imago added for the needy
                    if (token.IsString(strAdapter))
                    {
                        iUseAdapter = strAdapter.GetInteger();
                    }
				}
            }
            else // wlp 2006 - adapted this string featture to add ASGS Ticket to cdKey field
            if (token.IsString(str)){} ;
            }

		//Orion - 2009 ACSS : check the alleg pipe for the auth token
		//trekClient.SetCDKey(ReadAuthPipe());

		debugf("Logging into steam.\n");

		// BT - STEAM
		if (SteamUser() != nullptr && SteamUser()->BLoggedOn() == true)
		{
			ZString personaName = SteamFriends()->GetPersonaName();

			debugf("Steam Persona Name: " + personaName + "\n");

			personaName = CleanUpSteamName(personaName);

			debugf("Cleaned Persona Name: " + personaName + "\n");

			trekClient.SaveCharacterName(personaName);

			debugf("trekClient - Saved persona name as character name.\n");

			char steamID[64];
			sprintf(steamID, "%" PRIu64, SteamUser()->GetSteamID().ConvertToUint64());
			trekClient.SetCDKey(steamID);
		}

		debugf("Steam login complete.\n");

        // 
        // Check for other running copies of the app
        //
        if (bSingleInstance)
        {
            HWND hOldInstance = FindWindow(TrekWindow::GetTopLevelWindowClassname(), 
                TrekWindow::GetWindowTitle());

            // if we found another copy of the app
            if (hOldInstance)
            {
                // find out if it has any popups
                HWND hPopup = GetLastActivePopup(hOldInstance);

                // Bring the main window to the top.
                if (SetForegroundWindow(hOldInstance))
                {
                    // If the old instance was minimized, restore it.
                    if (IsIconic(hOldInstance))
                    {
                        ShowWindow(hOldInstance, SW_RESTORE);
                    }

                    // If the old instance had a pop-up, bring that to the foreground
                    if (hPopup != hOldInstance)
                    {
                        SetForegroundWindow(hPopup);
                    }
                }
    
                return S_FALSE;
            }
        }

        //
        // Create the window
        //

// BUILD_DX9
		// Ask the user for video settings. -- 
		//   -adapter switch added for the needy
		//   Raise dialog only if "Safe Mode" activated (any software/primary/secondary switches sent) 
		// imago 6/29/09 7/1/09 removed hardware, asgs sends this under normal conditions
		// BT - 10/17 - Force the client to launch windowed, then we'll take it full screen later. 
		// BT - 10/17 - The video picker no longer creates devices that work with the create texture D3D functions. This may be from me forcing it to load windowed and then pulling it full screen. However, starting windowed and then pulling full screen seems to have fixed a whole bunch of start up issues. 
		if( PromptUserForVideoSettings(false, iUseAdapter, GetModuleHandle(NULL), pathStr , GetConfiguration()) == false )
		{
			return E_FAIL;
		}

		CD3DDevice9::Get()->UpdateCurrentMode( );
	
        TRef<TrekWindow> pwindow = 
            TrekWindow::Create(
                this, 
                strCommandLine,
				pathStr,
                bMovies
            );

        if (!pwindow->IsValid()) {
            return E_FAIL;
        }

        //
        // Handling command line options
        //

        if (bStartTraining)
            GetWindow ()->screen (ScreenIDTrainScreen);

        return hr;
    }

    void Terminate()
    {
      EffectApp::Terminate();
      CoUninitialize();
    }
} g_trekImpl;
