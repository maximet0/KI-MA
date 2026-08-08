#pragma once
#include <DirectXMath.h>
#include <array>

namespace Game {
	
	enum GameObjectType {
		Invalid = 0,
		Background,
		Terrain,
		Player,
	};

	struct GameCollider {
		DirectX::XMFLOAT2 position = {0, 0};
		DirectX::XMFLOAT2 size = {0, 0};
	};

	struct PhysicsInfo {
		float mass = 1.0f;

		DirectX::XMFLOAT2 velocity = { 0, 0 };
		DirectX::XMFLOAT2 acceleration = { 0, 0 };
	};

	struct GameObject {
		GameObjectType type = GameObjectType::Background;	
		DirectX::XMFLOAT2 position = {0, 0};
		DirectX::XMFLOAT2 size = {0, 0};
		uint32_t textureHandle = 0;

		GameCollider collider;

		bool isStatic = true;
		PhysicsInfo physics;

	};

}