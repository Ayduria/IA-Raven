#include "Goal_Scavenge.h"
#include "Messaging/Telegram.h"
#include "Raven_Messages.h"
#include "Raven_Bot.h"
#include "Raven_Game.h"
#include "navigation/Raven_PathPlanner.h"

#include "goals/Goal_FollowPath.h"


//------------------------------- Activate ------------------------------------
//-----------------------------------------------------------------------------
void Goal_Scavenge::Activate()
{
	m_iStatus = active;

	RemoveAllSubgoals();
}

//------------------------------ Process --------------------------------------
//-----------------------------------------------------------------------------
int Goal_Scavenge::Process()
{
	ActivateIfInactive();

	m_iStatus = ProcessSubgoals();

	return m_iStatus;
}

//---------------------------- HandleMessage ----------------------------------
//-----------------------------------------------------------------------------
bool Goal_Scavenge::HandleMessage(const Telegram& msg) 
{
	//first, pass the message down the goal hierarchy
	bool bHandled = ForwardMessageToFrontMostSubgoal(msg);

	//if the msg was not handled, test to see if this goal can handle it
	if (bHandled == false)
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
	return true;
}

void Goal_Scavenge::Render()
{
	//forward the request to the subgoals
	Goal_Composite<Raven_Bot>::Render();

	//draw a bullseye
	gdi->BlackPen();
	gdi->BlueBrush();
	gdi->Circle(m_vDestination, 6);
	gdi->RedBrush();
	gdi->RedPen();
	gdi->Circle(m_vDestination, 4);
	gdi->YellowBrush();
	gdi->YellowPen();
	gdi->Circle(m_vDestination, 2);
}