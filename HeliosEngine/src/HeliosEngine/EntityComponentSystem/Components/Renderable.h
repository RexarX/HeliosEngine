#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

namespace Helios
{
  struct Renderable
  {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
  };
}