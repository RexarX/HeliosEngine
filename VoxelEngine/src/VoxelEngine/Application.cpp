#include "vepch.h"

#include "Application.h"

#include "Render/Renderer.h"

#include "Utils/sleep_util.h"

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

	void Application::SleepFor(const double seconds)
	{
		sleep(seconds);
	}

	void Application::Run()
	{
		Timestep timestep;
		double frameTime;

		m_FramerateLimit = 1 / m_Window->GetFramerate();

		while (m_Running) {
			m_Timer.Start();
			
			m_Window->ClearBuffer();
			m_Window->PollEvents();

			for (Layer* layer : m_LayerStack) {
				layer->OnUpdate(timestep);
			}

			m_Window->OnUpdate();

			frameTime = m_Timer.Stop();
			if (m_FramerateLimit != 0 && (frameTime < m_FramerateLimit)) {
				SleepFor(m_FramerateLimit - frameTime);
			}

			timestep = m_Timer.Stop();
			VE_TRACE("Framerate: {0}fps", timestep.GetFramerate());
		}
	}
}