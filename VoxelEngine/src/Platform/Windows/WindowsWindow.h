#pragma once

#include "Window.h"

#include "Render/GraphicsContext.h"

#include "Events/KeyEvent.h"

struct GLFWwindow;

namespace VoxelEngine
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void SwapBuffers() override;
		void ClearBuffer() override;
		void PoolEvents() override;
		void OnUpdate() override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetVSync(const bool enabled) override;
		void SetMinimized(const bool enabled) override;
		void SetFocused(const double enabled) override;
		void SetFramerate(const double framerate) override;
		void SetLastFramerate(const double framerate) override;

		double GetFramerate() const override;
		double GetLastFramerate() const override;

		bool IsVSync() const override;
		bool IsMinimized() const override;
		bool IsFocused() const override;

		inline virtual void* GetNativeWindow() const { return m_Window; }

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		static void OnResize(GLFWwindow* window, const int width, const int height);

	private:
		GLFWwindow* m_Window;
		static std::unique_ptr<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			double Framerate;
			double LastFramerate;
			bool VSync;
			bool Minimized;
			bool Focus;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}