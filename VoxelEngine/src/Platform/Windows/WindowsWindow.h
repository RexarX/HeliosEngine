#pragma once

#include "Window.h"

#include "Renderer/GraphicsContext.h"

#include "Events/KeyEvent.h"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace Engine
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void PoolEvents() override;
		void OnUpdate() override;

		void InitImGui() override;
		void ShutdownImGui() override;
		void BeginFrameImGui() override;
		void EndFrameImGui() override;

		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetVSync(const bool enabled) override;
		void SetMinimized(const bool enabled) override;
		void SetFocused(const double enabled) override;
		void SetFullscreen(const bool enabled) override;
		void SetFramerate(const double framerate) override;
		void SetImGuiState(const bool enabled) override;

		inline const uint32_t GetWidth() const override { return m_Data.Width; }
		inline const uint32_t GetHeight() const override { return m_Data.Height; }

		inline const double GetFramerate() const override;

		inline const bool IsVSync() const override;
		inline const bool IsMinimized() const override;
		inline const bool IsFocused() const override;
		inline const bool IsFullscreen() const override;
		inline const bool IsImGuiEnabled() const override;

		virtual inline void* GetNativeWindow() const { return m_Window; }

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

	private:
		GLFWwindow* m_Window;
		GLFWmonitor* m_Monitor;
		const GLFWvidmode* m_Mode;

		std::shared_ptr<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			uint32_t posX, posY;
			double Framerate = 0.0;
			bool VSync = true;
			bool Minimized = false;
			bool Focus = false;
			bool Fullscreen = false;
			bool ShowImGui = false;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}