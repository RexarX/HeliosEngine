#pragma once

#include "Mesh.h"
#include "Material.h"
#include "Shader.h"
#include "VertexArray.h"
#include "DataStructures.h"

namespace VoxelEngine
{
  class Object
  {
  public:
    Object() = default;
    Object(const std::string& name);
    ~Object() = default;

    void SetName(const std::string& name) { m_Name = name; }

    void SetID(const uint32_t id) { m_ID = id; }

    void SetTransform(const Transform& transform) { m_Transform = transform; }

    void SetPosition(const glm::vec3& position) { m_Transform.position = position; }
    void SetRotation(const glm::vec3& rotation) { m_Transform.rotation = rotation; }
    void SetScale(const glm::vec3& scale) { m_Transform.scale = scale; }

    void SetMesh(const std::shared_ptr<Mesh>& mesh) { m_Mesh = mesh; }
    void SetMaterial(const std::shared_ptr<Material>& material) { m_Material = material; }

    void SetShader(const std::shared_ptr<Shader>& shader) { m_Shader = shader; }
    void SetVertexArray(std::shared_ptr<VertexArray>& vertexArray) { m_VertexArray = vertexArray; }
    void SetVertexBuffer(std::shared_ptr<VertexBuffer>& vertexBuffer) { m_VertexBuffer = vertexBuffer; }
    void SetIndexBuffer(std::shared_ptr<IndexBuffer>& indexBuffer) { m_IndexBuffer = indexBuffer; }
    void AddUniformBuffer(const std::shared_ptr<UniformBuffer>& uniformBuffer) { m_UniformBuffers.push_back(uniformBuffer); }

    inline const std::string& GetName() const { return m_Name; }

    inline const uint32_t GetID() const { return m_ID; }

    inline const Transform& GetTransform() const { return m_Transform; }
    inline Transform& GetTransform() { return m_Transform; }

    inline const std::shared_ptr<Mesh>& GetMesh() const { return m_Mesh; }
    inline std::shared_ptr<Mesh>& GetMesh() { return m_Mesh; }

    inline const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
    inline std::shared_ptr<Material>& GetMaterial() { return m_Material; }

    inline const std::shared_ptr<Shader>& GetShader() const { return m_Shader; }
    inline std::shared_ptr<Shader>& GetShader() { return m_Shader; }

    inline const std::shared_ptr<VertexArray>& GetVertexArray() const { return m_VertexArray; }
    inline std::shared_ptr<VertexArray>& GetVertexArray() { return m_VertexArray; }

    inline const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
    inline std::shared_ptr<VertexBuffer>& GetVertexBuffer() { return m_VertexBuffer; }

    inline const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
    inline std::shared_ptr<IndexBuffer>& GetIndexBuffer() { return m_IndexBuffer; }

    inline const std::vector<std::shared_ptr<UniformBuffer>>& GetUniformBuffers() const { return m_UniformBuffers; }
    inline std::vector<std::shared_ptr<UniformBuffer>>& GetUniformBuffers() { return m_UniformBuffers; }

  private:
    uint32_t m_ID = 0;
    std::string m_Name;

    Transform m_Transform;

    std::shared_ptr<Mesh> m_Mesh = nullptr;
    std::shared_ptr<Material> m_Material = nullptr;

    std::shared_ptr<Shader> m_Shader = nullptr;
    std::shared_ptr<VertexArray> m_VertexArray = nullptr;
    std::shared_ptr<VertexBuffer> m_VertexBuffer = nullptr;
    std::shared_ptr<IndexBuffer> m_IndexBuffer = nullptr;
    std::vector<std::shared_ptr<UniformBuffer>> m_UniformBuffers;
  };
}