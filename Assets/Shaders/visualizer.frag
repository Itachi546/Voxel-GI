#version 450

layout(location = 0) out vec4 fragColor;

layout(rgba8) uniform readonly image3D uVolumeTexture;

in flat vec3 vVoxelSpacePos;

void main() {
   vec3 color = imageLoad(uVolumeTexture, ivec3(vVoxelSpacePos)).rgb;
   fragColor = vec4(color, 1.0f);
}