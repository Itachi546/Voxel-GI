#version 450

in vec3 gNormal;
in vec3 gWorldPos;
in vec2 gUV;

uniform vec2 uVoxelDims;

layout(rgba8, binding = 0) uniform image3D uVoxelTexture;

vec3 lightDir = normalize(vec3(0.1, 0.5, -0.1));

const float E = 0.001;
bool isInsideCube(vec3 position) {
    return abs(gWorldPos.x) < 1.0f + E && abs(gWorldPos.y) < 1.0f + E && abs(gWorldPos.z) < 1.0f + E;
}

void main() {
   float diffuse = max(dot(gNormal, lightDir), 0.0f);
   vec3 col = diffuse * vec3(1.28, 1.20, 0.99);
   col += (gNormal.y * 0.5 + 0.5) * vec3(0.16, 0.20, 0.28);
   col *= vec3(0.8, 0.3, 0.2);
   col /= (1.0f + col);
   //col = pow(col, vec3(0.4545));
   
   if(isInsideCube(gWorldPos)) {
     ivec3 voxelCoord = ivec3(gWorldPos * uVoxelDims.x);
     imageStore(uVoxelTexture, voxelCoord, vec4(col, 1.0f));
   }
}
