#pragma once
#include <filesystem>
#include <vector>

#include "GameObject.h"

namespace Game {

	class GameLevel {
	public:
		GameLevel();
		~GameLevel();

		ObjectID addGameObject(GameObject gameObject);
		void removeGameObject(ObjectID id);

		void loadLevel(std::filesystem::path levelPath);
		void saveLevel(std::filesystem::path levelPath, bool saveFile = false);

		void setScore(uint32_t score) { m_Score = score; };
		void setPlayerLives(uint32_t playerLives) { m_PlayerLives = playerLives; };

		uint32_t getScore() { return m_Score; };
		uint32_t getPlayerLives() { return m_PlayerLives; };

		GameObject& getGameObject(ObjectID id);

		std::vector<GameObject>& getGameObjects() { return m_GameObjects; }

		std::vector<uint32_t> getGameObjectGroupIndices(uint32_t groupID);

		uint32_t getGameObjectCount() { return m_GameObjects.size(); };
	private:

		uint32_t m_PlayerLives = 3;
		uint32_t m_Score = 0;

		std::vector<GameObject> m_GameObjects;
	};


}