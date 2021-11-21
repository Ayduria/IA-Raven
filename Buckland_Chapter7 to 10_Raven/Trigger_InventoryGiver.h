#pragma once
#include "Triggers/Trigger.h"
#include "Raven_WeaponSystem.h"
#include "Raven_Bot.h"

class Trigger_InventoryGiver :  public Trigger<Raven_Bot>
{
	// Inventory of deceased
	std::vector<WeaponData*> mInventory;
	// Position of death
	Vector2D mPosition;
	//vrtex buffers for rocket shape
	std::vector<Vector2D>         m_vecRLVB;
	std::vector<Vector2D>         m_vecRLVBTrans;

public:
	Trigger_InventoryGiver(int id, std::vector<WeaponData*> inventory, Vector2D position, int nodeIndex);
	~Trigger_InventoryGiver() {};
	void Try(Raven_Bot* pBot);
	// we do not need to update this trigger since he will be deleted when a bot will take the inventory.
	void Update() {};
	// draw each symbol on top of one another 
	void Render();
};

