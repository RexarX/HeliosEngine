#pragma once

#include "Core.h"

#include "Window.h"

#include "LayerStack.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "VoxelEngine/Render/Shader.h"
#include "VoxelEngine/Render/Buffer.h"
#include "VoxelEngine/Render/VertexArray.h"

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

		std::shared_ptr<Shader> m_Shader;
		std::shared_ptr<VertexArray> m_VertexArray;

		std::shared_ptr<Shader> m_BlueShader;
		std::shared_ptr<VertexArray> m_SquareVA;

		bool m_Running = true;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}