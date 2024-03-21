#pragma once

#include "Core.h"

#include "Window.h"

#include "LayerStack.h"

#include "Timestep.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace VoxelEngine
{
	class VOXELENGINE_API Application {
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	
	private:
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		std::unique_ptr<Window> m_Window;
		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;

		float m_LastFrameUpdate = 0.0f;

		float m_FramerateLimit = 0.0f;

		bool m_Running = true;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}