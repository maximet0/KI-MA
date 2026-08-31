#include "GameInstance.h"

#include "Core/Application.h"

#include "external/ImGui/ImGui.h"
#include "external/ImGui/misc/cpp/imgui_stdlib.h"
#include "Core/Logger.h"
#include "Events/Callbacks.h"
#include "GamePhysics.h"
#include "GameTriggers.h"

#include <cmath>



namespace Game {

	bool keyDown[256] = { false };

	GameInstance::GameInstance(GameSettings settings)
		: m_GameSettings(settings)
	{
		m_Target = new Graphics::RenderTarget({ 640, 360 }, {0.1, 0.3, 0.7, 1.0});
		m_GameLevel.loadLevel(m_GameSettings.levelPath);

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::WindowCloseCallback>(Events::EventType::WINDOW_CLOSE, [this](Core::Window * window) {
			m_CloseApplication = true;
		});

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::KeyboardKeyCallback>(Events::EventType::KEYBOARD_KEY, [this](Core::Window* window, Events::KeyState c, Events::KeyboardKey key, int scancode) {
			if(c == Events::KeyState::Down) {
				keyDown[(uint8_t)key] = true;
			}
			else if(c == Events::KeyState::Up) {
				keyDown[(uint8_t)key] = false;
			}
		});

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::MousePosCallback>(Events::EventType::MOUSE_POS, [this](Core::Window* window, DirectX::XMINT2 mousePos, DirectX::XMINT2 aMousePos) {
			m_MousePos.x = mousePos.x;
			m_MousePos.y = mousePos.y;

			});

		Core::Application::getApplication()->getEventSystem()->registerCallback<Events::MouseWheelCallback>(Events::EventType::MOUSE_WHEEL, [this](Core::Window* window, DirectX::XMINT2 wheelDelta) {
			m_MouseWheelDelta.x += wheelDelta.x;
			m_MouseWheelDelta.y += wheelDelta.y;
			});

		m_DrawObject.flags = GameObjectFlags::Static;
		m_DrawObject.position = { 0, 0 };
		m_DrawObject.size = { 32, 32 };
		m_DrawObject.textureName = "";

		m_DrawObject.collider.offset = { 0, 0 };
		m_DrawObject.collider.size = { 32, 32 };


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
		m_CameraFollowPlayer = true;
	}

	float maxSpeed = 300.0f;
	float acceleration = 2500.0f;
	float jumpForce = 420.0f;

	float gracePeriod = 0.1f;

	void GameInstance::update(float deltaTime)
	{
		if ((m_SimulationMode && !m_Paused) || !m_GameSettings.levelEditorMode) {
			static ObjectID playerID = 0;
			if (!(m_GameLevel.getGameObject(playerID).flags & GameObjectFlags::Player)) {
				for (auto& obj : m_GameLevel.getGameObjects()) {
					if (obj.flags & GameObjectFlags::Player) {
						playerID = obj.id;
						break;
					}
				}
			}

			static bool grounded = false;

			if (playerID != 0) {
				GameObject& player = m_GameLevel.getGameObject(playerID);

				if (m_GameLevel.getPlayerLives() == 0) {
					m_Paused = true;
					return;
				}
				
				player.physics.acceleration.x = 0.0f;

				if (grounded) acceleration = 2500.0f;
				else acceleration = 800.0f;

				if (keyDown[Events::KeyboardKey::Key_W] && (grounded || gracePeriod > 0.0f)) {
					player.physics.velocity.y = -jumpForce;
				}
				if (keyDown[Events::KeyboardKey::Key_A]) {
					player.physics.acceleration.x = -acceleration;
				}
				else if (keyDown[Events::KeyboardKey::Key_D]) {
					player.physics.acceleration.x = acceleration;
				}

				player.physics.velocity.x = std::clamp(player.physics.velocity.x, -maxSpeed, maxSpeed);


				if (m_CameraFollowPlayer) {
					m_CameraPosition.x = player.position.x + player.size.x / 2;
					m_CameraPosition.y = player.position.y + player.size.y / 2;
				}

			}


			GameTriggers::updateTriggers(*this, deltaTime);

			GamePhysics::updatePhysics(m_GameLevel, deltaTime, m_Gravity);


			if (playerID != 0) {
				//Ground Check
				GameObject& player = m_GameLevel.getGameObject(playerID);

				RaycastHit hit = GamePhysics::boxcast(m_GameLevel, { player.position.x, player.position.y + player.size.y + 0.02f }, { player.size.x * 0.9f, 0.02f }, { 0, 1 }, 5.0f);
				grounded = hit.hit;
				if (grounded) {
					gracePeriod = 0.1f;
				}
				else {
					gracePeriod -= deltaTime;
				}

			}

		}
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

			if(ImGui::BeginCombo("##Texture Set", textureManager.getTextureSetByID(setID).setName.c_str())) {
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

			if(textureSet.textures.size() == 0) {
				ImGui::Text("No textures in set");
				ImGui::End();
				return modified;
			}




			for (auto& texture : textureSet.textures) {
				if(drawTextureEntry(texture, obj.textureName, setID)) {
					obj.textureName = texture.textureName;
					modified = true;
				}
			}

			

			ImGui::End();
		}

		return modified;

	}

