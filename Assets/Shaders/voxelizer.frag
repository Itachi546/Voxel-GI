#version 450

layout(location = 0) out vec4 fragColor;

in vec3 gNormal;
in vec3 gWorldPos;
in vec2 gUV;

uniform int uVoxelDims;

layout(rgba8) uniform image3D uVoxelTexture;

vec3 lightDir = normalize(vec3(0.1, 0.5, -0.1));

void main() {
   float diffuse = max(dot(gNormal, lightDir), 0.0f);
   vec3 col = diffuse * vec3(1.28, 1.20, 0.99);
   col += (gNormal.y * 0.5 + 0.5) * vec3(0.16, 0.20, 0.28);

   col /= (1.0f + col);
   col = pow(col, vec3(0.4545));
   fragColor = vec4(col, 1.0f);

   ivec3 voxelCoord = ivec3(gWorldPos * uVoxelDims);
   imageStore(uVoxelTexture, voxelCoord, vec4(col, 1.0f));
}
