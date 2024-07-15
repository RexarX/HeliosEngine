#pragma once

#include "EntityComponentSystem/Systems/System.h"

#include "pch.h"

namespace Engine
{
  using ComponentID = uint32_t;

  class VOXELENGINE_API ECSManager
  {
  public:
    ECSManager() = default;
    ECSManager(const ECSManager&);
    ECSManager(ECSManager&&) noexcept;
    ~ECSManager() = default;

    const EntityID CreateEntity();

    void DestroyEntity(const EntityID entity);

    template<typename T>
    const ComponentID RegisterComponent()
    {
      CORE_ASSERT(m_ComponentCounter < MAX_COMPONENTS, "Too many components!");

      ComponentID id = GetComponentID<T>();

      m_ComponentArrays[id].resize(MAX_ENTITIES * sizeof(T));
      m_ComponentSizes[id] = sizeof(T);

      return id;
    }

    template<typename T>
    void AddComponent(const EntityID entity, const T& component)
    {
      CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

      ComponentID componentID = GetComponentID<T>();

      CORE_ASSERT(componentID < m_ComponentCounter, "Component does not exist!");

      std::memcpy(&m_ComponentArrays[componentID][entity * m_ComponentSizes[componentID]], &component, sizeof(T));

      m_Entities[entity].mask.set(componentID);
    }

    template<typename T>
    void RemoveComponent(const EntityID entity)
    {
      CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

      ComponentID componentID = GetComponentID<T>();

      CORE_ASSERT(componentID < m_ComponentCounter, "Component does not exist!");

      m_Entities[entity].mask.reset(componentID);
    }

    template<typename T>
    T* GetComponent(const EntityID entity)
    {
      CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

      ComponentID componentID = GetComponentID<T>();

      CORE_ASSERT(componentID < m_ComponentCounter, "Component does not exist!");

      if (!m_Entities[entity].mask.test(componentID)) { return nullptr; }

      return reinterpret_cast<T*>(&m_ComponentArrays[componentID][entity * m_ComponentSizes[componentID]]);
    }

    template<typename T>
    const ComponentID GetComponentID()
    {
      static ComponentID id = m_ComponentCounter++;
      return id;
    }

    template<typename T>
    const bool HasComponent(const EntityID entity) const
    {
      CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

      ComponentID componentID = GetComponentID<T>();

      return m_Entities[entity].mask.test(componentID);
    }

    const std::vector<EntityID>& GetEntitiesWithComponents(const ComponentMask mask) const;
    const std::vector<EntityID>& GetEntitiesWithAnyOfComponents(const ComponentMask mask) const;

    template<typename T>
    T* RegisterSystem(const uint32_t priority = 0)
    {
      static_assert(std::is_base_of<System, T>::value, "T must inherit from System!");

      auto system = std::make_shared<T>();

      auto it = std::lower_bound(m_SortedSystems.begin(), m_SortedSystems.end(), priority,
                                 [](const auto& a, const uint32_t p) { return a.priority > p; });

      m_SortedSystems.insert(it, SystemEntry{ priority, system });
      m_SystemMap[std::type_index(typeid(T))] = system;

      return system.get();
    }

    template<typename T>
    T* GetSystem()
    {
      auto it = m_SystemMap.find(std::type_index(typeid(T)));
      if (it != m_SystemMap.end()) {
        return std::static_pointer_cast<T>(it->second).get();
      }
      CORE_ASSERT(false, "System not registered!");
      return nullptr;
    }

    template<typename T>
    void RemoveSystem()
    {
      auto typeIndex = std::type_index(typeid(T));
      auto mapIt = m_SystemMap.find(typeIndex);
      if (mapIt != m_SystemMap.end()) {
        auto vectorIt = std::find_if(m_SortedSystems.begin(), m_SortedSystems.end(),
          [&](const auto& entry) { return entry.system == mapIt->second; });

        if (vectorIt != m_SortedSystems.end()) {
          m_SortedSystems.erase(vectorIt);
        }
        m_SystemMap.erase(mapIt);
      }
    }

    void OnUpdateSystems(const Timestep deltaTime);
    void OnEventSystems(Event& event);

    ECSManager& operator=(const ECSManager&);
    ECSManager& operator=(ECSManager&&) noexcept;

  private:
    std::vector<Entity> m_Entities;
    std::vector<EntityID> m_FreeEntities;

    std::array<std::vector<std::byte>, MAX_COMPONENTS> m_ComponentArrays;
    std::array<uint64_t, MAX_COMPONENTS> m_ComponentSizes;

    uint32_t m_ComponentCounter = 0;

    struct SystemEntry
    {
      uint32_t priority;
      std::shared_ptr<System> system;
    };

    std::vector<SystemEntry> m_SortedSystems;
    std::unordered_map<std::type_index, std::shared_ptr<System>> m_SystemMap;
  };
}