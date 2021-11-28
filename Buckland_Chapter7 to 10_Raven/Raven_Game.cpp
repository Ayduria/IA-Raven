#include "Raven_Game.h"
#include "LearningBot.h"
#include "Raven_ObjectEnumerations.h"
#include "misc/WindowUtils.h"
#include "misc/Cgdi.h"
#include "Raven_SteeringBehaviors.h"
#include "lua/Raven_Scriptor.h"
#include "navigation/Raven_PathPlanner.h"
#include "game/EntityManager.h"
#include "2d/WallIntersectionTests.h"
#include "Raven_Map.h"
#include "Raven_Door.h"
#include "Raven_UserOptions.h"
#include "Time/PrecisionTimer.h"
#include "Raven_SensoryMemory.h"
#include "Raven_WeaponSystem.h"
#include "armory/Raven_Weapon.h"
#include "messaging/MessageDispatcher.h"
#include "Raven_Messages.h"
#include "GraveMarkers.h"

#include "armory/Raven_Projectile.h"
#include "armory/Projectile_Rocket.h"
#include "armory/Projectile_Pellet.h"
#include "armory/Projectile_Slug.h"
#include "armory/Projectile_Bolt.h"

#include "goals/Goal_Think.h"
#include "goals/Raven_Goal_Types.h"

#include <thread> // pour la fonction d'apprentissage


//uncomment to write object creation/deletion to debug console
//#define  LOG_CREATIONAL_STUFF
#ifdef LOG_CREATIONAL_STUFF
#include "misc/Stream_Utility_Functions.h"
#endif // LOG_CREATIONAL_STUFF

#include "debug/DebugConsole.h"

//----------------------------- ctor ------------------------------------------
//-----------------------------------------------------------------------------
Raven_Game::Raven_Game():m_pSelectedBot(NULL),
                         m_bPaused(false),
                         m_bRemoveABot(false),
                         m_bRemoveATeammateBot(false),
                         m_isRamboSpawned(false),
                         m_estEntraine(false),
                         m_bRemoveRambo(false),
                         m_hasReadDataFile(false),
                         m_loadRamboData(false),
                         m_CanPopulateRamboData(false),
                         m_pMap(NULL),
                         m_pPathManager(NULL),
                         m_pGraveMarkers(NULL)
{
  //Populate head color
  m_pHeadColorList.push_back(HeadColor::black);
  m_pHeadColorList.push_back(HeadColor::blue);
  m_pHeadColorList.push_back(HeadColor::brown);
  m_pHeadColorList.push_back(HeadColor::darkgreen);
  m_pHeadColorList.push_back(HeadColor::green);
  m_pHeadColorList.push_back(HeadColor::grey);
  m_pHeadColorList.push_back(HeadColor::yellow);
  m_pHeadColorList.push_back(HeadColor::lightblue);
  m_pHeadColorList.push_back(HeadColor::orange);
  m_pHeadColorList.push_back(HeadColor::white);

  //load in the default map
  LoadMap(script->GetString("StartMap"));

  // chaque début d'un nouveau jeu. ré-initilisaliser le dataset d'entrainement

  m_TrainingSet = CData();

  m_LancerApprentissage = false;
}


//------------------------------ dtor -----------------------------------------
//-----------------------------------------------------------------------------
Raven_Game::~Raven_Game()
{
  Clear();
  delete m_pPathManager;
  delete m_pMap;
  
  delete m_pGraveMarkers;
}


//---------------------------- Clear ------------------------------------------
//
//  deletes all the current objects ready for a map load
//-----------------------------------------------------------------------------
void Raven_Game::Clear()
{
#ifdef LOG_CREATIONAL_STUFF
    debug_con << "\n------------------------------ Clearup -------------------------------" <<"";
#endif

  //delete the bots
  std::list<Raven_Bot*>::iterator it = m_Bots.begin();
  for (it; it != m_Bots.end(); ++it)
  {
#ifdef LOG_CREATIONAL_STUFF
    debug_con << "deleting entity id: " << (*it)->ID() << " of type "
              << GetNameOfType((*it)->EntityType()) << "(" << (*it)->EntityType() << ")" <<"";
#endif

    delete *it;
  }

  //delete any active projectiles
  std::list<Raven_Projectile*>::iterator curW = m_Projectiles.begin();
  for (curW; curW != m_Projectiles.end(); ++curW)
  {
#ifdef LOG_CREATIONAL_STUFF
    debug_con << "deleting projectile id: " << (*curW)->ID() << "";
#endif

    delete *curW;
  }

  //clear the containers
  m_Projectiles.clear();
  m_Bots.clear();

  m_pSelectedBot = NULL;


}

void Raven_Game::TrainThread() {

    m_LancerApprentissage = true;

    debug_con << "lancement de l'apprentissage" << "";

    m_ModeleApprentissage = CNeuralNet(m_TrainingSet.GetInputNb(), m_TrainingSet.GetTargetsNb(), NUM_HIDDEN_NEURONS, LEARNING_RATE);
    bool isTraining = m_ModeleApprentissage.Train(&m_TrainingSet);

    if (isTraining) {
        debug_con << "Modele d'apprentissage de tir est appris" << "";
        m_estEntraine = true;

    }

}

