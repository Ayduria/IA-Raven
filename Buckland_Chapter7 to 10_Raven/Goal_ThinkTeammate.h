#ifndef GOAL_THINKTEAMMATE_H
#define GOAL_THINKTEAMMATE_H
#pragma warning (disable:4786)
//-----------------------------------------------------------------------------
//
//  Name:   Goal_Think .h
//
//  Author: Mat Buckland (www.ai-junkie.com)
//
//  Desc:   class to arbitrate between a collection of high level goals, and
//          to process those goals.
//-----------------------------------------------------------------------------
#include <vector>
#include <string>
#include "2d/Vector2D.h"
#include "goals/Goal_Think.h"
#include "Raven_Bot.h"
#include "goals/Goal_Evaluator.h"


class Goal_ThinkTeammate : public Goal_Think
{
public:
	Goal_ThinkTeammate(Raven_Bot* pBot, int goalType);
	~Goal_ThinkTeammate();
};

#endif