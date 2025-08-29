project(DepthBuffering)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp" "include/*.h")

add_executable(DepthBuffering ${src})
setup_output_directories(DepthBuffering)

target_compile_definitions(DepthBuffering PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_include_directories(DepthBuffering PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(DepthBuffering PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/stb)
target_include_directories(DepthBuffering PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(DepthBuffering PRIVATE include)

target_link_libraries(DepthBuffering PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(DepthBuffering PRIVATE glfw)