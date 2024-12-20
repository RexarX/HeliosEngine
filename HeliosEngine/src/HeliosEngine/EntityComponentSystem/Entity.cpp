#include "Entity.inl"

namespace Helios {
  Entity::Entity(entt::entity entity, Scene* scene)
    : m_Entity(entity), m_Scene(scene) {
  }

  void Entity::Destroy() {
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to destroy entity: Scene is null!");
      return;
    }

    m_Scene->DestroyEntity(*this);
    m_Entity = entt::null;
    m_Scene = nullptr;
  }

  void Entity::SetParent(Entity& parent) {
    if (this == &parent) {
      CORE_ASSERT(false, "Failed to set entity's parent: Entity cannot be its own parent!");
      return;
    }
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to set entity's parent: Scene is null!");
      return;
    }
    if (parent.m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to set entity's parent: Parent's scene is null!");
      return;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to set entity's parent: Entity is not valid!");
      return;
    }
    if (parent.m_Entity == entt::null || !m_Scene->m_Registry.valid(parent.m_Entity)) {
      CORE_ASSERT(false, "Failed to set entity's parent: Parent entity is not valid!");
      return;
    }

    m_Scene->m_Registry.get<Relationship>(m_Entity).parent = &parent;
    m_Scene->m_Registry.get<Relationship>(parent.m_Entity).children.push_back(this);
  }

  void Entity::RemoveParent() {
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to remove entity's parent: Scene is null!");
      return;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to remove entity's parent: Entity is not valid!");
      return;
    }

    Relationship& relationship = m_Scene->m_Registry.get<Relationship>(m_Entity);
    relationship.parent->RemoveChild(*this);
    relationship.parent = nullptr;
  }

  void Entity::AddChild(Entity& child) {
    if (this == &child) {
      CORE_ASSERT(false, "Failed to add entity's child: Entity cannot be its own child!");
      return;
    }
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to add entity's child: Scene is null!");
      return;
    }
    if (child.m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to add entity's child: Child's scene is null!");
      return;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to add entity's child: Entity is not valid!");
      return;
    }
    if (child.m_Entity == entt::null || !m_Scene->m_Registry.valid(child.m_Entity)) {
      CORE_ASSERT(false, "Failed to add entity's child: Child entity is not valid!");
      return;
    }

    m_Scene->m_Registry.get<Relationship>(m_Entity).children.push_back(&child);
    m_Scene->m_Registry.get<Relationship>(child.m_Entity).parent = this;
  }

  void Entity::RemoveChild(Entity& child) const {
    if (this == &child) {
      CORE_ASSERT(false, "Failed to remove entity's child: Entity cannot be its own child!");
      return;
    }
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to remove entity's child: Scene is null!");
      return;
    }
    if (child.m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to remove entity's child: Child's scene is null!");
      return;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to remove entity's child: Entity is not valid!");
      return;
    }
    if (child.m_Entity == entt::null || !m_Scene->m_Registry.valid(child.m_Entity)) {
      CORE_ASSERT(false, "Failed to remove entity's child: Child entity is not valid!");
      return;
    }

    Relationship& releationship = m_Scene->m_Registry.get<Relationship>(m_Entity);
    
    auto it = std::find(releationship.children.begin(), releationship.children.end(), &child);
    releationship.children.erase(it);

    m_Scene->m_Registry.get<Relationship>(child.m_Entity).parent = nullptr;
  }

  Entity& Entity::GetParent() const {
    if (m_Scene == nullptr) {
      CORE_ASSERT(false, "Failed to get entity's parent: Scene is null!");
      return m_Scene->m_InvalidEntity;
    }
    if (m_Entity == entt::null || !m_Scene->m_Registry.valid(m_Entity)) {
      CORE_ASSERT(false, "Failed to get entity's parent: Entity is not valid!");
      return m_Scene->m_InvalidEntity;
    }

    Entity* parent = m_Scene->m_Registry.get<Relationship>(m_Entity).parent;
    if (parent == nullptr) {
      CORE_ASSERT(false, "Failed to get entity's parent: Entity has no parent!");
      return m_Scene->m_InvalidEntity;
    }

    return *parent;
  }

  const std::vector<Entity*>& Entity::GetChildren() const {
    CORE_ASSERT(m_Scene != nullptr, "Failed to get entity's children: Scene is null!");
    CORE_ASSERT(m_Entity != entt::null && m_Scene->m_Registry.valid(m_Entity),
                "Failed to get entity's parent: Entity is not valid!");

    return m_Scene->m_Registry.get<Relationship>(m_Entity).children;
  }

  bool Entity::IsValid() const {
    if (m_Scene != nullptr && m_Entity != entt::null) {
      return m_Scene->m_Registry.valid(m_Entity);
    }
    return false;
  }
}