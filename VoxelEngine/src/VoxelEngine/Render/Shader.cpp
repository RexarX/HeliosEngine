#include "vepch.h"

#include "Shader.h"

#include <glad/glad.h>

namespace VoxelEngine
{
  Shader::Shader(const std::string& vertex, const std::string& fragment)
  {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* src = vertex.c_str();
    glCompileShader(vertexShader);

    GLint isCompiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
      GLint maxLength = 0;
      glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

      glDeleteShader(vertexShader);

      VE_CORE_ASSERT("{0}", infoLog.data());
      VE_CORE_ASSERT(false, "Vertex shader compilation failed!");
      return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    src = fragment.c_str();
    glCompileShader(fragmentShader);

    isCompiled = 0;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
      GLint maxLength = 0;
      glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

      glDeleteShader(fragmentShader);

      VE_CORE_ASSERT("{0}", infoLog.data());
      VE_CORE_ASSERT(false, "Fragment shader compilation failed!");
      return;
    }

    m_RendererID = glCreateProgram();
    GLuint program = m_RendererID;
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
    if (isLinked == GL_FALSE) {
      GLint maxLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

      glDeleteProgram(program);
      glDeleteShader(vertexShader);
      glDeleteShader(fragmentShader);

      VE_CORE_ERROR("{0}", infoLog.data());
      VE_CORE_ASSERT(false, "Shader link failure!");
      return;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
  }

  Shader::~Shader()
  {
    glDeleteProgram(m_RendererID);
  }

  void Shader::Bind() const
  {
    glUseProgram(m_RendererID);
  }

  void Shader::UnBind() const
  {
    glUseProgram(0);
  }
};