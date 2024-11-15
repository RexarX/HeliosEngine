#pragma once

#include "Core.h"

#include <functional>
#include <string>
#include <sstream>

namespace Helios {
	enum class EventType : uint32_t {
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMov,
		AppTick, AppUpdate, AppRender,
		KeyPress, KeyRelease,
		MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll,
		CustomEvent
	};

	enum EventCategory {
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

	namespace detail {
		static inline EventType getNextCustomEventType() {
			static uint32_t nextId = static_cast<uint32_t>(EventType::CustomEvent);
			return static_cast<EventType>(++nextId);
		}

		template <typename T>
		static inline EventType registerEventType() {
			static EventType type = getNextCustomEventType();
			return type;
		}
	}

	#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { \
																   if constexpr (std::is_enum_v<decltype(EventType::type)>) { \
																	   return EventType::type; \
																	 } else { \
																	   return detail::registerEventType<type##Event>(); \
																	 } \
																 } \
																 virtual inline EventType GetEventType() const override { return GetStaticType(); } \
																 virtual inline const char* GetName() const override { return #type; }

	#define EVENT_CLASS_CATEGORY(category) virtual inline uint32_t GetCategoryFlags() const override { return category; }

	class HELIOSENGINE_API Event {
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

	class EventDispatcher {
	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template <typename T> requires std::is_base_of_v<Event, T> 
		bool Dispatch(const std::function<bool(T&)>& func) const {
			if (m_Event.GetEventType() == T::GetStaticType()) {
				m_Event.Handled = func(static_cast<T&>(m_Event));
				return true;
			}

			return false;
		}

	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, Event& e) { return os << e.ToString(); }
}