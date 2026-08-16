#pragma once
#include <filesystem>
#include <array>

#include "GameObject.h"

namespace Game {

	// Für dieses Projekt sollten 65536 GameObjects ausreichen
	constexpr size_t maxGameObjects = 65536;

	class GameLevel {
	public:
		GameLevel();
		~GameLevel();

		uint16_t addGameObject(GameObject gameObject);
		void removeGameObject(uint16_t index);

		void loadLevel(std::filesystem::path levelPath);
		void saveLevel(std::filesystem::path levelPath);

		void optimizeLevel();

		void setScore(uint32_t score) { m_Score = score; };
		void setPlayerLives(uint32_t playerLives) { m_PlayerLives = playerLives; };

		uint32_t getScore() { return m_Score; };
		uint32_t getPlayerLives() { return m_PlayerLives; };

		std::array<GameObject, maxGameObjects>& getGameObjects() { return m_GameObjects; }
		uint16_t getGameObjectCount() { return m_ObjectCount; };
	private:

		uint32_t m_PlayerLives = 3;
		uint32_t m_Score = 0;

		// Slot basiert, damit man die GameObjects einfach per Index ansprechen kann.
		std::array<GameObject, maxGameObjects> m_GameObjects;

		std::vector<uint16_t> m_FreeSlots;
		uint16_t m_ObjectCount = 0;
		uint16_t m_LastFreeIndex = 0;
	};


}