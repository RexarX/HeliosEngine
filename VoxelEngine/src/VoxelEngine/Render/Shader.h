#pragma once

#include "UniformBuffer.h"

#include <glm/glm.hpp>

namespace VoxelEngine
{
	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual inline const uint32_t GetID() const = 0;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddUniform(const void* data = nullptr, const uint32_t size = 128) = 0;

		virtual void UploadUniformData(const std::string& name, const void* data, const uint32_t size = 128) = 0;

		virtual void UploadUniformInt(const std::string& name, const int value) = 0;

		virtual void UploadUniformFloat(const std::string& name, const float value) = 0;
		virtual void UploadUniformFloat2(const std::string& name, const glm::vec2& value) = 0;
		virtual void UploadUniformFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void UploadUniformFloat4(const std::string& name, const glm::vec4& value) = 0;

		virtual void UploadUniformMat3(const std::string& name, const glm::mat3& matrix) = 0;
		virtual void UploadUniformMat4(const std::string& name, const glm::mat4& matrix) = 0;

		static std::shared_ptr<Shader> Create(const std::string& filepath);
		static std::shared_ptr<Shader> Create(const std::string& vertex, const std::string& fragment);
	};

	class ShaderLibrary
	{
	public:
		void Add(const uint32_t id, const std::shared_ptr<Shader>& shader);
		void Add(const std::shared_ptr<Shader>& shader);
		std::shared_ptr<Shader> Load(const uint32_t id, const std::string& filepath);

		std::shared_ptr<Shader> Get(const uint32_t id);

		bool Exists(const uint32_t id) const;

	private:
		std::unordered_map<uint32_t, std::shared_ptr<Shader>> m_Shaders;
	};
};