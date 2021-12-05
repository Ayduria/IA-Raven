#include "Goal_Flee.h"
#include "Goals/Raven_Feature.h"
#include "Goals/Goal_MoveToPosition.h"

//------------------------------- Activate ------------------------------------
//-----------------------------------------------------------------------------
void Goal_Flee::Activate()
{
    m_iStatus = active;

    //make sure the subgoal list is clear.
    RemoveAllSubgoals();

    // bot moves in opposite direction from target
    AddSubgoal(new Goal_MoveToPosition(m_pOwner, m_pOwner->GetTargetSys()->GetLastRecordedPosition().GetReverse()));

    //if target is no longer in view this goal can be removed from the queue
    if (!m_pOwner->GetTargetSys()->isTargetWithinFOV())
    {
        m_iStatus = completed;
    }
}

//------------------------------ Process --------------------------------------
//-----------------------------------------------------------------------------
int Goal_Flee::Process()
{
    //if status is inactive, call Activate()
    ActivateIfInactive();

    //process the subgoals
    m_iStatus = ProcessSubgoals();

    return m_iStatus;
}
