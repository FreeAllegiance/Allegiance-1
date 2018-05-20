/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	training.h
**
**  Author:  
**
**  Description:
**      Header file for the training library external interfaces.
**
**  History:
*/
#ifndef _TRAINING_H_
#define _TRAINING_H_

namespace Training
{
    //------------------------------------------------------------------------------
    // mission ids
    //------------------------------------------------------------------------------
    enum
    {
        c_TM_1_Introduction     = 1,
        c_TM_2_Basic_Flight     = 2,
        c_TM_3_Basic_Weaponry   = 3,
        c_TM_4_Enemy_Engagement = 4,
        c_TM_5_Command_View     = 5,
        c_TM_6_Practice_Arena   = 6,
        c_TM_7_Live             = 7,
        c_TM_8_Nanite           = 8, //TheBored 06-JUL-07: adding nanite mission
        c_TM_10_Free_Flight     = 10
    };

    //------------------------------------------------------------------------------
    // primary interface to the training missions
    //------------------------------------------------------------------------------
    bool            StartMission (int iMissionIndex);
    bool            HandleMission (void);
    bool            EndMission (void);
    void            SetSkipPostSlideshow (void);
    bool            IsTraining (void);
    bool            IsInstalled (void);
    bool            CommandViewEnabled(void);
    int             GetTrainingMissionID (void);
    SectorID        GetStartSectorID (void);
    int             GetKillCount(void);
    void            SetupShipAndCamera (void);

    //------------------------------------------------------------------------------
    // event trapping
    //------------------------------------------------------------------------------
    void            RecordChat (ChatTarget recipient);
    bool            RecordKeyPress (TrekKey key);
    bool            ShipLanded (void);
    void            ShipDied (ImodelIGC* pLauncher);
    bool            RestoreShip (void);
    void            ShipKilled(IshipIGC* pShip, ImodelIGC* pLauncher);
    bool            HandlePickDefaultOrder(IshipIGC* pShip);
    void            KillStationEvent(IstationIGC* pStation, ImodelIGC* pLauncher);

    //------------------------------------------------------------------------------
    // control constraint functions - note that these could be inlined
    //------------------------------------------------------------------------------
    ControlData&    ApproveControls (ControlData& inputControls);
    int             ApproveActions (int iInputMask);

    //------------------------------------------------------------------------------
}

#endif  //_TRAINING_H_
