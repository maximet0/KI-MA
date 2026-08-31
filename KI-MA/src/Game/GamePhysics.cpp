#include "GamePhysics.h"

#include "Core/Logger.h"

namespace Game {
	RaycastHit GamePhysics::boxcast(GameLevel& level, const DirectX::XMFLOAT2& origin, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT2& direction, float maxDistance)
	{
		// Schrittgrösse
		float stepSize = 0.5f;

		//Alle GameObjects im Level durchgehen und prüfen, ob sie mit dem Boxcast kollidieren.
		for (auto& obj : level.getGameObjects()) {
			if ((obj.flags & GameObjectFlags::Trigger) != 0) continue;
			GameCollider& collider = obj.collider;
			DirectX::XMFLOAT2 colliderPos = { obj.position.x + collider.offset.x, obj.position.y + collider.offset.y };
			DirectX::XMFLOAT2 colliderSize = collider.size;
			for (float d = 0.0f; d <= maxDistance; d += stepSize) {
				DirectX::XMFLOAT2 boxPos = { origin.x + direction.x * d, origin.y + direction.y * d };
				if (boxPos.x < colliderPos.x + colliderSize.x && boxPos.x + size.x > colliderPos.x &&
					boxPos.y < colliderPos.y + colliderSize.y && boxPos.y + size.y > colliderPos.y) {
					RaycastHit hit;
					hit.object = &obj;
					hit.hit = true;
					hit.hitPoint = boxPos;
					return hit;
				}
			}
		}


		return RaycastHit();
	}

	RaycastHit GamePhysics::raycast(GameLevel& level, const DirectX::XMFLOAT2& origin, const DirectX::XMFLOAT2& direction, float maxDistance)
	{
		
		float stepSize = .5f;
		
		//Alle GameObjects im Level durchgehen und prüfen, ob sie mit dem Raycast kollidieren.
		for (auto& obj : level.getGameObjects()) {	
			if ((obj.flags & GameObjectFlags::Trigger) != 0) continue;
			GameCollider& collider = obj.collider;
			DirectX::XMFLOAT2 colliderPos = { obj.position.x + collider.offset.x, obj.position.y + collider.offset.y };
			DirectX::XMFLOAT2 colliderSize = collider.size;
			for (float d = 0.0f; d <= maxDistance; d += stepSize) {
				DirectX::XMFLOAT2 point = { origin.x + direction.x * d, origin.y + direction.y * d };
				if (point.x >= colliderPos.x && point.x <= colliderPos.x + colliderSize.x &&
					point.y >= colliderPos.y && point.y <= colliderPos.y + colliderSize.y) {
					RaycastHit hit;
					hit.object = &obj;
					hit.hit = true;
					hit.hitPoint = point;
					return hit;
				}
			}
		}
		return RaycastHit();
	}

	void GamePhysics::resolveAxis(GameLevel& level, GameObject& a, float deltaTime, bool isYAxis, bool& noneCollided, bool& noneCollidedWithGround, bool& hitPositive, bool& hitNegative) {

		for (auto& b : level.getGameObjects()) {
			if (&b == &a) continue;
			GameCollider& colliderA = a.collider;
			GameCollider& colliderB = b.collider;
			CollisionResult res = checkCollision(colliderA, a, colliderB, b, isYAxis);

			if (res.collided && b.flags & GameObjectFlags::Static && b.flags & GameObjectFlags::Trigger) {
				for (auto& trig : b.triggers) {
					if (trig->type == TriggerType::None) continue;
					trig->insideTrigger = true;
					//if (trig->activationState != TriggerActivationState::Finished) 
				}
				continue;
			}

			float overlap = isYAxis ? res.overlap.y : res.overlap.x;
			float otherOverlap = isYAxis ? res.overlap.x : res.overlap.y;
			if (overlap == 0.0f) continue;

			if (otherOverlap != 0.0f && std::abs(otherOverlap) < std::abs(overlap)) continue;

			if (overlap > 0.0f) hitPositive = true;
			else hitNegative = true;

			noneCollided = false;
			a.triggerGroup = b.triggerGroup;

			float& aPos = isYAxis ? a.position.y : a.position.x;
			float& aVel = isYAxis ? a.physics.velocity.y : a.physics.velocity.x;
			float aGroupVel = isYAxis ? a.physics.groupVelocity.y : a.physics.groupVelocity.x;
			float bGroupVel = isYAxis ? b.physics.groupVelocity.y : b.physics.groupVelocity.x;

			/*if (isYAxis) {
				float relativeVelocityY = (aVel + aGroupVel) - bGroupVel;
				if ((relativeVelocityY > 0.0f && overlap > 0.0f) || (relativeVelocityY < 0.0f && overlap < 0.0f))
					continue;
			}*/

			if(b.triggerGroup != 0) noneCollidedWithGround = false;

			if (b.flags & GameObjectFlags::Static) {
				noneCollidedWithGround = false;
				if (b.flags & GameObjectFlags::Trigger) continue;

				aPos += overlap;

				float relativeVelocity = aVel + aGroupVel - bGroupVel;
				if (relativeVelocity * overlap < 0.0f) 
					aVel = bGroupVel - aGroupVel;
			}
			else {
				float pushMash = b.physics.mass;
				bool canMove = true;

				if (isYAxis && overlap < 0.0f) canMove = false;
				else if (isYAxis) {
					checkPushableRec(level, b, { 0, overlap }, a.physics.mass, pushMash, canMove);
				}
				else {
					checkPushableRec(level, b, { overlap, 0 }, a.physics.mass, pushMash, canMove);
				}
				
				if (canMove) {
					float massRatio = a.physics.mass / (a.physics.mass + pushMash);
					float& bPos = isYAxis ? b.position.y : b.position.x;
					aPos += overlap * (1.0f - massRatio);
					bPos -= overlap * massRatio;
				}
				else {
					aPos += overlap;
					float relativeVelocity = aVel + aGroupVel - bGroupVel;
					if (relativeVelocity * overlap < 0.0f) 
						aVel = bGroupVel - aGroupVel;
				}
			}	
		}
	}

