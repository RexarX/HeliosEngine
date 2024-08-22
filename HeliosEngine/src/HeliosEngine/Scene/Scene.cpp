#include "Scene.h"

namespace Helios
{
  Scene::Scene()
    : m_RootEntity(CreateEntity("Root"))
  {
    m_Registry.get<Relationship>(m_RootEntity.GetEntity()).isRoot = true;
  }

  Scene::Scene(const std::string& name)
    : m_Name(name), m_RootEntity(CreateEntity(name))
  {
    m_Registry.get<Relationship>(m_RootEntity.GetEntity()).isRoot = true;
  }

  Scene::Scene(Scene&& other) noexcept
    : m_Name(std::move(other.m_Name)), m_Active(other.m_Active),
    m_Registry(std::move(other.m_Registry)), m_EntityMap(std::move(other.m_EntityMap)),
    m_RootEntity(std::move(other.m_RootEntity)), m_RenderingSystem(std::move(other.m_RenderingSystem)),
    m_ScriptSystem(std::move(other.m_ScriptSystem))
  {
    other.m_Active = false;
    other.m_Name.clear();
    other.m_Registry.clear();
    other.m_EntityMap.clear();
    other.m_RootEntity = Entity();
  }

  void Scene::OnUpdate(Timestep deltaTime)
  {
    m_EventSystem.OnUpdate(m_Registry);
    m_ScriptSystem.OnUpdate(m_Registry, deltaTime);
    m_CameraSystem.OnUpdate(m_Registry);
  }

  void Scene::OnEvent(Event& event)
  {
    m_ScriptSystem.OnEvent(m_Registry, event);
    m_CameraSystem.OnEvent(m_Registry, event);
  }

  void Scene::Draw()
  {
    m_RenderingSystem.OnUpdate(m_Registry);
  }
  
  void Scene::Load()
  {
  }

  void Scene::Unload()
  {
  }

  Entity Scene::CreateEntity(const std::string& name)
  {
    entt::entity entity = m_Registry.create();
    UUID id;

    m_Registry.emplace<ID>(entity, id);
    m_Registry.emplace<Tag>(entity, name.empty() ? "Entity" : name);
    m_Registry.emplace<Relationship>(entity);
    m_EntityMap.insert(std::make_pair(id, entity));

    return Entity(entity, this);
  }

  void Scene::DestroyEntity(Entity& entity)
  {
    entt::entity entt = entity.GetEntity();
    if (!m_Registry.valid(entt)) { CORE_ASSERT(false, "Entity does not exist!"); return; }
    auto& relationship = m_Registry.get<Relationship>(entt);
    std::vector<entt::entity>& children = relationship.children;
    for (entt::entity child : children) {
      DestroyEntityRecursively(child);
    }
    relationship.parent = entt::null;
    children.clear();
    m_EntityMap.erase(m_Registry.get<ID>(entt).id);
    m_Registry.destroy(entt);
  }

  Entity Scene::FindEntityByUUID(UUID uuid)
  {
    if (!m_EntityMap.contains(uuid)) { CORE_ASSERT(false, "Entity does not exist!"); return Entity(); }
    return Entity(m_EntityMap[uuid], this);
  }

  Entity Scene::GetActiveCameraEntity()
  {
    entt::entity cameraEntity = entt::null;

    for (entt::entity entity : m_Registry.view<Camera>()) {
      if (m_Registry.get<Camera>(entity).currect) { cameraEntity = entity; break; }
    }

    if (cameraEntity == entt::null) { CORE_ASSERT(false, "No active camera found!"); return Entity(); }

    return Entity(cameraEntity, this);
  }

  Scene& Scene::operator=(Scene&& other) noexcept
  {
    if (this != &other) {
      m_Name = std::move(other.m_Name);
      m_Active = other.m_Active;
      m_Registry = std::move(other.m_Registry);
      m_EntityMap = std::move(other.m_EntityMap);
      m_RootEntity = std::move(other.m_RootEntity);
      m_RenderingSystem = std::move(other.m_RenderingSystem);
      m_ScriptSystem = std::move(other.m_ScriptSystem);

      other.m_Name.clear();
      other.m_Active = false;
      other.m_Registry.clear();
      other.m_EntityMap.clear();
      other.m_RootEntity = Entity();
    }

    return *this;
  }

  void Scene::DestroyEntityRecursively(entt::entity entity)
  {
    if (!m_Registry.valid(entity)) { CORE_ASSERT(false, "Entity does not exist!"); return; }
    auto& relationship = m_Registry.get<Relationship>(entity);
    std::vector<entt::entity>& children = relationship.children;
    for (entt::entity child : children) {
      DestroyEntityRecursively(entity);
    }
    relationship.parent = entt::null;
    children.clear();
    m_EntityMap.erase(m_Registry.get<ID>(entity).id);
    m_Registry.destroy(entity);
  }
}