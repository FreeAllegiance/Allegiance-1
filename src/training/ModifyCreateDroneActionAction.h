
#ifndef _MCDA_ACTION_H_
#define _MCDA_ACTION_H_

#ifndef _ACTION_H_
#include "Action.h"
#endif// _ACTION_H_
#include "CreateDroneAction.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class definitions
    //------------------------------------------------------------------------------
    class ModifyCreateDroneActionAction : public Action
    {
    public:
        /* void */      ModifyCreateDroneActionAction(CreateDroneAction* cda);
        /* void */      ~ModifyCreateDroneActionAction(void);
        void            Execute(void);
        void            ModifyCreatedBehaviour(WingmanBehaviourBitMask behaviour);
        void            ModifyCreatedTechLevel(char techLevel);
        void            ModifyCreatedHullType(HullID hullID);
        void            ModifyCreatedName(ZString name);
        void            ModifyCreatedLocation(SectorID sectorID, const Vector& position);
        void            ModifyCreatedCommand(int targetID, ObjectType targetType, CommandID command);

    protected:
        CreateDroneAction*      m_cda;
        WingmanBehaviourBitMask m_behaviour;
        char                    m_techLevel;
        HullID                  m_hullID;
        bool                    b_changeName;
        ZString                 m_name;
        SectorID                m_locationSector;
        Vector                  m_locationPosition;
        int                     m_commandTargetID;
        ObjectType              m_commandTargetType;
        CommandID               m_commandID;
    };

    //------------------------------------------------------------------------------
}

#endif  //_MCDA_ACTION_H_
