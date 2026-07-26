#include "GameInstance.h"

#include "Core/Application.h"

#include "external/ImGui/ImGui.h"
#include "Core/Logger.h"
#include "Events/Callbacks.h"


namespace Game {

	GameInstance::GameInstance(GameSettings settings)
		: m_GameSettings(settings)
	{
		m_Target = new Graphics::RenderTarget({ 640, 360 });
		m_GameLevel.loadLevel(m_GameSettings.levelPath);

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::WindowCloseCallback>(Events::EventType::WINDOW_CLOSE, [this](Core::Window * window) {
			m_CloseApplication = true;
		});

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


	void GameInstance::drawGUI()
	{
		Core::Application* app = Core::Application::getApplication();
		Graphics::Renderer* renderer = app->getRenderer();

		if (m_GameSettings.levelEditorMode) {

			bool saveSettings = false;
			ImGui::Begin("Level Editor");

			static GameSettings currentSettings = m_GameSettings;

			std::string levelPathStr = currentSettings.levelPath.string();
			ImGui::InputText("Level Path", (char*)levelPathStr.c_str(), 256);
			currentSettings.levelPath = levelPathStr;
			ImGui::Checkbox("Level Editor Mode", &currentSettings.levelEditorMode);
			ImGui::Checkbox("Show Colliders", &currentSettings.showColliders);

			if (ImGui::Button("Save Settings")) {
				if (currentSettings.levelEditorMode != m_GameSettings.levelEditorMode || currentSettings.levelPath != m_GameSettings.levelPath) {
					saveSettings = true;
				}
			}


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
			mousePos = ImGui::GetMousePos();

			ImGui::End();


			//Berechne die Position der Maus relativ zum RenderTarget
			ImVec2 relMousePos = { mousePos.x - offset.x, mousePos.y - offset.y };

			static int16_t selectedIndex = -1;

			if (gameViewMouseClicked) {
				Core::Logger::Debug("{} {}", relMousePos.x, relMousePos.y);
				validIndex = 0;
				for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
					if (gameObjects[i].type != GameObjectType::Invalid) validIndex++;
					else continue;

					if(relMousePos.x >= gameObjects[i].position.x && relMousePos.x <= gameObjects[i].position.x + gameObjects[i].size.x &&
					   relMousePos.y >= gameObjects[i].position.y && relMousePos.y <= gameObjects[i].position.y + gameObjects[i].size.y) {
						selectedIndex = i;
						break;
					}
				}
			}


			ImGui::Begin("ObjectList");

			if (ImGui::Button("Add Object")) {
				GameObject defaultObj;
				defaultObj.type = GameObjectType::Background;
				defaultObj.position = { 200, 200 };
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
				DirectX::XMFLOAT2 oldPos = gameObjects[selectedIndex].position;
				DirectX::XMFLOAT2 oldSize = gameObjects[selectedIndex].size;
				uint32_t oldTex = gameObjects[selectedIndex].textureHandle;
				
				ImGui::SliderFloat2("Position", &gameObjects[selectedIndex].position.x, 0, 1280);
				ImGui::SliderFloat2("Size", &gameObjects[selectedIndex].size.x, 0, 1280);
				ImGui::InputInt("Texture Handle", (int*)&gameObjects[selectedIndex].textureHandle);

				if (oldPos.x != gameObjects[selectedIndex].position.x || oldPos.y != gameObjects[selectedIndex].position.y ||
					oldSize.x != gameObjects[selectedIndex].size.x || oldSize.y != gameObjects[selectedIndex].size.y ||
					oldTex != gameObjects[selectedIndex].textureHandle) {
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
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x / 2, m_Target->getSize().y / 2));
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

		renderer->drawRects();
		renderer->endRenderTarget(m_Target);

	}



}