//-------------------------------- Update -------------------------------------
//
//  calls the update function of each entity
//-----------------------------------------------------------------------------
void Raven_Game::Update()
{ 
  //don't update if the user has paused the game
  if (m_bPaused) return;

  m_pGraveMarkers->Update();

  //get any player keyboard input
  GetPlayerInput();
  
  //update all the queued searches in the path manager
  m_pPathManager->UpdateSearches();

  //update any doors
  std::vector<Raven_Door*>::iterator curDoor =m_pMap->GetDoors().begin();
  for (curDoor; curDoor != m_pMap->GetDoors().end(); ++curDoor)
  {
    (*curDoor)->Update();
  }

  //update any current projectiles
  std::list<Raven_Projectile*>::iterator curW = m_Projectiles.begin();
  while (curW != m_Projectiles.end())
  {
    //test for any dead projectiles and remove them if necessary
    if (!(*curW)->isDead())
    {
      (*curW)->Update();

      ++curW;
    }
    else
    {    
      delete *curW;

      curW = m_Projectiles.erase(curW);
    }   
  }
  
  //update the bots
  bool bSpawnPossible = true;
  
  std::list<Raven_Bot*>::iterator curBot = m_Bots.begin();
  for (curBot; curBot != m_Bots.end(); ++curBot)
  {
    //if this bot's status is 'respawning' attempt to resurrect it from
    //an unoccupied spawn point
    if ((*curBot)->isSpawning() && bSpawnPossible)
    {
      bSpawnPossible = AttemptToAddBot(*curBot);
    }
    
    //if this bot's status is 'dead' add a grave at its current location 
    //then change its status to 'respawning'
    else if ((*curBot)->isDead())
    {
        if ((*curBot)->isRambo())
        {
            LearningBot* rambo = (LearningBot*)(*curBot);
            double timeAlive = (std::clock() - rambo->GetTimerTime()) / (double)CLOCKS_PER_SEC;
            debug_con << "Rambo: Mission failed! My score was: " << rambo->GetRamboScore() << "";
            debug_con << "Rambo: I was on the battlefield for " << timeAlive << " seconds" << "";
            rambo->ResetRamboScore();
            rambo->ResetRamboTimer();
        }

      //create a grave
      m_pGraveMarkers->AddGrave((*curBot)->Pos());

      //change its status to spawning
      (*curBot)->SetSpawning();
    }

    //if this bot is alive update it.
    else if ( (*curBot)->isAlive())
    {
      (*curBot)->Update();

      if ((*curBot)->isPossessed() && !m_hasReadDataFile && m_CanPopulateRamboData)
      {
          (*curBot)->m_vecObservation.clear();
          (*curBot)->m_vecTarget.clear();

          if ((*curBot)->GetTargetSys()->GetTarget())
          {
              float data_distance = ((*curBot)->Pos().Distance((*curBot)->GetTargetSys()->GetTarget()->Pos()));
              bool data_isTargetInFov = (*curBot)->GetTargetSys()->isTargetWithinFOV();
              int data_remainingAmmo = (*curBot)->GetWeaponSys()->GetAmmoRemainingForWeapon((*curBot)->GetWeaponSys()->GetCurrentWeapon()->GetType());
              int data_weaponType = ((*curBot)->GetWeaponSys())->GetCurrentWeapon()->GetType();
              int data_health = ((*curBot)->Health());

              bool data_hasShoot = (*curBot)->GetHasShoot() ? 1 : 0;

              if (!m_dataOutfile.is_open() && m_dataCount < 200)
              {
                  m_dataOutfile.open("data.rambo", ios::binary | ios::out);
              }

              //on crée un échantillon de 200 observations. Juste assez pour ne pas s'accaparer de la mémoire...
              if (m_dataCount < m_totalDataCount && m_dataOutfile.is_open()) {

                  string data_distanceString = std::to_string(data_distance);
                  string data_remainingAmmoString = std::to_string(data_remainingAmmo);
                  string data_weaponTypeString = std::to_string(data_weaponType);
                  string data_healthString = std::to_string(data_health);

                  //ajouter une observation au data file
                  m_dataOutfile.write((char*)&data_distanceString, sizeof(string)); // sizeof can take a type
                  m_dataOutfile.write((char*)&data_isTargetInFov, sizeof(bool));
                  m_dataOutfile.write((char*)&data_remainingAmmoString, sizeof(string));
                  m_dataOutfile.write((char*)&data_weaponTypeString, sizeof(string));
                  m_dataOutfile.write((char*)&data_healthString, sizeof(string));
                  m_dataOutfile.write((char*)&data_hasShoot, sizeof(bool));
                  
                  m_dataCount++;
                  debug_con << "la taille du training set" << m_dataCount << "";
                  if (m_dataCount == m_totalDataCount)
                  {
                      m_dataOutfile.close();
                  }
              }
          }
      }
    }
    (*curBot)->SetHasShoot(false);
  } 

  if (m_estEntraine && !m_isRamboSpawned)
  {
      AddRambo();
      m_isRamboSpawned = true;

     // m_dataOutfile.open("modele.rambo");
     // boost::archive::text_oarchive oa(m_dataOutfile);
      //oa << m_ModeleApprentissage;
      //m_dataOutfile.write((char*)&m_ModeleApprentissage, sizeof(m_ModeleApprentissage)); // sizeof can take a type
      //m_dataOutfile.close();
  }

  //update the triggers
  m_pMap->UpdateTriggerSystem(m_Bots);

  //if the user has requested that the number of bots be decreased, remove
  //one
  if (m_bRemoveABot)
  { 
      bool removeLastBotAdded = true;
      if (m_pSelectedBot)
      {
          if (m_pSelectedBot->isLeader()) // If we have a leader bot selected, delete this bot.
          {
              removeLastBotAdded = false;

              if (!m_Bots.empty())
              {
                  Raven_Bot* pBot = nullptr;
                  int botHeadColor = -1;
                  for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
                      if ((*it)->isLeader())
                      {
                          if ((*it)->ID() == m_pSelectedBot->ID()) // Leader bot to be deleted
                          {
                              pBot = *it;
                              break;
                          }
                      }
                  }
                  botHeadColor = pBot->GetHeadColor();
                  if (pBot)
                  {
                      if (pBot == m_pSelectedBot)m_pSelectedBot = 0;
                      NotifyAllBotsOfRemoval(pBot);

                      // kill all teammate
                      while (pBot->GetTeammatesIDs().size() != 0)
                      {
                          Raven_Bot* teammateBot = nullptr;
                          bool foundTeammate = false;
                          for (int i = 0; i < pBot->GetTeammatesIDs().size(); i++)
                          {
                              for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
                                  if (!(*it)->isLeader() && (*it)->ID() == pBot->GetTeammatesIDs()[i])
                                  {
                                      teammateBot = *it;
                                      foundTeammate = true;
                                      break;
                                  }
                              }
                              if (foundTeammate)
                                  break;
                          }

                          if (teammateBot)
                          {
                              if (teammateBot == m_pSelectedBot)m_pSelectedBot = 0;
                              NotifyAllBotsOfRemoval(teammateBot);

                              // Remove teammate from leader
                              pBot->RemoveTeammate(teammateBot->ID());

                              for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                                  if ((*it) == teammateBot)
                                  {
                                      delete (*it);
                                      break;
                                  }
                              }
                              m_Bots.remove(teammateBot);
                              teammateBot = 0;
                          }
                      }

                      for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                          if ((*it) == pBot)
                          {
                              delete (*it);
                              break;
                          }
                      }
                      AddBotHeadColorBack(botHeadColor);
                      m_Bots.remove(pBot);
                      pBot = 0;
                  }
              }
          }
      }

      if (removeLastBotAdded) // If no leader bot were selected, the last leader bot added will be removed.
      {
          if (!m_Bots.empty())
          {
              Raven_Bot* pBot = nullptr;
              int botHeadColor = -1;
              for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
                  if ((*it)->isLeader())
                  {
                      pBot = *it;
                      break;
                  }
              }
              botHeadColor = pBot->GetHeadColor();
              if (pBot)
              {
                  if (pBot == m_pSelectedBot)m_pSelectedBot = 0;
                  NotifyAllBotsOfRemoval(pBot);

                  // kill all teammate
                  while (pBot->GetTeammatesIDs().size() != 0)
                  {
                      Raven_Bot* teammateBot = nullptr;
                      bool foundTeammate = false;
                      for (int i = 0; i < pBot->GetTeammatesIDs().size(); i++)
                      {
                          for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
                              if (!(*it)->isLeader() && (*it)->ID() == pBot->GetTeammatesIDs()[i])
                              {
                                  teammateBot = *it;
                                  foundTeammate = true;
                                  break;
                              }
                          }
                          if (foundTeammate)
                              break;
                      }

                      if (teammateBot)
                      {
                          if (teammateBot == m_pSelectedBot)m_pSelectedBot = 0;
                          NotifyAllBotsOfRemoval(teammateBot);

                          // Remove teammate from leader
                          pBot->RemoveTeammate(teammateBot->ID());

                          for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                              if ((*it) == teammateBot)
                              {
                                  delete (*it);
                                  break;
                              }
                          }
                          m_Bots.remove(teammateBot);
                          teammateBot = 0;
                      }
                  }

                  for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                      if ((*it) == pBot)
                      {
                          delete (*it);
                          break;
                      }
                  }
                  AddBotHeadColorBack(botHeadColor);
                  m_Bots.remove(pBot);
                  pBot = 0;
              }
          }
      }

        m_bRemoveABot = false;
  }

  if (m_bRemoveATeammateBot)
  {
      bool removeLastBotAdded = true;
      if (m_pSelectedBot)
      {
          if (m_pSelectedBot->isLeader()) // If we have a leader bot selected, we remove the last teammate bot added for him.
          {
              removeLastBotAdded = false;

              if (!m_Bots.empty())
              {
                  Raven_Bot* pBot = nullptr;
                  for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
                      if (!(*it)->isLeader() && !(*it)->isRambo())
                      {
                          if (((Raven_Teammate*)(*it))->GetLeader()->ID() == m_pSelectedBot->ID()) // Delete only if he is a teammate of the selected bot.
                          {
                              pBot = *it;
                              break;
                          }
                      }
                  }

                  if (pBot)
                  {
                      if (pBot == m_pSelectedBot)m_pSelectedBot = 0;
                      NotifyAllBotsOfRemoval(pBot);

                      // Remove teammate from leader
                      Raven_Teammate* tBot = (Raven_Teammate*)pBot;
                      Raven_Bot* leader = tBot->GetLeader();
                      leader->RemoveTeammate(pBot->ID());

                      for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                          if ((*it) == pBot)
                          {
                              delete (*it);
                              break;
                          }
                      }
                      m_Bots.remove(pBot);
                      pBot = 0;
                  }
              }
          }
      }
      if (removeLastBotAdded) // If no leader bot is selected, we remove the last teammate bot added for any bot.
      {
          if (!m_Bots.empty())
          {
              Raven_Bot* pBot = nullptr;
              for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
                  if (!(*it)->isLeader() && !(*it)->isRambo())
                  {
                      pBot = *it;
                      break;
                  }
              }

              if (pBot)
              {
                  if (pBot == m_pSelectedBot)m_pSelectedBot = 0;
                  NotifyAllBotsOfRemoval(pBot);

                  // Remove teammate from leader
                  Raven_Teammate* tBot = (Raven_Teammate*)pBot;
                  Raven_Bot* leader = tBot->GetLeader();
                  leader->RemoveTeammate(pBot->ID());

                  for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                      if ((*it) == pBot)
                      {
                          delete (*it);
                          break;
                      }
                  }
                  m_Bots.remove(pBot);
                  pBot = 0;
              }
          }
      }

      m_bRemoveATeammateBot = false;
  }

  if (m_bRemoveRambo)
  {
      if (!m_Bots.empty())
      {
          Raven_Bot* pBot = nullptr;
          for (std::list<Raven_Bot*>::reverse_iterator it = m_Bots.rbegin(); it != m_Bots.rend(); ++it) {
              if ((*it)->isRambo())
              {
                  pBot = *it;
                  break;
              }
          }
          if (pBot)
          {
              if (pBot == m_pSelectedBot)m_pSelectedBot = 0;
              NotifyAllBotsOfRemoval(pBot);

              for (std::list<Raven_Bot*>::iterator it = m_Bots.begin(); it != m_Bots.end(); ++it) {
                  if ((*it) == pBot)
                  {
                      delete (*it);
                      break;
                  }
              }
              m_Bots.remove(pBot);
              pBot = 0;
          }
      }
  }

  if ((m_dataCount >= m_totalDataCount && !m_hasReadDataFile) || (m_loadRamboData && !m_hasReadDataFile))
  {
      if (m_loadRamboData)
      {
          m_dataCount = m_totalDataCount;
      }

      m_hasReadDataFile = true;
      m_dataInfile.open("data.rambo", ios::binary | ios::in);
      
      if (m_dataInfile.is_open())
      {
         

          while (m_dataReadcount < m_dataCount)
          {
              m_dataInfile.read((char*)&data_distanceString, sizeof(string));
              m_dataInfile.read((char*)&data_isTargetInFov, sizeof(bool));
              m_dataInfile.read((char*)&data_remainingAmmoString, sizeof(string));
              m_dataInfile.read((char*)&data_weaponTypeString, sizeof(string));
              m_dataInfile.read((char*)&data_healthString, sizeof(string));
              m_dataInfile.read((char*)&data_hasShoot, sizeof(bool));

              data_distance = std::stof(data_distanceString);
              data_remainingAmmo = std::stoi(data_remainingAmmoString);
              data_weaponType = std::stoi(data_weaponTypeString);
              data_health = std::stoi(data_healthString);

              std::vector<double> m_vecObservation; //distance-target, visibilite, quantite-arme, type arme, son niveau de vie
              std::vector<double> m_vecTarget; //classe representer sous d'un vecteur de sortie. 

              m_vecObservation.push_back(data_distance);
              m_vecObservation.push_back(data_isTargetInFov);
              m_vecObservation.push_back(data_remainingAmmo);
              m_vecObservation.push_back(data_weaponType);
              m_vecObservation.push_back(data_health);

              m_vecTarget.push_back(data_hasShoot);

              AddData(m_vecObservation, m_vecTarget);
              m_dataReadcount++; 
          }
          m_dataInfile.close();
      }
  }

  //Lancer l'apprentissage quand le jeu de données est suffisant
  //la fonction d'apprentissage s'effectue en parallèle : thread

  if ((m_TrainingSet.GetInputSet().size() >= m_totalDataCount) & (!m_LancerApprentissage)) {

      debug_con << "On passe par la" << "";

      std::thread t1(&Raven_Game::TrainThread, this);
      t1.detach();
  }
}

