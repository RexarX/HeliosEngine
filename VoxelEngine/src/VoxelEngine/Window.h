#pragma once

#include "vepch.h"

#include "Core.h"

#include "Events/Event.h"

namespace VoxelEngine
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

	class VOXELENGINE_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void SwapBuffers() = 0;
		virtual void ClearBuffer() = 0;
		virtual void PoolEvents() = 0;
		virtual void OnUpdate() = 0;

		virtual void InitImGui() = 0;
		virtual void ShutdownImGui() = 0;
		virtual void Begin() = 0;
		virtual void End() = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(const bool enabled) = 0;
		virtual void SetMinimized(const bool enabled) = 0;
		virtual void SetFocused(const double enabled) = 0;
		virtual void SetFullscreen(const bool enabled) = 0;
		virtual void SetFramerate(const double framerate) = 0;
		virtual void SetLastFramerate(const double framerate) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual double GetFramerate() const = 0;
		virtual double GetLastFramerate() const = 0;

		virtual bool IsVSync() const = 0;
		virtual bool IsMinimized() const = 0;
		virtual bool IsFocused() const = 0;
    virtual bool IsFullscreen() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static std::unique_ptr<Window> Create(const WindowProps& props = WindowProps());
	};
}