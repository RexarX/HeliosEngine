#include "Application.h"

#include "ImGui/ImGuiLayer.h"

#include "Config/ConfigManager.h"

namespace Helios {
	Application::Application(std::string_view name)
	: m_Name(name) {
		CORE_ASSERT(m_Instance == nullptr, "Application already exists!");
		m_Instance = this;

		Init();
	}

	Application::~Application() {
		ConfigManager& configManager = ConfigManager::Get();
		UserConfig& userConfig = configManager.GetConfig<UserConfig>();
		userConfig.LoadFromWindow(*m_Window);
		configManager.SaveConfiguration<UserConfig>(PathManager::GetUserConfigDirectory() / "config.toml");
	}

	void Application::Run() {
#ifdef ENABLE_PROFILING
		bool profiling = false;
#endif

		double lastFrameUpdateTime = 0.0;
		m_FramerateLimitSec = m_FramerateLimit == 0.0 ? 0.0 : 1.0 / m_FramerateLimit;
		
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

			if (m_Window->GetState() != Window::State::Minimized && (m_FramerateLimit == 0.0
					|| m_DeltaTime.GetSeconds() >= m_FramerateLimitSec)) {
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
				if (m_ImguiEnabled) {
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

	void Application::OnEvent(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_FN(Application::OnWindowResize));
		dispatcher.Dispatch<KeyPressEvent>(BIND_FN(Application::OnKeyPress));

		for (Layer* layer : m_LayerStack) {
			layer->OnEvent(event);
			if (event.IsHandled()) { break; }
		}
	}

	void Application::SetFramerateLimit(double limit) {
		m_FramerateLimit = limit;
		m_FramerateLimitSec = limit > 0.0 ? 1.0 / limit : 0.0;
	}

	void Application::Init() {
		PROFILE_FUNCTION();

		if (m_Running) {
			CORE_ASSERT(false, "Application in already inilialized!");
			return;
		}

		ConfigManager& configManager = ConfigManager::Get();
		UserConfig& userConfig = configManager.GetConfig<UserConfig>();
		configManager.LoadConfiguration<UserConfig>(PathManager::GetUserConfigDirectory() / "config.toml");

		SetFramerateLimit(userConfig.GetFramerateLimit());

		m_Window = Window::Create();
		m_Window->SetEventCallback(BIND_FN(Application::OnEvent));

#ifndef RELEASE_MODE
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
#endif

		m_Running = true;
	}

	bool Application::OnWindowClose(WindowCloseEvent& event) {
		m_Running = false;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event) {
		if (event.GetWidth() == 0 || event.GetHeight() == 0) {
			m_Window->SetState(Window::State::Minimized);
		} else if (m_Window->GetState() == Window::State::Minimized) {
			m_Window->SetState(Window::State::Focused);
		}

		return false;
	}

	bool Application::OnKeyPress(KeyPressEvent& event) {
#ifndef RELEASE_MODE
		if (event.GetKeyCode() == Key::Home) {
			m_ImguiEnabled = !m_ImguiEnabled;
			m_ImGuiLayer->BlockEvents(m_ImguiEnabled);
		}
#endif

#ifdef ENABLE_PROFILING
		if (event.GetKeyCode() == Key::F9) {
			m_Profile = true;
		}
#endif
		return false;
	}
}