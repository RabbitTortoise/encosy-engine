# Shader Building
cmake_minimum_required(VERSION 3.28)

macro(APPEND_GLSL_TO_TARGET targetName workingDir binaryDir shaderlist)
	find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
	set(SHADER_DIR "${workingDir}/RenderCore/Shaders")
	set(COMPILED_DIR "${workingDir}/Resources/Shaders")
	set(DEBUG_DIR "${binaryDir}/Shaders")

	add_custom_target(
	    EngineShaders
		SOURCES ${shaderlist} EngineShaderBuild.cmake
	    )
	add_dependencies(${targetName} EngineShaders)

	foreach(GLSL ${shaderlist})
	  get_filename_component(FILE_NAME ${GLSL} NAME)
	  SET(SPIRV_FILE "${FILE_NAME}.spv")
	  set(SPIRV "${COMPILED_DIR}/${FILE_NAME}.spv")
	
	  add_custom_command(
		TARGET EngineShaders
	    COMMAND "${GLSL_VALIDATOR}" -V ${GLSL} -o ${SPIRV}
	    DEPENDS ${GLSL})
	  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
	endforeach(GLSL)

	add_custom_command(
		TARGET EngineShaders POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMPILED_DIR}  "${binaryDir}/Engine/Resources/Shaders"
	)

endmacro()
