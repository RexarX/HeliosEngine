#include "Renderer.h"
#include "VertexArray.h"

#include "Platform/Vulkan/VulkanContext.h"

#include "vepch.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define MAX_VERTICES 65536

namespace VoxelEngine
{
  struct Cube
  {
    const std::string name = "Cube";
    std::shared_ptr<VertexArray> vertexArray;
    std::shared_ptr<VertexBuffer> vertexBuffer;
    std::shared_ptr<IndexBuffer> indexBuffer;
    std::shared_ptr<UniformBuffer> uniformBuffer;
    std::shared_ptr<Shader> shader;
  };

  struct Line
  {
    std::shared_ptr<VertexArray> LineVertex;
    std::shared_ptr<VertexBuffer> LineBuffer;
    std::shared_ptr<Shader> LineShader;
  };

  struct Skybox
  {
    std::shared_ptr<VertexArray> SkyboxVertex;
    std::shared_ptr<VertexBuffer> SkyboxBuffer;
    std::shared_ptr<Shader> SkyboxShader;
  };

  static Cube s_Cube;
  static Line s_Line;
  static Skybox s_Skybox;
  static UploadData data;

	void Renderer::Init()
	{
		RenderCommand::Init();
    
    constexpr float cubeVertices[] = {
      //front
      -0.5f, -0.5f, 0.5f,// 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
      0.5f, -0.5f, 0.5f,// 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, 0.5f,// 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top right
      0.5f, 0.5f, 0.5f,// 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top right
      -0.5f, 0.5f, 0.5f,// 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top left
      -0.5f, -0.5f, 0.5f,// 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left

      //back
      -0.5f, -0.5f, -0.5f,// 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom left
      0.5f, 0.5f, -0.5f,// 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top right
      0.5f, -0.5f, -0.5f,// 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, -0.5f// 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, -0.5f,// 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f, -0.5f,// 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top left
      
      //left
      -0.5f, 0.5f, -0.5f,// -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, 0.5f,// -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f, 0.5f,// -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
      -0.5f, -0.5f, 0.5f,// -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f, -0.5f,// -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, -0.5f,// -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right

      //right
      0.5f, 0.5f, 0.5f,// 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
      0.5f, -0.5f, -0.5f,// 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, -0.5f,// 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // top right
      0.5f, -0.5f, -0.5f,// 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, 0.5f,// 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
      0.5f, -0.5f, 0.5f,// 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // bottom left

      //bottom
      0.5f, -0.5f, -0.5f,// 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, 0.5f,// 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, -0.5f, -0.5f,// 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top left
      -0.5f, -0.5f, 0.5f,// 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      0.5f, -0.5f, -0.5f,// 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top right
      0.5f, -0.5f, 0.5f,// 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom right

      //top
      -0.5f, 0.5f, -0.5f,// 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
      0.5f, 0.5f, 0.5f,// 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, -0.5f,// 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top right
      0.5f, 0.5f, 0.5f,// 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -0.5f, 0.5f, -0.5f,// 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
      -0.5f, 0.5f, 0.5f,// 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
    };

    constexpr std::array<float, 70> vertices = {
      0.5f, 0.5f, -0.5f, 0.0f, 2.0f / 3, // 1
      0.5f, -0.5f, -0.5f, 0.0f, 1.0f / 3, // 2
      -0.5f, -0.5f, -0.5f, 0.25f, 1.0f / 3, // 3
      -0.5f, 0.5f, -0.5f, 0.25f, 2.0f / 3, // 4
      -0.5f, -0.5f, 0.5f, 0.5f, 1.0f / 3, // 5
      -0.5f, 0.5f, 0.5f, 0.5f, 2.0f / 3, // 6
      -0.5f, 0.5f, -0.5f, 0.5f, 1.0f, // 7
      0.5f, 0.5f, 0.5f, 0.75f, 2.0f / 3, // 8
      0.5f, 0.5f, -0.5f, 0.75f, 1.0f, // 9
      0.5f, 0.5f, -0.5f, 1.0f, 2.0f / 3, // 10
      0.5f, -0.5f, -0.5f, 1.0f, 1.0f / 3, // 11
      0.5f, -0.5f, 0.5f, 0.75f, 1.0f / 3, // 12
      0.5f, -0.5f, -0.5f, 0.75f, 0.0f, // 13
      -0.5f, -0.5f, -0.5f, 0.5f, 0.0f, // 14
    };

    constexpr std::array<uint32_t, 36> cubeIndices = {
      0, 1, 2, 0, 2, 3, // back
      3, 2, 4, 3, 4, 5, // left
      6, 5, 7, 6, 7, 8, // top
      7, 11, 10, 7, 10, 9, // right
      5, 4, 11, 5, 11, 7, // front
      4, 13, 12, 4, 12, 11, // bottom
    };

    constexpr float lineVertices[] = {
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f
    };

    constexpr float skyboxVertices[] = {
      // positions          
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      -1.0f,  1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f
    };
    
    /*
    s_Cube.shader = Shader::Create(s_Cube.name, VOXELENGINE_DIR + "Assets/Shaders/Cube.vert",
                                                VOXELENGINE_DIR + "Assets/Shaders/Cube.frag");

    s_Cube.uniformBuffer = UniformBuffer::Create(s_Cube.name, sizeof(s_SceneData));

    s_Cube.shader->AddUniformBuffer(s_Cube.uniformBuffer);
    s_Cube.shader->Unbind();

    s_Cube.vertexArray = VertexArray::Create(s_Cube.name);

    s_Cube.vertexBuffer = VertexBuffer::Create(s_Cube.name, vertices.data(), vertices.size() * sizeof(float));
    s_Cube.vertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Pos" },
      { ShaderDataType::Float2, "a_TexCoord" }
      });

    s_Cube.vertexArray->AddVertexBuffer(s_Cube.vertexBuffer);

    s_Cube.vertexBuffer->Unbind();

    s_Cube.indexBuffer = IndexBuffer::Create(s_Cube.name, cubeIndices.data(), cubeIndices.size());

    s_Cube.vertexArray->SetIndexBuffer(s_Cube.indexBuffer);

    s_Cube.indexBuffer->Unbind();

    s_Cube.vertexBuffer->Unbind();*/

    /*s_Line.LineVertex = VertexArray::Create();

    s_Line.LineBuffer = VertexBuffer::Create(lineVertices, sizeof(lineVertices));
    s_Line.LineBuffer->SetLayout({ { ShaderDataType::Float3, "a_Pos" } });

    s_Line.LineVertex->AddVertexBuffer(s_Line.LineBuffer);

    s_Line.LineBuffer->Unbind();

    s_Line.LineVertex->Unbind();

    s_Line.LineShader = Shader::Create("Line", VOXELENGINE_DIR + "Assets/Shaders/Line.vert",
                                               VOXELENGINE_DIR + "Assets/Shaders/Line.frag");
    s_Line.LineShader->Unbind();

    s_Skybox.SkyboxVertex = VertexArray::Create();

    s_Skybox.SkyboxBuffer = VertexBuffer::Create(skyboxVertices, sizeof(skyboxVertices));
    s_Skybox.SkyboxBuffer->SetLayout({ { ShaderDataType::Float3, "a_Pos" } });

    s_Skybox.SkyboxVertex->AddVertexBuffer(s_Skybox.SkyboxBuffer);

    s_Skybox.SkyboxBuffer->Unbind();

    s_Skybox.SkyboxVertex->Unbind();

    s_Skybox.SkyboxShader = Shader::Create("Skybox", VOXELENGINE_DIR + "Assets/Shaders/Skybox.vert",
                                                     VOXELENGINE_DIR + "Assets/Shaders/Skybox.frag");
    s_Skybox.SkyboxShader->Unbind();*/
	}

