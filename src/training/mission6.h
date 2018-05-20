/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	Mission6.h
**
**  Author:  
**
**  Description:
**      Header file for the training library "Mission6" interface.
**
**  History:
*/
#ifndef _MISSION_PRACTICE_ARENA_H_
#define _MISSION_PRACTICE_ARENA_H_

#ifndef _TRAINING_MISSION_H_
#include "TrainingMission.h"
#endif  //_TRAINING_MISSION_H_

#ifndef _GOAL_H_
#include "Goal.h"
#endif  //_GOAL_H_

namespace Training
{
    //------------------------------------------------------------------------------
    // class definitions
    //------------------------------------------------------------------------------
    class Mission6 : public TrainingMission
    {
        public:
                                            Mission6(void);
            virtual /* void */              ~Mission6 (void);
            virtual int                     GetMissionID (void);
            virtual SectorID                GetStartSectorID (void);
            virtual bool                    RestoreShip (void);
            virtual bool                    ShipLanded(void);
            virtual bool                    HandlePickDefaultOrder(IshipIGC* pShip);

        protected:
            virtual void                    CreateUniverse (void);
            virtual Condition*              CreateMission (void);

        bool    bUndockHintShown;
    };

    //------------------------------------------------------------------------------
}

#endif  //_MISSION_PRACTICE_ARENA_H_
