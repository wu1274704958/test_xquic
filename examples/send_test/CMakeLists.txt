aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
set(CUR_TARGET_NAME send_test)
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas)
target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${boringssl_INCLUDE_DIR}
)
target_link_libraries(${CUR_TARGET_NAME} PRIVATE 
    lsquic::lsquic
    lsquic
    ${OTH_LIBRARY}
    ${boringssl_LIBRARY}
    ZLIB::ZLIB
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    mqas
    sigc-3.0
    protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
)

include(fileop)

bulk_copy_files(${CUR_TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR} POST_BUILD mqas_dependencies_BINARY)
