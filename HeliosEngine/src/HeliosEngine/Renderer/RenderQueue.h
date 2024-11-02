#pragma once

#include "EntityComponentSystem/Components.h"

#include <glm/glm.hpp>

namespace Helios
{
  struct SceneData
  {
    glm::mat4 projectionViewMatrix = glm::mat4(1.0f);
  };

  struct RenderObject
  {
    const Renderable& renderable;
    const Transform& transform;
  };

  class RenderQueue
  {
  public:
    RenderQueue() = default;
    ~RenderQueue() = default;

    void Clear() { m_RenderObjects.clear(); }

    void AddRenderObject(const RenderObject& renderObject) {
      m_RenderObjects.push_back(renderObject);
    }

    void EmplaceRenderObject(const Renderable& renderable, const Transform& transform) {
      m_RenderObjects.emplace_back(renderable, transform);
    }

    void SetSceneData(const SceneData& sceneData) { m_SceneData = sceneData; }
    void SetSceneData(const glm::mat4& projectionViewMatrix) {
      m_SceneData.projectionViewMatrix = projectionViewMatrix;
    }

    inline const std::vector<RenderObject>& GetRenderObjects() const { return m_RenderObjects; }
    inline const SceneData& GetSceneData() const { return m_SceneData; }

  private:
    std::vector<RenderObject> m_RenderObjects;

    SceneData m_SceneData;
  };
}