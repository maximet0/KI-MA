#pragma once
#include <functional>
#include <DirectXMath.h>

#include "Event.h"


namespace Events {
	typedef std::function<void(Core::Window* window, DirectX::XMINT2 viewportSize)> WindowResizeCallback;
	typedef std::function<void(Core::Window* window, DirectX::XMINT2 viewportSize)> WindowMoveCallback;
	typedef std::function<void(Core::Window* window)> WindowCloseCallback;

	typedef std::function<void(Core::Window* window)> MouseLeaveCallback;
	typedef std::function<void(Core::Window* window)> MouseEnterCallback;
	typedef std::function<void(Core::Window* window, DirectX::XMINT2 mousePos, DirectX::XMINT2 aMousePos)> MousePosCallback;
	typedef std::function<void(Core::Window* window, KeyState state, MouseButton button)> MouseButtonCallback;
	typedef std::function<void(Core::Window* window, DirectX::XMINT2 wheelDelta)> MouseWheelCallback;

	typedef std::function<void(Core::Window* window, wchar_t c)> KeyboardCharCallback;
	typedef std::function<void(Core::Window* window, KeyState c, KeyboardKey key, int scancode)> KeyboardKeyCallback;

}