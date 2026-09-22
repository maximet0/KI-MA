#pragma once
#include <unordered_map>
#include "GameObject.h"

#include "GameInstance.h"

namespace Game {
	class GameInstance;

	class GameEditor {
	public:
		GameEditor();

		void update(float deltaTime);
		
		void drawGUI();
		void render();


	private:	
		void drawGameView();
		void drawObjectList();
		void drawDebugInfo();
		void drawProperties();
		void drawContentBrowser();

		void drawEditorSettings(bool& open);

		bool drawObjectProperties(GameObject& obj, bool pos, bool& textureSelectorOpen);

		void renderGameLevelPreview(Graphics::RenderTarget* target, DirectX::XMFLOAT2 cameraPos, float cameraZoom, GameLevel& level);

		uint32_t m_EditorTextureSetID = 0;

		std::filesystem::path m_ContentFolderPath;
		std::filesystem::path m_CurrentContentFolderPath;

		std::vector<std::filesystem::path> m_ThumbnailGenPaths;

		GameInstance m_GameInstance;

		GameObject m_DragPreviewObject;

		GameObject m_DrawObject;
		GameObject m_SelectedObjectTemplate;
		std::filesystem::path m_SelectedTemplatePath;

		ObjectID m_LastClickedObject;

		bool m_DrawEnabled = false;
		bool m_GridLock = true;
		bool m_ShowColliders = true;

		bool m_SimulationMode = false;
		bool m_ActiveLevelSaved = true;

		enum class SelectionType {
			None,
			LevelObject,
			TemplateObject,
			Level
		};
	
		SelectionType m_SelectionType = SelectionType::None;
		void* m_Selection = nullptr;

		DirectX::XMFLOAT2 m_MousePos = { 0, 0 };
		DirectX::XMFLOAT2 m_LastMousePos = { 0, 0 };
		DirectX::XMFLOAT2 m_MouseDelta = { 0, 0 };

		DirectX::XMINT2 m_MouseWheelDelta = { 0, 0 };

		std::unordered_map<std::filesystem::path, uint32_t> m_ThumbnailCache;
	};
}

