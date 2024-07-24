#pragma once

#include "Scene/SceneNode.h"

#include "EntityComponentSystem/Manager/ECSManager.h"

namespace Helios
{
  class HELIOSENGINE_API Scene
  {
  public:
    Scene();
    Scene(const std::string& name);
    Scene(const Scene&) = delete;
    Scene(Scene&&) noexcept;
    ~Scene();

    void OnUpdate(const Timestep deltaTime) { m_ECSManager->OnUpdateSystems(deltaTime); }
    void OnEvent(Event& event) { m_ECSManager->OnEventSystems(event); }

    void SetName(const std::string& name) { m_Name = name; }
    void SetActive(const bool active) { m_Active = active; }

    SceneNode* AddNode(const std::string& name) const;
    void RemoveNode(SceneNode* node);

    template<typename T>
    T& AddNodeComponent(const SceneNode* node, const T& component);

    template<typename T, typename... Args>
    T& EmplaceNodeComponent(const SceneNode* node, Args&&... args);

    template<typename T>
    void RemoveNodeComponent(const SceneNode* node);

    template<typename T>
    const bool HasNodeComponent(const SceneNode* node) const;

    template<typename T>
    T& GetNodeComponent(const SceneNode* node);

    template<typename T>
    void AttachScript(const SceneNode* node, const T& script);

    template<typename T>
    void AttachScript(const SceneNode* node, T&& script);

    template<typename T, typename... Args>
    void EmplaceScript(const SceneNode* node, Args&&... args);

    template<typename T>
    void DetachScript(SceneNode* node);

    template<typename T>
    inline T& RegisterSystem(const uint32_t priority = 0) {
      return m_ECSManager->RegisterSystem<T>(priority);
    }

    template<typename T>
    inline T& GetSystem() { return m_ECSManager->GetSystem<T>(); }

    template<typename T>
    void RemoveSystem() { m_ECSManager->RemoveSystem<T>(); }

    inline const std::string& GetName() const { return m_Name; }
    inline const bool IsActive() const { return m_Active; }

    inline SceneNode* GetRootNode() const { return m_RootNode; }
    inline std::vector<SceneNode*>& GetNodes() { return m_RootNode->GetChildren(); }
    inline const std::vector<SceneNode*>& GetNodes() const { return m_RootNode->GetChildren(); }

    inline ECSManager& GetECSManager() { return *m_ECSManager; }

    Scene& operator=(const Scene&) = delete;
    Scene& operator=(Scene&&) noexcept;

  private:
    std::string m_Name = "default";
    bool m_Active = false;

    SceneNode* m_RootNode = nullptr;

    std::unique_ptr<ECSManager> m_ECSManager = nullptr;
  };

  template<typename T>
  T& Scene::AddNodeComponent(const SceneNode* node, const T& component)
  {
    CORE_ASSERT(node != nullptr, "Node is null!");
    m_ECSManager->RegisterComponent<T>();
    return m_ECSManager->AddComponent<T>(node->GetEntity(), component);
  }

  template<typename T, typename... Args>
  T& Scene::EmplaceNodeComponent(const SceneNode* node, Args&&... args)
  {
    CORE_ASSERT(node != nullptr, "Node is null!");
    m_ECSManager->RegisterComponent<T>();
    return m_ECSManager->EmplaceComponent<T>(node->GetEntity(), std::forward<Args>(args)...);
  }

  template<typename T>
  void Scene::RemoveNodeComponent(const SceneNode* node)
  {
    CORE_ASSERT(node != nullptr, "Node is null!");
    m_ECSManager->RemoveComponent<T>(node->GetEntity());
  }

  template<typename T>
  const bool Scene::HasNodeComponent(const SceneNode* node) const
  {
    CORE_ASSERT(node != nullptr, "Node is null!");
    return m_ECSManager->HasComponent<T>(node->GetEntity());
  }

  template<typename T>
  T& Scene::GetNodeComponent(const SceneNode* node)
  {
    CORE_ASSERT(node != nullptr, "Node is null!");
    return m_ECSManager->GetComponent<T>(node->GetEntity());
  }

  template<typename T>
  void Scene::AttachScript(const SceneNode* node, const T& script)
  {
    static_assert(std::is_base_of<Script, T>::value, "T must inherit from Script!");

    auto& scriptComponent = AddNodeComponent(node, script);
    scriptComponent.OnAttach();
  }

  template<typename T>
  void Scene::AttachScript(const SceneNode* node, T&& script)
  {
    auto& scriptComponent = AddNodeComponent(node, script);
    scriptComponent.OnAttach();
  }

  template<typename T, typename... Args>
  void Scene::EmplaceScript(const SceneNode* node, Args&&... args)
  {
    static_assert(std::is_base_of<Script, T>::value, "T must inherit from Script!");

    auto& scriptComponent = EmplaceNodeComponent<T>(node, std::forward<Args>(args)...);
    scriptComponent.OnAttach();
  }

  template<typename T>
  void Scene::DetachScript(SceneNode* node)
  {
    if (node == nullptr) { return; }

    EntityID entityId = node->GetEntity();

    if (m_ECSManager->HasComponent<Script>(entityId)) {
      auto& scriptComponent = m_ECSManager->GetComponent<Script>(entityId);
      scriptComponent.OnDetach();

      if (scriptComponent.IsEmpty()) {
        m_ECSManager->RemoveComponent<Script>(entityId);
      }
    }
  }
}