#include "VulkanVertexArray.h"

#include "vepch.h"

namespace VoxelEngine
{
	VulkanVertexArray::VulkanVertexArray()
	{
	}

	VulkanVertexArray::~VulkanVertexArray()
	{
	}

	void VulkanVertexArray::Bind() const
	{
	}

	void VulkanVertexArray::Unbind() const
	{
	}

	void VulkanVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		VE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");
		const auto& layout = vertexBuffer->GetLayout();
		for (const auto& element : layout) {
			++m_VertexBufferIndex;
		}

		m_VertexBuffers.push_back(vertexBuffer);
	}

	void VulkanVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		m_IndexBuffer = indexBuffer;
	}
	void VulkanVertexArray::AddVertexAttribDivisor(const uint32_t index, const uint32_t divisor)
	{
	}
}