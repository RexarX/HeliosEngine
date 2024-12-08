#pragma once

#include "Event.h"

#include "HeliosEngine/MouseButtonCodes.h"

namespace Helios  {
	class HELIOSENGINE_API MouseMoveEvent final : public Event {
	public:
		explicit MouseMoveEvent(uint32_t x, uint32_t y, int32_t deltaX, int32_t deltaY)
			: m_MouseX(x), m_MouseY(y), m_DeltaX(deltaX), m_DeltaY(deltaY) {}
		
		explicit MouseMoveEvent(const MouseMoveEvent&) = default;
		MouseMoveEvent(MouseMoveEvent&&) = default;
		virtual ~MouseMoveEvent() = default;

		inline std::pair<uint32_t, uint32_t> GetPos() const { return { m_MouseX, m_MouseY }; }
		inline uint32_t GetX() const { return m_MouseX; }
		inline uint32_t GetY() const { return m_MouseY; }

		inline std::pair<int32_t, int32_t> GetDelta() const { return { m_DeltaX, m_DeltaY }; }
		inline int32_t GetDeltaX() const { return m_DeltaX; }
		inline int32_t GetDeltaY() const { return m_DeltaY; }

		inline std::string ToString() const override {
			return std::format("MouseMoveEvent: {}, {} (delta: {}, {})", m_MouseX, m_MouseY,
												 m_DeltaX, m_DeltaY);
		}

		EVENT_CLASS_TYPE(MouseMove)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		uint32_t m_MouseX = 0;
		uint32_t m_MouseY = 0;

		int32_t m_DeltaX = 0;
		int32_t m_DeltaY = 0;
	};

	class HELIOSENGINE_API MouseScrollEvent final : public Event {
	public:
		explicit MouseScrollEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {}

		explicit MouseScrollEvent(const MouseScrollEvent&) = default;
		MouseScrollEvent(MouseScrollEvent&&) = default;
    virtual ~MouseScrollEvent() = default;

		inline std::pair<float, float> GetDelta() const { return { m_XOffset, m_YOffset }; }
		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

		inline std::string ToString() const override {
			return std::format("MouseScrolledEvent: {}, {}", m_XOffset, m_YOffset);
		}

		EVENT_CLASS_TYPE(MouseScroll)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset = 0.0f;
		float m_YOffset = 0.0f;
	};

	class HELIOSENGINE_API MouseButtonEvent : public Event {
	public:
		virtual ~MouseButtonEvent() = default;

		inline MouseCode GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	protected:
		MouseButtonEvent(MouseCode button) : m_Button(button) {}

	protected:
		MouseCode m_Button;
	};

	class HELIOSENGINE_API MouseButtonPressEvent final : public MouseButtonEvent {
	public:
		explicit MouseButtonPressEvent(MouseCode button) : MouseButtonEvent(button) {}
		explicit MouseButtonPressEvent(const MouseButtonPressEvent&) = default;
		MouseButtonPressEvent(MouseButtonPressEvent&&) = default;
		virtual ~MouseButtonPressEvent() = default;

		inline std::string ToString() const override {
			return std::format("MouseButtonPressEvent: {}", m_Button);
		}

		EVENT_CLASS_TYPE(MouseButtonPress)
	};

	class HELIOSENGINE_API MouseButtonReleaseEvent final : public MouseButtonEvent {
	public:
		explicit MouseButtonReleaseEvent(MouseCode button) : MouseButtonEvent(button) {}
		explicit MouseButtonReleaseEvent(const MouseButtonReleaseEvent&) = default;
		MouseButtonReleaseEvent(MouseButtonReleaseEvent&&) = default;
    virtual ~MouseButtonReleaseEvent() = default;

		inline std::string ToString() const override {
			return std::format("MouseButtonReleaseEvent: {}", m_Button);
		}

		EVENT_CLASS_TYPE(MouseButtonRelease)
	};
}