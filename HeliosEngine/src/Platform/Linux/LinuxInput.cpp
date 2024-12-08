#include "LinuxInput.h"

#include "HeliosEngine/Application.h"

#include <GLFW/glfw3.h>

namespace Helios {
	bool LinuxInput::IsKeyPressedImpl(KeyCode keycode) const {
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool LinuxInput::IsMouseButtonPressedImpl(MouseCode button) const {
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, button);
		return state == GLFW_PRESS;
	}

	std::pair<uint32_t, uint32_t> LinuxInput::GetMousePositionImpl() const {
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos(0.0), ypos(0.0);
		glfwGetCursorPos(window, &xpos, &ypos);
		return { static_cast<uint32_t>(xpos), static_cast<uint32_t>(ypos) };
	}
}