#pragma once 

namespace Helios
{
  constexpr uint32_t MAX_ENTITIES = 10000;
  constexpr uint32_t MAX_COMPONENTS = 64;

  using EntityID = uint32_t;
  using ComponentMask = std::bitset<MAX_COMPONENTS>;

  struct HELIOSENGINE_API Entity
  {
    EntityID id;
    ComponentMask mask;
  };
}