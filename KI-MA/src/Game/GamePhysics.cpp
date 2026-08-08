#include "GamePhysics.h"

#include "Core/Logger.h"

namespace Game {
	RaycastHit GamePhysics::boxcast(GameLevel& level, const DirectX::XMFLOAT2& origin, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT2& direction, float maxDistance)
	{
		// Schrittgrösse
		float stepSize = 0.25f;

		//Alle GameObjects im Level durchgehen und prüfen, ob sie mit dem Boxcast kollidieren.
		auto& gameObjects = level.getGameObjects();
		for (uint32_t i = 0; i < level.getGameObjectCount(); i++) {
			GameObject& obj = gameObjects[i];
			if (obj.type == GameObjectType::Invalid) continue;
			GameCollider& collider = obj.collider;
			DirectX::XMFLOAT2 colliderPos = { obj.position.x + collider.position.x, obj.position.y + collider.position.y };
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

		float stepSize = 0.25f;
		
		//Alle GameObjects im Level durchgehen und prüfen, ob sie mit dem Raycast kollidieren.
		auto& gameObjects = level.getGameObjects();
		for (uint32_t i = 0; i < level.getGameObjectCount(); i++) {
			GameObject& obj = gameObjects[i];
			if (obj.type == GameObjectType::Invalid) continue;
			GameCollider& collider = obj.collider;
			DirectX::XMFLOAT2 colliderPos = { obj.position.x + collider.position.x, obj.position.y + collider.position.y };
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



	void GamePhysics::updatePhysics(GameLevel& level, float deltaTime, float gravity)
	{
		auto& gameObjects = level.getGameObjects();


		// Alle GameObjects durchgehen und die Physik anwenden
		for (uint16_t i = 0; i < level.getGameObjectCount(); i++) {
			GameObject& a = gameObjects[i];
			if (a.isStatic || a.type == GameObjectType::Invalid) continue;

			// Beschleunigungen anwenden
			a.physics.velocity.x += a.physics.acceleration.x * deltaTime;
			a.physics.velocity.y += (gravity + a.physics.acceleration.y) * deltaTime;
			
			// X Position basierend auf der Geschwindigkeit aktualisieren
			a.position.x += a.physics.velocity.x * deltaTime;

			// Kollisionen auf der X-Achse prüfen
			for (uint16_t j = 0; j < level.getGameObjectCount(); j++) {
				if (j == i || gameObjects[j].type == GameObjectType::Invalid) continue;
				GameObject& b = gameObjects[j];

				GameCollider& colliderA = a.collider;
				GameCollider& colliderB = b.collider;
				float deltaX = checkCollisionX(colliderA, a, colliderB, b);
				if (deltaX != 0.0f) {
					if (b.isStatic) {
						a.position.x += deltaX;
						a.physics.velocity.x = 0.0f;
					}
					else {
						float pushMash = b.physics.mass;
						bool canMove = true;

						checkPushableRec(level, b, { deltaX, 0 }, a.physics.mass, pushMash, canMove);

						if (canMove) {
							float massRatio = a.physics.mass / (a.physics.mass + pushMash);
							a.position.x += deltaX * (1.0f - massRatio);;
							b.position.x -= deltaX * massRatio;
						}
						else {
							a.position.x += deltaX;
							a.physics.velocity.x = 0.0f;
						}
					}
				}
			}

			// Y Position basierend auf der Geschwindigkeit aktualisieren
			a.position.y += a.physics.velocity.y * deltaTime;

			// Kollisionen auf der Y-Achse prüfen
			for (uint16_t j = 0; j < level.getGameObjectCount(); j++) {
				if (j == i || gameObjects[j].type == GameObjectType::Invalid) continue;
				GameObject& b = gameObjects[j];

				GameCollider& colliderA = a.collider;
				GameCollider& colliderB = b.collider;
				float deltaY = checkCollisionY(colliderA, a, colliderB, b);
				if (deltaY != 0.0f) {
					if (b.isStatic) {
						a.position.y += deltaY;
						a.physics.velocity.y = 0.0f;
					}
					else {
						float pushMash = b.physics.mass;
						bool canMove = true;
	
						if (deltaY < 0.0f) canMove = false;
						else checkPushableRec(level, b, { 0, deltaY }, a.physics.mass, pushMash, canMove);
								
						if (canMove) {
							float massRatio = a.physics.mass / (a.physics.mass + pushMash);
							a.position.y += deltaY * (1.0f - massRatio);
							b.position.y -= deltaY * massRatio;
						}
						else
						{
							a.position.y += deltaY;
							a.physics.velocity.y = 0.0f;
						}
					}
				}
			}
		}
	}

	//Toleranz Probleme bei den kollisionen zu vermeiden.
	constexpr float tolerance = 1.0f;


	float GamePhysics::checkCollisionX(const GameCollider& a, const GameObject& aObj, const GameCollider& b, const GameObject& bObj)
	{
		DirectX::XMFLOAT2 aPos = { a.position.x + aObj.position.x, a.position.y + aObj.position.y };
		DirectX::XMFLOAT2 bPos = { b.position.x + bObj.position.x, b.position.y + bObj.position.y };
		
		//AABB kollisionerkennung 
		if (aPos.x < bPos.x + b.size.x && aPos.x + a.size.x > bPos.x &&
			aPos.y < bPos.y + b.size.y - tolerance && aPos.y + a.size.y > bPos.y + tolerance) {
			float aCenter = aPos.x + a.size.x * 0.5f;
			float bCenter = bPos.x + b.size.x * 0.5f;

			// Differenz der Mittelpunkte in X-Richtung
			float dx = aCenter - bCenter;
			float halfWidth = (a.size.x + b.size.x) * 0.5f;

			// Berechne die Überlappung in X-Richtung
			float overlap = halfWidth - std::abs(dx);
			// Nur wenn sich das Objekt in die Richtung bewegt, sollte eine Kollision erkannt werden.
			if (aObj.physics.velocity.x > 0.0f && dx < 0.0f) return -overlap;
			else if (aObj.physics.velocity.x < 0.0f && dx > 0.0f) return overlap;
			else return 0.0f;
		}

		return 0.0f;
	}

	float GamePhysics::checkCollisionY(const GameCollider& a, const GameObject& aObj, const GameCollider& b, const GameObject& bObj)
	{
		DirectX::XMFLOAT2 aPos = { a.position.x + aObj.position.x, a.position.y + aObj.position.y };
		DirectX::XMFLOAT2 bPos = { b.position.x + bObj.position.x, b.position.y + bObj.position.y };

		//AABB kollisionerkennung 
		if (aPos.x < bPos.x + b.size.x - tolerance && aPos.x + a.size.x > bPos.x + tolerance &&
			aPos.y < bPos.y + b.size.y && aPos.y + a.size.y > bPos.y) {
			float aCenter = aPos.y + a.size.y * 0.5f;
			float bCenter = bPos.y + b.size.y * 0.5f;

			// Differenz der Mittelpunkte in Y-Richtung
			float dy = aCenter - bCenter;
			float halfWidth = (a.size.y + b.size.y) * 0.5f;

			// Berechne die Überlappung in Y-Richtung
			float overlap = halfWidth - std::abs(dy);
			// Nur wenn sich das Objekt in die Richtung bewegt, sollte eine Kollision erkannt werden.
			if (aObj.physics.velocity.y > 0.0f && dy < 0.0f) return -overlap;
			else if (aObj.physics.velocity.y < 0.0f && dy > 0.0f) return overlap;
			else return 0.0f;
		}

		return 0.0f;
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
			// Wenn das nächste Objekt statisch ist, sollte die Kette nicht verschoben werden können.
			if (obj->isStatic) canMove = false;
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

