#pragma once

#include "VoxelEngine/Core.h"

#include "VoxelEngine/Window.h"

#include "VoxelEngine/LayerStack.h"

#include "VoxelEngine/Events/Event.h"
#include "VoxelEngine/Events/ApplicationEvent.h"

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
		bool m_Running = true;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}