include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-CapowCore
    tmp.cpp
)
target_link_libraries(test-CapowCore PUBLIC CapowCore GTest::gmock_main)
target_folder(test-CapowCore "Tests")
gtest_discover_tests(test-CapowCore)
