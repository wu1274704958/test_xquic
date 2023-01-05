aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
set(CUR_TARGET_NAME test_ser)
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${COMM_INCLUDE_DIR} 
    ${xquic_INCLUDE_DIR}) 
target_link_libraries(${CUR_TARGET_NAME} PRIVATE 
    ${xquic_LIBRARY}
    ${OTH_LIBRARY}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("$<TARGET_FILE_DIR:${CUR_TARGET_NAME}>/xquic.dll")
   if(NOT EXISTS "$<TARGET_FILE_DIR:${CUR_TARGET_NAME}>/xquic.dll")
       add_custom_command(TARGET ${CUR_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${xquic_BIN_DIR}/ssl.dll" $<TARGET_FILE_DIR:${CUR_TARGET_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${xquic_BIN_DIR}/xquic.dll" $<TARGET_FILE_DIR:${CUR_TARGET_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${xquic_BIN_DIR}/crypto.dll" $<TARGET_FILE_DIR:${CUR_TARGET_NAME}>
            )
   endif()
endif()