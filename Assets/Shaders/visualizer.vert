#version 450

layout(location = 0) in vec3 position;

uniform mat4 uVP;
uniform int uVoxelDims;
uniform float uVoxelSpan;
uniform float uUnitVoxelSize;
struct InstanceData {
   float x, y, z;
};

layout(std430, binding = 0) buffer DrawData {
   InstanceData instanceData[];
};

out flat vec3 vVoxelSpacePos;

vec3 toWorldSpace(vec3 p) {
    vec3 pClip = (p / uVoxelDims) * 2.0f - 1.0f;
    return pClip * uVoxelSpan;
}

void main() 
{
   InstanceData data = instanceData[gl_InstanceID];

   vec3 voxelSpacePos = vec3(data.x, data.y, data.z);

   float voxelScale =  0.5;

   vVoxelSpacePos = voxelSpacePos;

   vec3 worldPos = toWorldSpace(voxelSpacePos);

   gl_Position = uVP * vec4(position * 0.5 * uUnitVoxelSize + worldPos, 1.0f);
}