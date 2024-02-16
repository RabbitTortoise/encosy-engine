#version 460

// Shader input
layout (location = 0) in vec4 inFragColor;
layout (location = 1) in vec2 inFragUV;

// Texture to access
layout(set = 2, binding = 0) uniform sampler2D AlbedoTexture;

// Output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
	// Return color
    outFragColor = texture(AlbedoTexture, inFragUV);
}