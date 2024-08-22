#include "Entity.inl"

namespace Helios
{
  Entity::Entity(entt::entity entity, Scene* scene)
    : m_Entity(entity), m_Scene(scene)
  {
  }

  void Entity::Destroy()
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    m_Scene->DestroyEntity(*this);
    m_Entity = entt::null;
    m_Scene = nullptr;
  }

  void Entity::SetParent(Entity& parent) const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (this == &parent) { CORE_ASSERT(false, "Entity cannot be its own parent!"); return; }
    entt::entity entity = parent.GetEntity();
    if (!m_Scene->m_Registry.valid(entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }

    m_Scene->m_Registry.get<Relationship>(m_Entity).parent = entity;
    m_Scene->m_Registry.get<Relationship>(entity).children.push_back(m_Entity);
  }

  void Entity::RemoveParent() const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }

    m_Scene->m_Registry.get<Relationship>(m_Entity).parent = entt::null;
  }

  void Entity::AddChild(Entity& child) const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    entt::entity entity = child.GetEntity();
    if (!m_Scene->m_Registry.valid(entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (this == &child) { CORE_ASSERT(false, "Entity cannot be its own child!"); return; }

    m_Scene->m_Registry.get<Relationship>(m_Entity).children.push_back(entity);
    m_Scene->m_Registry.get<Relationship>(entity).parent = m_Entity;
  }

  Entity Entity::GetParent() const
  {
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return Entity(); }

    return Entity(m_Scene->m_Registry.get<Relationship>(m_Entity).parent, m_Scene);
  }

  const std::vector<Entity>& Entity::GetChildren() const
  {
    static std::vector<Entity> children;

    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Scene is null!");
      children.clear();
      return children;
    }

    if (!m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Entity is not valid!");
      children.clear();
      return children;
    }
    
    for (entt::entity child : m_Scene->m_Registry.get<Relationship>(m_Entity).children) {
      children.push_back(Entity(child, m_Scene));
    }

    return children;
  }

  inline bool Entity::IsValid() const {
    return m_Scene != nullptr && m_Entity != entt::null && m_Scene->m_Registry.valid(m_Entity);
  }
}