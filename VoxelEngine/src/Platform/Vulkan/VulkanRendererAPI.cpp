#include "VulkanRendererAPI.h"
#include "VulkanContext.h"

#include "vepch.h"

namespace VoxelEngine
{
	void VulkanRendererAPI::Init()
	{
		SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
	{
	}

	void VulkanRendererAPI::SetViewport(const uint32_t x, const uint32_t y,
																			const uint32_t width, const uint32_t height)
	{
	}

	void VulkanRendererAPI::SetDepthMask(const bool mask)
	{
		if (mask) { VulkanContext::Get().GetComputeEffect(VulkanContext::Get().
			GetCurrentComputeEffect()).pipelineBuilder.EnableDepthTest();
		}
		else {
			VulkanContext::Get().GetComputeEffect(VulkanContext::Get().
				GetCurrentComputeEffect()).pipelineBuilder.DisableDepthTest();
		}
	}
	
	void VulkanRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray,
																			const uint32_t indexCount)
	{
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		
	}

	void VulkanRendererAPI::DrawIndexedInstanced(const std::shared_ptr<VertexArray>& vertexArray,
																							 const uint32_t indexCount, const uint32_t instanceCount)
	{
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		
	}

	void VulkanRendererAPI::DrawArray(const std::shared_ptr<VertexArray>& vertexArray,
																		const uint32_t vertexCount)
	{
    
	}

	void VulkanRendererAPI::DrawArraysInstanced(const std::shared_ptr<VertexArray>& vertexArray,
																							const uint32_t vertexCount, const uint32_t instanceCount)
	{
		
	}

	void VulkanRendererAPI::DrawLine(const std::shared_ptr<VertexArray>& vertexArray,
																	 const uint32_t vertexCount)
	{
    
	}
}