#pragma once

#include "EntityComponentSystem/Systems/System.h"

#include "Renderer/GraphicsContext.h"

namespace Helios
{
  class HELIOSENGINE_API RenderingSystem : public System
  {
  public:
    virtual ~RenderingSystem() = default;

    inline std::unique_ptr<System> Clone() const override {
      return std::make_unique<RenderingSystem>(*this);
    }

    void OnUpdate(ECSManager& ecs, const Timestep deltaTime) override;
    void OnEvent(ECSManager& ecs, Event& event) override;

  private:
    std::shared_ptr<GraphicsContext> m_GraphicsContext = nullptr;
  };
}