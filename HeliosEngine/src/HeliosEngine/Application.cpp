#include "Application.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"

#include "ImGui/ImGuiLayer.h"

#include "Renderer/GraphicsContext.h"

namespace Helios
{
	Application* Application::m_Instance = nullptr;

	Application::Application()
	{
		CORE_ASSERT(m_Instance == nullptr, "Application already exists!");
		m_Instance = this;

		m_Window = std::move(Window::Create());
		m_Window->SetEventCallback(BIND_FN(Application::OnEvent));

#if defined(DEBUG_MODE) || defined(RELEASE_MODE)
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
#endif

		m_Running = true;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_FN(Application::OnWindowResize));
		dispatcher.Dispatch<KeyPressedEvent>(BIND_FN(Application::OnKeyPressed));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) {
			(*--it)->OnEvent(e);
			if (e.Handled) { break; }
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0) { m_Window->SetMinimized(true); }
		else if (m_Window->IsMinimized()) { m_Window->SetMinimized(false); }
		return true;
	}

 bool Application::OnKeyPressed(KeyPressedEvent& e)
	{
#ifndef DIST_MODE
    if (e.GetKeyCode() == Key::Insert) {
			bool currentState = m_Window->IsImGuiEnabled();
			m_ImGuiLayer->BlockEvents(!currentState);
      m_Window->SetImGuiState(!currentState);
    }
#endif

		return true;
	}

	void Application::Run()
	{
		double lastFrameUpdateTime = 0.0;
		m_FramerateLimit = m_Window->GetFramerate() == 0.0 ? 0.0 : 1.0 / m_Window->GetFramerate();
		m_Timer.Start();
		while (m_Running) {
			m_Timer.Stop();
			m_DeltaTime = m_Timer.GetElapsedSec() - lastFrameUpdateTime;

			m_Window->PoolEvents();

			if (!m_Window->IsMinimized() && (m_FramerateLimit == 0.0 || (double)m_DeltaTime >= m_FramerateLimit))
			{
				//CORE_TRACE("Delta time: {0}ms", m_DeltaTime.GetMilliseconds());
				
				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(m_DeltaTime);
				}

				m_Window->BeginFrame();
				for (Layer* layer : m_LayerStack) {
					layer->Draw();
				}
				m_Window->EndFrame();

#ifndef DIST_MODE
				if (m_Window->IsImGuiEnabled()) {
					m_Window->BeginFrameImGui();
					for (Layer* layer : m_LayerStack) {
						layer->OnImGuiRender(ImGui::GetCurrentContext());
					}
					m_Window->EndFrameImGui();
				}
#endif

				m_Window->OnUpdate();
				lastFrameUpdateTime = m_Timer.GetElapsedSec();
			}
		}
	}
}