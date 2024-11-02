#pragma once

#include "pch.h"

#include <entt/entt.hpp>

namespace Helios
{
  class ScriptSystem
  {
  public:
    ScriptSystem() = default;
    ~ScriptSystem() = default;

    void OnUpdate(entt::registry& registry, Timestep deltaTime);
    void OnEvent(entt::registry& registry, const Event& event);
  };
}