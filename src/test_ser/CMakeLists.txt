aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
set(CUR_TARGET_NAME test_ser)
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${COMM_INCLUDE_DIR}
    ${boringssl_INCLUDE_DIR}
)
target_link_libraries(${CUR_TARGET_NAME} PRIVATE 
    lsquic::lsquic
    ${OTH_LIBRARY}
    ${boringssl_LIBRARY}
    ZLIB::ZLIB
)

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/cert-def.pem")
#openssl req -newkey rsa:4096 -nodes -sha256 -keyout key-def.pem -x509 -days 365 -out cert-def.pem
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "certificate/cert-def.pem"   ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy "certificate/key-def.pem"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

endif()