//----------------------------- AddBotHeadColorBack -------------------------------
//-----------------------------------------------------------------------------
void Raven_Game::AddBotHeadColorBack(int botHeadColor)
{
    if (HeadColor(botHeadColor) != HeadColor::white)
    {
        m_pHeadColorList.pop_back();
        m_pHeadColorList.push_back(HeadColor(botHeadColor));
        m_pHeadColorList.push_back(HeadColor::white);
    }
}


//----------------------------- AttemptToAddBot -------------------------------
//-----------------------------------------------------------------------------
bool Raven_Game::AttemptToAddBot(Raven_Bot* pBot)
{
  //make sure there are some spawn points available
  if (m_pMap->GetSpawnPoints().size() <= 0)
  {
    ErrorBox("Map has no spawn points!"); return false;
  }

  //we'll make the same number of attempts to spawn a bot this update as
  //there are spawn points
  int attempts = m_pMap->GetSpawnPoints().size();

  while (--attempts >= 0)
  { 
    //select a random spawn point
    Vector2D pos = m_pMap->GetRandomSpawnPoint();

    //check to see if it's occupied
    std::list<Raven_Bot*>::const_iterator curBot = m_Bots.begin();

    bool bAvailable = true;

    for (curBot; curBot != m_Bots.end(); ++curBot)
    {
      //if the spawn point is unoccupied spawn a bot
      if (Vec2DDistance(pos, (*curBot)->Pos()) < (*curBot)->BRadius())
      {
        bAvailable = false;
      }
    }

    if (bAvailable)
    {  
      pBot->Spawn(pos);

      return true;   
    }
  }

  return false;
}

