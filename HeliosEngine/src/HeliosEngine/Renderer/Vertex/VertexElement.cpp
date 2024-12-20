#include "VertexElement.h"

namespace Helios {
  VertexElement::VertexElement(std::string_view name, DataType type, bool normalized)
    : m_Name(name), m_Type(type), m_Size(DataTypeSize(type)), m_Normalized(normalized) {
  }

  uint32_t VertexElement::DataTypeSize(DataType type) {
    switch (type) {
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

    CORE_ASSERT(false, "Failed to get data type size: Unknown DataType!");
    return 0;
  }

  uint32_t VertexElement::GetComponentCount(DataType type) {
    switch (type) {
      case DataType::Int: return 1;
      case DataType::Int2: return 2;
      case DataType::Int3: return 3;
      case DataType::Int4: return 4;
      case DataType::Float: return 1;
      case DataType::Vec2: return 2;
      case DataType::Vec3: return 3;
      case DataType::Vec4: return 4;
      case DataType::Mat3: return 3 * 3;
      case DataType::Mat4: return 4 * 4;
      case DataType::Bool: return 1;
    }

    CORE_ASSERT(false, "Failed to get component count: Unknown DataType!");
    return 0;
  }
}