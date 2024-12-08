#pragma once

#include "Window.h"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace Helios {
	class GraphicsContext;

	class LinuxWindow final : public Window {
	public:
		LinuxWindow();
		virtual ~LinuxWindow();

		void PoolEvents() override;
		void OnUpdate() override;
		void BeginFrame() override;
		void EndFrame() override;

		void InitImGui() override;
		void ShutdownImGui() override;
		void BeginFrameImGui() override;
		void EndFrameImGui() override;
		
		void SetState(State state) override;
		void SetMode(Mode mode) override;

	  void SetSize(uint32_t width, uint32_t height) override;
		void SetResolution(uint32_t resX, uint32_t resY) override;
	  void SetPosition(uint32_t x, uint32_t y) override;

		void SetRefreshRate(uint32_t refreshRate) override;
		void SetVSync(bool enabled) override;

		void SetEventCallback(const EventCallbackFn& callback) override { m_EventCallback = callback; }

		inline const std::vector<Capabilities>& GetCapabilities() const override { return m_Capabilities; }

		inline State GetState() const override { return m_Properties.state; }
		inline Mode GetMode() const override { return m_Properties.mode; }

		inline std::pair<uint32_t, uint32_t> GetSize() const override { return m_Properties.size; }
		inline uint32_t GetWidth() const override { return m_Properties.size.first; }
		inline uint32_t GetHeight() const override { return m_Properties.size.second; }

		inline std::pair<uint32_t, uint32_t> GetResolution() const override { return m_Properties.resolution; }
		inline uint32_t GetResolutionX() const override { return m_Properties.resolution.first; }
		inline uint32_t GetResolutionY() const override { return m_Properties.resolution.second; }

		inline std::pair<uint32_t, uint32_t> GetPosition() const override { return m_Properties.position; }
		inline uint32_t GetPosX() const override { return m_Properties.position.first; }
		inline uint32_t GetPosY() const override { return m_Properties.position.second; }

		inline uint32_t GetRefreshRate() const override { return m_Properties.refreshRate; }
		inline bool IsVSync() const override { return m_Properties.vsync; }

		inline void* GetNativeWindow() const override { return m_Window; }

	private:
		static void GLFWErrorCallback(int error, const char* description) {
			CORE_ERROR("GLFW Error ({}): {}!", error, description);
		}

		void Init();
		void Shutdown();

		void UpdateMonitor();

		static void MonitorCallback(GLFWmonitor* monitor, int event);

	private:
		static inline bool s_GLFWInitialized = false;

		GLFWwindow* m_Window = nullptr;
		GLFWmonitor* m_Monitor = nullptr;

		std::vector<Capabilities> m_Capabilities;

		std::shared_ptr<GraphicsContext> m_Context = nullptr;

		RendererAPI::API m_Api = RendererAPI::API::None;
		Properties m_Properties;
		State m_PreviousState = State::Unspecified;
		
		bool m_ChangedState = false;
		std::pair<uint32_t, uint32_t> m_LastMousePos{ 0, 0 };

		EventCallbackFn m_EventCallback;
	};
}