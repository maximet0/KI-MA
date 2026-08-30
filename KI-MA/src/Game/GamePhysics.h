#pragma once

#include "GameObject.h"
#include "GameLevel.h"

namespace Game {

	struct RaycastHit {
		GameObject* object = nullptr;
		bool hit = false;
		DirectX::XMFLOAT2 hitPoint = { 0, 0 };
	};

	struct CollisionResult {
		bool collided = false;
		DirectX::XMFLOAT2 overlap = { 0, 0 };
	};


	class GamePhysics {
	public:
		static RaycastHit boxcast(GameLevel& level, const DirectX::XMFLOAT2& origin, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT2& direction, float maxDistance);
		static RaycastHit raycast(GameLevel& level, const DirectX::XMFLOAT2& origin, const DirectX::XMFLOAT2& direction, float maxDistance);

		static void updatePhysics(GameLevel& level, float deltaTime, float gravity);
	private:
		
		static void resolveAxis(GameLevel& level, GameObject& a, float deltaTime, bool isYAxis, bool& noneCollided, bool& noneCollidedWithGround);
		//static bool checkCollision(const GameCollider& a, const GameObject& aObj, const GameCollider& b, const GameObject& bObj);

		/// <summary>
		/// Gibt die Überlappung in X-Richtung zurück, wenn eine Kollision vorliegt. Ansonsten 0.
		/// </summary>
		/// <param name="a"></param>
		/// <param name="b"></param>
		/// <returns></returns>
		static CollisionResult checkCollision(const GameCollider& a, const GameObject& aObj, const GameCollider& b, const GameObject& bObj, bool yAxis);


		/// <summary>
		/// Rekursive Funktion, die prüft, ob eine Kette von Objekten verschoben werden kann und diese Verschiebung auf Objekte in der Kette anwendet
		/// </summary>
		/// <param name="level"></param>
		/// <param name="originObj"></param>
		/// <param name="direction"></param>
		/// <param name="totalMass"></param>
		/// <param name="canMove"></param>
		static void checkPushableRec(GameLevel& level, GameObject& originObj, const DirectX::XMFLOAT2& direction, float firstMass, float& totalMass, bool& canMove);

	};
}