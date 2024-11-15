#pragma once

#include "Event.h"

#include "HeliosEngine/MouseButtonCodes.h"

namespace Helios  {
	class HELIOSENGINE_API MouseMoveEvent final : public Event {
	public:
		MouseMoveEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}
		virtual ~MouseMoveEvent() = default;

		inline float GetX() const { return m_MouseX; }
		inline float GetY() const { return m_MouseY; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseMoveEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMove)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_MouseX, m_MouseY;
	};

	class HELIOSENGINE_API MouseScrollEvent final : public Event {
	public:
		MouseScrollEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) 
		{
		}

    virtual ~MouseScrollEvent() = default;

		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScroll)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset, m_YOffset;
	};

	class HELIOSENGINE_API MouseButtonEvent : public Event {
	public:
		virtual ~MouseButtonEvent() = default;

		inline MouseCode GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	protected:
		MouseButtonEvent(MouseCode button) : m_Button(button) {}

		MouseCode m_Button;
	};

	class HELIOSENGINE_API MouseButtonPressEvent final : public MouseButtonEvent {
	public:
		MouseButtonPressEvent(MouseCode button) : MouseButtonEvent(button) {}
		virtual ~MouseButtonPressEvent() = default;

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseButtonPressEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPress)
	};

	class HELIOSENGINE_API MouseButtonReleaseEvent final : public MouseButtonEvent {
	public:
		MouseButtonReleaseEvent(MouseCode button) : MouseButtonEvent(button) {}
    virtual ~MouseButtonReleaseEvent() = default;

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonRelease)
	};
}