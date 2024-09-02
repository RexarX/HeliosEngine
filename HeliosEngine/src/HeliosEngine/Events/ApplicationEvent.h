#pragma once

#include "Event.h"

namespace Helios 
{
	class HELIOSENGINE_API WindowResizeEvent final : public Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height)
		{
		}

		virtual ~WindowResizeEvent() = default;

		inline uint32_t GetWidth() const { return m_Width; }
		inline uint32_t GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		uint32_t m_Width, m_Height;
	};

	class HELIOSENGINE_API WindowCloseEvent final : public Event
	{
	public:
		WindowCloseEvent() = default;
		virtual ~WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API WindowFocusedEvent final : public Event
	{
	public:
		WindowFocusedEvent() = default;
    virtual ~WindowFocusedEvent() = default;

		EVENT_CLASS_TYPE(WindowFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API WindowLostFocusEvent final : public Event
	{
	public:
		WindowLostFocusEvent() = default;
    virtual ~WindowLostFocusEvent() = default;

		EVENT_CLASS_TYPE(WindowLostFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API AppTickEvent final : public Event
	{
	public:
		AppTickEvent() = default;
    virtual ~AppTickEvent() = default;

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HELIOSENGINE_API AppUpdateEvent final : public Event
	{
	public:
		AppUpdateEvent() = default;
    AppUpdateEvent(double deltaTime) : m_DeltaTime(deltaTime) {}
    virtual ~AppUpdateEvent() = default;

    inline double GetDeltaTime() const { return m_DeltaTime; }

		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		double m_DeltaTime;
	};

	class HELIOSENGINE_API AppRenderEvent final : public Event
	{
	public:
		AppRenderEvent() = default;
    virtual ~AppRenderEvent() = default;

		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};
}