#pragma once

#include "Renderer/RendererAPI.h"

#include "pch.h"

namespace Helios {
	class HELIOSENGINE_API Window {
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		enum class Mode {
			Unspecified = 0,
			Windowed,
			Borderless,
			Fullscreen
		};

		enum class State {
			Unspecified = 0,
			Focused, UnFocused,
			Minimized
		};

		struct Properties {
			Window::Mode mode = Window::Mode::Unspecified;
			State state = State::Unspecified;

			std::pair<uint32_t, uint32_t> size{ 0, 0 };
			std::pair<uint32_t, uint32_t> resolution{ 0, 0 };
			std::pair<uint32_t, uint32_t> position{ 0, 0 };
			uint32_t refreshRate = 0;
			bool vsync = false;
		};

		struct Capabilities {
			std::pair<uint32_t, uint32_t> resolution{ 0, 0 };
			uint32_t refreshRate = 0;
		};

		virtual ~Window() = default;

		virtual void PoolEvents() = 0;
		virtual void OnUpdate() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void InitImGui() = 0;
		virtual void ShutdownImGui() = 0;
		virtual void BeginFrameImGui() = 0;
		virtual void EndFrameImGui() = 0;
		
		virtual void SetState(State state) = 0;
		virtual void SetMode(Mode mode) = 0;

		virtual void SetSize(uint32_t width, uint32_t height) = 0;
		virtual void SetResolution(uint32_t resX, uint32_t resY) = 0;
		virtual void SetPosition(uint32_t x, uint32_t y) = 0;

		virtual void SetRefreshRate(uint32_t refreshRate) = 0;
		virtual void SetVSync(bool enabled) = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

		virtual inline const std::vector<Capabilities>& GetCapabilities() const = 0;

		virtual inline State GetState() const = 0;
		virtual inline Mode GetMode() const = 0;

		virtual inline std::pair<uint32_t, uint32_t> GetSize() const = 0;
		virtual inline uint32_t GetWidth() const = 0;
		virtual inline uint32_t GetHeight() const = 0;

		virtual inline std::pair<uint32_t, uint32_t> GetPosition() const = 0;
		virtual inline uint32_t GetPosX() const = 0;
		virtual inline uint32_t GetPosY() const = 0;

		virtual inline std::pair<uint32_t, uint32_t> GetResolution() const = 0;
		virtual inline uint32_t GetResolutionX() const = 0;
		virtual inline uint32_t GetResolutionY() const = 0;

		virtual inline uint32_t GetRefreshRate() const = 0;
		virtual inline bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static std::unique_ptr<Window> Create();
	};
}