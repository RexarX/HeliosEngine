#include "OpenGLTexture.h"

#include "vepch.h"

#include <stb_image/stb_image.h>

namespace VoxelEngine
{
  uint32_t OpenGLTexture::m_TextureSlotCounter = 0;

	bool OpenGLTexture::m_GenerateMipmaps = true;
	float OpenGLTexture::m_AnisoLevel = 16.0f;

	static GLenum VoxelEngineImageFormatToGLDataFormat(const ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGB8:  return GL_RGB;
		case ImageFormat::RGBA8: return GL_RGBA;
		}

		CORE_ASSERT(false, "Unknown ImageFormat!");
		return 0; 
	}

	static GLenum VoxelEngineImageFormatToGLInternalFormat(const ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGB8:  return GL_RGB8;
		case ImageFormat::RGBA8: return GL_RGBA8;
		}

		CORE_ASSERT(false, "Unknown ImageFormat!");
		return 0;
	}

	OpenGLTexture::OpenGLTexture(const TextureSpecification& specification)
		: m_Specification(specification), m_Width(m_Specification.Width),
			m_Height(m_Specification.Height), m_TextureSlot(m_TextureSlotCounter++)
	{
		m_IsLoaded = true;

		m_InternalFormat = VoxelEngineImageFormatToGLInternalFormat(m_Specification.Format);
		m_DataFormat = VoxelEngineImageFormatToGLDataFormat(m_Specification.Format);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
		glTexParameterfv(m_RendererID, GL_TEXTURE_BORDER_COLOR, borderColor);

		++m_TextureSlotCounter;
	}

	OpenGLTexture::OpenGLTexture(const std::string& path)
		: m_Path(path), m_TextureSlot(m_TextureSlotCounter++)
	{
		int32_t width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = nullptr;
		data = stbi_load(path.c_str(), &width, &height, &channels, 0);

		if (data) {
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4) {
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3) {
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			if (!(m_InternalFormat & m_DataFormat)) { CORE_ERROR("Format not supported!"); }

			glGenTextures(1, &m_RendererID);
			glBindTexture(GL_TEXTURE_2D, m_RendererID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			if (m_GenerateMipmaps) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			} else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat,
									 GL_UNSIGNED_BYTE, data);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, m_AnisoLevel);

			if (m_GenerateMipmaps) { glGenerateMipmap(GL_TEXTURE_2D); }

			stbi_image_free(data);
		}

		if (!m_IsLoaded) { CORE_ERROR("Failed to load texture!"); }
	}

	OpenGLTexture::OpenGLTexture(const std::vector<std::string>& paths)
	{
	}

	OpenGLTexture::OpenGLTexture(const std::string& right, const std::string& left, const std::string& top,
															 const std::string& bottom, const std::string& front, const std::string& back)
		: m_Paths({ right, left, top, bottom, front, back }), m_TextureSlot(m_TextureSlotCounter++)
	{
		glGenTextures(1, &m_RendererID);

		int32_t width, height, channels;
		stbi_uc* data = nullptr;

		uint32_t cnt = 0;

		for (uint32_t i = 0; i < 6; ++i) {
			data = stbi_load(m_Paths[i].c_str(), &width, &height, &channels, 0);
			if (data) {
				++cnt;

				m_Width = width;
				m_Height = height;

				GLenum internalFormat = 0, dataFormat = 0;
				if (channels == 4) {
					internalFormat = GL_RGBA8;
					dataFormat = GL_RGBA;
				} else if (channels == 3) {
					internalFormat = GL_RGB8;
					dataFormat = GL_RGB;
				}

				m_InternalFormat = internalFormat;
				m_DataFormat = dataFormat;

				if (!(m_InternalFormat & m_DataFormat)) { CORE_ERROR("Format not supported!"); }

				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_InternalFormat, m_Width, m_Height, 0,
										 m_DataFormat, GL_UNSIGNED_BYTE, data);

				stbi_image_free(data);
			}
		}

		if (cnt == 6) { m_IsLoaded = true; }
		else { CORE_ERROR("Failed to load cubemap!"); return; }

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture::SetData(const void* data, const uint32_t size)
	{
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		if (size != (m_Width * m_Height * bpp)) { CORE_ERROR("Data must be entire texture!"); }
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat,
												GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture::Bind(const uint32_t slot) const
	{
		glActiveTexture(GL_TEXTURE0 + m_TextureSlot);
		glBindTextureUnit(m_TextureSlot, m_RendererID);
	}
}