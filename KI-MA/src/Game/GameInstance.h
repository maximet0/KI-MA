#pragma once

#include "GameLevel.h"

#include "Graphics/RenderTarget.h"

namespace Game {

	struct GameSettings {
		std::filesystem::path levelPath;
		bool levelEditorMode = false;
		bool showColliders = true;
	};

	class GameInstance {
	public:
		GameInstance(GameSettings settings);
		~GameInstance();

		void setGameSettings(GameSettings settings);

		void update(float deltaTime);

		void drawGUI();
		void render();

		Graphics::RenderTarget* getTarget() { return m_Target; };

	private:
		bool drawObjectProperties(GameObject& obj, bool pos);

		bool m_CloseApplication = false;
		bool m_LevelSaved = true;
		GameSettings m_LastGameSettings;

		Graphics::RenderTarget* m_Target;

		GameSettings m_GameSettings;
		GameLevel m_GameLevel;

		GameLevel m_SavedLevel;

		bool m_SimulationMode = false;
		bool m_Paused = false;
		bool m_GridLock = true;
		bool m_DrawMode = false;
		GameObject m_DrawObject;

		DirectX::XMFLOAT2 m_CameraPosition = { 0, 0 };
		float m_CameraZoom = 1.0f;

		bool m_CameraFollowPlayer = true;


		DirectX::XMFLOAT2 m_MousePos = { 0, 0 };
		DirectX::XMFLOAT2 m_LastMousePos = { 0, 0 };
		DirectX::XMFLOAT2 m_MouseDelta = { 0, 0 };

		DirectX::XMINT2 m_MouseWheelDelta = { 0, 0 };

		constexpr static float m_Gravity = 9.81f * 64;
		friend class GameTriggers;
	};

}