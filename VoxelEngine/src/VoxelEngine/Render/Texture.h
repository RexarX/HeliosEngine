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

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual void Bind(const uint32_t slot = 0) const = 0;

		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual uint32_t GetRendererID() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual void SetData(const void* data, const uint32_t size) = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;

		static std::shared_ptr<Texture> Create(const TextureSpecification& specification);
		static std::shared_ptr<Texture> Create(const std::string& path, const bool generateMips = false,
																					 const float anisoLevel = 0.0f);
		static std::shared_ptr<Texture> Create(const std::string& right, const std::string& left,
																					 const std::string& top, const std::string& bottom,
																					 const std::string& front, const std::string& back);
	};
}