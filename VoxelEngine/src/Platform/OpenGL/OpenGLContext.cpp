#include "OpenGLContext.h"

#include "vepch.h"

#include <glad/glad.h>

#include <glfw/glfw3.h>

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

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		VE_CORE_INFO("OpenGL Info:");
		VE_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		VE_CORE_INFO("  GPU: {0}", (const char*)glGetString(GL_RENDERER));
		VE_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));
	}

	void OpenGLContext::Shutdown()
	{
	}

	void OpenGLContext::Update()
	{
		SwapBuffers();
		ClearBuffer();
	}

	void OpenGLContext::SwapBuffers()
	{
    glfwSwapBuffers(m_WindowHandle);
	}

	void OpenGLContext::ClearBuffer()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLContext::SetViewport(const uint32_t width, const uint32_t height)
	{
    glViewport(0, 0, width, height);
	}
}