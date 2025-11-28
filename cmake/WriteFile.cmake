# WriteFile.cmake - 文件写入脚本
# 用法: cmake -D TARGET_FILE="filename" -D Content="content" -P WriteFile.cmake
#
# 参数:
#   TARGET_FILE - 目标文件名（必需）
#   Content     - 要写入的内容（必需）
#   MODE        - 写入模式: APPEND（追加，默认）或 OVERWRITE（覆盖）

cmake_minimum_required(VERSION 3.10)

# 参数验证
if(NOT TARGET_FILE)
    message(FATAL_ERROR "TARGET_FILE parameter is required")
endif()

if(NOT DEFINED Content)
    message(FATAL_ERROR "Content parameter is required")
endif()

# 设置默认值
if(NOT DEFINED MODE)
    set(MODE "APPEND")
endif()

if(NOT DEFINED TIMESTAMP)
    set(TIMESTAMP "OFF")
endif()

# 获取文件绝对路径
get_filename_component(target_file_abs "${TARGET_FILE}" ABSOLUTE)

# 创建目录（如果不存在）
get_filename_component(target_dir "${target_file_abs}" DIRECTORY)
if(NOT EXISTS "${target_dir}")
    file(MAKE_DIRECTORY "${target_dir}")
    message(STATUS "Created directory: ${target_dir}")
endif()

# 准备要写入的内容
set(final_content "${Content};")

# 执行文件写入操作
if(MODE STREQUAL "OVERWRITE")
    # 覆盖模式
    file(WRITE "${target_file_abs}" "${final_content}")
    message(STATUS "File overwritten: ${target_file_abs}")

elseif(MODE STREQUAL "APPEND")
    # 追加模式
    if(EXISTS "${target_file_abs}")
        file(APPEND "${target_file_abs}" "${final_content}")
        message(STATUS "Content appended to: ${target_file_abs}")
    else()
        file(WRITE "${target_file_abs}" "${final_content}")
        message(STATUS "File created with content: ${target_file_abs}")
    endif()

else()
    message(FATAL_ERROR "Invalid MODE: ${MODE}. Use APPEND or OVERWRITE")
endif()
