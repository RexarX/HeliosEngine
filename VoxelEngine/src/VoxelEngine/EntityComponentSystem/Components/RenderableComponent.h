#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

#include "pch.h"

namespace Engine
{
  struct RenderableComponent
  {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
  };
}