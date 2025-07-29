#if 0
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


int main()
{
    glfwInit();
    return 0;
}
#endif

#if 1
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