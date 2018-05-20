#ifndef _VIDEOSETTINGS_H_
#define _VIDEOSETTINGS_H_

#include <windows.h>
#include <zstring.h>

#include "Configuration.h"

//Imago added bool bStartFullscreen, bool bRaise 6/29/09, only prompts when "Safe Mode" and restored the -windowed command line switch
//Rock: Removed bRaise, maybe reintroduce when it would be used again. Config instead of registry path.
bool PromptUserForVideoSettings(bool bStartFullscreen, int iAdapter, HINSTANCE hInstance, PathString & szArtPath, TRef<UpdatingConfiguration> config );

#endif // _VIDEOSETTINGS_H_
