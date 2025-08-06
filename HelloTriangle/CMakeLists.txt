project(HelloTriangle)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp" "include/*.h")

add_executable(HelloTriangle ${src})
setup_output_directories(HelloTriangle)

target_compile_definitions(HelloTriangle PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_include_directories(HelloTriangle PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(HelloTriangle PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(HelloTriangle PRIVATE include)

target_link_libraries(HelloTriangle PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(HelloTriangle PRIVATE glfw)