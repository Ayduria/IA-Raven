#pragma once
#include "Raven_Bot.h"
#include "Raven_Game.h"
#include "CData.h"
#include "CNeuralNet.h"

class LearningBot : public Raven_Bot
{
private:
	CNeuralNet m_ModeleAppris;

	int ramboScore;

	std::clock_t start;

public:

	LearningBot(Raven_Game* world, Vector2D pos);
	~LearningBot();


	void Update();

	int GetRamboScore() { return ramboScore; }
	void ResetRamboScore() { ramboScore = 0; }
	std::clock_t GetTimerTime() { return start; }
	void ResetRamboTimer() { start = std::clock(); }

	bool HandleMessage(const Telegram& msg) override;
};

