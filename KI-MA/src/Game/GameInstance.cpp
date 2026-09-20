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
		m_CameraFollowPlayer = FollowPlayerAxis::FollowPlayerBoth;
	}

	float maxSpeed = 300.0f;
	float acceleration = 2500.0f;
	float jumpForce = 440.0f;

	float gracePeriod = 0.1f;

	void GameInstance::update(float deltaTime)
	{
		if (!m_Paused) {
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

				if (m_CameraFollowPlayer & FollowPlayerAxis::FollowPlayerX) 
					m_CameraPosition.x = player.position.x + player.size.x / 2;
				
				if (m_CameraFollowPlayer & FollowPlayerAxis::FollowPlayerY) 
					m_CameraPosition.y = player.position.y + player.size.y / 2;

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

	void GameInstance::drawGUI()
	{
		Core::Application* app = Core::Application::getApplication();
		Graphics::Renderer* renderer = app->getRenderer();


		ImGui::Begin("Game Instance");
		auto textureHandle = renderer->getTextureManager().getSRVGPUDescriptorHandle(m_Target->getSRVDescriptorIndex());
		ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(m_Target->getSize().x, m_Target->getSize().y));
		ImGui::End();

		if (m_CloseApplication) {
			app->exit();
		}

		m_MouseDelta.x = m_MousePos.x - m_LastMousePos.x;
		m_MouseDelta.y = m_MousePos.y - m_LastMousePos.y;
		m_LastMousePos = m_MousePos;

		m_MouseWheelDelta.x = 0;
		m_MouseWheelDelta.y = 0;
	}

	void GameInstance::render()
	{
		Graphics::Renderer* renderer = Core::Application::getApplication()->getRenderer();

		for (auto& obj : m_GameLevel.getGameObjects()) {
			uint32_t textureHandle = renderer->getTextureManager().getTextureIDFromSet(m_TextureSetID, obj.textureName);

			float zLayer = 3.0f;
			if (obj.flags & GameObjectFlags::Background) zLayer = 5.0f;
			if (obj.flags & GameObjectFlags::Player) zLayer = 2.0f;

			renderer->submitRect(obj.position, zLayer, obj.size, textureHandle, obj.repeatTexture);
		}
	}



}

