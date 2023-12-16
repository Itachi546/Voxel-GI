#version 450

layout(location = 0) out vec4 fragColor;

layout(r32ui) uniform readonly uimage3D uVolumeTexture;

in flat vec3 vVoxelSpacePos;

vec4 convRGBA8ToVec4(uint val) {
    return vec4( float((val & 0x000000FF)), 
                 float((val & 0x0000FF00) >> 8U), 
                 float((val & 0x00FF0000) >> 16U), 
                 float((val & 0xFF000000) >> 24U));
}

void main() {
   vec4 color = convRGBA8ToVec4(imageLoad(uVolumeTexture, ivec3(vVoxelSpacePos)).r) / 255.0f;
   fragColor = vec4(color.rgb, 1.0f);
}