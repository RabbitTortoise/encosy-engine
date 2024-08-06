
#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable

// Data layouts
// Vertex data layout
struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
	vec3 tangent;
	float padding;
}; 

//Instance data
struct RaytracingInstanceData
{
	uint modelIndex;
	uint textureSet;
	uint padding1;
	uint padding2;
	vec3 color;
    float textureRepeat;
};

// Hit payload layout
struct hitPayload
{
  vec4 hitValue;
};

// Vertex buffer layout
layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
  Vertex vertices[];
};

// Indices buffer layout
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer IndexBuffer {
  int indices[]; 
};

// Sets and bindings
// Tlas and camera data
layout(set = 1, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = 1) uniform CameraProperties
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 cameraPosition;
} CameraData;


// Lighting Data
layout(set = 2, binding = 0) uniform LightingBuffer{   
    vec4 ambientLightColor;
    vec4 directionalLightColor;
   vec4 directionalLightDir;
} LightingData;

// Instance Data
layout(std430, set = 3, binding = 0) readonly buffer InstanceDataBuffer{
	RaytracingInstanceData instanceData[];
} InstanceData;

// Vertex Data
layout(std430, set = 4, binding = 0) readonly buffer ModelVertexBuffer{
	Vertex vertices[];
} ModelVertexData[];

// Index Data
layout(std430, set = 4, binding = 1) readonly buffer ModelIndexBuffer{
	 int indices[];
} ModelIndexData[];

// Texture Data
layout(set = 5, binding = 0) uniform sampler TextureSampler;
layout(set = 5, binding = 1) uniform texture2D AlbedoTextures[];
layout(set = 5, binding = 2) uniform texture2D AOTextures[];
layout(set = 5, binding = 3) uniform texture2D DepthTextures[];
layout(set = 5, binding = 4) uniform texture2D MetallicTextures[];
layout(set = 5, binding = 5) uniform texture2D NormalTextures[];
layout(set = 5, binding = 6) uniform texture2D RoughnessTextures[];


// Data passed between shaders
layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
hitAttributeEXT vec2 attribs;


// Constants
const float PI = 3.14159265359;
float gamma = 2.2;
float parallaxScale = 10.0;

