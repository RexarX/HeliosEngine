#pragma once

#include "Entity.h"

#include "Scene/Scene.h"

namespace Helios {
  template <typename T, typename ...Args>
  T& Entity::EmplaceComponent(Args&&... args) const {
    CORE_ASSERT(m_Scene != nullptr, "Failed to emplace entity '{}' component: Scene is null!", typeid(T).name());
    CORE_ASSERT(m_Entity != entt::null && m_Scene->m_Registry.valid(m_Entity),
                "Failed to emplace entity '{}' component: Entity is not valid!", typeid(T).name());

    return m_Scene->m_Registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
  }

  template <typename T>
  void Entity::RemoveComponent() const {
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to remove entity's '{}' component: Scene is null!", typeid(T).name());
      return;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to remove entity's '{}' component: Entity is not valid!", typeid(T).name());
      return;
    }
    if (m_Scene->m_Registry.try_get<T>(m_Entity) == nullptr) {
      CORE_ASSERT(false, "Failed to remove entity's '{}' component: Entity does not have component!",
                  typeid(T).name());
      return;
    }

    m_Scene->m_Registry.remove<T>(m_Entity);
  }

  template <typename T>
  bool Entity::HasComponent() const {
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to check if entity has '{}' component: Scene is null!", typeid(T).name());
      return false;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to check if entity has '{}' component: Entity is null!", typeid(T).name());
      return false;
    }

    return m_Scene->m_Registry.try_get<T>(m_Entity) != nullptr;
  }
  
  template <typename T>
  T& Entity::GetComponent() const {
    CORE_ASSERT(m_Scene != nullptr, "Failed to get entity's '{}' component: Scene is null!", typeid(T).name());
    CORE_ASSERT(m_Entity != entt::null && m_Scene->m_Registry.valid(m_Entity),
                "Failed to get entity's '{}' component: Entity is not valid!", typeid(T).name());
    T* ptr = m_Scene->m_Registry.try_get<T>(m_Entity);
    CORE_ASSERT(ptr != nullptr, "Failed to get entity's '{}' component: Entity does not have component!",
                typeid(T).name());

    return *ptr;
  }

  template <typename T, typename... Args>
  void Entity::EmplaceScriptComponent(Args&&... args) const {
    static_assert(std::is_base_of_v<Scriptable, T>,
                  "Failed to emplace script component: T must inherit from Scriptable!");
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to emplace '{}' script component: Scene is null!", typeid(T).name());
      return;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to emplace '{}' script component: Entity is not valid!", typeid(T).name());
      return;
    }

    auto& script = m_Scene->m_Registry.emplace<Script>(m_Entity, std::make_unique<T>(std::forward<Args>(args)...));
    script.scriptable->m_EventSystem = &m_Scene->m_EventSystem;
    script.scriptable->m_Entity = this;
    script.scriptable->OnAttach();
  }
}