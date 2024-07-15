#include "EntityComponentSystem/Manager/ECSManager.h"

#include "EntityComponentSystem/Systems/System.h"

namespace Engine
{
  ECSManager::ECSManager(const ECSManager& other)
    : m_Entities(other.m_Entities), m_FreeEntities(other.m_FreeEntities),
    m_ComponentArrays(other.m_ComponentArrays), m_ComponentSizes(other.m_ComponentSizes),
    m_ComponentCounter(other.m_ComponentCounter), m_SortedSystems(other.m_SortedSystems)
  {
    for (const auto& [type_index, system] : other.m_SystemMap) {
      m_SystemMap[type_index] = std::move(system->Clone());
    }
  }

  ECSManager::ECSManager(ECSManager&& other) noexcept
    : m_Entities(other.m_Entities), m_FreeEntities(other.m_FreeEntities),
    m_ComponentArrays(other.m_ComponentArrays), m_ComponentSizes(other.m_ComponentSizes),
    m_ComponentCounter(other.m_ComponentCounter)
  {
    other.m_Entities.clear();
    other.m_FreeEntities.clear();
    
    for (auto& array : other.m_ComponentArrays) {
      array.clear();
    }

    other.m_ComponentCounter = 0;

    for (auto& [type_index, system] : other.m_SystemMap) {
      m_SystemMap[type_index] = std::move(system);
    }

    other.m_SystemMap.clear();

    m_SortedSystems.insert(m_SortedSystems.begin(), std::make_move_iterator(other.m_SortedSystems.begin()),
                                                    std::make_move_iterator(other.m_SortedSystems.end()));

    other.m_SortedSystems.clear();
  }

  const EntityID ECSManager::CreateEntity()
  {
    EntityID id;

    if (m_FreeEntities.empty()) {
      id = static_cast<EntityID>(m_Entities.size());
      m_Entities.insert(m_Entities.end(), Entity{ id, ComponentMask() });
    }
    else {
      id = m_FreeEntities.back();
      m_FreeEntities.pop_back();
      m_Entities[id] = Entity{ id, ComponentMask() };
    }

    return id;
  }

  void ECSManager::DestroyEntity(const EntityID entity)
  {
    CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

    m_Entities[entity].mask.reset();
    m_FreeEntities.push_back(entity);
  }

  const std::vector<EntityID>& ECSManager::GetEntitiesWithComponents(const ComponentMask mask) const
  {
    static std::vector<EntityID> result;
    result.clear();

    for (const auto& entity : m_Entities) {
      if ((entity.mask & mask) == mask) {
        result.push_back(entity.id);
      }
    }

    return result;
  }

  const std::vector<EntityID>& ECSManager::GetEntitiesWithAnyOfComponents(const ComponentMask mask) const
  {
    static std::vector<EntityID> result;
    result.clear();

    for (const auto& entity : m_Entities) {
      if ((entity.mask & mask).any()) {
        result.push_back(entity.id);
      }
    }

    return result;
  }

  void ECSManager::OnUpdateSystems(const Timestep deltaTime)
  {
    for (auto& entry : m_SortedSystems) {
      entry.system->OnUpdate(*this, deltaTime);
    }
  }

  void ECSManager::OnEventSystems(Event& event)
  {
    for (auto& entry : m_SortedSystems) {
      entry.system->OnEvent(*this, event);
    }
  }

  ECSManager& ECSManager::operator=(const ECSManager& other)
  {
    m_Entities = other.m_Entities;
    m_FreeEntities = other.m_FreeEntities;
    m_ComponentArrays = other.m_ComponentArrays;
    m_ComponentSizes = other.m_ComponentSizes;
    m_ComponentCounter = other.m_ComponentCounter;

    for (const auto& [type_index, system] : other.m_SystemMap) {
      m_SystemMap[type_index] = std::move(system->Clone());
    }

    m_SortedSystems = other.m_SortedSystems;

    return *this;
  }

  ECSManager& ECSManager::operator=(ECSManager&& other) noexcept
  {
    m_Entities = other.m_Entities;
    m_FreeEntities = other.m_FreeEntities;
    m_ComponentArrays = other.m_ComponentArrays;
    m_ComponentSizes = other.m_ComponentSizes;
    m_ComponentCounter = other.m_ComponentCounter;

    other.m_Entities.clear();
    other.m_FreeEntities.clear();

    for (auto& array : other.m_ComponentArrays) {
      array.clear();
    }

    other.m_ComponentCounter = 0;

    for (auto& [type_index, system] : other.m_SystemMap) {
      m_SystemMap[type_index] = std::move(system);
    }

    other.m_SystemMap.clear();

    m_SortedSystems.insert(m_SortedSystems.begin(), std::make_move_iterator(other.m_SortedSystems.begin()),
                                                    std::make_move_iterator(other.m_SortedSystems.end()));

    other.m_SortedSystems.clear();

    return *this;
  }
}