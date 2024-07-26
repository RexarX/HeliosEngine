#include "Renderer/Texture.h"

#include <stb_image/stb_image.h>

namespace Helios
{
  Texture::Texture(const std::string& path, const uint32_t mipLevel, const uint32_t anisoLevel,
                   const ImageFormat format)
    : m_MipLevel(mipLevel), m_AnisoLevel(anisoLevel), m_Format(format)
  {
		int32_t width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* imData = nullptr;
		imData = stbi_load(path.c_str(), &width, &height, &channels, 4);

		if (imData == nullptr) { CORE_ERROR("Failed to load texture!"); }
		else { m_ImageData = std::make_unique<std::byte[]>(*imData); }

		m_Width = width;
		m_Height = height;

		if (channels == 4) { m_Format = ImageFormat::RGBA8; }
		else if (channels == 3) { m_Format = ImageFormat::RGB8; }
  }
}