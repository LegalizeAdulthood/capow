cmake_minimum_required(VERSION 3.26)

set(CAPOW_PROJECT_NAME "CAPow" CACHE STRING "Project name for IDEs")
project("${CAPOW_PROJECT_NAME}" VERSION 8.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(misc_target)
include(set_cxx_standard)
include(target_folder)
include(vs_startup_project)
include(CTest)

add_subdirectory(source)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

misc_target(FILES
    "CMakePresets.json"
)
misc_group_sources("Editing" FILES
    ".clang-format"
    ".editorconfig"
)
misc_group_sources("Git" FILES
    ".gitignore"
    ".gitmodules"
)
misc_group_sources("GitHub" FILES
    ".github/workflows/cmake.yml"
    "LICENSE"
)
misc_group_sources("Documentation" FILES "README.md")
misc_group_sources("CMake" FILES
    "CMakePresets.json"
    "cmake/misc_target.cmake"
    "cmake/set_cxx_standard.cmake"
    "cmake/target_folder.cmake"
    "cmake/vs_startup_project.cmake"
)
misc_group_sources("VcPkg" FILES "vcpkg.json")
