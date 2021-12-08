#ifndef RAVEN_SCAVENGE_EVALUATOR
#define RAVEN_SCAVENGE_EVALUATOR
#pragma warning (disable:4786)
//-----------------------------------------------------------------------------
//
//  Name:   GetWeaponGoal_Evaluator.h
//
//  Author: Mat Buckland (www.ai-junkie.com)
//
//  Desc:  class to calculate how desirable the goal of fetching a teammate's inventory 
//         is 
//-----------------------------------------------------------------------------

#include "goals/Goal_Evaluator.h"
#include "Raven_Bot.h"
#include "Raven_Teammate.h"

class ScavengeGoal_Evaluator : public Goal_Evaluator
{

public:

    ScavengeGoal_Evaluator(double bias) :Goal_Evaluator(bias)
    {}

    double CalculateDesirability(Raven_Bot* pBot);

    void  SetGoal(Raven_Bot* pBot);

    void  RenderInfo(Vector2D Position, Raven_Bot* pBot);
};

#endif