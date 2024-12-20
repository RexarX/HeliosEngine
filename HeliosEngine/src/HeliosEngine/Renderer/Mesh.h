#pragma once

#include "Vertex/MeshData.h"

namespace Helios {  
  class HELIOSENGINE_API Mesh {
  public:
    enum class Type {
      Static,
      Dynamic
    };

    virtual ~Mesh() = default;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    virtual void SetMeshData(const MeshData& meshData) = 0;

    virtual inline Type GetType() const = 0;

    virtual inline bool IsLoaded() const = 0;

    virtual inline const std::vector<std::byte>& GetVertices() const = 0;
    virtual inline const std::vector<uint32_t>& GetIndices() const = 0;

    virtual inline uint32_t GetVertexCount() const = 0;
    virtual inline uint32_t GetIndexCount() const = 0;

    virtual inline uint64_t GetVertexSize() const = 0;
    virtual inline uint64_t GetIndexSize() const = 0;

    virtual inline const VertexLayout& GetVertexLayout() const = 0;

    static std::shared_ptr<Mesh> Create(Type type, const MeshData& vertexData);
    static std::shared_ptr<Mesh> Create(Type type, MeshData&& vertexData);
  };
}