#include "VulkanVertexArray.h"
#include "VulkanContext.h"
#include "VulkanBuffer.h"

#include "vepch.h"

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000
#include <vma/vk_mem_alloc.h>

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
		std::shared_ptr<VulkanVertexBuffer> vulkanVertexBuffer = std::dynamic_pointer_cast<VulkanVertexBuffer>(vertexBuffer);

		void* data = vulkanVertexBuffer->GetStagingBuffer().allocation->GetMappedData();

		memcpy(data, vulkanVertexBuffer->GetVertices().data(),
					 vulkanVertexBuffer->GetVertices().size() * sizeof(float));

		VE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");
		const BufferLayout& layout = vertexBuffer->GetLayout();

		vk::VertexInputBindingDescription bindingDescription;
		bindingDescription.binding = m_Binding;
		bindingDescription.stride = layout.GetStride();
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		VulkanContext::Get().GetPipelineData().vertexInputBindings.emplace_back(bindingDescription);

		for (const auto& element : layout) {
			vk::VertexInputAttributeDescription attributeDescription;
			attributeDescription.binding = m_Binding;
			attributeDescription.location = m_VertexBufferIndex;
			attributeDescription.format = ShaderDataTypeToVulkanBaseType(element.type_);
			attributeDescription.offset = element.offset_;

			VulkanContext::Get().GetPipelineData().vertexInputStates.emplace_back(attributeDescription);

      ++m_VertexBufferIndex;
		}

		if (vulkanVertexBuffer->GetVertices().size() != 0) {
			VulkanContext::Get().ImmediateSubmit([&](vk::CommandBuffer cmd) {
				vk::BufferCopy vertexCopy;
				vertexCopy.dstOffset = 0;
				vertexCopy.srcOffset = 0;
				vertexCopy.size = vulkanVertexBuffer->GetVertices().size() * sizeof(float);

				cmd.copyBuffer(vulkanVertexBuffer->GetStagingBuffer().buffer, vulkanVertexBuffer->GetBuffer().buffer,
					1, &vertexCopy);
				});
		}

		m_VertexBuffers.push_back(vertexBuffer);
		++m_Binding;
	}

	void VulkanVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		std::shared_ptr<VulkanIndexBuffer> vulkanIndexBuffer = std::dynamic_pointer_cast<VulkanIndexBuffer>(indexBuffer);
		std::shared_ptr<VulkanVertexBuffer> vulkanVertexBuffer = std::dynamic_pointer_cast<VulkanVertexBuffer>(m_VertexBuffers.back());

		if (indexBuffer != nullptr) {
			void* data = vulkanVertexBuffer->GetStagingBuffer().allocation->GetMappedData();

			memcpy((int8_t*)data + vulkanVertexBuffer->GetVertices().size() * sizeof(float),
						 vulkanIndexBuffer->GetIndices().data(), indexBuffer->GetCount() * sizeof(uint32_t));
		}

		if (indexBuffer->GetCount() != 0) {
			VulkanContext::Get().ImmediateSubmit([&](vk::CommandBuffer cmd) {
				vk::BufferCopy indexCopy;
				indexCopy.dstOffset = 0;
				indexCopy.srcOffset = 0;
				indexCopy.size = indexBuffer->GetCount() * sizeof(uint32_t);

				cmd.copyBuffer(vulkanIndexBuffer->GetStagingBuffer().buffer, vulkanIndexBuffer->GetBuffer().buffer,
					1, &indexCopy);
				});
		}

		m_IndexBuffer = indexBuffer;
	}

	void VulkanVertexArray::AddVertexAttribDivisor(const uint32_t index, const uint32_t divisor)
	{
	}
}