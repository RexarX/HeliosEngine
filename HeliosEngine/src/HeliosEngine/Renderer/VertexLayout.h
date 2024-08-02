#pragma once

#include "pch.h"

namespace Helios
{
  enum class DataType
  {
    None = 0,
    Float, Float2, Float3, Float4,
    Mat3, Mat4,
    Int, Int2, Int3, Int4,
    Bool
  };

  static const uint32_t DataTypeSize(const DataType type)
  {
    switch (type)
    {
    case DataType::Float: return 4;
    case DataType::Float2: return 4 * 2;
    case DataType::Float3: return 4 * 3;
    case DataType::Float4: return 4 * 4;
    case DataType::Mat3: return 4 * 3 * 3;
    case DataType::Mat4: return 4 * 4 * 4;
    case DataType::Int: return 4;
    case DataType::Int2: return 4 * 2;
    case DataType::Int3: return 4 * 3;
    case DataType::Int4: return 4 * 4;
    case DataType::Bool: return 1;
    }

    CORE_ASSERT(false, "Unknown DataType!");
    return 0;
  }

  struct VertexElement
  {
    VertexElement() = default;

    VertexElement(const DataType type, const std::string& name,
                  const bool normalized = false)
      : name(name), type(type), normalized(normalized), size(DataTypeSize(type))
    {
    }

    const uint32_t GetComponentCount() const
    {
      switch (type)
      {
      case DataType::Float:  return 1;
      case DataType::Float2: return 2;
      case DataType::Float3: return 3;
      case DataType::Float4: return 4;
      case DataType::Mat3:   return 3 * 3;
      case DataType::Mat4:   return 4 * 4;
      case DataType::Int:    return 1;
      case DataType::Int2:   return 2;
      case DataType::Int3:   return 3;
      case DataType::Int4:   return 4;
      case DataType::Bool:   return 1;
      }

      CORE_ASSERT(false, "Unknown DataType!");
      return 0;
    }

    std::string name;
    DataType type;

    uint32_t size = 0;
    uint32_t offset = 0;

    bool normalized = false;
  };

  class VertexLayout
  {
  public:
    VertexLayout() = default;

    VertexLayout(const std::initializer_list<VertexElement>& elements)
      : m_Elements(elements)
    {
      CalculateOffsetsAndStride();
    }

    inline const uint64_t GetStride() const { return m_Stride; }
    inline const std::vector<VertexElement>& GetElements() const { return m_Elements; }

    std::vector<VertexElement>::iterator begin() { return m_Elements.begin(); }
    std::vector<VertexElement>::iterator end() { return m_Elements.end(); }
    std::vector<VertexElement>::const_iterator begin() const { return m_Elements.begin(); }
    std::vector<VertexElement>::const_iterator end() const { return m_Elements.end(); }

  private:
    void CalculateOffsetsAndStride()
    {
      uint64_t offset = 0;
      m_Stride = 0;
      for (auto& element : m_Elements) {
        element.offset = offset;
        offset += element.size;
        m_Stride += element.size;
      }
    }

  private:
    std::vector<VertexElement> m_Elements;
    uint64_t m_Stride = 0;
  };
}