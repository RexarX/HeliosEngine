#include "WindowsInput.h"

#include "HeliosEngine/Application.h"

#include <GLFW/glfw3.h>

namespace Helios {
	std::unique_ptr<Input> Input::m_Instance = std::make_unique<WindowsInput>();

	bool WindowsInput::IsKeyPressedImpl(KeyCode keycode) {
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool WindowsInput::IsMouseButtonPressedImpl(MouseCode button) {
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, button);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> WindowsInput::GetMousePositionImpl() {
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { static_cast<float>(xpos), static_cast<float>(ypos) };
	}

	float WindowsInput::GetMouseXImpl() {
		auto [x, y] = GetMousePositionImpl();
		return x;
	}
	float WindowsInput::GetMouseYImpl() {
		auto [x, y] = GetMousePositionImpl();
		return y;
	}
}