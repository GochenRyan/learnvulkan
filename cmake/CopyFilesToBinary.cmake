# 函数：将指定目录下的所有文件（非递归）复制到 ${CMAKE_BINARY_DIR} 根目录
# 使用示例：copy_all_files_to_build("/path/to/thirdparty/lib")
function(copy_all_files_to_build source_dir)
    # 转换为绝对路径并检查目录是否存在
    get_filename_component(source_dir "${source_dir}" ABSOLUTE)
    if(NOT IS_DIRECTORY "${source_dir}")
        message(FATAL_ERROR "目录 ${source_dir} 不存在，无法复制文件。")
    endif()

    # 使用 GLOB 列出当前目录下的所有文件（排除子目录）
    file(GLOB files LIST_DIRECTORIES FALSE "${source_dir}/*")

    # 遍历所有文件并复制到构建目录根
    foreach(file_path IN LISTS files)
        file(COPY "${file_path}" DESTINATION "${CMAKE_BINARY_DIR}")
    endforeach()
endfunction()
