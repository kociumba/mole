# This is taken with modifications from the bgfx cmake project here: https://github.com/bkaradzic/bgfx.cmake

function(_mole_bin2c_parse ARG_OUT)
    set(options "")
    set(oneValueArgs INPUT_FILE;OUTPUT_FILE;ARRAY_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
    set(CLI "")

    if (ARG_INPUT_FILE)
        list(APPEND CLI "${ARG_INPUT_FILE}")
    else ()
        message(SEND_ERROR "Call to _mole_bin2c_parse() must have an INPUT_FILE")
    endif ()

    if (ARG_OUTPUT_FILE)
        list(APPEND CLI "${ARG_OUTPUT_FILE}")
    else ()
        message(SEND_ERROR "Call to _mole_bin2c_parse() must have an OUTPUT_FILE")
    endif ()
    
    if (ARG_ARRAY_NAME)
        list(APPEND CLI "${ARG_ARRAY_NAME}")
    else ()
        get_filename_component(INPUT_BASENAME "${ARG_INPUT_FILE}" NAME_WE)
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" ARRAY_NAME "${INPUT_BASENAME}")
        list(APPEND CLI "${ARRAY_NAME}")
    endif ()

    set(${ARG_OUT} ${CLI} PARENT_SCOPE)
endfunction()

function(mole_compile_binary_to_header)
    set(options "")
    set(oneValueArgs INPUT_FILE;OUTPUT_FILE;ARRAY_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
    _mole_bin2c_parse(
            CLI
            INPUT_FILE ${ARG_INPUT_FILE}
            OUTPUT_FILE ${ARG_OUTPUT_FILE}
            ARRAY_NAME ${ARG_ARRAY_NAME}
    )
    add_custom_command(
            OUTPUT ${ARG_OUTPUT_FILE}
            COMMAND $<TARGET_FILE:bin2c> ${CLI}
            MAIN_DEPENDENCY ${ARG_INPUT_FILE}
            DEPENDS bin2c
    )
endfunction()