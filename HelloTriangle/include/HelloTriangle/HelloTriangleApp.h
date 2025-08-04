#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
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
private:
    VkInstance instance;
    GLFWwindow* window;
};