	constexpr float maxStepDistance = 8.0f;
	constexpr float airDamping = 2.0f;
	constexpr float groundDamping = 10.0f;

	void GamePhysics::updatePhysics(GameLevel& level, float deltaTime, float gravity)
	{
		auto& gameObjects = level.getGameObjects();

		float maxSpeed = 0.0f;
		for (auto& obj : gameObjects) {
			float vx = obj.physics.velocity.x + obj.physics.groupVelocity.x;
			float  vy = obj.physics.velocity.y + obj.physics.groupVelocity.y;
			float speed = std::sqrt(vx * vx + vy * vy);
			if (speed > maxSpeed) maxSpeed = speed;
		}

		int subSteps = std::max(1, (int)std::ceil(maxSpeed * deltaTime / maxStepDistance));
		float subDeltaTime = deltaTime / subSteps;

		for (uint32_t step = 0; step < subSteps; step++) {

			// Alle GameObjects durchgehen und die Physik anwenden
			for (auto& a : gameObjects) {
				if ((a.flags & GameObjectFlags::Static) != 0) continue;

				// Beschleunigungen anwenden
				a.physics.velocity.x += a.physics.acceleration.x * subDeltaTime;
				a.physics.velocity.y += (gravity + a.physics.acceleration.y) * subDeltaTime;

				a.position.x += (a.physics.velocity.x + a.physics.groupVelocity.x) * subDeltaTime;

				bool noneCollided = true;
				bool noneCollidedWithGround = true;
				bool hitPositive = false;
				bool hitNegative = false;

				bool crushed = false;

				resolveAxis(level, a, subDeltaTime, false, noneCollided, noneCollidedWithGround, hitPositive, hitNegative);

				if (hitNegative && hitPositive) crushed = true;
				hitNegative = false;
				hitPositive = false;

				a.position.y += (a.physics.velocity.y + a.physics.groupVelocity.y) * subDeltaTime;
				resolveAxis(level, a, subDeltaTime, true, noneCollided, noneCollidedWithGround, hitPositive, hitNegative);

				if (hitNegative && hitPositive) crushed = true;

				if (noneCollided) {
					a.triggerGroup = 0;
				}

				if (crushed) {
					a.position.x = -9999;
					a.position.y = -9999;
					if (a.flags & GameObjectFlags::Player) {
						level.setPlayerLives(0);
					}
				}

				float dampFactor = exp(-(noneCollidedWithGround ? airDamping : groundDamping) * subDeltaTime);
				a.physics.velocity.x *= dampFactor;
				a.physics.velocity.y *= dampFactor;
				a.physics.groupVelocity.x *= dampFactor;
				a.physics.groupVelocity.y *= dampFactor;

				if (std::abs(a.physics.velocity.x) < 1.f) a.physics.velocity.x = 0.0f;
				if (std::abs(a.physics.velocity.y) < 1.f) a.physics.velocity.y = 0.0f;
				if (std::abs(a.physics.groupVelocity.x) < 1.f) a.physics.groupVelocity.x = 0.0f;
				if (std::abs(a.physics.groupVelocity.y) < 1.f) a.physics.groupVelocity.y = 0.0f;

			}
		}
	}


	//Toleranz Probleme bei den kollisionen zu vermeiden.
	constexpr float tolerance = 1.0f;

