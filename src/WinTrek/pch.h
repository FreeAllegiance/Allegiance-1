// pch.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define __MODULE__ "WinTrek"

//
// C Headers
//

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

//
// Windows headers
//

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <commctrl.h>

//
// STL headers
//

#pragma warning( disable : 4786)  

#include <algorithm>
#include <map>
#include <set>
#include <vector>

// Need to include zlib before any DirectX headers

#define ID_APP_ICON "winTrekIcon"
#include "zlib.h"

//
// DX headers
//

//#include <dplobby.h>
#include <dinput.h>

#include "guids.h"

// BT - STEAM
#include "steam_api.h"

//
// WinTrek headers
//
#include "ddvideo.h" //Imago 6/28/09 re-write

#include "engine.h"
#include "effect.h"
#include "utility.h"
#include "igc.h"
#include "zone.h"
#include "trekinput.h"
#include "resource.h"
#include "Zone.h"
#ifdef USEAUTH
#include "zauth.h"
#endif
#include "messagesAll.h"
#include "messages.h"
#include "messagesLC.h"
#include "ClubMessages.h" // KG - moved in global /Inc
#include "clintlib.h"
#include "AutoDownload.h"

#include "soundengine.h"

#include "screen.h"
#include "wintrek.h"
#include "treki.h"
#include "indicator.h"
#include "radarimage.h"
#include "trekctrls.h"
#include "trekmdl.h"
#include "loadout.h"

#include "wintrekp.h"
#include "artwork.h"
#include "ZoneSquad.h"
#include "logon.h"
#include "passworddialog.h"
#include "regkey.h"
#include "treksound.h"
#include "GameTypes.h"

// BT - STEAM
#include "CallsignTagInfo.h"
