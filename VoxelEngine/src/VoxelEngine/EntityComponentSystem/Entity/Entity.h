#pragma once 

namespace Engine
{
  constexpr uint32_t MAX_ENTITIES = 10000;
  constexpr uint32_t MAX_COMPONENTS = 64;

  using EntityID = int32_t;
  using ComponentMask = std::bitset<MAX_COMPONENTS>;

  struct VOXELENGINE_API Entity
  {
    EntityID id;
    ComponentMask mask;
  };
}