#include "pch.h"
#include "Training.h"

//////////////////////////////////////////////////////////////////////////////
//
// Training Screen
//
//////////////////////////////////////////////////////////////////////////////

TRef<ModifiableNumber>  g_pnumberMissionNumber;

class TrainingScreen :
    public Screen, 
    public EventTargetContainer<TrainingScreen>,
    public TrekClientEventSink,
    public IItemEvent::Sink
{
private:
    TRef<Pane>                          m_ppane;
    TRef<ButtonPane>                    m_pbuttonBack;
    TRef<ButtonPane>                    m_pbuttonTrainMission1;
    TRef<ButtonPane>                    m_pbuttonTrainMission2;
    TRef<ButtonPane>                    m_pbuttonTrainMission3;
    TRef<ButtonPane>                    m_pbuttonTrainMission4;
    TRef<ButtonPane>                    m_pbuttonTrainMission5;
    TRef<ButtonPane>                    m_pbuttonTrainMission6;
	//TM7 removed
	TRef<ButtonPane>                    m_pbuttonTrainMission8; //TheBored 06-JUL-07: nanite mission
    TRef<ButtonPane>                    m_pbuttonTrain;

    static  int                         m_iMissionNext;

public:
    TrainingScreen(Modeler* pmodeler)
    {
        //
        // exports
        //

        TRef<INameSpace> pnsTrainingData = pmodeler->CreateNameSpace("trainingdata", pmodeler->GetNameSpace("gamepanes"));

        pnsTrainingData->AddMember("missionNumber", g_pnumberMissionNumber = new ModifiableNumber(m_iMissionNext));

        // Load the members from MDL

        TRef<INameSpace> pns = pmodeler->GetNameSpace("TrainingScreen");

        CastTo(m_ppane,                     pns->FindMember("screen"                    ));
        CastTo(m_pbuttonBack,               pns->FindMember("backButtonPane"            ));
        CastTo(m_pbuttonTrainMission1,      pns->FindMember("trainMission1ButtonPane"   ));
        CastTo(m_pbuttonTrainMission2,      pns->FindMember("trainMission2ButtonPane"   ));
        CastTo(m_pbuttonTrainMission3,      pns->FindMember("trainMission3ButtonPane"   ));
        CastTo(m_pbuttonTrainMission4,      pns->FindMember("trainMission4ButtonPane"   ));
        CastTo(m_pbuttonTrainMission5,      pns->FindMember("trainMission5ButtonPane"   ));
        CastTo(m_pbuttonTrainMission6,      pns->FindMember("trainMission6ButtonPane"   ));
		//TM7 removed
		CastTo(m_pbuttonTrainMission8,      pns->FindMember("trainMission8ButtonPane"   )); //TheBored 06-JUL-07: nanite mission
        CastTo(m_pbuttonTrain,              pns->FindMember("trainButtonPane"           ));

        pmodeler->UnloadNameSpace("TrainingScreen");

        //
        // buttons
        //

		// mdvalley: Pointers, Classes, and bears, oh my!
        AddEventTarget(&TrainingScreen::OnButtonBack, m_pbuttonBack->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrain->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrainMission1, m_pbuttonTrainMission1->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrainMission2, m_pbuttonTrainMission2->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrainMission3, m_pbuttonTrainMission3->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrainMission4, m_pbuttonTrainMission4->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrainMission5, m_pbuttonTrainMission5->GetEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrainMission6, m_pbuttonTrainMission6->GetEventSource());
		//TM7 removed
		AddEventTarget(&TrainingScreen::OnButtonTrainMission8, m_pbuttonTrainMission8->GetEventSource()); //TheBored 06-JUL-07: nanite mission

        //
        // buttons
        //

        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission1->GetDoubleClickEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission2->GetDoubleClickEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission3->GetDoubleClickEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission4->GetDoubleClickEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission5->GetDoubleClickEventSource());
        AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission6->GetDoubleClickEventSource());
		//TM7 removed
		AddEventTarget(&TrainingScreen::OnButtonTrain, m_pbuttonTrainMission8->GetDoubleClickEventSource()); //TheBored 06-JUL-07: nanite mission

        // hilite the current mission
		CheckButton(m_iMissionNext);
        m_pbuttonTrain->SetChecked (true);
//#ifdef _DEBUG
		if (bStartTraining) {
			g_pnumberMissionNumber->SetValue(Training::c_TM_10_Free_Flight);
			OnButtonTrain();
		}
