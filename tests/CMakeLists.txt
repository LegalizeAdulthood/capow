include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-CapowCore
    AlpakaUtilities_test.cpp
    ImageBuffer_test.cpp
    WavePlaneImage_test.cpp
)
target_include_directories(test-CapowCore PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(test-CapowCore PUBLIC CapowCore GTest::gmock_main)
target_folder(test-CapowCore "Tests")
gtest_discover_tests(test-CapowCore)
