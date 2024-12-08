#pragma once

#include "Event.h"

namespace Helios  {
	class HELIOSENGINE_API WindowResizeEvent final : public Event {
	public:
		explicit WindowResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height) {}

		explicit WindowResizeEvent(const WindowResizeEvent&) = default;
		WindowResizeEvent(WindowResizeEvent&&) = default;
		virtual ~WindowResizeEvent() = default;

		inline std::pair<uint32_t, uint32_t> GetSize() const { return { m_Width, m_Height }; }
		inline uint32_t GetWidth() const { return m_Width; }
		inline uint32_t GetHeight() const { return m_Height; }

		inline std::string ToString() const override {
			return std::format("WindowResizeEvent: {}, {}", m_Width, m_Height);
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
	};

	class HELIOSENGINE_API WindowCloseEvent final : public Event {
	public:
		WindowCloseEvent() = default;
		explicit WindowCloseEvent(const WindowCloseEvent&) = default;
		WindowCloseEvent(WindowCloseEvent&&) = default;
		virtual ~WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API WindowFocusEvent final : public Event {
	public:
		WindowFocusEvent() = default;
		explicit WindowFocusEvent(const WindowFocusEvent&) = default;
		WindowFocusEvent(WindowFocusEvent&&) = default;
    virtual ~WindowFocusEvent() = default;

		EVENT_CLASS_TYPE(WindowFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API WindowLostFocusEvent final : public Event {
	public:
		WindowLostFocusEvent() = default;
		explicit WindowLostFocusEvent(const WindowLostFocusEvent&) = default;
		WindowLostFocusEvent(WindowLostFocusEvent&&) = default;
    virtual ~WindowLostFocusEvent() = default;

		EVENT_CLASS_TYPE(WindowLostFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API AppTickEvent final : public Event {
	public:
		AppTickEvent() = default;
		explicit AppTickEvent(const AppTickEvent&) = default;
		AppTickEvent(AppTickEvent&&) = default;
    virtual ~AppTickEvent() = default;

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API AppUpdateEvent final : public Event {
	public:
		explicit AppUpdateEvent(double deltaTime) : m_DeltaTime(deltaTime) {}
		explicit AppUpdateEvent(const AppUpdateEvent&) = default;
		AppUpdateEvent(AppUpdateEvent&&) = default;
    virtual ~AppUpdateEvent() = default;

    inline double GetDeltaTime() const { return m_DeltaTime; }

		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		double m_DeltaTime = 0.0;
	};

	class HELIOSENGINE_API AppRenderEvent final : public Event {
	public:
		AppRenderEvent() = default;
		explicit AppRenderEvent(const AppRenderEvent&) = default;
		AppRenderEvent(AppRenderEvent&&) = default;
    virtual ~AppRenderEvent() = default;

		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};
}