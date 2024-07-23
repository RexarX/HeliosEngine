#include "EntityComponentSystem/Manager/ECSManager.h"

#include "EntityComponentSystem/Systems/System.h"
#include "EntityComponentSystem/Systems/EventSystem.h"

namespace Engine
{
  ComponentRegistry::ComponentRegistry(const ComponentRegistry& other)
    : m_ComponentIDs(other.m_ComponentIDs), m_NextID(other.m_NextID)
  {
  }

  ComponentRegistry::ComponentRegistry(ComponentRegistry&& other) noexcept
    : m_ComponentIDs(std::move(other.m_ComponentIDs)), m_NextID(other.m_NextID)
  {
    other.m_ComponentIDs.clear();
    other.m_NextID = 0;
  }

  const ComponentID ComponentRegistry::GetID(const std::type_info& type)
  {
    std::type_index typeIndex(type);
    auto it = m_ComponentIDs.find(typeIndex);
    if (it == m_ComponentIDs.end()) {
      m_ComponentIDs[typeIndex] = m_NextID;
      return m_NextID++;
    }
    return it->second;
  }

  ComponentRegistry& ComponentRegistry::operator=(const ComponentRegistry& other)
  {
    m_ComponentIDs = other.m_ComponentIDs;
    m_NextID = other.m_NextID;

    return *this;
  }

  ComponentRegistry& ComponentRegistry::operator=(ComponentRegistry&& other) noexcept
  {
    m_ComponentIDs = std::move(other.m_ComponentIDs);
    m_NextID = other.m_NextID;

    other.m_ComponentIDs.clear();
    other.m_NextID = 0;

    return *this;
  }

  ECSManager::ECSManager(const ECSManager& other)
    : m_Entities(other.m_Entities), m_FreeEntities(other.m_FreeEntities),
    m_ComponentRegistry(other.m_ComponentRegistry),
    m_ComponentArrays(other.m_ComponentArrays), m_ComponentSizes(other.m_ComponentSizes),
    m_ComponentDestructors(other.m_ComponentDestructors), m_SortedSystems(other.m_SortedSystems)
  {
    for (const auto& [type_index, system] : other.m_SystemMap) {
      m_SystemMap[type_index] = std::move(system->Clone());
    }
  }

  ECSManager::ECSManager(ECSManager&& other) noexcept
    : m_Entities(std::move(other.m_Entities)), m_FreeEntities(std::move(other.m_FreeEntities)),
    m_ComponentRegistry(std::move(other.m_ComponentRegistry)), m_ComponentArrays(std::move(other.m_ComponentArrays)),
    m_ComponentSizes(std::move(other.m_ComponentSizes)), m_ComponentDestructors(std::move(other.m_ComponentDestructors)),
    m_SortedSystems(std::move(other.m_SortedSystems)), m_SystemMap(std::move(other.m_SystemMap))

  {
    other.m_Entities.clear();
    other.m_FreeEntities.clear();

    for (auto& array : other.m_ComponentArrays) {
      array.clear();
    }

    other.m_SystemMap.clear();
    other.m_SortedSystems.clear();
  }

  ECSManager::~ECSManager()
  {
    for (EntityID entity = 0; entity < m_Entities.size(); ++entity) {
      DestroyEntity(entity);
    }
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

    for (ComponentID id = 0; id < m_ComponentRegistry.GetComponentCount(); ++id) {
      if (m_Entities[entity].mask.test(id)) {
        m_ComponentDestructors[id](m_ComponentArrays[id].data() + entity * m_ComponentSizes[id]);
        std::memset(m_ComponentArrays[id].data() + entity * m_ComponentSizes[id], 0, m_ComponentSizes[id]);
      }
    }

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
    auto& eventSystem = GetSystem<EventSystem>();
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
    if (this == &other) { return *this; }

    m_Entities = other.m_Entities;
    m_FreeEntities = other.m_FreeEntities;
    m_ComponentRegistry = other.m_ComponentRegistry;
    m_ComponentArrays = other.m_ComponentArrays;
    m_ComponentSizes = other.m_ComponentSizes;
    m_ComponentDestructors = other.m_ComponentDestructors;

    for (const auto& [type_index, system] : other.m_SystemMap) {
      m_SystemMap[type_index] = std::move(system->Clone());
    }

    m_SortedSystems = other.m_SortedSystems;

    return *this;
  }

  ECSManager& ECSManager::operator=(ECSManager&& other) noexcept
  {
    if (this == &other) { return *this; }

    m_Entities = std::move(other.m_Entities);
    m_FreeEntities = std::move(other.m_FreeEntities);
    m_ComponentArrays = std::move(other.m_ComponentArrays);
    m_ComponentSizes = std::move(other.m_ComponentSizes);
    m_ComponentDestructors = std::move(other.m_ComponentDestructors);

    other.m_Entities.clear();
    other.m_FreeEntities.clear();

    for (auto& array : other.m_ComponentArrays) {
      array.clear();
    }

    other.m_SystemMap.clear();
    other.m_SortedSystems.clear();

    return *this;
  }
}