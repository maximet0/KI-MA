#include "GameLevel.h"

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
		//TODO: Levels laden / Speichern
	}

	void GameLevel::saveLevel(std::filesystem::path levelPath)
	{
		//TODO: Levels laden / Speichern
	}

}

