#version 450

in vec3 gNormal;
in vec3 gWorldPos;
in vec2 gUV;

uniform vec2 uVoxelDims;

layout(r32ui, binding = 0) uniform volatile uimage3D uVoxelTexture;

vec3 lightDir = normalize(vec3(0.1, 0.5, -0.1));

const float E = 0.001;
bool isInsideCube(vec3 position) {
    return abs(gWorldPos.x) < 1.0f + E && abs(gWorldPos.y) < 1.0f + E && abs(gWorldPos.z) < 1.0f + E;
}

uint convVec4ToRGBA8(vec4 val) {
    return (uint(val.w) & 0x000000FF)   << 24U
            |(uint(val.z) & 0x000000FF) << 16U
            |(uint(val.y) & 0x000000FF) << 8U 
            |(uint(val.x) & 0x000000FF);
}

vec4 convRGBA8ToVec4(uint val) {
    return vec4( float((val & 0x000000FF)), 
                 float((val & 0x0000FF00) >> 8U), 
                 float((val & 0x00FF0000) >> 16U), 
                 float((val & 0xFF000000) >> 24U));
}

#define MAX_NUM_AVG_ITERATIONS 100
uint imageAtomicRGBA8Avg(ivec3 coords, vec4 newVal) {
    newVal.xyz *= 255.0; // Optimise following calculations
    uint newValU = convVec4ToRGBA8(newVal);
    uint lastValU = 0; 
    uint currValU;
    vec4 currVal;
    uint numIterations = 0;
    // Loop as long as destination value gets changed by other threads
    while((currValU = imageAtomicCompSwap(uVoxelTexture, coords, lastValU, newValU))
          != lastValU
          && numIterations < MAX_NUM_AVG_ITERATIONS) {
        lastValU = currValU;

        currVal = convRGBA8ToVec4(currValU);
        currVal.xyz *= currVal.a; // Denormalize

        currVal += newVal; // Add new value
        currVal.xyz /= currVal.a; // Renormalize

        newValU = convVec4ToRGBA8(currVal);

        ++numIterations;
    }

    // currVal now contains the calculated color: now convert it to a proper alpha-premultiplied version
    newVal = convRGBA8ToVec4(newValU);
    newVal.a = 255.0;
    newValU = convVec4ToRGBA8(newVal);
    imageStore(uVoxelTexture, coords, uvec4(newValU));
    return newValU;
}



void main() {
   float diffuse = max(dot(gNormal, lightDir), 0.0f);
   vec3 col = diffuse * vec3(1.28, 1.20, 0.99);
   col += (gNormal.y * 0.5 + 0.5) * vec3(0.16, 0.20, 0.28);

   col /= (1.0f + col);
   col = pow(col, vec3(0.4545));
   
   if(isInsideCube(gWorldPos)) {
     ivec3 voxelCoord = ivec3(gWorldPos * uVoxelDims.x);
     imageAtomicRGBA8Avg(voxelCoord, vec4(col, 1.0f));
   }
}
