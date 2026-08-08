#include "GameInstance.h"

#include "Core/Application.h"

#include "external/ImGui/ImGui.h"
#include "Core/Logger.h"
#include "Events/Callbacks.h"
#include "GamePhysics.h"

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
				keyDown[(int)key] = true;
				//Core::Logger::Debug("Key Pressed: {}", (int)key);
			}
			else if(c == Events::KeyState::Up) {
				keyDown[(int)key] = false;
				//Core::Logger::Debug("Key Released: {}", (int)key);
			}
		});

		m_DrawObject.type = GameObjectType::Background;
		m_DrawObject.position = { 0, 0 };
		m_DrawObject.size = { 32, 32 };
		m_DrawObject.textureHandle = 1;

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
					if (m_GameLevel.getGameObjects()[i].type == GameObjectType::Player) {
						playerID = i;
						break;
					}
				}
			}

			static bool grounded = false;

			if (playerID != -1) {
				GameObject& player = m_GameLevel.getGameObjects()[playerID];
				player.physics.acceleration.x = 0.0f;


				if (keyDown[(int)Events::KeyboardKey::Key_W] && (grounded || gracePeriod > 0.0f)) {
					player.physics.velocity.y = -jumpForce;
				}
				if (keyDown[(int)Events::KeyboardKey::Key_A]) {
					player.physics.acceleration.x = -acceleration;
				}
				else if (keyDown[(int)Events::KeyboardKey::Key_D]) {
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


				if (!keyDown[(int)Events::KeyboardKey::Key_A] && !keyDown[(int)Events::KeyboardKey::Key_D])
				{
					if (std::abs(player.physics.velocity.x) < deceleration * deltaTime)
						player.physics.velocity.x = 0;
				}

				player.physics.velocity.x = std::clamp(player.physics.velocity.x, -maxSpeed, maxSpeed);


			}

			GamePhysics::updatePhysics(m_GameLevel, deltaTime, m_Gravity);

			if (playerID != -1) {
				//Ground Check
				GameObject& player = m_GameLevel.getGameObjects()[playerID];
				for (uint16_t i = 0; i < 16; i++) {
					float xPos = player.position.x + (player.size.x / 16) * i;
					RaycastHit hit = GamePhysics::raycast(m_GameLevel, { xPos, player.position.y + player.size.y + 0.02f }, { 0, 1 }, 5.0f);
					grounded = hit.hit;
					if (grounded) {
						gracePeriod = 0.1f;
						break;
					}
				}
				
				if(!grounded) gracePeriod -= deltaTime;
				
			}
			

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
	
		if (ImGui::Combo("Type", (int*)&obj.type, "Invalid\0Background\0Terrain\0Player\0")) {
			modified += 1;
		}

		if (ImGui::Checkbox("Static", &obj.isStatic)) {
			modified += 1;
		}

		modified += ImGui::InputInt("Texture Handle", (int*)&obj.textureHandle);

		ImGui::Text("Collider Properties");
		ImGui::PushID("Collider");
		modified += ImGui::DragFloat2("Position", &obj.collider.position.x, 0.5f);
		modified += ImGui::DragFloat2("Size", &obj.collider.size.x, 0.5f);
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
			ImGui::Text("Level Editor");

			if (m_SimulationMode) {
				if (ImGui::Button(m_Paused ? "Resume Simulation" : "Pause Simulation")) {
					m_Paused = !m_Paused;
				}

				ImGui::SameLine();

				if (ImGui::Button("Reset Simulation")) {
					m_GameLevel = m_SavedLevel;
				}

				ImGui::SameLine();

				if(ImGui::Button("Stop Simulation")) {
					m_GameLevel = m_SavedLevel;
					m_SimulationMode = false;
					
				}
			}
			else {
				if (ImGui::Button("Start Simulation")) {
					m_GameLevel.optimizeLevel();
					m_SavedLevel = m_GameLevel;
					m_SimulationMode = true;
				}
			}

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
			bool gameViewLeftMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
			bool gameViewRightMouseDown = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right);
			mousePos = ImGui::GetMousePos();

			//Berechne die Position der Maus relativ zum RenderTarget
			ImVec2 relMousePos = { mousePos.x - offset.x, mousePos.y - offset.y };

			static int32_t selectedIndex = -1;
			int32_t clickedIndex = -1;
			if (gameViewLeftMouseDown || gameViewRightMouseDown) {
				//Core::Logger::Debug("{} {}", relMousePos.x, relMousePos.y);
				validIndex = objectCount;
				for (int32_t i = maxGameObjects - 1; i >= 0 && validIndex >= 0; i--) {
					if (gameObjects[i].type != GameObjectType::Invalid) validIndex--;
					else continue;

					if (relMousePos.x >= gameObjects[i].position.x && relMousePos.x <= gameObjects[i].position.x + gameObjects[i].size.x &&
						relMousePos.y >= gameObjects[i].position.y && relMousePos.y <= gameObjects[i].position.y + gameObjects[i].size.y) {
						if(gameViewLeftMouseDown) selectedIndex = i;
						clickedIndex = i;
						break;
					}
				}
			}

			if (m_DrawMode) {
				//ImGui::
				if (gameViewLeftMouseDown && clickedIndex == -1) {
					
					m_DrawObject.position.x = mousePos.x - offset.x;
					m_DrawObject.position.y = mousePos.y - offset.y;

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
				defaultObj.type = GameObjectType::Background;
				defaultObj.position = { 0, 0 };
				defaultObj.size = { 32, 32 };
				defaultObj.textureHandle = 1;

				defaultObj.collider.position = { 0, 0 };
				defaultObj.collider.size = { 32, 32 };

				m_GameLevel.addGameObject(defaultObj);
				m_LevelSaved = false;
			}

			objectCount = m_GameLevel.getGameObjectCount();

			validIndex = 0;
			if (ImGui::TreeNodeEx("GameObjects", ImGuiTreeNodeFlags_DefaultOpen, "GameObjects (%d)", objectCount)) {

				for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
					if (gameObjects[i].type == GameObjectType::Invalid) continue;
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
			if (gameObjects[i].type == GameObjectType::Invalid) continue;
			validIndex++;

			if (gameObjects[i].type == GameObjectType::Background) {
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, gameObjects[i].textureHandle);
			}
		}

		validIndex = 0;
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if (gameObjects[i].type == GameObjectType::Invalid) continue;
			validIndex++;

			if (gameObjects[i].type == GameObjectType::Terrain) {
				renderer->submitRect(gameObjects[i].position, gameObjects[i].size, gameObjects[i].textureHandle);
			}
		}

		validIndex = 0;
		for (uint16_t i = 0; i < maxGameObjects && validIndex < objectCount; i++) {
			if (gameObjects[i].type == GameObjectType::Invalid) continue;
			validIndex++;

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
				if (gameObjects[i].type == GameObjectType::Invalid) continue;
				validIndex++;
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, 0x00FF00FF);
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, 0x00FF00FF);
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x + gameObjects[i].collider.size.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, 0x00FF00FF);
				renderer->submitLine({ gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y + gameObjects[i].collider.size.y }, { gameObjects[i].position.x + gameObjects[i].collider.position.x, gameObjects[i].position.y + gameObjects[i].collider.position.y }, 0x00FF00FF);
			}
		}

		//renderer->submitLine({ 0, 0 }, { 1280, 720 }, 0xFFFFFFFF);

		renderer->drawRects();
		renderer->drawLines();

		renderer->endRenderTarget(m_Target);

	}



}

