#include <glad/glad.h>

#include "vepch.h"

#include "Renderer.h"

#include "VertexArray.h"

#include <Platform/OpenGL/OpenGLShader.h>
#include <glm/ext/matrix_transform.hpp>

namespace VoxelEngine
{
  struct CubeData
  {
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoord;
  };

  struct Cube
  {
    std::shared_ptr<VertexArray> CubeVertex;
    std::shared_ptr<VertexBuffer> CubeBuffer;
    std::shared_ptr<IndexBuffer> CubeIndex;

    std::shared_ptr<Shader> CubeShader;
    std::shared_ptr<Shader> CubeTextureShader;
  };

	std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();

  static Cube s_Cube;

	void Renderer::Init()
	{
		RenderCommand::Init();

    const float vertices[8 * 3] = {
        -1, -1,  0.5, //0
         1, -1,  0.5, //1
        -1,  1,  0.5, //2
         1,  1,  0.5, //3
        -1, -1, -0.5, //4
         1, -1, -0.5, //5
        -1,  1, -0.5, //6
         1,  1, -0.5  //7
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

    s_Cube.CubeShader = Shader::Create("C:/Cube.glsl");
	}

  void Renderer::OnWindowResize(const uint32_t& width, const uint32_t& height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

	void Renderer::BeginScene(Camera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
    s_Cube.CubeShader->Unbind();
    s_Cube.CubeVertex->Unbind();
    s_Cube.CubeBuffer->Unbind();
    s_Cube.CubeIndex->Unbind();
	}

	void Renderer::Submit()
	{
		s_Cube.CubeShader->Bind();
    s_Cube.CubeVertex->Bind();
    s_Cube.CubeIndex->Bind();

		RenderCommand::DrawIndexed(s_Cube.CubeVertex, 36);
	}

	void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture)
	{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
      * glm::scale(glm::mat4(1.0f), size);

    Submit();
	}
}
