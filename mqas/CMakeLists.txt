aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/io" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/core" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/comm" CUR_SRC_FILES)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src/core/proto" CUR_SRC_FILES)

set(CUR_TARGET_NAME mqas)
if(BUILD_MQAS_LIB_OBJ)
    add_library(${CUR_TARGET_NAME} OBJECT ${CUR_SRC_FILES})
    target_compile_definitions(${CUR_TARGET_NAME} PRIVATE MQAS_NO_EXPORT)
    set_target_properties(${CUR_TARGET_NAME} PROPERTIES 
        INTERFACE_COMPILE_DEFINITIONS "MQAS_NO_EXPORT=1")
else()
    message("BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
    if(BUILD_SHARED_LIBS)
        add_library(${CUR_TARGET_NAME} SHARED ${CUR_SRC_FILES})
        target_compile_definitions(${CUR_TARGET_NAME} PRIVATE BUILDING_MQAS_SHARED ELPP_AS_DLL ELPP_EXPORT_SYMBOLS)
    else()
        add_library(${CUR_TARGET_NAME} STATIC ${CUR_SRC_FILES})
        target_compile_definitions(${CUR_TARGET_NAME} PRIVATE MQAS_NO_EXPORT)
        set_target_properties(${CUR_TARGET_NAME} PROPERTIES 
            INTERFACE_COMPILE_DEFINITIONS "MQAS_NO_EXPORT=1")
    endif()
endif()

set_target_properties(${CUR_TARGET_NAME} PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")


target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${boringssl_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${easyloggingpp_INCLUDE_DIR}
)

target_link_libraries(${CUR_TARGET_NAME} PRIVATE 
    lsquic::lsquic
    ${OTH_LIBRARY}
    ${boringssl_LIBRARY}
    ZLIB::ZLIB
    $<IF:$<TARGET_EXISTS:uv_a>,uv_a,uv>
    toml11::toml11
    ${easyloggingpp_LIBRARY}
)