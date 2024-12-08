#pragma once

#include "Event.h"

#include "HeliosEngine/KeyCodes.h"

namespace Helios  {
	class HELIOSENGINE_API KeyEvent : public Event {
	public:
		virtual ~KeyEvent() = default;

		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

	protected:
		explicit KeyEvent(KeyCode keycode) : m_KeyCode(keycode) {}

	protected:
		KeyCode m_KeyCode;
	};

	class HELIOSENGINE_API KeyPressEvent final : public KeyEvent {
	public:
		explicit KeyPressEvent(KeyCode keycode, uint32_t repeatCount)
			: KeyEvent(keycode), m_RepeatCount(repeatCount)  {}

		explicit KeyPressEvent(const KeyPressEvent&) = default;
		KeyPressEvent(KeyPressEvent&&) = default;
		virtual ~KeyPressEvent() = default;

		inline uint32_t GetRepeatCount() const { return m_RepeatCount; }

		inline std::string ToString() const override {
			return std::format("KeyPressEvent: {} ({} repeats)", m_KeyCode, m_RepeatCount);
		}

		EVENT_CLASS_TYPE(KeyPress)

	private:
		uint32_t m_RepeatCount = 0;
	};

	class HELIOSENGINE_API KeyReleaseEvent final : public KeyEvent {
	public:
		explicit KeyReleaseEvent(KeyCode keycode) : KeyEvent(keycode) {}
		explicit KeyReleaseEvent(const KeyReleaseEvent&) = default;
		KeyReleaseEvent(KeyReleaseEvent&&) = default;
    virtual ~KeyReleaseEvent() = default;

		inline std::string ToString() const override {
			return std::format("KeyReleaseEvent: {}", m_KeyCode);
		}

		EVENT_CLASS_TYPE(KeyRelease)
	};
}