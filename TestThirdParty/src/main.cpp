#if 0
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


int main()
{
    glfwInit();
    return 0;
}
#endif

#if 0
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>


int main()
{
    glm::mat4 matrix;
    return 0;
}
#endif

#if 1
#include <vulkan/vulkan.h>
#include <iostream>

int main() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Link Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance (error code " << result << ")\n";
        return -1;
    }

    std::cout << "Vulkan instance created successfully!\n";

    vkDestroyInstance(instance, nullptr);
    return 0;
}
#endif