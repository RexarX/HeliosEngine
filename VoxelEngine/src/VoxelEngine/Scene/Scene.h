#pragma once

#include "Scene/SceneNode.h"

#include "EntityComponentSystem/Manager/ECSManager.h"

namespace Engine
{
  class VOXELENGINE_API Scene
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

    template<typename T, typename... Args>
    inline T& EmplaceNodeComponent(const SceneNode* node, Args&&... args);

    template<typename T>
    void RemoveNodeComponent(const SceneNode* node) { m_ECSManager->RemoveComponent<T>(node->GetEntity()); }

    template<typename T>
    inline const bool HasNodeComponent(const SceneNode* node) const { return m_ECSManager->HasComponent<T>(node->GetEntity()); }

    template<typename T>
    inline T& GetNodeComponent(const SceneNode* node) { return m_ECSManager->GetComponent<T>(node->GetEntity()); }

    inline const std::string& GetName() const { return m_Name; }
    inline const bool IsActive() const { return m_Active; }

    inline SceneNode* GetRootNode() const { return m_RootNode; }
    inline std::vector<SceneNode*>& GetNodes() { return m_RootNode->GetChildren(); }
    inline const std::vector<SceneNode*>& GetNodes() const { return m_RootNode->GetChildren(); }

    inline ECSManager& GetECSManager() { return *m_ECSManager; }

    template<typename T>
    inline T& RegisterSystem(const uint32_t priority = 0) { return m_ECSManager->RegisterSystem<T>(priority); }

    template<typename T>
    inline T& GetSystem() { return m_ECSManager->GetSystem<T>(); }

    template<typename T>
    void RemoveSystem() { m_ECSManager->RemoveSystem<T>(); }

    Scene& operator=(const Scene&) = delete;
    Scene& operator=(Scene&&) noexcept;

  private:
    std::string m_Name = "default";
    bool m_Active = false;

    SceneNode* m_RootNode = nullptr;

    std::unique_ptr<ECSManager> m_ECSManager = nullptr;
  };

  template<typename T, typename... Args>
  inline T& Scene::EmplaceNodeComponent(const SceneNode* node, Args&&... args)
  {
    m_ECSManager->RegisterComponent<T>();
    return m_ECSManager->EmplaceComponent<T>(node->GetEntity(), std::forward<Args>(args)...);
  }
}