#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

uniform mat4 uModel;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    mat3 normalTransform = mat3(transpose(inverse(uModel)));
    vec4 worldPos = uModel * vec4(position, 1.0f);

    vWorldPos = worldPos.xyz;
    vNormal = normalize(normalTransform * normal);
    vUV = uv;

    gl_Position = worldPos;
}