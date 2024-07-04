#include "OpenGLContext.h"

#include "vepch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <glad/glad.h>

#include <glfw/glfw3.h>

namespace VoxelEngine
{
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);

		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		CORE_ASSERT(status, "Failed to initialize Glad!");

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		CORE_INFO("OpenGL Info:");
		CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		CORE_INFO("  GPU: {0}", (const char*)glGetString(GL_RENDERER));
		CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));
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

	void OpenGLContext::InitImGui()
	{
		ImGui_ImplGlfw_InitForOpenGL(m_WindowHandle, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void OpenGLContext::ShutdownImGui()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
	}

	void OpenGLContext::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void OpenGLContext::End()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		GLFWwindow* backup_current_context = glfwGetCurrentContext();

		#ifdef VE_PLATFORM_WINDOWS
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
		#endif

		glfwMakeContextCurrent(backup_current_context);
	}

	void OpenGLContext::SetVSync(const bool enabled)
	{
		glfwSwapInterval(enabled ? 1 : 0);
	}

	void OpenGLContext::SetResized(const bool resized)
	{
	}

	void OpenGLContext::SetImGuiState(const bool enabled)
	{
	}
}