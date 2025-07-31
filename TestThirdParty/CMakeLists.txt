project(TestThirdParty)

file(GLOB_RECURSE src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(TestThirdParty ${src})
setup_output_directories(TestThirdParty)

target_include_directories(TestThirdParty PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/glm)
target_include_directories(TestThirdParty PRIVATE ${Vulkan_INCLUDE_DIR})
target_link_libraries(TestThirdParty PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(TestThirdParty PRIVATE glfw)