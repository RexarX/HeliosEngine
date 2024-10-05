#include "CameraSystem.h"

#include "EntityComponentSystem/Components.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Helios
{
  void CameraSystem::OnUpdate(entt::registry& registry)
  {
    const auto& view = registry.view<Camera>();
    entt::entity cameraEntity = entt::null;

    for (entt::entity entity : view) {
      if (view.get<Camera>(entity).currect) { cameraEntity = entity; break; }
    }

    if (cameraEntity == entt::null) { return; }

    Camera& cameraComponent = registry.get<Camera>(cameraEntity);
    Transform& cameraTransform = registry.get<Transform>(cameraEntity);

    glm::vec3 direction = {
      cos(glm::radians(cameraTransform.rotation.y)) * cos(glm::radians(cameraTransform.rotation.x)),
      sin(glm::radians(cameraTransform.rotation.x)),
      sin(glm::radians(cameraTransform.rotation.y)) * cos(glm::radians(cameraTransform.rotation.x))
    };

    glm::vec3 cameraLeft = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 cameraUp = glm::cross(cameraLeft, direction);
    glm::vec3 cameraForward = glm::cross(cameraLeft, cameraUp);

    cameraComponent.camera.SetDirection(direction);
    cameraComponent.camera.SetLeft(cameraLeft);
    cameraComponent.camera.SetUp(cameraUp);
    cameraComponent.camera.SetForward(cameraForward);

    cameraComponent.camera.SetViewMatrix(
      glm::lookAt(cameraTransform.position, cameraTransform.position - direction, cameraUp)
    );

    cameraComponent.camera.SetProjectionMatrix(
      glm::perspective(cameraComponent.camera.GetFOV(), cameraComponent.camera.GetAspectRatio(),
      cameraComponent.camera.GetFarDistance(), cameraComponent.camera.GetNearDistance())
    );
  }

  void CameraSystem::OnEvent(entt::registry& registry, const Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(BIND_FN_WITH_REF(CameraSystem::OnWindowResize, registry));
  }

  bool CameraSystem::OnWindowResize(entt::registry& registry, const WindowResizeEvent& event)
  {
    if (event.GetWidth() == 0 || event.GetHeight() == 0) { return true; }

    for (entt::entity entity : registry.view<Camera>()) {
      registry.get<Camera>(entity).camera.SetAspectRatio(static_cast<float>(event.GetWidth()) /
                                                         static_cast<float>(event.GetHeight()));
    }

    return true;
  }
}