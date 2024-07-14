#pragma once

#include "Event.h"

#include "VoxelEngine/MouseButtonCodes.h"

namespace Engine 
{
	class VOXELENGINE_API MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(const float x, const float y)
			: m_MouseX(x), m_MouseY(y) 
		{
		}

		inline const float GetX() const { return m_MouseX; }
		inline const float GetY() const { return m_MouseY; }

		const std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_MouseX, m_MouseY;
	};

	class VOXELENGINE_API MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(const float xOffset, const float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) 
		{
		}

		inline const float GetXOffset() const { return m_XOffset; }
		inline const float GetYOffset() const { return m_YOffset; }

		const std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset, m_YOffset;
	};

	class VOXELENGINE_API MouseButtonEvent : public Event
	{
	public:
		inline const MouseCode GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	protected:
		MouseButtonEvent(const MouseCode button)
			: m_Button(button) 
		{
		}

		MouseCode m_Button;
	};

	class VOXELENGINE_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const MouseCode button)
			: MouseButtonEvent(button) 
		{
		}

		const std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class VOXELENGINE_API MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(MouseCode button)
			: MouseButtonEvent(button) 
		{
		}

		const std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}