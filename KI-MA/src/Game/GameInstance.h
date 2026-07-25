#pragma once

#include "GameLevel.h"

#include "Graphics/RenderTarget.h"

namespace Game {

	struct GameSettings {
		std::filesystem::path levelPath;
		bool levelEditorMode = false;

	};

	class GameInstance {
	public:
		GameInstance(GameSettings settings);
		~GameInstance();

		void setGameSettings(const GameSettings& settings) { m_GameSettings = settings; }

		void update(float deltaTime);

		void drawGUI();
		void render();

		Graphics::RenderTarget* getTarget() { return m_Target; };

	private:
		Graphics::RenderTarget* m_Target;

		GameSettings m_GameSettings;
		GameLevel m_GameLevel;
	};

}