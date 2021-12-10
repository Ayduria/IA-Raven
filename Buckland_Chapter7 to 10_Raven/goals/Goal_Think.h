#ifndef GOAL_THINK_H
#define GOAL_THINK_H
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
#include "Goals/Goal_Composite.h"
#include "Goal_Evaluator.h"
#include "../Raven_Teammate.h"


class Goal_Think : public Goal_Composite<Raven_Bot>
{
private:
  
  typedef std::vector<Goal_Evaluator*>   GoalEvaluators;
  std::vector<MyPos*>					 m_scavengeables;

protected:
  
  GoalEvaluators  m_Evaluators;

public:

  Goal_Think(Raven_Bot* pBot, int goalType);
  ~Goal_Think();

  //this method iterates through each goal evaluator and selects the one
  //that has the highest score as the current goal
  void Arbitrate();

  //returns true if the given goal is not at the front of the subgoal list
  bool notPresent(unsigned int GoalType)const;

  void RemovePack(int index);

  //the usual suspects
  int  Process();
  void Activate();
  void Terminate(){}
  
  //top level goal types
  void AddGoal_MoveToPosition(Vector2D pos);
  void AddGoal_Scavenge();
  void AddGoal_GetItem(unsigned int ItemType);
  void AddGoal_Explore();
  void AddGoal_AttackTarget();

  //this adds the MoveToPosition goal to the *back* of the subgoal list.
  void QueueGoal_MoveToPosition(Vector2D pos);

  bool HandleMessage(const Telegram& msg);

  bool HasKnownScavengeables();

  //this renders the evaluations (goal scores) at the specified location
  void  RenderEvaluations(int left, int top)const;
  void  Render();

};


#endif