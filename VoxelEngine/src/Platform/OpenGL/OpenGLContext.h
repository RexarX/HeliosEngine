#pragma once

#include "Render/GraphicsContext.h"

struct GLFWwindow;

namespace VoxelEngine
{
	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void Update() override;
		virtual void SwapBuffers() override;
		virtual void ClearBuffer() override;
		virtual void SetViewport(const uint32_t width, const uint32_t height) override;

		virtual void InitImGui() override;
    virtual void ShutdownImGui() override;
		virtual void Begin() override;
		virtual void End() override;

		virtual void SetVSync(const bool enabled) override;
		virtual void SetResized(const bool resized) override;

		virtual void SetImGuiState(const bool enabled) override;

	private:
		GLFWwindow* m_WindowHandle;
	};
}