#include "GameLevel.h"
#include <fstream>

namespace Game {
	GameLevel::GameLevel()
	{
		memset(m_GameObjects.data(), 0, maxGameObjects * sizeof(GameObject));
	}

	GameLevel::~GameLevel()
	{

	}

	uint16_t GameLevel::addGameObject(GameObject gameObject)
	{
		uint16_t index = m_LastFreeIndex;
		if (!m_FreeSlots.empty()) {
			index = m_FreeSlots.back();
			m_FreeSlots.pop_back();
		}
		else m_LastFreeIndex++;
		m_GameObjects[index] = gameObject;
		m_ObjectCount++;
		return index;
	}

	void GameLevel::removeGameObject(uint16_t index)
	{
		m_FreeSlots.push_back(index);
		m_GameObjects[index].type = GameObjectType::Invalid;
		m_ObjectCount--;
	}

	void GameLevel::loadLevel(std::filesystem::path levelPath)
	{
		if (std::filesystem::exists(levelPath)) {
			std::fstream file(levelPath, std::ios::in | std::ios::binary);
			file.seekg(0, std::ios::end);
			uint32_t fileSize = file.tellg();
			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(m_GameObjects.data()), fileSize);
			m_ObjectCount = fileSize / sizeof(GameObject);
			m_LastFreeIndex = m_ObjectCount;
		}
		else {
			m_FreeSlots.clear();
			memset(m_GameObjects.data(), 0, maxGameObjects * sizeof(GameObject));
			m_ObjectCount = 0;
			m_LastFreeIndex = 0;
		}
	}

	void GameLevel::saveLevel(std::filesystem::path levelPath)
	{
		std::fstream file(levelPath, std::ios::out | std::ios::binary);
		GameObject* gameObjects = new GameObject[m_LastFreeIndex];

		uint16_t validIndex = 0;
		for (uint16_t i = 0; i < m_LastFreeIndex; i++) {
			if (m_GameObjects[i].type == GameObjectType::Invalid) continue;

			gameObjects[validIndex] = m_GameObjects[i];
			validIndex++;
		}

		file.write(reinterpret_cast<char*>(gameObjects), validIndex * sizeof(GameObject));
		delete[] gameObjects;
	}

}

