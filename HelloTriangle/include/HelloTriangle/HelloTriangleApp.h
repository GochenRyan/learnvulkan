#pragma once
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

#include <glm/glm.hpp>

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
    glm::vec2 pos;
    glm::vec3 color;

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

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        /*
            The **binding** parameter tells Vulkan from which binding the per-vertex data comes.
            The **location** parameter references the location directive of the input in the vertex shader.
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
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
        };
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

class HelloTriangleApp final
{
public:
    HelloTriangleApp() noexcept = default;
    HelloTriangleApp(const HelloTriangleApp& rhs) = delete;
    HelloTriangleApp& operator=(const HelloTriangleApp& rhs) = delete;
    HelloTriangleApp(HelloTriangleApp&& rhs) = delete;
    HelloTriangleApp& operator=(HelloTriangleApp&& rhs) = delete;
    ~HelloTriangleApp() = default;
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

    void createVertexBuffer();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
private:
    GLFWwindow* window{ nullptr };

    vk::raii::Context  context;
    vk::raii::Instance instance{ nullptr };
    vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

    vk::raii::PhysicalDevice physicalDevice{ nullptr };

    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,  // It provides the capability of "swapchain" for Vulkan applications - that is, on top of the Window System Integration (WSI), to implement the process of rendering images to the screen
        vk::KHRSpirv14ExtensionName,  // Allow the Vulkan driver to directly accept the shader Intermediate Language of version SPIR-V 1.4. SPIR-V is the shader binary format used by Vulkan (as well as OpenCL).
        vk::KHRSynchronization2ExtensionName,  // It provides a new generation of Vulkan synchronization API, simplifying and unifying the use of synchronization primitives such as command buffers, pipeline barriers, events, and semaphores
        vk::KHRCreateRenderpass2ExtensionName  // The creation interface for "Render Pass" has been expanded and improved, allowing you to specify more abundant subpass dependencies and attachment state transitions at one time during creation.
    };

    vk::raii::Device device{ nullptr };
    vk::raii::Queue queue{ nullptr };

    // KHR: Khronos
    vk::raii::SurfaceKHR surface{ nullptr };
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;

    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::PipelineLayout pipelineLayout = nullptr;

    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    uint32_t queueIndex = ~0;

    uint32_t semaphoreIndex = 0;
    uint32_t currentFrame = 0;
    
    vk::raii::Buffer vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;
public:
    /*
        Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, it is not guaranteed to happen. 
        That’s why we’ll add some extra code to also handle resizes explicitly. 
    */
    bool framebufferResized = false;
};