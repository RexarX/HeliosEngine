#include "VertexLayout.h"

namespace Helios {
  VertexLayout::VertexLayout(const std::initializer_list<VertexElement>& elements)
    : m_Elements(elements) {
    CalculateOffsetsAndStride();
  }

  VertexLayout& VertexLayout::AddElement(std::string_view name, VertexElement::DataType type, bool normalized) {
    m_Elements.emplace_back(name, type, normalized);
    CalculateOffsetsAndStride();
    return *this;
  }

  void VertexLayout::CalculateOffsetsAndStride() {
    uint64_t offset = 0;
    m_Stride = 0;
    for (VertexElement& element : m_Elements) {
      element.m_Offset = offset;
      offset += element.m_Size;
      m_Stride += element.m_Size;
    }
  }
}