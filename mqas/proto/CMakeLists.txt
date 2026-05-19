if(TARGET protobuf::protoc)

find_program(BASH_EXECUTABLE
    NAMES bash
    REQUIRED
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. common.proto
    COMMAND ${BASH_EXECUTABLE} gen_common.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common.proto protobuf::protoc
    COMMENT "Generating common proto"
)

add_custom_target(mqas_proto_common ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.cc
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/msg_wrapper.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/msg_wrapper.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. msg_wrapper.proto
    COMMAND ${BASH_EXECUTABLE} gen_msg_wrapper.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/msg_wrapper.proto protobuf::protoc
    COMMENT "Generating msg_wrapper proto"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/p2p.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/p2p.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. p2p.proto
    COMMAND ${BASH_EXECUTABLE} gen_p2p.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/p2p.proto protobuf::protoc
        ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.h
    COMMENT "Generating p2p proto"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/relay.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/relay.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. relay.proto
    COMMAND ${BASH_EXECUTABLE} gen_relay.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/relay.proto protobuf::protoc
        ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.h
    COMMENT "Generating relay proto"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/send_file.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/send_file.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. send_file.proto
    COMMAND ${BASH_EXECUTABLE} gen_send_file.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/send_file.proto protobuf::protoc
    COMMENT "Generating send_file proto"
)

add_custom_target(mqas_proto_msg_wrapper ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/msg_wrapper.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/msg_wrapper.pb.cc
)

add_custom_target(mqas_proto_p2p ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/p2p.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/p2p.pb.cc
)
add_dependencies(mqas_proto_p2p mqas_proto_common)

add_custom_target(mqas_proto_relay ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/relay.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/relay.pb.cc
)
add_dependencies(mqas_proto_relay mqas_proto_common)

add_custom_target(mqas_proto_send_file ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/send_file.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/send_file.pb.cc
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/p2p_client.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/p2p_client.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. p2p_client.proto
    COMMAND ${BASH_EXECUTABLE} gen_p2p_client.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/p2p_client.proto protobuf::protoc
        ${CMAKE_CURRENT_SOURCE_DIR}/common.pb.h
    COMMENT "Generating p2p_client proto"
)

add_custom_target(mqas_proto_p2p_client ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/p2p_client.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/p2p_client.pb.cc
)
add_dependencies(mqas_proto_p2p_client mqas_proto_common)

endif()