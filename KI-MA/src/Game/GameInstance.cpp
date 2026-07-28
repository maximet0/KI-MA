#include "GameInstance.h"

#include "Core/Application.h"

#include "external/ImGui/ImGui.h"
#include "Core/Logger.h"
#include "Events/Callbacks.h"


namespace Game {

	GameInstance::GameInstance(GameSettings settings)
		: m_GameSettings(settings)
	{
		m_Target = new Graphics::RenderTarget({ 640, 360 }, {0.1, 0.3, 0.7, 1.0});
		m_GameLevel.loadLevel(m_GameSettings.levelPath);

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::WindowCloseCallback>(Events::EventType::WINDOW_CLOSE, [this](Core::Window * window) {
			m_CloseApplication = true;
		});

		m_DrawObject.type = GameObjectType::Background;
		m_DrawObject.position = { 0, 0 };
		m_DrawObject.size = { 32, 32 };
		m_DrawObject.textureHandle = 1;

		m_DrawObject.colliders[0].position = { 0, 0 };
		m_DrawObject.colliders[0].size = { 32, 32 };
		m_DrawObject.colliderCount = 1;


		ImGuiIO& io = ImGui::GetIO();
		io.ConfigWindowsMoveFromTitleBarOnly = true;
	}

	GameInstance::~GameInstance()
	{

	}

	void GameInstance::setGameSettings(GameSettings settings)
	{
		if (m_GameSettings.levelPath != settings.levelPath) {
			if (!m_LevelSaved) {
				ImGui::OpenPopup("Save Level");
			}
			else m_GameLevel.loadLevel(settings.levelPath);
		}
		m_LastGameSettings = m_GameSettings;
		m_GameSettings = settings;
	}

	void GameInstance::update(float deltaTime)
	{
		
	}

	bool GameInstance::drawObjectProperties(GameObject& obj, bool pos) {
		int modified = 0;


		if (pos) {
			if (ImGui::DragFloat2("Position", &obj.position.x, m_GridLock ? 8.0f : 1.0f) ) {
				modified += 1;
				if (m_GridLock) {
					obj.position.x = std::floor(obj.position.x / 32.0f) * 32.0f;
					obj.position.y = std::floor(obj.position.y / 32.0f) * 32.0f;
				}
				
			}
		}
		if (ImGui::DragFloat2("Size", &obj.size.x, m_GridLock ? 8.0f : 1.0f)) {
			modified += 1;
			if (m_GridLock) {
				obj.size.x = std::floor(obj.size.x / 32.0f) * 32.0f;
				obj.size.y = std::floor(obj.size.y / 32.0f) * 32.0f;
			}
		}
	
		modified += ImGui::InputInt("Texture Handle", (int*)&obj.textureHandle);
		if (ImGui::TreeNodeEx("Colliders", ImGuiTreeNodeFlags_DefaultOpen, "Colliders (%d)", obj.colliderCount)) {
			for (uint32_t i = 0; i < obj.colliderCount; i++) {
				if (ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Collider %d", i)) {
					modified += ImGui::DragFloat2("Position", &obj.colliders[i].position.x, 0.5f);
					modified += ImGui::DragFloat2("Size", &obj.colliders[i].size.x, 0.5f);
				}
			}
			ImGui::TreePop();
		}

		return modified > 0;
	}

