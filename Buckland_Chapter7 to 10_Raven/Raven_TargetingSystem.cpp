#include "Raven_TargetingSystem.h"
#include "Raven_Bot.h"
#include "Raven_Teammate.h"
#include "Raven_SensoryMemory.h"
#include "Messaging/MessageDispatcher.h"
#include "Raven_Messages.h"


//-------------------------------- ctor ---------------------------------------
//-----------------------------------------------------------------------------
Raven_TargetingSystem::Raven_TargetingSystem(Raven_Bot* owner):m_pOwner(owner),
                                                               m_pCurrentTarget(0)
{}



//----------------------------- Update ----------------------------------------

//-----------------------------------------------------------------------------
void Raven_TargetingSystem::Update()
{
    /// <summary>
    /// TODO: RECHECK PSQ LES TEAMMTATES DEVRAIT PAS VRM IGNORE LEURS ALENTOUR
    /// 
    /// - Envoie le message Msg_YoTeamINeedHelp quand on controle le joueur
    /// - Drop weapon et envoie au teammate la position
    ///     Get Held weapon of dead bot + ammo count
    ///     RavenWeapon* weapon = m_pWeaponSys->GetWeaponFromInventory();
    ///     int ammo = m_pWeaponSys->GetAmmoRemainingForWeapon();
    ///     spawn something on the ground (tombstone? square? something)
    ///     
    ///     Add a goal (to get item - go to position what ever that does it)
    ///     
    /// </summary>
    bool ignoreTargetingSystem = false;
    if (!m_pOwner->isLeader())
    {
        Raven_Teammate* owner = (Raven_Teammate*)m_pOwner;
        if (owner->IsTargetFromTeam())
        {
            Raven_Bot* target = owner->GetTargetBot();
            if (target)
            {
                if (owner->GetTargetBot()->isAlive())
                {
                    ignoreTargetingSystem = true;
                }
                else
                {
                    owner->ClearTargetFromTeam();
                }
            }
        }
    }

    int oldTargetId = 0;
    if (m_pCurrentTarget)
    {
        oldTargetId = m_pCurrentTarget->ID();
    }

    if (!ignoreTargetingSystem)
    {
        double ClosestDistSoFar = MaxDouble;
        m_pCurrentTarget = 0;

        //grab a list of all the opponents the owner can sense
        std::list<Raven_Bot*> SensedBots;
        SensedBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedOpponents();

        std::list<Raven_Bot*>::const_iterator curBot = SensedBots.begin();
        for (curBot; curBot != SensedBots.end(); ++curBot)
        {
            if ((*curBot != m_pOwner))
            {
                bool isTeammate = false;
                if (m_pOwner->isLeader())
                {
                    if (!(*curBot)->isLeader())
                    {
                        isTeammate = ((Raven_Teammate*)*curBot)->GetLeader() == m_pOwner;
                    }
                }
                else
                {
                    if ((*curBot)->isLeader())
                    {
                        isTeammate = ((Raven_Teammate*)m_pOwner)->GetLeader() == *curBot;
                    }
                    else
                    {
                        isTeammate = ((Raven_Teammate*)*curBot)->GetLeader() == ((Raven_Teammate*)m_pOwner)->GetLeader();
                    }
                }

                if ((*curBot)->isAlive() && !isTeammate)
                {
                    double dist = Vec2DDistanceSq((*curBot)->Pos(), m_pOwner->Pos());

                    if (dist < ClosestDistSoFar)
                    {
                        ClosestDistSoFar = dist;
                        m_pCurrentTarget = *curBot;
                    }
                }
            }
            //make sure the bot is alive and that it is not the owner
        }
    }

    if (m_pOwner->isLeader() && m_pCurrentTarget)
    {
        if (oldTargetId != m_pCurrentTarget->ID() && oldTargetId != 0)
        {
            std::vector<int> teammatesIDs = m_pOwner->GetTeammatesIDs();
            if (teammatesIDs.size() > 0)
            {
                Raven_Bot* target = m_pOwner->GetTargetSys()->GetTarget();
                if (target != m_pOwner)
                {
                    MemorySlice* slice = m_pOwner->GetSensoryMem()->GetMemorySliceOfOpponent(target);
                    for (size_t i = 0; i < teammatesIDs.size(); i++)
                    {
                        Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                            m_pOwner->ID(),
                            teammatesIDs[i],
                            Msg_YoTeamINeedHelp,
                            (void*)slice);
                    }
                }
            }
        }
    }
}




bool Raven_TargetingSystem::isTargetWithinFOV()const
{
  return m_pOwner->GetSensoryMem()->isOpponentWithinFOV(m_pCurrentTarget);
}

bool Raven_TargetingSystem::isTargetShootable()const
{
  return m_pOwner->GetSensoryMem()->isOpponentShootable(m_pCurrentTarget);
}

Vector2D Raven_TargetingSystem::GetLastRecordedPosition()const
{
  return m_pOwner->GetSensoryMem()->GetLastRecordedPositionOfOpponent(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenVisible()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenVisible(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenOutOfView()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenOutOfView(m_pCurrentTarget);
}
