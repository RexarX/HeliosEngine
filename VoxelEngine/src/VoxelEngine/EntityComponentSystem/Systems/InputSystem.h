#pragma once

#include "EntityComponentSystem/Systems/System.h"
#include "EntityComponentSystem/Components/MouseInputComponent.h"
#include "EntityComponentSystem/Components/KeyboardInputComponent.h"
#include "EntityComponentSystem/Components/CameraComponent.h"
#include "EntityComponentSystem/Components/CameraControllerComponent.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

namespace Engine
{
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
    const bool OnWindowFocused(ECSManager& ecs, WindowFocusedEvent& event);
    const bool OnWindowLostFocus(ECSManager& ecs, WindowLostFocusEvent& event);

    const bool OnMouseMoved(ECSManager& ecs, MouseMovedEvent& event);
    const bool OnMouseButtonPressed(ECSManager& ecs, MouseButtonPressedEvent& event);
    const bool OnMouseButtonReleased(ECSManager& ecs, MouseButtonReleasedEvent& event);

    const bool OnKeyPressed(ECSManager& ecs, KeyPressedEvent& event);
    const bool OnKeyReleased(ECSManager& ecs, KeyReleasedEvent& event);
  };
}