	CollisionResult GamePhysics::checkCollision(const GameCollider& a, const GameObject& aObj, const GameCollider& b, const GameObject& bObj, bool yAxis)
	{
		CollisionResult result;

		DirectX::XMVECTOR offsets = DirectX::XMVectorSet(a.offset.x, a.offset.y, b.offset.x, b.offset.y);
		DirectX::XMVECTOR positions = DirectX::XMVectorSet(aObj.position.x, aObj.position.y, bObj.position.x, bObj.position.y);
		positions = DirectX::XMVectorAdd(offsets, positions);

		DirectX::XMVECTOR colliderSize;
		if (yAxis) colliderSize = DirectX::XMVectorSet(a.size.x - tolerance, a.size.y, b.size.x - tolerance, b.size.y);
		else colliderSize = DirectX::XMVectorSet(a.size.x, a.size.y - tolerance, b.size.x, b.size.y - tolerance);

		DirectX::XMVECTOR colliderBounds = DirectX::XMVectorAdd(positions, colliderSize);
		colliderBounds = DirectX::XMVectorSwizzle<2, 3, 0, 1>(colliderBounds);

		uint32_t cr = DirectX::XMVector4GreaterR(colliderBounds, positions);

		//AABB kollisionerkennung 
		if (DirectX::XMComparisonAllTrue(cr)) {
			DirectX::XMVECTOR overlap = DirectX::XMVectorSubtract(colliderBounds, positions);
			DirectX::XMVECTOR overlapSwizzled = DirectX::XMVectorSwizzle<2, 3, 0, 1>(overlap);

			overlap = DirectX::XMVectorMin(overlap, overlapSwizzled);

			DirectX::XMVECTOR positionDiff = DirectX::XMVectorSubtract(positions, DirectX::XMVectorSwizzle<2, 3, 0, 1>(positions));
			DirectX::XMVECTOR sign = DirectX::XMVectorSelect(DirectX::XMVectorReplicate(1.0f), DirectX::XMVectorReplicate(-1.0f), DirectX::XMVectorLess(positionDiff, DirectX::XMVectorZero()));

			overlap = DirectX::XMVectorMultiply(overlap, sign);

			DirectX::XMFLOAT2 overlapFloat;
			DirectX::XMStoreFloat2(&result.overlap, overlap);

			result.collided = true;
		}

		return result;
	}


	void GamePhysics::checkPushableRec(GameLevel& level, GameObject& originObj, const DirectX::XMFLOAT2& direction, float firstMass, float& totalMass, bool& canMove)
	{
		RaycastHit hit;
		float dirSign = 0;
		float deltaX = 0;
		float deltaY = 0;

		if (direction.x != 0) {
			//X Richtung
			deltaX = direction.x;
			dirSign = (-deltaX > 0) - (-deltaX < 0);
			// Anfangsposition für den Boxcast, außerhalb des Ursprungsobjekts um Kollisionen mit sich selbst zu vermeiden.
			float xOrigin = 0;
			if (deltaX > 0) xOrigin = originObj.position.x - 0.01f;
			else xOrigin = originObj.position.x + originObj.size.x + 0.01f;

			// Boxcast in die Richtung der Bewegung, um das Objekt zu finden, welches als nächstes in der Kette verschoben werden muss.
			hit = boxcast(level, { xOrigin, originObj.position.y }, { 0.01f, originObj.size.y }, { dirSign, 0 }, 0.5f);
		}
		else {
			// Y Richtung
			deltaY = direction.y;
			dirSign = (-deltaY > 0) - (-deltaY < 0);
			float yOrigin = 0;
			if (deltaY > 0) yOrigin = originObj.position.y - 0.01f;
			else yOrigin = originObj.position.y + originObj.size.y + 0.01f;
			hit = boxcast(level, { originObj.position.x, yOrigin }, { originObj.size.x, 0.01f }, { 0, dirSign }, 0.5f);
		}

		if (hit.hit) {
			GameObject* obj = hit.object;
			if (obj->id == originObj.id) return;
			// Wenn das nächste Objekt statisch ist, sollte die Kette nicht verschoben werden können.
			if (obj->flags & GameObjectFlags::Static) canMove = false;
			else {
				// Masse des Objekts zur Gesamtmasse hinzufügen und rekursiv prüfen, ob das nächste Objekt in der Kette verschoben werden kann.
				totalMass += obj->physics.mass;
				checkPushableRec(level, *obj, direction, firstMass, totalMass, canMove);

				// Position des Objekts in der Kette basierend auf dem Massenverhältnis zwischen dem ersten Objekt und der Gesamtmasse verschieben. 
				if (deltaX != 0 && canMove) {
					float massRatio = firstMass / (originObj.physics.mass + totalMass);
					obj->position.x -= deltaX * massRatio;
				}
				else if (deltaY != 0 && canMove) {
					float massRatio = firstMass / (originObj.physics.mass + totalMass);
					obj->position.y -= deltaY * massRatio;
				}

			}
		}

	}

}

