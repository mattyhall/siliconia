#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 colour;

layout (location = 0) out vec3 out_colour;

layout (push_constant) uniform constants
{
    mat4 model_matrix;
} PushConstants;

void main()
{
    gl_Position = PushConstants.model_matrix * vec4(pos, 1.0f);
    out_colour = colour;
}