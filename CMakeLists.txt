cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(LearnVulkan LANGUAGES CXX)

add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)

include(cmake/OutputDirectories.cmake)
include(cmake/CopyFilesToBinary.cmake)
include(cmake/CopySpecifiedFilesToBuild.cmake)

if (PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(WARNING "The Binary directory of CMake cannot be the same as source directory!")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (WIN32)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

set(ENV{VULKAN_SDK} ${CMAKE_SOURCE_DIR}/ThirdParty/vulkansdk)
find_package(Vulkan REQUIRED)

message(STATUS "==> FindVulkan.cmake path: ${_FINDVULKAN_CMAKE_FILE}")

message("find vulkan : ${Vulkan_FOUND}")
if (${Vulkan_FOUND})
    message("vulkan libs : ${Vulkan_LIBRARY}")
    copy_specified_files_to_build(FILES ${Vulkan_LIBRARY})
endif()

set(BUILD_SHARED_LIBS ON)
add_subdirectory(ThirdParty/glfw)
setup_output_directories(glfw)

add_subdirectory(TestThirdParty)

# add_subdirectory(Common)
