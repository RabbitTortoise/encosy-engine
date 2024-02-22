#version 460

// Shader input
layout (location = 0) in vec4 inFragPos;
layout (location = 1) in vec2 inFragUV;
layout (location = 2) in vec3 inTangentFragPos;
layout (location = 3) in vec3 inTangentLightDir;


// Lighting Data
layout(set = 1, binding = 0) uniform LightingBuffer{   
    vec4 ambientLightColor;
    vec4 directionalLightColor;
    vec4 directionalLightDir;
} LightingData;

// Textures
layout(set = 2, binding = 0) uniform sampler2D DiffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D NormalTexture;

// Texture Settings
layout(set = 2, binding = 2) uniform TextureOptionsBuffer{   
    float TextureRepeat;
} TextureOptions;

// Output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
    // Sample textures
    vec2 uv = fract(inFragUV * TextureOptions.TextureRepeat);
    vec3 diffuseColor = texture(DiffuseTexture, uv).rgb;
    vec3 normal = texture(NormalTexture, uv).rgb;

    // Lighting calculations are done in Tangent space!

    // Calculate ambient light
    vec3 ambientLight = LightingData.ambientLightColor.rgb * LightingData.ambientLightColor.a;
        
    // Calculate normals
    normal = normal * 2.0 - 1.0;   
    normal = normalize(normal); 

    // Calculate directional light
    float lightImpact = max(dot(normal, inTangentLightDir), 0.0) * LightingData.directionalLightColor.a;
    vec3 directionalLight = lightImpact * LightingData.directionalLightColor.rgb;

    vec3 ambientColor = ambientLight * diffuseColor;
    vec3 directionalColor = directionalLight * diffuseColor;
    vec3 color = ambientColor + directionalColor;

    outFragColor = vec4(color, 1.0);
}