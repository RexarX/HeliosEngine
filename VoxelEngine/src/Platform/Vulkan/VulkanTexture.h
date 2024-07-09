#pragma once

#include "Render/Texture.h"

#include "VulkanStructs.h"

#include <vulkan/vulkan.hpp>

#include <vma/vk_mem_alloc.h>

namespace VoxelEngine
{
	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(const TextureSpecification& specification);
		VulkanTexture(const std::string& path);
		VulkanTexture(const std::vector<std::string>& paths);
		VulkanTexture(const std::string& right, const std::string& left,
									const std::string& top, const std::string& bottom,
									const std::string& front, const std::string& back);
		virtual ~VulkanTexture();

		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		virtual inline const uint32_t GetWidth() const override { return m_Width; }
		virtual inline const uint32_t GetHeight() const override { return m_Height; }

		virtual inline const std::string& GetPath() const override { return m_Path; }

		virtual inline const uint32_t GetSlot() const override { return m_TextureSlot; }

		virtual inline const bool IsLoaded() const override { return m_IsLoaded; }

		virtual void SetData(const void* data, const uint32_t size) override;

		virtual void Bind(const uint32_t slot = 0) const override;

		virtual inline const bool operator==(const Texture& other) const override { return true; }

		inline const AllocatedImage& GetImage() const { return m_Image; }

		static void SetGenerateMipmaps(const bool value) { m_GenerateMipmaps = value; }
		static void SetAnisoLevel(const float value) { m_AnisoLevel = value; }

	private:
		AllocatedImage create_image(const vk::Extent3D size, const vk::Format format,
																const vk::ImageUsageFlags usage) const;
		AllocatedImage create_image(const void* data, const vk::Extent3D size, const vk::Format format,
																const vk::ImageUsageFlags usage) const;

	private:
		TextureSpecification m_Specification;

		uint32_t m_Width, m_Height;
		vk::Format m_DataFormat;

		uint32_t m_TextureSlot;

		static bool m_GenerateMipmaps;
		static float m_AnisoLevel;

		bool m_IsLoaded = false;

		std::string m_Path;
		std::array<std::string, 6> m_Paths;

		AllocatedImage m_Image;
	};
}