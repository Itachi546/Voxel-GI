#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vNormal[];
in vec3 vWorldPos[];
in vec2 vUV[];
in flat int vMaterialIndex[];

out vec3 gWorldPos;
out vec3 gNormal;
out vec2 gUV;
out vec3 gLightDirection;
out flat int gMaterialIndex;

uniform mat4 uVoxelSpaceTransform;
uniform vec3 uLightPosition;
// X - voxelDimension, Y- voxelSize
uniform vec2 uVoxelDims;

vec3 ToVoxelSpace(vec3 p, float halfSize) {
   return p / halfSize;
}

void main() {
   vec3 e1 = normalize(vWorldPos[1] - vWorldPos[0]);
   vec3 e2 = normalize(vWorldPos[2] - vWorldPos[0]);

   vec3 faceNormal = cross(e1, e2);

   uint dominantAxis = abs(faceNormal[1]) > abs(faceNormal[0]) ? 1 : 0;
   dominantAxis = abs(faceNormal[2]) > abs(faceNormal[dominantAxis]) ? 2 : dominantAxis;

   float halfSize = uVoxelDims.x * uVoxelDims.y * 0.5;
   for(int i = 0; i < 3; ++i) {
      vec3 voxelSpacePosition = ToVoxelSpace(vWorldPos[i], halfSize);

      vec3 projectedPosition = voxelSpacePosition;
      if(dominantAxis == 0)
        projectedPosition = projectedPosition.zyx;
      else if(dominantAxis == 1)
        projectedPosition = projectedPosition.xzy;
      projectedPosition.z = 0.0f;

      gWorldPos = voxelSpacePosition * 0.5 + 0.5; 
      gLightDirection = uLightPosition - vWorldPos[i];
      gUV = vUV[i];
      gNormal = faceNormal;
      gMaterialIndex = vMaterialIndex[i];

      gl_Position = vec4(projectedPosition, 1.0f);
      EmitVertex();
   }
   EndPrimitive();

}