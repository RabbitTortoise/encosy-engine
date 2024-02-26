# Specify required versions
cmake_minimum_required(VERSION 3.28)
set (CMAKE_CXX_STANDARD    20    CACHE    STRING    "C++ Version"    FORCE)
set (CXX_SCAN_FOR_MODULES    ON    CACHE    STRING    "Modules"    FORCE)

# Compile static libraries using multithreading.
add_compile_options(
    $<$<CONFIG:>:/MT> #---------|
    $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
    $<$<CONFIG:Release>:/MT> #--|
)

# Set the project (solution name)
project(
	EncosyGame
	VERSION 0.0.1
    DESCRIPTION "Game using EncosyEngine"
	LANGUAGES CXX
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Engine")

# Used sources
list(APPEND EncosyGame_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/Core/main.cppm"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/Demo/Demo.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/StressTest/StressTest.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/StressTest/MovementComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/StressTest/MovementSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/StressTest/MovementSystemThreaded.ixx"
)

# Used shaders, game
list(APPEND EncosyGame_SHADERS_SRC
)

add_executable(EncosyGame)
target_sources(EncosyGame PUBLIC FILE_SET CXX_MODULES FILES ${EncosyGame_SRC} 
BASE_DIRS 
	${CMAKE_CURRENT_SOURCE_DIR}/Game/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Game/StressTest
	${CMAKE_CURRENT_SOURCE_DIR}/Game/Demo
)
target_link_libraries(EncosyGame PUBLIC EncosyEngine)

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(EncosyGame TREE "${CMAKE_CURRENT_SOURCE_DIR}/Game" FILES ${EncosyGame_SRC})


set(CONFIGURATION_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
set_target_properties(EncosyGame PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY ${CONFIGURATION_BINARY_DIR})

# Resource copying for debug build
set(TEXTURES_DIR "${PROJECT_SOURCE_DIR}/Game/Resources/Textures")
set(MODELS_DIR "${PROJECT_SOURCE_DIR}/Game/Resources/Models")
set(SHADERS_DIR "${PROJECT_SOURCE_DIR}/Game/Resources/Shaders")
set(BUILD_RESOURCES_DIR "${CONFIGURATION_BINARY_DIR}/Game/Resources")

add_custom_command(
TARGET EncosyGame POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${TEXTURES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODELS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Textures"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Models"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Shaders"
)

# Shader building for the game is not yet supported
#include("${PROJECT_SOURCE_DIR}/Game/GameShaderBuild.cmake")
#APPEND_GLSL_TO_TARGET(EncosyGame ${PROJECT_SOURCE_DIR} ${BUILD_RESOURCES_DIR} "${EncosyGame_SHADERS_SRC}")

# Copy game resources to binary
add_custom_command(
TARGET EncosyGame POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CONFIGURATION_BINARY_DIR}/Game/Resources/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Game/Resources/" "${CONFIGURATION_BINARY_DIR}/Game/Resources"
)

# Copy engine resources to binary
add_custom_command(
TARGET EncosyGame POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CONFIGURATION_BINARY_DIR}/Engine/Resources/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Engine/Resources/" "${CONFIGURATION_BINARY_DIR}/Engine/Resources"
)