// ----------------------------------------------------------------------------
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, texture2D depthMap)
{ 
    // number of depth layers
    const float minLayers = 64;
    const float maxLayers = 128;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * parallaxScale; 
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(sampler2D(depthMap, TextureSampler), currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords.x -= deltaTexCoords.x;
        currentTexCoords.y += deltaTexCoords.y;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(sampler2D(depthMap, TextureSampler), currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = vec2(0);
    prevTexCoords.x = currentTexCoords.x + deltaTexCoords.x;
    prevTexCoords.y = currentTexCoords.y - deltaTexCoords.y;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(sampler2D(depthMap, TextureSampler), prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------


void main()
{
    //Instance Data
    uint modelIndex = InstanceData.instanceData[gl_InstanceCustomIndexEXT].modelIndex;
    uint textureSet = InstanceData.instanceData[gl_InstanceCustomIndexEXT].textureSet;
    vec3 instanceColor = InstanceData.instanceData[gl_InstanceCustomIndexEXT].color;
    float textureRepeat = InstanceData.instanceData[gl_InstanceCustomIndexEXT].textureRepeat;

    //Indices of the triangle
    int index1 = ModelIndexData[modelIndex].indices[3 * gl_PrimitiveID];
    int index2 = ModelIndexData[modelIndex].indices[3 * gl_PrimitiveID + 1];
    int index3 = ModelIndexData[modelIndex].indices[3 * gl_PrimitiveID + 2];

     // Vertex of the triangle
    Vertex v0 = ModelVertexData[modelIndex].vertices[index1];
    Vertex v1 = ModelVertexData[modelIndex].vertices[index2];
    Vertex v2 = ModelVertexData[modelIndex].vertices[index3];

    // Barycentric Coordinates
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    // Computing the coordinates of the hit position
    const vec3 hitPos = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
    const vec3 hitWorldPos = vec3(gl_ObjectToWorldEXT * vec4(hitPos, 1.0));  // Transforming the position to world space

    // Computing the uv coordinates at hit position
    const vec2 uv0 = vec2(v0.uv_x, v0.uv_y);
    const vec2 uv1 = vec2(v1.uv_x, v1.uv_y);
    const vec2 uv2 = vec2(v2.uv_x, v2.uv_y);
    vec2 hitUv = uv0 * barycentricCoords.x + uv1 * barycentricCoords.y + uv2 * barycentricCoords.z;
    hitUv = fract(hitUv * textureRepeat);

    // Computing the normal at hit position
    const vec3 vertexNormal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
    // Computing the tangent at hit position
    vec3 vertexTangent =  normalize(v0.tangent * barycentricCoords.x + v1.tangent * barycentricCoords.y + v2.tangent * barycentricCoords.z);
    vertexTangent = normalize(vertexTangent - dot(vertexTangent, vertexNormal) * vertexNormal);
    const vec3 vertexBiTangent = cross(vertexNormal, vertexTangent);

    const mat3 TBN = mat3(
    normalize(vec3(gl_ObjectToWorldEXT * vec4(vertexTangent,  0.0))),
    normalize(vec3(gl_ObjectToWorldEXT * vec4(vertexBiTangent,  0.0))),
    normalize(vec3(gl_ObjectToWorldEXT * vec4(vertexNormal,  0.0)))
    );

    //Parallax Mapping
    mat3 TBNInverse = transpose(TBN);
    vec4 TangentFragPos = vec4(TBNInverse * hitWorldPos.xyz, 1.0);
    vec4 TangentCamPos = vec4(TBNInverse * CameraData.cameraPosition.xyz, 1.0);
    vec3 TangentViewDir = normalize(TangentCamPos - TangentFragPos).xyz;
    hitUv = ParallaxMapping(hitUv, TangentViewDir, MetallicTextures[textureSet]);   


    // Light variables
    const vec3 ambientLightColor = LightingData.ambientLightColor.rgb;
    const float ambientLightIntensity = LightingData.ambientLightColor.a;
    const vec3 directionalLightColor = LightingData.directionalLightColor.rgb;
    const float directionalLightIntensity = LightingData.directionalLightColor.a;
    const vec3 worldSpaceLightDir = normalize(LightingData.directionalLightDir.rgb);
    vec3 LightDir = normalize(worldSpaceLightDir);
    //vec3 LightDir = normalize(worldSpaceLightPos - hitWorldPos); FOR POINT LIGHT
    float LightDist = length(worldSpaceLightDir - hitWorldPos);

    // Texture sampling
    const vec3 albedoMap     = pow(texture(sampler2D(AlbedoTextures[textureSet], TextureSampler), hitUv).rgb, vec3(gamma));
    //const vec3 albedoMap     = texture(sampler2D(AlbedoTextures[textureSet], TextureSampler), hitUv).rgb;
    const float aoMap        = texture(sampler2D(AOTextures[textureSet], TextureSampler), hitUv).r;
    const float metallicMap  = texture(sampler2D(MetallicTextures[textureSet], TextureSampler), hitUv).r;
    const vec3 normalMap     = texture(sampler2D(NormalTextures[textureSet], TextureSampler), hitUv).rgb;
    const float roughnessMap = texture(sampler2D(RoughnessTextures[textureSet], TextureSampler), hitUv).r;

    // Calculate world space normal
    const vec3 Normal = normalize(TBN * (normalMap * 2.0 - 1.0));   
    const vec3 ViewDir = normalize(CameraData.cameraPosition.xyz - hitWorldPos).xyz;

    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedoMap, metallicMap);

    // Total light output
    vec3 Lo = vec3(0.0);

    // Directional calculations.
    // Shadow tracing
    float shadowImpactOnLightIntensity = 1;
    float shadowImpactOnSpecular = 1;
    if(dot(Normal, LightDir) > 0)
    {
        float tMin   = 0.001;
        float tMax   = 10000.0;
        //float tMax   = LightDist //FOR POINT LIGHT;
        vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
        vec3  rayDir = LightDir;
        uint  flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                1,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
        );
        if(isShadowed)
        {
            shadowImpactOnLightIntensity = 0.01;
            shadowImpactOnSpecular = 0.0;
        }
    }

	vec3 H = normalize(ViewDir + LightDir);
	vec3 radiance = directionalLightColor.rgb * directionalLightIntensity * shadowImpactOnLightIntensity;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(Normal, H, roughnessMap);   
    float G   = GeometrySmith(Normal, ViewDir, LightDir, roughnessMap);      
    vec3 F    = fresnelSchlickRoughness(max(dot(H, ViewDir), 0.0), F0, roughnessMap);
        
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallicMap;	  

    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(Normal, ViewDir), 0.0) * max(dot(Normal, LightDir), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator * shadowImpactOnSpecular;


     float NdotL = max(dot(Normal, LightDir), 0.0);
     Lo += (kD * albedoMap * instanceColor / PI + specular) * radiance * NdotL + ambientLightColor * ambientLightIntensity;

    vec3 color = Lo;
    // HDR tonemapping
    //color = color / (color + vec3(1.0));
    // Gamma correct
    color = pow(color, vec3(1.0/gamma)); 
    prd.hitValue = vec4(color, 1.0);
}