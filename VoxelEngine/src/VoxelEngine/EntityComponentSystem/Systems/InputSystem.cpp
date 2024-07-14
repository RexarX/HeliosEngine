#include "EntityComponentSystem/Systems/InputSystem.h"
#include "EntityComponentSystem/Systems/SystemImpl.h"
#include "EntityComponentSystem/Manager/ECSManager.h"

namespace Engine
{
  void InputSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
    auto& entities = ecs.GetEntitiesWithAnyOfComponents(
      GetRequiredComponents<MouseInputComponent, KeyboardInputComponent,
                            CameraComponent, CameraControllerComponent>(ecs)
    );

    for (const auto entity : entities) {
      auto* mouse = ecs.GetComponent<MouseInputComponent>(entity);
      auto* keyboard = ecs.GetComponent<KeyboardInputComponent>(entity);
      auto* camera = ecs.GetComponent<CameraComponent>(entity);
      auto* cameraController = ecs.GetComponent<CameraControllerComponent>(entity);

      if (mouse != nullptr) { mouse->mouseDelta = { 0.0f, 0.0f }; }
      if (keyboard != nullptr) { keyboard->keyStates.fill(false); }
      if (cameraController != nullptr) {
        cameraController->OnUpdate(deltaTime, ecs, entity);
        if (camera != nullptr) {
          camera->SetPosition(cameraController->GetPosition());
          camera->SetRotation(cameraController->GetRotation());
        }
      }
    }
  }

  void InputSystem::OnEvent(ECSManager& ecs, Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnMouseMoved, ecs));
    dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnMouseButtonPressed, ecs));
    dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnMouseButtonReleased, ecs));
    dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnKeyPressed, ecs));
    dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnKeyReleased, ecs));
  }

  const bool InputSystem::OnMouseMoved(ECSManager& ecs, MouseMovedEvent& event)
  {
    auto& entities = ecs.GetEntitiesWithComponents(GetRequiredComponents<MouseInputComponent>(ecs));
    for (const auto entity : entities) {
      auto* mouse = ecs.GetComponent<MouseInputComponent>(entity);
      auto* camera = ecs.GetComponent<CameraComponent>(entity);
      auto* cameraController = ecs.GetComponent<CameraControllerComponent>(entity);

      if (mouse != nullptr) {
        if (mouse->firstInput) {
          mouse->mousePosition = { event.GetX(), event.GetY() };
          mouse->firstInput = false;
        }
        else {
          mouse->mouseDelta = { event.GetX() - mouse->mousePosition.x,
                                event.GetY() - mouse->mousePosition.y };

          mouse->mousePosition = { event.GetX(), event.GetY() };
        }

        if (cameraController != nullptr) {
          cameraController->OnMouseMoved(mouse->mousePosition, mouse->mouseDelta);
          if (camera != nullptr) {
            camera->SetRotation(cameraController->GetRotation());
          }
        }
      }
    }
    return true;
  }

  const bool InputSystem::OnMouseButtonPressed(ECSManager& ecs, MouseButtonPressedEvent& event)
  {
    auto& entities = ecs.GetEntitiesWithComponents(GetRequiredComponents<MouseInputComponent>(ecs));
    for (const auto entity : entities) {
      auto* mouse = ecs.GetComponent<MouseInputComponent>(entity);
      if (mouse != nullptr) { mouse->mouseButtonStates[event.GetMouseButton()] = true; }
    }
    return true;
  }

  const bool InputSystem::OnMouseButtonReleased(ECSManager& ecs, MouseButtonReleasedEvent& event)
  {
    auto& entities = ecs.GetEntitiesWithComponents(GetRequiredComponents< MouseInputComponent>(ecs));
    for (const auto entity : entities) {
      auto* mouse = ecs.GetComponent<MouseInputComponent>(entity);
      if (mouse != nullptr) { mouse->mouseButtonStates[event.GetMouseButton()] = false; }
    }
    return true;
  }

  const bool InputSystem::OnKeyPressed(ECSManager& ecs, KeyPressedEvent& event)
  {
    auto& entities = ecs.GetEntitiesWithComponents(GetRequiredComponents<KeyboardInputComponent>(ecs));
    for (const auto entity : entities) {
      auto* keyboard = ecs.GetComponent<KeyboardInputComponent>(entity);
      auto* cameraController = ecs.GetComponent<CameraControllerComponent>(entity);
      if (keyboard != nullptr) { keyboard->keyStates[event.GetKeyCode()] = true; }
      if (cameraController != nullptr) { cameraController->OnKeyPressed(event.GetKeyCode()); }
    }
    return true;
  }

  const bool InputSystem::OnKeyReleased(ECSManager& ecs, KeyReleasedEvent& event)
  {
    auto& entities = ecs.GetEntitiesWithComponents(GetRequiredComponents<KeyboardInputComponent>(ecs));
    for (const auto entity : entities) {
      auto* keyboard = ecs.GetComponent<KeyboardInputComponent>(entity);
      if (keyboard != nullptr) { keyboard->keyStates[event.GetKeyCode()] = false; }
    }
    return true;
  }
}