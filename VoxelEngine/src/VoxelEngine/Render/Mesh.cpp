#include "Mesh.h"

namespace VoxelEngine
{
  std::shared_ptr<Mesh> Mesh::Create() { return std::make_shared<Mesh>(); }
}