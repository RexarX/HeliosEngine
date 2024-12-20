#pragma once

#include "VertexElement.h"

namespace Helios {  
  class VertexLayout {
  public:
    VertexLayout(const std::initializer_list<VertexElement>& elements);
    VertexLayout(const VertexLayout&) = default;
    VertexLayout(VertexLayout&&) noexcept = default;
    ~VertexLayout() = default;

    VertexLayout& AddElement(std::string_view name, VertexElement::DataType type,  bool normalized = false);

    VertexLayout& operator=(const VertexLayout&) = default;
    VertexLayout& operator=(VertexLayout&&) noexcept = default;

    inline bool operator==(const VertexLayout& other) const { return m_Elements == other.m_Elements; }

    inline std::vector<VertexElement>::iterator begin() { return m_Elements.begin(); }
    inline std::vector<VertexElement>::iterator end() { return m_Elements.end(); }
    inline std::vector<VertexElement>::const_iterator begin() const { return m_Elements.cbegin(); }
    inline std::vector<VertexElement>::const_iterator end() const { return m_Elements.cend(); }

    inline bool IsEmpty() const { return m_Elements.empty(); }

    inline uint64_t GetElementCount() const { return m_Elements.size(); }

    inline uint64_t GetStride() const { return m_Stride; }

    inline const std::vector<VertexElement>& GetElements() const { return m_Elements; }
    
  private:
    void CalculateOffsetsAndStride();

  private:
    std::vector<VertexElement> m_Elements;
    uint64_t m_Stride = 0;
  };
}