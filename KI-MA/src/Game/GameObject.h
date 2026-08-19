#pragma once
#include <DirectXMath.h>
#include <array>

namespace Game {
	enum GameObjectFlags {
		Invalid = 0,
		Valid = 1 << 0,
		Background = 1 << 1,
		Player = 1 << 2,
		Trigger = 1 << 3,
		Static = 1 << 4
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

	enum class TriggerType {
		None = 0,
		CameraTrigger = 1,
		ObjectMoveTrigger = 2,
		ScoreTrigger = 3,
		FinishTrigger = 4,
		DamageTrigger = 5,
	};

	struct CameraTriggerInfo {
		bool followPlayer = false;
		float targetZoom = 1.0f;
		DirectX::XMFLOAT2 targetPosition = { 0, 0 };
		float transitionTime = 1.0f;

		float triggerProgress = 0.0f;
		bool startFollowPlayer = false;
		float startZoom = 0.0f;
		DirectX::XMFLOAT2 startPosition = { 0, 0 };
	};

	struct MovePoint {
		DirectX::XMFLOAT2 position = { 0, 0 };
		float moveTime = 0.0f;

		float pointProgress = 0.0f;
		DirectX::XMFLOAT2 startPosition = { 0, 0 };
	};

	struct ObjectMoveTriggerInfo {
		bool loop = false;
		uint16_t targetCount;
		std::array<uint32_t, 32> targetObjectIDs;
		std::array<MovePoint, 16> pathPoints;

		uint32_t currentPoint = 0;
	};

	struct ScoreTriggerInfo {
		uint32_t scoreAmount = 0;
	};

	struct FinishTriggerInfo {
		int32_t minimumScore = 0;
		char nextLevelPath[256];
	};

	struct DamageTriggerInfo {
		uint32_t damageAmount = 0;
	};

	enum TriggerActivationState {
		Inactive = 0,
		Active = 1,
		Finished = 2
	};

	enum class TriggerCondition {
		None = 0,
		OnEnter = 1,
		OnExit = 2,
		OnStay = 3,
		OnScreenVisible = 4,
	};


	struct TriggerInfo {
		TriggerType type = TriggerType::None;
		TriggerCondition condition = TriggerCondition::None;
		bool singleUse = false;


		uint8_t activationState = 0;
		bool wasInsideTrigger = false;
		bool insideTrigger = false;



		CameraTriggerInfo cameraTrigger;
		ObjectMoveTriggerInfo objectMoveTrigger;
		ScoreTriggerInfo scoreTrigger;
		FinishTriggerInfo finishTrigger;
		DamageTriggerInfo damageTrigger;
	};

	struct GameObject {
		GameObjectFlags flags = GameObjectFlags::Invalid;
		DirectX::XMFLOAT2 position = {0, 0};
		DirectX::XMFLOAT2 size = {0, 0};
		char objectName[64] = "Unnamed";
		char textureName[64] = "";

		GameCollider collider;

		PhysicsInfo physics;
		std::array<TriggerInfo, 8> triggers;
	};

}