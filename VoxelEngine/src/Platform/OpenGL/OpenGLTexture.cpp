#include "vepch.h"

#include "OpenGLTexture.h"

#include <stb_image.h>

namespace VoxelEngine
{
	static GLenum VoxelEngineImageFormatToGLDataFormat(const ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGB8:  return GL_RGB;
		case ImageFormat::RGBA8: return GL_RGBA;
		}

		VE_CORE_ASSERT(false, "Unknown ImageFormat!");
		return 0; 
	}

	static GLenum VoxelEngineImageFormatToGLInternalFormat(const ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGB8:  return GL_RGB8;
		case ImageFormat::RGBA8: return GL_RGBA8;
		}

		VE_CORE_ASSERT(false, "Unknown ImageFormat!");
		return 0;
	}

	OpenGLTexture::OpenGLTexture(const TextureSpecification& specification)
		: m_Specification(specification), m_Width(m_Specification.Width),
			m_Height(m_Specification.Height)
	{
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
	}

	OpenGLTexture::OpenGLTexture(const std::string& path)
		: m_Path(path)
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

			VE_CORE_ASSERT(m_InternalFormat & m_DataFormat, "Format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

			glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat,
									 GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat,
													GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}
	}

	OpenGLTexture::OpenGLTexture(const std::string& right, const std::string& left, const std::string& top,
															 const std::string& bottom, const std::string& front, const std::string& back)
		: m_Paths({ right, left, top, bottom, front, back })
	{
		glGenTextures(1, &m_RendererID);

		int32_t width, height, channels;
		stbi_uc* data = nullptr;
		for (uint32_t i = 0; i < 6; ++i) {
			data = stbi_load(m_Paths[i].c_str(), &width, &height, &channels, 0);
			if (data) {
				++m_LoadedCnt;

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

				VE_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");
			}
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_InternalFormat, m_Width, m_Height, 0,
				m_DataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}

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
		VE_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat,
												GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture::Bind(const uint32_t slot) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}
}