function(require_variable name)
    if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
        message(FATAL_ERROR "${name} is required")
    endif()
endfunction()

function(format_command output)
    set(commandText "")
    foreach(commandArg IN LISTS ARGN)
        if(commandText STREQUAL "")
            set(commandText "\"${commandArg}\"")
        else()
            string(APPEND commandText " \"${commandArg}\"")
        endif()
    endforeach()
    set(${output} "${commandText}" PARENT_SCOPE)
endfunction()

function(run_command description)
    format_command(commandText ${ARGN})
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE commandResult
        OUTPUT_VARIABLE commandOutput
        ERROR_VARIABLE commandError
    )
    if(commandResult)
        message(FATAL_ERROR
            "${description} failed with exit ${commandResult}\n"
            "command:\n${commandText}\n"
            "stdout:\n${commandOutput}\n"
            "stderr:\n${commandError}"
        )
    endif()
endfunction()

require_variable(CAPOW_EXECUTABLE)
require_variable(IMAGE_COMPARE)
require_variable(RULE)
require_variable(STEPS)
require_variable(SEED)
require_variable(CPU_IMAGE)
require_variable(GPU_IMAGE)
require_variable(DIFF_IMAGE)
set(toleranceArguments "")
if(DEFINED TOLERANCE AND NOT "${TOLERANCE}" STREQUAL "")
    list(APPEND toleranceArguments --tolerance "${TOLERANCE}")
endif()

file(REMOVE "${CPU_IMAGE}" "${GPU_IMAGE}" "${DIFF_IMAGE}")

run_command("CPU batch image generation"
    "${CAPOW_EXECUTABLE}"
    --batch
    --backend cpu
    --rule "${RULE}"
    --steps "${STEPS}"
    --seed "${SEED}"
    --output "${CPU_IMAGE}"
)

run_command("GPU batch image generation"
    "${CAPOW_EXECUTABLE}"
    --batch
    --backend gpu
    --rule "${RULE}"
    --steps "${STEPS}"
    --seed "${SEED}"
    --output "${GPU_IMAGE}"
)

format_command(compareCommand
    "${IMAGE_COMPARE}"
    --expected "${CPU_IMAGE}"
    --actual "${GPU_IMAGE}"
    --diff "${DIFF_IMAGE}"
    ${toleranceArguments}
)

execute_process(
    COMMAND "${IMAGE_COMPARE}"
        --expected "${CPU_IMAGE}"
        --actual "${GPU_IMAGE}"
        --diff "${DIFF_IMAGE}"
        ${toleranceArguments}
    RESULT_VARIABLE compareResult
    OUTPUT_VARIABLE compareOutput
    ERROR_VARIABLE compareError
)

if(compareResult)
    if(NOT EXISTS "${DIFF_IMAGE}")
        message(FATAL_ERROR
            "image-compare failed with exit ${compareResult} and did not "
            "write ${DIFF_IMAGE}\n"
            "command:\n${compareCommand}\n"
            "stdout:\n${compareOutput}\n"
            "stderr:\n${compareError}"
        )
    endif()
    message(FATAL_ERROR
        "image-compare failed with exit ${compareResult}\n"
        "command:\n${compareCommand}\n"
        "stdout:\n${compareOutput}\n"
        "stderr:\n${compareError}\n"
        "diff:\n${DIFF_IMAGE}"
    )
endif()
