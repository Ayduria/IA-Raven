#include "Trigger_InventoryGiver.h"
#include "Raven_ObjectEnumerations.h"
#include "armory/Raven_Weapon.h"

Trigger_InventoryGiver::Trigger_InventoryGiver(int id, std::vector<WeaponData*> inventory, Vector2D position, int nodeIndex) : Trigger<Raven_Bot>(id),
                                                                                                                               mInventory(inventory)
{
    SetPos(position);
    AddCircularTriggerRegion(Pos(), script->GetDouble("DefaultGiverTriggerRange"));
    SetGraphNodeIndex(nodeIndex);
    SetActive();
    const int NumRocketVerts = 8;
    const Vector2D rip[NumRocketVerts] = { Vector2D(0, 3),
                                         Vector2D(1, 2),
                                         Vector2D(1, 0),
                                         Vector2D(2, -2),
                                         Vector2D(-2, -2),
                                         Vector2D(-1, 0),
                                         Vector2D(-1, 2),
                                         Vector2D(0, 3) };

    for (int i = 0; i < NumRocketVerts; ++i)
    {
        m_vecRLVB.push_back(rip[i]);
    }

    this->SetEntityType(type_pack);
}

void Trigger_InventoryGiver::Try(Raven_Bot* pBot)
{
	if (this->isTouchingTrigger(pBot->Pos(), pBot->BRadius()))
	{
		for(WeaponData* weaponData: mInventory)
		{
			pBot->GetWeaponSys()->AddWeapon(weaponData);
		}
        SetInactive();
        SetToBeRemovedFromGame();
	}
}

void Trigger_InventoryGiver::Render()
{
    gdi->HollowBrush();
    gdi->GreenPen();
    gdi->Rect(Pos().x - 10, Pos().y - 10, Pos().x + 10, Pos().y + 10);
    for (WeaponData* w : mInventory)
    {
        switch (w->weaponType)
        {
        case type_rail_gun:
        {
            gdi->BluePen();
            gdi->BlueBrush();
            gdi->Circle(Pos(), 3);
            gdi->ThickBluePen();
            gdi->Line(Pos(), Vector2D(Pos().x, Pos().y - 9));
        }

        break;

        case type_shotgun:
        {

            gdi->BlackBrush();
            gdi->BrownPen();
            const double sz = 3.0;
            gdi->Circle(Pos().x - sz, Pos().y, sz);
            gdi->Circle(Pos().x + sz, Pos().y, sz);
        }

        break;

        case type_rocket_launcher:
        {

            Vector2D facing(-1, 0);

            m_vecRLVBTrans = WorldTransform(m_vecRLVB,
                Pos(),
                facing,
                facing.Perp(),
                Vector2D(2.5, 2.5));

            gdi->RedPen();
            gdi->ClosedShape(m_vecRLVBTrans);
        }

        break;
        }
    }
}