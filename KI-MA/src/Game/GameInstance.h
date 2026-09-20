#pragma once

#include "GameLevel.h"

#include "Graphics/RenderTarget.h"

namespace Core {
	class Application;
}

namespace Game {

	struct GameSettings {
		std::filesystem::path levelPath;
		bool levelEditorMode = false;
		bool showColliders = true;
	};

	class GameInstance {
	public:
		GameInstance(GameSettings settings = {});
		~GameInstance();

		void setGameSettings(GameSettings settings);

		void update(float deltaTime);

		void drawGUI();
		void render();

		Graphics::RenderTarget* getTarget() { return m_Target; };

	private:
		bool drawObjectProperties(GameObject& obj, bool pos, bool& textureSelectorOpen);

		bool m_CloseApplication = false;
		bool m_LevelSaved = true;
		GameSettings m_LastGameSettings;

		Graphics::RenderTarget* m_Target;

		GameSettings m_GameSettings;
		GameLevel m_GameLevel;

		bool m_Paused = false;

		DirectX::XMFLOAT2 m_CameraPosition = { 0, 0 };
		float m_CameraZoom = 1.0f;

		FollowPlayerAxis m_CameraFollowPlayer = FollowPlayerAxis::FollowPlayerBoth;

		DirectX::XMFLOAT2 m_MousePos = { 0, 0 };
		DirectX::XMFLOAT2 m_LastMousePos = { 0, 0 };
		DirectX::XMFLOAT2 m_MouseDelta = { 0, 0 };

		DirectX::XMINT2 m_MouseWheelDelta = { 0, 0 };

		constexpr static float m_Gravity = 9.81f * 96;

		uint32_t m_TextureSetID;

		friend class Core::Application;
		friend class GameTriggers;
		friend class GameEditor;
	};

}