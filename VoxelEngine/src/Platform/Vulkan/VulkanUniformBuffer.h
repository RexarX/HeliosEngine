#pragma once

#include "VulkanStructs.h"

#include "Render/UniformBuffer.h"

namespace VoxelEngine
{
	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(const std::string& name, const uint32_t size, const uint32_t binding);
		virtual ~VulkanUniformBuffer();

		virtual void SetData(const void* data, const uint32_t size, const uint32_t offset) override;

		const void* GetData() const { return m_Buffer.info.pMappedData; }
		uint32_t GetSize() const { return m_Size; }

		const std::string&GetName() const { return m_Name; }

		AllocatedBuffer& GetBuffer() { return m_Buffer; }

	private:
		std::string m_Name;

		uint32_t m_Size = 0;

		AllocatedBuffer m_Buffer;
	};
}