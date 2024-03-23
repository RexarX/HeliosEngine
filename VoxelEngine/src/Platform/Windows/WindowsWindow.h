#pragma once

#include "VoxelEngine/Window.h"

#include <GLFW/glfw3.h>

namespace VoxelEngine
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void ClearBuffer() override;
		void OnUpdate() override;
		void PollEvents() override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetVSync(const bool enabled) override;
		void SetFramerate(const double framerate) override;

		double GetFramerate() const override;

		bool IsVSync() const override;

		inline virtual void* GetNativeWindow() const { return m_Window; }

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			double Framerate;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}