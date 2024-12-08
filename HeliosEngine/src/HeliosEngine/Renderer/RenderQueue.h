#pragma once

#include "EntityComponentSystem/Components.h"

#include <glm/glm.hpp>

namespace Helios {
  class RenderQueue {
  public:
    struct SceneData {
      glm::mat4 projectionViewMatrix = glm::mat4(1.0f);
    };

    struct RenderObject {
      Renderable renderable;
      Transform transform;

      bool visible = true;

      inline bool operator==(const RenderObject& renderObject) const {
        return renderable == renderObject.renderable && transform == renderObject.transform;
      }
    };

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

    inline const std::vector<RenderObject>& GetRenderObjects() const { return m_RenderObjects; }
    inline const SceneData& GetSceneData() const { return m_SceneData; }

  private:
    std::vector<RenderObject> m_RenderObjects;

    SceneData m_SceneData;
  };
}