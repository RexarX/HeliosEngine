#include "vepch.h"

#include "OpenGLContext.h"

#include <GLFW/glfw3.h>

#include <GLAD/glad.h>

#include <GL/GL.h>

namespace VoxelEngine
{
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		VE_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		VE_CORE_ASSERT(status, "Failed to initialize Glad!");

		VE_CORE_INFO("OpenGL Info:");
		//VE_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		//VE_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		//VE_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}
}