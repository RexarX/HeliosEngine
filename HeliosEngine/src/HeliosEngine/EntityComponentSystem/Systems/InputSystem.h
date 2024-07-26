#pragma once

#include "EntityComponentSystem/Systems/System.h"
#include "EntityComponentSystem/Systems/EventSystem.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"
#include "Events/InputEvent.h"

#include <glm/glm.hpp>

namespace Helios
{
  static constexpr uint32_t MAX_MOUSE_BUTTONS = 8;

  struct HELIOSENGINE_API MouseInput
  {
    glm::vec2 mousePosition = { 0.0f, 0.0f };
    glm::vec2 mouseDelta = { 0.0f, 0.0f };

    bool firstInput = true;

    std::array<bool, MAX_MOUSE_BUTTONS> mouseButtonStates;
  };

  static constexpr uint32_t MAX_KEYS = 348;

  struct HELIOSENGINE_API KeyboardInput
  {
    std::array<bool, MAX_KEYS> keyStates;
  };

  class HELIOSENGINE_API InputSystem : public System
  {
  public:
    InputSystem() = default;
    virtual ~InputSystem() = default;

    inline std::unique_ptr<System> Clone() const override {
      return std::make_unique<InputSystem>(*this);
    }

    void OnUpdate(ECSManager& ecs, const Timestep deltaTime) override;
    void OnEvent(ECSManager& ecs, Event& event) override;

  private:
    const bool OnWindowFocused(EventSystem& eventSystem, WindowFocusedEvent& event);
    const bool OnWindowLostFocus(EventSystem& eventSystem, WindowLostFocusEvent& event);

    const bool OnMouseMoved(EventSystem& eventSystem, MouseMovedEvent& event);
    const bool OnMouseButtonPressed(EventSystem& eventSystem, MouseButtonPressedEvent& event); // todo
    const bool OnMouseButtonReleased(EventSystem& eventSystem, MouseButtonReleasedEvent& event); // todo

    const bool OnKeyPressed(EventSystem& eventSystem, KeyPressedEvent& event); // todo
    const bool OnKeyReleased(EventSystem& eventSystem, KeyReleasedEvent& event); // todo
  
  private:
    MouseInput m_MouseInput;
    KeyboardInput m_KeyboardInput;
  };
}