	void GameInstance::drawGUI()
	{
		Core::Application* app = Core::Application::getApplication();
		Graphics::Renderer* renderer = app->getRenderer();

		if (m_GameSettings.levelEditorMode) {

			bool saveSettings = false;
			ImGui::Begin("Level Editor");

			static GameSettings currentSettings = m_GameSettings;

			std::string levelPathStr = currentSettings.levelPath.string();
			ImGui::Text("Level Editor");

			ImGui::Checkbox("Grid Lock", &m_GridLock);
			ImGui::Checkbox("Draw Mode", &m_DrawMode);
			if (m_DrawMode) {
				ImGui::Text("Draw Object Properties");
				drawObjectProperties(m_DrawObject, false);

			}
			ImGui::Separator();
			ImGui::Text("Game Settings");
			ImGui::InputText("Level Path", (char*)levelPathStr.c_str(), 256);
			currentSettings.levelPath = levelPathStr;
			ImGui::Checkbox("Level Editor Mode", &currentSettings.levelEditorMode);
			ImGui::Checkbox("Show Colliders", &currentSettings.showColliders);

			if (ImGui::Button("Save Settings")) {
				if (currentSettings.levelEditorMode != m_GameSettings.levelEditorMode || currentSettings.levelPath != m_GameSettings.levelPath || currentSettings.showColliders != m_GameSettings.showColliders) {
					saveSettings = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save Level")) {
				m_GameLevel.saveLevel(m_GameSettings.levelPath);
				m_LevelSaved = true;
			}

			ImGui::End();

			if (saveSettings) {
				setGameSettings(currentSettings);
				currentSettings = m_GameSettings;
			}

			ImVec2 mousePos;
			
			auto& gameObjects = m_GameLevel.getGameObjects();
			uint16_t objectCount = m_GameLevel.getGameObjectCount();

			uint16_t validIndex = 0;

			ImGui::Begin("Game");
			auto textureHandle = renderer->getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x, m_Target->getSize().y));
			ImVec2 offset = ImGui::GetItemRectMin();
			bool gameViewMouseClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
			bool gameViewMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
			mousePos = ImGui::GetMousePos();

			//Berechne die Position der Maus relativ zum RenderTarget
			ImVec2 relMousePos = { mousePos.x - offset.x, mousePos.y - offset.y };

			static int16_t selectedIndex = -1;
			int16_t clickedIndex = -1;
			if (gameViewMouseClicked) {
				Core::Logger::Debug("{} {}", relMousePos.x, relMousePos.y);
				validIndex = objectCount;
				for (int32_t i = maxGameObjects - 1; i >= 0 && validIndex >= 0; i--) {
					if (gameObjects[i].type != GameObjectType::Invalid) validIndex--;
					else continue;

					if (relMousePos.x >= gameObjects[i].position.x && relMousePos.x <= gameObjects[i].position.x + gameObjects[i].size.x &&
						relMousePos.y >= gameObjects[i].position.y && relMousePos.y <= gameObjects[i].position.y + gameObjects[i].size.y) {
						selectedIndex = i;
						clickedIndex = i;
						break;
					}
				}
			}

			if (m_DrawMode) {
				//ImGui::
				if (gameViewMouseDown && clickedIndex == -1) {
					m_DrawObject.position.x = mousePos.x - offset.x;
					m_DrawObject.position.y = mousePos.y - offset.y;

					if (m_GridLock) {
						m_DrawObject.position.x = std::floor(m_DrawObject.position.x / 32.0f) * 32.0f;
						m_DrawObject.position.y = std::floor(m_DrawObject.position.y / 32.0f) * 32.0f;
					}

					m_GameLevel.addGameObject(m_DrawObject);
					m_LevelSaved = false;
				}
			}

			ImGui::End();


			ImGui::Begin("ObjectList");

			if (ImGui::Button("Add Object")) {
				GameObject defaultObj;
				defaultObj.type = GameObjectType::Background;
				defaultObj.position = { 0, 0 };
				defaultObj.size = { 32, 32 };
				defaultObj.textureHandle = 1;

				defaultObj.colliders[0].position = { 0, 0 };
				defaultObj.colliders[0].size = { 32, 32 };
				defaultObj.colliderCount = 1;

				m_GameLevel.addGameObject(defaultObj);
				m_LevelSaved = false;
			}

			validIndex = 0;
			if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen, "GameObjects (%d)", objectCount)) {

				for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
					if (gameObjects[i].type != GameObjectType::Invalid) validIndex++;
					else continue;

					ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
					if (selectedIndex == i) nodeFlags |= ImGuiTreeNodeFlags_Selected;

					ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "gameObject", i);

					if (ImGui::IsItemClicked()) {
						selectedIndex = i;
					}
				}
				ImGui::TreePop();
			}

			ImGui::End();

			if (m_CloseApplication) {
				if (!m_LevelSaved) {
					ImGui::OpenPopup("Save Level");
				}
				else {
					app->exit();
				}
			}

			ImGui::Begin("Object Properties");

			if (selectedIndex != -1) {
				if (drawObjectProperties(gameObjects[selectedIndex], true)) {
					m_LevelSaved = false;
				}
			}

			ImGui::End();

			if (ImGui::BeginPopupModal("Save Level", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
				ImGui::Text("Unsaved Changes. Save?");
				ImGui::SetCursorPosX(ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * 3 - ImGui::GetStyle().ItemInnerSpacing.x * 6 - ImGui::CalcTextSize("Yes").x - ImGui::CalcTextSize("No").x - ImGui::CalcTextSize("Cancel").x);
				if (ImGui::Button("Yes")) {
					m_GameLevel.saveLevel(m_LastGameSettings.levelPath);
					if (!m_CloseApplication) m_GameLevel.loadLevel(m_GameSettings.levelPath);
					m_LevelSaved = true;
					if (m_CloseApplication) app->exit();
					ImGui::CloseCurrentPopup();
				};
				ImGui::SameLine();
				if (ImGui::Button("No")) {
					if (m_CloseApplication) app->exit();
					m_GameLevel.loadLevel(m_GameSettings.levelPath);
					m_LevelSaved = true;
					ImGui::CloseCurrentPopup();
				};
				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					setGameSettings(m_LastGameSettings);
					m_LevelSaved = false;
					m_CloseApplication = false;
					ImGui::CloseCurrentPopup();
					currentSettings = m_GameSettings;
				};

				ImGui::EndPopup();
			}

		}
		else {
			ImGui::Begin("Game Instance");
			auto textureHandle = renderer->getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x, m_Target->getSize().y));
			ImGui::End();

			if (m_CloseApplication) {
				app->exit();
			}
		}
	}

	void GameInstance::render()
	{
		Core::Application* application = Core::Application::getApplication();
		Graphics::Renderer* renderer = application->getRenderer();


		renderer->beginRenderTarget(m_Target);

		auto& gameObjects = m_GameLevel.getGameObjects();
		uint16_t objectCount = m_GameLevel.getGameObjectCount();

		uint16_t validIndex = 0;
		//Hintergrund -> Terrain -> Spieler //TODO: Z-Layer im Renderer
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if (gameObjects[i].type != GameObjectType::Invalid) validIndex++;
			else continue;

			if (gameObjects[i].type == GameObjectType::Background) {
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, gameObjects[i].textureHandle);
			}
		}

		validIndex = 0;
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if (gameObjects[i].type != GameObjectType::Invalid) validIndex++;
			else continue;

			if (gameObjects[i].type == GameObjectType::Terrain) {
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, gameObjects[i].textureHandle);
			}
		}

		validIndex = 0;
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if (gameObjects[i].type != GameObjectType::Invalid) validIndex++;
			else continue;

			if (gameObjects[i].type == GameObjectType::Player) {
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, gameObjects[i].textureHandle);
			}
		}

		if (m_GameSettings.levelEditorMode) {
			for (int x = 0; x < 1280; x += 32) {
				renderer->submitLine({ (float)x, 0 }, { (float)x, 720 }, 0x6F829440);
			}
			for (int y = 0; y < 720; y += 32) {
				renderer->submitLine({ 0, (float)y }, { 1280, (float)y }, 0x6F829440);
			}
		}

		validIndex = 0;
		if (m_GameSettings.showColliders) {
			for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
				if (gameObjects[i].type != GameObjectType::Invalid) validIndex++;
				GameCollider* colliders = gameObjects[i].colliders.data();
				for (uint8_t j = 0; j < gameObjects[i].colliderCount; j++) {
					renderer->submitLine({ gameObjects[i].position.x + colliders[j].position.x, gameObjects[i].position.y + colliders[j].position.y }, { gameObjects[i].position.x + colliders[j].position.x + colliders[j].size.x, gameObjects[i].position.y + colliders[j].position.y }, 0x00FF00FF);
					renderer->submitLine({ gameObjects[i].position.x + colliders[j].position.x + colliders[j].size.x, gameObjects[i].position.y + colliders[j].position.y }, { gameObjects[i].position.x + colliders[j].position.x + colliders[j].size.x, gameObjects[i].position.y + colliders[j].position.y + colliders[j].size.y }, 0x00FF00FF);
					renderer->submitLine({ gameObjects[i].position.x + colliders[j].position.x + colliders[j].size.x, gameObjects[i].position.y + colliders[j].position.y + colliders[j].size.y }, { gameObjects[i].position.x + colliders[j].position.x, gameObjects[i].position.y + colliders[j].position.y + colliders[j].size.y }, 0x00FF00FF);
					renderer->submitLine({ gameObjects[i].position.x + colliders[j].position.x, gameObjects[i].position.y + colliders[j].position.y + colliders[j].size.y }, { gameObjects[i].position.x + colliders[j].position.x, gameObjects[i].position.y + colliders[j].position.y }, 0x00FF00FF);
				}
			}
		}

		//renderer->submitLine({ 0, 0 }, { 1280, 720 }, 0xFFFFFFFF);

		renderer->drawRects();
		renderer->drawLines();

		renderer->endRenderTarget(m_Target);

	}



}

