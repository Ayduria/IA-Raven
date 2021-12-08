#ifndef GOAL_SCAVENGE_H
#define GOAL_SCAVENGE_H
#pragma warning (disable:4786)

#include <vector>
#include "Goals/Goal_Composite.h"
#include "2D/Vector2D.h"
#include "Raven_Teammate.h"
#include "triggers/trigger.h"
#include "goals/Raven_Goal_Types.h"



class Goal_Scavenge : public Goal_Composite<Raven_Bot>
{
private:

    //the position the bot wants to reach
    std::vector<MyPos*> m_vDestinations;
    Trigger<Raven_Bot>* m_pPack = 0;
    Vector2D m_pPackLocation = Vector2D(0,0);

    int m_packToScavenge = 0;

    bool packHasBeenStolen();

public:

    Goal_Scavenge(Raven_Bot* pBot,
        std::vector<MyPos*> Scavengeables) :

        Goal_Composite<Raven_Bot>(pBot,
            goal_move_to_position),
        m_vDestinations(Scavengeables)
    {}

    //the usual suspects
    void Activate();
    int  Process();
    void Terminate() {}

    //this goal is able to accept messages
    bool HandleMessage(const Telegram& msg);

    void Render();
};

#endif