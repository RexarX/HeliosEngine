#pragma once

#include "vepch.h"

namespace VoxelEngine
{
	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() {}
		virtual void SetData(const void* data, const uint32_t size, const uint32_t offset = 0) = 0;

		static std::shared_ptr<UniformBuffer> Create(const uint32_t size, const uint32_t binding = 0);
	};
}