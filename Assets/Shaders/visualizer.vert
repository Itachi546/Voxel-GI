#version 450

layout(location = 0) in vec3 position;

uniform mat4 uVP;
uniform int uVoxelHalfSize;

struct InstanceData {
   float x, y, z;
};

layout(std430, binding = 0) buffer DrawData {
   InstanceData instanceData[];
};

out vec3 vWorldPos;

void main() 
{
   InstanceData data = instanceData[gl_InstanceID];
   vec3 worldPos = vec3(data.x, data.y, data.z);

   vWorldPos = worldPos;
   gl_Position = uVP * vec4(position + worldPos - uVoxelHalfSize, 1.0f);
}