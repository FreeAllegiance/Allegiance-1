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


bool CheckForAllGuard()
{
  // KGJV: allways bypass
  return true;
  // Bypass any other tests if -nod is specified on the command line
  ZString strCmdLine(::GetCommandLine());
  while (!strCmdLine.IsEmpty())
    if (strCmdLine.GetToken() == "-nod")
      return true;

  // Load KERNEL32
  HINSTANCE hinstKernel32 = ::GetModuleHandle("kernel32.dll");
  assert(hinstKernel32);

  // Get the address of IsDebuggerPresent, if available
  typedef BOOL (WINAPI* PFNIsDebuggerPresent)(VOID);
  PFNIsDebuggerPresent pfnIsDebuggerPresent = (PFNIsDebuggerPresent)
    ::GetProcAddress(hinstKernel32, "IsDebuggerPresent");
  if (pfnIsDebuggerPresent)
  {
    // Indicate that we are being debugged, if we are
    if ((*pfnIsDebuggerPresent)())
      return true;
  }
  else
  {
    // Win95 doesn't support IsDebuggerPresent, so we must check some other ways

    // Format an event name using the current process ID
    char szEvent[24];
    sprintf(szEvent, "MSRGuard_%08X", GetCurrentProcessId());

    // Determine if the named event already exists
    HANDLE hEvent = ::OpenEvent(EVENT_ALL_ACCESS, false, szEvent);
    if (hEvent)
    {
      // Close the event handle and indicate that we are being debugged
      ::CloseHandle(hEvent);
      return true;
    }
  }

  // Get the command-line options (again)
  strCmdLine = ::GetCommandLine();
  strCmdLine.GetToken();

  // Get the fully-qualified path to the current process
  char szModulePath[_MAX_PATH];
  GetModuleFileName(NULL, szModulePath, sizeof(szModulePath));
  char szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
  _splitpath(szModulePath, szDrive, szDir, NULL, NULL);
  _makepath(szModulePath, szDrive, szDir, NULL, NULL);

  // Get the ArtPath, since that's where AllGuard.exe should be
  HKEY hKey = NULL;
  if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ, &hKey))
    return true; // If it can't be read, just keep running
  char szArtPath[_MAX_PATH];
  DWORD cbArtPath = sizeof(szArtPath);
  if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, "ArtPath", NULL, NULL, (BYTE*)&szArtPath, &cbArtPath))
  {
    lstrcpy(szArtPath, szModulePath);
    lstrcat(szArtPath, "artwork\\");
    cbArtPath = lstrlen(szArtPath);
  }
  else if ('\\' != szArtPath[cbArtPath - 1])
  {
    lstrcat(szArtPath, "\\");
    ++cbArtPath;
  }
  ::RegCloseKey(hKey);

  // Append the AllGuard.exe filename and parameters
  char szAllGuard[_MAX_PATH * 4];
  sprintf(szAllGuard, "\"%sAllGuard.exe\" %s", szArtPath, (LPCSTR)strCmdLine);

  // Create the AllGuard.exe process
  STARTUPINFO si = {sizeof(si)};
  PROCESS_INFORMATION pi;
  if (!::CreateProcess(NULL, szAllGuard, NULL, NULL, false, 0, NULL, szModulePath, &si, &pi))
    return true; // If it can't be created, just keep running

  // Indicate false to exit this instance of the process
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

          HKEY hKey;
          DWORD dwType;
          char  szValue[MAX_PATH];
          DWORD cbValue = MAX_PATH;

          // NOTE: please keep reloader.cpp's GetArtPath() in sync with this!!!
          if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ, &hKey))
          {
              // Get MoveInProgress from registry
              if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "MoveInProgress", NULL, &dwType, (unsigned char*)&szValue, &cbValue) &&
                  *((DWORD*)szValue) == 1)
              {
                  if (::MessageBox(NULL, "The AutoUpdate process failed to finish.  Try again to finish?  (YES is recommended)", "Error", MB_ICONERROR | MB_YESNO) == IDYES)
                  {
                      if (!LaunchReloaderAndExit(false))
                      {
                          ::MessageBox(NULL, "Couldn't launch Reloader.exe", "Fatal Error", MB_ICONERROR);
                          ::ExitProcess(0);
                          return S_FALSE;
                      }
                  }
                  else
                  {
                      ::ExitProcess(0);
                      return S_FALSE;
                  }
              }
              ::RegCloseKey(hKey);
          }
        }

        //
        // Check to see if we are being debugged
        //
        if (!CheckForAllGuard())
          return S_FALSE;


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

        PathString pathStr; // = ZString::GetProfileString("Federation", "ArtPath");

        HKEY hKey;
        DWORD dwType;
        char  szValue[MAX_PATH];
        DWORD dwValue;
        DWORD cbValue = MAX_PATH;

        // NOTE: please keep reloader.cpp's GetArtPath() in sync with this!!!
        if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ, &hKey))
        {
            // Get the art path from the registry
            if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, "ArtPath", NULL, &dwType, (unsigned char*)&szValue, &cbValue))
            {
                // Set ArtPath to be relative to the application path
                GetModuleFileNameA(NULL, szValue, MAX_PATH);
                char*   p = strrchr(szValue, '\\');
                if (!p)
                    p = szValue;
                else
                    p++;

                strcpy(p, "artwork");

                //Create a subdirectory for the artwork (nothing will happen if it already there)
                CreateDirectoryA(szValue, NULL);
            }
            pathStr = szValue;
 
            cbValue = MAX_PATH; // reset this

            // Start the frame rate data log, if necessary
            if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "LogFrameData", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            {
                cbValue = MAX_PATH;
                if (dwValue==1)
                {
                    if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "LogFrameDataPath", NULL, &dwType, (unsigned char*)&szValue, &cbValue))
                    {
                        if (strlen(szValue)>0)
                        {
                            ZString strFile = szValue;
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
                    }
                }
            }

            ::RegCloseKey(hKey);
        }

        if (pathStr.IsEmpty()) {
            // marksn: to make everyone consistent, this should go back to getmodulefilename path
            //         like for convex hull and sounds
            // pathStr = PathString::GetCurrentDirectory() + "artwork";

            char    logFileName[MAX_PATH + 16];
            GetModuleFileName(NULL, logFileName, MAX_PATH);
            char*   p = strrchr(logFileName, '\\');
            if (!p)
                p = logFileName;
            else
                p++;
            strcpy(p, "artwork");
            pathStr = logFileName;
        }
		
		//Imago 8/16/09
		ZVersionInfo vi;
		debugf("Running %s %s\nArtpath: %s\nCommand line: %s\n", (PCC) vi.GetInternalName(), 
			(PCC) vi.GetStringValue("FileVersion"),(PCC) pathStr, (PCC) strCommandLine);

