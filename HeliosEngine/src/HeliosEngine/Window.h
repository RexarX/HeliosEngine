#pragma once

#include "Core.h"

#include "Events/Event.h"

#include "pch.h"

namespace Helios
{
	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "VoxelCraft",
			const uint32_t width = 1280,
			const uint32_t height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void PoolEvents() = 0;
		virtual void OnUpdate() = 0;

		virtual void InitImGui() = 0;
		virtual void ShutdownImGui() = 0;
		virtual void BeginFrameImGui() = 0;
		virtual void EndFrameImGui() = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(const bool enabled) = 0;
		virtual void SetMinimized(const bool enabled) = 0;
		virtual void SetFocused(const double enabled) = 0;
		virtual void SetFullscreen(const bool enabled) = 0;
		virtual void SetFramerate(const double framerate) = 0;
		virtual void SetImGuiState(const bool enabled) = 0;

		virtual inline const uint32_t GetWidth() const = 0;
		virtual inline const uint32_t GetHeight() const = 0;

		virtual inline const double GetFramerate() const = 0;

		virtual inline const bool IsVSync() const = 0;
		virtual inline const bool IsMinimized() const = 0;
		virtual inline const bool IsFocused() const = 0;
    virtual inline const bool IsFullscreen() const = 0;
		virtual inline const bool IsImGuiEnabled() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static std::unique_ptr<Window> Create(const WindowProps& props = WindowProps());
	};
}