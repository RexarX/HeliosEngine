#pragma once

#include "Window.h"
#include "LayerStack.h"

namespace Helios {
	class ImGuiLayer;

	class HELIOSENGINE_API Application {
	public:
		Application(std::string_view name);
		virtual ~Application();

		void Run();

		void OnEvent(Event& event);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }
		void PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); }

		void SetFramerateLimit(double limit);

		inline const std::string& GetName() const { return m_Name; }

		inline const Window& GetWindow() const { return *m_Window; }
		inline Window& GetWindow() { return *m_Window; }

		inline Timestep GetDeltaTime() const { return m_DeltaTime; }
		inline uint32_t GetFramerateLimit() const { return m_FramerateLimit; }
		inline uint64_t GetFrameNumber() const { return m_FrameCounter; }

		static inline Application& Get() { return *m_Instance; }
	
	private:
		void Init();

		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

		bool OnKeyPress(KeyPressEvent& event);

	private:
		static inline Application* m_Instance = nullptr;

		std::string m_Name;

		std::unique_ptr<Window> m_Window = nullptr;

		LayerStack m_LayerStack;

#ifndef RELEASE_MODE
		ImGuiLayer* m_ImGuiLayer = nullptr;
#endif

		Timestep m_DeltaTime = 0.0;
		uint32_t m_FramerateLimit = 0.0;
		double m_FramerateLimitSec = 0.0;
		uint64_t m_FrameCounter = 0;

		bool m_Running = false;
		bool m_ImguiEnabled = false;

#ifdef ENABLE_PROFILING
		bool m_Profile = false;
#endif
	};

	inline Application* CreateApplication();
}