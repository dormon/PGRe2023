cmake_minimum_required(VERSION 3.18)

project(PGRE2023)
SET(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 


option(SDL_SHARED "" OFF)
option(SDL_STATIC "" ON)
add_subdirectory(libs/SDL-release-2.28.4)

add_subdirectory(libs/geGL)

add_executable(${PROJECT_NAME} 
  src/main.cpp
  src/bunny.cpp
  src/bunny.hpp)

target_link_libraries(${PROJECT_NAME} SDL2::SDL2 SDL2::SDL2main geGL::geGL)
target_include_directories(${PROJECT_NAME} PUBLIC src/)

add_custom_target(run ./PGRE2023)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