	bool GameInstance::drawObjectProperties(GameObject& obj, bool pos, bool& textureSelectorOpen) {
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
		modified += drawTextureSelector(m_TextureSetID, obj, textureSelectorOpen);

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

				if (ImGui::Button("X")){
					it = obj.triggers.erase(it);
					modified += 1;
					if (it == obj.triggers.end()) {
						if(open) ImGui::TreePop();
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
						}
						modified += 1;
					}

					if(ImGui::Combo("Trigger Condition", (int*)&trigger->condition, "None\0On Enter\0On Exit\0On Stay\0")) {
						modified += 1;
					}

					modified += ImGui::Checkbox("Single Use", &trigger->singleUse);

					switch (trigger->type) {
						case TriggerType::CameraTrigger: {
							CameraTrigger* cameraTrigger = static_cast<CameraTrigger*>(trigger.get());
							modified += ImGui::Checkbox("Follow Player", &cameraTrigger->followPlayer);
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

	void GameInstance::drawGUI()
	{
		Core::Application* app = Core::Application::getApplication();
		Graphics::Renderer* renderer = app->getRenderer();

		if (m_GameSettings.levelEditorMode) {

			if (!m_SimulationMode) m_CameraFollowPlayer = false;

			bool saveSettings = false;
			ImGui::Begin("Level Editor");

			static GameSettings currentSettings = m_GameSettings;

			std::string levelPathStr = currentSettings.levelPath.string();


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

			if (m_SimulationMode) ImGui::BeginDisabled();

			if (ImGui::Button("Save Level")) {
				m_GameLevel.saveLevel(m_GameSettings.levelPath);
				m_LevelSaved = true;
			}
			
			if (m_SimulationMode) ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::Text("Level Editor");

			if (m_SimulationMode) {
				if (ImGui::Button(m_Paused ? "Resume Simulation" : "Pause Simulation")) {
					m_Paused = !m_Paused;
				}

				ImGui::SameLine();

				if (ImGui::Button("Reset Simulation")) {
					m_GameLevel.loadLevel("tmpSimSave.lvl");
					m_CameraPosition = { 0, 0 };
					m_CameraFollowPlayer = true;
				}

				ImGui::SameLine();

				if(ImGui::Button("Stop Simulation")) {
					m_GameLevel.loadLevel("tmpSimSave.lvl");
					m_SimulationMode = false;
					m_CameraPosition = { 0, 0 };
					m_CameraFollowPlayer = false;
				}
			}
			else {
				if (ImGui::Button("Start Simulation")) {
					m_GameLevel.saveLevel("tmpSimSave.lvl", true);
					m_SimulationMode = true;
					m_CameraFollowPlayer = true;
				}
			}

			ImGui::Checkbox("Grid Lock", &m_GridLock);
			ImGui::Checkbox("Draw Mode", &m_DrawMode);
			if (m_DrawMode) {
				ImGui::Text("Draw Object Properties");
				static bool textureSelectorOpen = false;
				drawObjectProperties(m_DrawObject, false, textureSelectorOpen);

			}


			ImGui::End();

			if (saveSettings) {
				setGameSettings(currentSettings);
				currentSettings = m_GameSettings;
			}


			ImGui::Begin("Game");
			auto textureHandle = renderer->getTextureManager().getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x, m_Target->getSize().y));
			DirectX::XMFLOAT2 offset = { ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
			bool gameViewLeftMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
			bool gameViewRightMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right);

			//Berechne die Position der Maus relativ zum RenderTarget
			DirectX::XMFLOAT2 relMousePos = { m_MousePos.x - offset.x, m_MousePos.y - offset.y };

			//Berechne die Position in der Welt
			DirectX::XMVECTOR worldPosVec = DirectX::XMLoadFloat2(&m_MousePos);
			
			worldPosVec = DirectX::XMVectorSubtract(worldPosVec, DirectX::XMLoadFloat2(&offset));
			worldPosVec = DirectX::XMVectorSubtract(worldPosVec, DirectX::XMVectorSet(m_Target->getSize().x * 0.5f, m_Target->getSize().y * 0.5f, 0.0f, 0.0f));
			worldPosVec = DirectX::XMVectorScale(worldPosVec, m_CameraZoom);
			worldPosVec = DirectX::XMVectorAdd(worldPosVec, DirectX::XMLoadFloat2(&m_CameraPosition));

			DirectX::XMFLOAT2 worldPos;
			DirectX::XMStoreFloat2(&worldPos, worldPosVec);


			static ObjectID selectedID = 0;
			ObjectID clickedID = 0;
			if (gameViewLeftMouseDown || gameViewRightMouseDown) {

				for (auto it = m_GameLevel.getGameObjects().end(); it != m_GameLevel.getGameObjects().begin(); it--) {
					GameObject& obj = *(it - 1);
					if (worldPos.x >= obj.position.x && worldPos.x <= obj.position.x + obj.size.x &&
						worldPos.y >= obj.position.y && worldPos.y <= obj.position.y + obj.size.y) {
						if (gameViewLeftMouseDown) selectedID = obj.id;
						clickedID = obj.id;
						break;
					}
				}
			}


			static bool cameraDrag = false;

			if (keyDown[Events::KeyboardKey::Key_LSHIFT] && gameViewRightMouseDown) cameraDrag = true;
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) cameraDrag = false;

			if (cameraDrag) {
				m_CameraPosition.x -= m_MouseDelta.x * m_CameraZoom;
				m_CameraPosition.y -= m_MouseDelta.y * m_CameraZoom;
			}

			if (keyDown[Events::KeyboardKey::Key_LSHIFT] && ImGui::IsItemHovered() && m_MouseWheelDelta.y != 0.0f) {
				m_CameraZoom -= m_MouseWheelDelta.y * 0.1f;
				if (m_CameraZoom < 0.1f) m_CameraZoom = 0.1f;
				if (m_CameraZoom > 10.0f) m_CameraZoom = 10.0f;
			}


			if (m_DrawMode) {
				//ImGui::
				if (gameViewLeftMouseDown && clickedID == 0) {
					
					m_DrawObject.position.x = worldPos.x;
					m_DrawObject.position.y = worldPos.y;

					if (m_GridLock) {
						m_DrawObject.position.x = std::floor(m_DrawObject.position.x / 32.0f) * 32.0f;
						m_DrawObject.position.y = std::floor(m_DrawObject.position.y / 32.0f) * 32.0f;
					}

					m_GameLevel.addGameObject(m_DrawObject);
					m_LevelSaved = false;
				}
				if (gameViewRightMouseDown && clickedID != 0 && !cameraDrag) {
					m_GameLevel.removeGameObject(clickedID);
					selectedID = 0;
					m_LevelSaved = false;
				}
			}


			ImGui::Text("Debug Info:");
			ImGui::Text("	Score: %d", m_GameLevel.getScore());
			ImGui::Text("	Player Lives: %d", m_GameLevel.getPlayerLives());
			ImGui::Text("	Camera Position: (%.2f, %.2f)", m_CameraPosition.x, m_CameraPosition.y);
			ImGui::End();


			ImGui::Begin("ObjectList");

			if (ImGui::Button("Add Object")) {
				GameObject defaultObj;
				defaultObj.flags = GameObjectFlags::Static;
				defaultObj.position = { 0, 0 };
				defaultObj.size = { 32, 32 };
				defaultObj.textureName = "";

				defaultObj.collider.offset = { 0, 0 };
				defaultObj.collider.size = { 32, 32 };

				m_GameLevel.addGameObject(defaultObj);
				m_LevelSaved = false;
			}

			if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen, "GameObjects (%d)", m_GameLevel.getGameObjectCount())) {

				for (auto& obj : m_GameLevel.getGameObjects()) {
					ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
					if (selectedID == obj.id) nodeFlags |= ImGuiTreeNodeFlags_Selected;

					ImGui::TreeNodeEx((void*)(intptr_t)obj.id, nodeFlags, obj.objectName.c_str(), obj.id);

					if (ImGui::IsItemClicked()) {
						selectedID = obj.id;
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

			if (selectedID != 0) {
				static bool textureSelectorOpen = false;
				if (drawObjectProperties(m_GameLevel.getGameObject(selectedID), true, textureSelectorOpen)) {
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
			auto textureHandle = renderer->getTextureManager().getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x, m_Target->getSize().y));
			ImGui::End();

			if (m_CloseApplication) {
				app->exit();
			}
		}

		m_MouseDelta.x = m_MousePos.x - m_LastMousePos.x;
		m_MouseDelta.y = m_MousePos.y - m_LastMousePos.y;
		m_LastMousePos = m_MousePos;

		m_MouseWheelDelta.x = 0;
		m_MouseWheelDelta.y = 0;
	}

	void GameInstance::render()
	{
		Core::Application* application = Core::Application::getApplication();
		Graphics::Renderer* renderer = application->getRenderer();


		renderer->beginRenderTarget(m_Target, m_CameraPosition, 1.0f / m_CameraZoom);

		for (auto& obj : m_GameLevel.getGameObjects()) {
			uint32_t textureHandle = renderer->getTextureManager().getTextureIDFromSet(m_TextureSetID, obj.textureName);

			float zLayer = 3.0f;
			if (obj.flags & GameObjectFlags::Background) zLayer = 5.0f;
			if (obj.flags & GameObjectFlags::Player) zLayer = 2.0f;

			renderer->submitRect(obj.position, zLayer, obj.size, textureHandle, obj.repeatTexture);
		}

		if (m_GameSettings.levelEditorMode) {
			//Ein Raster basierend auf der Kameraposition und dem Zoomlevel zeichnen;
			
			constexpr float gridSize = 32.0f;
			constexpr float invGridSize = 1.0f / gridSize;
			constexpr float gridSize2x = gridSize * 2;

			float halfWidth = m_Target->getSize().x * 0.5f * m_CameraZoom;
			float halfHeight = m_Target->getSize().y * 0.5f * m_CameraZoom;

			float left = m_CameraPosition.x - halfWidth;
			float right = m_CameraPosition.x + halfWidth;
			float top = m_CameraPosition.y - halfHeight;
			float bottom = m_CameraPosition.y + halfHeight;

			float startX = std::floor(left / gridSize) * gridSize;
			float startY = std::floor(top / gridSize) * gridSize;

			for (int32_t x = startX; x <= right; x += gridSize) {
				renderer->submitLine({ (float)x, (float)top }, { (float)x, (float)bottom }, 0x6F829440);
			}

			for (int32_t y = startY; y <= bottom; y += gridSize) {
				renderer->submitLine({ (float)right, (float)y }, { (float)left, (float)y }, 0x6F829440);
			}

		}

		if (m_GameSettings.showColliders) {
			for (auto& obj : m_GameLevel.getGameObjects()) {
				uint32_t color = 0x00FF00FF;
				if ((obj.flags & GameObjectFlags::Trigger) != 0) color = 0xFF0000FF;
				if ((obj.flags & GameObjectFlags::Static) == 0) color = 0x0000FFFF;
				renderer->submitLine({ obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y }, { obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y }, color);
				renderer->submitLine({ obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y }, { obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, color);
				renderer->submitLine({ obj.position.x + obj.collider.offset.x + obj.collider.size.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, { obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, color);
				renderer->submitLine({ obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y + obj.collider.size.y }, { obj.position.x + obj.collider.offset.x, obj.position.y + obj.collider.offset.y }, color);
			}
		}

		renderer->drawRects();
		renderer->drawLines();

		renderer->endRenderTarget(m_Target);

	}



}

