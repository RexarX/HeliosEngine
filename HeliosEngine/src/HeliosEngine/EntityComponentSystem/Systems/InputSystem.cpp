#include "EntityComponentSystem/Systems/InputSystem.h"
#include "EntityComponentSystem/Systems/SystemImpl.h"

namespace Helios
{
  void InputSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
  }

  void InputSystem::OnEvent(ECSManager& ecs, Event& event)
  {
    EventSystem& eventSystem = ecs.GetSystem<EventSystem>();

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowFocusedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnWindowFocused, eventSystem));
    dispatcher.Dispatch<WindowLostFocusEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnWindowLostFocus, eventSystem));
    dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnMouseMoved, eventSystem));
    dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnMouseButtonPressed, eventSystem));
    dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnMouseButtonReleased, eventSystem));
    dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnKeyPressed, eventSystem));
    dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN_WITH_REF(InputSystem::OnKeyReleased, eventSystem));
  }

  const bool InputSystem::OnWindowFocused(EventSystem& eventSystem, WindowFocusedEvent& event)
  {
    m_MouseInput.firstInput = true;
    return true;
  }

  const bool InputSystem::OnWindowLostFocus(EventSystem& eventSystem, WindowLostFocusEvent& event)
  {
    m_MouseInput.firstInput = true;
    return true;
  }

  const bool InputSystem::OnMouseMoved(EventSystem& eventSystem, MouseMovedEvent& event)
  {
    if (m_MouseInput.firstInput) {
      m_MouseInput.firstInput = false;
    } 
    else {
      m_MouseInput.mouseDelta = { event.GetX() - m_MouseInput.mousePosition.x,
                                  event.GetY() - m_MouseInput.mousePosition.y };
    }
    m_MouseInput.mousePosition = { event.GetX(), event.GetY() };

    MouseMovedAction action(m_MouseInput.mousePosition.x, m_MouseInput.mousePosition.y,
                            m_MouseInput.mouseDelta.x, m_MouseInput.mouseDelta.y);

    eventSystem.PushEvent(action);

    return true;
  }

  const bool InputSystem::OnMouseButtonPressed(EventSystem& eventSystem, MouseButtonPressedEvent& event)
  {
    m_MouseInput.mouseButtonStates[event.GetMouseButton()] = true;

    MouseButtonPressedAction action(event.GetMouseButton());

    eventSystem.PushEvent(action);

    return true;
  }

  const bool InputSystem::OnMouseButtonReleased(EventSystem& eventSystem, MouseButtonReleasedEvent& event)
  {
    m_MouseInput.mouseButtonStates[event.GetMouseButton()] = false;

    MouseButtonReleasedAction action(event.GetMouseButton());

    eventSystem.PushEvent(action);

    return true;
  }

  const bool InputSystem::OnKeyPressed(EventSystem& eventSystem, KeyPressedEvent& event)
  {
    m_KeyboardInput.keyStates[event.GetKeyCode()] = true;

    KeyPressedAction action(event.GetKeyCode(), event.GetRepeatCount());

    eventSystem.PushEvent(action);

    return true;
  }

  const bool InputSystem::OnKeyReleased(EventSystem& eventSystem, KeyReleasedEvent& event)
  {
    m_KeyboardInput.keyStates[event.GetKeyCode()] = false;

    KeyReleasedAction action(event.GetKeyCode());

    eventSystem.PushEvent(action);
    return true;
  }
}