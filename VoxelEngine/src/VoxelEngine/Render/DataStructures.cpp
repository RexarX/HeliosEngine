#include "DataStructures.h"

#include <glm/gtc/matrix_transform.hpp>

namespace VoxelEngine
{
  void Transform::CalculateTransformMatrix()
  {
    transformMatrix = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), scale);
    transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
  }
}