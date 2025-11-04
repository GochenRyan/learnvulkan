cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(LearnVulkan LANGUAGES CXX)

add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)

include(cmake/OutputDirectories.cmake)
include(cmake/CopyFilesToBinary.cmake)
include(cmake/CopySpecifiedFilesToBuild.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

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

# Traverse all shader directories
foreach(shader_dir ${SHADER_DIRS})
    # Collect all .slang files in this directory
    file(GLOB shader_files "${shader_dir}/*.slang")

    # Traverse each .slang file
    foreach(shader_file ${shader_files})
        # Get the file name (without extension)
        get_filename_component(shader_name ${shader_file} NAME_WE)

        # Build the path of the output .spv file (default)
        set(output_file "${shader_dir}/${shader_name}.spv")
        # Build the path of the ORM variant .spv file
        set(output_file_orm "${shader_dir}/${shader_name}_ORM.spv")

        # Read shader source to check whether it references USE_ORM (several common patterns)
        file(READ "${shader_file}" shader_text)

        # Look for a few patterns that indicate the shader uses the macro
        string(FIND "${shader_text}" "USE_ORM" found_useorm)            # any appearance of token
        string(FIND "${shader_text}" "defined(USE_ORM)" found_defined)  # defined(USE_ORM)
        string(FIND "${shader_text}" "#ifdef USE_ORM" found_ifdef)      # #ifdef USE_ORM
        string(FIND "${shader_text}" "#if USE_ORM" found_if)            # #if USE_ORM

        set(need_orm_variant FALSE)
        if(NOT found_useorm EQUAL -1 OR NOT found_defined EQUAL -1 OR NOT found_ifdef EQUAL -1 OR NOT found_if EQUAL -1)
            set(need_orm_variant TRUE)
        endif()

        # Construct command strings (use quotes to handle spaces)
        set(CMD_DEFAULT "${CMAKE_SOURCE_DIR}/compile.bat ${shader_file} ${output_file}")
        set(CMD_ORM     "${CMAKE_SOURCE_DIR}/compile.bat ${shader_file} ${output_file_orm} -D USE_ORM")

        add_custom_command(
            TARGET RunScript
            COMMAND cmd.exe /c ${CMD_DEFAULT}
            WORKING_DIRECTORY "${shader_dir}"
            COMMENT "Parsing shader: ${shader_file}"
        )

        if(need_orm_variant)
            add_custom_command(
                TARGET RunScript
                COMMAND cmd.exe /c ${CMD_ORM}
                WORKING_DIRECTORY "${shader_dir}"
                COMMENT "Parsing shader (USE_ORM): ${shader_file}"
            )
        else()
            message(VERBOSE "Skipping ORM variant for ${shader_file} (no USE_ORM found).")
        endif()
    endforeach()
endforeach()

set(FBXSDK_LIB_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/lib)
find_package(FBXSDK REQUIRED)

# Assets
set(ASSETS_SRC_DIR "${CMAKE_SOURCE_DIR}/Assets")
add_compile_definitions(ASSETS_SRC_DIR="${ASSETS_SRC_DIR}")

copy_all_files_to_build(${FBXSDK_LIB_DIR})

# Project
set(BUILD_SHARED_LIBS ON)
add_subdirectory(ThirdParty/glfw)
setup_output_directories(glfw)

add_subdirectory(TestThirdParty)
add_subdirectory(HelloTriangle)
add_subdirectory(TextureMapping)
add_subdirectory(DepthBuffering)
add_subdirectory(LoadingModel)
add_subdirectory(RSubpass)
add_subdirectory(DeferredRendering)

# add_subdirectory(Common)
