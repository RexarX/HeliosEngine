#include "WindowsWindow.h"	

#include "Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace Helios {
  static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description) {
		CORE_ERROR("GLFW Error ({}): {}", error, description);
	}

  WindowsWindow::WindowsWindow(std::string_view title, uint32_t width, uint32_t height) {
		Init(title, width, height);
  }

  WindowsWindow::~WindowsWindow() {
    Shutdown();
  }

  void WindowsWindow::Init(std::string_view title, uint32_t width, uint32_t height) {
		PROFILE_FUNCTION();

		m_Data.title = title;
		m_Data.width = width;
		m_Data.height = height;

		CORE_INFO("Creating window {} ({}, {})", title, width, height);

		if (!s_GLFWInitialized) {
			int result = glfwInit();
			CORE_ASSERT_CRITICAL(result, "Failed to initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_Window = glfwCreateWindow((int)width, (int)height, m_Data.title.c_str(), nullptr, nullptr);
		m_Monitor = glfwGetPrimaryMonitor();

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();
		m_Context->SetViewport(width, height);

		glfwSetWindowUserPointer(m_Window, reinterpret_cast<void*>(this));

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
				WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
				win.m_Data.width = width;
				win.m_Data.height = height;

				WindowResizeEvent event(width, height);
				win.m_Context->SetViewport(width, height);
				win.m_Data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
				WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
				WindowCloseEvent event;
				win.m_Data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
				WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				switch (action) {
					case GLFW_PRESS: {
						KeyPressEvent event(key, 0);
						win.m_Data.EventCallback(event);
						break;
					}

					case GLFW_RELEASE: {
						KeyReleaseEvent event(key);
						win.m_Data.EventCallback(event);
						break;
					}

					case GLFW_REPEAT: {
							KeyPressEvent event(key, 1);
							win.m_Data.EventCallback(event);
							break;
					}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
				WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				switch (action) {
					case GLFW_PRESS: {
						MouseButtonPressEvent event(button);
						win.m_Data.EventCallback(event);
						break;
					}

					case GLFW_RELEASE: {
						MouseButtonReleaseEvent event(button);
						win.m_Data.EventCallback(event);
						break;
					}

					case GLFW_REPEAT: {
						MouseButtonPressEvent event(button);
						win.m_Data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
				WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				MouseScrollEvent event((float)xOffset, (float)yOffset);
				win.m_Data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
				WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				MouseMoveEvent event((float)xPos, (float)yPos);
				win.m_Data.EventCallback(event);
			});
  }

  void WindowsWindow::Shutdown() {
		PROFILE_FUNCTION();
		m_Context->Shutdown();
		glfwDestroyWindow(m_Window);
  }

	void WindowsWindow::PoolEvents() {
		PROFILE_FUNCTION();
		glfwPollEvents();
	}

	void WindowsWindow::OnUpdate() {
		m_Context->Update();
	}

	void WindowsWindow::BeginFrame() {
		m_Context->BeginFrame();
	}

	void WindowsWindow::EndFrame() {
		m_Context->EndFrame();
	}

	void WindowsWindow::InitImGui() {
		m_Context->InitImGui();
	}

	void WindowsWindow::ShutdownImGui() {
		m_Context->ShutdownImGui();
	}

	void WindowsWindow::BeginFrameImGui() {
		m_Context->BeginFrameImGui();
	}

	void WindowsWindow::EndFrameImGui() {
		m_Context->EndFrameImGui();
	}

  void WindowsWindow::SetVSync(bool enabled) {
		m_Context->SetVSync(enabled);
		m_Data.vsync = enabled;
  }

	void WindowsWindow::SetFocused(double enabled) {
		if (enabled) {
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			WindowFocusEvent event;
			m_Data.EventCallback(event);
		} else {
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			WindowLostFocusEvent event;
			m_Data.EventCallback(event);
		}
		
		m_Data.focused = enabled;
	}

	void WindowsWindow::SetFullscreen(bool enabled) {
		if (enabled) {
			uint32_t width, height;
			width = m_Data.width;
      height = m_Data.height;

			int xpos, ypos;
			glfwGetWindowPos(m_Window, &xpos, &ypos);

			m_Data.posX = xpos;
			m_Data.posY = ypos;

			glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, m_Mode->width, m_Mode->height, m_Mode->refreshRate);

			m_Data.width = width;
      m_Data.height = height;
		} else {
			glfwSetWindowMonitor(m_Window, nullptr, m_Data.posX, m_Data.posX, m_Data.width, m_Data.height, GLFW_DONT_CARE);
		}

		m_Data.fullscreen = enabled;
	}

	void WindowsWindow::SetImGuiState(bool enabled) {
		m_Context->SetImGuiState(enabled);
    m_Data.showImGui = enabled;
	}
}