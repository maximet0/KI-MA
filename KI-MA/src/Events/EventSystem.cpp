#include "EventSystem.h"

#include "Callbacks.h"

namespace Events {

	void EventSystem::registerEvent(Event* event)
	{
		m_EventQueue.push_back(event);
	}

	void EventSystem::removeCallback(uint32_t callbackID)
	{
		m_Callbacks.erase(callbackID);
	}

	void EventSystem::pollEvents()
	{
		for (Event* e : m_EventQueue) {
			for (auto& callback : m_Callbacks) {
				if (callback.second.type == e->type) {
					switch (e->type) {
					case EventType::WINDOW_RESIZE: {
						WindowResizeEvent* wre = static_cast<WindowResizeEvent*>(e);
						std::any_cast<WindowResizeCallback>(callback.second.callback)(wre->window, wre->viewportSize);
						break;
					}
					case EventType::WINDOW_MOVE: {
						WindowMoveEvent* wme = static_cast<WindowMoveEvent*>(e);
						std::any_cast<WindowMoveCallback>(callback.second.callback)(wme->window, wme->viewportPos);
						break;
					}
					case EventType::WINDOW_CLOSE: {
						WindowCloseEvent* wce = static_cast<WindowCloseEvent*>(e);
						std::any_cast<WindowCloseCallback>(callback.second.callback)(wce->window);
						break;
					}	
					case EventType::MOUSE_LEAVE: {
						MouseLeaveEvent* mle = static_cast<MouseLeaveEvent*>(e);
						std::any_cast<MouseLeaveCallback>(callback.second.callback)(mle->window);
						break;

					}
					case EventType::MOUSE_ENTER: {
						MouseEnterEvent* mee = static_cast<MouseEnterEvent*>(e);
						std::any_cast<MouseEnterCallback>(callback.second.callback)(mee->window);
						break;
					}
					case EventType::MOUSE_POS: {
						MousePosEvent* mpe = static_cast<MousePosEvent*>(e);
						std::any_cast<MousePosCallback>(callback.second.callback)(mpe->window, mpe->mousePos, mpe->absoluteMousePos);
						break;
					}
					case EventType::MOUSE_BUTTON: {
						MouseButtonEvent* mbe = static_cast<MouseButtonEvent*>(e);
						std::any_cast<MouseButtonCallback>(callback.second.callback)(mbe->window, mbe->state, mbe->button);
						break;
					}
					case EventType::MOUSE_WHEEL: {
						MouseWheelEvent* mwe = static_cast<MouseWheelEvent*>(e);
						std::any_cast<MouseWheelCallback>(callback.second.callback)(mwe->window, mwe->wheelDelta);
						break;
					}
					case EventType::KEYBOARD_CHAR: {
						KeyboardCharEvent* kce = static_cast<KeyboardCharEvent*>(e);
						std::any_cast<KeyboardCharCallback>(callback.second.callback)(kce->window, kce->c);
						break;
					}
					case EventType::KEYBOARD_KEY: {
						KeyboardKeyEvent* kke = static_cast<KeyboardKeyEvent*>(e);
						std::any_cast<KeyboardKeyCallback>(callback.second.callback)(kke->window, kke->state, kke->key, kke->scancode);
						break;
					}
					default:
						break;
					}
				}
			}
		}

		m_EventQueue.clear();
	}
}


