#pragma once

#include "VertexLayout.h"

#include <glm/glm.hpp>

namespace Helios {
  class Vertex {
  public:
    Vertex(const VertexLayout& layout) 
      : m_Layout(layout) {
      m_Data.resize(layout.GetStride());
    }

    Vertex(const Vertex&) = default;
    Vertex(Vertex&&) noexcept = default;
    ~Vertex() = default;

    template <typename T>
    Vertex& SetAttribute(std::string_view name, T&& value);

    template <typename T>
    Vertex& SetAttribute(std::string_view name, std::span<T> values);

    void Clear() { m_Data.clear(); }

    inline bool IsEmpty() const { return m_Data.empty(); }

    inline uint64_t Size() const { return m_Data.size(); }

    Vertex& operator=(const Vertex&) = default;
    Vertex& operator=(Vertex&&) noexcept = default;

    inline bool operator==(const Vertex& other) const {
      return m_Data == other.m_Data && m_Layout == other.m_Layout;
    }

    inline const VertexLayout& GetLayout() const { return m_Layout; }

    inline const std::vector<std::byte>& GetData() const { return m_Data; }

  private:
    template <typename T>
    static void ValidateType(const VertexElement& element);
    
  private:
    VertexLayout m_Layout;
    std::vector<std::byte> m_Data;
  };

  template <typename T>
  Vertex& Vertex::SetAttribute(std::string_view name, T&& value) {
    if (m_Layout.IsEmpty()) {
      CORE_ASSERT(false, "Failed to set attribute: Layout is empty!");
      return *this;
    }

    const auto& elements = m_Layout.GetElements();
    auto it = std::find_if(elements.begin(), elements.end(), [&name](const VertexElement& element) {
      return element.GetName() == name;
    });

    if (it == elements.end()) {
      CORE_ASSERT(false, "Failed to set attribute: value '{}' not found in the layout!", name);
      return *this;
    }

    ValidateType<T>(it);
    std::memcpy(m_Data.data() + it->offset, &value, it->size);
    return *this;
  }

  template <typename T>
  Vertex& Vertex::SetAttribute(std::string_view name, std::span<T> values) {
    if (values.empty()) {
      CORE_ASSERT(false, "Failed to set attribute: values in empty!");
      return *this;
    }
    
    const auto& elements = m_Layout.GetElements();
    auto it = std::find_if(elements.begin(), elements.end(), [&name](const VertexElement& element) {
      return element.GetName() == name;
    });

    if (it == elements.end()) {
      CORE_ASSERT(false, "Failed to set attribute: value '{}' not found in the layout!", name);
      return *this;
    }

    if (values.size() != it->GetComponentCount()) {
      CORE_ASSERT(false, "Failed to set attribute: Component count mismatch for attribute '{}'!", name);
      return *this;
    }

    if (values.size() * sizeof(T) != it->size) {
      CORE_ASSERT(false, "Failed to set attribute: Size mismatch for attribute '{}'!", name);
      return *this;
    }
    
    std::memcpy(m_Data.data() + it->offset, values.data(), it->size);
    return *this;
  }

  template <typename T>
  void Vertex::ValidateType(const VertexElement& element) {
    bool valid = false;
    VertexElement::DataType type = element.GetType();
    if constexpr (std::is_same_v<T, int32_t>) {
      valid = type == VertexElement::DataType::Int;
    } else if constexpr (std::is_same_v<T, glm::ivec2>) {
      valid = type == VertexElement::DataType::Int2;
    } else if constexpr (std::is_same_v<T, glm::ivec3>) {
      valid = type == VertexElement::DataType::Int3;
    } else if constexpr (std::is_same_v<T, glm::ivec4>) {
      valid = type == VertexElement::DataType::Int4;
    } else if constexpr (std::is_same_v<T, float>) {
      valid = type == VertexElement::DataType::Float;
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
      valid = type == VertexElement::DataType::Vec2;
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
      valid = type == VertexElement::DataType::Vec3;
    } else if constexpr (std::is_same_v<T, glm::vec4>) {
      valid = type == VertexElement::DataType::Vec4;
    } else if constexpr (std::is_same_v<T, glm::mat3>) {
      valid = type == VertexElement::DataType::Mat3;
    } else if constexpr (std::is_same_v<T, glm::mat4>) {
      valid = type == VertexElement::DataType::Mat4;
    } else if constexpr (std::is_same_v<T, bool>) {
      valid = type == VertexElement::DataType::Bool;
    }

    CORE_ASSERT(valid, "Failed to validate type: Type mismatch for attribute '{}'!", element.GetName());
  }
}