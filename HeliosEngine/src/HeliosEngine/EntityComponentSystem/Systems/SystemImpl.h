#include "EntityComponentSystem/Systems/System.h"

#include "EntityComponentSystem/Manager/ECSManager.h"

namespace Helios
{
  template <typename... Args>
  const ComponentMask System::GetRequiredComponents(ECSManager& ecs) const
  {
    ComponentMask mask;

    if constexpr (sizeof...(Args) > 0) {
      (..., mask.set(ecs.GetComponentID<Args>()));
    }

    return mask;
  }
}