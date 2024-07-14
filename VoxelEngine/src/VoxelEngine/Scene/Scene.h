#pragma once

#include "EntityComponentSystem/Manager/ECSManager.h"

namespace Engine
{
  class VOXELENGINE_API Scene
  {
  public:
    Scene();
    Scene(const std::string& name);
    ~Scene() = default;

    void SetName(const std::string& name) { m_Name = name; }

    void SetActive(const bool active) { m_Active = active; }

    inline const EntityID CreateEntity() { return m_ECSManager.CreateEntity(); }
    void DestroyEntity(const EntityID entity) { m_ECSManager.DestroyEntity(entity); }

    template<typename T>
    inline const ComponentID RegisterComponent() { return m_ECSManager.RegisterComponent<T>(); }

    template<typename T>
    void AddComponent(const EntityID entity, const T& component) {
      m_ECSManager.AddComponent<T>(entity, component);
    }

    template<typename T>
    void RemoveComponent(const EntityID entity) {
      m_ECSManager.RemoveComponent<T>(entity);
    }

    template<typename T>
    inline T* GetComponent(const EntityID entity) {
      return m_ECSManager.GetComponent<T>(entity);
    }

    template<typename T>
    const bool HasComponent(const EntityID entity) const { return m_ECSManager.HasComponent<T>(entity); }

    const std::vector<EntityID>& GetEntitiesWithComponents(const ComponentMask mask) const {
      return m_ECSManager.GetEntitiesWithComponents(mask);
    }

    template<typename T>
    inline T* RegisterSystem() {
      return m_ECSManager.RegisterSystem<T>();
    }

    template<typename T>
    inline T* GetSystem() { return m_ECSManager.GetSystem<T>(); }

    template<typename T>
    void RemoveSystem() { m_ECSManager.RemoveSystem<T>(); }

    void OnUpdate(const Timestep deltaTime) { m_ECSManager.OnUpdateSystems(deltaTime); }
    void OnEvent(Event& event) { m_ECSManager.OnEventSystems(event); }

    inline ECSManager& GetECSManager() { return m_ECSManager; }
    inline const ECSManager& GetECSManager() const { return m_ECSManager; }

    inline const std::string& GetName() const { return m_Name; }

    inline const bool IsActive() const { return m_Active; }

  private:
    std::string m_Name;

    bool m_Active = false;

    ECSManager m_ECSManager;
  };
}