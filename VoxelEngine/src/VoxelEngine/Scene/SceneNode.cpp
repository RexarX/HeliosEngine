#include "Scene/SceneNode.h"

namespace Engine
{
  SceneNode::SceneNode(const std::string& name)
    : m_Name(name)
  {
  }

  SceneNode::SceneNode(const std::string& name, const EntityID entity)
    : m_Name(name), m_Entity(entity)
  {
  }

  SceneNode::SceneNode(SceneNode&& other) noexcept
    : m_Name(std::move(other.m_Name)), m_Entity(other.m_Entity),
    m_Parent(other.m_Parent), m_Children(std::move(other.m_Children))
  {
    other.m_Name.clear();
    other.m_Parent = nullptr;
    other.m_Children.clear();
  }

  SceneNode::~SceneNode()
  {
    Destroy();
  }

  void SceneNode::Destroy()
  {
    for (auto* child : m_Children) {
      RemoveChild(child);
    }
    m_Name.clear();
    m_Parent = nullptr;
    m_Children.clear();
  }

  void SceneNode::AddChild(SceneNode* child)
  {
    if (child == nullptr || child->m_Parent == this) { return; }
    m_Children.push_back(child);
    child->SetParent(this);
  }

  void SceneNode::RemoveChild(SceneNode* child)
  {
    if (child == nullptr) { return; }
    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
      child->Destroy();
      m_Children.erase(it);
      delete child;
    }
  }

  void SceneNode::SetParent(SceneNode* parent)
  {
    if (parent == m_Parent || parent == nullptr) { return; }
    m_Parent = parent;
  }

  SceneNode& SceneNode::operator=(SceneNode&& other) noexcept
  {
    if (this == &other) { return *this; }

    m_Name = std::move(other.m_Name);
    m_Entity = other.m_Entity;
    m_Parent = other.m_Parent;
    m_Children = std::move(other.m_Children);

    other.m_Name.clear();
    other.m_Parent = nullptr;
    other.m_Children.clear();

    return *this;
  }
}