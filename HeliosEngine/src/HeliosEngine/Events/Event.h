#pragma once

#include "pch.h"

namespace Helios
{
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

	#define EVENT_CLASS_TYPE(type) static inline EventType GetStaticType() { return EventType::type; }\
																 virtual inline EventType GetEventType() const override { return GetStaticType(); }\
																 virtual inline const char* GetName() const override { return #type; }

	#define EVENT_CLASS_CATEGORY(category) virtual inline uint32_t GetCategoryFlags() const override { return category; }

	class HELIOSENGINE_API Event
	{
	public:
		virtual ~Event() = default;

		virtual inline EventType GetEventType() const = 0;
		virtual inline const char* GetName() const = 0;
		virtual inline uint32_t GetCategoryFlags() const = 0;
		virtual inline std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category) const { return GetCategoryFlags() & category; }

    public:
			bool Handled = false;
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(const Event& event) : m_Event(const_cast<Event&>(event)) {}

		template<typename T>
		bool Dispatch(const std::function<bool(const T&)>& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType()) {
				m_Event.Handled = func(static_cast<const T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.ToString(); }
}