#pragma once

#include "GameLevel.h"

#include "Graphics/RenderTarget.h"

namespace Game {

	struct GameSettings {
		std::filesystem::path levelPath;
		bool levelEditorMode = false;
		bool showColliders = false;
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

		bool m_GridLock = true;
		bool m_DrawMode = false;
		GameObject m_DrawObject;
	};

}