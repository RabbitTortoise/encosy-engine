# Specify minimum supported cmake version and set c++23
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
	Encosy
	VERSION 24.2.25
    DESCRIPTION "Data-oriented game engine"
	LANGUAGES CXX
)

#GLM options
set (GLM_ENABLE_CXX_20 "Enable C++ 20" ON    CACHE    BOOL    "GLM"    FORCE)

#GLM options
set (FMT_MODULE    OFF    CACHE    BOOL    "FMT"    FORCE)

# Assimp options
set (ASSIMP_BUILD_ZLIB    ON    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_TESTS    OFF    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_ASSIMP_TOOLS    OFF    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT    OFF    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT    OFF    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_FBX_IMPORTER    ON    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_FBX_EXPORTER    ON    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_OBJ_IMPORTER    ON    CACHE    BOOL    "ASSIMP"    FORCE)
set (ASSIMP_BUILD_OBJ_EXPORTER    ON    CACHE    BOOL    "ASSIMP"    FORCE)

# SDL options
set (SDL_AUDIO    OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_DISKAUDIO    OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_DUMMYAUDIO    OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_DUMMYVIDEO    OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_HAPTIC   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_JOYSTICK   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_HIDAPI   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_HIDAPI_JOYSTICK   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_HIDAPI_LIBUSB   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_HIDAPI_LIBUSB_SHARED   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_OPENGLES   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_RENDER   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_RENDER_D3D   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_RENDER_METAL   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_SENSOR   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_TEST_LIBRARY   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_VIDEO_CAPTURE   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_VIRTUAL_JOYSTICK   OFF    CACHE    BOOL    "SDL3"    FORCE)

set (SDL_VIDEO   ON    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_OPENGL   ON    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_VULKAN   ON    CACHE    BOOL    "SDL3"    FORCE)

# Shared / Static
set (SDL_STATIC   ON    CACHE    BOOL    "SDL3"    FORCE)
set (BUILD_SHARED_LIBS   OFF    CACHE    BOOL    "SDL3"    FORCE)
set (SDL_SHARED   OFF    CACHE    BOOL    "SDL3"    FORCE)


# Subdirectories for external libraries
add_subdirectory(External/glm)
add_subdirectory(External/fmt)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/assimp)
add_subdirectory(External/SDL)

# STB_Image
list(APPEND stb_image_DIR "${CMAKE_CURRENT_SOURCE_DIR}/External/stb_image/")
list(APPEND stb_image_SRC "${CMAKE_CURRENT_SOURCE_DIR}/External/stb_image/stb_image.h")

# Vulkan
find_package(Vulkan REQUIRED)

# Imgui setup
set(Imgui_DIR "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui")
list(APPEND Imgui_SRC
	"${Imgui_DIR}/imconfig.h"
	"${Imgui_DIR}/imgui.cpp"
	"${Imgui_DIR}/imgui.h"
	"${Imgui_DIR}/imgui_demo.cpp"
	"${Imgui_DIR}/imgui_draw.cpp"
	"${Imgui_DIR}/imgui_internal.h"
	"${Imgui_DIR}/imgui_tables.cpp"
	"${Imgui_DIR}/imgui_widgets.cpp"
	"${Imgui_DIR}/imstb_rectpack.h"
	"${Imgui_DIR}/imstb_textedit.h"
	"${Imgui_DIR}/imstb_truetype.h"
	"${Imgui_DIR}/backends/imgui_impl_sdl3.h"
	"${Imgui_DIR}/backends/imgui_impl_sdl3.cpp"
	"${Imgui_DIR}/backends/imgui_impl_vulkan.h"
	"${Imgui_DIR}/backends/imgui_impl_vulkan.cpp"
)

add_library(Imgui STATIC)
target_sources(Imgui PUBLIC ${Imgui_SRC} ${SDL3_INCLUDE_DIRS})
target_include_directories(Imgui PUBLIC ${Imgui_DIR})
target_link_libraries(Imgui PUBLIC SDL3::SDL3-static)



