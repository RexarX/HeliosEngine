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

  void Entity::SetParent(Entity& parent)
  {
    if (this == &parent) { CORE_ASSERT(false, "Entity cannot be its own parent!"); return; }
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (parent.m_Scene == nullptr) { CORE_ASSERT(false, "Parent scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (!m_Scene->m_Registry.valid(parent.m_Entity)) { CORE_ASSERT(false, "Parent entity is not valid!"); return; }

    m_Scene->m_Registry.get<Relationship>(m_Entity).parent = &parent;
    m_Scene->m_Registry.get<Relationship>(parent.m_Entity).children.push_back(this);
  }

  void Entity::RemoveParent() const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }

    m_Scene->m_Registry.get<Relationship>(m_Entity).parent = nullptr;
  }

  void Entity::AddChild(Entity& child)
  {
    if (this == &child) { CORE_ASSERT(false, "Entity cannot be its own child!"); return; }
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (child.m_Scene == nullptr) { CORE_ASSERT(false, "Child scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (!m_Scene->m_Registry.valid(child.m_Entity)) { CORE_ASSERT(false, "Child entity is not valid!"); return; }

    m_Scene->m_Registry.get<Relationship>(m_Entity).children.push_back(&child);
    m_Scene->m_Registry.get<Relationship>(child.m_Entity).parent = this;
  }

  Entity& Entity::GetParent() const
  {
    CORE_ASSERT(m_Scene != nullptr, "Scene is null!");
    CORE_ASSERT(m_Scene->m_Registry.valid(m_Entity), "Entity is not valid!");
    Entity* parent = m_Scene->m_Registry.get<Relationship>(m_Entity).parent;
    CORE_ASSERT(parent != nullptr, "Entity has no parent!");

    return *parent;
  }

  const std::vector<Entity*>& Entity::GetChildren() const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); }

    return m_Scene->m_Registry.get<Relationship>(m_Entity).children;
  }

  inline bool Entity::IsValid() const {
    return m_Scene != nullptr && m_Entity != entt::null && m_Scene->m_Registry.valid(m_Entity);
  }
}