function(copy_files_to_destination)
    set(options)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs FILES)
    cmake_parse_arguments(COPY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT COPY_FILES)
        message(WARNING "No files specified for copying")
        return()
    endif()
    
    if(NOT COPY_DESTINATION)
        message(FATAL_ERROR "Destination path not specified")
    endif()
    
    file(MAKE_DIRECTORY "${COPY_DESTINATION}")
    
    foreach(file IN LISTS COPY_FILES)

        get_filename_component(abs_file "${file}" ABSOLUTE)

        if(NOT EXISTS "${abs_file}")
            message(WARNING "File not found: ${abs_file}")
            continue()
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${abs_file}"
                "${COPY_DESTINATION}"
            RESULT_VARIABLE result
        )
        
        if(NOT result EQUAL 0)
            message(WARNING "Failed to copy ${abs_file} to ${COPY_DESTINATION}")
        endif()
    endforeach()
    
    message(STATUS "Copied ${CMAKE_MATCH_COUNT} files to ${COPY_DESTINATION}")
endfunction()

macro(bulk_copy_files TARGET_NAME DEST_DIR PHASE FILE_LIST)
    if(NOT PHASE)
        set(PHASE POST_BUILD)
    endif()
    
    set(valid_times PRE_BUILD PRE_LINK POST_BUILD)
    if(NOT PHASE IN_LIST valid_times)
        message(FATAL_ERROR "Invalid execution time: ${PHASE}. Valid options: PRE_BUILD, PRE_LINK, POST_BUILD")
    endif()
    
    foreach(file IN LISTS ${FILE_LIST})
        add_custom_command(TARGET ${TARGET_NAME}
            ${PHASE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${file}"
                "${DEST_DIR}"
            COMMENT "Copying ${file} to ${DEST_DIR}"
        )
    endforeach()
endmacro()