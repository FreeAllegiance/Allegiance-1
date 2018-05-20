
#ifndef _ADD_RESPAWN_ACTION_H_
#define _ADD_RESPAWN_ACTION_H_

#ifndef _ACTION_H_
#include "Action.h"
#endif  //_ACTION_H_

#include"CreateDroneAction.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class definitions
    //------------------------------------------------------------------------------
    class AddRespawnAction : public Action
    {
    public:
        /* void */          AddRespawnAction(CreateDroneAction* cda, float downtime, char spawnGroup=NA, bool spawnOnAdd = true);
        virtual /* void */          ~AddRespawnAction(void);
        virtual void                Execute(void);

    protected:
        CreateDroneAction*  m_cda;
        float               m_Downtime;
        char                m_SpawnGroup;
        bool                m_SpawnOnAdd;
    };

    //------------------------------------------------------------------------------
}

#endif  //_DO_DAMAGE_ACTION_H_
