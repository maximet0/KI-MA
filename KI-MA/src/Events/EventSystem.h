#pragma once

#include "Event.h"
#include <any>
#include <unordered_map>

namespace Events {

	struct EventCallback {
		EventType type = EventType::UNKNOWN;
		std::any callback;
	};

	class EventSystem {
	public:
		void registerEvent(Event* event);
		
		template<typename T>
		uint32_t registerCallback(EventType type, T callback) {
			m_Callbacks[m_CurCallbackID++] = { type, callback };
			return m_CurCallbackID - 1;
		}

		void removeCallback(uint32_t callbackID);

		void pollEvents();

	private:
		std::unordered_map<uint64_t, EventCallback> m_Callbacks;
		std::vector<Event*> m_EventQueue;

		uint32_t m_CurCallbackID = 0;

	};
}