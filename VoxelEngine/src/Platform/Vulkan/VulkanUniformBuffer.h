#pragma once

#include "Render/UniformBuffer.h"

#include "VulkanStructs.h"

namespace VoxelEngine
{
	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(const uint32_t size, const uint32_t binding);
		virtual ~VulkanUniformBuffer();

		virtual void SetData(const void* data, const uint32_t size, const uint32_t offset) override;

		inline const void* GetData() const { return m_Buffer.info.pMappedData; }
		const inline uint32_t GetSize() const { return m_Size; }

		inline AllocatedBuffer& GetBuffer() { return m_Buffer; }

	private:
		uint32_t m_Size = 0;

		AllocatedBuffer m_Buffer;
	};
}