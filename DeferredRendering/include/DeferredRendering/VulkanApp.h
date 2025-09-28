#pragma once
#include <DeferredRendering/Model.h>
#include <DeferredRendering/VulkanBuffer.h>
#include <DeferredRendering/VulkanDevice.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <algorithm>
 
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>

#define GLFW_INCLUDE_VULKAN // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr  uint32_t WIDTH = 800;
constexpr  uint32_t HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 color;
    glm::vec3 norm;
    glm::vec3 tangent;

    /*
        A vertex binding describes at which rate to load data from memory throughout the vertices. 
        It specifies the **number of bytes between data entries** and whether to move to the next data entry after each **vertex** or after each **instance**.
    */
    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return { 
            // The binding parameter specifies the index of the binding in the array of bindings.
            0, 
            // The stride parameter specifies the number of bytes from one entry to the next
            sizeof(Vertex), 
            /*
                The inputRate parameter can have one of the following values:
                    VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
                    VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
            */
            vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 5> getAttributeDescriptions()
    {
        /*
            The **location** parameter references the location directive of the input in the vertex shader.
            The **binding** parameter tells Vulkan from which binding the per-vertex data comes.
            The **format** parameter describes the type of data for the attribute.
                float: VK_FORMAT_R32_SFLOAT
                float2: VK_FORMAT_R32G32_SFLOAT
                float3: VK_FORMAT_R32G32B32_SFLOAT
                float4: VK_FORMAT_R32G32B32A32_SFLOAT

                int2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
                uint4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
                double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
            The **offset** parameter has specified the number of bytes since the start of the per-vertex data to read from. T
        */

        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, norm)),
            vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent))
        };
    }
};

static inline uint32_t floatToBits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(float));
    return u;
}

struct VertexHash
{
    size_t operator()(const Vertex& v) const noexcept
    {
        // Each group combines two 32-bit floats into one 64-bit and then mixes them with constants
        uint64_t h1 = (static_cast<uint64_t>(floatToBits(v.pos.x)) << 32) ^ floatToBits(v.pos.y);
        uint64_t h2 = (static_cast<uint64_t>(floatToBits(v.pos.z)) << 32) ^ floatToBits(v.texCoord.x);
        uint64_t h3 = (static_cast<uint64_t>(floatToBits(v.texCoord.y)) << 32) ^ floatToBits(v.color.x);
        uint64_t h4 = (static_cast<uint64_t>(floatToBits(v.color.y)) << 32) ^ floatToBits(v.color.z);
        uint64_t h5 = (static_cast<uint64_t>(floatToBits(v.norm.x)) << 32) ^ floatToBits(v.norm.y);
        uint64_t h6 = static_cast<uint64_t>(floatToBits(v.norm.z));

        // Commonly used 64-bit mixing constants (derived from splitmix64 / PCG, etc.) can be replaced with other seeds
        const uint64_t C1 = 11400714819323198485ull;
        const uint64_t C2 = 14029467366897019727ull;
        const uint64_t C3 = 1609587929392839161ull;
        const uint64_t C4 = 9650029242287828579ull;

        uint64_t h = h1;
        h ^= h2 * C1;
        h ^= h3 * C2;
        h ^= h4 * C3;
        h ^= h5 * C4;
        h ^= h6 * (C1 ^ C2);

        // Finally, perform point dispersion (optional)
        h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ull;
        h = (h ^ (h >> 27)) * 0x94d049bb133111ebull;
        h = h ^ (h >> 31);

        return static_cast<size_t>(h);
    }
};

struct VertexEqual
{
    bool operator()(const Vertex& a, const Vertex& b) const noexcept
    {
        return  floatToBits(a.pos.x) == floatToBits(b.pos.x) &&
                floatToBits(a.pos.y) == floatToBits(b.pos.y) &&
                floatToBits(a.pos.z) == floatToBits(b.pos.z) &&
                floatToBits(a.texCoord.x) == floatToBits(b.texCoord.x) &&
                floatToBits(a.texCoord.y) == floatToBits(b.texCoord.y) &&
                floatToBits(a.color.x) == floatToBits(b.color.x) &&
                floatToBits(a.color.y) == floatToBits(b.color.y) &&
                floatToBits(a.color.z) == floatToBits(b.color.z) &&
                floatToBits(a.norm.x) == floatToBits(b.norm.x) &&
                floatToBits(a.norm.y) == floatToBits(b.norm.y) &&
                floatToBits(a.norm.z) == floatToBits(b.norm.z);
    }
};

struct FramebufferAttachment
{
    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory mem = nullptr;
    vk::raii::ImageView view = nullptr;
    vk::Format format = {};
};

struct Framebuffer
{
    uint32_t width, height;
    vk::raii::Framebuffer framebuffer = nullptr;
    FramebufferAttachment position, normal, albedo;
    FramebufferAttachment depth;
    vk::raii::RenderPass renderPass = nullptr;
};

