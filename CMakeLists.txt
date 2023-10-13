cmake_minimum_required(VERSION 3.18)

project(PGRE2023)

add_subdirectory(libs/SDL-release-2.28.4)

add_executable(${PROJECT_NAME} src/main.cpp)