//-------------------------- AddBots --------------------------------------
//
//  Adds a bot and switches on the default steering behavior
//-----------------------------------------------------------------------------
void Raven_Game::AddBots(unsigned int NumBotsToAdd)
{ 
  while (NumBotsToAdd--)
  {
    //create a bot. (its position is irrelevant at this point because it will
    //not be rendered until it is spawned)
    Raven_Bot* rb = new Raven_Bot(this, Vector2D(), script->GetDouble("Leader_Scale"), m_pHeadColorList.front());
    /*if (m_pHeadColorList.size() > 1 && m_pHeadColorList.front() == HeadColor::hollow)
    {
        m_pHeadColorList.pop_front();
        rb = new Raven_Bot(this, Vector2D(), script->GetDouble("Leader_Scale"), m_pHeadColorList.front());
    }
    else
    {
        rb = new Raven_Bot(this, Vector2D(), script->GetDouble("Leader_Scale"), m_pHeadColorList.front());
    }*/
    if (m_pHeadColorList.size() > 1)
    {
        m_pHeadColorList.pop_front();
    }

    //switch the default steering behaviors on
    rb->GetSteering()->WallAvoidanceOn();
    rb->GetSteering()->SeparationOn();

    m_Bots.push_back(rb);

    //register the bot with the entity manager
    EntityMgr->RegisterEntity(rb);

    
#ifdef LOG_CREATIONAL_STUFF
  debug_con << "Adding bot with ID " << ttos(rb->ID()) << "";
#endif
  }
}

