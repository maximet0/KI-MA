#include "GameTriggers.h"
#include "GameInstance.h"

#include "Core/Logger.h"

namespace Game{
	void GameTriggers::updateTriggers(GameInstance& instance, float deltaTime)
	{
		for (uint16_t i = 0; i < instance.m_GameLevel.getGameObjectCount(); i++) {
			GameObject& obj = instance.m_GameLevel.getGameObjects()[i];
			if (obj.flags & GameObjectFlags::Trigger) {
				for (auto& trig : obj.triggers) {
					if (trig->type == TriggerType::None) continue;

					switch (trig->condition) {
					case TriggerCondition::OnEnter:
						if (trig->insideTrigger && !trig->wasInsideTrigger && trig->activationState == TriggerActivationState::Inactive) {
							trig->activationState = TriggerActivationState::Active;
						}
						break;
					case TriggerCondition::OnStay:
						if (trig->insideTrigger && trig->activationState != TriggerActivationState::Finished) trig->activationState = TriggerActivationState::Active;
						else if (trig->wasInsideTrigger) trig->activationState = TriggerActivationState::Finished;
						break;
					case TriggerCondition::OnExit:
						if (!trig->insideTrigger && trig->wasInsideTrigger && trig->activationState == TriggerActivationState::Inactive) {
							trig->activationState = TriggerActivationState::Active;
						}
						if (trig->insideTrigger && trig->wasInsideTrigger && trig->activationState == TriggerActivationState::Active)
							trig->activationState = TriggerActivationState::Finished;
						break;
					}


					// Trigger Activated
					if (trig->activationState == TriggerActivationState::Active || trig->activationState == TriggerActivationState::Finished) {
						switch (trig->type) {
						case TriggerType::CameraTrigger: {
							updateCameraTrigger(instance, static_cast<CameraTrigger*>(trig.get()), deltaTime);
							break;
						}
						case TriggerType::ObjectMoveTrigger: {
							updateObjectMoveTrigger(instance, static_cast<ObjectMoveTrigger*>(trig.get()), deltaTime);
							break;
						}
						case TriggerType::ScoreTrigger: {
							updateScoreTrigger(instance, static_cast<ScoreTrigger*>(trig.get()), deltaTime);
							break;
						}
						case TriggerType::DamageTrigger: {
							updateDamageTrigger(instance, static_cast<DamageTrigger*>(trig.get()), deltaTime);
							break;
						}
						case TriggerType::FinishTrigger: {
							updateFinishTrigger(instance, static_cast<FinishTrigger*>(trig.get()), deltaTime);
							break;
						}
						}

					}

					if (trig->activationState == TriggerActivationState::Finished) {
						if(!trig->singleUse && !(trig->condition == TriggerCondition::OnStay && trig->wasInsideTrigger)) 
							trig->activationState = TriggerActivationState::Inactive;
					}

					if (trig->insideTrigger) trig->wasInsideTrigger = true;
					else trig->wasInsideTrigger = false;
					trig->insideTrigger = false;
				}
			}
		}

	}

	void GameTriggers::updateFinishTrigger(GameInstance& instance, FinishTrigger* trig, float deltaTime)
	{
		if (trig->activationState == TriggerActivationState::Finished) return;

		if (instance.m_GameLevel.getScore() >= trig->minimumScore) {
			if (trig->nextLevelPath == "") {
				Core::Logger::Fatal("Implement Game Win");
			}

			uint32_t score = instance.m_GameLevel.getScore();
			uint32_t playerLives = instance.m_GameLevel.getPlayerLives();
			instance.m_GameLevel.loadLevel(trig->nextLevelPath);
			instance.m_GameLevel.setScore(score);
			instance.m_GameLevel.setPlayerLives(playerLives);
			return;
		}
	}

