project(TextureMapping)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp" "include/*.h")

add_executable(TextureMapping ${src})
setup_output_directories(TextureMapping)

target_compile_definitions(TextureMapping PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_include_directories(TextureMapping PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(TextureMapping PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/stb)
target_include_directories(TextureMapping PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(TextureMapping PRIVATE include)

target_link_libraries(TextureMapping PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(TextureMapping PRIVATE glfw)