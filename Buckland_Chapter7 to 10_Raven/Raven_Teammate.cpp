#include "Raven_Teammate.h"
#include "misc/Cgdi.h"
#include "misc/utils.h"
#include "2D/Transformations.h"
#include "2D/Geometry.h"
#include "lua/Raven_Scriptor.h"
#include "Raven_Game.h"
#include "navigation/Raven_PathPlanner.h"
#include "Raven_SteeringBehaviors.h"
#include "Raven_UserOptions.h"
#include "time/Regulator.h"
#include "Raven_WeaponSystem.h"
#include "Raven_SensoryMemory.h"

#include "Messaging/Telegram.h"
#include "Raven_Messages.h"
#include "Messaging/MessageDispatcher.h"

#include "goals/Raven_Goal_Types.h"
#include "Goal_ThinkTeammate.h"

#include "Debug/DebugConsole.h"

//-------------------------- ctor ---------------------------------------------
Raven_Teammate::Raven_Teammate(Raven_Game* world, Vector2D pos, Raven_Bot* leader) :
    Raven_Bot(world, pos, script->GetDouble("Bot_Scale"), leader->GetHeadColor())
{
    m_bLeader = false;
    m_leader = leader;
    m_pBrain = new Goal_ThinkTeammate(this, goal_thinkteammate);
    m_bIsTargetFromTeam = false;
}


//------------------------------- Spawn ---------------------------------------
//
//  spawns the bot at the given position
//-----------------------------------------------------------------------------
void Raven_Teammate::Spawn(Vector2D pos)
{
    SetAlive();
    m_pBrain->RemoveAllSubgoals();
    m_pTargSys->ClearTarget();
    // Spawning near leader position
    SetPos(m_leader->Pos() + Vector2D(1, 0));
    m_pWeaponSys->Initialize();
    RestoreHealthToMaximum();
}

//-------------------------------- Update -------------------------------------
//
void Raven_Teammate::Update()
{
    Raven_Bot::Update();
}

//-------------------------------- HandleMessage -------------------------------------
//
bool Raven_Teammate::HandleMessage(const Telegram& msg)
{
    if (Raven_Bot::HandleMessage(msg))
        return true;
    //handle any messages not handles by the goals
    switch (msg.Msg)
    {
    case Msg_YoTeamINeedHelp:
    {
        MemorySlice* slice = (MemorySlice*)msg.ExtraInfo;
        m_pSensoryMem->UpdateBotFromMemory(slice);
        m_pTargSys->SetTarget(slice->opponent);
        m_bIsTargetFromTeam = true;
        GetBrain()->AddGoal_AttackTarget();
        return true;
    }
    default: return false;
    }
}

Raven_Bot* Raven_Teammate::GetLeader() 
{
    return m_leader;
}

//------------------------------- Exorcise ------------------------------------
//
//  called when a human is exorcised from this bot and the AI takes control
//-----------------------------------------------------------------------------
void Raven_Teammate::Exorcise()
{
    // Empty since we will never pocess a teammate.
}

bool Raven_Teammate::IsTargetFromTeam()
{
    return m_bIsTargetFromTeam;
}

void Raven_Teammate::ClearTargetFromTeam()
{
    m_bIsTargetFromTeam = false;
}