#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Timestep.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"

#include "ImGui/ImGuiLayer.h"

#include "Utils/Timer.h"

namespace Helios
{
	class HELIOSENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		static inline Application& Get() { return *m_Instance; }

		inline Timestep GetDeltaTime() const { return m_DeltaTime; }

		inline const Window& GetWindow() const { return *m_Window; }
		inline Window& GetWindow() { return *m_Window; }
	
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		bool OnKeyPressed(KeyPressedEvent& e);

	private:
		static Application* m_Instance;

		std::unique_ptr<Window> m_Window;

		LayerStack m_LayerStack;

		ImGuiLayer* m_ImGuiLayer;

		bool m_Running = true;

		Utils::Timer m_Timer;
		Timestep m_DeltaTime;
		double m_FramerateLimit = 0.0;
	};

	Application* CreateApplication();
}