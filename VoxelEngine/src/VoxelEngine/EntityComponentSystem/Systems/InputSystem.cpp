#include "EntityComponentSystem/Systems/InputSystem.h"
#include "EntityComponentSystem/Manager/ECSManager.h"

namespace Engine
{
  void InputSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
    auto entities = ecs.GetEntitiesWithComponents(GetRequiredComponents(ecs));
    for (auto entity : entities) {
      auto* input = ecs.GetComponent<InputComponent>(entity);
      auto* camera = ecs.GetComponent<CameraComponent>(entity);
      auto* controller = ecs.GetComponent<CameraControllerComponent>(entity);

      if (input != nullptr) { input->mouseDelta = { 0.0f, 0.0f }; }
      if (controller != nullptr) {
        controller->OnUpdate(deltaTime, ecs, entity);
        if (camera != nullptr) {
          camera->SetPosition(controller->GetPosition());
          camera->SetRotation(controller->GetRotation());
        }
      }
    }
  }

  void InputSystem::OnEvent(ECSManager& ecs, Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseMovedEvent>(std::bind(&InputSystem::OnMouseMoved, this, std::ref(ecs), std::placeholders::_1));
    dispatcher.Dispatch<MouseButtonPressedEvent>(std::bind(&InputSystem::OnMouseButtonPressed, this, std::ref(ecs), std::placeholders::_1));
    dispatcher.Dispatch<MouseButtonReleasedEvent>(std::bind(&InputSystem::OnMouseButtonReleased, this, std::ref(ecs), std::placeholders::_1));
    dispatcher.Dispatch<KeyPressedEvent>(std::bind(&InputSystem::OnKeyPressed, this, std::ref(ecs), std::placeholders::_1));
    dispatcher.Dispatch<KeyReleasedEvent>(std::bind(&InputSystem::OnKeyReleased, this, std::ref(ecs), std::placeholders::_1));
  }

  const ComponentMask InputSystem::GetRequiredComponents(ECSManager& ecs) const
  {
    ComponentMask mask;
    mask.set(ecs.GetComponentID<InputComponent>());
    return mask;
  }

  const bool InputSystem::OnMouseMoved(ECSManager& ecs, MouseMovedEvent& event)
  {
    auto entities = ecs.GetEntitiesWithComponents(GetRequiredComponents(ecs));
    for (auto entity : entities) {
      auto* input = ecs.GetComponent<InputComponent>(entity);
      auto* camera = ecs.GetComponent<CameraComponent>(entity);
      auto* controller = ecs.GetComponent<CameraControllerComponent>(entity);

      if (input != nullptr) {
        if (input->firstInput) {
          input->mousePosition = { event.GetX(), event.GetY() };
          input->firstInput = false;
        }
        else {
          input->mouseDelta = { event.GetX() - input->mousePosition.x,
                                event.GetY() - input->mousePosition.y };

          input->mousePosition = { event.GetX(), event.GetY() };
        }

        if (controller != nullptr) {
          controller->OnMouseMoved(input->mousePosition, input->mouseDelta);
          if (camera != nullptr) {
            camera->SetRotation(controller->GetRotation());
          }
        }
      }
    }
    return true;
  }

  const bool InputSystem::OnMouseButtonPressed(ECSManager& ecs, MouseButtonPressedEvent& event)
  {
    auto entities = ecs.GetEntitiesWithComponents(GetRequiredComponents(ecs));
    for (auto entity : entities) {
      auto* input = ecs.GetComponent<InputComponent>(entity);
      if (input != nullptr) { input->mouseButtonStates[event.GetMouseButton()] = true; }
    }
    return true;
  }

  const bool InputSystem::OnMouseButtonReleased(ECSManager& ecs, MouseButtonReleasedEvent& event)
  {
    auto entities = ecs.GetEntitiesWithComponents(GetRequiredComponents(ecs));
    for (auto entity : entities) {
      auto* input = ecs.GetComponent<InputComponent>(entity);
      if (input != nullptr) { input->mouseButtonStates[event.GetMouseButton()] = false; }
    }
    return true;
  }

  const bool InputSystem::OnKeyPressed(ECSManager& ecs, KeyPressedEvent& event)
  {
    auto entities = ecs.GetEntitiesWithComponents(GetRequiredComponents(ecs));
    for (auto entity : entities) {
      auto* input = ecs.GetComponent<InputComponent>(entity);
      auto* controller = ecs.GetComponent<CameraControllerComponent>(entity);
      if (input != nullptr) { input->keyStates[event.GetKeyCode()] = true; }
      if (controller != nullptr) { controller->OnKeyPressed(event.GetKeyCode()); }
    }
    return true;
  }

  const bool InputSystem::OnKeyReleased(ECSManager& ecs, KeyReleasedEvent& event)
  {
    auto entities = ecs.GetEntitiesWithComponents(GetRequiredComponents(ecs));
    for (auto entity : entities) {
      auto* input = ecs.GetComponent<InputComponent>(entity);
      if (input != nullptr) { input->keyStates[event.GetKeyCode()] = false; }
    }
    return true;
  }
}