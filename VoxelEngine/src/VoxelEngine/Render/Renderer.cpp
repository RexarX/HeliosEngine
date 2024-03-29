#include <glad/glad.h>

#include "vepch.h"

#include "Renderer.h"

#include "VertexArray.h"

#include "Application.h"

#include <Platform/OpenGL/OpenGLShader.h>

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
    std::shared_ptr<Texture> CubeTexture;
  };

	std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();

  static Cube s_Cube;

	void Renderer::Init()
	{
		RenderCommand::Init();

    const float vertices[8 * 3] = {
        -0.5, -0.5,  0.5, //0
         0.5, -0.5,  0.5, //0.5
        -0.5,  0.5,  0.5, //2
         0.5,  0.5,  0.5, //3
        -0.5, -0.5, -0.5, //4
         0.5, -0.5, -0.5, //5
        -0.5,  0.5, -0.5, //6
         0.5,  0.5, -0.5  //7
    };

    const uint32_t CubeIndices[6 * 6] = {
        //Top
        2, 6, 7,
        2, 3, 7,

        //Bottom
        0, 4, 5,
        0, 1, 5,

        //Left
        0, 2, 6,
        0, 4, 6,

        //Right
        1, 3, 7,
        1, 5, 7,

        //Front
        0, 2, 3,
        0, 1, 3,

        //Back
        4, 6, 7,
        4, 5, 7
    };

    s_Cube.CubeVertex = VertexArray::Create();
    s_Cube.CubeVertex->Bind();

    s_Cube.CubeBuffer = VertexBuffer::Create(vertices, 36 * sizeof(float));
    s_Cube.CubeBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Pos" },
      { ShaderDataType::Float2, "a_TexCoord" }
     });

    s_Cube.CubeIndex = IndexBuffer::Create(CubeIndices, 36);
    s_Cube.CubeVertex->SetIndexBuffer(s_Cube.CubeIndex);


    s_Cube.CubeVertex->AddVertexBuffer(s_Cube.CubeBuffer);
    
    s_Cube.CubeBuffer->Unbind();
    s_Cube.CubeVertex->Unbind();

    s_Cube.CubeShader = Shader::Create(ROOT + "VoxelEngine/Assets/Shaders/Cube.glsl");
	}

  void Renderer::OnWindowResize(const uint32_t width, const uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

	void Renderer::BeginScene(Camera& camera)
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
    s_Cube.CubeIndex->Unbind();
	}

	void Renderer::Submit(glm::mat4& transform)
	{
		s_Cube.CubeShader->Bind();
    s_Cube.CubeVertex->Bind();
    s_Cube.CubeIndex->Bind();

    s_Cube.CubeShader->UploadUniformMat4("u_Projection", s_SceneData->ProjectionMatrix);
    s_Cube.CubeShader->UploadUniformMat4("u_View", s_SceneData->ViewMatrix);
    s_Cube.CubeShader->UploadUniformMat4("u_Transform", transform);

		RenderCommand::DrawIndexed(s_Cube.CubeVertex, 36);
	}

	void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& size, const std::shared_ptr<Texture>& texture)
	{
    texture->Bind();
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    Submit(transform);
	}
}