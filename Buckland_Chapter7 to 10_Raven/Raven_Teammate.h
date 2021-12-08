#ifndef RAVEN_TEAMMATE_H
#define RAVEN_TEAMMATE_H
#pragma warning (disable:4786)

#include "Raven_Bot.h"
//-----------------------------------------------------------------------------
//
//  Name:   Raven_Teammate.h
//
//  Author: Vincent Blais and Francis Tremblay
//
//  Desc:
//-----------------------------------------------------------------------------
#include <vector>
#include <iosfwd>
#include <map>

#include "game/MovingEntity.h"
#include "misc/utils.h"
#include "Raven_TargetingSystem.h"
#include "Raven_Map.h"


class Raven_PathPlanner;
class Raven_Steering;
class Raven_Game;
class Regulator;
class Raven_Weapon;
struct Telegram;
class Raven_Bot;
class Goal_Think;
class Raven_WeaponSystem;
class Raven_SensoryMemory;

struct MyPos
{
	Vector2D Pos;
	void* TriggerType;
};

class Raven_Teammate : public Raven_Bot
{
private:
	Raven_Bot* m_leader;
	bool	   m_bIsTargetFromTeam;

public:
	Raven_Teammate(Raven_Game* world, Vector2D pos, Raven_Bot* leader);

	//the usual suspects
	void Update() override;
	bool HandleMessage(const Telegram& msg) override;

	//interface for human player
	void Exorcise() override;

	//spawns the bot at the given position
	void Spawn(Vector2D pos) override;

	//check if current target is from team
	bool IsTargetFromTeam();

	void ClearTargetFromTeam();

	Raven_Bot* GetLeader();
};

#endif