//#endif
    }

    ~TrainingScreen()
    {
    }

	void CheckButton(const int iMission)
	{
		// uncheck everything.
		m_pbuttonTrainMission1->SetChecked(false);
		m_pbuttonTrainMission2->SetChecked(false);
		m_pbuttonTrainMission3->SetChecked(false);
		m_pbuttonTrainMission4->SetChecked(false);
		m_pbuttonTrainMission5->SetChecked(false);
		m_pbuttonTrainMission6->SetChecked(false);
		//TM7 removed
		m_pbuttonTrainMission8->SetChecked(false);

		// check the mission
		switch (iMission)
		{
			case Training::c_TM_1_Introduction:
				m_pbuttonTrainMission1->SetChecked(true);
				break;
			case Training::c_TM_2_Basic_Flight:
				m_pbuttonTrainMission2->SetChecked(true);
				break;
			case Training::c_TM_3_Basic_Weaponry:
				m_pbuttonTrainMission3->SetChecked(true);
				break;
			case Training::c_TM_4_Enemy_Engagement:
				m_pbuttonTrainMission4->SetChecked(true);
				break;
			case Training::c_TM_5_Command_View:
				m_pbuttonTrainMission5->SetChecked(true);
				break;
			case Training::c_TM_6_Practice_Arena:
				m_pbuttonTrainMission6->SetChecked(true);
				break;
				//TheBored 06-JUL-07: nanite mission
			case Training::c_TM_8_Nanite:
				m_pbuttonTrainMission8->SetChecked(true);
				break;
				//End TB
			default:
				break;
		}
	}

    //////////////////////////////////////////////////////////////////////////////
    //
    // UI Events
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        return true;
    }

    bool OnButtonBack()
    {
        GetWindow()->screen(ScreenIDIntroScreen);
        return false;
    }

    bool OnButtonTrain()
    {
        // here we mark the fact that a training mission has been launched
        GetConfiguration()->SetBoolValue("Ui.ShowStartupTrainingSuggestion", false);

        // check to see which mission this is, and advance to the next mission
        int iMission = static_cast<int> (g_pnumberMissionNumber->GetValue ());
        if (iMission == m_iMissionNext)
        {
            m_iMissionNext++;
			// TM7 does not exist anymore, and there is nothing after TM8. After TM6 always select TM8. 
            if (m_iMissionNext >= Training::c_TM_7_Live)
                m_iMissionNext = Training::c_TM_8_Nanite;
        }

        // check to see which mission this is
        switch (iMission)
        {
            case Training::c_TM_1_Introduction:
                GetWindow ()->screen (ScreenIDPostTrainSlideshow);
                break;
            case Training::c_TM_2_Basic_Flight:
            case Training::c_TM_3_Basic_Weaponry:
            case Training::c_TM_4_Enemy_Engagement:
            case Training::c_TM_5_Command_View:
            case Training::c_TM_6_Practice_Arena:
            case Training::c_TM_8_Nanite:
                // note that the training slideshow can get the mission number from the global
                // value when subsequently launching.
                GetWindow()->screen(ScreenIDTrainSlideshow);
                break;
            case Training::c_TM_10_Free_Flight:
                //Skip slideshow -- Let stuff load before changing it
                TRef<IMessageBox>   pMsgBox = CreateMessageBox("Training simulation initiated...", NULL, false);
                GetWindow()->GetPopupContainer()->OpenPopup(pMsgBox, false);
                AddEventTarget(&TrainingScreen::OnFreeFlightSwitchOut, GetWindow(), 0.1f);
                break;
        }
        return true;
    }

    bool OnFreeFlightSwitchOut(void)
    {
        GetWindow()->StartTraining(Training::c_TM_10_Free_Flight);
        return false;
    }

	bool OnButtonMission(const int iMission)
	{
		g_pnumberMissionNumber->SetValue(iMission);
		CheckButton(iMission);
		m_pbuttonTrain->SetEnabled(true);
		m_pbuttonTrain->SetChecked(true);
		return true;
	}

    bool OnButtonTrainMission1()
    {
		return OnButtonMission(Training::c_TM_1_Introduction);
    }

    bool OnButtonTrainMission2()
    {
		return OnButtonMission(Training::c_TM_2_Basic_Flight);
    }

    bool OnButtonTrainMission3()
    {
		return OnButtonMission(Training::c_TM_3_Basic_Weaponry);
    }

    bool OnButtonTrainMission4()
    {
		return OnButtonMission(Training::c_TM_4_Enemy_Engagement);
    }

    bool OnButtonTrainMission5()
    {
		return OnButtonMission(Training::c_TM_5_Command_View);
    }

    bool OnButtonTrainMission6()
    {
		return OnButtonMission(Training::c_TM_6_Practice_Arena);
    }

	//TheBored 06-JUL-07: #8 added
	bool OnButtonTrainMission8()
    {
		return OnButtonMission(Training::c_TM_8_Nanite);
    }
	//End TB

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    //////////////////////////////////////////////////////////////////////////////

    void OnEnterMission (void)
    {
        debugf ("Game has started!\n");

        // set the cluster
        GetWindow ()->SetCluster (trekClient.GetShip ()->GetCluster ());

        // switch to the combat screen
        GetWindow ()->screen (ScreenIDCombat);

        // set the station so we don't launch in a lifepod
        IstationIGC*    pstation = trekClient.GetShip ()->GetStation();
        if (pstation)
            trekClient.ReplaceLoadout (pstation);
    }

    //////////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////////
//
// static initializer
//
//////////////////////////////////////////////////////////////////////////////
int             TrainingScreen::m_iMissionNext = Training::c_TM_1_Introduction;

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateTrainingScreen(Modeler* pmodeler)
{
    return new TrainingScreen(pmodeler);
}
