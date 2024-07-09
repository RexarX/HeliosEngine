#pragma once

#include "Core.h"

namespace VoxelEngine
{
	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F
	};

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
	};

	class VOXELENGINE_API Texture
	{
	public:
		virtual ~Texture() = default;

		virtual void Bind(const uint32_t slot = 0) const = 0;

		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual inline const uint32_t GetWidth() const = 0;
		virtual inline const uint32_t GetHeight() const = 0;

		virtual inline const std::string& GetPath() const = 0;

		virtual inline const uint32_t GetSlot() const = 0;

		virtual inline const bool IsLoaded() const = 0;

		virtual void SetData(const void* data, const uint32_t size) = 0;

		virtual inline const bool operator==(const Texture& other) const = 0;

		static std::shared_ptr<Texture> Create(const TextureSpecification& specification);
		static std::shared_ptr<Texture> Create(const std::string& path);

		static std::shared_ptr<Texture> CreateArray(const std::vector<std::string>& paths);
		static std::shared_ptr<Texture> Create(const std::string& right, const std::string& left,
																					 const std::string& top, const std::string& bottom,
																					 const std::string& front, const std::string& back);

		static void SetGenerateMipmaps(const bool value);
		static void SetAnisoLevel(const float value);
	};
}