#version 450

#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(binding = 1) readonly buffer TransformData
{
   mat4 aTransformData[];
};

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out flat int vMaterialIndex;

void main() {
    mat4 modelMatrix = aTransformData[gl_DrawIDARB];
    mat3 normalTransform = mat3(transpose(inverse(modelMatrix)));
    vec4 worldPos = modelMatrix * vec4(position, 1.0f);

    vWorldPos = worldPos.xyz;
    vNormal = normalize(normalTransform * normal);
    vUV = uv;
    vMaterialIndex = gl_DrawIDARB;
    gl_Position = worldPos;
}