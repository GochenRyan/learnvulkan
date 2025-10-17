#include <DeferredRendering/VulkanDevice.h>

VulkanDevice::VulkanDevice(vk::raii::PhysicalDevice&& pd)
{
	physicalDevice = pd;
}

VulkanDevice::~VulkanDevice()
{
}

/*
    Graphics cards can offer different types of memory to allocate from.
    Each type of memory varies in terms of allowed operations and performance characteristics.
    We need to combine the requirements of the buffer and our own application requirements to find the right type of memory to use.
*/
uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    /*
        The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
        Memory heaps are distinct memory resources like **dedicated VRAM** and **swap space in RAM** for when VRAM runs out.
        The different types of memory exist within these heaps.
    */
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanDevice::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory, void* data)
{
    vk::BufferCreateInfo bufferInfo{
        // used to configure sparse buffer memory,
        .flags = {},
        // specifies the size of the buffer in bytes.
        .size = size,
        // indicates for which purposes the data in the buffer is going to be used. It is possible to specify multiple purposes using a bitwise or.
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };
    buffer = vk::raii::Buffer(logicDevice, bufferInfo);

    /*
        The VkMemoryRequirements struct has three fields:
            size: The size of the required memory in bytes may differ from bufferInfo.size.
            alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends on bufferInfo.usage and bufferInfo.flags.
            memoryTypeBits: Bit field of the memory types that are suitable for the buffer.
    */
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties)
    };

    /*
        It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory for every individual buffer.
        The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080.
        The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a single allocation among many different objects
        by using the offset parameters that we've seen in many functions.

        You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative.
        However, for this tutorial, it's okay to use a separate allocation for every resource, because we won't come close to hitting any of these limits for now.
    */
    bufferMemory = logicDevice.allocateMemory(memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, /* the offset within the region of memory. If the offset is non-zero, then it is required to be divisible by memRequirements.alignment. */0);

    if (data != nullptr)
    {
        void* mapped = bufferMemory.mapMemory(0, size);
        memcpy(mapped, data, size);
        bufferMemory.unmapMemory();
    }
}

void VulkanDevice::CreateImage(uint32_t width, uint32_t height, vk::Format format, uint32_t miplevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageCI{
        /*
            tells Vulkan with what kind of coordinate system the texels in the image are going to be addressed. It is possible to create 1D, 2D and 3D images.
            One dimensional images can be used to store an array of data or gradient,
            two dimensional images are mainly used for textures,
            and three dimensional images can be used to store voxel volumes
        */
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height,
        //  The extent field specifies the dimensions of the image, basically how many texels there are on each axis. That’s why depth must be 1 instead of 0
        1},
        .mipLevels = miplevels,
        .arrayLayers = 1,
        // The samples flag is related to multisampling. This is only relevant for images that will be used as attachments, so stick to one sample. 
        .samples = vk::SampleCountFlagBits::e1,
        /*
            VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
            VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access
        */
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        // The image will only be used by one queue family: the one that supports graphics (and therefore also) transfer operations.
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = {},
        .initialLayout = vk::ImageLayout::eUndefined
    };

    image = logicDevice.createImage(imageCI);
    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties)
    };
    imageMemory = logicDevice.allocateMemory(allocInfo);
    image.bindMemory(imageMemory, 0);
}

vk::raii::ImageView VulkanDevice::CreateImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags) const
{
    /*
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        Each component (r, g, b, a) in the components field specifies the mapping method of the image color channel.
        VK_COMPONENT_SWIZZLE_IDENTITY indicates no swapping, that is, the red channel remains red, the green channel remains green, and so on.
        Vulkan allows the order of image channels to be adjusted through component swapping (Swizzle) without modifying the image data itself. This is very useful in the following scenarios:
            Format mismatch: When the image format does not match the channel order expected by the shader (for example, the image is stored as BGR, but the shader expects RGB).
            Monochrome channel: Map multiple channels to the same value (for example, set the Alpha channel as the red channel).
            Simplify data processing: Avoid preprocessing image data on the CPU side.
    */

    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    return logicDevice.createImageView(viewInfo);
}

std::unique_ptr<vk::raii::CommandBuffer> VulkanDevice::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
    };
    std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(std::move(vk::raii::CommandBuffers(logicDevice, allocInfo).front()));

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    commandBuffer->begin(beginInfo);

    return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Queue& queue)
{
    commandBuffer.end();
    queue.submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer
        }, nullptr);
    queue.waitIdle();
}

void VulkanDevice::transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, const vk::raii::Queue& queue)
{
    auto commandBuffer = beginSingleTimeCommands();
    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    if (oldLayout == vk::ImageLayout::eUndefined &&
        /*
            There is actually a special type of image layout that supports all operations, VK_IMAGE_LAYOUT_GENERAL.
            The problem with it, of course, is that it doesn’t necessarily offer the best performance for any operation.
            It is required for some special cases, like using an image as both input and output, or for reading an image after it has left the preinitialized layout.
        */
        newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer->pipelineBarrier(
        /*
            https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported
        */
        // The first parameter after the command buffer specifies in which pipeline stage the operations occur that should happen before the barrier.
        sourceStage,
        // The second parameter specifies the pipeline stage in which operations will wait on the barrier. 
        destinationStage, {}, {}, nullptr, barrier);
    endSingleTimeCommands(*commandBuffer, queue);
}

void VulkanDevice::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const vk::raii::Queue& queue)
{
    auto commandBuffer = beginSingleTimeCommands();
    vk::BufferImageCopy region{
        .bufferOffset = 0,

        /*
            The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory.
            For example, you could have some padding bytes between rows of the image.
        */
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };
    commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
    endSingleTimeCommands(*commandBuffer, queue);
}

void VulkanDevice::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size, const vk::raii::Queue& queue)
{
    auto commandCopyBuffer = beginSingleTimeCommands();
    commandCopyBuffer->copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    endSingleTimeCommands(*commandCopyBuffer, queue);
}