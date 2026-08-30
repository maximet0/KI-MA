#pragma once
#include "GameTriggers.h"

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <fstream>


namespace Game {
	enum GameObjectFlags {
		None = 0,
		Background = 1 << 0,
		Player = 1 << 1,
		Trigger = 1 << 2,
		Static = 1 << 3
	};

	struct GameCollider {
		DirectX::XMFLOAT2 offset = {0, 0};
		DirectX::XMFLOAT2 size = {0, 0};
	};

	struct PhysicsInfo {
		float mass = 1.0f;

		DirectX::XMFLOAT2 velocity = { 0, 0 };
		DirectX::XMFLOAT2 groupVelocity = { 0, 0 };

		DirectX::XMFLOAT2 acceleration = { 0, 0 };
	};

	typedef uint64_t ObjectID;

	struct GameObject {
		GameObjectFlags flags = GameObjectFlags::None;
		ObjectID id = 0;
		uint32_t triggerGroup = 0;
		DirectX::XMFLOAT2 position = {0, 0};
		DirectX::XMFLOAT2 size = {0, 0};

		std::string objectName = "Invalid";
		std::string textureName = "";
		bool repeatTexture = false;

		GameCollider collider;
		PhysicsInfo physics;
		std::vector<std::shared_ptr<TriggerBase>> triggers;

		void saveToFile(std::ofstream& file, bool saveState);
		void loadFromFile(std::ifstream& file, bool saveState);
	};

}