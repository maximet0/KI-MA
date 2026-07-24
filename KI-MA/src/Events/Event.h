#pragma once
#include <cstdint>
#include <DirectXMath.h>

#include "Core/Window.h"

namespace Events {

	enum EventType {
		UNKNOWN,
		WINDOW_RESIZE,
		WINDOW_MOVE,
		WINDOW_CLOSE,
		MOUSE_LEAVE,
		MOUSE_ENTER,
		MOUSE_POS,
		MOUSE_BUTTON,
		MOUSE_WHEEL,
		KEYBOARD_CHAR,
		KEYBOARD_KEY,
	};

	enum KeyState {
		Clicked,
		Down,
		Up
	};

	enum MouseButton {
		Left,
		Middle,
		Right,
		Extra1,
		Extra2
	};

	enum KeyboardKey : uint8_t {
		Key_Unknown = 0x00,
		Key_BackSpace = 0x08, Key_Tab,
		Key_Enter = 0x0D,
		Key_Pause = 0x13, Key_Caps,
		Key_Esc = 0x1B, Key_Convert, Key_NonConvert, Key_Accept, Key_ModeChange,
		Key_Space, Key_PageUp, Key_PageDown, Key_End, Key_Home, Key_Left, Key_Up, Key_Right, Key_Down, Key_Select,
		Key_Print, Key_Execute, Key_PrintScr, Key_Insert, Key_Delete, Key_Help,
		Key_0, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
		Key_SemiColon = 0x3B,
		Key_A = 0x41, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J,
		Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T, Key_U,
		Key_V, Key_W, Key_X, Key_Y, Key_Z,
		Key_LWin, Key_RWin, Key_Apps, Key_Sleep,
		Key_Num0 = 0x60, Key_Num1, Key_Num2, Key_Num3, Key_Num4, Key_Num5, Key_Num6, Key_Num7, Key_Num8, Key_Num9,
		Key_Multiply, Key_Add, Key_Seperator, Key_Subtract, Key_Decimal, Key_Divide,
		Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
		Key_F13, Key_F14, Key_F15, Key_F16, Key_F17, Key_F18, Key_F19, Key_F20, Key_F21, Key_F22, Key_F23, Key_F24,
		Key_NumLock = 0x90, Key_Scroll, Key_NumEqual,
		Key_LSHIFT = 0xA0, Key_RSHIFT, Key_LCONTROL, Key_RControl, Key_LALT, Key_RALT,
		Key_Quote = 0xDE,
		Key_Equal = 0xBB, Key_Comma, Key_Dash, Key_Period, Key_Slash, Key_Grave,
		Key_LBracket = 0xDB, Key_BackSlash, Key_RBracket,
	};

	class Event {
	public:
		EventType type;
		explicit Event(EventType type) : type(type) {}
	};

	// --- Window Events ---
	class WindowResizeEvent : public Event {
	public:
		Core::Window* window;
		DirectX::XMINT2 viewportSize;

		WindowResizeEvent(Core::Window* w, DirectX::XMINT2 size)
			: Event(EventType::WINDOW_RESIZE), window(w), viewportSize(size) {
		}
	};

	class WindowMoveEvent : public Event {
	public:
		Core::Window* window;
		DirectX::XMINT2 viewportPos;

		WindowMoveEvent(Core::Window* w, DirectX::XMINT2 pos)
			: Event(EventType::WINDOW_MOVE), window(w), viewportPos(pos) {
		}
	};

	class WindowCloseEvent : public Event {
	public:
		Core::Window* window;

		explicit WindowCloseEvent(Core::Window* w)
			: Event(EventType::WINDOW_CLOSE), window(w) {
		}
	};

	// --- Mouse Events ---
	class MouseLeaveEvent : public Event {
	public:
		Core::Window* window;

		explicit MouseLeaveEvent(Core::Window* w)
			: Event(EventType::MOUSE_LEAVE), window(w) {
		}
	};

	class MouseEnterEvent : public Event {
	public:
		Core::Window* window;

		explicit MouseEnterEvent(Core::Window* w)
			: Event(EventType::MOUSE_ENTER), window(w) {
		}
	};

	class MousePosEvent : public Event {
	public:
		Core::Window* window;
		DirectX::XMINT2 mousePos;
		DirectX::XMINT2 absoluteMousePos;

		MousePosEvent(Core::Window* w, DirectX::XMINT2 pos, DirectX::XMINT2 absPos)
			: Event(EventType::MOUSE_POS), window(w), mousePos(pos), absoluteMousePos(absPos) {
		}
	};

	class MouseButtonEvent : public Event {
	public:
		Core::Window* window;
		KeyState state;
		MouseButton button;

		MouseButtonEvent(Core::Window* w, KeyState s, MouseButton b)
			: Event(EventType::MOUSE_BUTTON), window(w), state(s), button(b) {
		}
	};

	class MouseWheelEvent : public Event {
	public:
		Core::Window* window;
		DirectX::XMINT2 wheelDelta;

		MouseWheelEvent(Core::Window* w, DirectX::XMINT2 delta)
			: Event(EventType::MOUSE_WHEEL), window(w), wheelDelta(delta) {
		}
	};

	// --- Keyboard Events ---
	class KeyboardCharEvent : public Event {
	public:
		Core::Window* window;
		wchar_t c;

		KeyboardCharEvent(Core::Window* w, wchar_t ch)
			: Event(EventType::KEYBOARD_CHAR), window(w), c(ch) {
		}
	};

	class KeyboardKeyEvent : public Event {
	public:
		Core::Window* window;
		KeyState state;
		KeyboardKey key;
		int scancode;

		KeyboardKeyEvent(Core::Window* w, KeyState s, KeyboardKey k, int sc)
			: Event(EventType::KEYBOARD_KEY), window(w), state(s), key(k), scancode(sc) {
		}
	};

}