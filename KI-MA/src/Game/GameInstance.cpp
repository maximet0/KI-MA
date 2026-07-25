#include "GameInstance.h"

#include "Core/Application.h"

#include "external/ImGui/ImGui.h"

namespace Game {
	GameInstance::GameInstance(GameSettings settings)
		: m_GameSettings(settings)
	{
		m_Target = new Graphics::RenderTarget({ 1280, 720 });
	}

	GameInstance::~GameInstance()
	{
	}

	void GameInstance::update(float deltaTime)
	{
		
	}

	void GameInstance::drawGUI()
	{
		Core::Application* app = Core::Application::getApplication();
		Graphics::Renderer* renderer = app->getRenderer();

		if (m_GameSettings.levelEditorMode) {
			ImGui::Begin("Game");
			auto textureHandle = renderer->getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x / 2, m_Target->getSize().y / 2));
			ImGui::End();


			ImGui::Begin("ObjectList");

			if (ImGui::Button("Add Object")) {
				GameObject defaultObj;
				defaultObj.type = GameObjectType::Background;
				defaultObj.position = { 200, 200 };
				defaultObj.size = { 32, 32 };
				defaultObj.textureHandle = 1;


				m_GameLevel.addGameObject(defaultObj);
			}

			auto& gameObjects = m_GameLevel.getGameObjects();
			uint16_t objectCount = m_GameLevel.getGameObjectCount();

			static int16_t selectedIndex = -1;
			uint16_t validIndex = 0;
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

			ImGui::Begin("Object Properties");

			if (selectedIndex != -1) {
				ImGui::SliderFloat2("Position", &gameObjects[selectedIndex].position.x, 0, 1280);
				ImGui::SliderFloat2("Size", &gameObjects[selectedIndex].size.x, 0, 1280);
				ImGui::InputInt("Texture Handle", (int*)&gameObjects[selectedIndex].textureHandle);
			}

			ImGui::End();

		}
		else {
			ImGui::Begin("Game Instance");
			auto textureHandle = renderer->getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x / 2, m_Target->getSize().y / 2));
			ImGui::End();
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

