#include "WindowsWindow.h"	

#include "Application.h"

#include "Renderer/GraphicsContext.h"

#include "Config/ConfigManager.h"

#include <GLFW/glfw3.h>

namespace Helios {
	WindowsWindow::WindowsWindow() {
		Init();
	}

	WindowsWindow::~WindowsWindow() {
		Shutdown();
	}

	void WindowsWindow::Init() {
		PROFILE_FUNCTION();
		if (!s_GLFWInitialized) {
			glfwSetErrorCallback(GLFWErrorCallback);
			int result = glfwInit();
			CORE_ASSERT_CRITICAL(result == GLFW_TRUE, "Failed to initialize GLFW!");
			s_GLFWInitialized = true;
		}

		UserConfig& config = ConfigManager::Get().GetConfig<UserConfig>();

		m_Api = config.GetRenderAPI();
		if (m_Api == RendererAPI::API::None) { m_Api = RendererAPI::API::Vulkan; }
		if (m_Api == RendererAPI::API::Vulkan) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		}

		UpdateMonitor();

		m_Properties.state = State::UnFocused;

		const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
		Capabilities& lastCapabilities = m_Capabilities.back();

		uint32_t maxCurrentWidth = static_cast<uint32_t>(mode->width);
		uint32_t maxCurrentHeight = static_cast<uint32_t>(mode->height);
		uint32_t maxResX = static_cast<uint32_t>(m_Capabilities.back().resolution.first);
		uint32_t maxResY = static_cast<uint32_t>(m_Capabilities.back().resolution.second);
		uint32_t highestRefreshRate = m_Capabilities.back().refreshRate;

		auto [width, height] = config.GetWindowSize();
		if (width == 0) { width = maxCurrentWidth; }
		else { width = std::min(width, maxCurrentWidth); }

		if (height == 0) { height = maxCurrentHeight; }
		else { height = std::min(height, maxCurrentHeight); }
		
		m_Properties.size = { width, height };

		auto [resX, resY] = config.GetWindowResolution();
		if (resX == 0) { resX = maxResX; }
		else { resX = std::min(resX, maxResX); }

		if (resY == 0) { resY = maxResY; }
		else { resY = std::min(resY, maxResY); }

		m_Properties.resolution = { resX, resY };

		uint32_t refreshRate = config.GetWindowRefreshRate();
		if (refreshRate == 0) { refreshRate = highestRefreshRate; }
		else { refreshRate = std::min(refreshRate, highestRefreshRate); }

		m_Properties.vsync = config.IsVSync();
		m_Properties.refreshRate = refreshRate;

		Mode windowMode = config.GetWindowMode();
		if (windowMode == Mode::Unspecified) { windowMode = Mode::Borderless; }
		if (windowMode == Mode::Borderless) {
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}

		m_Properties.mode = windowMode;

		const char* title = Application::Get().GetName().c_str();

		m_Window = glfwCreateWindow(m_Properties.mode == Mode::Borderless ? static_cast<int>(maxCurrentWidth) : static_cast<int>(width),
																m_Properties.mode == Mode::Borderless ? static_cast<int>(maxCurrentHeight) : static_cast<int>(height),
																title, m_Properties.mode == Mode::Fullscreen ? m_Monitor : nullptr, nullptr);

		CORE_ASSERT_CRITICAL(m_Window != nullptr, "Failed to create window!");

		auto& [posX, posY] = m_Properties.position;
		int xPos(0), yPos(0);
		if (windowMode != Mode::Borderless) {
			glfwGetWindowPos(m_Window, &xPos, &yPos);
		}
		posX = static_cast<uint32_t>(xPos);
		posY = static_cast<uint32_t>(yPos);

		CORE_INFO("Created window {} with ({}, {}) size at ({}, {}).", title, width, height, posX, posY);

		m_Context = GraphicsContext::Create(m_Api, m_Window);
		CORE_ASSERT_CRITICAL(m_Context != nullptr, "Failed to create graphics context!");

		switch (windowMode) {
			case Mode::Windowed: {
				glfwSetWindowMonitor(m_Window, nullptr, xPos, yPos,
														 static_cast<int>(width), static_cast<int>(height),
														 static_cast<int>(refreshRate));
				break;
			}

			case Mode::Borderless: {
				glfwSetWindowMonitor(m_Window, nullptr, 0, 0,
														 static_cast<int>(resX), static_cast<int>(resY),
														 static_cast<int>(refreshRate));
				break;
			}

			case Mode::Fullscreen: {
				glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, static_cast<int>(resX), static_cast<int>(resY),
														 static_cast<int>(refreshRate));
				break;
			}

			case Mode::Unspecified: {
				CORE_ASSERT(false, "Window mode 'Unspecified' is not supported!");
				break;
			}

