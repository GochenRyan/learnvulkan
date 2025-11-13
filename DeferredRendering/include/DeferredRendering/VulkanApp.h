#pragma once
#include <DeferredRendering/Camera.hpp>
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

constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 1080;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

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
    FramebufferAttachment position, normal, albedo, metallic, roughness, emissive, ao, orm;
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
    ~VulkanApp();
public:
    void Run();
private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void drawFrame();
    void cleanup();
    void cleanupSwapChain();
    void recreateSwapChain();
private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void createDepthResources();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createSwapChainImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCamera();
private:
    bool loadModel();
    void createOffScreenFramebuffer();
    void createUniformBuffers();
    void createDescriptors();
    void createGraphicPipeline();
private:
    void recordCommandBuffer(uint32_t imageIndex);

    void createDescriptorPool();
    void createDescriptorSets();
private:
    /* helper */
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    std::vector<char> readFile(std::string_view filePath);
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    void transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask);

    void updateUniformBufferOffscreen(uint32_t currentImage);
    void updateUniformBufferComposition(uint32_t currentFrame);
    
    void createTextureSampler();
    
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();
    bool hasStencilComponent(vk::Format format);
    void createAttachment(vk::ImageUsageFlagBits usage, FramebufferAttachment* attachment);
    void handleInput();
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

    std::unique_ptr<VulkanDevice> deviceVK = nullptr;
    vk::raii::Queue queue{ nullptr };

    // KHR: Khronos
    vk::raii::SurfaceKHR surface{ nullptr };
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;

    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::PipelineLayout pipelineLayoutOffScreen = nullptr;
    vk::raii::PipelineLayout pipelineLayoutComposition = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    uint32_t queueIndex = ~0;

    uint32_t semaphoreIndex = 0;
    uint32_t currentFrame = 0;
    
    std::vector<void*> uniformBuffersMapped;
    vk::raii::DescriptorPool descriptorPool = nullptr;

    vk::raii::Sampler textureSampler = nullptr;

    vk::raii::Image depthImage = nullptr;
    vk::raii::DeviceMemory depthImageMemory = nullptr;
    vk::raii::ImageView depthImageView = nullptr;

    vk::raii::RenderPass renderPass = nullptr;
    std::vector<vk::raii::Framebuffer> swapChainFramebuffers;

    Framebuffer offScreenFramebuffer{};
    vk::raii::Sampler colorSampler = nullptr;

    struct 
    {
        vk::raii::Pipeline offscreen = nullptr;
        vk::raii::Pipeline composition = nullptr;
    } pipelines;

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

    std::vector<Model> models;

    struct DescriptorSets
    {
        std::vector<vk::raii::DescriptorSet> modelUBOs;
        vk::raii::DescriptorSet composition = nullptr;
    };

    vk::raii::DescriptorSetLayout descriptorSetLayoutOffScreenUBO = nullptr;
    vk::raii::DescriptorSetLayout descriptorSetLayoutComposition = nullptr;
    vk::raii::DescriptorSetLayout descriptorSetLayoutOffScreenMat = nullptr;

    std::array<DescriptorSets, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    float frameTimer{ 1.0f };
public:
    /*
        Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, it is not guaranteed to happen. 
        That's why we'll add some extra code to also handle resizes explicitly. 
    */
    bool framebufferResized = false;
    int32_t debugDisplayTarget = 0;

    Camera camera;
    double lastXPos{};
    double lastYPos{};
};