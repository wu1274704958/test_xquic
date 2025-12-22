# CopyFiles.cmake - 文件复制脚本
# 用法: cmake -D TARGET_DIR="目标目录" -D FILE_LIST="文件列表文件" -P CopyFiles.cmake
#
# 参数:
#   TARGET_DIR - 目标目录（必需）
#   FILE_LIST  - 包含文件列表的文件路径（必需），文件内容格式：路径1;路径2;路径3
#   COPY_MODE  - 复制模式: ALWAYS（总是复制，默认）或 IF_DIFFERENT（仅当不同时复制）
#   CREATE_SUBDIRS - 是否创建子目录结构: ON（创建）或 OFF（不创建，默认）
#   VERBOSE    - 详细输出: ON（详细）或 OFF（简洁，默认）

cmake_minimum_required(VERSION 3.10)

# 参数验证函数
function(validate_parameters)
    if(NOT TARGET_DIR)
        message(FATAL_ERROR "TARGET_DIR parameter is required")
    endif()

    if(NOT FILE_LIST)
        message(FATAL_ERROR "FILE_LIST parameter is required")
    endif()

    if(NOT EXISTS "${FILE_LIST}")
        message(FATAL_ERROR "File list file does not exist: ${FILE_LIST}")
    endif()
endfunction()

# 设置默认值函数
function(set_defaults)
    if(NOT DEFINED COPY_MODE)
        set(COPY_MODE "ALWAYS")
    endif()

    if(NOT DEFINED CREATE_SUBDIRS)
        set(CREATE_SUBDIRS "OFF")
    endif()

    if(NOT DEFINED VERBOSE)
        set(VERBOSE "OFF")
    endif()
endfunction()

# 创建目标目录
function(create_target_directory)
    if(NOT EXISTS "${TARGET_DIR}")
        file(MAKE_DIRECTORY "${TARGET_DIR}")
        message(STATUS "Created target directory: ${TARGET_DIR}")
    endif()
endfunction()

# 读取文件列表
function(read_file_list result_var)
    # 读取文件内容
    file(READ "${FILE_LIST}" file_content)

    # 移除可能的换行符和多余空格
    string(STRIP "${file_content}" stripped_content)
    string(REPLACE "\n" ";" content_with_semicolons "${stripped_content}")

    # 按分号分割成列表
    set(file_list)
    foreach(item ${content_with_semicolons})
        # 移除每个项目两端的空格
        string(STRIP "${item}" clean_item)
        if(clean_item)
            list(APPEND file_list "${clean_item}")
        endif()
    endforeach()

    set(${result_var} ${file_list} PARENT_SCOPE)
endfunction()

# 验证源文件是否存在
function(validate_source_files file_list valid_files_var missing_files_var)
    set(valid_files)
    set(missing_files)

    foreach(src_file ${file_list})
        if(EXISTS "${src_file}")
            list(APPEND valid_files "${src_file}")
        else()
            list(APPEND missing_files "${src_file}")
            if(VERBOSE)
                message(WARNING "Source file does not exist: ${src_file}")
            endif()
        endif()
    endforeach()

    set(${valid_files_var} ${valid_files} PARENT_SCOPE)
    set(${missing_files_var} ${missing_files} PARENT_SCOPE)
endfunction()

# 计算目标文件路径
function(get_destination_path src_file dest_path_var)
    if(CREATE_SUBDIRS STREQUAL "ON")
        # 保持目录结构
        get_filename_component(src_dir "${src_file}" DIRECTORY)
        get_filename_component(filename "${src_file}" NAME)

        # 计算相对路径（相对于源文件列表文件所在目录）
        get_filename_component(file_list_dir "${FILE_LIST}" DIRECTORY)
        if(file_list_dir)
            file(RELATIVE_PATH rel_path "${file_list_dir}" "${src_dir}")
        else()
            # 如果文件列表在当前目录，使用绝对路径
            get_filename_component(rel_path "${src_dir}" REALPATH)
        endif()

        if(rel_path)
            set(dest_path "${TARGET_DIR}/${rel_path}/${filename}")
        else()
            set(dest_path "${TARGET_DIR}/${filename}")
        endif()
    else()
        # 扁平化结构 - 所有文件都复制到目标目录根目录
        get_filename_component(filename "${src_file}" NAME)
        set(dest_path "${TARGET_DIR}/${filename}")
    endif()

    set(${dest_path_var} "${dest_path}" PARENT_SCOPE)
endfunction()

# 复制单个文件
function(copy_single_file src_file result_var)
    get_destination_path("${src_file}" dest_path)

    # 创建目标目录（如果需要）
    get_filename_component(dest_dir "${dest_path}" DIRECTORY)
    if(NOT EXISTS "${dest_dir}")
        file(MAKE_DIRECTORY "${dest_dir}")
        if(VERBOSE)
            message(STATUS "Created directory: ${dest_dir}")
        endif()
    endif()

    # 执行复制操作
    if(COPY_MODE STREQUAL "IF_DIFFERENT")
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src_file}" "${dest_path}"
                RESULT_VARIABLE copy_result
                OUTPUT_QUIET
                ERROR_QUIET
        )
    else()
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy "${src_file}" "${dest_path}"
                RESULT_VARIABLE copy_result
                OUTPUT_QUIET
                ERROR_QUIET
        )
    endif()

    # 处理结果
    if(copy_result EQUAL 0)
        if(VERBOSE)
            message(STATUS "Copied: ${src_file} -> ${dest_path}")
        endif()
        set(${result_var} YES PARENT_SCOPE)
        return()
    else()
        message(WARNING "Failed to copy: ${src_file} -> ${dest_path}")
        set(${result_var} NO PARENT_SCOPE)
        return()
    endif()
endfunction()

# 主执行函数
function(main)
    # 验证参数
    validate_parameters()

    # 设置默认值
    set_defaults()

    # 创建目标目录
    create_target_directory()

    # 读取文件列表
    read_file_list(file_list)

    if(NOT file_list)
        message(WARNING "No files found in file list: ${FILE_LIST}")
        return()
    endif()

    if(VERBOSE)
        message(STATUS "Found ${file_list} files in file list")
    endif()

    # 验证源文件
    validate_source_files("${file_list}" valid_files missing_files)

    list(LENGTH valid_files valid_count)
    list(LENGTH missing_files missing_count)

    if(valid_count EQUAL 0)
        message(WARNING "No valid source files found")
        return()
    endif()

    if(missing_count GREATER 0)
        message(WARNING "${missing_count} source file(s) are missing")
    endif()

    # 复制文件
    set(success_count 0)
    set(failure_count 0)

    foreach(src_file ${valid_files})
        copy_single_file("${src_file}" is_success)
        if(is_success)
            math(EXPR success_count "${success_count} + 1")
        else()
            math(EXPR failure_count "${failure_count} + 1")
        endif()
    endforeach()

    # 输出结果摘要
    message(STATUS "Copy operation completed:")
    message(STATUS "  Successfully copied: ${success_count} file(s)")
    if(failure_count GREATER 0)
        message(STATUS "  Failed to copy: ${failure_count} file(s)")
    endif()
    if(missing_count GREATER 0)
        message(STATUS "  Missing source files: ${missing_count} file(s)")
    endif()

endfunction()

# 执行主函数
main()