#include "VulkanVertexArray.h"

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "VulkanUniformBuffer.h"

namespace VoxelEngine
{
	static vk::Format ShaderDataTypeToVulkanBaseType(const ShaderDataType type)
	{
		switch (type)
		{
		case VoxelEngine::ShaderDataType::Float:    return vk::Format::eR32Sfloat;
		case VoxelEngine::ShaderDataType::Float2:   return vk::Format::eR32G32Sfloat;
		case VoxelEngine::ShaderDataType::Float3:   return vk::Format::eR32G32B32Sfloat;
		case VoxelEngine::ShaderDataType::Float4:   return vk::Format::eR32G32B32A32Sfloat;
		case VoxelEngine::ShaderDataType::Mat3:     return vk::Format::eR32G32B32Sfloat;
		case VoxelEngine::ShaderDataType::Mat4:     return vk::Format::eR32G32B32A32Sfloat;
		case VoxelEngine::ShaderDataType::Int:      return vk::Format::eR32Sint;
		case VoxelEngine::ShaderDataType::Int2:     return vk::Format::eR32G32Sint;
		case VoxelEngine::ShaderDataType::Int3:     return vk::Format::eR32G32B32Sint;
		case VoxelEngine::ShaderDataType::Int4:     return vk::Format::eR32G32B32A32Sint;
		case VoxelEngine::ShaderDataType::Bool:     return vk::Format::eR8Sint;
		}

		VE_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return vk::Format::eUndefined;
	}

	VulkanVertexArray::VulkanVertexArray(const char* name)
		: m_Name(name)
	{
		VulkanContext::Get().SetCurrentComputeEffect(name);
	}

	VulkanVertexArray::~VulkanVertexArray()
	{
	}

	void VulkanVertexArray::Bind() const
	{
		VulkanContext::Get().SetCurrentComputeEffect(m_Name);
	}

	void VulkanVertexArray::Unbind() const
	{
	}

	void VulkanVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		VulkanContext& context = VulkanContext::Get();

		std::shared_ptr<VulkanVertexBuffer> vulkanVertexBuffer = std::static_pointer_cast<VulkanVertexBuffer>(vertexBuffer);

		VE_CORE_ASSERT(m_Name == vulkanVertexBuffer->GetName(), "Buffer name does not match vertex array name!");

		void* data;
		vmaMapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation, &data);

		memcpy(data, vulkanVertexBuffer->GetVertices().data(), vulkanVertexBuffer->GetVertices().size() * sizeof(float));

		VE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");
		const BufferLayout& layout = vertexBuffer->GetLayout();

		vk::VertexInputBindingDescription bindingDescription;
		bindingDescription.binding = 0;
		bindingDescription.stride = layout.GetStride();
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		context.GetComputeEffect(m_Name).pipelineBuilder.vertexInputBindings.emplace_back(bindingDescription);

		for (const auto& element : layout) {
			vk::VertexInputAttributeDescription attributeDescription;
			attributeDescription.binding = 0;
			attributeDescription.location = m_VertexBufferIndex;
			attributeDescription.format = ShaderDataTypeToVulkanBaseType(element.type_);
			attributeDescription.offset = element.offset_;

			context.GetComputeEffect(m_Name).pipelineBuilder.vertexInputStates.emplace_back(attributeDescription);

      ++m_VertexBufferIndex;
		}

		if (vulkanVertexBuffer->GetVertices().size() != 0) {
			context.ImmediateSubmit([&](const vk::CommandBuffer cmd) {
				vk::BufferCopy vertexCopy;
				vertexCopy.dstOffset = 0;
				vertexCopy.srcOffset = 0;
				vertexCopy.size = vulkanVertexBuffer->GetVertices().size() * sizeof(float);

				cmd.copyBuffer(vulkanVertexBuffer->GetStagingBuffer().buffer, vulkanVertexBuffer->GetBuffer().buffer,
											 1, &vertexCopy);
				});
		}

		vmaUnmapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation);

		context.GetComputeEffect(m_Name).pipelineBuilder.vertexBuffer = vulkanVertexBuffer->GetBuffer().buffer;

		m_VertexBuffer = vertexBuffer;
	}

	void VulkanVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		VulkanContext& context = VulkanContext::Get();

		std::shared_ptr<VulkanIndexBuffer> vulkanIndexBuffer = std::static_pointer_cast<VulkanIndexBuffer>(indexBuffer);
		std::shared_ptr<VulkanVertexBuffer> vulkanVertexBuffer = std::static_pointer_cast<VulkanVertexBuffer>(m_VertexBuffer);

		VE_CORE_ASSERT(m_Name == vulkanIndexBuffer->GetName(), "Buffer name does not match index buffer name!");

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

				cmd.copyBuffer(vulkanIndexBuffer->GetStagingBuffer().buffer, vulkanIndexBuffer->GetBuffer().buffer,
											 1, &indexCopy);
				});
		}

		vmaUnmapMemory(context.GetAllocator(), vulkanVertexBuffer->GetStagingBuffer().allocation);

		m_IndexBuffer = indexBuffer;
	}

	void VulkanVertexArray::AddVertexAttribDivisor(const uint32_t index, const uint32_t divisor)
	{
	}
}