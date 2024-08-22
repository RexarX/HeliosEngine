#pragma once

#include "Entity.h"

#include "Scene/Scene.h"

namespace Helios
{
  template <typename T, typename ...Args>
  T& Entity::EmplaceComponent(Args&&... args) const
  {
    CORE_ASSERT(m_Scene != nullptr, "Scene is null!");
    CORE_ASSERT(m_Scene->m_Registry.valid(m_Entity), "Entity is not valid!");

    return m_Scene->m_Registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
  }

  template <typename T>
  void Entity::RemoveComponent() const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (m_Scene->m_Registry.try_get<T>(m_Entity) == nullptr) { CORE_ASSERT(false, "Entity does not have component!"); return; }

    m_Scene->m_Registry.remove<T>(m_Entity);
  }

  template <typename T>
  bool Entity::HasComponent() const
  {
    CORE_ASSERT(m_Scene != nullptr, "Scene is null!");
    CORE_ASSERT(m_Scene->m_Registry.valid(m_Entity), "Entity is not valid!");

    return m_Scene->m_Registry.try_get<T>(m_Entity) != nullptr;
  }

  template <typename T>
  T& Entity::GetComponent() const
  {
    CORE_ASSERT(m_Scene != nullptr, "Scene is null!");
    CORE_ASSERT(m_Scene->m_Registry.valid(m_Entity), "Entity is not valid!");
    T* ptr = m_Scene->m_Registry.try_get<T>(m_Entity);
    CORE_ASSERT(ptr != nullptr, "Entity does not have component!");

    return *ptr;
  }

  template <typename T, typename... Args>
  void Entity::EmplaceScript(Args&&... args) const
  {
    static_assert(std::is_base_of<Script, T>::value, "T must inherit from Script!");
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (m_Scene->m_Registry.try_get<T>(m_Entity) == nullptr) { CORE_ASSERT(false, "Entity does not have component!"); return; }

    auto& script = m_Scene->m_Registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
    script.m_EventSystem = &m_Scene->m_EventSystem;
    script.OnAttach();
  }

  template <typename T>
  void Entity::DetachScript() const
  {
    static_assert(std::is_base_of<Script, T>::value, "T must inherit from Script!");
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    T* ptr = m_Scene->m_Registry.try_get<T>(m_Entity);
    if (ptr == nullptr) { CORE_ASSERT(false, "Entity does not have script!"); return; }

    m_Scene->m_Registry.remove<T>(m_Entity);
  }
}