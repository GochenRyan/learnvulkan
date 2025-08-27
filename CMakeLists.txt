cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(LearnVulkan LANGUAGES CXX)

add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)

include(cmake/OutputDirectories.cmake)
include(cmake/CopyFilesToBinary.cmake)
include(cmake/CopySpecifiedFilesToBuild.cmake)

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(WARNING "The Binary directory of CMake cannot be the same as source directory!")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# set(ENV{VULKAN_SDK} ${CMAKE_SOURCE_DIR}/ThirdParty/vulkansdk)
find_package(Vulkan REQUIRED)

message(STATUS "==> FindVulkan.cmake path: ${_FINDVULKAN_CMAKE_FILE}")

message("find vulkan : ${Vulkan_FOUND}")

if(${Vulkan_FOUND})
    message("vulkan libs : ${Vulkan_LIBRARY}")
    copy_specified_files_to_build(FILES ${Vulkan_LIBRARY})
endif()

# Find all subdirectories that contain shader.slang
file(GLOB_RECURSE SHADER_DIRS_LIST
    "${CMAKE_SOURCE_DIR}/*/*.slang"
)

# Extract the directory path and remove duplicates
set(SHADER_DIRS "")
foreach(shader_file ${SHADER_DIRS_LIST})
    get_filename_component(shader_dir "${shader_file}" DIRECTORY)
    list(APPEND SHADER_DIRS "${shader_dir}")
endforeach()

list(REMOVE_DUPLICATES SHADER_DIRS)

# Define custom goals
add_custom_target(
    RunScript ALL
    COMMENT "Parsing all shaders..."
)

# Add commands for each directory
foreach(shader_dir ${SHADER_DIRS})
    add_custom_command(
        TARGET RunScript
        COMMAND cmd.exe /c "${CMAKE_SOURCE_DIR}/compile.bat" 
            "${shader_dir}/shader.slang" 
            "${shader_dir}/slang.spv"
        WORKING_DIRECTORY "${shader_dir}"
        COMMENT "Parsing shader in: ${shader_dir}"
    )
endforeach()

# Assets
set(ASSETS_SRC_DIR "${CMAKE_SOURCE_DIR}/Assets")
add_compile_definitions(ASSETS_SRC_DIR="${ASSETS_SRC_DIR}")

# Project
set(BUILD_SHARED_LIBS ON)
add_subdirectory(ThirdParty/glfw)
setup_output_directories(glfw)

add_subdirectory(TestThirdParty)
add_subdirectory(HelloTriangle)
add_subdirectory(TextureMapping)

# add_subdirectory(Common)
