#include "vepch.h"

#include "WindowsWindow.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>

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
      int success = glfwInit();
      VE_CORE_ASSERT(success, "Could not initialize GLFW!");
      glfwSetErrorCallback(GLFWErrorCallback);
      s_GLFWInitialized = true;
    }

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(),
																nullptr, nullptr);
 
		m_Context = std::make_unique<OpenGLContext>(m_Window);
		m_Context->Init();
		m_Context->SetViewport(props.Width, props.Height);

		glfwSetWindowUserPointer(m_Window, &m_Data);

		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		SetVSync(true);
		SetMinimized(false);
		SetFramerate(0.0);

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, const int width,
															const int height)
      {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;
				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, const int key, const int scancode,
											 const int action, const int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}

				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}

				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, const int button,
															 const int action, const int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}

				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}

				case GLFW_REPEAT:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, const double xOffset,
													const double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, const double xPos,
														 const double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
  }

  void WindowsWindow::Shutdown()
  {
		glfwDestroyWindow(m_Window);
  }

	void WindowsWindow::ClearBuffer() 
	{
		m_Context->ClearBuffer();
	}

  void WindowsWindow::OnUpdate()
  {
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
		ClearBuffer();
  }

  void WindowsWindow::SetVSync(const bool enabled)
  {
		if (enabled) { glfwSwapInterval(1); }
		else { glfwSwapInterval(0); }

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
		}
		else {
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		m_Data.Focus = enabled;
	}

  void WindowsWindow::SetFramerate(const double framerate)
  {
    m_Data.Framerate = framerate;
  }

	void WindowsWindow::SetLastFramerate(const double framerate)
	{
		m_Data.LastFramerate = framerate;
	}

	double WindowsWindow::GetFramerate() const
	{
		return m_Data.Framerate;
	}

	double WindowsWindow::GetLastFramerate() const
	{
		return m_Data.LastFramerate;
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
}