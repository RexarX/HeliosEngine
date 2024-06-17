#pragma once

#include "UniformBuffer.h"

#include <glm/glm.hpp>

namespace VoxelEngine
{
	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddUniformBuffer(const std::shared_ptr<UniformBuffer>& uniformBuffer) = 0;

		virtual void UploadUniformInt(const char* name, const int value) = 0;

		virtual void UploadUniformFloat(const char* name, const float value) = 0;
		virtual void UploadUniformFloat2(const char* name, const glm::vec2& value) = 0;
		virtual void UploadUniformFloat3(const char* name, const glm::vec3& value) = 0;
		virtual void UploadUniformFloat4(const char* name, const glm::vec4& value) = 0;

		virtual void UploadUniformMat3(const char* name, const glm::mat3& matrix) = 0;
		virtual void UploadUniformMat4(const char* name, const glm::mat4& matrix) = 0;

		virtual void UploadUniformData(const char* name, const void* data, const uint32_t size) = 0;

		virtual const char* GetName() const = 0;

		static std::shared_ptr<Shader> Create(const std::string& filepath);
		static std::shared_ptr<Shader> Create(const char* name, const std::string& vertex,
																														const std::string& fragment);
	};

	class ShaderLibrary
	{
	public:
		void Add(const char* name, const std::shared_ptr<Shader>& shader);
		void Add(const std::shared_ptr<Shader>& shader);
		std::shared_ptr<Shader> Load(const std::string& filepath);
		std::shared_ptr<Shader> Load(const char* name, const std::string& filepath);

		std::shared_ptr<Shader> Get(const char* name);

		bool Exists(const char* name) const;

	private:
		std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
	};
};