#pragma once

#include "VoxelEngine/Render/GraphicsContext.h"

#include <vulkan/vulkan.hpp>

#include <glfw/glfw3.h>

#include <optional>

namespace VoxelEngine
{
	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void ClearBuffer() override;
		virtual void SetViewport(const uint32_t width, const uint32_t height) override;

	private:
		struct QueueFamilyIndices {
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			inline bool isComplete() const {
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		vk::UniqueInstance m_Instance;

		vk::SurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;
		vk::UniqueDevice m_Device;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

	private:
		void CreateInstance();
    void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		
		bool IsDeviceSuitable(const vk::PhysicalDevice& device) const;
		QueueFamilyIndices FindQueueFamilies(const vk::PhysicalDevice& device) const;
		std::vector<const char*> GetRequiredExtensions() const;

	private:
		GLFWwindow* m_WindowHandle;		
	};
}