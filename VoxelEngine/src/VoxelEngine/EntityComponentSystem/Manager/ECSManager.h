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
    T* RegisterSystem()
    {
      static_assert(std::is_base_of<System, T>::value, "T must inherit from System!");

      m_Systems[std::type_index(typeid(T))] = std::make_unique<T>();

      return m_Systems[std::type_index(typeid(T))].get();
    }

    template<typename T>
    T* GetSystem()
    {
      auto it = m_Systems.find(std::type_index(typeid(T)));

      CORE_ASSERT(it != m_Systems.end(), "System not registered!");

      return static_cast<T*>(it->second.get());
    }

    template<typename T>
    void RemoveSystem() { m_Systems.erase(std::type_index(typeid(T))); }

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

    std::unordered_map<std::type_index, std::unique_ptr<System>> m_Systems;
  };
}