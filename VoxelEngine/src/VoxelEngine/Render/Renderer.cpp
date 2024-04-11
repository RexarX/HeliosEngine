#include "vepch.h"

#include "Renderer.h"

#include "VertexArray.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VoxelEngine
{
  struct Cube
  {
    std::shared_ptr<VertexArray> CubeVertex;
    std::shared_ptr<VertexBuffer> CubeBuffer;
    std::shared_ptr<IndexBuffer> CubeIndex;

    std::shared_ptr<Shader> CubeShader;
  };

	std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();

  static Cube s_Cube;

	void Renderer::Init()
	{
		RenderCommand::Init();
    
    const float vertices[] = {
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

    s_Cube.CubeVertex = VertexArray::Create();
    s_Cube.CubeVertex->Bind();

    s_Cube.CubeBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
    s_Cube.CubeBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Pos" },
      { ShaderDataType::Float3, "a_Normal" },
      { ShaderDataType::Float2, "a_TexCoord" }
      });

    s_Cube.CubeVertex->AddVertexBuffer(s_Cube.CubeBuffer);
    
    s_Cube.CubeBuffer->Unbind();
    s_Cube.CubeVertex->Unbind();

    s_Cube.CubeShader = Shader::Create(ROOT + "VoxelEngine/Assets/Shaders/Cube.glsl");
	}

  void Renderer::OnWindowResize(const uint32_t width, const uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

	void Renderer::BeginScene(const Camera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    s_SceneData->ProjectionMatrix = camera.GetProjectionMatrix();
    s_SceneData->ViewMatrix = camera.GetViewMatrix();
	}

	void Renderer::EndScene()
	{
    s_Cube.CubeShader->Unbind();
    s_Cube.CubeVertex->Unbind();
    s_Cube.CubeBuffer->Unbind();
	}

	void Renderer::Submit(glm::mat4& transform)
	{
		s_Cube.CubeShader->Bind();
    s_Cube.CubeVertex->Bind();

    s_Cube.CubeShader->UploadUniformMat4("u_Projection", s_SceneData->ProjectionMatrix);
    s_Cube.CubeShader->UploadUniformMat4("u_View", s_SceneData->ViewMatrix);
    s_Cube.CubeShader->UploadUniformMat4("u_Transform", transform);

		RenderCommand::DrawArray(s_Cube.CubeVertex, 36);
	}

  void Renderer::Shutdown()
  {
  }

	void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& rotation,
    const glm::vec3& size, const std::shared_ptr<Texture>& texture)
	{
    texture->Bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    Submit(transform);
	}
}