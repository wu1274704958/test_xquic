aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
set(CUR_TARGET_NAME pb_stream_cli)
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas easyloggingpp)
target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${boringssl_INCLUDE_DIR}
    ${easyloggingpp_INCLUDE_DIR}
)
target_link_libraries(${CUR_TARGET_NAME} PRIVATE
        lsquic::lsquic
        lsquic
        ${OTH_LIBRARY}
        ${boringssl_LIBRARY}
        ZLIB::ZLIB
        $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
        mqas
        easyloggingpp
        protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
        sigc-3.0
)


add_custom_command(
    OUTPUT say_hello.pb.cc say_hello.pb.h
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=. say_hello.proto
    COMMAND ${CMAKE_COMMAND} -E copy_if_different say_hello.pb.cc ../pb_stream_ser/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different say_hello.pb.h ../pb_stream_ser/
    DEPENDS say_hello.proto
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(test_pb_stream_proto DEPENDS say_hello.pb.cc say_hello.pb.h)

add_dependencies(${CUR_TARGET_NAME} test_pb_stream_proto)

include(fileop)

bulk_copy_files(${CUR_TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR} POST_BUILD mqas_dependencies_BINARY)

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()