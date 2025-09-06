if(TARGET protobuf::protoc)

find_program(BASH_EXECUTABLE
    NAMES bash
    REQUIRED
)


add_custom_target(mqas_proto_msg_wrapper ALL
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. msg_wrapper.proto
    COMMAND ${BASH_EXECUTABLE} gen_msg_wrapper.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(mqas_proto_p2p ALL
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. p2p.proto
    COMMAND ${BASH_EXECUTABLE} gen_p2p.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(mqas_proto_relay ALL
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. relay.proto
    COMMAND ${BASH_EXECUTABLE} gen_relay.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(mqas_proto_send_file ALL
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=dllexport_decl=MQAS_EXTERN:. send_file.proto
    COMMAND ${BASH_EXECUTABLE} gen_send_file.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(mqas_proto_msg_wrapper protobuf::protoc)
add_dependencies(mqas_proto_p2p protobuf::protoc)
add_dependencies(mqas_proto_relay protobuf::protoc)
add_dependencies(mqas_proto_send_file protobuf::protoc)

endif()