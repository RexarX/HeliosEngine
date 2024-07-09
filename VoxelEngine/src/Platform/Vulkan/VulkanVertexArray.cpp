#include "VulkanVertexArray.h"
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "VulkanUniformBuffer.h"

namespace VoxelEngine
{
	void VulkanVertexArray::Bind() const
	{
	}

	void VulkanVertexArray::Unbind() const
	{
	}

	void VulkanVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		VulkanContext& context = VulkanContext::Get();

		std::shared_ptr<VulkanVertexBuffer> vulkanVertexBuffer = std::static_pointer_cast<VulkanVertexBuffer>(vertexBuffer);

		void* data;
		vmaMapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation, &data);

		memcpy(data, vulkanVertexBuffer->GetVertices().data(), vulkanVertexBuffer->GetVertices().size() * sizeof(float));

		CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

		if (vulkanVertexBuffer->GetVertices().size() != 0) {
			context.ImmediateSubmit([&](const vk::CommandBuffer cmd) {
				vk::BufferCopy vertexCopy;
				vertexCopy.dstOffset = 0;
				vertexCopy.srcOffset = 0;
				vertexCopy.size = vulkanVertexBuffer->GetVertices().size() * sizeof(float);

				cmd.copyBuffer(static_cast<vk::Buffer>(vulkanVertexBuffer->GetStagingBuffer().buffer),
											 static_cast<vk::Buffer>(vulkanVertexBuffer->GetBuffer().buffer),
											 1, &vertexCopy);
				});
		}

		context.GetDeletionQueue().push_function([&]() {
			vmaUnmapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation);
			});

		m_VertexBuffer = vulkanVertexBuffer;
	}

	void VulkanVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		VulkanContext& context = VulkanContext::Get();

		std::shared_ptr<VulkanIndexBuffer> vulkanIndexBuffer = std::static_pointer_cast<VulkanIndexBuffer>(indexBuffer);
		std::shared_ptr<VulkanVertexBuffer> vulkanVertexBuffer = std::static_pointer_cast<VulkanVertexBuffer>(m_VertexBuffer);

		if (indexBuffer != nullptr) {
			void* data;
			vmaMapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation, &data);

			memcpy((int8_t*)data + vulkanVertexBuffer->GetVertices().size() * sizeof(float),
						 vulkanIndexBuffer->GetIndices().data(), indexBuffer->GetCount() * sizeof(uint32_t));
		}

		if (indexBuffer->GetCount() != 0) {
			context.ImmediateSubmit([&](const vk::CommandBuffer cmd) {
				vk::BufferCopy indexCopy;
				indexCopy.dstOffset = 0;
				indexCopy.srcOffset = 0;
				indexCopy.size = indexBuffer->GetCount() * sizeof(uint32_t);

				cmd.copyBuffer(static_cast<vk::Buffer>(vulkanIndexBuffer->GetStagingBuffer().buffer),
											 static_cast<vk::Buffer>(vulkanIndexBuffer->GetBuffer().buffer),
											 1, &indexCopy);
				});
		}

		context.GetDeletionQueue().push_function([&]() {
			vmaUnmapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation);
    });

		m_IndexBuffer = vulkanIndexBuffer;
	}

	void VulkanVertexArray::AddVertexAttribDivisor(const uint32_t index, const uint32_t divisor)
	{
	}
}