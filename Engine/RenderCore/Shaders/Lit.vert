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

//Vertex buffer layout
layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};


// Inputs
// Camera data
layout(set = 0, binding = 0) uniform CameraBuffer{   
    mat4 view;
    mat4 proj;
	mat4 viewproj; 
} CameraData;

// Object Model Matrix
layout(set = 1, binding = 0) uniform ObjectBuffer{   
    mat4 model;
} ObjectData;


// Push constants block
layout( push_constant ) uniform constants
{	
	// Address to model vertexbuffer
	VertexBuffer vertexBuffer;
} PushConstants;

// Outputs
layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec2 outFragUV;


void main() 
{	
	// Load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	// Calculate vertex position
	gl_Position = CameraData.viewproj * ObjectData.model * vec4(v.position, 1.0f);

	// Sample vertex color
	vec2 uv = vec2(v.uv_x, v.uv_y);
	outFragUV = uv;
	outFragColor = v.color;
}
