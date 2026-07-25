#pragma once
#include <DirectXMath.h>

namespace Game {
	
	enum GameObjectType {
		Invalid = 0,
		Background,
		Terrain,
		Player,
	};

	struct GameObject {
		GameObjectType type = GameObjectType::Background;	
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		uint32_t textureHandle;
	};

}