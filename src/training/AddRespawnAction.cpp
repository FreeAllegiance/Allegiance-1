

#include    "pch.h"
#include    "AddRespawnAction.h"
#include    "TrainingMission.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // global variabes
    //------------------------------------------------------------------------------
    extern TrainingMission*    g_pMission;

    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  AddRespawnAction::AddRespawnAction(CreateDroneAction* cda, float downtime, char spawnGroup, bool spawnOnAdd) :
        m_cda(cda),
        m_Downtime(downtime),
        m_SpawnGroup(spawnGroup),
        m_SpawnOnAdd(spawnOnAdd)
    {
    }

    //------------------------------------------------------------------------------
    /* void */  AddRespawnAction::~AddRespawnAction(void)
    {
    }

    //------------------------------------------------------------------------------
    void        AddRespawnAction::Execute(void)
    {
        g_pMission->AddRespawn(m_cda, m_Downtime, m_SpawnGroup, m_SpawnOnAdd);
    }

    //------------------------------------------------------------------------------
}