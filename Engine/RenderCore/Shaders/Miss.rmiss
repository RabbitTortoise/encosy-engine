#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

// Data layouts
// Hit payload layout
struct hitPayload
{
  vec4 hitValue;
};

// Data passed between shaders
layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
    prd.hitValue = vec4(0,0,0,0);
}
