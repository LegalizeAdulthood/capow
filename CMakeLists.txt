cmake_minimum_required(VERSION 3.26)

set(CAPOW_PROJECT_NAME "CAPow" CACHE STRING "Project name for IDEs")
project("${CAPOW_PROJECT_NAME}")

add_subdirectory(source)
