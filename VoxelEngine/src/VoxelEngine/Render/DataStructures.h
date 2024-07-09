#pragma once

#include <glm/glm.hpp>

namespace VoxelEngine
{
  struct SceneData
  {
    glm::mat4 projectionViewMatrix;
  };

  struct UploadData
  {
    glm::mat4 projectionViewMatrix;
    glm::mat4 transformMatrix;
  };

  struct Transform
  {
    void CalculateTransformMatrix();

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 transformMatrix;
  };
}