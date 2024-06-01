#include "vepch.h"

#include "WindowsWindow.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include "Render/RendererAPI.h"

#include <GLFW/glfw3.h>
#include <Vulkan/VulkanContext.h>

namespace VoxelEngine
{
  static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(const int error, const char* description)
	{
		VE_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

  std::unique_ptr<Window> Window::Create(const WindowProps& props)
  {
    return std::make_unique<WindowsWindow>(props);
  }

  WindowsWindow::WindowsWindow(const WindowProps& props)
  {
		Init(props);
  }

  WindowsWindow::~WindowsWindow()
  {
    Shutdown();
  }

  void WindowsWindow::Init(const WindowProps& props)
  {
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		VE_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);
		
    if (!s_GLFWInitialized) {
			auto result = glfwInit();
			VE_CORE_ASSERT(result, "Could not initialize GLFW!");
      glfwSetErrorCallback(GLFWErrorCallback);
      s_GLFWInitialized = true;
    }

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan) { glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); }
		
		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(),
																nullptr, nullptr);

		m_Monitor = glfwGetPrimaryMonitor();
		m_Mode = glfwGetVideoMode(m_Monitor);

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();
		m_Context->SetViewport(props.Width, props.Height);

		glfwSetWindowUserPointer(m_Window, reinterpret_cast<void*>(this));

		SetVSync(true);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			WindowsWindow& win = *(WindowsWindow*)glfwGetWindowUserPointer(window);
			win.m_Data.Width = width;
      win.m_Data.Height = height;
			WindowResizeEvent event(width, height);
      win.m_Data.EventCallback(event);

      win.m_Context->SetResized(true);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
        WindowsWindow& win = *(WindowsWindow*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				win.m_Data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, const int key, const int scancode,
											 const int action, const int mods)
			{
        WindowsWindow& win = *(WindowsWindow*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					win.m_Data.EventCallback(event);
					break;
				}

				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					win.m_Data.EventCallback(event);
					break;
				}

				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					win.m_Data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, const int button,
															 const int action, const int mods)
			{
				WindowsWindow& win = *(WindowsWindow*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					win.m_Data.EventCallback(event);
					break;
				}

				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					win.m_Data.EventCallback(event);
					break;
				}

				case GLFW_REPEAT:
				{
					MouseButtonPressedEvent event(button);
					win.m_Data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, const double xOffset,
													const double yOffset)
			{
				WindowsWindow& win = *(WindowsWindow*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				win.m_Data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, const double xPos,
														 const double yPos)
			{
				WindowsWindow& win = *(WindowsWindow*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				win.m_Data.EventCallback(event);
			});
  }

  void WindowsWindow::Shutdown()
  {
		m_Context->Shutdown();
		glfwDestroyWindow(m_Window);
  }

	void WindowsWindow::SwapBuffers()
	{
		m_Context->SwapBuffers();
	}

	void WindowsWindow::ClearBuffer()
	{
		m_Context->ClearBuffer();
	}

	void WindowsWindow::PoolEvents()
	{
		glfwPollEvents();
	}

  void WindowsWindow::OnUpdate()
  {
		m_Context->Update();
  }

  void WindowsWindow::InitImGui()
  {
    m_Context->InitImGui();
  }

	void WindowsWindow::ShutdownImGui()
	{
		m_Context->ShutdownImGui();
	}

	void WindowsWindow::Begin()
	{
		m_Context->Begin();
	}

	void WindowsWindow::End()
	{
    m_Context->End();
	}

  void WindowsWindow::SetVSync(const bool enabled)
  {
		m_Context->SetVSync(enabled);
		m_Data.VSync = enabled;
  }

  void WindowsWindow::SetMinimized(const bool enabled)
  {
		m_Data.Minimized = enabled;
  }

	void WindowsWindow::SetFocused(const double enabled)
	{
		if (enabled) {
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else {
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		m_Data.Focus = enabled;
	}

	void WindowsWindow::SetFullscreen(const bool enabled)
	{
		if (enabled) {
			uint32_t width, height;
			width = m_Data.Width;
      height = m_Data.Height;

			int xpos, ypos;
			glfwGetWindowPos(m_Window, &xpos, &ypos);

			m_Data.posX = xpos;
			m_Data.posY = ypos;

			glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, m_Mode->width, m_Mode->height, m_Mode->refreshRate);

			m_Data.Width = width;
      m_Data.Height = height;
			m_Data.Fullscreen = enabled;
		} else {
			glfwSetWindowMonitor(m_Window, nullptr, m_Data.posX, m_Data.posX, m_Data.Width, m_Data.Height, GLFW_DONT_CARE);
		}

		m_Data.Fullscreen = enabled;
	}

  void WindowsWindow::SetFramerate(const double framerate)
  {
    m_Data.Framerate = framerate;
  }

	void WindowsWindow::SetImGuiState(const bool enabled)
	{
		m_Context->SetImGuiState(enabled);
    m_Data.ShowImGui = enabled;
	}

	double WindowsWindow::GetFramerate() const
	{
		return m_Data.Framerate;
	}

  bool WindowsWindow::IsVSync() const
  {
		return m_Data.VSync;
  }

	bool WindowsWindow::IsMinimized() const
	{
		return m_Data.Minimized;
	}

	bool WindowsWindow::IsFocused() const
	{
		return m_Data.Focus;
	}

	bool WindowsWindow::IsFullscreen() const
	{
		return m_Data.Fullscreen;
	}

	bool WindowsWindow::IsImGuiEnabled() const
	{
		return m_Data.ShowImGui;
	}
}