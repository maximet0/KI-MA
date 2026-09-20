#include "GameEditor.h"

#include "Core/Logger.h"
#include "Core/Application.h"
#include "Events/Callbacks.h"

#include "Graphics/TextureManager.h"

#include "external/ImGui/imgui.h"
#include "external/ImGui/misc/cpp/imgui_stdlib.h"

namespace Game {


	GameEditor::GameEditor()
		: m_GameInstance()
	{
		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::MousePosCallback>(Events::EventType::MOUSE_POS, [this](Core::Window* window, DirectX::XMINT2 mousePos, DirectX::XMINT2 aMousePos) {
			m_MousePos.x = mousePos.x;
			m_MousePos.y = mousePos.y;
			});

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::MouseWheelCallback>(Events::EventType::MOUSE_WHEEL, [this](Core::Window* window, DirectX::XMINT2 wheelDelta) {
			m_MouseWheelDelta.x += wheelDelta.x;
			m_MouseWheelDelta.y += wheelDelta.y;
			});

		HKEY hKey;
		LPCWSTR subKey = L"Software\\KI-MA\\EditorSettings";

		HRESULT result = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, nullptr, &hKey, nullptr);

		DWORD size = 0;
		DWORD type = 0;

		result = RegQueryValueExW(hKey, L"ContentBrowserPath", nullptr, &type, nullptr, &size);

		if (result == ERROR_FILE_NOT_FOUND) {
			std::wstring defaultPath = std::filesystem::current_path().wstring();
			RegSetValueExW(hKey, L"ContentBrowserPath", 0, REG_SZ, (const BYTE*)defaultPath.c_str(), (DWORD)(defaultPath.size() + 1) * sizeof(WCHAR));
			size = (DWORD)(defaultPath.size() + 1) * sizeof(WCHAR);
			type = REG_SZ;
		}

		std::wstring contentBrowserPathW(size / sizeof(WCHAR), L'\0');

		RegQueryValueExW(hKey, L"ContentBrowserPath", nullptr, nullptr, (LPBYTE)contentBrowserPathW.data(), &size);

