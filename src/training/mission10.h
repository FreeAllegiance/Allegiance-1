/*
**  File:	mission10.h
**
**  Description:
**      Header file for the training library "Mission FreeFlight" interface.
*/

#ifndef _MISSION_FREEFLIGHT_H_
#define _MISSION_FREEFLIGHT_H_

#ifndef _TRAINING_MISSION_H_
#include "TrainingMission.h"
#endif

#ifndef _GOAL_H_
#include "Goal.h"
#endif


namespace Training
{
    //------------------------------------------------------------------------------
    // class definitions
    //------------------------------------------------------------------------------
    class Mission10 : public TrainingMission
    {
    public:
                                        Mission10(void);
        virtual /* void */              ~Mission10(void);
        virtual int                     GetMissionID(void);
        virtual SectorID                GetStartSectorID(void);
        virtual bool                    RestoreShip(void);
        virtual bool                    ShipLanded(void);
        virtual bool                    HandlePickDefaultOrder(IshipIGC* pShip);

    protected:
        virtual void                    CreateUniverse(void);
        virtual Condition*              CreateMission(void);

        bool    bUndockHintShown;
    };

    //------------------------------------------------------------------------------
}

#endif  //_MISSION_FREEFLIGHT_H_
