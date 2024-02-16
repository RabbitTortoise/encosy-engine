# Shader Building
cmake_minimum_required(VERSION 3.28)

macro(APPEND_GLSL_TO_TARGET targetName workingDir shaderlist)
	find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
	set(SHADER_DIR "${workingDir}/RenderCore/Shaders")
	set(COMPILED_DIR "${workingDir}/Resources/Shaders")

	add_custom_target(
	    RenderCoreShaders
		SOURCES ${shaderlist} ShaderBuild.cmake
	    )
	add_dependencies(${targetName} RenderCoreShaders)

	foreach(GLSL ${shaderlist})
	  get_filename_component(FILE_NAME ${GLSL} NAME)
	  set(SPIRV "${COMPILED_DIR}/${FILE_NAME}.spv")
	  
	  add_custom_command(TARGET RenderCoreShaders
	    COMMAND "${GLSL_VALIDATOR}" -V ${GLSL} -o ${SPIRV}
	    DEPENDS ${GLSL})
	  list(APPEND SPIRV_BINARY_FILES ${SPIRV})

	endforeach(GLSL)

endmacro()
