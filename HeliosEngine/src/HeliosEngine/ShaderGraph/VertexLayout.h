#pragma once

#include "pch.h"

namespace Helios {
  struct VertexElement {
    enum class DataType {
      Int, Int2, Int3, Int4,
      Float, Vec2, Vec3, Vec4,
      Mat3, Mat4,
      Bool
    };

    VertexElement() = default;
    VertexElement(std::string_view name, DataType type, bool normalized = false)
      : name(name), type(type), size(DataTypeSize(type)), normalized(normalized) {
    }

    uint32_t DataTypeSize(DataType type) const {
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

      CORE_ASSERT(false, "Unknown DataType!");
      return 0;
    }

    std::string name;
    DataType type;

    uint32_t size = 0;
    uint32_t offset = 0;

    bool normalized = false;
  };
  
  class VertexLayout {
  public:
    VertexLayout() = default;
    VertexLayout(const std::initializer_list<VertexElement>& elements)
      : m_Elements(elements) {
      CalculateOffsetsAndStride();
    }

    inline uint64_t GetStride() const { return m_Stride; }
    inline const std::vector<VertexElement>& GetElements() const { return m_Elements; }

    inline std::vector<VertexElement>::iterator begin() { return m_Elements.begin(); }
    inline std::vector<VertexElement>::iterator end() { return m_Elements.end(); }
    inline std::vector<VertexElement>::const_iterator begin() const { return m_Elements.begin(); }
    inline std::vector<VertexElement>::const_iterator end() const { return m_Elements.end(); }

  private:
    void CalculateOffsetsAndStride() {
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