cmake_minimum_required(VERSION 3.26)

set(CAPOW_PROJECT_NAME "CAPow" CACHE STRING "Project name for IDEs")
project("${CAPOW_PROJECT_NAME}")

add_subdirectory(source)

add_custom_target(Miscellaneous)
target_sources(Miscellaneous
    PRIVATE
        ".clang-format"
        ".editorconfig"
        ".github/workflows/cmake.yml"
        ".gitignore"
        "CMakeLists.txt"
        "CMakePresets.json"
        "README.md"
        "LICENSE"
)
source_group("Editing" FILES
    ".clang-format"
    ".editorconfig"
)
source_group("Git" FILES
    ".gitignore"
)
source_group("GitHub" FILES
    ".github/workflows/cmake.yml"
    "LICENSE"
)
source_group("Documentation" FILES
    "README.md"
)
source_group("CMake" FILES
    "CMakeLists.txt"
    "CMakePresets.json"
)
