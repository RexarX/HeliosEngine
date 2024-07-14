#pragma once

#include "VoxelEngine/KeyCodes.h"
#include "VoxelEngine/MouseButtonCodes.h"

#include <glm/glm.hpp>

namespace Engine
{
  static constexpr uint32_t MAX_KEYS = 348;
  static constexpr uint32_t MAX_MOUSE_BUTTONS = 8;

  struct VOXELENGINE_API InputComponent
  {
    glm::vec2 mousePosition = { 0.0f, 0.0f };
    glm::vec2 mouseDelta = { 0.0f, 0.0f };

    bool firstInput = true;

    std::array<bool, MAX_KEYS> keyStates;
    std::array<bool, MAX_MOUSE_BUTTONS> mouseButtonStates;
  };
}