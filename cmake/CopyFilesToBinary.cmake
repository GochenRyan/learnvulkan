# CopyAllFilesToBuild.cmake
# Function: Copy all files from the specified directory (non-recursive) to the root of the build directory (${CMAKE_BINARY_DIR})
# Usage example:
#   include("${CMAKE_SOURCE_DIR}/cmake/CopyAllFilesToBuild.cmake")
#   copy_all_files_to_build("/path/to/thirdparty/lib")

function(copy_all_files_to_build source_dir)
    # Convert source_dir to absolute path and verify the directory exists
    get_filename_component(source_dir "${source_dir}" ABSOLUTE)
    if(NOT IS_DIRECTORY "${source_dir}")
        message(FATAL_ERROR "Directory ${source_dir} does not exist, cannot copy files.")
    endif()

    # Use GLOB to list all files in the directory (excluding subdirectories)
    file(GLOB files LIST_DIRECTORIES FALSE "${source_dir}/*")

    # Iterate over the file list and copy each file to the build directory root
    foreach(file_path IN LISTS files)
        file(COPY "${file_path}" DESTINATION "${CMAKE_BINARY_DIR}")
    endforeach()
endfunction()