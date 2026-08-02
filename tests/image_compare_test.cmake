function(require_variable name)
    if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
        message(FATAL_ERROR "${name} is required")
    endif()
endfunction()

function(run_command description)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE commandResult
        OUTPUT_VARIABLE commandOutput
        ERROR_VARIABLE commandError
    )
    if(commandResult)
        message(FATAL_ERROR
            "${description} failed with exit ${commandResult}\n"
            "stdout:\n${commandOutput}\n"
            "stderr:\n${commandError}"
        )
    endif()
endfunction()

require_variable(IMAGE_COMPARE)
require_variable(EXPECTED_IMAGE)
require_variable(ACTUAL_IMAGE)
require_variable(DIFF_IMAGE)
require_variable(EXPECTED_RESULT)

file(REMOVE "${DIFF_IMAGE}")

if(DEFINED EXPECTED_VARIANT)
    file(REMOVE "${EXPECTED_IMAGE}")
    run_command("expected image generation"
        "${IMAGE_COMPARE}" --write-test-image "${EXPECTED_IMAGE}"
        --variant "${EXPECTED_VARIANT}"
    )
endif()

if(DEFINED ACTUAL_VARIANT)
    file(REMOVE "${ACTUAL_IMAGE}")
    run_command("actual image generation"
        "${IMAGE_COMPARE}" --write-test-image "${ACTUAL_IMAGE}"
        --variant "${ACTUAL_VARIANT}"
    )
endif()

execute_process(
    COMMAND "${IMAGE_COMPARE}"
        --expected "${EXPECTED_IMAGE}"
        --actual "${ACTUAL_IMAGE}"
        --diff "${DIFF_IMAGE}"
    RESULT_VARIABLE compareResult
    OUTPUT_VARIABLE compareOutput
    ERROR_VARIABLE compareError
)

if(NOT compareResult EQUAL EXPECTED_RESULT)
    message(FATAL_ERROR
        "expected image-compare exit ${EXPECTED_RESULT}, got "
        "${compareResult}\n"
        "stdout:\n${compareOutput}\n"
        "stderr:\n${compareError}"
    )
endif()

if(EXPECTED_RESULT AND NOT EXISTS "${DIFF_IMAGE}")
    message(FATAL_ERROR "diff image was not written: ${DIFF_IMAGE}")
endif()
