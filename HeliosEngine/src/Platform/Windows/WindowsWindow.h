#pragma once

#include "Window.h"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace Helios {
	class GraphicsContext;

	class WindowsWindow final : public Window {
	public:
		WindowsWindow(std::string_view title, uint32_t width, uint32_t height);
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
		void SetMinimized(bool enabled) override { m_Data.minimized = enabled; }
		void SetFocused(double enabled) override;
		void SetFullscreen(bool enabled) override;
		void SetFramerate(double framerate) override { m_Data.framerate = framerate; }
		void SetImGuiState(bool enabled) override;

		inline uint32_t GetWidth() const override { return m_Data.width; }
		inline uint32_t GetHeight() const override { return m_Data.height; }

		inline double GetFramerate() const override { return m_Data.framerate; }

		inline bool IsVSync() const override { return m_Data.vsync; }
		inline bool IsMinimized() const override { return m_Data.minimized; }
		inline bool IsFocused() const override { return m_Data.focused; }
		inline bool IsFullscreen() const override { return m_Data.fullscreen; }
		inline bool IsImGuiEnabled() const override { return m_Data.showImGui; }

		inline void* GetNativeWindow() const override { return m_Window; }

	private:
		void Init(std::string_view title, uint32_t width, uint32_t heigh);
		void Shutdown();

	private:
		GLFWwindow* m_Window = nullptr;
		GLFWmonitor* m_Monitor = nullptr;
		const GLFWvidmode* m_Mode = nullptr;

		std::shared_ptr<GraphicsContext> m_Context = nullptr;

		struct WindowData {
			std::string title;
			uint32_t width, height;
			uint32_t posX, posY;
			double framerate = 0.0;
			bool vsync = true;
			bool minimized = false;
			bool focused = false;
			bool fullscreen = false;
			bool showImGui = false;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}