			default: CORE_ASSERT(false, "Unknown window mode!"); break;
		}

		m_Context->SetViewport(resX, resY);
		m_Context->SetVSync(m_Properties.vsync);
		m_Context->Init();

		switch (m_Properties.state) {
			case State::Focused: {
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				break;
			}

			case State::UnFocused: {
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				break;
			}

			case State::Minimized: {
				CORE_ASSERT(false, "Window state 'Minimized' is not supported at initialization!");
				break;
			}

			case State::Unspecified: {
				CORE_ASSERT(false, "Window state 'Unspecified' is not supported!");
				break;
			}

			default: CORE_ASSERT(false, "Unknown window state!"); break;
		}

		glfwSetWindowUserPointer(m_Window, reinterpret_cast<void*>(this));
		glfwSetMonitorUserPointer(m_Monitor, reinterpret_cast<void*>(this));

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

			if (width == 0 || height == 0) {
				win.m_PreviousState = win.m_Properties.state;
				win.m_Properties.state = State::Minimized;
				win.m_ChangedState = true;
				glfwIconifyWindow(window);
				return;
			}

			if (win.m_Properties.state == State::Minimized) {
				win.SetState(win.m_PreviousState);
			}

			uint32_t w = static_cast<uint32_t>(width);
			uint32_t h = static_cast<uint32_t>(height);
			win.m_Properties.size = { w, h };

			win.m_Context->SetViewport(w, h);

			WindowResizeEvent event(w, h);
			win.m_EventCallback(event);
		});

		glfwSetWindowPosCallback(m_Window, [](GLFWwindow* window, int xPos, int yPos) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
			win.m_Properties.position = { static_cast<uint32_t>(xPos), static_cast<uint32_t>(yPos) };
		});

		glfwSetMonitorCallback(MonitorCallback);

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
			WindowCloseEvent event;
			win.m_EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

			switch (action) {
				case GLFW_PRESS: {
					KeyPressEvent event(static_cast<KeyCode>(key), 0);
					win.m_EventCallback(event);
					return;
				}

				case GLFW_RELEASE: {
					KeyReleaseEvent event(key);
					win.m_EventCallback(event);
					return;
				}

				case GLFW_REPEAT: {
					KeyPressEvent event(static_cast<KeyCode>(key), 1);
					win.m_EventCallback(event);
					return;
				}
			}
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

			switch (action) {
				case GLFW_PRESS: {
					MouseButtonPressEvent event(static_cast<MouseCode>(button));
					win.m_EventCallback(event);
					return;
				}

				case GLFW_RELEASE: {
					MouseButtonReleaseEvent event(static_cast<MouseCode>(button));
					win.m_EventCallback(event);
					return;
				}

				case GLFW_REPEAT: {
					MouseButtonPressEvent event(static_cast<MouseCode>(button));
					win.m_EventCallback(event);
					return;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
			MouseScrollEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			win.m_EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

			uint32_t posX = static_cast<uint32_t>(xPos);
			uint32_t posY = static_cast<uint32_t>(yPos);
			int32_t deltaX = 0;
			int32_t deltaY = 0;
			if (win.m_ChangedState) {
				deltaX = posX;
				deltaY = posY;
			}

			auto& [lastX, lastY] = win.m_LastMousePos;
			deltaX = posX - lastX;
			deltaY = posY - lastY;

			lastX = posX;
			lastY = posY;

			MouseMoveEvent event(posX, posY, deltaX, deltaY);
			win.m_EventCallback(event);
		});
  }

  void WindowsWindow::Shutdown() {
		PROFILE_FUNCTION();
		m_Context->Shutdown();
		glfwDestroyWindow(m_Window);
  }

	void WindowsWindow::PoolEvents() {
		PROFILE_FUNCTION();
		glfwPollEvents();
	}

	void WindowsWindow::OnUpdate() {
		m_Context->Update();
	}

	void WindowsWindow::BeginFrame() {
		m_Context->BeginFrame();
	}

	void WindowsWindow::EndFrame() {
		m_Context->EndFrame();
	}

	void WindowsWindow::InitImGui() {
		m_Context->InitImGui();
	}

	void WindowsWindow::ShutdownImGui() {
		m_Context->ShutdownImGui();
	}

	void WindowsWindow::BeginFrameImGui() {
		m_Context->BeginFrameImGui();
	}

	void WindowsWindow::EndFrameImGui() {
		m_Context->EndFrameImGui();
	}

	void WindowsWindow::SetState(State state) {
		if (m_Properties.state == state) { return; }

		switch (state) {
			case State::Focused: {
				if (m_Properties.state == State::Minimized) {
					glfwRestoreWindow(m_Window);
				}
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				WindowFocusEvent event;
				m_EventCallback(event);
				break;
			}

			case State::UnFocused: {
				if (m_Properties.state == State::Minimized) {
					glfwRestoreWindow(m_Window);
				}
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				WindowLostFocusEvent event;
				m_EventCallback(event);
				break;
			}

			case State::Minimized: {
				glfwIconifyWindow(m_Window);
				m_PreviousState = m_Properties.state;
				break;
			}

			case State::Unspecified: {
				CORE_ASSERT(false, "Window state 'Unspecified' is not supported!");
				return;
			}

			default: CORE_ASSERT(false, "Unknown window state!"); return;
		}

		m_ChangedState = true;
		m_Properties.state = state;
	}

	void WindowsWindow::SetMode(Mode mode) {
		if (m_Properties.mode == mode) { return; }

		switch (mode) {
			case Mode::Windowed: {
				if (m_Properties.mode == Mode::Borderless) {
					glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
					glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
				}

				m_Properties.mode = mode;
				if (m_Monitor != nullptr) {
					auto [x, y] = m_Properties.position;
					auto [width, height] = m_Properties.size;

					if (m_Monitor != nullptr) {
						glfwSetWindowMonitor(m_Window, nullptr, static_cast<int>(x), static_cast<int>(y),
																 static_cast<int>(width), static_cast<int>(height),
																 GLFW_DONT_CARE);
					}
				}
				return;
			}

			case Mode::Borderless: {
				m_Properties.mode = mode;

				if (m_Monitor != nullptr) {
					glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
					glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
					const GLFWvidmode* vidMode = glfwGetVideoMode(m_Monitor);
					glfwSetWindowSize(m_Window, vidMode->width, vidMode->height);
				}
				return;
			}

			case Mode::Fullscreen: {
				m_Properties.mode = mode;
				if (m_Monitor != nullptr) {
					auto [resX, resY] = m_Properties.resolution;
					glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, static_cast<int>(resX), static_cast<int>(resY),
															 static_cast<int>(m_Properties.refreshRate));
				}
				return;
			}

			case Mode::Unspecified: {
				CORE_ASSERT(false, "Window mode 'Unspecified' is not supported!");
				return;
			}

			default: CORE_ASSERT(false, "Unknown window mode!"); return;
		}
	}

	void WindowsWindow::SetSize(uint32_t width, uint32_t height) {
		auto& [w, h] = m_Properties.size;
		if (width == w && height == h) { return; }

		if (m_Monitor == nullptr) {
			w = width;
			h = height;
			return;
		}

		const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
		uint32_t maxWidth = static_cast<uint32_t>(mode->width);
		uint32_t maxHeight = static_cast<uint32_t>(mode->height);

		if (width == 0 || width > maxWidth || height == 0 || height > maxHeight) {
			CORE_ASSERT(false, "Cannot set window size to ({}, {}): width and height must be inside of the monitor's range ({}, {})!",
									width, height, maxWidth, maxHeight);
			return;
		}

		w = width;
		h = height;

		glfwSetWindowSize(m_Window, static_cast<int>(width), static_cast<int>(height));
	}

	void WindowsWindow::SetResolution(uint32_t resX, uint32_t resY) {
		auto& [resolutionX, resolutionY] = m_Properties.resolution;
		if (resolutionX == resX && resolutionY == resY) { return; }

		if (m_Monitor == nullptr) {
			resolutionX = resX;
			resolutionY = resY;
			return;
		}

		const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
		uint32_t maxWidth = static_cast<uint32_t>(mode->width);
		uint32_t maxHeight = static_cast<uint32_t>(mode->height);

		if (resX == 0 || resX > maxWidth || resY == 0 || resY > maxHeight) {
			CORE_ASSERT(false, "Cannot set window resolution to ({}, {}): resX and resY must be inside of the monitor's range ({}, {})!",
									resX, resY, maxWidth, maxHeight);
			return;
		}

		resolutionX = resX;
		resolutionY = resY;

		if (m_Properties.mode == Mode::Fullscreen) {
			m_Context->SetViewport(resX, resY);
		}
	}

	void WindowsWindow::SetPosition(uint32_t x, uint32_t y) {
		auto& [posX, posY] = m_Properties.position;
		if (x == posX && y == posY) { return; }

		if (m_Monitor == nullptr) {
			posX = x;
			posY = y;
			return;
		}

		const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
		uint32_t maxWidth = static_cast<uint32_t>(mode->width);
		uint32_t maxHeight = static_cast<uint32_t>(mode->height);

		if (x > maxWidth ||  y > maxHeight) {
			CORE_ASSERT(false, "Cannot set window position to ({}, {}): x and y values must be inside of the monitor's range ({}, {})!",
									x, y, maxWidth, maxHeight);
			return;
		}

		posX = x;
		posY = y;

		if (m_Properties.mode == Mode::Windowed) {
			glfwSetWindowPos(m_Window, static_cast<int>(x), static_cast<int>(y));
		}
	}

	void WindowsWindow::SetRefreshRate(uint32_t refreshRate) {
		if (refreshRate == m_Properties.refreshRate) { return; }

		if (m_Monitor == nullptr) {
			m_Properties.refreshRate = refreshRate;
			return;
		}

		const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
		uint32_t highestRefreshRate = mode->refreshRate;
		if (refreshRate == 0 || refreshRate > highestRefreshRate) {
			CORE_ASSERT(false, "Cannot set refresh rate to {} hz: exceeds max screen supported value({})!",
									refreshRate, highestRefreshRate);
			return;
		}

		m_Properties.refreshRate = refreshRate;

		if (m_Properties.mode == Mode::Fullscreen) {
			auto [resX, resY] = m_Properties.resolution;
			glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, static_cast<int>(resX), static_cast<int>(resY),
													 static_cast<int>(refreshRate));
		}
	}

	void WindowsWindow::SetVSync(bool enabled) {
		if (m_Properties.vsync != enabled) {
			m_Context->SetVSync(enabled);
			m_Properties.vsync = enabled;
		}
	}

	void WindowsWindow::UpdateMonitor() {
		m_Capabilities.clear();

		m_Monitor = glfwGetPrimaryMonitor();
		CORE_ASSERT_CRITICAL(m_Monitor != nullptr, "Display not found!");

		int count = 0;
		const GLFWvidmode* modes = glfwGetVideoModes(m_Monitor, &count);

		m_Capabilities.reserve(count);
		for (int i = 0; i < count; ++i) {
			Capabilities cap{};
			cap.resolution = {
					static_cast<uint32_t>(modes[i].width),
					static_cast<uint32_t>(modes[i].height)
			};
			cap.refreshRate = static_cast<uint32_t>(modes[i].refreshRate);
			m_Capabilities.emplace_back(std::move(cap));
		}
	}

	void WindowsWindow::MonitorCallback(GLFWmonitor* monitor, int event) {
		WindowsWindow& win = *static_cast<WindowsWindow*>(glfwGetMonitorUserPointer(monitor));
		if (monitor == win.m_Monitor) { return; }

		if (event == GLFW_CONNECTED && monitor == glfwGetPrimaryMonitor()) {
			win.UpdateMonitor();

			const GLFWvidmode* mode = glfwGetVideoMode(win.m_Monitor);
			uint32_t maxWidth = static_cast<uint32_t>(mode->width);
			uint32_t maxHeight = static_cast<uint32_t>(mode->height);
			
			bool changeViewport = false;

			auto& [width, height] = win.m_Properties.size;
			if (width > maxWidth && height > maxHeight) {
				width = maxWidth;
				height = maxHeight;
				changeViewport = true;
			}

			auto& [resX, resY] = win.m_Properties.resolution;
			auto [maxResX, maxResY] = win.m_Capabilities.back().resolution;
			if (resX > maxResX && resY > maxResY) {
				resX = maxResX;
				resY = maxResY;
				changeViewport = true;
			}

			auto& [x, y] = win.m_Properties.position;
			if (x > maxWidth && y > maxHeight) {
				x = maxWidth;
				y = maxHeight;
			}

			uint32_t maxRefreshRate = win.m_Capabilities.back().refreshRate;
			if (win.m_Properties.refreshRate > maxRefreshRate) {
				win.m_Properties.refreshRate = maxRefreshRate;
			}

			switch (win.m_Properties.mode) {
				case Mode::Windowed: {
					glfwSetWindowMonitor(win.m_Window, nullptr, static_cast<int>(x), static_cast<int>(y),
															 static_cast<int>(width), static_cast<int>(height), GLFW_DONT_CARE);
					if (changeViewport) { win.m_Context->SetViewport(width, height); }
					return;
				}

				case Mode::Borderless: {
					glfwSetWindowSize(win.m_Window, mode->width, mode->height);
					if (changeViewport) { win.m_Context->SetViewport(maxWidth, maxHeight); }
					return;
				}

				case Mode::Fullscreen: {
					glfwSetWindowMonitor(win.m_Window, win.m_Monitor, 0, 0, static_cast<int>(resX), static_cast<int>(resY),
															 static_cast<int>(win.m_Properties.refreshRate));
					if (changeViewport) { win.m_Context->SetViewport(resX, resY); }
					return;
				}

				case Mode::Unspecified: {
					CORE_ASSERT(false, "Window mode 'Unspecified' is not supported!");
					return;
				}

				default: CORE_ASSERT(false, "Unknown window mode!"); return;
			}
		} else if (event == GLFW_DISCONNECTED && monitor == glfwGetPrimaryMonitor()) {
			GLFWmonitor* primiryMonitor = glfwGetPrimaryMonitor();
			if (primiryMonitor == nullptr) { return; }
			win.UpdateMonitor();
		}
	}
}