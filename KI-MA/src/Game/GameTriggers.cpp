#include "GameTriggers.h"

#include "Core/Logger.h"

namespace Game{
	void GameTriggers::updateTriggers(GameInstance& instance, float deltaTime)
	{
		for (uint16_t i = 0; i < instance.m_GameLevel.getGameObjectCount(); i++) {
			GameObject& obj = instance.m_GameLevel.getGameObjects()[i];
			if (obj.flags & GameObjectFlags::Trigger) {
				for (TriggerInfo& trig : obj.triggers) {
					if (trig.type == TriggerType::None) continue;

					switch (trig.condition) {
					case TriggerCondition::OnEnter:
						if (trig.insideTrigger && !trig.wasInsideTrigger && trig.activationState == TriggerActivationState::Inactive) {
							trig.activationState = TriggerActivationState::Active;
						}
						break;
					case TriggerCondition::OnStay:
						if (trig.insideTrigger) {
							trig.activationState = TriggerActivationState::Active;
						}
						else {
							trig.activationState = TriggerActivationState::Finished;
						}
						break;
					case TriggerCondition::OnExit:
						if (!trig.insideTrigger && trig.wasInsideTrigger && trig.activationState == TriggerActivationState::Inactive) {
							trig.activationState = TriggerActivationState::Active;
						}
						break;
					}


					// Trigger Activated
					if (trig.activationState == TriggerActivationState::Active) {
						switch (trig.type) {
						case TriggerType::CameraTrigger: {
							updateCameraTrigger(instance, trig, deltaTime);
							break;
						}
						case TriggerType::ObjectMoveTrigger: {
							updateObjectMoveTrigger(instance, trig, deltaTime);
							break;
						}
						case TriggerType::ScoreTrigger: {
							updateScoreTrigger(instance, trig, deltaTime);
							break;
						}
						case TriggerType::DamageTrigger: {
							updateDamageTrigger(instance, trig, deltaTime);
							break;
						}
						case TriggerType::FinishTrigger: {


							if (instance.m_GameLevel.getScore() >= trig.finishTrigger.minimumScore) {
								if (strcmp(trig.finishTrigger.nextLevelPath, "") == 0) {
									Core::Logger::Fatal("Implement Game Win");
								}
	
								uint32_t score = instance.m_GameLevel.getScore();
								uint32_t playerLives = instance.m_GameLevel.getPlayerLives();
								instance.m_GameLevel.loadLevel(trig.finishTrigger.nextLevelPath);
								instance.m_GameLevel.setScore(score);
								instance.m_GameLevel.setPlayerLives(playerLives);
								return;
							}
							else {
								break;
							}
						}
						}

					}
					else if (trig.activationState == TriggerActivationState::Finished) {
						if(!trig.singleUse) trig.activationState = TriggerActivationState::Inactive;
					}


					if (trig.insideTrigger) trig.wasInsideTrigger = true;
					else trig.wasInsideTrigger = false;
					trig.insideTrigger = false;
				}
			}
		}

	}

	void GameTriggers::updateCameraTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime)
	{

		auto& cameraTrigger = trig.cameraTrigger;

		if (cameraTrigger.triggerProgress <= 0.0f || (!trig.wasInsideTrigger && trig.insideTrigger)) {
			cameraTrigger.triggerProgress = 0.0f;
			cameraTrigger.startPosition = instance.m_CameraPosition;
			cameraTrigger.startZoom = instance.m_CameraZoom;
			cameraTrigger.startFollowPlayer = instance.m_CameraFollowPlayer;
			instance.m_CameraFollowPlayer = cameraTrigger.followPlayer;
		}

		if (cameraTrigger.followPlayer) {
			cameraTrigger.targetPosition = instance.m_CameraPosition;
		}
		cameraTrigger.triggerProgress += deltaTime / cameraTrigger.transitionTime;

		DirectX::XMVECTOR startVec = DirectX::XMVectorSet(cameraTrigger.startPosition.x, cameraTrigger.startPosition.y, cameraTrigger.startZoom, 0);
		DirectX::XMVECTOR targetVec = DirectX::XMVectorSet(cameraTrigger.targetPosition.x, cameraTrigger.targetPosition.y, cameraTrigger.targetZoom, 0);

		cameraTrigger.triggerProgress = std::clamp(cameraTrigger.triggerProgress, 0.0f, 1.0f);

		DirectX::XMFLOAT3 result;
		DirectX::XMStoreFloat3(&result, DirectX::XMVectorLerp(startVec, targetVec, cameraTrigger.triggerProgress));

		instance.m_CameraPosition.x = result.x;
		instance.m_CameraPosition.y = result.y;
		instance.m_CameraZoom = result.z;

		if (cameraTrigger.triggerProgress >= 1.0f) {
			trig.activationState = TriggerActivationState::Finished;
			cameraTrigger.triggerProgress = 0.0f;
		}
	}

	void GameTriggers::updateObjectMoveTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime)
	{
		auto& oMT = trig.objectMoveTrigger;
		if (oMT.targetCount == 0) return;


		auto& curMovePoint = oMT.pathPoints[oMT.currentPoint];
		if (curMovePoint.moveTime != 0) {
			auto& anchorObject = instance.m_GameLevel.getGameObjects()[oMT.targetObjectIDs[0]];
			if (curMovePoint.pointProgress <= 0.0f) {
				curMovePoint.startPosition = anchorObject.position;
			}

			DirectX::XMVECTOR anchorPos = DirectX::XMLoadFloat2(&anchorObject.position);

			curMovePoint.pointProgress += deltaTime / curMovePoint.moveTime;

			for (uint32_t i = 0; i < oMT.targetCount; i++) {
				auto& targetObject = instance.m_GameLevel.getGameObjects()[oMT.targetObjectIDs[i]];

				DirectX::XMVECTOR startVec = DirectX::XMLoadFloat2(&curMovePoint.startPosition);
				DirectX::XMVECTOR targetVec = DirectX::XMLoadFloat2(&curMovePoint.position);

				DirectX::XMVECTOR anchorOffset = DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&targetObject.position), anchorPos);
				
				startVec = DirectX::XMVectorAdd(startVec, anchorOffset);
				targetVec = DirectX::XMVectorAdd(targetVec, anchorOffset);

				DirectX::XMStoreFloat2(&targetObject.position, DirectX::XMVectorLerp(startVec, targetVec, curMovePoint.pointProgress));
			}

			curMovePoint.pointProgress = std::clamp(curMovePoint.pointProgress, 0.0f, 1.0f);

			if (curMovePoint.pointProgress >= 1.0f) {
				curMovePoint.pointProgress = 0.0f;
				oMT.currentPoint++;
				if (oMT.pathPoints[oMT.currentPoint].moveTime == 0) {
					if (oMT.loop) oMT.currentPoint = 0;
					else trig.activationState = TriggerActivationState::Finished;
				}
			}
		}
	}

	void GameTriggers::updateScoreTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime)
	{
		auto sT = trig.scoreTrigger;
		instance.m_GameLevel.setScore(instance.m_GameLevel.getScore() + sT.scoreAmount);
		trig.activationState = TriggerActivationState::Finished;
	}

	void GameTriggers::updateDamageTrigger(GameInstance& instance, TriggerInfo& trig, float deltaTime)
	{
		auto dT = trig.damageTrigger;
		instance.m_GameLevel.setPlayerLives(instance.m_GameLevel.getPlayerLives() + dT.damageAmount);
		trig.activationState = TriggerActivationState::Finished;
	}

}