// BUILD_DX9
		// Now set later for D3D build, as modeller isn't valid yet.
		//GetModeler()->SetArtPath(pathStr);
// BUILD_DX9
 		UTL::SetArtPath(pathStr);
		
		/*{
			HRESULT hr = FirstRunEula(pathStr);
		
          if (hr == E_FAIL)
          {
              ::MessageBox(NULL, "Error while trying to load ebueula.dll. Please reboot and retry.  If it still fails, reinstall Allegiance", "Initialization Error", MB_OK);
              return S_FALSE;
          }
          else
          if (hr == S_FALSE) 
          {
              ::MessageBox(NULL, "You must accept the End User License Agreement before playing the Allegiance", "Allegiance", MB_OK);
              return S_FALSE;
          }
          else
          {
            assert(hr == S_OK);
          }
        }*/

// BUILD_DX9
        //
        // load the fonts
        //

//        TrekResources::Initialize(GetModeler());
// BUILD_DX9

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
        bool bSoftware        = false;
        bool bHardware        = false;
        bool bPrimary         = false;
        bool bSecondary       = false;
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
                } else if (str == "software") {
                    bSoftware = true;
                } else if (str == "hardware") {
                    bHardware = true;
                } else if (str == "primary") {
                    bPrimary = true;
                } else if (str == "secondary") {
                    bSecondary = true;
                } else if (str == "nooutput") {
                    g_bOutput = false;
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
                    g_outputdebugstring  = true;           //wlp allow debug outputs
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
		//bool bRaise = (bSoftware || bPrimary || bSecondary) ? true : false;
		bool bRaise = false; // BT - 10/17 - The video picker no longer creates devices that work with the create texture D3D functions. This may be from me forcing it to load windowed and then pulling it full screen. However, starting windowed and then pulling full screen seems to have fixed a whole bunch of start up issues. 
		if( PromptUserForVideoSettings(false, bRaise, iUseAdapter, GetModuleHandle(NULL), pathStr , ALLEGIANCE_REGISTRY_KEY_ROOT) == false )
		{
			return E_FAIL;
		}

		CD3DDevice9::Get()->UpdateCurrentMode( );
	
        TRef<TrekWindow> pwindow = 
            TrekWindow::Create(
                this, 
                strCommandLine,
				pathStr,
                bMovies,
                bSoftware,
                bHardware,
                bPrimary,
                bSecondary
            );
// #else
        //TRef<TrekWindow> pwindow = 
        //    TrekWindow::Create(
        //        this, 
        //        strCommandLine,
        //        bMovies,
        //        bSoftware,
        //        bHardware,
        //        bPrimary,
        //        bSecondary
        //    );
// BUILD_DX9

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
