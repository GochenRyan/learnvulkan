project(LoadingModel)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp" "include/*.h")

add_executable(LoadingModel ${src})
setup_output_directories(LoadingModel)

target_compile_definitions(LoadingModel PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

target_include_directories(LoadingModel PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(LoadingModel PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/stb)
target_include_directories(LoadingModel PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(LoadingModel PRIVATE include)

target_link_libraries(LoadingModel PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(LoadingModel PRIVATE glfw)
target_link_libraries(LoadingModel PRIVATE FBXSDK::FBXSDK)
target_compile_definitions(LoadingModel PRIVATE FBXSDK_SHARED)