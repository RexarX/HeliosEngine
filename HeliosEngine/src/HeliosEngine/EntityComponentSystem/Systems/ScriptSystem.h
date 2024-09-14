#pragma once

#include "HeliosEngine/Timestep.h"

#include <entt/entt.hpp>

namespace Helios
{
  class Event;

  class ScriptSystem
  {
  public:
    ScriptSystem() = default;
    ~ScriptSystem() = default;

    void OnUpdate(entt::registry& registry, Timestep deltaTime);
    void OnEvent(entt::registry& registry, Event& event);
  };
}