#include "Goal_ThinkTeammate.h"
#include <list>
#include "misc/Cgdi.h"
#include "Raven_ObjectEnumerations.h"
#include "misc/utils.h"
#include "lua/Raven_Scriptor.h"

#include "goals/Goal_MoveToPosition.h"
#include "goals/Goal_Explore.h"
#include "goals/Goal_GetItem.h"
#include "goals/Goal_Wander.h"
#include "goals/Raven_Goal_Types.h"
#include "goals/Goal_AttackTarget.h"


#include "goals/GetWeaponGoal_Evaluator.h"
#include "goals/GetHealthGoal_Evaluator.h"
#include "goals/ExploreGoal_Evaluator.h"
#include "goals/AttackTargetGoal_Evaluator.h"

Goal_ThinkTeammate::Goal_ThinkTeammate(Raven_Bot* pBot, int goalType): Goal_Think(pBot, goalType)
{
	
    //these biases could be loaded in from a script on a per bot basis
    //but for now we'll just give them some random values
    const double LowRangeOfBias = 0.5;
    const double HighRangeOfBias = 1.5;

    double HealthBias = RandInRange(LowRangeOfBias, HighRangeOfBias);
    double ShotgunBias = RandInRange(LowRangeOfBias, HighRangeOfBias);
    double RocketLauncherBias = RandInRange(LowRangeOfBias, HighRangeOfBias);
    double RailgunBias = RandInRange(LowRangeOfBias, HighRangeOfBias);
    double GrenadeBias = RandInRange(LowRangeOfBias, HighRangeOfBias);
    double ExploreBias = RandInRange(LowRangeOfBias, HighRangeOfBias);
    double AttackBias = 5;

    //create the evaluator objects
    m_Evaluators.clear();
    m_Evaluators.push_back(new GetHealthGoal_Evaluator(HealthBias));
    m_Evaluators.push_back(new ExploreGoal_Evaluator(ExploreBias));
    m_Evaluators.push_back(new AttackTargetGoal_Evaluator(AttackBias));
    m_Evaluators.push_back(new GetWeaponGoal_Evaluator(ShotgunBias,
        type_shotgun));
    m_Evaluators.push_back(new GetWeaponGoal_Evaluator(RailgunBias,
        type_rail_gun));
    m_Evaluators.push_back(new GetWeaponGoal_Evaluator(RocketLauncherBias,
        type_rocket_launcher));
    m_Evaluators.push_back(new GetWeaponGoal_Evaluator(GrenadeBias,
        type_grenade));
}