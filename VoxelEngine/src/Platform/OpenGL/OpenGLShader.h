#pragma once

#include "Render/Shader.h"

#include <glm/glm.hpp>

typedef unsigned int GLenum;

namespace VoxelEngine
{
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const char* name, const std::string& vertex,
																	 const std::string& fragment);
		virtual ~OpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual const char* GetName() const override { return m_Name; }

		virtual void AddUniformBuffer(const std::shared_ptr<UniformBuffer>& uniformBuffer) override;

		virtual void UploadUniformInt(const char* name, const int value) override;

		virtual void UploadUniformFloat(const char* name, const float value) override;
		virtual void UploadUniformFloat2(const char* name, const glm::vec2& value) override;
		virtual void UploadUniformFloat3(const char* name, const glm::vec3& value) override;
		virtual void UploadUniformFloat4(const char* name, const glm::vec4& value) override;

		virtual void UploadUniformMat3(const char* name, const glm::mat3& matrix) override;
		virtual void UploadUniformMat4(const char* name, const glm::mat4& matrix) override;

		virtual void UploadUniformData(const char* name, const void* data, const uint32_t size) override;

	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);

		bool GLSLtoSPV(const GLenum shaderType, const std::string& glslShader,
									 std::vector<uint32_t>& spvShader) const;

	private:
		uint32_t m_RendererID;
		const char* m_Name;

		bool m_Compiled = false;
	};
}