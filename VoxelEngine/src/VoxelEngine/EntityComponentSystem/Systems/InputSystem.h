#pragma once

#include "EntityComponentSystem/Systems/System.h"
#include "EntityComponentSystem/Components/CameraComponent.h"
#include "EntityComponentSystem/Components/CameraControllerComponent.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"
#include "Events/InputEvent.h"

namespace Engine
{
  using EventCallbackFn = std::function<void(Event&)>;

  static constexpr uint32_t MAX_MOUSE_BUTTONS = 8;

  struct VOXELENGINE_API MouseInput
  {
    glm::vec2 mousePosition = { 0.0f, 0.0f };
    glm::vec2 mouseDelta = { 0.0f, 0.0f };

    bool firstInput = true;

    std::array<bool, MAX_MOUSE_BUTTONS> mouseButtonStates;
  };

  static constexpr uint32_t MAX_KEYS = 348;

  struct VOXELENGINE_API KeyboardInput
  {
    std::array<bool, MAX_KEYS> keyStates;
  };

  class VOXELENGINE_API InputSystem : public System
  {
  public:
    virtual ~InputSystem() = default;

    std::shared_ptr<System> Clone() const override
    {
      return std::make_shared<InputSystem>(*this);
    }

    void OnUpdate(ECSManager& ecs, const Timestep deltaTime) override;
    void OnEvent(ECSManager& ecs, Event& event) override;

  private:
    const bool OnWindowFocused(WindowFocusedEvent& event);
    const bool OnWindowLostFocus(WindowLostFocusEvent& event);

    const bool OnMouseMoved(MouseMovedEvent& event);
    const bool OnMouseButtonPressed(MouseButtonPressedEvent& event); // todo
    const bool OnMouseButtonReleased(MouseButtonReleasedEvent& event); // todo

    const bool OnKeyPressed(KeyPressedEvent& event); // todo
    const bool OnKeyReleased(KeyReleasedEvent& event); // todo
  
  private:
    MouseInput m_MouseInput;
    KeyboardInput m_KeyboardInput;

    EventCallbackFn m_EventCallback;
  };
}