
#ifndef _KILLS_REACHED_CONDITION_H_
#define _KILLS_REACHED_CONDITION_H_

#ifndef _CONDITION_H_
#include "Condition.h"
#endif  //_CONDITION_H_

namespace Training
{
    //------------------------------------------------------------------------------
    // class definitions
    //------------------------------------------------------------------------------
    class KillsReachedCondition : public Condition
    {
    public:
                /* void */          KillsReachedCondition(int kills);
                /* void */          ~KillsReachedCondition(void);
                bool                Evaluate(void);

    protected:
        int m_kills;
    };

    //------------------------------------------------------------------------------
}

#endif  //_KILLS_REACHED_CONDITION_H_