//-------------------------- AddTeammates --------------------------------------
//
//  Adds a teammate and switches on the default steering behavior
//-----------------------------------------------------------------------------
void Raven_Game::AddTeammates(unsigned int NumTeammatesToAdd)
{
    if (m_pSelectedBot)
    {
        while (NumTeammatesToAdd--)
        {
            //create a teammate. (its position is irrelevant at this point because it will
            //not be rendered until it is spawned)
            Raven_Bot* rb = new Raven_Teammate(this, Vector2D(), m_pSelectedBot);

            m_pSelectedBot->AddTeammate(rb->ID());

            //switch the default steering behaviors on
            rb->GetSteering()->WallAvoidanceOn();
            rb->GetSteering()->SeparationOn();

            m_Bots.push_back(rb);

            //register the bot with the entity manager
            EntityMgr->RegisterEntity(rb);


#ifdef LOG_CREATIONAL_STUFF
            debug_con << "Adding teammate with ID " << ttos(rb->ID()) << "";
#endif
        }
    }
    else
    {
        debug_con << "Failed to add teammate. You need to have selected a leader first." << "";
    }
}

void Raven_Game::AddRambo()
{
    Raven_Bot* rb = new LearningBot(this, Vector2D());
    debug_con << "Instanciation d'un bot apprenant" << rb->ID() << "";

    //switch the default steering behaviors on
    rb->GetSteering()->WallAvoidanceOn();
    rb->GetSteering()->SeparationOn();

    m_Bots.push_back(rb);

    //register the bot with the entity manager
    EntityMgr->RegisterEntity(rb);

#ifdef LOG_CREATIONAL_STUFF
    debug_con << "Adding bot with ID " << ttos(rb->ID()) << "";
#endif
}

//---------------------------- NotifyAllBotsOfRemoval -------------------------
//
//  when a bot is removed from the game by a user all remianing bots
//  must be notifies so that they can remove any references to that bot from
//  their memory
//-----------------------------------------------------------------------------
void Raven_Game::NotifyAllBotsOfRemoval(Raven_Bot* pRemovedBot)const
{
    std::list<Raven_Bot*>::const_iterator curBot = m_Bots.begin();
    for (curBot; curBot != m_Bots.end(); ++curBot)
    {
      Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                              SENDER_ID_IRRELEVANT,
                              (*curBot)->ID(),
                              Msg_UserHasRemovedBot,
                              pRemovedBot);

    }
}

//ajout à chaque update d'un bot des données sur son cmportement
bool Raven_Game::AddData(vector<double>& data, vector<double>& targets)
{
    if (data.size() > 0 && targets.size() > 0) {

        if (m_TrainingSet.GetInputNb() <= 0)
            m_TrainingSet = CData(data.size(), targets.size());

        if (data.size() == m_TrainingSet.GetInputNb() && targets.size() == m_TrainingSet.GetTargetsNb()) {

            m_TrainingSet.AddData(data, targets);
            return true;
        }
    }

    return false;

}

//-------------------------------RemoveBot ------------------------------------
//
//  removes the last bot to be added from the game
//-----------------------------------------------------------------------------
void Raven_Game::RemoveBot()
{
    m_bRemoveABot = true;
}

//-------------------------------RemoveTeammate ------------------------------------
//
//  removes the last teammate to be added from the game
//-----------------------------------------------------------------------------
void Raven_Game::RemoveTeammate()
{
    m_bRemoveATeammateBot = true;
}

//-------------------------------RemoveRambo ------------------------------------
//
//  removes rambo from the game
//-----------------------------------------------------------------------------
void Raven_Game::RemoveRambo()
{
    m_bRemoveRambo = true;
}

//--------------------------- AddBolt -----------------------------------------
//-----------------------------------------------------------------------------
void Raven_Game::AddBolt(Raven_Bot* shooter, Vector2D target)
{
  Raven_Projectile* rp = new Bolt(shooter, target);

  m_Projectiles.push_back(rp);
  
  #ifdef LOG_CREATIONAL_STUFF
  debug_con << "Adding a bolt " << rp->ID() << " at pos " << rp->Pos() << "";
  #endif
}

//------------------------------ AddRocket --------------------------------
void Raven_Game::AddRocket(Raven_Bot* shooter, Vector2D target)
{
  Raven_Projectile* rp = new Rocket(shooter, target);

  m_Projectiles.push_back(rp);
  
  #ifdef LOG_CREATIONAL_STUFF
  debug_con << "Adding a rocket " << rp->ID() << " at pos " << rp->Pos() << "";
  #endif
}

