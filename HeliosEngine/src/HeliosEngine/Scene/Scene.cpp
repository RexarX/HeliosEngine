#include "Scene.h"

namespace Helios
{
  Scene::Scene()
    : m_ECSManager(std::make_unique<ECSManager>()), m_RootNode(new SceneNode(m_Name))
  {
  }

  Scene::Scene(const std::string& name)
    : m_Name(name), m_ECSManager(std::make_unique<ECSManager>()),
    m_RootNode(new SceneNode(name))
  {
  }

  Scene::Scene(Scene&& other) noexcept
    : m_Name(std::move(other.m_Name)), m_ECSManager(std::move(other.m_ECSManager)),
    m_RootNode(other.m_RootNode)
  {
    other.m_Name.clear();
    other.m_ECSManager.release();
    other.m_RootNode = nullptr;
  }
  Scene::~Scene()
  {
    if (m_RootNode != nullptr) {
      m_RootNode->Destroy();
      delete m_RootNode;
    }
  }

  void Scene::Load()
  {
  }

  void Scene::Unload()
  {
  }

  SceneNode* Scene::AddNode(const std::string& name) const
  {
    EntityID id = m_ECSManager->CreateEntity();

    SceneNode* node = new SceneNode(name, id);
    node->SetParent(m_RootNode);
    m_RootNode->AddChild(node);
    return node;
  }

  void Scene::RemoveNode(SceneNode* node)
  {
    if (node == nullptr) { return; }
    m_ECSManager->DestroyEntity(node->GetEntity());
    m_RootNode->RemoveChild(node);
  }

  Scene& Scene::operator=(Scene&& other) noexcept
  {
    if (this == &other) { return *this; }

    m_Name = std::move(other.m_Name);
    m_ECSManager = std::move(other.m_ECSManager);
    m_RootNode = other.m_RootNode;

    other.m_Name.clear();
    other.m_ECSManager.release();
    other.m_RootNode = nullptr;

    return *this;
  }
}