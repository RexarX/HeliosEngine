#pragma once

#include "Event.h"

#include "HeliosEngine/KeyCodes.h"

namespace Helios 
{
	class HELIOSENGINE_API KeyEvent : public Event
	{
	public:
		virtual ~KeyEvent() = default;

		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

	protected:
		KeyEvent(KeyCode keycode) : m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};

	class HELIOSENGINE_API KeyPressedEvent final : public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode keycode, uint32_t repeatCount)
			: KeyEvent(keycode), m_RepeatCount(repeatCount) 
		{
		}

		virtual ~KeyPressedEvent() = default;

		inline uint32_t GetRepeatCount() const { return m_RepeatCount; }

	  std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		uint32_t m_RepeatCount;
	};

	class HELIOSENGINE_API KeyReleasedEvent final : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keycode) : KeyEvent(keycode) {}
    virtual ~KeyReleasedEvent() = default;

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};
}