#pragma once 

#include "pch.h"

#include <entt/entt.hpp>

namespace Helios {
  class Scene;

  class HELIOSENGINE_API Entity {
  public:
    Entity() = default;
    Entity(const Entity&) = default;
    ~Entity() = default;

    void Destroy();

    template <typename T, typename... Args>
    T& EmplaceComponent(Args&&... args) const;

    template <typename T>
    void RemoveComponent() const;

    template <typename T>
    bool HasComponent() const;

    template <typename T>
    T& GetComponent() const;

    template <typename T, typename... Args>
    void EmplaceScriptComponent(Args&&... args) const;

    void SetParent(Entity& parent);
    void RemoveParent();

    void AddChild(Entity& child);
    void RemoveChild(Entity& child) const;

    Entity& GetParent() const;
    const std::vector<Entity*>& GetChildren() const;

    bool IsValid() const;

    inline entt::entity GetEntity() const { return m_Entity; }

    Entity& operator=(const Entity&) = default;

  private:
    Entity(entt::entity entity, Scene* scene);
    Entity(Entity&&) noexcept = default;

    Entity& operator=(Entity&&) noexcept = default;

  private:
    entt::entity m_Entity = entt::null;
    Scene* m_Scene = nullptr;

    friend class Scene;
  };
}