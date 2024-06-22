#include "vepch.h"

#include "Renderer.h"

#include "VertexArray.h"

#include "Platform/Vulkan/VulkanContext.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define MAX_VERTICES 65536

namespace VoxelEngine
{
  struct Triangle
  {
    const char* name = "Triangle";
    std::shared_ptr<VertexArray> VertexArray;
    std::shared_ptr<VertexBuffer> VertexBuffer;
    std::shared_ptr<IndexBuffer> IndexBuffer;

    std::shared_ptr<UniformBuffer> UniformBuffer;

    std::shared_ptr<Shader> Shader;
  };

  struct Cube
  {
    std::shared_ptr<VertexArray> CubeVertex;
    std::shared_ptr<VertexBuffer> CubeBuffer;
    std::shared_ptr<VertexBuffer> CubeMatrixBuffer;
    std::shared_ptr<IndexBuffer> CubeIndex;

    std::shared_ptr<Shader> CubeShader;
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

  struct ChunkedData
  {
    std::shared_ptr<VertexArray> ChunkVertex;
    std::shared_ptr<VertexBuffer> ChunkBuffer;
    std::shared_ptr<IndexBuffer> ChunkIndex;
  };
  
	std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();

  static Triangle s_Triangle;
  static Cube s_Cube;
  static Line s_Line;
  static Skybox s_Skybox;
  static ChunkedData s_ChunkedData;

	void Renderer::Init()
	{
		RenderCommand::Init();
    
    constexpr float cubeVertices[] = {
      //front
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
      0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top right
      0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top right
      -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top left
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left

      //back
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom left
      0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top right
      0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top left
      
      //left
      -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
      -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right

      //right
      0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
      0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // top right
      0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
      0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // bottom left

      //bottom
      0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top left
      -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top right
      0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom right

      //top
      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
      0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top right
      0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
      -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
    };

    constexpr float vertices[] = {
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

    uint32_t cubeIndices[] = {
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

    constexpr float triangle[] = {
      -1.0f,  1.0f, 0.0f,
       1.0f,  1.0f, 0.0f,
       0.0f, -1.0f, 0.0f
    };
    
    /*s_ChunkedData.ChunkVertex = VertexArray::Create();

    s_ChunkedData.ChunkBuffer = VertexBuffer::Create(sizeof(float) * MAX_VERTICES);

    s_ChunkedData.ChunkBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Pos" },
      { ShaderDataType::Float2, "a_TexCoord" }
      });

    s_ChunkedData.ChunkVertex->AddVertexBuffer(s_ChunkedData.ChunkBuffer);

    s_ChunkedData.ChunkBuffer->Unbind();

    s_Cube.CubeVertex = VertexArray::Create();

    s_Cube.CubeBuffer = VertexBuffer::Create(vertices, sizeof(vertices) * sizeof(float));
    s_Cube.CubeBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Pos" },
      { ShaderDataType::Float2, "a_TexCoord" }
      });

    s_Cube.CubeVertex->AddVertexBuffer(s_Cube.CubeBuffer);

    s_Cube.CubeBuffer->Unbind();

    s_Cube.CubeIndex = IndexBuffer::Create(cubeIndices, sizeof(cubeIndices));

    s_Cube.CubeVertex->SetIndexBuffer(s_Cube.CubeIndex);

    s_Cube.CubeIndex->Unbind();
    
    s_Cube.CubeMatrixBuffer = VertexBuffer::Create(sizeof(glm::mat4) * 100000);
    s_Cube.CubeMatrixBuffer->SetLayout({
      { ShaderDataType::Float4, "a_Transform" },
      { ShaderDataType::Float4, "a_Transform" },
      { ShaderDataType::Float4, "a_Transform" },
      { ShaderDataType::Float4, "a_Transform" },
      });

    s_Cube.CubeVertex->AddVertexBuffer(s_Cube.CubeMatrixBuffer);
    s_Cube.CubeVertex->AddVertexAttribDivisor(2, 1);
    s_Cube.CubeVertex->AddVertexAttribDivisor(3, 1);
    s_Cube.CubeVertex->AddVertexAttribDivisor(4, 1);
    s_Cube.CubeVertex->AddVertexAttribDivisor(5, 1);

    s_Cube.CubeMatrixBuffer->Unbind();

    s_Cube.CubeVertex->Unbind();

    s_Cube.CubeShader = Shader::Create("Cube", VOXELENGINE_DIR + "Assets/Shaders/Cube.vert",
                                               VOXELENGINE_DIR + "Assets/Shaders/Cube.frag");
    s_Cube.CubeShader->Unbind();

    s_Line.LineVertex = VertexArray::Create();

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
    
    s_Triangle.Shader = Shader::Create(s_Triangle.name, VOXELENGINE_DIR + "Assets/Shaders/Triangle.vert",
                                                        VOXELENGINE_DIR + "Assets/Shaders/Triangle.frag");

    s_Triangle.VertexArray = VertexArray::Create(s_Triangle.name);

    s_Triangle.VertexBuffer = VertexBuffer::Create(s_Triangle.name, triangle, sizeof(triangle));
    s_Triangle.VertexBuffer->SetLayout({ { ShaderDataType::Float3, "a_Pos" } });

    s_Triangle.UniformBuffer = UniformBuffer::Create(s_Triangle.name, 10000, 0);
    s_Triangle.Shader->AddUniformBuffer(s_Triangle.UniformBuffer);

    s_Triangle.VertexArray->AddVertexBuffer(s_Triangle.VertexBuffer);
    
    s_Triangle.VertexArray->Unbind();

    s_Triangle.VertexBuffer->Unbind();

    s_Triangle.Shader->Unbind();

    if (GetAPI() == RendererAPI::API::Vulkan) { VulkanContext::Get().Build(); }
	}

  void Renderer::Shutdown()
  {
  }

  void Renderer::OnWindowResize(const uint32_t width, const uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

  void Renderer::DrawChunk(const Chunk& chunk)
  {
    
    //algorithm

    s_ChunkedData.ChunkBuffer->Bind();
    s_ChunkedData.ChunkBuffer->SetData(chunk.GetVertices(),
                                       chunk.GetVerticesCount() * sizeof(float));

    s_ChunkedData.ChunkIndex->Bind();
    s_ChunkedData.ChunkIndex->SetData(chunk.GetIndices(),
                                      chunk.GetIndicesCount() * sizeof(uint32_t));
  }

	void Renderer::BeginScene(const Camera& camera)
	{
    s_SceneData->ViewMatrix = camera.GetViewMatrix();
    s_SceneData->ProjectionMatrix = camera.GetProjectionMatrix();
		s_SceneData->ProjectionViewMatrix = camera.GetProjectionViewMatrix();
	}

	void Renderer::EndScene()
	{
    s_Triangle.Shader->Unbind();
    s_Triangle.VertexArray->Unbind();
    s_Triangle.VertexBuffer->Unbind();

    /*s_Cube.CubeShader->Unbind();
    s_Cube.CubeVertex->Unbind();
    s_Cube.CubeBuffer->Unbind();
    s_Cube.CubeMatrixBuffer->Unbind();

    s_Line.LineShader->Unbind();
    s_Line.LineVertex->Unbind();
    s_Line.LineBuffer->Unbind();*/
	}

  void Renderer::DrawTriangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& size)
  {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    s_Triangle.Shader->Bind();
    s_Triangle.VertexArray->Bind();
    s_Triangle.VertexBuffer->Bind();

    s_SceneData->TransformMatrix = transform;

    s_Triangle.UniformBuffer->SetData(s_SceneData.get(), sizeof(*s_SceneData));
    
    RenderCommand::DrawArray(s_Triangle.VertexArray, 3);
  }

  void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& rotation,
                          const glm::vec3& size, const std::shared_ptr<Texture>& texture)
  {
    texture->Bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    s_Cube.CubeMatrixBuffer->Bind();
    s_Cube.CubeMatrixBuffer->SetData(&transform, sizeof(transform));

    s_Cube.CubeShader->Bind();
    s_Cube.CubeVertex->Bind();
    s_Cube.CubeBuffer->Bind();
    s_Cube.CubeIndex->Bind();

    //s_Cube.CubeShader->UploadUniformMat4("u_Projection", s_SceneData->ProjectionViewMatrix);
    s_Cube.CubeShader->UploadUniformMat4("u_Transform", transform);

    RenderCommand::DrawIndexed(s_Cube.CubeVertex, s_Cube.CubeIndex->GetCount());
  }

