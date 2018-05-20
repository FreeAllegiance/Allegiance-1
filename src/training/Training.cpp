/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	training.cpp
**
**  Author:  
**
**  Description:
**      Implementation of the training library core.
**
**  History:
*/
#include    "pch.h"
#include    "Training.h"
#include    "Mission2.h"
#include    "Mission3.h"
#include    "Mission4.h"
#include    "Mission5.h"
#include    "Mission6.h"
#include    "Mission8.h" //TheBored 06-JUL-07: Adding nanite mission.
#include    "Mission10.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // global variabes
    //------------------------------------------------------------------------------
    ControlData         g_allowableControlScalars;  // yaw, pitch, roll, throttle
    ControlData         g_inputControls;            // yaw, pitch, roll, throttle - the ones the player is actually inputing
    ControlData         g_setInputControls;         // yaw, pitch, roll, throttle - the ones we are forcing on the player
    int                 g_iAllowableActionMask;
    int                 g_iSetInputAction;          // fire, afterburners, ... - the ones we force on the player
    TrainingMission*    g_pMission = 0;
    bool                g_bIsMission7;

    //------------------------------------------------------------------------------
    // primary interface to the training missions
    //------------------------------------------------------------------------------
    bool            StartMission (int iMissionIndex)
    {
        // assign default values to the control masks
        g_allowableControlScalars.jsValues[c_axisYaw] = 1.0f;
        g_allowableControlScalars.jsValues[c_axisPitch] = 1.0f;
        g_allowableControlScalars.jsValues[c_axisRoll] = 1.0f;
        g_allowableControlScalars.jsValues[c_axisThrottle] = 1.0f;
        g_iAllowableActionMask = 0xffffffff;

        // be sure the set controls are cleared
        g_setInputControls.jsValues[c_axisYaw] = NA;
        g_setInputControls.jsValues[c_axisPitch] = NA;
        g_setInputControls.jsValues[c_axisRoll] = NA;
        g_setInputControls.jsValues[c_axisThrottle] = NA;
        g_iSetInputAction = 0;

        // set the HUD overlay back to nothing
        ClearTrainingOverlay ();

        // clear the mission 7 flag
        g_bIsMission7 = false;

        // create the appropriate mission, and start it
        switch (iMissionIndex)
        {
            case c_TM_2_Basic_Flight:
                g_pMission = new Mission2;
                break;
            case c_TM_3_Basic_Weaponry:
                g_pMission = new Mission3;
                break;
            case c_TM_4_Enemy_Engagement:
                g_pMission = new Mission4;
                break;
            case c_TM_5_Command_View:
                g_pMission = new Mission5;
                break;
            case c_TM_6_Practice_Arena:
                g_pMission = new Mission6;
                break;
            case c_TM_8_Nanite:  //TheBored 06-JUL-07: adding nanite mission
                g_pMission = new Mission8;
                break;
            case c_TM_10_Free_Flight:
                g_pMission = new Mission10;
                break;
            case c_TM_7_Live:
                g_bIsMission7 = true;
                // fall through
            default:
                // create no mission
                return false;
        }
        assert (g_pMission);
        g_pMission->Start ();

        // start training missions with the throttle off
        trekClient.OverrideThrottle (-1.0f);

        // return true to indicate that we started a  mission
        return true;
    }

    //------------------------------------------------------------------------------
    bool            HandleMission (void)
    {
        // handle the mission if necessary
        if (g_pMission and g_pMission->Frame ())
        {
            TrainingMission*    pMission = g_pMission;
            g_pMission = 0;
            delete pMission;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    bool            EndMission (void)
    {
        if (g_bIsMission7)
            g_bIsMission7 = false;
        if (g_pMission)
        {
            g_pMission->Terminate ();
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    void            SetSkipPostSlideshow (void)
    {
        if (g_pMission)
            g_pMission->SetSkipPostSlideshow ();
    }

    //------------------------------------------------------------------------------
    bool            IsTraining (void)
    {
        return g_pMission ? true : false;
    }

    //------------------------------------------------------------------------------
    bool            IsInstalled (void)
    {
        return true;
    /*
        char        szFilename[MAX_PATH + 1];
        HRESULT     hr = UTL::getFile ("trainingsounddef", ".mdl", szFilename, true, true);
        return (hr == S_OK);
    */
    }

    //------------------------------------------------------------------------------
    bool            CommandViewEnabled(void) {
        return g_pMission ? g_pMission->GetCommandViewEnabled() : false;
    }

    //------------------------------------------------------------------------------
    int             GetTrainingMissionID (void)
    {
        if (g_bIsMission7)
            return c_TM_7_Live;
        else if (g_pMission)
            return g_pMission->GetMissionID ();
        return NA;
    }

    //------------------------------------------------------------------------------
    SectorID        GetStartSectorID (void)
    {
        assert (g_pMission);
        return g_pMission->GetStartSectorID ();
    }

    //------------------------------------------------------------------------------
    int             GetKillCount(void)
    {
        assert(g_pMission);
        return g_pMission->GetKillCount();
    }

    //------------------------------------------------------------------------------
    void            SetupShipAndCamera (void)
    {
        assert (g_pMission);
        g_pMission->SetupShipAndCamera ();
    }

    //------------------------------------------------------------------------------
    // event trapping
    //------------------------------------------------------------------------------
    void            RecordChat (ChatTarget recipient)
    {
        if (g_pMission)
            g_pMission->RecordChat (recipient);
    }

    //------------------------------------------------------------------------------
    bool            RecordKeyPress (TrekKey key)
    {
        if (g_pMission)
            return g_pMission->RecordKeyPress (key);
        return true;
    }

    //------------------------------------------------------------------------------
    bool            ShipLanded (void)
    {
        if (g_pMission)
            return g_pMission->ShipLanded ();
        return true;
    }

    //------------------------------------------------------------------------------
    void            ShipDied (ImodelIGC* pLauncher)
    {
        if (g_pMission)
            g_pMission->ShipDied (pLauncher);
    }

    //------------------------------------------------------------------------------
    bool            RestoreShip (void)
    {
        if (g_pMission)
            return g_pMission->RestoreShip ();
        return true;
    }

    //------------------------------------------------------------------------------
    void            ShipKilled(IshipIGC* pShip, ImodelIGC* pLauncher)
    {
        if (g_pMission)
            g_pMission->ShipKilled(pShip, pLauncher);
    }

    //------------------------------------------------------------------------------
    bool            HandlePickDefaultOrder(IshipIGC* pShip)
    {
        if (g_pMission)
            return g_pMission->HandlePickDefaultOrder(pShip);
        return false;
    }

    //------------------------------------------------------------------------------
    void            KillStationEvent(IstationIGC* pStation, ImodelIGC* pLauncher)
    {
        if (g_pMission)
            return g_pMission->KillStationEvent(pStation, pLauncher);
    }

    //------------------------------------------------------------------------------
    // control constraint functions - note that these could be inlined
    //------------------------------------------------------------------------------
    ControlData&    ApproveControls (ControlData& inputControls)
    {
        // copy the input controls for later retrieval
        g_inputControls = inputControls;

        // the fundamental limiter here is a multiplier that allows the commander of
        // the training mission to scale the control inputs as s/he sees fit. for the
        // most part, I expect that these values will be 0.0f or 1.0f
        inputControls.jsValues[c_axisYaw] *= g_allowableControlScalars.jsValues[c_axisYaw];
        inputControls.jsValues[c_axisPitch] *= g_allowableControlScalars.jsValues[c_axisPitch];
        inputControls.jsValues[c_axisRoll] *= g_allowableControlScalars.jsValues[c_axisRoll];

        // the throttle is mapping the range -1.0f to 1.0f from the joystick to the 
        // more useful range 0.0f to 1.0f, as a result we have to translate the value 
        // before scaling, then translate it back
        inputControls.jsValues[c_axisThrottle] = ((inputControls.jsValues[c_axisThrottle] + 1.0f) * g_allowableControlScalars.jsValues[c_axisThrottle]) - 1.0f;

        // here we check to see if the controls are specifically being overridden, and
        // if they are, we set them explicitly
        if (g_setInputControls.jsValues[c_axisYaw] != NA)
            inputControls.jsValues[c_axisYaw] = g_setInputControls.jsValues[c_axisYaw];
        if (g_setInputControls.jsValues[c_axisPitch] != NA)
            inputControls.jsValues[c_axisPitch] = g_setInputControls.jsValues[c_axisPitch];
        if (g_setInputControls.jsValues[c_axisRoll] != NA)
            inputControls.jsValues[c_axisRoll] = g_setInputControls.jsValues[c_axisRoll];
        if (g_setInputControls.jsValues[c_axisThrottle] != NA)
            inputControls.jsValues[c_axisThrottle] = (g_setInputControls.jsValues[c_axisThrottle] * 2.0f) - 1.0f;

        // the manipulated data is returned so it can be expressly used as a 
        // parameter in a function call
        return inputControls;
    }

    //------------------------------------------------------------------------------
    int             ApproveActions (int iInputMask)
    {
        // this gives the commander control over the actions by allowing her to
        // mask out the control inputs. The mask is combined with the actual
        // inputs using a "bitwise and", effectively disabling inputs whose mask
        // bit is not 1
        iInputMask &= g_iAllowableActionMask;

        // turn on whatever controls are being forced on
        iInputMask |= g_iSetInputAction;

        // return the result
        return iInputMask;
    }

    //------------------------------------------------------------------------------
}
