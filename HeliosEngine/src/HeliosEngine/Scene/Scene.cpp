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
    : m_Name(std::move(other.m_Name)), m_Active(other.m_Active), m_Loaded(other.m_Loaded),
    m_Registry(std::move(other.m_Registry)), m_EntityMap(std::move(other.m_EntityMap)),
    m_RootEntity(std::move(other.m_RootEntity)), m_EventSystem(std::move(other.m_EventSystem)),
    m_CameraSystem(std::move(other.m_CameraSystem)), m_ScriptSystem(std::move(other.m_ScriptSystem)),
    m_RenderingSystem(std::move(other.m_RenderingSystem))
  {
    other.m_Active = false;
    other.m_Loaded = false;
    other.m_Name.clear();
    other.m_Registry.clear();
    other.m_EntityMap.clear();
    other.m_RootEntity = Entity();
  }

  void Scene::OnUpdate(Timestep deltaTime)
  {
    if (!m_Active) { return; }
    if (!m_Loaded) {CORE_ASSERT(false, "Scene is not loaded!"); return;}

    m_EventSystem.OnUpdate(m_Registry);
    m_ScriptSystem.OnUpdate(m_Registry, deltaTime);
    m_CameraSystem.OnUpdate(m_Registry);
  }

  void Scene::OnEvent(Event& event)
  {
    if (!m_Active) { return; }
    if (!m_Loaded) { CORE_ASSERT(false, "Scene is not loaded!"); return; }

    m_ScriptSystem.OnEvent(m_Registry, event);
    m_CameraSystem.OnEvent(m_Registry, event);
  }

  void Scene::Draw()
  {
    if (!m_Active) { return; }
    if (!m_Loaded) { CORE_ASSERT(false, "Scene is not loaded!"); return; }

    m_RenderingSystem.OnUpdate(m_Registry);
  }
  
  void Scene::Load()
  {
    if (m_Loaded) { return; }

    // Load

    m_Loaded = true;
  }

  void Scene::Unload()
  {
    if (!m_Loaded) { return; }

    // Unload

    m_Active = false;
    m_Loaded = false;
  }

  Entity& Scene::CreateEntity(const std::string& name)
  {
    entt::entity entity = m_Registry.create();
    UUID id;

    m_Registry.emplace<ID>(entity, id);
    m_Registry.emplace<Tag>(entity, name.empty() ? "Entity" : name);
    m_Registry.emplace<Relationship>(entity);

    return m_EntityMap.emplace(id, Entity(entity, this)).first->second;
  }

  void Scene::DestroyEntity(Entity& entity)
  {
    if (!entity.IsValid()) { CORE_ASSERT(false, "Invalid entity!"); return; }
    entt::entity entt = entity.GetEntity();
    auto& relationship = m_Registry.get<Relationship>(entt);
    for (Entity& child : relationship.children) {
      DestroyEntity(child);
    }
    relationship.parent = Entity();
    relationship.children.clear();
    m_EntityMap.erase(m_Registry.get<ID>(entt).id);
    m_Registry.destroy(entt);
  }

  Entity& Scene::FindEntityByUUID(UUID uuid)
  {
    CORE_ASSERT(m_EntityMap.contains(uuid), "Entity does not exist!");
    return m_EntityMap.at(uuid);
  }

  Entity& Scene::GetActiveCameraEntity()
  {
    for (entt::entity entity : m_Registry.view<Camera>()) {
      if (m_Registry.get<Camera>(entity).currect) {
        return m_EntityMap.at(m_Registry.get<ID>(entity).id);
      }
    }

    CORE_ASSERT(false, "No active camera found!");
  }

  Scene& Scene::operator=(Scene&& other) noexcept
  {
    if (this != &other) {
      m_Name = std::move(other.m_Name);
      m_Active = other.m_Active;
      m_Loaded = other.m_Loaded;
      m_Registry = std::move(other.m_Registry);
      m_EntityMap = std::move(other.m_EntityMap);
      m_RootEntity = std::move(other.m_RootEntity);
      m_EventSystem = std::move(other.m_EventSystem);
      m_CameraSystem = std::move(other.m_CameraSystem);
      m_ScriptSystem = std::move(other.m_ScriptSystem);
      m_RenderingSystem = std::move(other.m_RenderingSystem);

      other.m_Name.clear();
      other.m_Active = false;
      other.m_Loaded = false;
      other.m_Registry.clear();
      other.m_EntityMap.clear();
      other.m_RootEntity = Entity();
    }

    return *this;
  }
}