#pragma once

#include "Object.h"
#include "CameraController.h"
#include "DataStructures.h"

namespace VoxelEngine
{
  class VOXELENGINE_API Scene
  {
  public:
    Scene();
    Scene(const std::string& name);
    ~Scene() = default;

    void OnUpdate(const Timestep ts);
    void OnEvent(Event& e);
    void Render();

    void AddObject(const Object& object, const std::string& name = std::string());

    void SetName(const std::string& name) { m_Name = name; }

    void SetActive(const bool active) { m_Active = active; }

    inline const std::string& GetName() const { return m_Name; }

    inline const bool IsActive() const { return m_Active; }

    inline const CameraController& GetCameraController() const { return m_CameraController; }
    inline CameraController& GetCameraController() { return m_CameraController; }

    inline const SceneData& GetSceneData() const { return s_SceneData; }

    inline const Object& GetObj(const uint32_t id) const { return m_Objects.at(id); }
    inline Object& GetObj(const uint32_t id) { return m_Objects[id]; }

  private:
    std::string m_Name;

    bool m_Active = false;

    uint32_t m_IdCounter = 0;

    CameraController m_CameraController;

    SceneData s_SceneData;
    
    std::unordered_map<uint32_t, Object> m_Objects;
  };
}