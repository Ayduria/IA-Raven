#include "ScavengeGoal_Evaluator.h"
#include "goals/Goal_Think.h"
#include "goals/Raven_Feature.h"
#include "misc/Stream_Utility_Functions.h"
#include "Raven_Map.h"
#include "Raven_Game.h"

#include <string>




//------------------------ CalculateDesirability ------------------------------
//-----------------------------------------------------------------------------
double ScavengeGoal_Evaluator::CalculateDesirability(Raven_Bot* pBot, MyPos* pPos)
{
	Raven_Map* map = pBot->GetWorld()->GetMap();

	Vector2D Pos = Vector2D(pPos->x, pPos->y);

	Raven_Map::GraphNode botPosition = map->GetClosestNodeIdToPosition(pBot->Pos());
	Raven_Map::GraphNode InventoryLocation = map->GetClosestNodeIdToPosition(Pos);

	m_distance = map->CalculateCostToTravelBetweenNodes(botPosition.Index(), InventoryLocation.Index());
	if (m_distance > 150)
		return 0;
	else
		return CalculateDesirability(pBot);

}

//------------------------ CalculateDesirability ------------------------------
//-----------------------------------------------------------------------------
double ScavengeGoal_Evaluator::CalculateDesirability(Raven_Bot* pBot)
{
	double tweaker = 0.015;
	double desirability = (150 * m_dCharacterBias * tweaker) / m_distance;
	Clamp(desirability, 0, 1);

	return desirability;
}

//----------------------------- SetGoal ---------------------------------------
//-----------------------------------------------------------------------------
void ScavengeGoal_Evaluator::SetGoal(Raven_Bot* pBot, MyPos* pPos)
{
	m_Pos = Vector2D(pPos->x, pPos->y);
	SetGoal(pBot);
}

//----------------------------- SetGoal ---------------------------------------
//-----------------------------------------------------------------------------
void ScavengeGoal_Evaluator::SetGoal(Raven_Bot* pBot)
{
	pBot->GetBrain()->AddGoal_Scavenge(m_Pos);
}

//-------------------------- RenderInfo ---------------------------------------
//-----------------------------------------------------------------------------
void ScavengeGoal_Evaluator::RenderInfo(Vector2D position, Raven_Bot* pBot)
{
	gdi->TextAtPos(position, "SC" + ttos(CalculateDesirability(pBot), 2));
}