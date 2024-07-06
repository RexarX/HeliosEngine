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
  
	std::unique_ptr<SceneData> Renderer::s_SceneData = std::make_unique<SceneData>();
  std::unordered_map<std::shared_ptr<Mesh>, MeshData> Renderer::m_Meshes;

  static Cube s_Cube;
  static Line s_Line;
  static Skybox s_Skybox;

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
    
    s_Cube.shader = Shader::Create(s_Cube.name, VOXELENGINE_DIR + "Assets/Shaders/Cube.vert",
                                                VOXELENGINE_DIR + "Assets/Shaders/Cube.frag");

    s_Cube.uniformBuffer = UniformBuffer::Create(s_Cube.name, sizeof(*s_SceneData));

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

    s_Cube.vertexBuffer->Unbind();

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

  const std::shared_ptr<Mesh>& Renderer::LoadModel(const std::string& path)
  {
    Mesh mesh;
    MeshData data;

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      std::filesystem::path extension = entry.path().extension();
      if (entry.is_regular_file()) {
        if (extension == ".obj") {
          CORE_INFO("Loading model: " + entry.path().string() + '!');
          if (!mesh.LoadObj(entry.path().string())) { CORE_ERROR("Failed to load model!"); return nullptr; }
          break;
        }
      }
    }

    std::filesystem::path pathToFile(path);
    data.name = pathToFile.filename().replace_extension("").string();

    std::vector<float>& vertices = mesh.GetVertices();
    std::vector<uint32_t>& indices = mesh.GetIndices();

    data.shader = Shader::Create(data.name, VOXELENGINE_DIR + "Assets/Shaders/Mesh.vert",
                                            VOXELENGINE_DIR + "Assets/Shaders/Mesh.frag");

    data.uniformBuffer = UniformBuffer::Create(data.name, sizeof(*s_SceneData));

    data.shader->AddUniformBuffer(data.uniformBuffer);

    data.vertexArray = VertexArray::Create(data.name);

    data.vertexBuffer = VertexBuffer::Create(data.name, vertices.data(), vertices.size() * sizeof(float));
    data.vertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Position" },
      { ShaderDataType::Float3, "a_Normal" },
      { ShaderDataType::Float2, "a_TexCoord" }
      });

    data.vertexArray->AddVertexBuffer(data.vertexBuffer);

    data.indexBuffer = IndexBuffer::Create(data.name, indices.data(), indices.size());

    data.vertexArray->SetIndexBuffer(data.indexBuffer);

    data.shader->Unbind();
    data.vertexArray->Unbind();
    data.vertexBuffer->Unbind();
    data.indexBuffer->Unbind();

    std::shared_ptr<Mesh> meshPtr = std::make_shared<Mesh>(mesh);

    m_Meshes.emplace(std::make_pair(meshPtr, data));

    for (const auto& entry : std::filesystem::directory_iterator(path + "/Textures")) {
      if (entry.is_regular_file()) {
        std::filesystem::path extension = entry.path().extension();
        if (extension == ".png" || extension == ".jpg") {
          CORE_INFO("Loading texture: " + entry.path().string() + '!');
          m_Meshes[meshPtr].textures.emplace_back(Texture::Create(data.name, entry.path().string()));
        }
      }
    }

    return meshPtr;
  }

	void Renderer::BeginScene(const Camera& camera)
	{
		s_SceneData->projectionViewMatrix = camera.GetProjectionViewMatrix();
	}

	void Renderer::EndScene()
	{
    if (s_Cube.shader != nullptr) { s_Cube.shader->Unbind(); }
    if (s_Cube.vertexArray != nullptr) { s_Cube.vertexArray->Unbind(); }
    if (s_Cube.vertexBuffer != nullptr) { s_Cube.vertexBuffer->Unbind(); }
    if (s_Cube.indexBuffer != nullptr) { s_Cube.indexBuffer->Unbind(); }

    for (auto& [mesh, meshData] : m_Meshes) {
      if (meshData.shader != nullptr) { meshData.shader->Unbind(); }
      if (meshData.vertexArray != nullptr) { meshData.vertexArray->Unbind(); }
      if (meshData.vertexBuffer != nullptr) { meshData.vertexBuffer->Unbind(); }
      if (meshData.indexBuffer != nullptr) { meshData.indexBuffer->Unbind(); }
    }

    //s_Line.LineShader->Unbind();
    //s_Line.LineVertex->Unbind();
    //s_Line.LineBuffer->Unbind();
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

    s_SceneData->transformMatrix = transform;

    s_Cube.uniformBuffer->SetData(s_SceneData.get(), sizeof(*s_SceneData));

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

    s_Line.LineShader->UploadUniformMat4("u_Projection", s_SceneData->projectionViewMatrix);
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

    s_Skybox.SkyboxShader->UploadUniformMat4("u_ViewProjection", s_SceneData->projectionViewMatrix);

    RenderCommand::DrawArray(s_Skybox.SkyboxVertex, 36);

    RenderCommand::SetDepthMask(true);
  }

  void Renderer::DrawMesh(const std::shared_ptr<Mesh>& mesh, const glm::vec3& position,
                          const glm::vec3& scale, const glm::vec3& rotation)
  {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), scale);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    MeshData& data = GetMeshData(mesh);

    for (auto& texture : data.textures) {
      texture->Bind();
    }

    data.shader->Bind();
    data.vertexArray->Bind();
    data.vertexBuffer->Bind();
    data.indexBuffer->Bind();

    s_SceneData->transformMatrix = transform;

    data.uniformBuffer->SetData(s_SceneData.get(), sizeof(*s_SceneData));

    RenderCommand::DrawIndexed(data.vertexArray, data.indexBuffer->GetCount());
  }
}