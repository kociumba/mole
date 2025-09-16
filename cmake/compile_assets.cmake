# This is taken with modifications from the bgfx cmake project here: https://github.com/bkaradzic/bgfx.cmake

function(_mole_bin2c_parse ARG_OUT)
    set(options "")
    set(oneValueArgs INPUT_FILE OUTPUT_BASE ARRAY_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
    set(CLI "")

    if (ARG_INPUT_FILE)
        list(APPEND CLI "${ARG_INPUT_FILE}")
    else ()
        message(SEND_ERROR "Call to _mole_bin2c_parse() must have an INPUT_FILE")
    endif ()

    if (ARG_OUTPUT_BASE)
        list(APPEND CLI "${ARG_OUTPUT_BASE}")
    else ()
        message(SEND_ERROR "Call to _mole_bin2c_parse() must have an OUTPUT_BASE")
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
    set(oneValueArgs INPUT_FILE OUTPUT_BASE ARRAY_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
    _mole_bin2c_parse(
            CLI
            INPUT_FILE ${ARG_INPUT_FILE}
            OUTPUT_BASE ${ARG_OUTPUT_BASE}
            ARRAY_NAME ${ARG_ARRAY_NAME}
    )

    set(OUTPUT_C "${ARG_OUTPUT_BASE}.c")
    set(OUTPUT_H "${ARG_OUTPUT_BASE}.h")

    add_custom_command(
            OUTPUT ${OUTPUT_C} ${OUTPUT_H}
            COMMAND bin2c ${CLI}
            MAIN_DEPENDENCY ${ARG_INPUT_FILE}
            COMMENT "Converting ${ARG_INPUT_FILE} to C array in ${OUTPUT_C} and ${OUTPUT_H}"
    )

    set_source_files_properties(${OUTPUT_C} ${OUTPUT_H} PROPERTIES
            GENERATED TRUE
            SKIP_PRECOMPILE_HEADERS TRUE
    )

    set(${ARG_ARRAY_NAME}_C ${OUTPUT_C} PARENT_SCOPE)
    set(${ARG_ARRAY_NAME}_H ${OUTPUT_H} PARENT_SCOPE)
endfunction()