//------------------------- AddRailGunSlug -----------------------------------
void Raven_Game::AddRailGunSlug(Raven_Bot* shooter, Vector2D target)
{
  Raven_Projectile* rp = new Slug(shooter, target);

  m_Projectiles.push_back(rp);
  
  #ifdef LOG_CREATIONAL_STUFF
  debug_con << "Adding a rail gun slug" << rp->ID() << " at pos " << rp->Pos() << "";
#endif
}

//------------------------- AddShotGunPellet -----------------------------------
void Raven_Game::AddShotGunPellet(Raven_Bot* shooter, Vector2D target)
{
  Raven_Projectile* rp = new Pellet(shooter, target);

  m_Projectiles.push_back(rp);
  
  #ifdef LOG_CREATIONAL_STUFF
  debug_con << "Adding a shotgun shell " << rp->ID() << " at pos " << rp->Pos() << "";
#endif
}


//----------------------------- GetBotAtPosition ------------------------------
//
//  given a position on the map this method returns the bot found with its
//  bounding radius of that position.
//  If there is no bot at the position the method returns NULL
//-----------------------------------------------------------------------------
Raven_Bot* Raven_Game::GetBotAtPosition(Vector2D CursorPos)const
{
  std::list<Raven_Bot*>::const_iterator curBot = m_Bots.begin();

  for (curBot; curBot != m_Bots.end(); ++curBot)
  {
    if (Vec2DDistance((*curBot)->Pos(), CursorPos) < (*curBot)->BRadius())
    {
      if ((*curBot)->isAlive())
      {
        return *curBot;
      }
    }
  }

  return NULL;
}

//-------------------------------- LoadMap ------------------------------------
//
//  sets up the game environment from map file
//-----------------------------------------------------------------------------
bool Raven_Game::LoadMap(const std::string& filename)
{  
  //clear any current bots and projectiles
  Clear();
  
  //out with the old
  delete m_pMap;
  delete m_pGraveMarkers;
  delete m_pPathManager;

  //in with the new
  m_pGraveMarkers = new GraveMarkers(script->GetDouble("GraveLifetime"));
  m_pPathManager = new PathManager<Raven_PathPlanner>(script->GetInt("MaxSearchCyclesPerUpdateStep"));
  m_pMap = new Raven_Map();

  //make sure the entity manager is reset
  EntityMgr->Reset();


  //load the new map data
  if (m_pMap->LoadMap(filename))
  { 
    AddBots(script->GetInt("NumBots"));
  
    return true;
  }

  return false;
}


//------------------------- ExorciseAnyPossessedBot ---------------------------
//
//  when called will release any possessed bot from user control
//-----------------------------------------------------------------------------
void Raven_Game::ExorciseAnyPossessedBot()
{
    if (m_pSelectedBot)
    {
        if (m_pSelectedBot->isPossessed())
        {
            m_pSelectedBot->Exorcise();
        }
        else
        {
            m_pSelectedBot = 0;
        }
    }
}

void Raven_Game::KillSelectedBot()
{
    if (m_pSelectedBot)
    {
        int headShot = 100;
        Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
            m_pSelectedBot->ID(),
            m_pSelectedBot->ID(),
            Msg_TakeThatMF,
            (void*)&headShot);
    }
}

//-------------------------- ClickRightMouseButton -----------------------------
//
//  this method is called when the user clicks the right mouse button.
//
//  the method checks to see if a bot is beneath the cursor. If so, the bot
//  is recorded as selected.
//
//  if the cursor is not over a bot then any selected bot/s will attempt to
//  move to that position.
//-----------------------------------------------------------------------------
void Raven_Game::ClickRightMouseButton(POINTS p)
{
  Raven_Bot* pBot = GetBotAtPosition(POINTStoVector(p));

  //if there is no selected bot just return;
  if (!pBot && m_pSelectedBot == NULL) return;

  //if the cursor is over a different bot to the existing selection,
  //change selection
  if (pBot && pBot != m_pSelectedBot)
  { 
    if (m_pSelectedBot) m_pSelectedBot->Exorcise();
    m_pSelectedBot = pBot;

    return;
  }

  //if the user clicks on a selected bot twice it becomes possessed(under
  //the player's control)
  if (pBot && pBot == m_pSelectedBot && pBot->isLeader())
  {
    m_pSelectedBot->TakePossession();

    //clear any current goals
    m_pSelectedBot->GetBrain()->RemoveAllSubgoals();
  }

  //if the bot is possessed then a right click moves the bot to the cursor
  //position
  if (m_pSelectedBot->isPossessed())
  {
    //if the shift key is pressed down at the same time as clicking then the
    //movement command will be queued
    if (IS_KEY_PRESSED('Q'))
    {
      m_pSelectedBot->GetBrain()->QueueGoal_MoveToPosition(POINTStoVector(p));
    }
    else
    {
      //clear any current goals
      m_pSelectedBot->GetBrain()->RemoveAllSubgoals();

      m_pSelectedBot->GetBrain()->AddGoal_MoveToPosition(POINTStoVector(p));
    }
  }
}

