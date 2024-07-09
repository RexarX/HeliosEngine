#include "OpenGLShader.h"

#include "vepch.h"

#include <glad/glad.h>

#include <shaderc/shaderc.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace VoxelEngine 
{
	static GLenum ShaderTypeFromString(const std::string_view type)
	{
		if (type == "vertex") { return GL_VERTEX_SHADER; }
		else if (type == "fragment") { return GL_FRAGMENT_SHADER; }
		else if (type == "compute") { return GL_COMPUTE_SHADER; }

		CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	shaderc_shader_kind translateShaderStage(const GLenum stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER: return shaderc_vertex_shader;
		case GL_FRAGMENT_SHADER: return shaderc_fragment_shader;
		case GL_COMPUTE_SHADER: return shaderc_compute_shader;
		}

		CORE_ASSERT(false, "Unknown shader stage!");
	}

	bool OpenGLShader::GLSLtoSPV(const GLenum shaderType, const std::string& glslShader,
															 const std::string& fileName, std::vector<uint32_t>& spvShader) const
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
			glslShader, translateShaderStage(shaderType),
			fileName.c_str(),
			options
		);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			CORE_ERROR(module.GetErrorMessage());
			return false;
		}

		spvShader = { module.cbegin(), module.cend() };

		return true;
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);
		Compile(shaderSources);

		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
	}

	OpenGLShader::OpenGLShader(const std::string& vertex, const std::string& fragment)
	{
		std::string vertexelement = ReadFile(vertex);
		std::string fragmentelement = ReadFile(fragment);

		std::unordered_map<GLenum, std::pair<std::string, std::string>> sources;

		sources[GL_VERTEX_SHADER].first = vertex;
    sources[GL_FRAGMENT_SHADER].first = fragment;
		sources[GL_VERTEX_SHADER].second = vertexelement;
		sources[GL_FRAGMENT_SHADER].second = fragmentelement;

		if (vertex.rfind(".spv") != std::string::npos || fragment.rfind(".spv") != std::string::npos) {
			m_Compiled = true;
		}

		Compile(sources);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	std::string OpenGLShader::ReadFile(const std::string& filepath)
	{
		INFO("Reading file '{0}'", filepath);
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in) {
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(result.data(), result.size());
			in.close();
		} 
		else { CORE_ERROR("Could not open file '{0}'", filepath); }
		return result;
	}

	std::unordered_map<GLenum, std::pair<std::string, std::string>> OpenGLShader::PreProcess(const std::string& source)
	{
		std::unordered_map<GLenum, std::pair<std::string, std::string>> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos) {
			size_t eol = source.find_first_of("\r\n", pos);
			CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)].second = source.substr(nextLinePos,
				pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void OpenGLShader::Compile(const std::unordered_map<GLenum, std::pair<std::string, std::string>>& shaderSources)
	{
		CORE_ASSERT(shaderSources.size() <= 2, "We only support 2 shaders for now");

		std::vector<uint32_t> spirv;

		GLuint program = glCreateProgram();
		
		std::array<GLenum, 2> glShaderIDs;
		int glShaderIDIndex = 0;
		for (const auto& element : shaderSources) {
			GLenum type = element.first;
			const std::string& source = element.second.second;

			GLuint shader = glCreateShader(type);
			if (m_Compiled) {
				spirv.resize(source.size() / sizeof(uint32_t));
				memcpy(spirv.data(), source.data(), source.size());
			} else {
				bool result = GLSLtoSPV(type, source, element.second.first, spirv);
				CORE_ASSERT(result, "Failed to compile shader!");
			}

			glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size() * sizeof(uint32_t));
			glSpecializeShader(shader, "main", 0, nullptr, nullptr);

			glAttachShader(program, shader);
			glShaderIDs[glShaderIDIndex++] = shader;
		}

		m_RendererID = program;

		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

			glDeleteProgram(program);

			for (auto id : glShaderIDs) {
				glDeleteShader(id);
			}

			CORE_ERROR("{0}", infoLog.data());
			CORE_ASSERT(false, "Shader link failure!");
			return;
		}

		for (auto id : glShaderIDs) {
			glDetachShader(program, id);
		}
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::AddUniform(const void* data, const uint32_t size)
	{
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, const int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, const float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformData(const std::string& name, const void* data, const uint32_t size)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, *static_cast<const GLint*>(data));
	}
}