	void GameTriggers::updateCameraTrigger(GameInstance& instance, CameraTrigger* trig, float deltaTime)
	{
		Core::Logger::Debug("Progress: {}", trig->triggerProgress);

		if (trig->activationState == TriggerActivationState::Finished) {
			trig->triggerProgress = 0.0f;
			return;
		}

		if (trig->triggerProgress <= 0.0f || (!trig->wasInsideTrigger && trig->insideTrigger)) {
			trig->startPosition = instance.m_CameraPosition;
			trig->startZoom = instance.m_CameraZoom;
			trig->startFollowPlayer = instance.m_CameraFollowPlayer;
			instance.m_CameraFollowPlayer = trig->followPlayer;
		}

		if (trig->followPlayer) {
			trig->targetPosition = instance.m_CameraPosition;
		}
		trig->triggerProgress += deltaTime / trig->transitionTime;

		DirectX::XMVECTOR startVec = DirectX::XMVectorSet(trig->startPosition.x, trig->startPosition.y, trig->startZoom, 0);
		DirectX::XMVECTOR targetVec = DirectX::XMVectorSet(trig->targetPosition.x, trig->targetPosition.y, trig->targetZoom, 0);

		trig->triggerProgress = std::clamp(trig->triggerProgress, 0.0f, 1.0f);

		DirectX::XMFLOAT3 result;
		DirectX::XMStoreFloat3(&result, DirectX::XMVectorLerp(startVec, targetVec, trig->triggerProgress));

		instance.m_CameraPosition.x = result.x;
		instance.m_CameraPosition.y = result.y;
		instance.m_CameraZoom = result.z;

		if (trig->triggerProgress >= 1.0f) {
			trig->activationState = TriggerActivationState::Finished;
			trig->triggerProgress = 0.0f;
		}
	}

	void GameTriggers::updateObjectMoveTrigger(GameInstance& instance, ObjectMoveTrigger* trig, float deltaTime)
	{
		if (trig->activationState == TriggerActivationState::Finished) {
			trig->currentPoint = 0;
			trig->pointProgress = 0.0f;
			return;
		}


		auto objectIndices = instance.m_GameLevel.getGameObjectGroupIndices(trig->targetGroupID);
		if (objectIndices.size() == 0) {
			trig->activationState = TriggerActivationState::Finished;
			return;
		}

		auto& curMovePoint = trig->pathPoints[trig->currentPoint];
		if (curMovePoint.moveTime != 0) {
			auto& objects = instance.m_GameLevel.getGameObjects();
			uint32_t anchorIndex = 0;
			for (uint32_t& index : objectIndices) {
				if (objects[index].flags & GameObjectFlags::Static) {
					anchorIndex = index;
					break;
				}
			}
			

			auto& anchorObject = instance.m_GameLevel.getGameObjects()[anchorIndex];
			if (trig->pointProgress <= 0.0f) {
				trig->startPosition = anchorObject.position;
			}

			DirectX::XMVECTOR anchorPos = DirectX::XMLoadFloat2(&anchorObject.position);

			trig->pointProgress += deltaTime / curMovePoint.moveTime;

			DirectX::XMVECTOR startVec = DirectX::XMLoadFloat2(&trig->startPosition);
			DirectX::XMVECTOR targetVec = DirectX::XMLoadFloat2(&curMovePoint.position);

			DirectX::XMVECTOR moveVec = DirectX::XMVectorLerp(startVec, targetVec, trig->pointProgress);

			DirectX::XMFLOAT2 velocity;
			DirectX::XMStoreFloat2(&velocity, DirectX::XMVectorDivide(DirectX::XMVectorSubtract(targetVec, startVec), DirectX::XMVectorSet(curMovePoint.moveTime, curMovePoint.moveTime, 0.0f, 0.0f)));

			for(auto& objIndex : objectIndices) {
				auto& targetObject = instance.m_GameLevel.getGameObjects()[objIndex];

				if (targetObject.flags & GameObjectFlags::Static) {
					DirectX::XMVECTOR anchorOffset = DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&targetObject.position), anchorPos);
					DirectX::XMStoreFloat2(&targetObject.position, DirectX::XMVectorAdd(moveVec, anchorOffset));
				}

				bool xDirChanged = (targetObject.physics.groupVelocity.x > 0.0f && velocity.x < 0.0f) || (targetObject.physics.groupVelocity.x < 0.0f && velocity.x > 0.0f);
				bool yDirChanged = (targetObject.physics.groupVelocity.y > 0.0f && velocity.y < 0.0f) || (targetObject.physics.groupVelocity.y < 0.0f && velocity.y > 0.0f);

				if((!xDirChanged && !yDirChanged) || (targetObject.flags & GameObjectFlags::Static)) {
					targetObject.physics.groupVelocity.x = velocity.x;
					targetObject.physics.groupVelocity.y = velocity.y;
				}
			}

			trig->pointProgress = std::clamp(trig->pointProgress, 0.0f, 1.0f);