//---------------------- ClickLeftMouseButton ---------------------------------
//-----------------------------------------------------------------------------
void Raven_Game::ClickLeftMouseButton(POINTS p)
{
  if (m_pSelectedBot && m_pSelectedBot->isPossessed())
  {
      // Get the bot at this position
      Raven_Bot* pBot = GetBotAtPosition(POINTStoVector(p));

      //if there is a bot, we might send the signal to our teammate
      if (pBot)
      {
          // Check if the bot is not already the target of our leader or ourselves
          if (m_pSelectedBot->GetTargetBot() != pBot && pBot != m_pSelectedBot)
          {
                std::vector<int> teammatesIDs = m_pSelectedBot->GetTeammatesIDs();
                // Check if pBot is not a teammate
                if (!(std::find(teammatesIDs.begin(), teammatesIDs.end(), pBot->ID()) != teammatesIDs.end()))
                {
                    m_pSelectedBot->GetTargetSys()->SetTarget(pBot);
                    MemorySlice* slice = m_pSelectedBot->GetSensoryMem()->GetMemorySliceOfOpponent(pBot);
                    // Send the memory slice to our teammate
                    for (size_t i = 0; i < teammatesIDs.size(); i++)
                    {
                        Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                            m_pSelectedBot->ID(),
                            teammatesIDs[i],
                            Msg_YoTeamINeedHelp,
                            (void*)slice);
                    }
                } 
          }
      }
      else
      {
          m_pSelectedBot->GetTargetSys()->SetTarget(nullptr);
      }
      m_pSelectedBot->FireWeapon(POINTStoVector(p));
      m_pSelectedBot->SetHasShoot(true);
  }
}

//------------------------ GetPlayerInput -------------------------------------
//
//  if a bot is possessed the keyboard is polled for user input and any 
//  relevant bot methods are called appropriately
//-----------------------------------------------------------------------------
void Raven_Game::GetPlayerInput()const
{
  if (m_pSelectedBot && m_pSelectedBot->isPossessed())
  {
      m_pSelectedBot->RotateFacingTowardPosition(GetClientCursorPosition());
   }
}


//-------------------- ChangeWeaponOfPossessedBot -----------------------------
//
//  changes the weapon of the possessed bot
//-----------------------------------------------------------------------------
void Raven_Game::ChangeWeaponOfPossessedBot(unsigned int weapon)const
{
  //ensure one of the bots has been possessed
  if (m_pSelectedBot)
  {
    switch(weapon)
    {
    case type_blaster:
      
      PossessedBot()->ChangeWeapon(type_blaster); return;

    case type_shotgun:
      
      PossessedBot()->ChangeWeapon(type_shotgun); return;

    case type_rocket_launcher:
      
      PossessedBot()->ChangeWeapon(type_rocket_launcher); return;

    case type_rail_gun:
      
      PossessedBot()->ChangeWeapon(type_rail_gun); return;

    }
  }
}

//---------------------------- isLOSOkay --------------------------------------
//
//  returns true if the ray between A and B is unobstructed.
//------------------------------------------------------------------------------
bool Raven_Game::isLOSOkay(Vector2D A, Vector2D B)const
{
  return !doWallsObstructLineSegment(A, B, m_pMap->GetWalls());
}

//------------------------- isPathObstructed ----------------------------------
//
//  returns true if a bot cannot move from A to B without bumping into 
//  world geometry. It achieves this by stepping from A to B in steps of
//  size BoundingRadius and testing for intersection with world geometry at
//  each point.
//-----------------------------------------------------------------------------
bool Raven_Game::isPathObstructed(Vector2D A,
                                  Vector2D B,
                                  double    BoundingRadius)const
{
  Vector2D ToB = Vec2DNormalize(B-A);

  Vector2D curPos = A;

  while (Vec2DDistanceSq(curPos, B) > BoundingRadius*BoundingRadius)
  {   
    //advance curPos one step
    curPos += ToB * 0.5 * BoundingRadius;
    
    //test all walls against the new position
    if (doWallsIntersectCircle(m_pMap->GetWalls(), curPos, BoundingRadius))
    {
      return true;
    }
  }

  return false;
}


//----------------------------- GetAllBotsInFOV ------------------------------
//
//  returns a vector of pointers to bots within the given bot's field of view
//-----------------------------------------------------------------------------
std::vector<Raven_Bot*>
Raven_Game::GetAllBotsInFOV(const Raven_Bot* pBot)const
{
  std::vector<Raven_Bot*> VisibleBots;

  std::list<Raven_Bot*>::const_iterator curBot = m_Bots.begin();
  for (curBot; curBot != m_Bots.end(); ++curBot)
  {
    //make sure time is not wasted checking against the same bot or against a
    // bot that is dead or re-spawning
    if (*curBot == pBot ||  !(*curBot)->isAlive()) continue;

    //first of all test to see if this bot is within the FOV
    if (isSecondInFOVOfFirst(pBot->Pos(),
                             pBot->Facing(),
                             (*curBot)->Pos(),
                             pBot->FieldOfView()))
    {
      //cast a ray from between the bots to test visibility. If the bot is
      //visible add it to the vector
      if (!doWallsObstructLineSegment(pBot->Pos(),
                              (*curBot)->Pos(),
                              m_pMap->GetWalls()))
      {
        VisibleBots.push_back(*curBot);
      }
    }
  }

  return VisibleBots;
}

//---------------------------- isSecondVisibleToFirst -------------------------

bool
Raven_Game::isSecondVisibleToFirst(const Raven_Bot* pFirst,
                                   const Raven_Bot* pSecond)const
{
  //if the two bots are equal or if one of them is not alive return false
  if ( !(pFirst == pSecond) && pSecond->isAlive())
  {
    //first of all test to see if this bot is within the FOV
    if (isSecondInFOVOfFirst(pFirst->Pos(),
                             pFirst->Facing(),
                             pSecond->Pos(),
                             pFirst->FieldOfView()))
    {
      //test the line segment connecting the bot's positions against the walls.
      //If the bot is visible add it to the vector
      if (!doWallsObstructLineSegment(pFirst->Pos(),
                                      pSecond->Pos(),
                                      m_pMap->GetWalls()))
      {
        return true;
      }
    }
  }

  return false;
}

