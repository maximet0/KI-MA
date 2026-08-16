#pragma once

#include "GameInstance.h"

namespace Game {

	class GameTriggers {
	public:

		static void updateTriggers(GameInstance& instance, float deltaTime);

	private:

		static void updateCameraTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime);
		static void updateObjectMoveTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime);
		static void updateScoreTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime);
		static void updateFinishTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime);
		static void updateDamageTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime);
	};

}