#include "efwindow.h"

#include "efapp.h"

//////////////////////////////////////////////////////////////////////////////
//
// Effect Object Factories
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// The EffectWindow Class
//
//////////////////////////////////////////////////////////////////////////////

EffectWindow::EffectWindow(
          EffectApp*	papp,
    UpdatingConfiguration* pConfiguration,
    const ZString&		strCommandLine,
    const ZString&		strTitle,
          bool			bStartFullscreen,
    const WinRect&		rect,
    const WinPoint&		sizeMin,
          HMENU			hmenu
) :
    EngineWindow(
        papp,
        pConfiguration,
        strCommandLine,
        strTitle,
        bStartFullscreen,
        rect,
        sizeMin,
        hmenu
    )
{
}
