#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Timestep.h"

#include "LayerStack.h"

#include "Utils/Timer.h"

namespace Helios
{
	class Event;
	class WindowCloseEvent;
	class WindowResizeEvent;
  class KeyPressedEvent;
	class ImGuiLayer;

	class HELIOSENGINE_API Application
	{
	public:
		Application();
		virtual ~Application() = default;

		void Run();

		void OnEvent(const Event& e);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }

		void PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); }

		static inline Application& Get() { return *m_Instance; }

		inline Timestep GetDeltaTime() const { return m_DeltaTime; }

		inline const Window& GetWindow() const { return *m_Window; }
		inline Window& GetWindow() { return *m_Window; }
	
	private:
		bool OnWindowClose(const WindowCloseEvent& e);
		bool OnWindowResize(const WindowResizeEvent& e);

		bool OnKeyPressed(const KeyPressedEvent& e);

	private:
		static Application* m_Instance;

		std::unique_ptr<Window> m_Window;

		LayerStack m_LayerStack;

		ImGuiLayer* m_ImGuiLayer = nullptr;

		bool m_Running = false;

		Utils::Timer m_Timer;
		Timestep m_DeltaTime;
		double m_FramerateLimit = 0.0;
	};

	Application* CreateApplication();
}