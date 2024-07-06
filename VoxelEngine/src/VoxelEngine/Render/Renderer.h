#pragma once

#include "RenderStart.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"

namespace VoxelEngine
{
	struct MeshData
	{
		std::string name;

		std::shared_ptr<Shader> shader;

		std::shared_ptr<VertexArray> vertexArray;
		std::shared_ptr<VertexBuffer> vertexBuffer;
		std::shared_ptr<IndexBuffer> indexBuffer;
		std::shared_ptr<UniformBuffer> uniformBuffer;

		std::vector<std::shared_ptr<Texture>> textures;
	};

	struct SceneData
	{
		glm::mat4 projectionViewMatrix;
		glm::mat4 transformMatrix;
	};

	class VOXELENGINE_API Renderer
	{
	public:
		static void Init();

		static void Shutdown();

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void OnWindowResize(const uint32_t width, const uint32_t height);

		static void SetGenerateMipmaps(const bool value) { Texture::SetGenerateMipmaps(value); }
		static void SetAnisoLevel(const float value) { Texture::SetAnisoLevel(value); }

		static const std::shared_ptr<Mesh>& LoadModel(const std::string& path);

		static void BeginScene(const Camera& camera);
		static void EndScene();

		static void DrawTriangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& size);

		static void DrawCube(const glm::vec3& position, const glm::vec3& size = glm::vec3(1.0f),
												 const glm::vec3& rotation = glm::vec3(0.0f),
												 const std::shared_ptr<Texture>& texture = nullptr);

		static void DrawLine(const glm::vec3& position, const glm::vec3& rotation, const float lenght);

		static void DrawSkybox(const std::shared_ptr<Texture>& texture);

		static void DrawMesh(const std::shared_ptr<Mesh>& mesh, const glm::vec3& position = glm::vec3(0.0f),
												 const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& rotation = glm::vec3(0.0f));

	private:
		static inline MeshData& GetMeshData(const std::shared_ptr<Mesh>& mesh) { return m_Meshes[mesh]; }

	private:
		static std::unique_ptr<SceneData> s_SceneData;

		static std::unordered_map<std::shared_ptr<Mesh>, MeshData> m_Meshes;
	};
}