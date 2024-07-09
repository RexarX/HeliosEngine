#pragma once

#include "Render/Texture.h"

#include <glad/glad.h>

namespace VoxelEngine
{
	class OpenGLTexture : public Texture
	{
	public:
		OpenGLTexture(const TextureSpecification& specification);
		OpenGLTexture(const std::string& path);
		OpenGLTexture(const std::vector<std::string>& paths);
		OpenGLTexture(const std::string& right, const std::string& left,
									const std::string& top, const std::string& bottom,
									const std::string& front, const std::string& back);
		virtual ~OpenGLTexture();

		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		virtual inline const uint32_t GetWidth() const override { return m_Width; }
		virtual inline const uint32_t GetHeight() const override { return m_Height; }

		virtual inline const std::string& GetPath() const override { return m_Path; }

		virtual inline const bool IsLoaded() const override { return m_IsLoaded; }

		virtual inline const uint32_t GetSlot() const override { return m_TextureSlot; }
				
		virtual void SetData(const void* data, const uint32_t size) override;

		virtual void Bind(const uint32_t slot = 0) const override;

		virtual inline const bool operator==(const Texture& other) const override { return true; }

		const inline uint32_t GetRendererID() const { return m_RendererID; }

		static void SetGenerateMipmaps(const bool value) { m_GenerateMipmaps = value; }
		static void SetAnisoLevel(const float value) { m_AnisoLevel = value; }

	private:
		TextureSpecification m_Specification;
		
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;

		uint32_t m_TextureSlot;
		static uint32_t	m_TextureSlotCounter;

		static bool m_GenerateMipmaps;
		static float m_AnisoLevel;

		bool m_IsLoaded = false;

		std::string m_Path;
		std::array<std::string, 6> m_Paths;
	};
}