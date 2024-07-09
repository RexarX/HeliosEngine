#include "ModelLoader.h"

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
    return pos == other.pos && texCoord == other.texCoord && normal == other.normal;
  }
};

namespace std
{
  template<> struct hash<Vertex>
  {
    uint64_t operator()(const Vertex& vertex) const
    {
      return hash<glm::vec3>()(vertex.pos) ^ hash<glm::vec3>()(vertex.normal) ^ hash<glm::vec2>()(vertex.texCoord);
    }
  };
}

namespace VoxelEngine
{
  Object ModelLoader::m_Object = Object();

  void ModelLoader::LoadModel(const std::string& path)
  {
    Reset();
    
    m_Object.SetName(std::filesystem::path(path).filename().string());

    m_Object.SetMesh(Mesh::Create());

    for (const auto& file : std::filesystem::directory_iterator(path)) {
      if (file.is_regular_file()) {
        if (file.path().extension().string() == ".obj") {
          CORE_INFO("Loading model: " + file.path().string() + '!');
          if (!LoadObjFormat(file.path().string())) { CORE_ERROR("Failed to load model!"); return; }
        }
      }
    }

    m_Object.GetShader() = Shader::Create(VOXELENGINE_DIR + "Assets/Shaders/Mesh.vert",
                                          VOXELENGINE_DIR + "Assets/Shaders/Mesh.frag");

    std::shared_ptr<Shader>& shader = m_Object.GetShader();

    m_Object.GetVertexArray() = VertexArray::Create();

    std::shared_ptr<VertexArray>& vertexArray = m_Object.GetVertexArray();

    m_Object.GetVertexBuffer() = VertexBuffer::Create(m_Object.GetMesh()->GetVertices().data(),
                                                      m_Object.GetMesh()->GetVertices().size() * sizeof(float));

    std::shared_ptr<VertexBuffer>& vertexBuffer = m_Object.GetVertexBuffer();

    vertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Position" },
      { ShaderDataType::Float3, "a_Normal" },
      { ShaderDataType::Float2, "a_TexCoord" } });

    m_Object.GetIndexBuffer() = IndexBuffer::Create(m_Object.GetMesh()->GetIndices().data(),
                                                    m_Object.GetMesh()->GetIndices().size());

    std::shared_ptr<IndexBuffer>& indexBuffer = m_Object.GetIndexBuffer();

    m_Object.AddUniformBuffer(UniformBuffer::Create(sizeof(UploadData)));

    vertexArray->AddVertexBuffer(vertexBuffer);
    vertexArray->SetIndexBuffer(indexBuffer);

    shader->Unbind();
    vertexArray->Unbind();
    vertexBuffer->Unbind();
    indexBuffer->Unbind();
  }

  void ModelLoader::Reset()
  {
    m_Object.GetMesh().reset();
    m_Object.SetName(std::string());
    m_Object.GetMesh().reset();
    m_Object.GetMaterial().reset();
    m_Object.GetShader().reset();
    m_Object.GetVertexArray().reset();
    m_Object.GetVertexBuffer().reset();
    m_Object.GetIndexBuffer().reset();

    for (auto& uniformBuffer : m_Object.GetUniformBuffers()) {
      uniformBuffer.reset();
    }
  }

  const bool ModelLoader::LoadObjFormat(const std::string& path)
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, (path).c_str(),
                          (path.substr(0, path.find_last_of('.'))).c_str())) {
      CORE_ERROR("Failed to load obj: " + err);
      return false;
    }

    std::vector<float>& vertices = m_Object.GetMesh()->GetVertices();

    std::vector<uint32_t>& indices = m_Object.GetMesh()->GetIndices();
    indices.reserve(attrib.vertices.size());

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
          uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size() / 8);
          vertices.insert(vertices.end(), { vertex.pos.x, vertex.pos.y, vertex.pos.z,
                                            vertex.normal.x, vertex.normal.y, vertex.normal.z,
                                            vertex.texCoord.x, vertex.texCoord.y });
        }

        indices.push_back(uniqueVertices[vertex]);
      }

      CORE_INFO("Mesh loaded: {0} vertices, {1} indices", vertices.size() / 8, indices.size());
    }

    return true;
  }
}