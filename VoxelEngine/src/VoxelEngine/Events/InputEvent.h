#pragma once

#include "Event.h"

#include "VoxelEngine/KeyCodes.h"
#include "VoxelEngine/MouseButtonCodes.h"

namespace Engine
{
  class VOXELENGINE_API MouseMovedAction : public Event
  {
  public:
    MouseMovedAction() = default;
    MouseMovedAction(const float mouseX, const float mouseY, const float deltaX, const float deltaY)
      : m_MouseX(mouseX), m_MouseY(mouseY), m_DeltaX(deltaX), m_DeltaY(deltaY) {
    }

    inline const float GetX() const { return m_MouseX; }
    inline const float GetY() const { return m_MouseY; }

    inline const float GetDeltaX() const { return m_DeltaX; }
    inline const float GetDeltaY() const { return m_DeltaY; }

    const std::string ToString() const override
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

  class VOXELENGINE_API MouseButtonPressedAction : public Event
  {
  public:
    MouseButtonPressedAction() = default;
    MouseButtonPressedAction(const MouseCode button)
      : m_Button(button) {
    }

    inline const float GetMouseButton() const { return m_Button; }

    const std::string ToString() const override
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

  class VOXELENGINE_API MouseButtonReleasedAction : public Event
  {
  public:
    MouseButtonReleasedAction() = default;
    MouseButtonReleasedAction(const MouseCode button)
      : m_Button(button) {
    }

    inline const float GetMouseButton() const { return m_Button; }

    const std::string ToString() const override
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

  class VOXELENGINE_API KeyPressedAction : public Event
  {
  public:
    KeyPressedAction() = default;
    KeyPressedAction(const KeyCode keycode, const uint32_t repeatCount)
      : m_KeyCode(keycode), m_RepeatCount(repeatCount) {
    }

    inline const KeyCode GetKeyCode() const { return m_KeyCode; }

    inline const uint32_t GetRepeatCount() const { return m_RepeatCount; }

    const std::string ToString() const override
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

  class VOXELENGINE_API KeyReleasedAction : public Event
  {
  public:
    KeyReleasedAction() = default;
    KeyReleasedAction(const KeyCode keycode)
      : m_KeyCode(keycode) {
    }

    inline const KeyCode GetKeyCode() const { return m_KeyCode; }

    const std::string ToString() const override
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