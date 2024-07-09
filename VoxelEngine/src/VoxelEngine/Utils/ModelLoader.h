#pragma once

#include "Render/Object.h"

namespace VoxelEngine
{
  class VOXELENGINE_API ModelLoader
  {
  public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    static void LoadModel(const std::string& path);

    static inline Object& GetObj() { return m_Object; }

  private:
    static void Reset();

    static const bool LoadObjFormat(const std::string& path);

  private:
    static Object m_Object;
  };
}