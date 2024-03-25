#include "vepch.h"

#include "Application.h"

#include "Render/Renderer.h"

#include <glfw/glfw3.h>

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

namespace VoxelEngine
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		VE_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		Renderer::Init();
	}
	
	Application::~Application()
	{
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); ) {
			(*--it)->OnEvent(e);
			if (e.Handled) { break; }
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	void Application::Run()
	{
		Timestep timestep, frametime;
		double time;
		double m_LastFrameUpdate(0.0), m_LastFrameTime(0.0);

		m_FramerateLimit = 1 / m_Window->GetFramerate();

		while (m_Running) {
			time = glfwGetTime();
			timestep = time - m_LastFrameUpdate;
			frametime = time - m_LastFrameTime;
			
			m_Window->PollEvents();

			for (Layer* layer : m_LayerStack) {
				layer->OnUpdate(timestep);
			}

			if (!m_Window->IsMinimized() && (frametime >= m_FramerateLimit || m_Window->GetFramerate() == 0.0)) {
				m_Window->ClearBuffer();
				for (Layer* layer : m_LayerStack) {
					layer->Draw();
				}
				m_Window->OnUpdate();
				
				m_LastFrameTime = time;

				VE_TRACE("Framerate: {0}fps", frametime.GetFramerate());
			}

			m_LastFrameUpdate = time;
		}
	}
}
