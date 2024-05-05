#pragma once

#include "VoxelEngine/Render/GraphicsContext.h"

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

		virtual void SetVSync(const bool enabled) override;

		virtual void SetResized(const bool resized) override;

	private:
		GLFWwindow* m_WindowHandle;
	};
}