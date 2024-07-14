#pragma once

#include "VoxelEngine/KeyCodes.h"

#include "pch.h"

#include <glm/glm.hpp>

namespace Engine
{
  static constexpr uint32_t MAX_KEYS = 348;

  struct VOXELENGINE_API KeyboardInputComponent
  {
    std::array<bool, MAX_KEYS> keyStates;
  };
}