include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-CapowCore
    AlpakaUtilities_test.cpp
    BatchOptions_test.cpp
    CapowRules_test.cpp
    ImageBuffer_test.cpp
    WavePlaneImage_test.cpp
)
target_include_directories(test-CapowCore PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(test-CapowCore PUBLIC CapowCore GTest::gmock_main)
if(CAPOW_ENABLE_ALPAKA)
    target_sources(test-CapowCore PRIVATE
        AlpakaBackend_test.cpp
        AlpakaBuffers_test.cpp
        AlpakaHeat1D_test.cpp
        AlpakaHeat2D_test.cpp
        AlpakaTiming_test.cpp
        AlpakaWave1D_test.cpp
        AlpakaWave2D_test.cpp
        AlpakaSyntheticHeat_test.cpp)
    target_link_libraries(test-CapowCore PUBLIC CapowAlpaka)

    add_executable(heat2d-timing
        heat2d-timing.cpp
    )
    target_include_directories(heat2d-timing PRIVATE "${PROJECT_SOURCE_DIR}/source")
    target_link_libraries(heat2d-timing PUBLIC CapowAlpaka CapowCore)
    target_folder(heat2d-timing "Tests")
endif()
target_folder(test-CapowCore "Tests")
gtest_discover_tests(test-CapowCore)

add_executable(image-compare
    image-compare.cpp
)
target_folder(image-compare "Tests")

if(CAPOW_ENABLE_ALPAKA)
    function(add_capow_batch_image_compare_test name rule steps seed)
        set(wrapArgument "")
        if(ARGC GREATER 4 AND NOT "${ARGV4}" STREQUAL "")
            set(wrapArgument "-DWRAP_MODE=${ARGV4}")
        endif()
        set(toleranceArgument "")
        if(ARGC GREATER 5)
            set(toleranceArgument "-DTOLERANCE=${ARGV5}")
        endif()
        add_test(NAME capowBatchImageCompare.${name}
            COMMAND ${CMAKE_COMMAND}
                "-DCAPOW_EXECUTABLE=$<SHELL_PATH:$<TARGET_FILE:Capow>>"
                "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
                "-DRULE=${rule}"
                "-DSTEPS=${steps}"
                "-DSEED=${seed}"
                "-DCPU_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-cpu.bmp"
                "-DGPU_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-gpu.bmp"
                "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-diff.bmp"
                ${wrapArgument}
                ${toleranceArgument}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/ImageCompareTest.cmake"
        )
        set_tests_properties(capowBatchImageCompare.${name} PROPERTIES
            LABELS "image"
        )
    endfunction()

    add_capow_batch_image_compare_test(syntheticHeat2D SYNTHETIC_HEAT_2D 40 1946)
    add_capow_batch_image_compare_test(caHeat2DWrap CA_HEAT_2D 40 1946 wrap)
    add_capow_batch_image_compare_test(caHeat2DFree CA_HEAT_2D 40 1946 free)
    add_capow_batch_image_compare_test(caHeat2DAbsorb CA_HEAT_2D 40 1946 absorb)
    add_capow_batch_image_compare_test(caHeat2DZero CA_HEAT_2D 40 1946 zero)
    add_capow_batch_image_compare_test(caHeat2DFixed CA_HEAT_2D 40 1946 fixed)
    add_capow_batch_image_compare_test(caWave2DWrap CA_WAVE_2D 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caHeatwaveWrap CA_HEATWAVE 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caHeatwave2Wrap CA_HEATWAVE2 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caWave1DWrap CA_WAVE 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caWave2Wrap CA_WAVE2 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caOscillatorWrap CA_OSCILLATOR 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caDiverseOscillatorWrap CA_DIVERSE_OSCILLATOR 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caOscillatorWaveWrap ALT_CA_OSCILLATOR_WAVE 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caDiverseOscillatorWaveWrap ALT_CA_DIVERSE_OSCILLATOR_WAVE 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caUlamWrap CA_ULAM_WAVE 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caAutoUlamWrap CA_AUTO_ULAM_WAVE 40 1946 wrap 1)
    add_capow_batch_image_compare_test(caCubicUlamWrap CA_CUBIC_ULAM_WAVE 40 1946 wrap 1)
endif()
