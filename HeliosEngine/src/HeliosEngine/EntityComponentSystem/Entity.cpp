#include "Entity.inl"
#include "Entity.h"

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

  void Entity::RemoveParent()
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }

    Relationship& relationship = m_Scene->m_Registry.get<Relationship>(m_Entity);
    relationship.parent->RemoveChild(*this);
    relationship.parent = nullptr;
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

  void Entity::RemoveChild(Entity& child) const
  {
    if (this == &child) { CORE_ASSERT(false, "Entity cannot be its own child!"); return; }
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return; }
    if (child.m_Scene == nullptr) { CORE_ASSERT(false, "Child scene is null!"); return; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return; }
    if (!m_Scene->m_Registry.valid(child.m_Entity)) { CORE_ASSERT(false, "Child entity is not valid!"); return; }

    Relationship& releationship = m_Scene->m_Registry.get<Relationship>(m_Entity);
    
    auto it = std::find(releationship.children.begin(), releationship.children.end(), &child);
    releationship.children.erase(it);

    m_Scene->m_Registry.get<Relationship>(child.m_Entity).parent = nullptr;
  }

  Entity& Entity::GetParent() const
  {
    if (m_Scene == nullptr) { CORE_ASSERT(false, "Scene is null!"); return m_Scene->m_EntityMap[0]; }
    if (!m_Scene->m_Registry.valid(m_Entity)) { CORE_ASSERT(false, "Entity is not valid!"); return m_Scene->m_EntityMap[0]; }
    Entity* parent = m_Scene->m_Registry.get<Relationship>(m_Entity).parent;
    if (parent == nullptr) { CORE_ASSERT(false, "Entity has no parent!"); return m_Scene->m_EntityMap[0]; }

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