		m_ContentFolderPath = std::filesystem::path(contentBrowserPathW.c_str());
		m_CurrentContentFolderPath = m_ContentFolderPath;
		RegCloseKey(hKey);
	}


	void GameEditor::update(float deltaTime)
	{
		static bool firstUpdate = true;
		if (firstUpdate) {
			// Editor Texturen erstellen
			Graphics::TextureManager& texMan = Core::Application::getApplication()->getRenderer()->getTextureManager();

			m_EditorTextureSetID = texMan.createTextureSet("Editor");

			uint32_t whiteTextureData = { 0xFFFFFFFF };
			texMan.beginEarlyTextureLoad();
			uint32_t whiteTextureID = texMan.loadTextureFromMemory((const char*)&whiteTextureData, 1, 1, 4);
			texMan.addTextureToSet(m_EditorTextureSetID, "WHITE", whiteTextureID);


			texMan.endEarlyTextureLoad();
			firstUpdate = false;
		}

		auto renderer = Core::Application::getApplication()->getRenderer();

		if (m_SimulationMode) {
			m_GameInstance.update(deltaTime);
		}
	
	}

	void GameEditor::drawGUI()
	{
		auto renderer = Core::Application::getApplication()->getRenderer();

		if (!m_SimulationMode) m_GameInstance.m_CameraFollowPlayer = FollowPlayerAxis::FollowPlayerNone;

		static bool editorSettingsOpen = false;

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Editor Settings")) {
					editorSettingsOpen = !editorSettingsOpen;
				}
				ImGui::Separator();
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		drawEditorSettings(editorSettingsOpen);

		ImGui::Begin("Level Editor");
		static std::string newPath = "";

		static GameSettings currentSettings = m_GameInstance.m_GameSettings;
		std::string levelPathStr = currentSettings.levelPath.string();

		ImGui::Text("Instance Settings");
		ImGui::InputText("Level Path", &levelPathStr);
		currentSettings.levelPath = levelPathStr;
		ImGui::Checkbox("Show Colliders", &m_ShowColliders);

		if (ImGui::Button("Save Settings")) {
			if (currentSettings.levelEditorMode != m_GameInstance.m_GameSettings.levelEditorMode || currentSettings.levelPath != m_GameInstance.m_GameSettings.levelPath) {
				m_GameInstance.setGameSettings(currentSettings);
				//saveSettings = true;
			}
		}

		ImGui::SameLine();

		if (m_SimulationMode) ImGui::BeginDisabled();

		if (ImGui::Button("Save Level")) {
			m_GameInstance.m_GameLevel.saveLevel(m_GameInstance.m_GameSettings.levelPath);
			m_ActiveLevelSaved = true;
		}

		if (m_SimulationMode) ImGui::EndDisabled();

		ImGui::End();


		drawGameView();
		drawContentBrowser();

		drawObjectList();
		drawDebugInfo();

		drawProperties();



		m_MouseDelta.x = m_MousePos.x - m_LastMousePos.x;
		m_MouseDelta.y = m_MousePos.y - m_LastMousePos.y;
		m_LastMousePos = m_MousePos;

		m_MouseWheelDelta.x = 0;
		m_MouseWheelDelta.y = 0;

	}

	void GameEditor::render()
	{
		Graphics::Renderer* renderer = Core::Application::getApplication()->getRenderer();

		Graphics::RenderTarget* target = m_GameInstance.getTarget();
		renderer->beginRenderTarget(target, m_GameInstance.m_CameraPosition, 1.0f / m_GameInstance.m_CameraZoom);


		m_GameInstance.render();


		uint32_t previewTextureHandle = renderer->getTextureManager().getTextureIDFromSet(m_GameInstance.m_TextureSetID, m_DragPreviewObject.textureName);
		renderer->submitRect(m_DragPreviewObject.position, 1.0f, m_DragPreviewObject.size, previewTextureHandle, m_DragPreviewObject.repeatTexture);

		constexpr float gridSize = 32.0f;
		constexpr float invGridSize = 1.0f / gridSize;
		constexpr float gridSize2x = gridSize * 2;

		float halfWidth = target->getSize().x * 0.5f * m_GameInstance.m_CameraZoom;
		float halfHeight = target->getSize().y * 0.5f * m_GameInstance.m_CameraZoom;

		float left = m_GameInstance.m_CameraPosition.x - halfWidth;
		float right = m_GameInstance.m_CameraPosition.x + halfWidth;
		float top = m_GameInstance.m_CameraPosition.y - halfHeight;
		float bottom = m_GameInstance.m_CameraPosition.y + halfHeight;

		float startX = std::floor(left / gridSize) * gridSize;
		float startY = std::floor(top / gridSize) * gridSize;

		for (int32_t x = startX; x <= right; x += gridSize) {
			renderer->submitLine({ (float)x, (float)top }, { (float)x, (float)bottom }, 0x6F829440);
		}

		for (int32_t y = startY; y <= bottom; y += gridSize) {
			renderer->submitLine({ (float)right, (float)y }, { (float)left, (float)y }, 0x6F829440);
		}

		if (m_ShowColliders) {
			for (auto& obj : m_GameInstance.m_GameLevel.getGameObjects()) {
				uint32_t color = 0x00FF00FF;
				if ((obj.flags & GameObjectFlags::Trigger) != 0) color = 0xFF0000FF;
				if ((obj.flags & GameObjectFlags::Static) == 0) color = 0x0000FFFF;
				renderer->submitLine({ obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y }, { obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y }, color);
				renderer->submitLine({ obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y }, { obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, color);
				renderer->submitLine({ obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, { obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, color);
				renderer->submitLine({ obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, { obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y }, color);
			}
		}

		if (m_SelectionType == SelectionType::LevelObject) {
			GameObject& selectedObj = m_GameInstance.m_GameLevel.getGameObject((ObjectID)m_Selection);
		
			uint32_t sizeX = (std::max)((uint32_t)selectedObj.size.x, 1u);
			uint32_t sizeY = (std::max)((uint32_t)selectedObj.size.y, 1u);
			
			renderer->submitLine({ selectedObj.position.x, selectedObj.position.y }, { selectedObj.position.x + sizeX, selectedObj.position.y }, 0xFFFFFFFF);
			renderer->submitLine({ selectedObj.position.x + sizeX, selectedObj.position.y }, { selectedObj.position.x + sizeX, selectedObj.position.y + sizeY }, 0xFFFFFFFF);
			renderer->submitLine({ selectedObj.position.x + sizeX, selectedObj.position.y + sizeY }, { selectedObj.position.x, selectedObj.position.y + sizeY }, 0xFFFFFFFF);
			renderer->submitLine({ selectedObj.position.x, selectedObj.position.y + sizeY }, { selectedObj.position.x, selectedObj.position.y }, 0xFFFFFFFF);

			float handleSize = (std::min)(8.0f, 8.0f * m_GameInstance.m_CameraZoom);
			renderer->submitRect({ selectedObj.position.x - handleSize * 0.5f, selectedObj.position.y + sizeY * 0.5f - handleSize * 0.5f }, 1.0f, { handleSize, handleSize }, renderer->getTextureManager().getTextureIDFromSet(m_EditorTextureSetID, "WHITE"), false);
			renderer->submitRect({ selectedObj.position.x + sizeX * 0.5f - handleSize * 0.5f, selectedObj.position.y - handleSize * 0.5f }, 1.0f, { handleSize, handleSize }, renderer->getTextureManager().getTextureIDFromSet(m_EditorTextureSetID, "WHITE"), false);
			renderer->submitRect({ selectedObj.position.x + sizeX - handleSize * 0.5f, selectedObj.position.y + sizeY * 0.5f - handleSize * 0.5f }, 1.0f, { handleSize, handleSize }, renderer->getTextureManager().getTextureIDFromSet(m_EditorTextureSetID, "WHITE"), false);
			renderer->submitRect({ selectedObj.position.x + sizeX * 0.5f - handleSize * 0.5f, selectedObj.position.y + sizeY - handleSize * 0.5f }, 1.0f, { handleSize, handleSize }, renderer->getTextureManager().getTextureIDFromSet(m_EditorTextureSetID, "WHITE"), false);
		}

		renderer->drawRects();
		renderer->drawLines();

		renderer->endRenderTarget(target);

		for (auto& path : m_ThumbnailGenPaths) {
			Graphics::RenderTarget target ({ 128, 128 }, {0.1, 0.3, 0.7, 1.0});
			GameLevel level;
			level.loadLevel(path);
			renderGameLevelPreview(&target, { 0, 0 }, 0.25f, level);

			std::ofstream thumbFile(path.string() + ".thumb", std::ios::binary);
			uint32_t* data = new uint32_t[128 * 128];
			target.copyToCpuBuffer(data, sizeof(uint32_t) * 128 * 128);
			Core::Logger::Debug("Generated thumbnail for level: {}", path.string());
			thumbFile.write(reinterpret_cast<char*>(data), sizeof(uint32_t) * 128 * 128);
			thumbFile.close();
			delete[] data;
		}

		m_ThumbnailGenPaths.clear();

	}

	void GameEditor::drawGameView()
	{
		auto renderer = Core::Application::getApplication()->getRenderer();

		ImGui::Begin("Game");
		if (m_SimulationMode) {
			if (ImGui::Button(m_GameInstance.m_Paused ? "Resume Simulation" : "Pause Simulation")) {
				m_GameInstance.m_Paused = !m_GameInstance.m_Paused;
			}

			ImGui::SameLine();

			if (ImGui::Button("Reset Simulation")) {
				m_GameInstance.m_GameLevel.loadLevel("tmpSimSave.lvl");
				m_GameInstance.m_CameraPosition = { 0, 0 };
				m_GameInstance.m_CameraFollowPlayer = FollowPlayerAxis::FollowPlayerBoth;
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop Simulation")) {
				m_GameInstance.m_GameLevel.loadLevel("tmpSimSave.lvl");
				m_SimulationMode = false;
				m_GameInstance.m_CameraPosition = { 0, 0 };
				m_GameInstance.m_CameraFollowPlayer = FollowPlayerAxis::FollowPlayerNone;
			}
		}
		else {
			if (ImGui::Button("Start Simulation")) {
				m_GameInstance.m_GameLevel.saveLevel("tmpSimSave.lvl", true);
				m_SimulationMode = true;
				m_GameInstance.m_CameraFollowPlayer = FollowPlayerAxis::FollowPlayerBoth;
			}
		}

		ImGui::SameLine();

		ImGui::Checkbox("Draw ObjTemplate", &m_DrawEnabled);

		float aspectRatio = (float)m_GameInstance.getTarget()->getSize().x / (float)m_GameInstance.getTarget()->getSize().y;

		uint32_t height = ImGui::GetContentRegionAvail().y;

		uint32_t width = static_cast<uint32_t>(height * aspectRatio);
		
		if (ImGui::GetContentRegionAvail().x < width) {
			width = ImGui::GetContentRegionAvail().x;
			height = static_cast<uint32_t>(width / aspectRatio);
		}

		float scaleRatio = (float)m_GameInstance.getTarget()->getSize().x / (float)width;


		
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);

		auto textureHandle = renderer->getTextureManager().getSRVGPUDescriptorHandle(m_GameInstance.getTarget()->getSRVDescriptorIndex());
		ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(width, height));
		DirectX::XMFLOAT2 offset = { ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
		bool gameViewLeftMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
		bool gameViewRightMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right);

		drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);


		//Berechne die Position der Maus relativ zum RenderTarget
		//DirectX::XMFLOAT2 relMousePos = { m_MousePos.x - offset.x, m_MousePos.y - offset.y };

		//Berechne die Position in der Welt
		DirectX::XMVECTOR worldPosVec = DirectX::XMLoadFloat2(&m_MousePos);

		worldPosVec = DirectX::XMVectorSubtract(worldPosVec, DirectX::XMLoadFloat2(&offset));
		worldPosVec = DirectX::XMVectorSubtract(worldPosVec, DirectX::XMVectorSet(width * 0.5f, height * 0.5f, 0.0f, 0.0f));
		worldPosVec = DirectX::XMVectorScale(worldPosVec, scaleRatio);
		worldPosVec = DirectX::XMVectorScale(worldPosVec, m_GameInstance.m_CameraZoom);
		worldPosVec = DirectX::XMVectorAdd(worldPosVec, DirectX::XMLoadFloat2(&m_GameInstance.m_CameraPosition));

		DirectX::XMFLOAT2 worldPos;
		DirectX::XMStoreFloat2(&worldPos, worldPosVec);


		ObjectID clickedID = 0;
		if (gameViewLeftMouseDown || gameViewRightMouseDown) {
			m_LastClickedObject = 0;
			for (uint32_t i = m_GameInstance.m_GameLevel.getGameObjects().size(); i > 0; i--) {

				GameObject& obj = m_GameInstance.m_GameLevel.getGameObjects().at(i-1);
				if ((worldPos.x >= obj.position.x && worldPos.x <= obj.position.x + obj.size.x &&
					worldPos.y >= obj.position.y && worldPos.y <= obj.position.y + obj.size.y) ||
					(worldPos.x >= obj.position.x + obj.collider.offset.x && worldPos.x <= obj.position.x + obj.collider.offset.x + obj.collider.size.x &&
					worldPos.y >= obj.position.y + obj.collider.offset.y && worldPos.y <= obj.position.y + obj.collider.offset.y + obj.collider.size.y)) {
					if (gameViewLeftMouseDown) {
						m_Selection = (void*)(obj.id);
						m_SelectionType = SelectionType::LevelObject;
					}
					clickedID = obj.id;
					m_LastClickedObject = obj.id;
					break;
				}
			}
		}

		static bool cameraDrag = false;


		if (!cameraDrag && ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems)) {
			if (m_LastClickedObject != 0) {
				if (ImGui::MenuItem("Delete Object")) {
					m_GameInstance.m_GameLevel.removeGameObject(m_LastClickedObject);
					m_LastClickedObject = 0;
					m_Selection = nullptr;
					m_SelectionType = SelectionType::None;
					m_ActiveLevelSaved = false;
				}
			}
			else {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && gameViewRightMouseDown) cameraDrag = true;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) cameraDrag = false;

		if (cameraDrag) {
			m_GameInstance.m_CameraPosition.x -= m_MouseDelta.x * m_GameInstance.m_CameraZoom;
			m_GameInstance.m_CameraPosition.y -= m_MouseDelta.y * m_GameInstance.m_CameraZoom;
		}

		if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsItemHovered() && m_MouseWheelDelta.y != 0.0f) {
			m_GameInstance.m_CameraZoom -= m_MouseWheelDelta.y * 0.1f;
			if (m_GameInstance.m_CameraZoom < 0.1f) m_GameInstance.m_CameraZoom = 0.1f;
			if (m_GameInstance.m_CameraZoom > 10.0f) m_GameInstance.m_CameraZoom = 10.0f;
		}


		if (m_DrawEnabled) {
			if (gameViewLeftMouseDown && clickedID == 0) {

				m_SelectedObjectTemplate.position.x = worldPos.x;
				m_SelectedObjectTemplate.position.y = worldPos.y;

				if (m_GridLock) {
					m_SelectedObjectTemplate.position.x = std::floor(m_SelectedObjectTemplate.position.x / 32.0f) * 32.0f;
					m_SelectedObjectTemplate.position.y = std::floor(m_SelectedObjectTemplate.position.y / 32.0f) * 32.0f;
				}

				m_GameInstance.m_GameLevel.addGameObject(m_SelectedObjectTemplate);
				m_ActiveLevelSaved = false;
			}
			if (gameViewRightMouseDown && clickedID != 0 && !cameraDrag) {
				m_GameInstance.m_GameLevel.removeGameObject(clickedID);
				if ((ObjectID)m_Selection == clickedID) {
					m_Selection = nullptr;
					m_SelectionType = SelectionType::None;
				}
				m_LastClickedObject = 0;
				m_ActiveLevelSaved = false;
			}
		}


		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OBJECT_TEMPLATE", ImGuiDragDropFlags_AcceptBeforeDelivery);
			if (payload != nullptr) {

				m_DragPreviewObject = *(GameObject*)payload->Data;
				m_DragPreviewObject.position.x = worldPos.x;
				m_DragPreviewObject.position.y = worldPos.y;

				if (m_GridLock) {
					m_DragPreviewObject.position.x = std::floor(m_DragPreviewObject.position.x / 32.0f) * 32.0f;
					m_DragPreviewObject.position.y = std::floor(m_DragPreviewObject.position.y / 32.0f) * 32.0f;
				}

				if (payload->IsDelivery()) {
					m_GameInstance.m_GameLevel.addGameObject(m_DragPreviewObject);

					m_DragPreviewObject.position.x = -99999999;
					m_DragPreviewObject.position.y = -99999999;

				}
			}

			payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_LEVEL", ImGuiDragDropFlags_AcceptBeforeDelivery);

			if (payload != nullptr) {
				if (payload->IsDelivery()) {
					const char* levelPathStr = (const char*)payload->Data;

					std::filesystem::path levelPath(levelPathStr);
					m_GameInstance.m_GameLevel.loadLevel(levelPath);
					m_GameInstance.m_GameSettings.levelPath = levelPath;
					m_ActiveLevelSaved = true;
				}
			}


			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void GameEditor::drawObjectList()
	{
		ImGui::Begin("ObjectList");
		if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "GameObjects (%d)", m_GameInstance.m_GameLevel.getGameObjectCount())) {

			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Save Level")) {
					m_GameInstance.m_GameLevel.saveLevel(m_GameInstance.m_GameSettings.levelPath);
					m_ActiveLevelSaved = true;
				}
				ImGui::EndPopup();
			}

			for (auto& obj : m_GameInstance.m_GameLevel.getGameObjects()) {
				ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (m_SelectionType == SelectionType::LevelObject && (ObjectID)m_Selection == obj.id) nodeFlags |= ImGuiTreeNodeFlags_Selected;

				ImGui::TreeNodeEx((void*)(intptr_t)obj.id, nodeFlags, obj.objectName.c_str(), obj.id);

				if (ImGui::IsItemClicked()) {
					m_Selection = (void*)(obj.id);
					m_SelectionType = SelectionType::LevelObject;
				}

				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Delete Object")) {
						m_GameInstance.m_GameLevel.removeGameObject(obj.id);
						if (m_SelectionType == SelectionType::LevelObject && (ObjectID)m_Selection == obj.id) {
							m_Selection = nullptr;
							m_SelectionType = SelectionType::None;
						}
						m_ActiveLevelSaved = false;
					}
					ImGui::EndPopup();
				}
			}

			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
				if (ImGui::MenuItem("Add Object")) {
					GameObject defaultObj;
					defaultObj.flags = GameObjectFlags::Static;
					defaultObj.position = { 0, 0 };
					defaultObj.size = { 32, 32 };
					defaultObj.textureName = "";
					defaultObj.collider.offset = { 0, 0 };
					defaultObj.collider.size = { 32, 32 };
					m_GameInstance.m_GameLevel.addGameObject(defaultObj);
					m_ActiveLevelSaved = false;
				}
				ImGui::EndPopup();
			}

			ImGui::TreePop();
		}
		ImGui::End();
	}

	void GameEditor::drawDebugInfo()
	{
		static ObjectID playerID = 0;
		if (!(m_GameInstance.m_GameLevel.getGameObject(playerID).flags & GameObjectFlags::Player)) {
			for (auto& obj : m_GameInstance.m_GameLevel.getGameObjects()) {
				if (obj.flags & GameObjectFlags::Player) {
					playerID = obj.id;
					break;
				}
			}
		}

		ImGui::Begin("Debug Info");
		ImGui::Text("Game Info: ");
		ImGui::Text("	Score: %d", m_GameInstance.m_GameLevel.getScore());
		ImGui::Text("Player Info:");
		ImGui::Text("	Player Lives: %d", m_GameInstance.m_GameLevel.getPlayerLives());
		ImGui::Text("	Player Position: (%.2f, %.2f)", m_GameInstance.m_GameLevel.getGameObject(playerID).position.x, m_GameInstance.m_GameLevel.getGameObject(playerID).position.y);
		
		ImGui::Text("	Player Velocity [Group]: (%.2f, %.2f) [(%.2f, %.2f)]", m_GameInstance.m_GameLevel.getGameObject(playerID).physics.velocity.x, m_GameInstance.m_GameLevel.getGameObject(playerID).physics.velocity.y, m_GameInstance.m_GameLevel.getGameObject(playerID).physics.groupVelocity.x, m_GameInstance.m_GameLevel.getGameObject(playerID).physics.groupVelocity.y);
		ImGui::Text("	Player Acceleration: (%.2f, %.2f)", m_GameInstance.m_GameLevel.getGameObject(playerID).physics.acceleration.x, m_GameInstance.m_GameLevel.getGameObject(playerID).physics.acceleration.y);
		ImGui::Text("	Player GroupID: %d", m_GameInstance.m_GameLevel.getGameObject(playerID).triggerGroup);

		ImGui::Text("Mouse Info:");
		ImGui::Text("	Mouse Position: (%.2f, %.2f)", m_MousePos.x, m_MousePos.y);
		ImGui::Text("	Mouse Delta: (%.2f, %.2f)", m_MouseDelta.x, m_MouseDelta.y);
		ImGui::Text("Camera Info:");
		ImGui::Text("	Camera Position: (%.2f, %.2f)", m_GameInstance.m_CameraPosition.x, m_GameInstance.m_CameraPosition.y);
		ImGui::Text("	Camera Zoom: %.2f", m_GameInstance.m_CameraZoom);
		ImGui::End();
	
	}

	bool drawTextureEntry(Graphics::TextureSetEntry& tex, std::string textureName, uint32_t setID) {
		bool modified = false;
		Core::Application* app = Core::Application::getApplication();
		Graphics::TextureManager& textureManager = app->getRenderer()->getTextureManager();
		static char newPath[256] = "";

		ImGui::PushID(tex.textureName.c_str());

		const ImGuiStyle& style = ImGui::GetStyle();

		float width = ImGui::GetContentRegionAvail().x;
		ImVec2 ItemPos = ImGui::GetCursorScreenPos();

		if (ImGui::InvisibleButton("##InvisibleButton", ImVec2(width, 32 + style.FramePadding.y * 2))) {
			modified = true;
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImColor col = ImGui::GetColorU32(ImGuiCol_Header);
		if (ImGui::IsItemHovered()) {
			col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
		}
		else if (tex.textureName == textureName) {
			col = ImGui::GetColorU32(ImGuiCol_HeaderActive);
		}

		drawList->AddRectFilled(ItemPos, ImVec2(ItemPos.x + width, ItemPos.y + 32 + style.FramePadding.y * 2), col);

		ItemPos.x += style.FramePadding.x;
		ItemPos.y += style.FramePadding.y;

		drawList->AddImage((ImTextureID)textureManager.getSRVGPUDescriptorHandle(tex.textureID).ptr, ItemPos, ImVec2(ItemPos.x + 32, ItemPos.y + 32));

		drawList->AddText(ImVec2(ItemPos.x + 40, ItemPos.y + 8), IM_COL32(255, 255, 255, 255), tex.textureName.c_str());

		bool rename = false;
		bool pathChange = false;

		if (ImGui::BeginPopupContextItem("##context")) {
			if (ImGui::MenuItem("Rename"))
			{
				rename = true;
				newPath[0] = '\0';
			}

			if (ImGui::MenuItem("Change Path"))
			{
				pathChange = true;
				newPath[0] = '\0';
			}

			if (ImGui::MenuItem("Delete"))
			{
				modified = true;
				textureManager.removeTextureFromSet(setID, tex.textureName);
			}

			ImGui::EndPopup();
		}

		if (rename) ImGui::OpenPopup("Rename Texture");
		else if (pathChange) ImGui::OpenPopup("Replace Texture");

		if (ImGui::BeginPopup("Replace Texture")) {
			if (newPath[0] == '\0') strcpy_s(newPath, sizeof(newPath), tex.texturePath.string().c_str());
			ImGui::InputText("New Texture Path", newPath, 256);

			if (ImGui::Button("Replace")) {
				textureManager.modifyTextureInSet(setID, tex.textureName, tex.textureName, std::filesystem::path(newPath));
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Rename Texture")) {
			if (newPath[0] == '\0') strcpy_s(newPath, sizeof(newPath), tex.textureName.c_str());
			ImGui::InputText("New Name", &tex.textureName);

			if (ImGui::Button("Rename")) {
				textureManager.modifyTextureInSet(setID, tex.textureName, std::string(newPath), tex.texturePath);
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
		return modified;
	}

	bool drawTextureSelector(uint32_t& setID, GameObject& obj, bool& open) {
		bool modified = false;
		Core::Application* app = Core::Application::getApplication();
		Graphics::TextureManager& textureManager = app->getRenderer()->getTextureManager();

		ImGui::Text("Texture %s", obj.textureName.c_str());
		ImGui::SameLine();

		if (ImGui::Button("Select Texture")) {
			open = true;
		}

		if (open) {

			std::string windowNameWithUniqueID = std::format("Texture Selector {}", (uint64_t)obj.id);
			ImGui::Begin(windowNameWithUniqueID.c_str(), &open);

			static char setName[64] = "";

			if (ImGui::BeginCombo("##Texture Set", textureManager.getTextureSetByID(setID).setName.c_str())) {
				auto& textureSets = textureManager.getTextureSets();
				for (auto& set : textureSets) {
					if (ImGui::Selectable(set.setName.c_str(), set.setID == setID)) {
						setID = set.setID;
					}
				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();

			if (ImGui::Button("Create New Set")) {
				ImGui::OpenPopup("Create New Set");
				setName[0] = '\0';
			}

			if (ImGui::BeginPopup("Create New Set")) {
				ImGui::InputText("Set Name", setName, 64);

				if (ImGui::Button("Create")) {
					setID = textureManager.createTextureSet(std::string(setName));
					textureManager.saveTextureSet(setID, std::filesystem::path("TextureSets/" + std::string(setName) + ".txst"));
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			auto& textureSets = textureManager.getTextureSets();

			if (textureSets.size() == 0) {
				ImGui::Text("No texture sets loaded");
				ImGui::End();
				return modified;
			}
			else {
				if (ImGui::Button("Save")) {
					textureManager.saveTextureSet(setID, std::filesystem::path("TextureSets/" + textureManager.getTextureSetByID(setID).setName + ".txst"));
				}

				ImGui::SameLine();

				if (textureSets.size() != 0) {
					if (ImGui::Button("Add Texture")) {
						static int counter = 1;
						std::string newName = std::format("Texture ({})", counter++);

						textureManager.addTextureToSet(setID, newName);
					}
				}
			}

			ImGui::Separator();

			auto& textureSet = textureManager.getTextureSetByID(setID);

			if (textureSet.textures.size() == 0) {
				ImGui::Text("No textures in set");
				ImGui::End();
				return modified;
			}



			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);

			for (auto& texture : textureSet.textures) {
				if (drawTextureEntry(texture, obj.textureName, setID)) {
					obj.textureName = texture.textureName;
					modified = true;
				}
			}

			drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);

			ImGui::End();
		}

		return modified;

	}

	bool GameEditor::drawObjectProperties(GameObject& obj, bool pos, bool& textureSelectorOpen) {
		int modified = 0;

		ImGui::PushID(&obj);

		ImGui::InputText("Name", &obj.objectName);

		if (pos) {
			if (ImGui::DragFloat2("Position", &obj.position.x, 2.0f)) modified += 1;

			if (ImGui::IsItemDeactivatedAfterEdit() && m_GridLock) {
				obj.position.x = std::floor(obj.position.x / 32.0f) * 32.0f;
				obj.position.y = std::floor(obj.position.y / 32.0f) * 32.0f;
				modified += 1;
			}

		}
		if (ImGui::DragFloat2("Size", &obj.size.x, 2.0f)) modified += 1;

		if (ImGui::IsItemDeactivatedAfterEdit() && m_GridLock) {
			obj.size.x = std::floor(obj.size.x / 32.0f) * 32.0f;
			obj.size.y = std::floor(obj.size.y / 32.0f) * 32.0f;
			modified += 1;
		}

		modified = ImGui::InputInt("Trigger Group", (int*)&obj.triggerGroup);

		if (ImGui::CheckboxFlags("Background", (unsigned int*)&obj.flags, GameObjectFlags::Background)) modified += 1;
		ImGui::SameLine();
		if (ImGui::CheckboxFlags("Player", (unsigned int*)&obj.flags, GameObjectFlags::Player)) modified += 1;
		ImGui::SameLine();
		if (ImGui::CheckboxFlags("Trigger", (unsigned int*)&obj.flags, GameObjectFlags::Trigger)) modified += 1;
		ImGui::SameLine();
		if (ImGui::CheckboxFlags("Static", (unsigned int*)&obj.flags, GameObjectFlags::Static)) modified += 1;



		//static bool m_TextureSelectorOpen = false;
		modified += drawTextureSelector(m_GameInstance.m_TextureSetID, obj, textureSelectorOpen);

		if (ImGui::Checkbox("Repeat Texture", &obj.repeatTexture)) modified += 1;

		ImGui::Text("Collider Properties");
		ImGui::PushID("Collider");
		modified += ImGui::DragFloat2("Offset", &obj.collider.offset.x, 0.5f);
		modified += ImGui::DragFloat2("Size", &obj.collider.size.x, 0.5f);
		ImGui::PopID();

		if (obj.flags & GameObjectFlags::Trigger) {
			ImGui::Text("Triggers");
			ImGui::PushID("Triggers");
			for (auto it = obj.triggers.begin(); it != obj.triggers.end(); it++) {
				std::shared_ptr<TriggerBase>& trigger = *it;

				ImGui::PushID(trigger.get());

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
				float contentRegionAvailX = ImGui::GetContentRegionAvail().x;
				bool open = ImGui::TreeNodeEx("Trigger", flags);

				ImGui::SameLine(contentRegionAvailX - ImGui::CalcTextSize("X").x);

				if (ImGui::Button("X")) {
					it = obj.triggers.erase(it);
					modified += 1;
					if (it == obj.triggers.end()) {
						if (open) ImGui::TreePop();
						ImGui::PopID();
						break;
					}
				};
				if (open) {
					if (ImGui::Combo("Trigger Type", (int*)&trigger->type, "None\0Camera Trigger\0Object Move Trigger\0Score Trigger\0Finish Trigger\0Damage Trigger\0")) {
						switch (trigger->type) {
						case TriggerType::CameraTrigger:
							trigger = std::make_unique<CameraTrigger>();
							break;
						case TriggerType::ObjectMoveTrigger:
							trigger = std::make_unique<ObjectMoveTrigger>();
							break;
						case TriggerType::ScoreTrigger:
							trigger = std::make_unique<ScoreTrigger>();
							break;
						case TriggerType::FinishTrigger:
							trigger = std::make_unique<FinishTrigger>();
							break;
						case TriggerType::DamageTrigger:
							trigger = std::make_unique<DamageTrigger>();
							break;
						default:
							trigger = std::make_unique<TriggerBase>();
							break;
						}
						modified += 1;
					}

					if (ImGui::Combo("Trigger Condition", (int*)&trigger->condition, "None\0On Enter\0On Exit\0On Stay\0")) {
						modified += 1;
					}

					modified += ImGui::Checkbox("Single Use", &trigger->singleUse);

					switch (trigger->type) {
					case TriggerType::CameraTrigger: {
						CameraTrigger* cameraTrigger = static_cast<CameraTrigger*>(trigger.get());

						modified += ImGui::CheckboxFlags("Follow Player X", (unsigned int*)&cameraTrigger->followPlayer, FollowPlayerAxis::FollowPlayerX);
						modified += ImGui::CheckboxFlags("Follow Player Y", (unsigned int*)&cameraTrigger->followPlayer, FollowPlayerAxis::FollowPlayerY);

						modified += ImGui::DragFloat2("Camera Position", &cameraTrigger->targetPosition.x, 1.0f);
						modified += ImGui::DragFloat("Transition Time (s)", &cameraTrigger->transitionTime, 0.1f);
						modified += ImGui::DragFloat("Target Zoom", &cameraTrigger->targetZoom, 0.1f);
						break;
					}
					case TriggerType::ObjectMoveTrigger: {
						ObjectMoveTrigger* objectMoveTrigger = static_cast<ObjectMoveTrigger*>(trigger.get());
						modified += ImGui::Checkbox("Loop", &objectMoveTrigger->loop);

						modified += ImGui::InputInt("Target Group ID", (int*)&objectMoveTrigger->targetGroupID);

						for (uint32_t j = 0; j < objectMoveTrigger->pathPoints.size(); j++) {
							ImGui::PushID(j);
							modified += ImGui::DragFloat2("Target Position", &objectMoveTrigger->pathPoints[j].position.x, 1.0f);
							modified += ImGui::DragFloat("Move Time (s)", &objectMoveTrigger->pathPoints[j].moveTime, 0.1f);
							ImGui::PopID();
						}

						if (ImGui::Button("Add Path Point")) {
							objectMoveTrigger->pathPoints.push_back(MovePoint());
							modified += 1;
						}

						break;
					}
					case TriggerType::DamageTrigger: {
						DamageTrigger* damageTrigger = static_cast<DamageTrigger*>(trigger.get());
						ImGui::InputInt("Change Amount", (int*)&damageTrigger->damageAmount);
						break;
					}
					case TriggerType::ScoreTrigger: {
						ScoreTrigger* scoreTrigger = static_cast<ScoreTrigger*>(trigger.get());
						ImGui::InputInt("Score Change Amount", (int*)&scoreTrigger->scoreAmount);
						break;
					}
					case TriggerType::FinishTrigger: {
						FinishTrigger* finishTrigger = static_cast<FinishTrigger*>(trigger.get());
						ImGui::InputInt("Minimum Score", (int*)&finishTrigger->minimumScore);
						ImGui::InputText("Next Level Path", &finishTrigger->nextLevelPath);
						break;
					}
					}

					ImGui::Separator();
					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			if (ImGui::Button("Add Trigger")) {
				obj.triggers.push_back(std::make_shared<TriggerBase>());
				modified += 1;
			}

			ImGui::PopID();
		}


		ImGui::PopID();

		return modified > 0;
	}

	void GameEditor::renderGameLevelPreview(Graphics::RenderTarget* target, DirectX::XMFLOAT2 cameraPos, float cameraZoom, GameLevel& level)
	{
		Graphics::Renderer* renderer = Core::Application::getApplication()->getRenderer();

		renderer->beginRenderTarget(target, cameraPos, cameraZoom);
		
		for (auto& obj : level.getGameObjects()) {
			uint32_t textureHandle = renderer->getTextureManager().getTextureIDFromSet(m_GameInstance.m_TextureSetID, obj.textureName);

			float zLayer = 3.0f;
			if (obj.flags & GameObjectFlags::Background) zLayer = 5.0f;
			if (obj.flags & GameObjectFlags::Player) zLayer = 2.0f;

			renderer->submitRect(obj.position, zLayer, obj.size, textureHandle, obj.repeatTexture);
		}

		renderer->drawRects();

		renderer->endRenderTarget(target);
	}

	void GameEditor::drawProperties()
	{
		ImGui::Begin("Properties");
		switch (m_SelectionType) {
			case SelectionType::LevelObject: {
				GameObject& obj = m_GameInstance.m_GameLevel.getGameObject((ObjectID)m_Selection);

				static bool textureSelectorOpen = false;
				if (drawObjectProperties(obj, true, textureSelectorOpen)) {
					m_ActiveLevelSaved = false;
				}
				break;
			}
			case SelectionType::TemplateObject: {
				static bool textureSelectorOpen = false;
				if (ImGui::Button("Save Template")) {
					std::ofstream file(m_SelectedTemplatePath, std::ios::binary);
					file.write("OBJT", 4);
					m_SelectedObjectTemplate.saveToFile(file, false);
					file.close();
				}
				drawObjectProperties(m_SelectedObjectTemplate, false, textureSelectorOpen);
				break;
			}
			default:
				if (m_Selection == nullptr) ImGui::Text("Nothing Selected");
				else ImGui::Text("Unknown Selection Type %d", (int)m_SelectionType);
				break;
			}
		ImGui::End();
	}

	void GameEditor::drawContentBrowser()
	{
		auto texMan = Core::Application::getApplication()->getRenderer()->getTextureManager();

		if (!std::filesystem::exists(m_ContentFolderPath) || !std::filesystem::is_directory(m_ContentFolderPath)) return;
		ImGui::Begin("Content Browser");
	
		std::filesystem::path p = m_CurrentContentFolderPath.lexically_relative(m_ContentFolderPath);
		std::filesystem::path pAdded = m_ContentFolderPath;

		if (ImGui::Button(m_ContentFolderPath.filename().string().c_str())) {
			m_CurrentContentFolderPath = pAdded;
		}
		ImGui::SameLine();
		ImGui::Text("/");

		for (auto& folder : p) {
			if (folder == ".") continue;
			pAdded /= folder;
			ImGui::SameLine();
			if (ImGui::Button(folder.string().c_str())) {
				m_CurrentContentFolderPath = pAdded;
			}
			ImGui::SameLine();
			ImGui::Text("/");
		}

		std::filesystem::directory_iterator dirIter(m_CurrentContentFolderPath);


		float columnWidth = 144.0f;
		uint32_t columnCount = (uint32_t)(ImGui::GetContentRegionAvail().x / columnWidth);
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		uint32_t texture = 0;

		std::error_code ec;
		for (auto& dir : dirIter) {
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			
			GameObject templateObj;
			if (dir.path().extension() == ".objt") {
				std::ifstream file(dir.path(), std::ios::binary);
				char header[4];
				file.read(header, 4);
				if (std::string(header, 4) == "OBJT") {
					templateObj.loadFromFile(file, false);
					texture = texMan.getTextureIDFromSet(m_GameInstance.m_TextureSetID, templateObj.textureName);
				}
			}
			else if (dir.path().extension() == ".lvl") {
				std::filesystem::path thumbnailPath = dir.path().string() + ".thumb";

				if (!std::filesystem::exists(thumbnailPath) || std::filesystem::last_write_time(dir.path()) > std::filesystem::last_write_time(thumbnailPath)) {
					m_ThumbnailGenPaths.push_back(dir.path());
				}
				else {
					std::ifstream file(thumbnailPath, std::ios::binary);

					uint32_t* data = new uint32_t[128 * 128];
					file.read((char*)data, 128 * 128 * sizeof(uint32_t));
					texture = texMan.loadTextureFromMemory((const char*)data, 128, 128, 4);
					file.close();
					delete[] data;
				}
			}
			else if (dir.path().extension() == ".thumb") continue;

			drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);

			ImGui::ImageButton(dir.path().filename().string().c_str(), (ImTextureID)Core::Application::getApplication()->getRenderer()->getTextureManager().getSRVGPUDescriptorHandle(texture).ptr, ImVec2(128.0f, 128.0f));
			drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);



			if (dir.is_directory()) {
				if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					m_CurrentContentFolderPath = dir.path();
				}
			}
			else {
				if (dir.path().extension() == ".lvl") {
					if (ImGui::IsItemClicked()) {
						m_Selection = (void*)&dir.path();
						m_SelectionType = SelectionType::Level;

						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							m_GameInstance.m_GameLevel.loadLevel(dir.path());
							m_GameInstance.m_GameSettings.levelPath = dir.path();
							m_SimulationMode = false;
							m_ActiveLevelSaved = true;
						}

					}

					if (ImGui::BeginDragDropSource()) {
						std::string path = dir.path().string();

						ImGui::SetDragDropPayload("CONTENT_BROWSER_LEVEL", path.c_str(), path.size() + 1);
						ImGui::EndDragDropSource();
					}

				}
				else if (dir.path().extension() == ".objt") {
					if (ImGui::IsItemClicked()) {
						m_SelectedObjectTemplate = templateObj;
						m_SelectedTemplatePath = dir.path();
						m_Selection = (void*)&m_SelectedObjectTemplate;
						m_SelectionType = SelectionType::TemplateObject;

					}

					if (ImGui::BeginDragDropSource()) {
						ImGui::SetDragDropPayload("OBJECT_TEMPLATE", &m_SelectedObjectTemplate, sizeof(GameObject));
						ImGui::EndDragDropSource();
					}
				}
			}

			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Delete")) {
					std::filesystem::remove_all(dir.path());
				}
				ImGui::EndPopup();
			}

			ImGui::TextWrapped("%s", dir.path().filename().string().c_str());

			ImGui::NextColumn();
		}

		if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::BeginMenu("Create")) {

				if (ImGui::MenuItem("Level")) {
					GameLevel newLevel;
					std::string newLevelName = "New Level.lvl";
					int counter = 1;
					while (std::filesystem::exists(m_CurrentContentFolderPath / newLevelName)) {
						newLevelName = "New Level (" + std::to_string(counter) + ").lvl";
						counter++;
					}
					newLevel.saveLevel(m_CurrentContentFolderPath / newLevelName);
				}

				if (ImGui::MenuItem("Object Preset")) {
					GameObject newObject;
					newObject.flags = GameObjectFlags::Static;
					newObject.position = { 0, 0 };
					newObject.size = { 32, 32 };
					newObject.textureName = "";

					newObject.collider.offset = { 0, 0 };
					newObject.collider.size = { 32, 32 };

					std::string newObjectName = "New Object.objt";
					int counter = 1;
					while (std::filesystem::exists(m_CurrentContentFolderPath / newObjectName)) {
						newObjectName = "New Object (" + std::to_string(counter) + ").objt";
						counter++;
					}

					std::ofstream file(m_CurrentContentFolderPath / newObjectName, std::ios::binary);
					file.write("OBJT", 4);
					newObject.saveToFile(file, false);
					file.close();
				}

				ImGui::Separator();
				if (ImGui::MenuItem("Create Folder")) {
					std::string newFolderName = "New Folder";
					std::filesystem::path newFolderPath = m_CurrentContentFolderPath / newFolderName;
					int counter = 1;
					while (std::filesystem::exists(newFolderPath)) {
						newFolderName = "New Folder (" + std::to_string(counter) + ")";
						newFolderPath = m_CurrentContentFolderPath / newFolderName;
						counter++;
					}
					std::filesystem::create_directory(newFolderPath);
				}
				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::End();

	}

	void GameEditor::drawEditorSettings(bool& open)
	{
		if (!open) return;
		ImGui::Begin("Editor Settings", &open, ImGuiWindowFlags_NoDocking);

		static std::string contentBrowserPath = m_ContentFolderPath.string();

		ImGui::Text("Content Browser Path:");
		ImGui::SameLine();
		ImGui::InputText("###Content Browser Path", &contentBrowserPath);
		ImGui::SameLine();

		if (ImGui::Button("Set Path")) {
			if (!std::filesystem::exists(contentBrowserPath)) {
				std::filesystem::create_directories(contentBrowserPath);
			}

			HKEY hKey;
			RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\KI-MA\\EditorSettings", 0, KEY_SET_VALUE, &hKey);

			std::wstring contentBrowserPathW = std::filesystem::path(contentBrowserPath).wstring();
			RegSetValueExW(hKey, L"ContentBrowserPath", 0, REG_SZ, (const BYTE*)contentBrowserPathW.c_str(), (DWORD)(contentBrowserPathW.size() + 1) * sizeof(WCHAR));
			RegCloseKey(hKey);
			m_ContentFolderPath = std::filesystem::path(contentBrowserPath);
			m_CurrentContentFolderPath = m_ContentFolderPath;
		}

		ImGui::End();

	}

}
