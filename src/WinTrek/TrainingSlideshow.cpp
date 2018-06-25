#include "pch.h"
#include "slideshow.h"
#include "training.h"


//////////////////////////////////////////////////////////////////////////////
//
// Training Screen
//
//////////////////////////////////////////////////////////////////////////////

class TrainingSlideshow :
    public Slideshow,
    public EventTargetContainer<TrainingSlideshow>
{
private:
    int                 m_iMission;

public:
    TrainingSlideshow (Modeler* pModeler, const ZString& strNamespace, int iMission) : 
    Slideshow (pModeler, strNamespace),
    m_iMission (iMission)
    {
    }

    ~TrainingSlideshow (void)
    {
    }

    void    Dismiss (void)
    {
        Slideshow::Dismiss ();

        // pop up the Connecting... dialog.   
        GetWindow()->SetWaitCursor();

        // open up a dialog to let the player know the world hasn't ended
        TRef<IMessageBox>   pMsgBox = CreateMessageBox ("Training simulation initiated...", NULL, false);            
        GetWindow ()->GetPopupContainer ()->OpenPopup (pMsgBox, false);

        // pause to let the "starting..." box draw itself
		// mdvalley: pause to add in another pointer and class name
        AddEventTarget (&TrainingSlideshow::OnSwitchOut, GetEngineWindow(), 0.1f);
    }

    bool OnSwitchOut (void)
    {
        GetWindow ()->StartTraining (m_iMission);
        return false;
    }

};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateTrainingSlideshow (Modeler* pModeler, const ZString& strNamespace, int iMissionIndex)
{
    return new TrainingSlideshow (pModeler, strNamespace, iMissionIndex);
}
