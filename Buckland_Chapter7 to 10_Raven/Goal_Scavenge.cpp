#include "Goal_Scavenge.h"
#include "Messaging/Telegram.h"
#include "Raven_Messages.h"
#include "Raven_Bot.h"
#include "Raven_Game.h"
#include "navigation/Raven_PathPlanner.h"

#include "goals/Goal_Think.h"
#include "goals/Goal_FollowPath.h"
#include "goals/Goal_SeekToPosition.h"
#include "goals/Goal_MoveToPosition.h"


//------------------------------- Activate ------------------------------------
//-----------------------------------------------------------------------------
void Goal_Scavenge::Activate()
{
	m_iStatus = active;

	RemoveAllSubgoals();
	// Pick a random location from the known existing packs
	m_packToScavenge = RandInt(0, (int) m_vDestinations.size() - 1);

	m_pPack = static_cast<Raven_Map::TriggerType*>(m_vDestinations[m_packToScavenge]->TriggerType);
	m_pPackLocation = m_pPack->Pos();

	if (m_pOwner->GetPathPlanner()->RequestPathToPosition(m_pPackLocation))
	{
		AddSubgoal(new Goal_MoveToPosition(m_pOwner, m_pPackLocation));
	}
}

//------------------------------ Process --------------------------------------
//-----------------------------------------------------------------------------
int Goal_Scavenge::Process()
{
	ActivateIfInactive();

	if (packHasBeenStolen())
	{
	  // remove a possible place for a pack of raven bot's memory
	  m_pOwner->GetBrain()->RemovePack(m_packToScavenge);
	  Terminate();
	}

	m_iStatus = ProcessSubgoals();

	// look Get Item
	// If box is not there, TERMINATE
	// if Box taken by me TERMINATE
	// Terminate + change bool MustScavenge of GoalThink to false
	return m_iStatus;
}

//---------------------------- HandleMessage ----------------------------------
//-----------------------------------------------------------------------------
bool Goal_Scavenge::HandleMessage(const Telegram& msg) 
{
	//first, pass the message down the goal hierarchy
	//bool bHandled = ForwardMessageToFrontMostSubgoal(msg);
	return ForwardMessageToFrontMostSubgoal(msg);
	//if the msg was not handled, test to see if this goal can handle it
	/*if (bHandled == false)
	{
		switch (msg.Msg)
		{
		case Msg_PathReady:

			//clear any existing goals
			RemoveAllSubgoals();
			AddSubgoal(new Goal_FollowPath(m_pOwner,
				m_pOwner->GetPathPlanner()->GetPath()));

			return true; //msg handled


		case Msg_NoPathAvailable:

			m_iStatus = failed;

			return true; //msg handled

		default: return false;
		}
	}

	//handled by subgoals
	return true;*/
}

bool Goal_Scavenge::packHasBeenStolen()
{
	// TODO check if pack still exist
	bool b1 = this->m_pPack;
	bool b2 = !m_pPack->isActive();
	bool b3 = m_pOwner->hasLOSto(m_pPack->Pos());

	return this->m_pPack &&
		!m_pPack->isActive() &&
		m_pOwner->hasLOSto(m_pPack->Pos());
}

void Goal_Scavenge::Render()
{
	//forward the request to the subgoals
	Goal_Composite<Raven_Bot>::Render();

	//draw a bullseye
	gdi->BlackPen();
	gdi->BlueBrush();
	gdi->Circle(m_pPackLocation, 6);
	gdi->RedBrush();
	gdi->RedPen();
	gdi->Circle(m_pPackLocation, 4);
	gdi->YellowBrush();
	gdi->YellowPen();
	gdi->Circle(m_pPackLocation, 2);
}