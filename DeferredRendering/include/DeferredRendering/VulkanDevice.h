#pragma once
#include <vulkan/vulkan_raii.hpp>

struct VulkanDevice
{
    VulkanDevice() = delete;
    explicit VulkanDevice(vk::raii::PhysicalDevice&& pd);
    ~VulkanDevice();
    VulkanDevice(const VulkanDevice& rhs) = delete;
    VulkanDevice(VulkanDevice&& rhs) = delete;
    VulkanDevice& operator=(const VulkanDevice& rhs) = delete;
    VulkanDevice& operator=(VulkanDevice&& rhs) = delete;

    //todo: Transfer the logic related to the device to this place
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);
    void CreateImage(uint32_t width, uint32_t height, vk::Format format, uint32_t miplevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory);
    std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands();
    void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Queue& queue);
    void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, const vk::raii::Queue& queue);
    void copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const vk::raii::Queue& queue);


    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device logicDevice = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
};