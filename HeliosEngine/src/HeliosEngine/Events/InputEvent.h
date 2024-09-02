#pragma once

#include "Event.h"

#include "HeliosEngine/KeyCodes.h"
#include "HeliosEngine/MouseButtonCodes.h"

namespace Helios
{
  class HELIOSENGINE_API MouseMovedAction final : public Event
  {
  public:
    MouseMovedAction() = default;
    MouseMovedAction(float mouseX, float mouseY, float deltaX, float deltaY)
      : m_MouseX(mouseX), m_MouseY(mouseY), m_DeltaX(deltaX), m_DeltaY(deltaY)
    {
    }

    virtual ~MouseMovedAction() = default;

    inline float GetX() const { return m_MouseX; }
    inline float GetY() const { return m_MouseY; }

    inline float GetDeltaX() const { return m_DeltaX; }
    inline float GetDeltaY() const { return m_DeltaY; }

    std::string ToString() const override
    {
      std::stringstream ss;
      ss << "MouseMovedAction:" << m_MouseX << ", " << m_MouseY << ", " << m_DeltaX << ", " << m_DeltaY;
      return ss.str();
    }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    float m_MouseX, m_MouseY;
    float m_DeltaX, m_DeltaY;
  };

  class HELIOSENGINE_API MouseButtonPressedAction final : public Event
  {
  public:
    MouseButtonPressedAction() = default;
    MouseButtonPressedAction(MouseCode button) : m_Button(button) {}
    virtual ~MouseButtonPressedAction() = default;

    inline float GetMouseButton() const { return m_Button; }

    std::string ToString() const override
    {
      std::stringstream ss;
      ss << "MouseButtonPressedAction: " << m_Button;
      return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    MouseCode m_Button;
  };

  class HELIOSENGINE_API MouseButtonReleasedAction final : public Event
  {
  public:
    MouseButtonReleasedAction() = default;
    MouseButtonReleasedAction(MouseCode button) : m_Button(button) {}
    virtual ~MouseButtonReleasedAction() = default;

    inline float GetMouseButton() const { return m_Button; }

    std::string ToString() const override
    {
      std::stringstream ss;
      ss << "MouseButtonReleasedAction: " << m_Button;
      return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonReleased)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    MouseCode m_Button;
  };

  class HELIOSENGINE_API KeyPressedAction final : public Event
  {
  public:
    KeyPressedAction() = default;
    KeyPressedAction(KeyCode keycode, uint32_t repeatCount)
      : m_KeyCode(keycode), m_RepeatCount(repeatCount)
    {
    }

    virtual ~KeyPressedAction() = default;

    inline KeyCode GetKeyCode() const { return m_KeyCode; }

    inline uint32_t GetRepeatCount() const { return m_RepeatCount; }

    std::string ToString() const override
    {
      std::stringstream ss;
      ss << "KeyPressedAction: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
      return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

  private:
    KeyCode m_KeyCode;
    uint32_t m_RepeatCount;
  };

  class HELIOSENGINE_API KeyReleasedAction final : public Event
  {
  public:
    KeyReleasedAction() = default;
    KeyReleasedAction(KeyCode keycode) : m_KeyCode(keycode) {}
    virtual ~KeyReleasedAction() = default;

    inline KeyCode GetKeyCode() const { return m_KeyCode; }

    std::string ToString() const override
    {
      std::stringstream ss;
      ss << "KeyReleasedAction: " << m_KeyCode;
      return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

  private:
    KeyCode m_KeyCode;
  };
}