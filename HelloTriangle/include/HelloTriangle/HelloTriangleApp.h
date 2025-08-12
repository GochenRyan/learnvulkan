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

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

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
    uint32_t findQueueFamilies();

    void createLogicalDevice();

    void createSurface();

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    void createImageViews();
private:
    GLFWwindow* window{ nullptr };

    vk::raii::Context  context;
    vk::raii::Instance instance{ nullptr };
    vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

    vk::raii::PhysicalDevice physicalDevice{ nullptr };

    std::vector<const char*> deviceExtensions = {
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
};