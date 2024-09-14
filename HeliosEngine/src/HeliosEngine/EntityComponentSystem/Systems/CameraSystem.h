#pragma once

#include <entt/entt.hpp>

namespace Helios
{
  class Event;
  class WindowResizeEvent;

  class CameraSystem
  {
  public:
    CameraSystem() = default;
    ~CameraSystem() = default;

    void OnUpdate(entt::registry& registry);
    void OnEvent(entt::registry& registry, Event& event);

  private:
    bool OnWindowResize(entt::registry& registry, WindowResizeEvent& event);
  };
}