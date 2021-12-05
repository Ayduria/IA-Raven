#ifndef GOAL_FLEE_H
#define GOAL_FLEE_H
#pragma warning (disable:4786)

#include "Goals/Goal_Composite.h"
#include "Goals/Raven_Goal_Types.h"
#include "Raven_Bot.h"

class Goal_Flee : public Goal_Composite<Raven_Bot>
{
public:

	Goal_Flee(Raven_Bot* pBot) :Goal_Composite<Raven_Bot>(pBot, goal_flee) {}

	void Activate();
	int  Process();
	void Terminate() {}
};

#endif