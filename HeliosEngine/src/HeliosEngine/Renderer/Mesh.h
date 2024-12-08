#pragma once

#include "VertexLayout.h"

namespace Helios {
class DynamicVertex {
  public:
    DynamicVertex(const VertexLayout& layout);

    template <typename T>
    void SetAttribute(std::string_view name, const T& value);

    inline const std::vector<std::byte>& GetData() const { return m_Data; }

  private:
    VertexLayout m_Layout;
    std::vector<std::byte> m_Data;
  };
  
  class HELIOSENGINE_API Mesh
  {
  public:
    enum class Type {
      Static,
      Dynamic
    };

    virtual ~Mesh() = default;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    virtual void SetData(const std::vector<std::byte>& vertexData, uint32_t vertexCount,
                         const std::vector<uint32_t>& indices = {}) = 0;

    virtual inline Type GetType() const = 0;

    virtual inline bool IsLoaded() const = 0;

    virtual inline const std::vector<std::byte>& GetVertices() const = 0;
    virtual inline const std::vector<uint32_t>& GetIndices() const = 0;

    virtual inline uint32_t GetVertexCount() const = 0;
    virtual inline uint32_t GetIndexCount() const = 0;

    virtual inline uint64_t GetVertexSize() const = 0;
    virtual inline uint64_t GetIndexSize() const = 0;

    virtual inline const VertexLayout& GetVertexLayout() const = 0;

    static std::shared_ptr<Mesh> Create(Type type, const VertexLayout& layout, const std::vector<std::byte>& vertices,
                                        uint32_t vertexCount, const std::vector<uint32_t>& indices = {});

    static std::shared_ptr<Mesh> Create(Type type, const VertexLayout& layout, uint32_t vertexCount, uint32_t indexCount);
  };

  template <typename T>
  void DynamicVertex::SetAttribute(std::string_view name, const T& value) {
    const auto& elements = m_Layout.GetElements();
    auto it = std::find_if(elements.begin(), elements.end(), [&name](const VertexElement& element) -> bool {
      return name == element.name;
    });

    if (it == elements.end()) {
      CORE_ASSERT(false, "Attribute not found in layout!")
      return;
    }

    memcpy(m_Data.data() + (*it).offset, &value, (*it).size);
  }
}