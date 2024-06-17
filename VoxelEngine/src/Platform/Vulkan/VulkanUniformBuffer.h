#pragma once

#include "Render/UniformBuffer.h"

#include "VulkanStructs.h"

namespace VoxelEngine
{
	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(const char* name, const uint32_t size, const uint32_t binding);
		virtual ~VulkanUniformBuffer();

		virtual void SetData(const void* data, const uint32_t size, const uint32_t offset) override;

		const void* GetData() const { return m_Data; }
		uint32_t GetSize() const { return m_Size; }
		uint32_t GetOffset() const { return m_Offset; }

		const char* GetName() const { return m_Name; }

		AllocatedBuffer& GetBuffer() { return m_Buffer; }

	private:
		const void* m_Data;

		const char* m_Name;

		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;

		AllocatedBuffer m_Buffer;
	};
}