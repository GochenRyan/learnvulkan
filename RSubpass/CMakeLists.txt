project(RSubpass)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp" "include/*.h")

add_executable(RSubpass ${src})
setup_output_directories(RSubpass)

target_compile_definitions(RSubpass PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_include_directories(RSubpass PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(RSubpass PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(RSubpass PRIVATE include)

target_link_libraries(RSubpass PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(RSubpass PRIVATE glfw)