#include "Mesh.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>


#include <tiny_obj_loader/tiny_obj_loader.h>

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texCoord;

  const bool operator==(const Vertex& other) const
  {
    return pos == other.pos && texCoord == other.texCoord;
  }
};

namespace std
{
  template<> struct hash<Vertex>
  {
    uint64_t operator()(const Vertex& vertex) const
    {
      return hash<glm::vec3>()(vertex.pos) ^ hash<glm::vec2>()(vertex.texCoord) << 1;
    }
  };
}

namespace VoxelEngine
{
  bool Mesh::LoadObj(const std::string& path)
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str())) {
      CORE_ERROR("Failed to load obj: " + err);
      return false;
    }

    Vertex vertex;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    
    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],     
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
        };

        vertex.normal = {
          attrib.normals[3 * index.normal_index + 0],
          attrib.normals[3 * index.normal_index + 1],
          attrib.normals[3 * index.normal_index + 2]
        };

        vertex.texCoord = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          attrib.texcoords[2 * index.texcoord_index + 1]
        };

        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size() / 8);
          m_Vertices.push_back(vertex.pos.x);
          m_Vertices.push_back(vertex.pos.y);
          m_Vertices.push_back(vertex.pos.z);
          m_Vertices.push_back(vertex.normal.x);
          m_Vertices.push_back(vertex.normal.y);
          m_Vertices.push_back(vertex.normal.z);
          m_Vertices.push_back(vertex.texCoord.x);
          m_Vertices.push_back(vertex.texCoord.y);
        }

        m_Indices.push_back(uniqueVertices[vertex]);
      }
    }

    return true;
  }
}