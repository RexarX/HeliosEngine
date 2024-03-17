#include "vepch.h"

#include "Renderer.h"
#include <glad/glad.h>

#include "Platform/OpenGL/OpenGLShader.h"

namespace VoxelEngine
{
  static float vertices[180] = {
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

  OpenGLShader cubeShader("assets/shaders/Cube.glsl");

	struct Cube 
	{
    
	};

	std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();

	void Renderer::Init()
	{
		RenderCommand::Init();
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
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

	void Renderer::DrawCube(const glm::vec3& transform, const glm::vec3& rotation, const std::shared_ptr<Texture>& texture) 
	{
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //verices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //vertex texture
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    texture.get()->Bind();

    cubeShader.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}