			if (trig->pointProgress >= 1.0f) {
				trig->pointProgress = 0.0f;
				trig->currentPoint++;
				if (trig->currentPoint >= trig->pathPoints.size()) {
					if (trig->loop) trig->currentPoint = 0;
					else {
						trig->activationState = TriggerActivationState::Finished;
						trig->currentPoint = 0;
						trig->pointProgress = 0.0f;
					}
				}
			}
		}
	}

	void GameTriggers::updateScoreTrigger(GameInstance& instance, ScoreTrigger* trig, float deltaTime)
	{
		if (trig->activationState == TriggerActivationState::Finished) return;

		instance.m_GameLevel.setScore(instance.m_GameLevel.getScore() + trig->scoreAmount);
		trig->activationState = TriggerActivationState::Finished;
	}

	void GameTriggers::updateDamageTrigger(GameInstance& instance, DamageTrigger* trig, float deltaTime)
	{
		if (trig->activationState == TriggerActivationState::Finished) return;

		instance.m_GameLevel.setPlayerLives(instance.m_GameLevel.getPlayerLives() + trig->damageAmount);
		trig->activationState = TriggerActivationState::Finished;
	}

	TriggerType type = TriggerType::None;
	TriggerCondition condition = TriggerCondition::None;
	bool singleUse = false;

	uint8_t activationState = 0;
	bool wasInsideTrigger = false;
	bool insideTrigger = false;

	void TriggerBase::saveToFile(std::ofstream& file, bool saveFile)
	{
		file.write((char*)&type, sizeof(type));
		file.write((char*)&condition, sizeof(condition));
		file.write((char*)&singleUse, sizeof(singleUse));

		if (saveFile) {
			file.write((char*)&activationState, sizeof(activationState));
			file.write((char*)&wasInsideTrigger, sizeof(wasInsideTrigger));
			file.write((char*)&insideTrigger, sizeof(insideTrigger));
		}

		switch (type) {
			case TriggerType::CameraTrigger: {
				CameraTrigger* trig = static_cast<CameraTrigger*>(this);
				trig->save(file, saveFile);
				break;
			}
			case TriggerType::ObjectMoveTrigger: {
				ObjectMoveTrigger* trig = static_cast<ObjectMoveTrigger*>(this);
				trig->save(file, saveFile);
				break;
			}
			case TriggerType::ScoreTrigger: {
				ScoreTrigger* trig = static_cast<ScoreTrigger*>(this);
				trig->save(file, saveFile);
				break;
			}
			case TriggerType::FinishTrigger: {
				FinishTrigger* trig = static_cast<FinishTrigger*>(this);
				trig->save(file, saveFile);
				break;
			}
			case TriggerType::DamageTrigger: {
				DamageTrigger* trig = static_cast<DamageTrigger*>(this);
				trig->save(file, saveFile);
				break;
			}
		}
	}

	void TriggerBase::loadFromFile(std::ifstream & file, TriggerType type, bool saveFile)
	{
		//file.read((char*)&type, sizeof(type));
		file.read((char*)&condition, sizeof(condition));
		file.read((char*)&singleUse, sizeof(singleUse));

		if (saveFile) {
			file.read((char*)&activationState, sizeof(activationState));
			file.read((char*)&wasInsideTrigger, sizeof(wasInsideTrigger));
			file.read((char*)&insideTrigger, sizeof(insideTrigger));
		}

		switch (type) {
			case TriggerType::CameraTrigger: {
				CameraTrigger* trig = static_cast<CameraTrigger*>(this);
				trig->load(file, saveFile);
				break;
			}
			case TriggerType::ObjectMoveTrigger: {
				ObjectMoveTrigger* trig = static_cast<ObjectMoveTrigger*>(this);
				trig->load(file, saveFile);
				break;
			}
			case TriggerType::ScoreTrigger: {
				ScoreTrigger* trig = static_cast<ScoreTrigger*>(this);
				trig->load(file, saveFile);
				break;
			}
			case TriggerType::FinishTrigger: {
				FinishTrigger* trig = static_cast<FinishTrigger*>(this);
				trig->load(file, saveFile);
				break;
			}
			case TriggerType::DamageTrigger: {
				DamageTrigger* trig = static_cast<DamageTrigger*>(this);
				trig->load(file, saveFile);
				break;
			}
		}
	}

	void CameraTrigger::save(std::ofstream& file, bool saveFile)
	{
		file.write((char*)&targetPosition, sizeof(targetPosition));
		file.write((char*)&targetZoom, sizeof(targetZoom));
		file.write((char*)&followPlayer, sizeof(followPlayer));
		file.write((char*)&transitionTime, sizeof(transitionTime));

		if (saveFile) {
			file.write((char*)&triggerProgress, sizeof(triggerProgress));
			file.write((char*)&startFollowPlayer, sizeof(startFollowPlayer));
			file.write((char*)&startZoom, sizeof(startZoom));
			file.write((char*)&startPosition, sizeof(startPosition));
		}

	}

	void CameraTrigger::load(std::ifstream & file, bool saveFile)
	{
		file.read((char*)&targetPosition, sizeof(targetPosition));
		file.read((char*)&targetZoom, sizeof(targetZoom));
		file.read((char*)&followPlayer, sizeof(followPlayer));
		file.read((char*)&transitionTime, sizeof(transitionTime));

		if (saveFile) {
			file.read((char*)&triggerProgress, sizeof(triggerProgress));
			file.read((char*)&startFollowPlayer, sizeof(startFollowPlayer));
			file.read((char*)&startZoom, sizeof(startZoom));
			file.read((char*)&startPosition, sizeof(startPosition));
		}
	}

	bool loop = false;
	std::vector<uint32_t> targetObjectIDs;
	std::vector<MovePoint> pathPoints;


	uint32_t currentPoint = 0;
	float pointProgress = 0.0f;
	DirectX::XMFLOAT2 startPosition = { 0, 0 };

	void ObjectMoveTrigger::save(std::ofstream& file, bool saveFile)
	{
		file.write((char*)&loop, sizeof(loop));

		file.write((char*)&targetGroupID, sizeof(targetGroupID));

		uint32_t pathPointCount = pathPoints.size();
		file.write((char*)&pathPointCount, sizeof(pathPointCount));
		file.write((char*)pathPoints.data(), sizeof(MovePoint) * pathPointCount);

		if (saveFile) {
			file.write((char*)&currentPoint, sizeof(currentPoint));
			file.write((char*)&pointProgress, sizeof(pointProgress));
			file.write((char*)&startPosition, sizeof(startPosition));
		}

	}

	void ObjectMoveTrigger::load(std::ifstream & file, bool saveFile)
	{
		file.read((char*)&loop, sizeof(loop));

		file.read((char*)&targetGroupID, sizeof(targetGroupID));

		uint32_t pathPointCount;
		file.read((char*)&pathPointCount, sizeof(pathPointCount));
		pathPoints.resize(pathPointCount);
		file.read((char*)pathPoints.data(), sizeof(MovePoint) * pathPointCount);

		if (saveFile) {
			file.read((char*)&currentPoint, sizeof(currentPoint));
			file.read((char*)&pointProgress, sizeof(pointProgress));
			file.read((char*)&startPosition, sizeof(startPosition));
		}
	}

	void ScoreTrigger::save(std::ofstream& file, bool saveFile)
	{
		file.write((char*)&scoreAmount, sizeof(scoreAmount));
	}

	void ScoreTrigger::load(std::ifstream & file, bool saveFile)
	{
		file.read((char*)&scoreAmount, sizeof(scoreAmount));
	}

	void FinishTrigger::save(std::ofstream& file, bool saveFile)
	{
		file.write((char*)&minimumScore, sizeof(minimumScore));
		uint32_t nextLevelPathLength = nextLevelPath.size();
		file.write((char*)&nextLevelPathLength, sizeof(nextLevelPathLength));
		file.write(nextLevelPath.c_str(), nextLevelPathLength);
	}

	void FinishTrigger::load(std::ifstream & file, bool saveFile)
	{
		file.read((char*)&minimumScore, sizeof(minimumScore));
		uint32_t nextLevelPathLength;
		file.read((char*)&nextLevelPathLength, sizeof(nextLevelPathLength));
		nextLevelPath.resize(nextLevelPathLength);
		file.read(&nextLevelPath[0], nextLevelPathLength);
	}

	void DamageTrigger::save(std::ofstream& file, bool saveFile)
	{
		file.write((char*)&damageAmount, sizeof(damageAmount));
	}

	void DamageTrigger::load(std::ifstream & file, bool saveFile)
	{
		file.read((char*)&damageAmount, sizeof(damageAmount));
	}

}

