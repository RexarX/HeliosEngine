#include "Application.h"

#include "ImGui/ImGuiLayer.h"

#include "Renderer/GraphicsContext.h"

namespace Helios {
	Application::Application() {
		PROFILE_BEGIN_SESSION("Initialization");
		PROFILE_FUNCTION();

		CORE_ASSERT(m_Instance == nullptr, "Application already exists!");
		m_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallback(BIND_FN(Application::OnEvent));
		
#ifndef RELEASE_MODE
		{
			PROFILE_SCOPE("ImGui initialization");

			m_ImGuiLayer = new ImGuiLayer();
			PushOverlay(m_ImGuiLayer);
		}
#endif

		m_Running = true;
	}

	Application::~Application() {
		PROFILE_END_SESSION();
		PROFILE_BEGIN_SESSION("Shutdown");
	}
	
	void Application::OnEvent(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_FN(Application::OnWindowResize));
		dispatcher.Dispatch<KeyPressEvent>(BIND_FN(Application::OnKeyPress));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
			(*it)->OnEvent(event);
			if (event.Handled) { break; }
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& event) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event) {
		if (event.GetWidth() == 0 || event.GetHeight() == 0) { m_Window->SetMinimized(true); }
		else if (m_Window->IsMinimized()) { m_Window->SetMinimized(false); }
		return true;
	}

  bool Application::OnKeyPress(KeyPressEvent& event) {
#ifndef RELEASE_MODE
    if (event.GetKeyCode() == Key::Home) {
			bool newState = !m_Window->IsImGuiEnabled();
			m_ImGuiLayer->BlockEvents(newState);
      m_Window->SetImGuiState(newState);
    }
#endif

#ifdef ENABLE_PROFILING
		if (event.GetKeyCode() == Key::F9) {
			m_Profile = true;
		}
#endif
		return true;
	}

	void Application::Run() {
		PROFILE_END_SESSION();
		PROFILE_BEGIN_SESSION("Frame");
#ifdef ENABLE_PROFILING
		bool profiling = false;
#endif
		double lastFrameUpdateTime = 0.0;
		m_FramerateLimit = m_Window->GetFramerate() == 0.0 ? 0.0 : 1.0 / m_Window->GetFramerate();
		
		Utils::Timer timer;
		timer.Start();
		while (m_Running) {
#ifdef ENABLE_PROFILING
			if (m_Profile) {
				ACTIVATE_PROFILER();
				profiling = true;
			}
#endif
			PROFILE_SCOPE_ONCE("Frame");

			timer.Stop();
			m_DeltaTime = timer.GetElapsedSec() - lastFrameUpdateTime;

			m_Window->PoolEvents();

			if (!m_Window->IsMinimized() && (m_FramerateLimit == 0.0 || (double)m_DeltaTime >= m_FramerateLimit)) {
				//CORE_TRACE("Delta time: {}ms", m_DeltaTime.GetMilliseconds());

				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(m_DeltaTime);
				}

				m_Window->BeginFrame();
				for (Layer* layer : m_LayerStack) {
					layer->Draw();
				}
				m_Window->EndFrame();

#ifndef RELEASE_MODE
				if (m_Window->IsImGuiEnabled()) {
					m_Window->BeginFrameImGui();
					for (Layer* layer : m_LayerStack) {
						layer->OnImGuiRender(ImGui::GetCurrentContext());
					}
					m_Window->EndFrameImGui();
				}
#endif
				m_Window->OnUpdate();
				lastFrameUpdateTime = timer.GetElapsedSec();
				++m_FrameCounter;
			}
#ifdef ENABLE_PROFILING
			if (profiling) {
				m_Profile = false;
				profiling = false;
			}
#endif
		}
	}
}