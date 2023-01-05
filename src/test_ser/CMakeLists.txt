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