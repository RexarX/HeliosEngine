#pragma once

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include <entt/entt.hpp>

namespace Helios
{
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