#pragma once

#include "EntityComponentSystem/Entity/Entity.h"

#include "pch.h"

namespace Engine
{
  class VOXELENGINE_API SceneNode
  {
  public:
    SceneNode() = default;
    SceneNode(const std::string& name);
    SceneNode(const std::string& name, const EntityID entity);
    SceneNode(const SceneNode&) = delete;
    SceneNode(SceneNode&&) noexcept;
    ~SceneNode();

    void Destroy();

    void SetName(const std::string& name) { m_Name = name; }

    void AddChild(SceneNode* child);
    void RemoveChild(SceneNode* child);
    void SetParent(SceneNode* parent);

    inline const std::string& GetName() const { return m_Name; }
    inline const EntityID GetEntity() const { return m_Entity; }

    inline SceneNode* GetParent() const { return m_Parent; }
    inline std::vector<SceneNode*>& GetChildren() { return m_Children; }
    inline const std::vector<SceneNode*>& GetChildren() const { return m_Children; }

    SceneNode& operator=(const SceneNode&) = delete;
    SceneNode& operator=(SceneNode&&) noexcept;

  private:
    std::string m_Name;
    EntityID m_Entity;

    SceneNode* m_Parent = nullptr;
    std::vector<SceneNode*> m_Children;
  };
}