#include "vepch.h"

#include "Renderer.h"

#include "VertexArray.h"

#include <Platform/OpenGL/OpenGLShader.h>

namespace VoxelEngine
{
	struct Cube
	{
    std::shared_ptr<VertexArray> CubeVertex;
    std::shared_ptr<Shader> CubeShader;
    std::shared_ptr<Shader> CubeTextureShader;
	};

	std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();

  static std::unique_ptr<Cube> s_Cube;

	void Renderer::Init()
	{
		RenderCommand::Init();

    const float vertices[180] = {
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left front 
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom left front 
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom right front 
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left front 
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, // top right front
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom right front 

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top left back
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // bottom left back 
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom right back 
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top left back 
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top right back
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom right back

    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left right
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom left right
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom right right
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left right
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top right right
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom right right

    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left left
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom left left
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom right left
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left left
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top right left
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom right left

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top left up
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, // bottom left up
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // bottom right up
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top left up
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top right up
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // bottom right up

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // top left bottom
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom left bottom
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom right bottom
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // top left bottom
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, // top right bottom
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom right bottom
    };

    uint32_t CubeIndices[6 * 6] = {
        0, 1, 3, 3, 1, 2,
        1, 5, 2, 2, 5, 6,
        5, 4, 6, 6, 4, 7,
        4, 0, 7, 7, 0, 3,
        3, 2, 7, 7, 2, 6,
        4, 5, 0, 0, 5, 1
    };

    s_Cube = std::make_unique<Cube>();

    s_Cube->CubeVertex = VertexArray::Create();

    std::shared_ptr<VertexBuffer> CubeVB = VertexBuffer::Create(vertices, sizeof(vertices));
    CubeVB->SetLayout({
      { ShaderDataType::Float3, "a_Pos" },
      { ShaderDataType::Float2, "a_TexCoord" }
     });

    s_Cube->CubeVertex->AddVertexBuffer(CubeVB);

    //std::shared_ptr<IndexBuffer> CubeIB = IndexBuffer::Create(CubeIndices, sizeof(CubeIndices) / sizeof(uint32_t));
    //s_Cube->CubeVertex->SetIndexBuffer(CubeIB);

    s_Cube->CubeShader = Shader::Create("C:/Cube.glsl");
    //s_Cube->CubeShader->Bind();
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

	}

	void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		shader->Bind();
		//std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		//std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);

		RenderCommand::DrawIndexed(vertexArray);
	}

	void Renderer::DrawCube(const glm::vec3& transform, const glm::vec3& rotation, const std::shared_ptr<Texture>& texture) 
	{
    texture->Bind();
    Submit(s_Cube->CubeShader, s_Cube->CubeVertex);
	}
}
