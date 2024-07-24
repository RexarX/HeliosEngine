#pragma once

#include "Event.h"

#include "HeliosEngine/KeyCodes.h"

namespace Helios 
{
	class HELIOSENGINE_API KeyEvent : public Event
	{
	public:
		inline const KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

	protected:
		KeyEvent(const KeyCode keycode)
			: m_KeyCode(keycode) 
		{
		}

		KeyCode m_KeyCode;
	};

	class HELIOSENGINE_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keycode, const uint32_t repeatCount)
			: KeyEvent(keycode), m_RepeatCount(repeatCount) 
		{
		}

		inline const uint32_t GetRepeatCount() const { return m_RepeatCount; }

		const std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		uint32_t m_RepeatCount;
	};

	class HELIOSENGINE_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keycode)
			: KeyEvent(keycode) 
		{
		}

		const std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};
}