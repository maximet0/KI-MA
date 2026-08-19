#include "GameInstance.h"

#include "Core/Application.h"

#include "external/ImGui/ImGui.h"
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

		m_DrawObject.flags = (GameObjectFlags)(GameObjectFlags::Valid | GameObjectFlags::Static);
		m_DrawObject.position = { 0, 0 };
		m_DrawObject.size = { 32, 32 };
		strcpy_s(m_DrawObject.textureName, "");

		m_DrawObject.collider.position = { 0, 0 };
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
	}

	//AABB Kollisionsabfrage
	/*CollisionInfo checkCollision(GameObject a, GameObject b) {
		CollisionInfo info;
		info.hit = false;
		info.direction = CollisionDirection::None;
		info.overlap = 0.0f;

		for (uint8_t i = 0; i < a.colliderCount; i++) {
			for (uint8_t j = 0; j < b.colliderCount; j++) {
				GameCollider colliderA = a.colliders[i];
				GameCollider colliderB = b.colliders[j];
				float aPosX = colliderA.position.x + a.position.x;
				float aPosY = colliderA.position.y + a.position.y;

				float bPosX = colliderB.position.x + b.position.x;
				float bPosY = colliderB.position.y + b.position.y;

				//Core::Logger::Debug("{} {} {} {}", aPosX, aPosY, bPosX, bPosY);
				
				if (aPosX < bPosX + colliderB.size.x &&
					aPosX + colliderA.size.x > bPosX &&
					aPosY < bPosY + colliderB.size.y &&
					aPosY + colliderA.size.y > bPosY) {

					info.hit = true;

					float aCenterX = aPosX + colliderA.size.x / 2;
					float aCenterY = aPosY + colliderA.size.y / 2;

					float bCenterX = bPosX + colliderB.size.x / 2;
					float bCenterY = bPosY + colliderB.size.y / 2;

					float dx = aCenterX - bCenterX;
					float dy = aCenterY - bCenterY;

					float halfWidth = (colliderA.size.x + colliderB.size.x) * 0.5f;
					float halfHeight = (colliderA.size.y + colliderB.size.y) * 0.5f;

					float overlapX = halfWidth - std::abs(dx);
					float overlapY = halfHeight - std::abs(dy);
					
					if (overlapX < overlapY) {
						info.direction = (dx < 0.0f) ? CollisionDirection::Left : CollisionDirection::Right;
						info.overlap = overlapX;
						return info;
					}
					else {
						info.direction = (dy < 0.0f) ? CollisionDirection::Bottom : CollisionDirection::Top;
						info.overlap = overlapY;
						return info;
					}
				}
			}
		}
		return info;
	}*/

	float maxSpeed = 250.0f;
	float acceleration = 1500.0f;
	float deceleration = 2000.0f;
	float jumpForce = 300.0f;

	float gracePeriod = 0.1f;

	void GameInstance::update(float deltaTime)
	{
		if ((m_SimulationMode && !m_Paused) || !m_GameSettings.levelEditorMode) {
			static int32_t playerID = -1;
			if (playerID == -1) {
				for (uint16_t i = 0; i < m_GameLevel.getGameObjectCount(); i++) {
					if (m_GameLevel.getGameObjects()[i].flags & GameObjectFlags::Player) {
						playerID = i;
						break;
					}
				}
			}

			static bool grounded = false;

			if (playerID != -1) {
				GameObject& player = m_GameLevel.getGameObjects()[playerID];
				player.physics.acceleration.x = 0.0f;


				if (keyDown[Events::KeyboardKey::Key_W] && (grounded || gracePeriod > 0.0f)) {
					player.physics.velocity.y = -jumpForce;
				}
				if (keyDown[Events::KeyboardKey::Key_A]) {
					player.physics.acceleration.x = -acceleration;
				}
				else if (keyDown[Events::KeyboardKey::Key_D]) {
					player.physics.acceleration.x = acceleration;
				}
				else {
					if (player.physics.velocity.x > 0.0f) {
						player.physics.acceleration.x -= deceleration;
					}
					else if (player.physics.velocity.x < 0.0f) {
						player.physics.acceleration.x += deceleration;
					}
				}


				if (!keyDown[Events::KeyboardKey::Key_A] && !keyDown[Events::KeyboardKey::Key_D])
				{
					if (std::abs(player.physics.velocity.x) < deceleration * deltaTime)
						player.physics.velocity.x = 0;
				}

				player.physics.velocity.x = std::clamp(player.physics.velocity.x, -maxSpeed, maxSpeed);


				if (m_CameraFollowPlayer) {
					m_CameraPosition.x = player.position.x + player.size.x / 2;
					m_CameraPosition.y = player.position.y + player.size.y / 2;
				}

			}

			GamePhysics::updatePhysics(m_GameLevel, deltaTime, m_Gravity);


			if (playerID != -1) {
				//Ground Check
				GameObject& player = m_GameLevel.getGameObjects()[playerID];

				RaycastHit hit = GamePhysics::boxcast(m_GameLevel, { player.position.x, player.position.y + player.size.y + 0.02f }, { player.size.x * 0.9f, 0.02f }, { 0, 1 }, 5.0f);
				grounded = hit.hit;
				if (grounded) {
					gracePeriod = 0.1f;
				}
				
				if(!grounded) gracePeriod -= deltaTime;
				
			}
			
			GameTriggers::updateTriggers(*this, deltaTime);

			/*grounded = false;
			uint16_t validIndex = 0;
			for (uint16_t i = 0; i < m_GameLevel.getGameObjectCount(); i++) {
				GameObject& obj = m_GameLevel.getGameObjects()[i];
				if (!obj.isStatic) {

					//obj.physics.velocity.x *= 0.9f;
					obj.physics.velocity.x += obj.physics.acceleration.x * deltaTime;
					obj.physics.velocity.y += (m_Gravity + obj.physics.acceleration.y) * deltaTime;

					obj.position.x += obj.physics.velocity.x * deltaTime;
					obj.position.y += obj.physics.velocity.y * deltaTime;

					for (uint16_t j = 0; j < m_GameLevel.getGameObjectCount(); j++) {
						if (j == i) continue;
						CollisionInfo collision = checkCollision(obj, m_GameLevel.getGameObjects()[j]);
						if (!collision.hit) continue;
						if (collision.direction == CollisionDirection::Bottom) {
							obj.physics.velocity.y = 0;
							obj.position.y -= collision.overlap;
							if(i == playerID) grounded = true;
						}
						else if (collision.direction == CollisionDirection::Left) {
							obj.physics.velocity.x = 0;
							obj.position.x -= collision.overlap;
						}
						else if (collision.direction == CollisionDirection::Right) {
							obj.physics.velocity.x = 0;
							obj.position.x += collision.overlap;
						}
						else if (collision.direction == CollisionDirection::Top) {
							obj.physics.velocity.y = 0;
							obj.position.y += collision.overlap;
						}
					}


				}
			}*/

		}
	}

	bool drawTextureEntry(Graphics::TextureSetEntry& tex, char textureName[64], uint32_t setID) {
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
		else if (tex.textureName == std::string(textureName)) {
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
			ImGui::InputText("New Name", newPath, 256);

			if (ImGui::Button("Rename")) {
				textureManager.modifyTextureInSet(setID, tex.textureName, std::string(newPath), tex.texturePath);
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
		return modified;
	}

	bool drawTextureSelector(uint32_t& setID, char textureName[64], bool& open) {
		bool modified = false;
		Core::Application* app = Core::Application::getApplication();
		Graphics::TextureManager& textureManager = app->getRenderer()->getTextureManager();


		ImGui::Text("Texture %s", textureName);
		ImGui::SameLine();

		if (ImGui::Button("Select Texture")) {
			open = true;
		}

		if (open) {

			ImGui::Begin("Texture Selector", &open);

			if (ImGui::Button("Create New Set")) {
				setID = textureManager.createTextureSet();
				textureManager.addTextureToSet(setID, "Texture");
			}
			auto& textureSets = textureManager.getTextureSets();

			if (textureSets.size() != 0) {
				if (ImGui::Button("Add Texture")) {
					static int counter = 1;
					std::string newName = std::format("Texture ({})", counter++);

					textureManager.addTextureToSet(setID, newName);
				}
			}


			ImGui::Separator();


			if (textureSets.size() == 0) {
				ImGui::Text("No texture sets loaded");
				ImGui::End();
				return modified;
			}

			auto& textureSet = textureManager.getTextureSetByID(setID);

			if(textureSet.textures.size() == 0) {
				ImGui::Text("No textures in set");
				ImGui::End();
				return modified;
			}




			for (auto& texture : textureSet.textures) {
				if(drawTextureEntry(texture, textureName, setID)) {
					strcpy_s(textureName, 64, texture.textureName.c_str());
					modified = true;
				}
			}

			

			ImGui::End();
		}

		return modified;

	}

	bool GameInstance::drawObjectProperties(GameObject& obj, bool pos) {
		int modified = 0;

		ImGui::PushID(&obj);

		ImGui::InputText("Name", obj.objectName, 64);

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
	
		if (ImGui::CheckboxFlags("Background", (unsigned int*)&obj.flags, GameObjectFlags::Background)) modified += 1;
		ImGui::SameLine();
		if (ImGui::CheckboxFlags("Player", (unsigned int*)&obj.flags, GameObjectFlags::Player)) modified += 1;
		ImGui::SameLine();
		if (ImGui::CheckboxFlags("Trigger", (unsigned int*)&obj.flags, GameObjectFlags::Trigger)) modified += 1;
		ImGui::SameLine();
		if (ImGui::CheckboxFlags("Static", (unsigned int*)&obj.flags, GameObjectFlags::Static)) modified += 1;

		static bool m_TextureSelectorOpen = false;
		modified += drawTextureSelector(m_TextureSetID, obj.textureName, m_TextureSelectorOpen);

		ImGui::Text("Collider Properties");
		ImGui::PushID("Collider");
		modified += ImGui::DragFloat2("Position", &obj.collider.position.x, 0.5f);
		modified += ImGui::DragFloat2("Size", &obj.collider.size.x, 0.5f);
		ImGui::PopID();

		if (obj.flags & GameObjectFlags::Trigger) {
			ImGui::Text("Triggers");
			ImGui::PushID("Triggers");
			for (uint32_t i = 0; i < obj.triggers.size(); i++) {
				ImGui::PushID(i);
				if (obj.triggers[i].type == TriggerType::None) {
					if (ImGui::Button("Add Trigger")) {
						obj.triggers[i].type = TriggerType::CameraTrigger;
						modified += 1;
					}
					ImGui::PopID();
					break;
				}

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
				float contentRegionAvailX = ImGui::GetContentRegionAvail().x;
				bool open = ImGui::TreeNodeEx("Trigger", flags);

				ImGui::SameLine(contentRegionAvailX - ImGui::CalcTextSize("X").x);

				if (ImGui::Button("X")){
					obj.triggers[i].type = TriggerType::None;
					modified += 1;
				};
				if (open) {
					if (ImGui::Combo("Trigger Type", (int*)&obj.triggers[i].type, "None\0Camera Trigger\0Object Move Trigger\0Score Trigger\0Finish Trigger\0Damage Trigger\0")) {
						modified += 1;
					}

					if(ImGui::Combo("Trigger Condition", (int*)&obj.triggers[i].condition, "None\0On Enter\0On Exit\0On Stay\0")) {
						modified += 1;
					}

					modified += ImGui::Checkbox("Single Use", &obj.triggers[i].singleUse);

					switch (obj.triggers[i].type) {
					case TriggerType::CameraTrigger:
						modified += ImGui::Checkbox("Follow Player", &obj.triggers[i].cameraTrigger.followPlayer);
						modified += ImGui::DragFloat2("Camera Position", &obj.triggers[i].cameraTrigger.targetPosition.x, 1.0f);
						modified += ImGui::DragFloat("Transition Time (s)", &obj.triggers[i].cameraTrigger.transitionTime, 0.1f);
						modified += ImGui::DragFloat("Target Zoom", &obj.triggers[i].cameraTrigger.targetZoom, 0.1f);
						break;
					case TriggerType::ObjectMoveTrigger: {

						modified += ImGui::Checkbox("Loop", &obj.triggers[i].objectMoveTrigger.loop);

						for (uint32_t j = 0; j < obj.triggers[i].objectMoveTrigger.targetCount; j++) {
							ImGui::PushID(j);
							modified += ImGui::InputInt("Target Object ID", (int*)&obj.triggers[i].objectMoveTrigger.targetObjectIDs[j]);
							ImGui::PopID();
						}
						if (obj.triggers[i].objectMoveTrigger.targetCount < 32) {
							if (ImGui::Button("Add Target Object ID")) {
								obj.triggers[i].objectMoveTrigger.targetCount++;
								modified += 1;
							}
						}	

						for (uint32_t j = 0; j < obj.triggers[i].objectMoveTrigger.targetCount; j++) {
							ImGui::PushID(j);
							modified += ImGui::DragFloat2("Target Position", &obj.triggers[i].objectMoveTrigger.pathPoints[j].position.x, 1.0f);
							modified += ImGui::DragFloat("Move Time (s)", &obj.triggers[i].objectMoveTrigger.pathPoints[j].moveTime, 0.1f);
							ImGui::PopID();
						}
						
						if (obj.triggers[i].objectMoveTrigger.targetCount < 16) {
							if (ImGui::Button("Add Path Point")) {
								obj.triggers[i].objectMoveTrigger.targetCount++;
								modified += 1;
							}
						}	

						break;
					}
					case TriggerType::DamageTrigger:
						ImGui::InputInt("Change Amount", (int*)&obj.triggers[i].damageTrigger.damageAmount);
						break;
					case TriggerType::ScoreTrigger:
						ImGui::InputInt("Score Change Amount", (int*)&obj.triggers[i].scoreTrigger.scoreAmount);
						break;
					case TriggerType::FinishTrigger:
						ImGui::InputInt("Minimum Score", (int*)&obj.triggers[i].finishTrigger.minimumScore);
						ImGui::InputText("Next Level Path", (char*)obj.triggers[i].finishTrigger.nextLevelPath, 256);
						break;
					}

					ImGui::Separator();
					ImGui::TreePop();
				}

				ImGui::PopID();
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
			if (ImGui::Button("Save Level")) {
				m_GameLevel.saveLevel(m_GameSettings.levelPath);
				m_LevelSaved = true;
			}

			ImGui::Separator();

			ImGui::Text("Level Editor");

			if (m_SimulationMode) {
				if (ImGui::Button(m_Paused ? "Resume Simulation" : "Pause Simulation")) {
					m_Paused = !m_Paused;
				}

				ImGui::SameLine();

				if (ImGui::Button("Reset Simulation")) {
					m_GameLevel = m_SavedLevel;
					m_CameraPosition = { 0, 0 };
					m_CameraFollowPlayer = true;
				}

				ImGui::SameLine();

				if(ImGui::Button("Stop Simulation")) {
					m_GameLevel = m_SavedLevel;
					m_SimulationMode = false;
					m_CameraPosition = { 0, 0 };
					m_CameraFollowPlayer = false;
				}
			}
			else {
				if (ImGui::Button("Start Simulation")) {
					m_GameLevel.optimizeLevel();
					m_SavedLevel = m_GameLevel;
					m_SimulationMode = true;
					m_CameraFollowPlayer = true;
				}
			}

			ImGui::Checkbox("Grid Lock", &m_GridLock);
			ImGui::Checkbox("Draw Mode", &m_DrawMode);
			if (m_DrawMode) {
				ImGui::Text("Draw Object Properties");
				drawObjectProperties(m_DrawObject, false);

			}


			ImGui::End();

			if (saveSettings) {
				setGameSettings(currentSettings);
				currentSettings = m_GameSettings;
			}

			auto& gameObjects = m_GameLevel.getGameObjects();
			uint16_t objectCount = m_GameLevel.getGameObjectCount();

			uint16_t validIndex = 0;

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


			static int32_t selectedIndex = -1;
			int32_t clickedIndex = -1;
			if (gameViewLeftMouseDown || gameViewRightMouseDown) {
				//Core::Logger::Debug("{} {}", relMousePos.x, relMousePos.y);
				validIndex = objectCount;
				for (int32_t i = maxGameObjects - 1; i >= 0 && validIndex >= 0; i--) {
					if (gameObjects[i].flags & GameObjectFlags::Valid) validIndex--;
					else continue;

					if (worldPos.x >= gameObjects[i].position.x && worldPos.x <= gameObjects[i].position.x + gameObjects[i].size.x &&
						worldPos.y >= gameObjects[i].position.y && worldPos.y <= gameObjects[i].position.y + gameObjects[i].size.y) {
						if(gameViewLeftMouseDown) selectedIndex = i;
						clickedIndex = i;
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
				if (gameViewLeftMouseDown && clickedIndex == -1) {
					
					m_DrawObject.position.x = worldPos.x;
					m_DrawObject.position.y = worldPos.y;

					if (m_GridLock) {
						m_DrawObject.position.x = std::floor(m_DrawObject.position.x / 32.0f) * 32.0f;
						m_DrawObject.position.y = std::floor(m_DrawObject.position.y / 32.0f) * 32.0f;
					}

					m_GameLevel.addGameObject(m_DrawObject);
					m_LevelSaved = false;
				}
				if (gameViewRightMouseDown && clickedIndex != -1) {
					m_GameLevel.removeGameObject(clickedIndex);
					selectedIndex = -1;
					m_LevelSaved = false;
				}
			}

			ImGui::End();


			ImGui::Begin("ObjectList");

			if (ImGui::Button("Add Object")) {
				GameObject defaultObj;
				defaultObj.flags = (GameObjectFlags)(GameObjectFlags::Valid | GameObjectFlags::Static);
				defaultObj.position = { 0, 0 };
				defaultObj.size = { 32, 32 };
				strcpy_s(defaultObj.textureName, "");

				defaultObj.collider.position = { 0, 0 };
				defaultObj.collider.size = { 32, 32 };

				m_GameLevel.addGameObject(defaultObj);
				m_LevelSaved = false;
			}

			objectCount = m_GameLevel.getGameObjectCount();

			validIndex = 0;
			if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen, "GameObjects (%d)", objectCount)) {

				for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
					if ((gameObjects[i].flags & GameObjectFlags::Valid) == 0) continue;
					validIndex++;

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

		auto& gameObjects = m_GameLevel.getGameObjects();
		uint16_t objectCount = m_GameLevel.getGameObjectCount();

		uint16_t validIndex = 0;
		//Hintergrund -> Terrain -> Spieler //TODO: Z-Layer im Renderer
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if ((gameObjects[i].flags & GameObjectFlags::Valid) == 0) continue;
			validIndex++;

			if (gameObjects[i].flags & GameObjectFlags::Background) {
				uint32_t textureHandle = renderer->getTextureManager().getTextureIDFromSet(m_TextureSetID, gameObjects[i].textureName);
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, textureHandle);
			}
		}

		validIndex = 0;
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if ((gameObjects[i].flags & GameObjectFlags::Valid) == 0) continue;
			validIndex++;

			if ((gameObjects[i].flags & GameObjectFlags::Background) == 0) {
				uint32_t textureHandle = renderer->getTextureManager().getTextureIDFromSet(m_TextureSetID, gameObjects[i].textureName);
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, textureHandle);
			}
		}

		validIndex = 0;
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if ((gameObjects[i].flags & GameObjectFlags::Valid) == 0) continue;
			validIndex++;

			if (gameObjects[i].flags & GameObjectFlags::Player) {
				uint32_t textureHandle = renderer->getTextureManager().getTextureIDFromSet(m_TextureSetID, gameObjects[i].textureName);
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, textureHandle);
			}
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

		validIndex = 0;
		if (m_GameSettings.showColliders) {
			for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
				uint32_t color = 0x00FF00FF;
				if ((gameObjects[i].flags & GameObjectFlags::Valid) == 0) continue;
				if ((gameObjects[i].flags & GameObjectFlags::Trigger) != 0) color = 0xFF0000FF;
				if ((gameObjects[i].flags & GameObjectFlags::Static) == 0) color = 0x0000FFFF;
				validIndex++;
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, color);
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, color);
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, color);
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, color);
			}
		}

		//renderer->submitLine({ 0, 0 }, { 1280, 720 }, 0xFFFFFFFF);

		renderer->drawRects();
		renderer->drawLines();

		renderer->endRenderTarget(m_Target);

	}



}

