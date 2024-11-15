#include "Scene.h"

namespace Helios {
  Scene::Scene()
    : m_RootEntity(CreateEntity("Root")) {
    m_InvalidEntity = Entity();
  }

  Scene::Scene(std::string_view name)
    : m_Name(name), m_RootEntity(CreateEntity("Root")) {
    m_InvalidEntity = Entity();
  }

  Scene::Scene(Scene&& other) noexcept
    : m_Name(std::move(other.m_Name)), m_Active(other.m_Active), m_Loaded(other.m_Loaded),
    m_Registry(std::move(other.m_Registry)), m_EntityMap(std::move(other.m_EntityMap)),
    m_RootEntity(std::move(other.m_RootEntity)), m_EventSystem(std::move(other.m_EventSystem)),
    m_ScriptSystem(std::move(other.m_ScriptSystem)), m_CameraSystem(std::move(other.m_CameraSystem)),
    m_RenderingSystem(std::move(other.m_RenderingSystem)) {
    m_InvalidEntity = Entity();

    other.m_Active = false;
    other.m_Loaded = false;
    other.m_Name.clear();
    other.m_Registry.clear();
    other.m_EntityMap.clear();
    other.m_RootEntity = Entity();
  }

  void Scene::OnUpdate(Timestep deltaTime) {
    if (!m_Active) { return; }
    if (!m_Loaded) { CORE_ASSERT(false, "Scene is not loaded!"); return; }

    m_EventSystem.OnUpdate();
    m_ScriptSystem.OnUpdate(m_Registry, deltaTime);
    m_CameraSystem.OnUpdate(m_Registry);
  }

  void Scene::OnEvent(Event& event) {
    if (!m_Active) { return; }
    if (!m_Loaded) { CORE_ASSERT(false, "Scene is not loaded!"); return; }

    m_ScriptSystem.OnEvent(m_Registry, event);
    m_CameraSystem.OnEvent(m_Registry, event);
  }

  void Scene::Draw() {
    if (!m_Active) { return; }
    if (!m_Loaded) { CORE_ASSERT(false, "Scene is not loaded!"); return; }

    m_RenderingSystem.OnUpdate(m_Registry);
  }
  
  void Scene::Load() {
    if (m_Loaded) { return; }

    const auto& view = m_Registry.view<Renderable>();
    m_RenderingSystem.GetResourceManager().InitializeResources(m_Registry, { view.begin(), view.end() });

    m_Loaded = true;
  }

  void Scene::Unload() {
    if (!m_Loaded) { return; }

    const auto& view = m_Registry.view<Renderable>();
    m_RenderingSystem.GetResourceManager().FreeResources(m_Registry, { view.begin(), view.end() });

    m_Active = false;
    m_Loaded = false;
  }

  Entity& Scene::CreateEntity(std::string_view name) {
    entt::entity entity = m_Registry.create();
    UUID id;

    m_Registry.emplace<ID>(entity, id);
    m_Registry.emplace<Tag>(entity, name.empty() ? "Entity" : name.data());
    m_Registry.emplace<Relationship>(entity);

    return m_EntityMap.emplace(id, Entity(entity, this)).first->second;
  }

  void Scene::DestroyEntity(Entity& entity) {
    if (!entity.IsValid()) { CORE_ASSERT(false, "Invalid entity!"); return; }
    entt::entity entt = entity.GetEntity();
    auto& relationship = m_Registry.get<Relationship>(entt);
    for (Entity* child : relationship.children) {
      DestroyEntity(*child);
    }
    
    relationship.parent = nullptr;
    relationship.children.clear();
    m_EntityMap.erase(m_Registry.get<ID>(entt).id);
    m_Registry.destroy(entt);
  }

  Entity& Scene::FindEntityByUUID(UUID uuid) {
    if (!m_EntityMap.contains(uuid)) {
      CORE_ASSERT(false, "Entity does not exist!");
      return m_InvalidEntity;
    }

    return m_EntityMap[uuid];
  }

  Entity& Scene::GetActiveCameraEntity() {
    for (entt::entity entity : m_Registry.view<Camera>()) {
      if (m_Registry.get<Camera>(entity).currect) {
        return m_EntityMap[m_Registry.get<ID>(entity).id];
      }
    }

    CORE_ASSERT(false, "No active camera found!");
    return m_InvalidEntity;
  }

  Scene& Scene::operator=(Scene&& other) noexcept {
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