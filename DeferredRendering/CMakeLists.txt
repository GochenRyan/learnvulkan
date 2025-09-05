project(DeferredRendering)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp" "include/*.h")

add_executable(DeferredRendering ${src})
setup_output_directories(DeferredRendering)

target_compile_definitions(DeferredRendering PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_include_directories(DeferredRendering PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(DeferredRendering PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/stb)
target_include_directories(DeferredRendering PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(DeferredRendering PRIVATE include)

target_link_libraries(DeferredRendering PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(DeferredRendering PRIVATE glfw)
target_link_libraries(DeferredRendering PRIVATE FBXSDK::FBXSDK)
target_compile_definitions(DeferredRendering PRIVATE FBXSDK_SHARED)