  void Renderer::Shutdown()
  {
  }

  void Renderer::OnWindowResize(const uint32_t width, const uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

  void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size,
                          const glm::vec3& rotation, const std::shared_ptr<Texture>& texture)
  {
    if (texture != nullptr) { texture->Bind(); }

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    s_Cube.shader->Bind();
    s_Cube.vertexArray->Bind();
    s_Cube.vertexBuffer->Bind();
    s_Cube.indexBuffer->Bind();

    //s_SceneData.transformMatrix = transform;

    //s_Cube.uniformBuffer->SetData(&s_SceneData, sizeof(s_SceneData));

    RenderCommand::DrawIndexed(s_Cube.vertexArray, s_Cube.indexBuffer->GetCount());
  }

  void Renderer::DrawLine(const glm::vec3& position, const glm::vec3& rotation,
                          const float lenght) 
  {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(lenght));
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    s_Line.LineShader->Bind();
    s_Line.LineVertex->Bind();
    s_Line.LineBuffer->Bind();

    //s_Line.LineShader->UploadUniformMat4("u_Projection", s_SceneData.projectionViewMatrix);
    s_Line.LineShader->UploadUniformMat4("u_Transform", transform);
    
    RenderCommand::DrawLine(s_Line.LineVertex, 2);
  }

  void Renderer::DrawSkybox(const std::shared_ptr<Texture>& texture)
  {
    RenderCommand::SetDepthMask(false);

    texture->Bind();

    s_Skybox.SkyboxShader->Bind();
    s_Skybox.SkyboxVertex->Bind();
    s_Skybox.SkyboxBuffer->Bind();

    //s_Skybox.SkyboxShader->UploadUniformMat4("u_ViewProjection", s_SceneData.projectionViewMatrix);

    RenderCommand::DrawArray(s_Skybox.SkyboxVertex, 36);

    RenderCommand::SetDepthMask(true);
  }

  void Renderer::DrawMesh(const std::shared_ptr<Mesh>& mesh, const glm::vec3& position,
                          const glm::vec3& scale, const glm::vec3& rotation)
  {
  }

  void Renderer::DrawObject(const SceneData& sceneData, const Object& object)
  {
    data.projectionViewMatrix = sceneData.projectionViewMatrix;
    data.transformMatrix = object.GetTransform().transformMatrix;

    if (object.GetMaterial() != nullptr) {
      object.GetMaterial()->GetDiffuseMap()->Bind();
    }

    object.GetShader()->Bind();
    object.GetVertexArray()->Bind();
    object.GetVertexBuffer()->Bind();
    object.GetIndexBuffer()->Bind();

    object.GetUniformBuffers()[0]->SetData(&data, sizeof(data));

    RenderCommand::DrawIndexed(object.GetVertexArray(), object.GetIndexBuffer()->GetCount());

    object.GetShader()->Unbind();
    object.GetVertexArray()->Unbind();
    object.GetVertexBuffer()->Unbind();
    object.GetIndexBuffer()->Unbind();
  }
}