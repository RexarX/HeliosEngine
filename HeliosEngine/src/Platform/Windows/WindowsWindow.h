#pragma once

#include "Window.h"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace Helios
{
	class GraphicsContext;

	class WindowsWindow final : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void PoolEvents() override;
		void OnUpdate() override;
		void BeginFrame() override;
		void EndFrame() override;

		void InitImGui() override;
		void ShutdownImGui() override;
		void BeginFrameImGui() override;
		void EndFrameImGui() override;

		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetVSync(bool enabled) override;
		void SetMinimized(bool enabled) override;
		void SetFocused(double enabled) override;
		void SetFullscreen(bool enabled) override;
		void SetFramerate(double framerate) override;
		void SetImGuiState(bool enabled) override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }

		inline double GetFramerate() const override;

		inline bool IsVSync() const override;
		inline bool IsMinimized() const override;
		inline bool IsFocused() const override;
		inline bool IsFullscreen() const override;
		inline bool IsImGuiEnabled() const override;

		inline void* GetNativeWindow() const override { return m_Window; }

	private:
		void Init(const WindowProps& props);
		void Shutdown();

	private:
		GLFWwindow* m_Window = nullptr;
		GLFWmonitor* m_Monitor = nullptr;
		const GLFWvidmode* m_Mode = nullptr;

		std::shared_ptr<GraphicsContext> m_Context = nullptr;

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