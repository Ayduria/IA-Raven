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

    Raven_Bot(world, pos)
{
    m_bLeader = false;
    m_leader = leader;
    m_pBrain = new Goal_ThinkTeammate(this, goal_thinkteammate);
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
    case Msg_HereMyStuff:
    {
        MyPos* pos = (MyPos*)msg.ExtraInfo;
        GetBrain()->AddGoal_MoveToPosition(Vector2D(pos->x, pos->y));
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

//--------------------------- Render -------------------------------------
//
//------------------------------------------------------------------------
void Raven_Teammate::Render()
{
    //when a bot is hit by a projectile this value is set to a constant user
    //defined value which dictates how long the bot should have a thick red
    //circle drawn around it (to indicate it's been hit) The circle is drawn
    //as long as this value is positive. (see Render)
    m_iNumUpdatesHitPersistant--;


    if (isDead() || isSpawning()) return;

    gdi->BluePen();

    m_vecBotVBTrans = WorldTransform(m_vecBotVB,
        Pos(),
        Facing(),
        Facing().Perp(),
        Scale());

    gdi->ClosedShape(m_vecBotVBTrans);

    //draw the head
    gdi->LightBlueBrush();
    gdi->Circle(Pos(), 6.0 * Scale().x);


    //render the bot's weapon
    m_pWeaponSys->RenderCurrentWeapon();

    //render a thick red circle if the bot gets hit by a weapon
    if (m_bHit)
    {
        gdi->ThickRedPen();
        gdi->HollowBrush();
        gdi->Circle(m_vPosition, BRadius() + 1);

        if (m_iNumUpdatesHitPersistant <= 0)
        {
            m_bHit = false;
        }
    }

    gdi->TransparentText();
    gdi->TextColor(0, 255, 0);

    if (UserOptions->m_bShowBotIDs)
    {
        gdi->TextAtPos(Pos().x - 10, Pos().y - 20, std::to_string(ID()));
    }

    if (UserOptions->m_bShowBotHealth)
    {
        gdi->TextAtPos(Pos().x - 40, Pos().y - 5, "H:" + std::to_string(Health()));
    }

    if (UserOptions->m_bShowScore)
    {
        gdi->TextAtPos(Pos().x - 40, Pos().y + 10, "Scr:" + std::to_string(Score()));
    }
}

bool Raven_Teammate::IsTargetFromTeam()
{
    return m_bIsTargetFromTeam;
}

void Raven_Teammate::ClearTargetFromTeam()
{
    m_bIsTargetFromTeam = false;
}