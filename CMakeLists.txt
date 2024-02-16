# Specify required versions
cmake_minimum_required(VERSION 3.28)
set (CMAKE_CXX_STANDARD    20    CACHE    STRING    "C++ Version"    FORCE)
set (CXX_SCAN_FOR_MODULES    ON    CACHE    STRING    "Modules"    FORCE)

# Set the project (solution name)
project(
	EncosyGame
	VERSION 0.0.1
    DESCRIPTION "Game using EncosyEngine"
	LANGUAGES CXX
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Engine")

# Used sources
list(APPEND Game_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/Core/main.cppm"
)

add_executable(EncosyGame)
target_sources(EncosyGame PUBLIC FILE_SET CXX_MODULES FILES ${Game_SRC} BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Game/Core)
target_link_libraries(EncosyGame PUBLIC EncosyEngine)
set_property(TARGET EncosyGame PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Debug")

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(EncosyGame TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Game_SRC})



# Resource copying for debug build
set(TEXTURES_DIR "${PROJECT_SOURCE_DIR}/Game/Resources/Textures")
set(MODELS_DIR "${PROJECT_SOURCE_DIR}/Game/Resources/Models")
set(SHADERS_DIR "${PROJECT_SOURCE_DIR}/Game/Resources/Shaders")
set(DEBUG_BUILD_RESOURCES_DIR "${PROJECT_SOURCE_DIR}/Build/Debug/Game/Resources")


add_custom_command(
TARGET EncosyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEBUG_BUILD_RESOURCES_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURES_DIR}  "${DEBUG_BUILD_RESOURCES_DIR}/Textures"
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${MODELS_DIR}  "${DEBUG_BUILD_RESOURCES_DIR}/Models"
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADERS_DIR}  "${DEBUG_BUILD_RESOURCES_DIR}/Shaders"
)

add_custom_command(
TARGET EncosyGame POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/Build/Debug/Engine/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Engine/Build/Debug/Engine/" "${PROJECT_SOURCE_DIR}/Build/Debug/Engine/"
)
