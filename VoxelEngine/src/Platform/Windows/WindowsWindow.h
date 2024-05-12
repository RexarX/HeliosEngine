#pragma once

#include "Window.h"

#include "Render/GraphicsContext.h"

#include "Events/KeyEvent.h"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

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

		void InitImGui() override;
		void ShutdownImGui() override;
		void Begin() override;
		void End() override;

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetVSync(const bool enabled) override;
		void SetMinimized(const bool enabled) override;
		void SetFocused(const double enabled) override;
		void SetFullscreen(const bool enabled) override;
		void SetFramerate(const double framerate) override;
		void SetImGuiState(const bool enabled) override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }

		double GetFramerate() const override;

		bool IsVSync() const override;
		bool IsMinimized() const override;
		bool IsFocused() const override;
		bool IsFullscreen() const override;
		bool IsImGuiEnabled() const override;

		inline virtual void* GetNativeWindow() const { return m_Window; }

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

	private:
		GLFWwindow* m_Window;
		GLFWmonitor* m_Monitor;
		const GLFWvidmode* m_Mode;
		std::unique_ptr<GraphicsContext> m_Context;

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