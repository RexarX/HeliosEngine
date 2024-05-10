#include "VulkanTexture.h"

#include "vepch.h"

#include <stb_image.h>

namespace VoxelEngine
{
	bool VulkanTexture::m_GenerateMipmaps = false;
	float VulkanTexture::m_AnisoLevel = 0.0f;

	VulkanTexture::VulkanTexture(const TextureSpecification& specification)
		: m_Specification(specification), m_Width(m_Specification.Width),
			m_Height(m_Specification.Height)
	{
	}

	VulkanTexture::VulkanTexture(const std::string& path)
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

			if (m_GenerateMipmaps) {
				
			}
			else {
				
			}

			stbi_image_free(data);
		}
	}

	VulkanTexture::VulkanTexture(const std::string& right, const std::string& left, const std::string& top,
															 const std::string& bottom, const std::string& front, const std::string& back)
		: m_Paths({ right, left, top, bottom, front, back })
	{
		int32_t width, height, channels;
		stbi_uc* data = nullptr;
		for (uint32_t i = 0; i < 6; ++i) {
			data = stbi_load(m_Paths[i].c_str(), &width, &height, &channels, 0);
			if (data) {
				++m_LoadedCnt;

				m_Width = width;
				m_Height = height;

			}

			if (m_LoadedCnt == 6) { m_IsLoaded = true; }

			stbi_image_free(data);
		}
	}

	VulkanTexture::~VulkanTexture()
	{
	}

	void VulkanTexture::SetData(const void* data, const uint32_t size)
	{
	}

	void VulkanTexture::Bind(const uint32_t slot) const
	{
	}
}