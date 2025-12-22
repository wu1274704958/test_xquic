aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(CUR_TARGET_NAME ${CURRENT_DIR_NAME})
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas)

target_link_libraries(${CUR_TARGET_NAME} PRIVATE
    mqas
)

add_custom_command(TARGET ${CUR_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -D TARGET_DIR="$<TARGET_FILE_DIR:${CUR_TARGET_NAME}>"
        -D FILE_LIST="$<TARGET_FILE_DIR:mqas>/dependencies.ini"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyFiles.cmake"
        COMMENT "Copying mqas dependencies binary files"
)

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()