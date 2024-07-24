#pragma once

#include "EntityComponentSystem/Systems/System.h"

namespace Helios
{
  class HELIOSENGINE_API ScriptSystem : public System
  {
  public:
    virtual ~ScriptSystem() = default;

    inline std::unique_ptr<System> Clone() const override {
      return std::make_unique<ScriptSystem>(*this);
    }

    void OnUpdate(ECSManager& ecs, const Timestep deltaTime) override;
    void OnEvent(ECSManager& ecs, Event& event) override;
  };
}