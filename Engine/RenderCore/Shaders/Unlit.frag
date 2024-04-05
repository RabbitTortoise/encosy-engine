#version 460

// Shader input
layout (location = 0) in vec4 inFragColor;
layout (location = 1) in vec2 inFragUV;

// Texture to access
layout(set = 2, binding = 0) uniform sampler2D DiffuseTexture;

// Texture Settings
layout(set = 2, binding = 1) uniform TextureOptionsBuffer{   
    float TextureRepeat;
    vec3 Color;
} TextureOptions;

// Output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
    vec2 uv = fract(inFragUV * TextureOptions.TextureRepeat);
	// Return color
    outFragColor = texture(DiffuseTexture, uv);
}