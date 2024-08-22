#pragma once 

#include "pch.h"

#include <entt/entt.hpp>

namespace Helios
{
  class Scene;

  class HELIOSENGINE_API Entity
  {
  public:
    Entity() = default;
    Entity(entt::entity entity, Scene* scene);
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
    void EmplaceScript(Args&&... args) const;

    template <typename T>
    void DetachScript() const;

    void SetParent(Entity& parent) const;
    void RemoveParent() const;

    void AddChild(Entity& child) const;

    Entity GetParent() const;
    const std::vector<Entity>& GetChildren() const;

    inline bool IsValid() const;

    inline entt::entity GetEntity() const { return m_Entity; }

  private:
    entt::entity m_Entity = entt::null;
    Scene* m_Scene = nullptr;
  };
}