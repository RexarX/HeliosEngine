#pragma once

#include "ShaderGraph/VertexLayout.h"

namespace Helios
{
  enum class MeshType
  {
    Static, Dynamic
  };

  class DynamicVertex
  {
  public:
    DynamicVertex(const VertexLayout& layout);

    template<typename T>
    void SetAttribute(std::string_view name, const T& value);

    inline const std::vector<std::byte>& GetData() const { return m_Data; }

  private:
    VertexLayout m_Layout;
    std::vector<std::byte> m_Data;
  };
  
  class HELIOSENGINE_API Mesh
  {
  public:
    virtual ~Mesh() = default;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    virtual void SetData(const std::vector<std::byte>& vertexData, uint32_t vertexCount,
                         const std::vector<uint32_t>& indices = {}) = 0;
    virtual void SetVertexLayout(const VertexLayout& layout) = 0;

    virtual inline MeshType GetType() const = 0;

    virtual inline bool IsLoaded() const = 0;

    virtual inline const std::vector<std::byte>& GetVertices() const = 0;
    virtual inline const std::vector<uint32_t>& GetIndices() const = 0;

    virtual inline uint32_t GetVertexCount() const = 0;
    virtual inline uint32_t GetIndexCount() const = 0;

    virtual inline uint64_t GetVertexSize() const = 0;
    virtual inline uint64_t GetIndexSize() const = 0;

    virtual inline const VertexLayout& GetVertexLayout() const = 0;

    static std::shared_ptr<Mesh> Create(MeshType type, const std::vector<std::byte>& vertices,
                                        uint32_t vertexCount, const std::vector<uint32_t>& indices = {});

    static std::shared_ptr<Mesh> Create(MeshType type, uint32_t vertexCount, uint32_t indexCount);
  };

  template<typename T>
  void DynamicVertex::SetAttribute(std::string_view name, const T& value)
  {
    for (const auto& element : m_Layout.GetElements()) {
      if (element.m_Name == name) {
        memcpy(m_Data.data() + element.m_Offset, &value, element.m_Size);
        return;
      }
    }
    CORE_ASSERT(false, "Attribute not found in layout!");
  }
}