  void Renderer::DrawCubesInstanced(const std::vector<glm::mat3>& cubes, const std::shared_ptr<Texture>& texture)
  {
    texture->Bind();

    glm::mat4* transform = new glm::mat4[cubes.size()];
    for (int32_t i = 0; i < cubes.size(); ++i) {
      transform[i] = glm::translate(glm::mat4(1.0f), cubes[i][0]) * glm::scale(glm::mat4(1.0f), cubes[i][1]);
      transform[i] = glm::rotate(transform[i], glm::radians(cubes[i][2].x), glm::vec3(1.0f, 0.0f, 0.0f));
      transform[i] = glm::rotate(transform[i], glm::radians(cubes[i][2].y), glm::vec3(0.0f, 1.0f, 0.0f));
      transform[i] = glm::rotate(transform[i], glm::radians(cubes[i][2].z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    s_Cube.CubeShader->Bind();
    s_Cube.CubeVertex->Bind();
    s_Cube.CubeBuffer->Bind();
    s_Cube.CubeIndex->Bind();

    s_Cube.CubeMatrixBuffer->Bind();
    s_Cube.CubeMatrixBuffer->SetData(transform, cubes.size() * sizeof(glm::mat4));

    s_Cube.CubeShader->UploadUniformMat4("u_Projection", s_SceneData->ProjectionViewMatrix);

    RenderCommand::DrawIndexedInstanced(s_Cube.CubeVertex, s_Cube.CubeIndex->GetCount(), cubes.size());

    delete[] transform;
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

    s_Line.LineShader->UploadUniformMat4("u_Projection", s_SceneData->ProjectionViewMatrix);
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

    s_Skybox.SkyboxShader->UploadUniformMat4("u_ViewProjection", s_SceneData->ProjectionViewMatrix);

    RenderCommand::DrawArray(s_Skybox.SkyboxVertex, 36);

    RenderCommand::SetDepthMask(true);
  }
}