# Used sources, Engine
list(APPEND EncosyEngine_MODULES_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/Application.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/Interface.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/WindowManager.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/MatrixCalculations.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/SetupCoreECS.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/Profiler.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/ProfilerInterface.ixx"
	
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS/TransformComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS/InputSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS/InputSystemData.ixx"

	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/EncosyCore.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/EncosyWorld.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/EntityManager.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/ComponentManager.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/SystemManager.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/ThreadedTaskRunner.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/Entity.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/SystemBase.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/System.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/SystemThreaded.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/EntityTypeStorage.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/ComponentTypeStorage.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore/SharedBetweenManagers.ixx"

	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/RenderCore.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/RenderCoreResources.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/RenderPipelineManager.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/VulkanTypes.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/VulkanInitializers.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/VulkanErrorLogger.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/VulkanUtilities.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/VulkanDescriptors.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/PipelineBuilder.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/AllocationHandler.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/MeshLoader.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/TextureLoader.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ShaderLoader.ixx"
	
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/MaterialComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/UnlitRenderSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/LitRenderSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/CameraComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/CameraControllerSystem.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/CameraControllerSystemData.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/StaticComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/ModelMatrixComponent.ixx"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/ECS/ModelMatrixBuilderSystem.ixx"
)	

# Used shaders, Engine
list(APPEND EncosyEngine_SHADERS_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/Shaders/Gradient.comp"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/Shaders/Lit.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/Shaders/Lit.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/Shaders/Unlit.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/RenderCore/Shaders/Unlit.frag"
)

# Used sources, Engine Testing
list(APPEND EncosyTesting_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/Testing/main.cppm"
)


# Add EncosyEngine as a static library
add_library(EncosyEngine STATIC)
target_sources(EncosyEngine 
PUBLIC FILE_SET CXX_MODULES 
FILES 
${EncosyEngine_MODULES_SRC} 
BASE_DIRS 
${CMAKE_CURRENT_SOURCE_DIR}/EncosyCore 
${CMAKE_CURRENT_SOURCE_DIR}/Core 
${CMAKE_CURRENT_SOURCE_DIR}/Systems 
${CMAKE_CURRENT_SOURCE_DIR}/RenderCore
)

target_sources(EncosyEngine PUBLIC FILE_SET HEADERS FILES ${stb_image_SRC} BASE_DIRS ${stb_image_DIR})
target_include_directories(EncosyEngine PUBLIC  ${stb_image_DIR})
target_compile_definitions(EncosyEngine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_link_libraries(EncosyEngine PUBLIC Imgui glm Vulkan::Vulkan SDL3::SDL3-static fmt::fmt vk-bootstrap::vk-bootstrap assimp::assimp)

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(EncosyEngine TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EncosyEngine_MODULES_SRC} ${stb_image_SRC})


# Add executable for testing purposes
add_executable(EncosyTesting)
target_sources(EncosyTesting 
PUBLIC FILE_SET CXX_MODULES FILES ${EncosyTesting_SRC} BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Testing)
target_link_libraries(EncosyTesting PUBLIC EncosyEngine)

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(EncosyTesting TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EncosyTesting_SRC})

set(CONFIGURATION_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
set_target_properties(EncosyTesting PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${CONFIGURATION_BINARY_DIR}")

# Resource copying
set(TEXTURES_DIR "${PROJECT_SOURCE_DIR}/Resources/Textures")
set(MODELS_DIR "${PROJECT_SOURCE_DIR}/Resources/Models")
set(SHADERS_DIR "${PROJECT_SOURCE_DIR}/Resources/Shaders")
set(BUILD_RESOURCES_DIR "${CONFIGURATION_BINARY_DIR}/Engine/Resources")

add_custom_target(Engine_Build_Setup
	COMMAND ${CMAKE_COMMAND} -E make_directory "${TEXTURES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODELS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Textures"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Models"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_RESOURCES_DIR}/Shaders"
)
add_dependencies(EncosyEngine Engine_Build_Setup)

# Shader building
include("${PROJECT_SOURCE_DIR}/EngineShaderBuild.cmake")
APPEND_GLSL_TO_TARGET(EncosyEngine ${PROJECT_SOURCE_DIR} ${BUILD_RESOURCES_DIR} "${EncosyEngine_SHADERS_SRC}")


add_custom_command(
TARGET EncosyEngine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CONFIGURATION_BINARY_DIR}/Engine/Resources"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Resources/" "${CONFIGURATION_BINARY_DIR}/Engine/Resources"
)
