# Specify required versions
cmake_minimum_required(VERSION 3.28)
set (CMAKE_CXX_STANDARD    20    CACHE    STRING    "C++ Version"    FORCE)
set (CXX_SCAN_FOR_MODULES    ON    CACHE    STRING    "Modules"    FORCE)

# Compile static libraries using multithreading.
add_compile_options(
    $<$<CONFIG:>:/MT>
    $<$<CONFIG:Debug>:/MTd>
    $<$<CONFIG:RelWithDebInfo>:/MTd>
    $<$<CONFIG:Release>:/MT>
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
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/CollisionDemo.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/DynamicDemo.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/DyingFollowerComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/DyingFollowerSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/FollowerComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/LeaderComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/LeaderMovementSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/SpawningSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/SpawningSystemData.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/MovementComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/FollowerMovementSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/SphereColliderComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/SphereCollisionSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/CollisionEventComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/FollowerModifySystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos/FollowerKillingSystem.ixx"
	
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/RotationTest/RotationTest.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/RotationTest/MovementComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/RotationTest/MovementSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game/RotationTest/MovementSystemThreaded.ixx"

	"${CMAKE_CURRENT_SOURCE_DIR}/Game/StaticTest/StaticTest.ixx"

)
# Used shaders, game
list(APPEND EncosyGame_SHADERS_SRC
)

add_executable(EncosyGame)
target_sources(EncosyGame PUBLIC FILE_SET CXX_MODULES FILES ${EncosyGame_SRC} 
BASE_DIRS 
	${CMAKE_CURRENT_SOURCE_DIR}/Game/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Game/RotationTest
	${CMAKE_CURRENT_SOURCE_DIR}/Game/StaticTest
	${CMAKE_CURRENT_SOURCE_DIR}/Game/CollisionDemos
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

add_custom_target(Game_Build_Setup
	COMMAND ${CMAKE_COMMAND} -E make_directory "${TEXTURES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODELS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Textures"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Models"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Shaders"
)
add_dependencies(EncosyGame Game_Build_Setup)
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
