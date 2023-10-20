cmake_minimum_required(VERSION 3.18)

project(PGRE2023)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 

add_subdirectory(libs/SDL-release-2.28.4)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} SDL2::SDL2 SDL2::SDL2main)

add_custom_target(run ./PGRE2023)