//--------------------- GetPosOfClosestSwitch -----------------------------
//
//  returns the position of the closest visible switch that triggers the
//  door of the specified ID
//-----------------------------------------------------------------------------
Vector2D 
Raven_Game::GetPosOfClosestSwitch(Vector2D botPos, unsigned int doorID)const
{
  std::vector<unsigned int> SwitchIDs;
  
  //first we need to get the ids of the switches attached to this door
  std::vector<Raven_Door*>::const_iterator curDoor;
  for (curDoor = m_pMap->GetDoors().begin();
       curDoor != m_pMap->GetDoors().end();
       ++curDoor)
  {
    if ((*curDoor)->ID() == doorID)
    {
       SwitchIDs = (*curDoor)->GetSwitchIDs(); break;
    }
  }

  Vector2D closest;
  double ClosestDist = MaxDouble;
  
  //now test to see which one is closest and visible
  std::vector<unsigned int>::iterator it;
  for (it = SwitchIDs.begin(); it != SwitchIDs.end(); ++it)
  {
    BaseGameEntity* trig = EntityMgr->GetEntityFromID(*it);

    if (isLOSOkay(botPos, trig->Pos()))
    {
      double dist = Vec2DDistanceSq(botPos, trig->Pos());

      if ( dist < ClosestDist)
      {
        ClosestDist = dist;
        closest = trig->Pos();
      }
    }
  }

  return closest;
}




    
//--------------------------- Render ------------------------------------------
//-----------------------------------------------------------------------------
void Raven_Game::Render()
{
  m_pGraveMarkers->Render();
  
  //render the map
  m_pMap->Render();

  //render all the bots unless the user has selected the option to only 
  //render those bots that are in the fov of the selected bot
  if (m_pSelectedBot && UserOptions->m_bOnlyShowBotsInTargetsFOV)
  {
    std::vector<Raven_Bot*> 
    VisibleBots = GetAllBotsInFOV(m_pSelectedBot);

    std::vector<Raven_Bot*>::const_iterator it = VisibleBots.begin();
    for (it; it != VisibleBots.end(); ++it) (*it)->Render();

    if (m_pSelectedBot) m_pSelectedBot->Render();
  }

  else
  {
    //render all the entities
    std::list<Raven_Bot*>::const_iterator curBot = m_Bots.begin();
    for (curBot; curBot != m_Bots.end(); ++curBot)
    {
      if ((*curBot)->isAlive())
      {
        (*curBot)->Render();
      }
    }
  }
  
  //render any projectiles
  std::list<Raven_Projectile*>::const_iterator curW = m_Projectiles.begin();
  for (curW; curW != m_Projectiles.end(); ++curW)
  {
    (*curW)->Render();
  }

 // gdi->TextAtPos(300, WindowHeight - 70, "Num Current Searches: " + ttos(m_pPathManager->GetNumActiveSearches()));

  //render a red circle around the selected bot (blue if possessed)
  if (m_pSelectedBot)
  {
    if (m_pSelectedBot->isPossessed())
    {
      gdi->BluePen(); gdi->HollowBrush();
      gdi->Circle(m_pSelectedBot->Pos(), m_pSelectedBot->BRadius()+1);
    }
    else
    {
      gdi->RedPen(); gdi->HollowBrush();
      gdi->Circle(m_pSelectedBot->Pos(), m_pSelectedBot->BRadius()+1);
    }


    if (UserOptions->m_bShowOpponentsSensedBySelectedBot)
    {
      m_pSelectedBot->GetSensoryMem()->RenderBoxesAroundRecentlySensed();
    }

    //render a square around the bot's target
    if (UserOptions->m_bShowTargetOfSelectedBot && m_pSelectedBot->GetTargetBot())
    {  
      
      gdi->ThickRedPen();

      Vector2D p = m_pSelectedBot->GetTargetBot()->Pos();
      double   b = m_pSelectedBot->GetTargetBot()->BRadius();
      
      gdi->Line(p.x-b, p.y-b, p.x+b, p.y-b);
      gdi->Line(p.x+b, p.y-b, p.x+b, p.y+b);
      gdi->Line(p.x+b, p.y+b, p.x-b, p.y+b);
      gdi->Line(p.x-b, p.y+b, p.x-b, p.y-b);
    }



    //render the path of the bot
    if (UserOptions->m_bShowPathOfSelectedBot)
    {
      m_pSelectedBot->GetBrain()->Render();
    }  
    
    //display the bot's goal stack
    if (UserOptions->m_bShowGoalsOfSelectedBot)
    {
      Vector2D p(m_pSelectedBot->Pos().x -50, m_pSelectedBot->Pos().y);

      m_pSelectedBot->GetBrain()->RenderAtPos(p, GoalTypeToString::Instance());
    }

    if (UserOptions->m_bShowGoalAppraisals)
    {
      m_pSelectedBot->GetBrain()->RenderEvaluations(5, 415);
    } 
    
    if (UserOptions->m_bShowWeaponAppraisals)
    {
      m_pSelectedBot->GetWeaponSys()->RenderDesirabilities();
    }

   if (IS_KEY_PRESSED('Q') && m_pSelectedBot->isPossessed())
    {
      gdi->TextColor(255,0,0);
      gdi->TextAtPos(GetClientCursorPosition(), "Queuing");
    }
  }
}
