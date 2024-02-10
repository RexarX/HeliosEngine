#pragma once

#include "vepch.h"

#include "VoxelEngine/Core.h"

#include "VoxelEngine/Events/Event.h"

namespace VoxelEngine
{
	struct WindowProps
	{
		std::string Title;
		uint16_t Width;
		uint16_t Height;

		WindowProps(const std::string& title = "Voxel Engine",
			uint16_t width = 1280,
			uint16_t height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class VOXELENGINE_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint16_t GetWidth() const = 0;
		virtual uint16_t GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Window* Create(const WindowProps& props = WindowProps());
	};
}