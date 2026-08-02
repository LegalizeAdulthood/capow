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
        AlpakaHeat2D_test.cpp
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

function(add_image_compare_test name actualVariant expectedResult)
    add_test(NAME imageCompare.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DIMAGE_COMPARE=$<SHELL_PATH:$<TARGET_FILE:image-compare>>"
            "-DEXPECTED_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-expected.bmp"
            "-DACTUAL_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-actual.bmp"
            "-DDIFF_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/${name}-diff.bmp"
            "-DEXPECTED_VARIANT=base"
            "-DACTUAL_VARIANT=${actualVariant}"
            "-DEXPECTED_RESULT=${expectedResult}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/image_compare_test.cmake"
    )
    set_tests_properties(imageCompare.${name} PROPERTIES LABELS "image")
endfunction()

add_image_compare_test(equalImagesPass base 0)
add_image_compare_test(differentImagesFail different 1)