class VulkanApp final
{
public:
    VulkanApp() noexcept = default;
    VulkanApp(const VulkanApp& rhs) = delete;
    VulkanApp& operator=(const VulkanApp& rhs) = delete;
    VulkanApp(VulkanApp&& rhs) = delete;
    VulkanApp& operator=(VulkanApp&& rhs) = delete;
    ~VulkanApp() = default;
public:
    void Run();
private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void createInstance();
    void cleanup();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void setupDebugMessenger();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSurface();

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    void createImageViews();

    void createGraphicPipeline();

    std::vector<char> readFile(std::string_view filePath);
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex);
    void transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask);

    void drawFrame();
    void createSyncObjects();

    void cleanupSwapChain();
    void recreateSwapChain();

    /*
        Vulkan Memory Management : https://developer.nvidia.com/vulkan-memory-management
    */
    void createVertexBuffer();
    
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    void createIndexBuffer();

    void createDescriptorSetLayout();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);
    void createDescriptorPool();
    void createDescriptorSets();

    void createTextureImage();
    void createTextureImageView();
    vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags);
    void createTextureSampler();

    void createDepthResources();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();
    bool hasStencilComponent(vk::Format format);

    bool loadModel();

    /* Subpass */
    void createRenderPass();
    void createFramebuffers();
    
    void createoffScreenFramebuffer();
    void createAttachment(vk::Format format, vk::ImageUsageFlagBits usage, FramebufferAttachment* attachment);
private:
    GLFWwindow* window{ nullptr };

    vk::raii::Context  context;
    vk::raii::Instance instance{ nullptr };
    vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,  // It provides the capability of "swapchain" for Vulkan applications - that is, on top of the Window System Integration (WSI), to implement the process of rendering images to the screen
        vk::KHRSpirv14ExtensionName,  // Allow the Vulkan driver to directly accept the shader Intermediate Language of version SPIR-V 1.4. SPIR-V is the shader binary format used by Vulkan (as well as OpenCL).
        vk::KHRSynchronization2ExtensionName,  // It provides a new generation of Vulkan synchronization API, simplifying and unifying the use of synchronization primitives such as command buffers, pipeline barriers, events, and semaphores
        vk::KHRCreateRenderpass2ExtensionName  // The creation interface for "Render Pass" has been expanded and improved, allowing you to specify more abundant subpass dependencies and attachment state transitions at one time during creation.
    };

    vk::raii::Queue queue{ nullptr };

    // KHR: Khronos
    vk::raii::SurfaceKHR surface{ nullptr };
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;

    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    uint32_t queueIndex = ~0;

    uint32_t semaphoreIndex = 0;
    uint32_t currentFrame = 0;
    
    vk::raii::Buffer vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;

    vk::raii::Buffer indexBuffer = nullptr;
    vk::raii::DeviceMemory indexBufferMemory = nullptr;

    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    
    std::vector<void*> uniformBuffersMapped;
    vk::raii::DescriptorPool descriptorPool = nullptr;

    vk::raii::Image textureImage = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    vk::raii::ImageView textureImageView = nullptr;
    vk::raii::Sampler textureSampler = nullptr;

    vk::raii::Image depthImage = nullptr;
    vk::raii::DeviceMemory depthImageMemory = nullptr;
    vk::raii::ImageView depthImageView = nullptr;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vk::raii::RenderPass renderPass = nullptr;
    std::vector<vk::raii::Framebuffer> swapChainFramebuffers;

    Framebuffer offScreenFramebuffer{};
    vk::raii::Sampler colorSampler = nullptr;

    struct 
    {
        vk::raii::Pipeline offscreen = nullptr;
        vk::raii::Pipeline composition = nullptr;
    } pipelines;

    struct DescriptorSets {
        vk::raii::DescriptorSet model = nullptr;
        vk::raii::DescriptorSet floor = nullptr;
        vk::raii::DescriptorSet composition = nullptr;
    };
    std::array<DescriptorSets, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    struct UniformDataOffscreen {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        glm::vec4 instancePos[3];
    } uniformDataOffscreen;

    struct Light {
        glm::vec4 position;
        glm::vec3 color;
        float radius;
    };

    struct UniformDataComposition {
        Light lights[6];
        glm::vec4 viewPos;
        int debugDisplayTarget = 0;
    } uniformDataComposition;

    struct UniformBuffers {
        VulkanBuffer offscreen;
        VulkanBuffer composition;
    };
    std::array<UniformBuffers, MAX_FRAMES_IN_FLIGHT> uniformBuffers;

    struct {
        Model model;
        Model Floor;
    } models;

    VulkanDevice* deviceVK = nullptr;
public:
    /*
        Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, it is not guaranteed to happen. 
        That's why we'll add some extra code to also handle resizes explicitly. 
    */
    bool framebufferResized = false;
};