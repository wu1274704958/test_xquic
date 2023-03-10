aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
set(CUR_TARGET_NAME send_datagram)
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas)
target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${boringssl_INCLUDE_DIR}
)
target_link_libraries(${CUR_TARGET_NAME} PRIVATE
        lsquic::lsquic
        ${OTH_LIBRARY}
        ${boringssl_LIBRARY}
        ZLIB::ZLIB
        $<IF:$<TARGET_EXISTS:uv_a>,uv_a,uv>
        mqas
        ${easyloggingpp_LIBRARY}
)

get_target_property(mqas_TARGET_TYPE mqas TYPE)
if(mqas_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET ${CUR_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mqas> ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()