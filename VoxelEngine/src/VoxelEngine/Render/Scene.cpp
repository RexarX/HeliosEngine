#include "Scene.h"
#include "Renderer.h"

#include "Platform/Vulkan/VulkanContext.h"

namespace VoxelEngine
{
  Scene::Scene()
    : m_Name("default"), m_CameraController(glm::vec3(0.0f), glm::vec3(0.0f), 16.0f / 9.0f)
  {
  }
  Scene::Scene(const std::string& name)
    : m_Name(name), m_CameraController(glm::vec3(0.0f), glm::vec3(0.0f), 16.0f / 9.0f)
  {
  }

  void Scene::OnUpdate(const Timestep ts)
  {
    m_CameraController.OnUpdate(ts);
  }

  void Scene::OnEvent(Event& e)
  {
    m_CameraController.OnEvent(e);
  }

  void Scene::Render()
  {
    s_SceneData.projectionViewMatrix = m_CameraController.GetCamera().GetProjectionViewMatrix();

    for (auto& [id, object] : m_Objects) {
      object.GetTransform().CalculateTransformMatrix();
      Renderer::DrawObject(s_SceneData, object);
    }
  }

  void Scene::AddObject(const Object& object, const std::string& name)
  {
    m_Objects.emplace(m_IdCounter, object);

    Object& obj = GetObj(m_IdCounter);

    if (!name.empty()) { obj.SetName(name); }
    obj.SetID(m_IdCounter);

    if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan) {
      VulkanContext& context = VulkanContext::Get();

      context.AddPipeline(m_IdCounter);

      Pipeline& pipeline = context.GetPipeline(m_IdCounter);

      pipeline.AddShader(obj.GetShader());
      pipeline.AddVertexArray(obj.GetVertexArray());

      for (auto& uniform : obj.GetUniformBuffers()) {
        pipeline.AddUniformBuffer(uniform);
      }

      if (obj.GetMaterial() != nullptr) {
        pipeline.AddTexture(obj.GetMaterial()->GetDiffuseMap());
      }
    }

    ++m_IdCounter;
  }
}