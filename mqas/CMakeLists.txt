aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/io" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/core" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/comm" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/core/proto" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/tools/proto" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/tools/stream" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/tools/model" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/tools/controller" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/tools" CUR_SRC_FILES)

set(CUR_TARGET_NAME mqas)
if(BUILD_MQAS_LIB_OBJ)
    message(FATAL_ERROR "mqas only support shared library,beacuse depeney easylogging++")
    add_library(${CUR_TARGET_NAME} OBJECT ${CUR_SRC_FILES})
    target_compile_definitions(${CUR_TARGET_NAME} PRIVATE MQAS_NO_EXPORT)
    set_target_properties(${CUR_TARGET_NAME} PROPERTIES 
        INTERFACE_COMPILE_DEFINITIONS "MQAS_NO_EXPORT=1")
else()
    message("MQAS_BUILD_SHARED_LIBS = ${MQAS_BUILD_SHARED_LIBS}")
    if(MQAS_BUILD_SHARED_LIBS)
        add_library(${CUR_TARGET_NAME} SHARED ${CUR_SRC_FILES})
        target_compile_definitions(${CUR_TARGET_NAME} PRIVATE BUILDING_MQAS_SHARED ELPP_EXPORT_SYMBOLS)
        set_target_properties(${CUR_TARGET_NAME} PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "MQAS_SHARED=1")
    else()
        message(FATAL_ERROR "mqas only support shared library,beacuse depeney easylogging++")
        add_library(${CUR_TARGET_NAME} STATIC ${CUR_SRC_FILES})
        target_compile_definitions(${CUR_TARGET_NAME} PRIVATE MQAS_NO_EXPORT)
        set_target_properties(${CUR_TARGET_NAME} PROPERTIES 
            INTERFACE_COMPILE_DEFINITIONS "MQAS_NO_EXPORT=1")
    endif()
endif()

add_dependencies(${CUR_TARGET_NAME} lsquic easyloggingpp)

set(mqas_dependencies_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" ${easyloggingpp_INCLUDE_DIR} ${protobuf_INCLUDE_DIR})

set_target_properties(${CUR_TARGET_NAME} PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES ${mqas_dependencies_INCLUDE_DIR})


target_include_directories(${CUR_TARGET_NAME} PUBLIC 
    ${boringssl_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${easyloggingpp_INCLUDE_DIR}
    ${protobuf_INCLUDE_DIR}
)

target_link_libraries(${CUR_TARGET_NAME} PUBLIC 
    lsquic::lsquic
    lsquic
    ${OTH_LIBRARY}
    ${boringssl_LIBRARY}
    ZLIB::ZLIB
    libuv::uv
    toml11::toml11
    easyloggingpp
    protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
    sigc-3.0
)

set(mqas_binary_dependencies ${boringssl_BINARY})

find_file(zlib_BINARY "zlib1${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS ${VCPKG_BINARAY_DIR})
if(zlib_BINARY)
    list(APPEND mqas_binary_dependencies ${zlib_BINARY})
endif()

include(CheckTargetType)

get_target_property(mqas_LINK_LIBRARIES mqas LINK_LIBRARIES)

foreach(lib ${mqas_LINK_LIBRARIES})
    check_target_is_shared(${lib} isSharedLib)
    if(isSharedLib)
        list(FIND mqas_binary_dependencies $<TARGET_FILE:${lib}> found_index)
        if(found_index EQUAL -1)
            list(APPEND mqas_binary_dependencies $<TARGET_FILE:${lib}>)
        endif ()
    endif ()
    if(TARGET ${lib})
        get_target_property(temp_LINK_LIBRARIES ${lib} LINK_LIBRARIES)
        if(temp_LINK_LIBRARIES)
            foreach(lib ${temp_LINK_LIBRARIES})
                check_target_is_shared(${lib} isSharedLib)
                if(isSharedLib)
                    list(FIND mqas_binary_dependencies $<TARGET_FILE:${lib}> found_index)
                    if(found_index EQUAL -1)
                        list(APPEND mqas_binary_dependencies $<TARGET_FILE:${lib}>)
                    endif ()
                endif ()
            endforeach ()
        endif ()
    endif ()
endforeach ()

add_subdirectory(proto)

set(index 0)

foreach (lib ${mqas_binary_dependencies})
    if(index EQUAL 0)
        set(mode "OVERWRITE")
    else()
        set(mode "APPEND")
    endif()
    add_custom_command(TARGET mqas POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            -D TARGET_FILE="$<TARGET_FILE_DIR:mqas>/dependencies.ini"
            -D Content="${lib}"
            -D MODE=${mode}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/WriteFile.cmake"
            COMMENT "Writing mqas binary dependencies"
    )
    math(EXPR index "${index} + 1")
endforeach ()

