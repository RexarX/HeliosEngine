#pragma once

#include "EntityComponentSystem/Components.h"
#include "EntityComponentSystem/Entity.h"

#include "EntityComponentSystem/Systems/RenderingSystem.h"
#include "EntityComponentSystem/Systems/CameraSystem.h"
#include "EntityComponentSystem/Systems/EventSystem.h"
#include "EntityComponentSystem/Systems/ScriptSystem.h"

namespace std
{
  template <typename T> struct hash;

  template<>
  struct hash<Helios::UUID>
  {
    inline std::size_t operator()(const Helios::UUID& uuid) const {
      return (uint64_t)uuid;
    }
  };
}

namespace Helios
{
  class HELIOSENGINE_API Scene
  {
  public:
    Scene();
    Scene(std::string_view name);
    Scene(const Scene&) = delete;
    Scene(Scene&&) noexcept;
    ~Scene() = default;

    void OnUpdate(Timestep deltaTime);
    void OnEvent(const Event& event);
    void Draw();

    void Load();
    void Unload();

    void SetName(std::string_view name) { m_Name = name; }
    void SetActive(bool active) { m_Active = active; }

    Entity& CreateEntity(std::string_view name);
    void DestroyEntity(Entity& entity);

    Entity& FindEntityByUUID(UUID uuid);

    Entity& GetActiveCameraEntity();

    template <typename T> requires std::is_base_of<Event, T>::value
    void PushEvent(T& event) { m_EventSystem.PushEvent(event); }

    inline const std::string& GetName() const { return m_Name; }
    inline bool IsActive() const { return m_Active; }

    inline Entity& GetRootEntity() { return m_RootEntity; }
    inline const Entity& GetRootEntity() const { return m_RootEntity; }

    Scene& operator=(const Scene&) = delete;
    Scene& operator=(Scene&&) noexcept;

    friend class Entity;

  private:
    std::string m_Name = "default";
    bool m_Active = false;
    bool m_Loaded = false;

    entt::registry m_Registry;
    std::unordered_map<UUID, Entity> m_EntityMap;

    Entity m_RootEntity;
    
    EventSystem m_EventSystem;
    ScriptSystem m_ScriptSystem;
    CameraSystem m_CameraSystem;
    RenderingSystem m_RenderingSystem;
  };
}