#version 460
#extension GL_EXT_buffer_reference : require

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

// Vertex buffer layout
layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

// Model Matrix buffer layout
layout(buffer_reference, std430) readonly buffer ModelMatrixBuffer{ 
	mat4 modelMatrixes[];
};

// Inputs
// Camera data
layout(set = 0, binding = 0) uniform CameraBuffer{   
    mat4 view;
    mat4 proj;
	mat4 viewproj; 
} CameraData;


// Lighting Data
layout(set = 1, binding = 0) uniform LightingBuffer{   
    vec4 ambientLightColor;
    vec4 directionalLightColor;
    vec4 directionalLightDir;
} LightingData;

// Push constants block
layout( push_constant ) uniform constants
{
	// Address to model vertexbuffer
	VertexBuffer vertexBuffer;
	// Address to model matrix buffer
	ModelMatrixBuffer modelMatrixBuffer;
} PushConstants; 

// Outputs
layout (location = 0) out vec4 outFragPos;
layout (location = 1) out vec2 outFragUV;
layout (location = 2) out vec3 outTangentFragPos;
layout (location = 3) out vec3 outTangentLightDir;

void main() 
{	
	// Load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	mat4 model = PushConstants.modelMatrixBuffer.modelMatrixes[gl_InstanceIndex];

	vec4 fragPos = model * vec4(v.position, 1.0);
	vec3 T = normalize(vec3(model * vec4(v.tangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(v.normal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
	mat3 transposedTBN = transpose(mat3(T, B, N));

	vec2 uv = vec2(v.uv_x, v.uv_y);
	outFragPos = fragPos;
	outFragUV = uv;
	outTangentFragPos = transposedTBN * fragPos.xyz;
    vec3 lightDir = normalize(-LightingData.directionalLightDir.xyz);
	outTangentLightDir = transposedTBN * lightDir;

	// Vertex position
	gl_Position = CameraData.viewproj * model * vec4(v.position, 1.0f);
}
