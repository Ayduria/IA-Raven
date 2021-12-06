#ifndef BOMB_H
#define BOMB_H
#pragma warning (disable:4786)
//-----------------------------------------------------------------------------
//
//  Name:   Rocket.h
//
//  Author: Francis Tremblay (Copy of Mat Buckland's weapon code mostly)
//
//  Desc:   class to implement a bomb
//
//-----------------------------------------------------------------------------

#include "armory/Raven_Projectile.h"

class Raven_Bot;

class Bomb : public Raven_Projectile
{
private:

    //the radius of damage, once the rocket has impacted
    double    m_dBlastRadius;

    // test
    double    m_dBombSpeedModifier;

    // Timer
    double    m_dExplosionTime;
    std::clock_t m_cTimer;

    // Glue
    bool      m_bIsGluedOnBot;
    Raven_Bot* m_GluedBot;
    Vector2D    m_GluedOffset;

    //this is used to render the splash when the rocket impacts
    double    m_dCurrentBlastRadius;

    //If the rocket has impacted we test all bots to see if they are within the 
    //blast radius and reduce their health accordingly
    void InflictDamageOnBotsWithinBlastRadius();

    //tests the trajectory of the shell for an impact
    void TestForImpact();

    void TestForTimer();

public:

    Bomb(Raven_Bot* shooter, Vector2D target);

    void DestroyIfAttachedToID(int ID);

    void Render();

    void Update();

};


#endif