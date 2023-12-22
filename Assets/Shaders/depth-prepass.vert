#version 450

#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(binding = 1) readonly buffer TransformData
{
   mat4 aTransformData[];
};

uniform mat4 uVP;

void main() {
    mat4 modelMatrix = aTransformData[gl_DrawIDARB];
    gl_Position = uVP * modelMatrix * vec4(position, 1.0f);
}