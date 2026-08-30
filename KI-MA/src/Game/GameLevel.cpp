#include "GameLevel.h"
#include <fstream>

#include <random>


namespace Game {
	GameLevel::GameLevel()
	{
		
	}

	GameLevel::~GameLevel()
	{

	}

	ObjectID GameLevel::addGameObject(GameObject gameObject)
	{
		static std::mt19937_64 rng(std::random_device{}());
		static std::uniform_int_distribution<ObjectID> dist(1, UINT64_MAX);

		gameObject.id = dist(rng);

		m_GameObjects.push_back(gameObject);
		return gameObject.id;
	}

	void GameLevel::removeGameObject(ObjectID id)
	{
		for (auto it = m_GameObjects.begin(); it != m_GameObjects.end(); ++it) {
			if (it->id == id) {
				m_GameObjects.erase(it);
				return;
			}
		}
	}

	char saveFileMagic[4] = { 'S', 'L', 'V', 'L' };
	char levelMagic[4] = { '_', 'L', 'V', 'L' };

	void GameLevel::loadLevel(std::filesystem::path levelPath)
	{
		m_GameObjects.clear();
		m_PlayerLives = 3;
		m_Score = 0;

		if (std::filesystem::exists(levelPath)) {
			std::ifstream file(levelPath, std::ios::in | std::ios::binary);

			char* magic = new char[4];

			bool saveFile = false;

			file.read(magic, sizeof(char) * 4);
			
			if (strncmp(magic, saveFileMagic, 4) == 0) {
				saveFile = true;
				file.read((char*)&m_Score, sizeof(m_Score));
				file.read((char*)&m_PlayerLives, sizeof(m_PlayerLives));
			}
			else if (strncmp(magic, levelMagic, 4) == 0) {
				m_Score = 0;
				m_PlayerLives = 3;
			}
			else {
				return;
			}

			uint32_t objectCount = 0;
			file.read((char*)&objectCount, sizeof(objectCount));

			m_GameObjects.resize(objectCount);

			for (uint32_t i = 0; i < objectCount; i++) {
				m_GameObjects[i].loadFromFile(file, saveFile);
			}
			
			delete[] magic;

			file.close();
		}
	}

	void GameLevel::saveLevel(std::filesystem::path levelPath, bool saveFile)
	{
		std::ofstream file(levelPath, std::ios::out | std::ios::binary);

		if (saveFile) {
			file.write(saveFileMagic, sizeof(saveFileMagic));
			file.write((char*)&m_Score, sizeof(m_Score));
			file.write((char*)&m_PlayerLives, sizeof(m_PlayerLives));
		}
		else {
			file.write(levelMagic, sizeof(levelMagic));
		}

		uint32_t objectCount = m_GameObjects.size();
		
		file.write((char*)&objectCount, sizeof(objectCount));

		for(auto& obj : m_GameObjects) {
			obj.saveToFile(file, saveFile);
		}

		file.close();
	}

	GameObject& GameLevel::getGameObject(ObjectID id)
	{
		static GameObject invalidObject;

		for (auto& obj : m_GameObjects) {
			if (obj.id == id) {
				return obj;
			}
		}
		return invalidObject;
	}

	std::vector<uint32_t> GameLevel::getGameObjectGroupIndices(uint32_t groupID)
	{
		std::vector<uint32_t> indices;

		for (int i = 0; i < m_GameObjects.size(); i++) {
			if (m_GameObjects[i].triggerGroup == groupID)
				indices.push_back(i);
		}

		return indices;
	}

}

