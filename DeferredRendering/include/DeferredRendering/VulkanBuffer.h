#pragma once
#include <vulkan/vulkan_raii.hpp>

struct VulkanBuffer final
{
	vk::raii::Buffer uniformBuffer = nullptr;
	vk::raii::DeviceMemory uniformBufferMemory = nullptr;
	void* uniformBuffersMapped = nullptr;
};