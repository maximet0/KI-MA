#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <fstream>

namespace Game {


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

	enum class TriggerType {
		None = 0,
		CameraTrigger = 1,
		ObjectMoveTrigger = 2,
		ScoreTrigger = 3,
		FinishTrigger = 4,
		DamageTrigger = 5,
	};

	struct TriggerBase {
		TriggerType type = TriggerType::None;
		TriggerCondition condition = TriggerCondition::None;
		bool singleUse = false;

		uint8_t activationState = 0;
		bool wasInsideTrigger = false;
		bool insideTrigger = false;

		void saveToFile(std::ofstream& file, bool saveFile);
		void loadFromFile(std::ifstream& file, TriggerType type, bool saveFile);
	};

	enum FollowPlayerAxis : uint8_t {
		FollowPlayerNone = 0,
		FollowPlayerX = 1 << 0,
		FollowPlayerY = 1 << 1,
		FollowPlayerBoth = FollowPlayerX | FollowPlayerY
	};

	struct CameraTrigger : TriggerBase {
		CameraTrigger() {
			type = TriggerType::CameraTrigger;
		}

		FollowPlayerAxis followPlayer = FollowPlayerAxis::FollowPlayerNone;
		float targetZoom = 1.0f;
		DirectX::XMFLOAT2 targetPosition = { 0, 0 };
		float transitionTime = 1.0f;

		float triggerProgress = 0.0f;
		FollowPlayerAxis startFollowPlayer = FollowPlayerAxis::FollowPlayerNone;
		float startZoom = 0.0f;
		DirectX::XMFLOAT2 startPosition = { 0, 0 };

		void save(std::ofstream& file, bool saveFile);
		void load(std::ifstream& file, bool saveFile);
	};

	struct MovePoint {
		DirectX::XMFLOAT2 position = { 0, 0 };
		float moveTime = 0.0f;
	};

	struct ObjectMoveTrigger : TriggerBase {
		ObjectMoveTrigger() {
			type = TriggerType::ObjectMoveTrigger;
		}

		bool loop = false;
		uint32_t targetGroupID = 0;
		std::vector<MovePoint> pathPoints;


		uint32_t currentPoint = 0;
		float pointProgress = 0.0f;
		DirectX::XMFLOAT2 startPosition = { 0, 0 };

		void save(std::ofstream& file, bool saveFile);
		void load(std::ifstream& file, bool saveFile);
	};

	struct ScoreTrigger : TriggerBase {
		ScoreTrigger() {
			type = TriggerType::ScoreTrigger;
		}

		uint32_t scoreAmount = 0;

		void save(std::ofstream& file, bool saveFile);
		void load(std::ifstream& file, bool saveFile);
	};

	struct FinishTrigger : TriggerBase {
		FinishTrigger() {
			type = TriggerType::FinishTrigger;
		}

		int32_t minimumScore = 0;
		std::string nextLevelPath = "";

		void save(std::ofstream& file, bool saveFile);
		void load(std::ifstream& file, bool saveFile);
	};

	struct DamageTrigger : TriggerBase {
		DamageTrigger() {
			type = TriggerType::DamageTrigger;
		}
		uint32_t damageAmount = 0;

		void save(std::ofstream& file, bool saveFile);
		void load(std::ifstream& file, bool saveFile);
	};

	class GameInstance;

	class GameTriggers {
	public:

		static void updateTriggers(GameInstance& instance, float deltaTime);
	private:

		static void updateCameraTrigger(GameInstance& instance, CameraTrigger* trig, float deltaTime);
		static void updateObjectMoveTrigger(GameInstance& instance, ObjectMoveTrigger* trig, float deltaTime);
		static void updateScoreTrigger(GameInstance& instance, ScoreTrigger* trig, float deltaTime);
		static void updateFinishTrigger(GameInstance& instance, FinishTrigger* trig, float deltaTime);
		static void updateDamageTrigger(GameInstance& instance, DamageTrigger* trig, float deltaTime);
	};

}