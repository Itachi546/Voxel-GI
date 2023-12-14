#version 450

layout(location = 0) out vec4 fragColor;

layout(rgba8) uniform image3D uVolumeTexture;

in vec3 vWorldPos;

void main() {
   vec3 color = imageLoad(uVolumeTexture, ivec3(vWorldPos)).rgb;
   fragColor = vec4(color, 1.0f);
}