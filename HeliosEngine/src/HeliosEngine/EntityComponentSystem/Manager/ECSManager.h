#pragma once

#include "EntityComponentSystem/Systems/System.h"

#include "EntityComponentSystem/Components/Script.h"

namespace Helios
{
  using ComponentID = uint32_t;

  class HELIOSENGINE_API ComponentRegistry
  {
  public:
    ComponentRegistry() = default;
    ComponentRegistry(const ComponentRegistry&);
    ComponentRegistry(ComponentRegistry&&) noexcept;
    ~ComponentRegistry() = default;

    const ComponentID GetID(const std::type_info& type);

    inline const uint32_t GetComponentCount() const { return m_NextID; }

    ComponentRegistry& operator=(const ComponentRegistry&);
    ComponentRegistry& operator=(ComponentRegistry&&) noexcept;

  private:
    std::unordered_map<std::type_index, ComponentID> m_ComponentIDs;
    ComponentID m_NextID = 0;
  };

  class HELIOSENGINE_API ECSManager
  {
  public:
    ECSManager() = default;
    ECSManager(const ECSManager&);
    ECSManager(ECSManager&&) noexcept;
    ~ECSManager();

    void OnUpdateSystems(const Timestep deltaTime);
    void OnEventSystems(Event& event);

    const EntityID CreateEntity();
    void DestroyEntity(const EntityID entity);

    template<typename T>
    const ComponentID RegisterComponent();

    template<typename T>
    T& AddComponent(const EntityID entity, const T& component);

    template<typename T, typename... Args>
    T& EmplaceComponent(const EntityID entity, Args&&... args);

    template<typename T>
    void RemoveComponent(const EntityID entity);

    template<typename T>
    T& GetComponent(const EntityID entity);

    template<typename T>
    const ComponentID GetComponentID();

    template<typename T>
    const bool HasComponent(const EntityID entity) const;

    const std::vector<EntityID>& GetEntitiesWithComponents(const ComponentMask mask) const;
    const std::vector<EntityID>& GetEntitiesWithAnyOfComponents(const ComponentMask mask) const;

    template<typename T>
    T& RegisterSystem(const uint32_t priority = 0);

    template<typename T>
    T& GetSystem();

    template<typename T>
    void RemoveSystem();

    ECSManager& operator=(const ECSManager&);
    ECSManager& operator=(ECSManager&&) noexcept;

  private:
    std::vector<Entity> m_Entities;
    std::vector<EntityID> m_FreeEntities;

    ComponentRegistry m_ComponentRegistry;
    std::array<std::vector<std::byte>, MAX_COMPONENTS> m_ComponentArrays;
    std::array<uint64_t, MAX_COMPONENTS> m_ComponentSizes;
    std::array<std::function<void(void*)>, MAX_COMPONENTS> m_ComponentDestructors;

    struct SystemEntry
    {
      uint32_t priority = 0;
      std::unique_ptr<System> system = nullptr;
      std::type_index type = typeid(void);
    };

    std::vector<SystemEntry> m_SortedSystems;
  };

  template<typename T>
  const ComponentID ECSManager::RegisterComponent()
  {
    CORE_ASSERT(m_ComponentRegistry.GetComponentCount() < MAX_COMPONENTS, "Too many components!");

    ComponentID id = GetComponentID<T>();
    m_ComponentArrays[id].resize(MAX_ENTITIES * sizeof(T));
    m_ComponentSizes[id] = sizeof(T);
    m_ComponentDestructors[id] = [](void* ptr) { static_cast<T*>(ptr)->~T(); };
    return id;
  }

  template<typename T>
  T& ECSManager::AddComponent(const EntityID entity, const T& component)
  {
    CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

    ComponentID componentID = GetComponentID<T>();

    CORE_ASSERT(componentID < m_ComponentRegistry.GetComponentCount(), "Component does not exist!");

    m_Entities[entity].mask.set(componentID);
    T* pComponent = new (m_ComponentArrays[componentID].data() + entity * sizeof(T)) T(component);
    return *pComponent;
  }

  template<typename T, typename... Args>
  T& ECSManager::EmplaceComponent(const EntityID entity, Args&&... args)
  {
    CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

    ComponentID componentID = GetComponentID<T>();

    CORE_ASSERT(componentID < m_ComponentRegistry.GetComponentCount(), "Component does not exist!");

    m_Entities[entity].mask.set(componentID);
    T* pComponent = new (m_ComponentArrays[componentID].data() + entity * sizeof(T)) T(std::forward<Args>(args)...);
    return *pComponent;
  }

  template<typename T>
  void ECSManager::RemoveComponent(const EntityID entity)
  {
    CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

    ComponentID componentID = GetComponentID<T>();

    CORE_ASSERT(componentID < m_ComponentRegistry.GetComponentCount(), "Component does not exist!");

    m_ComponentDestructors[componentID](m_ComponentArrays[componentID].data() + entity * sizeof(T));
    std::memset(m_ComponentArrays[componentID].data() + entity * sizeof(T), 0, sizeof(T));
    m_Entities[entity].mask.reset(componentID);
  }

  template<typename T>
  T& ECSManager::GetComponent(const EntityID entity)
  {
    CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

    ComponentID componentID = GetComponentID<T>();

    CORE_ASSERT(componentID < m_ComponentRegistry.GetComponentCount(), "Component does not exist!");
    CORE_ASSERT(m_Entities[entity].mask.test(componentID), "Entity does not have component!");

    return *reinterpret_cast<T*>(m_ComponentArrays[componentID].data() + entity * sizeof(T));
  }

  template<typename T>
  const ComponentID ECSManager::GetComponentID()
  {
    return m_ComponentRegistry.GetID(typeid(T));
  }

  template<typename T>
  const bool ECSManager::HasComponent(const EntityID entity) const
  {
    CORE_ASSERT(entity < m_Entities.size(), "Entity does not exist!");

    ComponentID componentID = GetComponentID<T>();
    return m_Entities[entity].mask.test(componentID);
  }

  template<typename T>
  T& ECSManager::RegisterSystem(const uint32_t priority)
  {
    static_assert(std::is_base_of<System, T>::value, "T must inherit from System!");

    auto system = std::make_unique<T>();
    T* systemPtr = system.get();

    auto it = std::lower_bound(m_SortedSystems.begin(), m_SortedSystems.end(), priority,
                               [](const auto& a, const uint32_t p) { return a.priority > p; });

    SystemEntry entry;
    entry.priority = priority;
    entry.system = std::move(system);
    entry.type = std::type_index(typeid(T));

    m_SortedSystems.insert(it, std::move(entry));

    return *systemPtr;
  }

  template<typename T>
  T& ECSManager::GetSystem()
  {
    auto typeIndex = std::type_index(typeid(T));
    auto it = std::find_if(m_SortedSystems.begin(), m_SortedSystems.end(),
                           [typeIndex](const auto& entry) { return entry.type == typeIndex; });

    CORE_ASSERT(it != m_SortedSystems.end(), "System not registered!");

    return *static_cast<T*>(it->system.get());
  }

  template<typename T>
  void ECSManager::RemoveSystem()
  {
    auto typeIndex = std::type_index(typeid(T));
    auto it = std::find_if(m_SortedSystems.begin(), m_SortedSystems.end(),
                           [typeIndex](const auto& entry) { return entry.type == typeIndex; });

    if (it != m_SortedSystems.end()) {
      m_SortedSystems.erase(it);
    }
  }
}