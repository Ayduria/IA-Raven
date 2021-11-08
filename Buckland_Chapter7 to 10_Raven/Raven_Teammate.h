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

class Raven_Teammate : public Raven_Bot
{
private:
	Raven_Bot* m_leader;

public:
	Raven_Teammate(Raven_Game* world, Vector2D pos, Raven_Bot* leader);

	//the usual suspects
	void Render() override;
	void Update() override;

	//interface for human player
	void Exorcise() override;

	//spawns the bot at the given position
	void Spawn(Vector2D pos) override;

	Raven_Bot* GetLeader();
};

#endif