#include "ScavengeGoal_Evaluator.h"
#include "goals/Goal_Think.h"
#include "goals/Raven_Feature.h"
#include "misc/Stream_Utility_Functions.h"
#include "Raven_Map.h"
#include "Raven_Game.h"

#include <string>

//------------------------ CalculateDesirability ------------------------------
//-----------------------------------------------------------------------------
double ScavengeGoal_Evaluator::CalculateDesirability(Raven_Bot* pBot)
{
	double distance = Raven_Feature::DistanceToItem(pBot, type_pack);
	if (distance > 0.6 || !pBot->GetBrain()->HasKnownScavengeables())
		return 0;
	else
	{
		double tweaker = 0.5;

		double desirability = (0.6 * m_dCharacterBias * tweaker) / distance;
		Clamp(desirability, 0, 1);

		return desirability;
	}
}

//----------------------------- SetGoal ---------------------------------------
//-----------------------------------------------------------------------------
void ScavengeGoal_Evaluator::SetGoal(Raven_Bot* pBot)
{
	pBot->GetBrain()->AddGoal_Scavenge();
}

//-------------------------- RenderInfo ---------------------------------------
//-----------------------------------------------------------------------------
void ScavengeGoal_Evaluator::RenderInfo(Vector2D position, Raven_Bot* pBot)
{
	gdi->TextAtPos(position, "SC" + ttos(CalculateDesirability(pBot), 2));
}