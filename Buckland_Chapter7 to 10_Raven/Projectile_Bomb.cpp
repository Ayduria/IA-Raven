#include "Projectile_Bomb.h"
#include "lua/Raven_Scriptor.h"
#include "misc/cgdi.h"
#include "Raven_Bot.h"
#include "Raven_Game.h"
#include "constants.h"
#include "2d/WallIntersectionTests.h"
#include "Raven_Map.h"

#include "Raven_Messages.h"
#include "Messaging/MessageDispatcher.h"

//-------------------------- ctor ---------------------------------------------
//-----------------------------------------------------------------------------
Bomb::Bomb(Raven_Bot* shooter, Vector2D target) :

    Raven_Projectile(target,
        shooter->GetWorld(),
        shooter->ID(),
        shooter->Pos(),
        shooter->Facing(),
        script->GetInt("Bomb_Damage"),
        script->GetDouble("Bomb_Scale"),
        script->GetDouble("Bomb_MaxSpeed"),
        script->GetDouble("Bomb_Mass"),
        script->GetDouble("Bomb_MaxForce")),

    m_dCurrentBlastRadius(0.0),
    m_dBlastRadius(script->GetDouble("Bomb_BlastRadius")),
    m_dBombSpeedModifier(1),
    m_dExplosionTime(script->GetDouble("Bomb_TimeToExplode")),
    m_bIsGluedOnBot(false)
{
    m_cTimer = std::clock();
    assert(target != Vector2D());
}


//------------------------------ Update ---------------------------------------
//-----------------------------------------------------------------------------
void Bomb::Update()
{
    if (!m_bImpacted && !m_bIsGluedOnBot)
    {
        // Hardcoded physics cause why not
        if (m_dBombSpeedModifier > 0.7)
            m_vVelocity += Heading();

        if (m_dBombSpeedModifier > 0)
            m_dBombSpeedModifier -= 0.01;

        m_vVelocity = m_vVelocity * 0.95;

        if (m_vVelocity.Length() <= 0.5)
        {
            m_vVelocity = Vector2D(0, 0);
        }

        //make sure vehicle does not exceed maximum velocity
        m_vVelocity.Truncate(m_dMaxSpeed);

        //update the position
        m_vPosition += m_vVelocity;

        TestForImpact();

        TestForTimer();
    }
    else if (m_bImpacted)
    {
        m_dCurrentBlastRadius += script->GetDouble("Bomb_ExplosionDecayRate");

        //when the rendered blast circle becomes equal in size to the blast radius
        //the rocket can be removed from the game
        if (m_dCurrentBlastRadius > m_dBlastRadius)
        {
            m_bDead = true;
        }
    }
    else if (m_bIsGluedOnBot)
    {
        if (m_GluedBot->isDead())
        {
            m_bDead = true;
        }
        else
        {
            Vector2D botFacing = m_GluedBot->Facing();

            double angle = atan2(botFacing.y, botFacing.x) * 180 / pi;
            if (angle < 0)
                angle = 360 - abs(angle);
            double theta = DegsToRads(angle);

            double cs = cos(theta);
            double sn = sin(theta);

            double px = m_GluedOffset.x * cs - m_GluedOffset.y * sn;
            double py = m_GluedOffset.x * sn + m_GluedOffset.y * cs;

            Vector2D newPos = Vector2D(px, py);

            m_vPosition = m_GluedBot->Pos() + newPos;
            TestForTimer();
        }
    }
}

void Bomb::TestForImpact()
{

    //if the projectile has reached the target position or it hits an entity
    //or wall it should explode/inflict damage/whatever and then mark itself
    //as dead


    //test to see if the line segment connecting the rocket's current position
    //and previous position intersects with any bots.
    Raven_Bot* hit = GetClosestIntersectingBot(m_vPosition - m_vVelocity, m_vPosition);

    //if hit
    if (hit)
    {
        m_bIsGluedOnBot = true;
        m_GluedBot = hit;
        m_vVelocity = Vector2D(0, 0);
        m_dBombSpeedModifier = 0;

        Vector2D botPosition = hit->Pos();
        Vector2D bombPosition = Pos();
        m_GluedOffset = bombPosition - botPosition;

        double angle = atan2(hit->Facing().y, hit->Facing().x) * 180 / pi;
        if (angle < 0)
            angle = 360 - abs(angle);
        double theta = DegsToRads(-angle);

        double cs = cos(theta);
        double sn = sin(theta);

        double px = m_GluedOffset.x * cs - m_GluedOffset.y * sn;
        double py = m_GluedOffset.x * sn + m_GluedOffset.y * cs;

        m_GluedOffset = Vector2D(px, py);
        m_vPosition = hit->Pos() + m_GluedOffset;
    }

    //test for impact with a wall
    double dist;
    if (FindClosestPointOfIntersectionWithWalls(m_vPosition - m_vVelocity,
        m_vPosition,
        dist,
        m_vImpactPoint,
        m_pWorld->GetMap()->GetWalls()))
    {
        m_vVelocity = Vector2D(0, 0);
        m_dBombSpeedModifier = 0;
        return;
    }
}

void Bomb::TestForTimer()
{
    if (((std::clock() - m_cTimer) / (double)CLOCKS_PER_SEC) > m_dExplosionTime)
    {
        m_bImpacted = true;
        InflictDamageOnBotsWithinBlastRadius();
    }
}

//--------------- InflictDamageOnBotsWithinBlastRadius ------------------------
//
//  If the rocket has impacted we test all bots to see if they are within the 
//  blast radius and reduce their health accordingly
//-----------------------------------------------------------------------------
void Bomb::InflictDamageOnBotsWithinBlastRadius()
{
    std::list<Raven_Bot*>::const_iterator curBot = m_pWorld->GetAllBots().begin();

    for (curBot; curBot != m_pWorld->GetAllBots().end(); ++curBot)
    {
        if (Vec2DDistance(Pos(), (*curBot)->Pos()) < m_dBlastRadius + (*curBot)->BRadius())
        {
            //send a message to the bot to let it know it's been hit, and who the
            //shot came from
            Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                m_iShooterID,
                (*curBot)->ID(),
                Msg_TakeThatMF,
                (void*)&m_iDamageInflicted);

        }
    }
}


//-------------------------- Render -------------------------------------------
//-----------------------------------------------------------------------------
void Bomb::Render()
{
    if (!m_bDead)
    {
        gdi->ThickBlackPen();
        gdi->GreenBrush();
        gdi->Circle(Pos(), 2);

        if (m_bImpacted)
        {
            gdi->HollowBrush();
            gdi->Circle(Pos(), m_dCurrentBlastRadius);
        }
    }
}

void Bomb::DestroyIfAttachedToID(int ID)
{
    if (m_bIsGluedOnBot)
    {
        if (m_GluedBot->ID() == ID)
        {
            m_bDead = true;
        }
    }
}