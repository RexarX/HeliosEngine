#pragma once

#include "HeliosEngine/Timestep.h"

#include "HeliosEngine/Events/Event.h"

#include "pch.h"

namespace Helios
{
  class HELIOSENGINE_API Script
  {
  public:
    virtual ~Script() = default;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;
    virtual void OnUpdate(const Timestep deltaTime) = 0;
    virtual void OnEvent(Event& event) = 0;
  };
}