#pragma once

#include "pch.h"

namespace Helios {
  class VertexElement {
  public:
    enum class DataType : uint32_t {
      Int, Int2, Int3, Int4,
      Float, Vec2, Vec3, Vec4,
      Mat3, Mat4,
      Bool
    };

    VertexElement(std::string_view name, DataType type, bool normalized = false);
    VertexElement(const VertexElement&) = default;
    VertexElement(VertexElement&&) noexcept = default;
    ~VertexElement() = default;

    static uint32_t DataTypeSize(DataType type);
    static uint32_t GetComponentCount(DataType type);
    
    inline uint32_t GetComponentCount() const { return GetComponentCount(m_Type); }

    VertexElement& operator=(const VertexElement&) = default;
    VertexElement& operator=(VertexElement&&) noexcept = default;

    inline bool operator==(const VertexElement& other) const {
      return m_Name == other.m_Name && m_Type == other.m_Type && m_Size == other.m_Size
             && m_Offset == other.m_Offset && m_Normalized == other.m_Normalized;
    }

    inline const std::string& GetName() const { return m_Name; }

    inline DataType GetType() const { return m_Type; }

    inline uint32_t GetSize() const { return m_Size; }
    inline uint32_t GetOffset() const { return m_Offset; }

    inline bool IsNormalized() const { return m_Normalized; }

  private:
    std::string m_Name;
    DataType m_Type;

    uint32_t m_Size = 0;
    uint32_t m_Offset = 0;

    bool m_Normalized = false;

    friend class VertexLayout;
  };
}