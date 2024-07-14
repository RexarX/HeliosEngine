#pragma once

#include "EntityComponentSystem/ForwardDecl.h"
#include "EntityComponentSystem/Entity/Entity.h" 

#include "VoxelEngine/Timestep.h"

#include "Events/Event.h"

namespace Engine
{
  class VOXELENGINE_API System
  {
  public:
    virtual ~System() = default;

    virtual std::unique_ptr<System> Clone() const = 0;

    virtual void OnUpdate(ECSManager& ecs, const Timestep deltaTime) = 0;
    virtual void OnEvent(ECSManager& ecs, Event& event) = 0;

    virtual const ComponentMask GetRequiredComponents(ECSManager& ecs) const = 0;
  };
}