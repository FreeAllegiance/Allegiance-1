
#include    "pch.h"
#include    "KillsReachedCondition.h"
#include    "Training.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  KillsReachedCondition::KillsReachedCondition(int kills) :
        m_kills(kills)
    {
    }

    //------------------------------------------------------------------------------
    /* void */  KillsReachedCondition::~KillsReachedCondition(void)
    {
    }

    //------------------------------------------------------------------------------
    bool        KillsReachedCondition::Evaluate(void)
    {
        if (Training::GetKillCount() >= m_kills)
            debugf(">>>reached %d kills\n", m_kills);
        return (Training::GetKillCount() >= m_kills);
    }
}