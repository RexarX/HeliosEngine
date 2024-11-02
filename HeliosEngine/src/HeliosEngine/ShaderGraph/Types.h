#pragma once

#include "pch.h"

namespace Helios
{
  enum class NodeType
  {
    Input,
    Output,
    Math,
    Texture2D
  };

  enum class DataType
  {
    Int, Int2, Int3, Int4,
    Float, Vec2, Vec3, Vec4,
    Mat3, Mat4,
    Bool,
    Sampler2D
  };

  struct NodePort
  {
    std::string name;
    DataType type;
  };

  static uint32_t DataTypeSize(DataType type)
  {
    switch (type)
    {
    case DataType::Int: return 4;
    case DataType::Int2: return 4 * 2;
    case DataType::Int3: return 4 * 3;
    case DataType::Int4: return 4 * 4;
    case DataType::Float: return 4;
    case DataType::Vec2: return 4 * 2;
    case DataType::Vec3: return 4 * 3;
    case DataType::Vec4: return 4 * 4;
    case DataType::Mat3: return 4 * 3 * 3;
    case DataType::Mat4: return 4 * 4 * 4;
    case DataType::Bool: return 1;
    }

    CORE_ASSERT(false, "Unknown DataType!")
    return 0;
  }
}