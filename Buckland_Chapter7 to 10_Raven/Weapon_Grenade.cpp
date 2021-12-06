#include "Weapon_Grenade.h"
#include "Raven_Bot.h"
#include "misc/Cgdi.h"
#include "Raven_Game.h"
#include "Raven_Map.h"
#include "lua/Raven_Scriptor.h"
#include "misc/utils.h"
#include "fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
Grenade::Grenade(Raven_Bot* owner) :

    Raven_Weapon(type_grenade,
        script->GetInt("Grenade_DefaultRounds"),
        script->GetInt("Grenade_MaxRoundsCarried"),
        script->GetDouble("Grenade_FiringFreq"),
        script->GetDouble("Grenade_IdealRange"),
        script->GetDouble("Bomb_MaxSpeed"),
        owner)
{
        //setup the vertex buffer
        const int NumWeaponVerts = 4;
        const Vector2D weapon[NumWeaponVerts] = { Vector2D(0, -3),
                                                 Vector2D(4, -6),
                                                 Vector2D(0, -9),
                                                 Vector2D(-4, -6)
        };
        for (int vtx = 0; vtx < NumWeaponVerts; ++vtx)
        {
            m_vecWeaponVB.push_back(weapon[vtx]);
        }

        //setup the fuzzy module
        InitializeFuzzyModule();

}

//------------------------------ ShootAt --------------------------------------

inline void Grenade::ShootAt(Vector2D pos)
{
    if (NumRoundsRemaining() > 0 && isReadyForNextShot())
    {
        //fire off a rocket!
        m_pOwner->GetWorld()->AddBomb(m_pOwner, pos);

        m_iNumRoundsLeft--;

        UpdateTimeWeaponIsNextAvailable();

        //add a trigger to the game so that the other bots can hear this shot
        //(provided they are within range)
        m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("Grenade_SoundRange"));
    }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
inline double Grenade::GetDesirability(double DistToTarget)
{
    if (m_iNumRoundsLeft == 0)
    {
        m_dLastDesirabilityScore = 0;
    }
    else
    {
        //fuzzify distance and amount of ammo
        m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
        m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

        m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
    }

    return m_dLastDesirabilityScore;
}

//--------------------------- InitializeFuzzyModule ---------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void Grenade::InitializeFuzzyModule()
{
    FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

    FzSet& Target_VeryClose = DistToTarget.AddLeftShoulderSet("Target_VeryClose", 0, 10, 50);
    FzSet& Target_Close = DistToTarget.AddTriangularSet("Target_Close", 10, 50, 150);
    FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium", 25, 150, 300);
    FzSet& Target_Far = DistToTarget.AddTriangularSet("Target_Far", 150, 300, 1000);
    FzSet& Target_VeryFar = DistToTarget.AddRightShoulderSet("Target_VeryFar", 300, 1000, 1500);

    FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");

    FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 60, 80, 100);
    FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 35, 60, 80);
    FzSet& SlightlyDesirable = Desirability.AddTriangularSet("SlightlyDesirable", 20, 35, 50);
    FzSet& Undesirable = Desirability.AddTriangularSet("Undesirable", 10, 20, 35);
    FzSet& VeryUndesirable = Desirability.AddLeftShoulderSet("VeryUndesirable", 0, 10, 20);

    FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");

    FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 50, 75, 100);
    FzSet& Ammo_High = AmmoStatus.AddTriangularSet("Ammo_High", 25, 50, 75);
    FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 10, 25, 50);
    FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 10, 25);
    FzSet& Ammo_VeryLow = AmmoStatus.AddTriangularSet("Ammo_VeryLow", 0, 0, 10);

    m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Loads), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_High), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Okay), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Low), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_VeryLow), VeryUndesirable);

    m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_High), Undesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_VeryLow), Undesirable);

    m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), SlightlyDesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_High), SlightlyDesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), SlightlyDesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), SlightlyDesirable);
    m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_VeryLow), SlightlyDesirable);

    m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);
    m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_High), Desirable);
    m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), Desirable);
    m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Desirable);
    m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_VeryLow), Undesirable);

    m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Loads), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_High), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Okay), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Low), VeryUndesirable);
    m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_VeryLow), VeryUndesirable);
}

//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void Grenade::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
        m_pOwner->Pos(),
        m_pOwner->Facing(),
        m_pOwner->Facing().Perp(),
        m_pOwner->Scale());

    gdi->ThickBlackPen();

    gdi->ClosedShape(m_vecWeaponVBTrans);
}