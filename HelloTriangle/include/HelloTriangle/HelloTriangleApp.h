#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class HelloTriangleApp final
{
public:
    HelloTriangleApp() noexcept;
    HelloTriangleApp(const HelloTriangleApp& rhs) = delete;
    HelloTriangleApp& operator=(const HelloTriangleApp& rhs) = delete;
    HelloTriangleApp(HelloTriangleApp&& rhs) = delete;
    HelloTriangleApp& operator=(HelloTriangleApp&& rhs) = delete;
    ~HelloTriangleApp();
public:
    void initVulkan();
    void createInstance();
    void cleanup();
private:
    VkInstance instance;
    GLFWwindow* window;
};