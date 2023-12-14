#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vNormal[];
in vec3 vWorldPos[];
in vec2 vUV[];

out vec3 gWorldPos;
out vec3 gNormal;
out vec2 gUV;

uniform mat4 uVP;
uniform mat4 uVoxelSpaceTransform;
uniform mat4 uView;
uniform int uVoxelDims;

void main() {
   vec3 e1 = normalize(vWorldPos[0] - vWorldPos[1]);
   vec3 e2 = normalize(vWorldPos[2] - vWorldPos[1]);

   vec3 faceNormal = cross(e1, e2);

   uint dominantAxis = abs(faceNormal[1]) > abs(faceNormal[0]) ? 1 : 0;
   dominantAxis = abs(faceNormal[2]) > abs(faceNormal[dominantAxis]) ? 2 : dominantAxis;

   float halfDims = 0.5 * uVoxelDims;
   for(int i = 0; i < 3; ++i) {
      vec3 voxelSpacePosition = vec3(uVoxelSpaceTransform * vec4(vWorldPos[i], 1.0f));

      vec3 projectedPosition = voxelSpacePosition;
      if(dominantAxis == 0)
        projectedPosition = projectedPosition.zyx;
      else if(dominantAxis == 1)
        projectedPosition = projectedPosition.xzy;
      projectedPosition.z = 0.0f;

      gWorldPos = voxelSpacePosition * 0.5 + 0.5;
      gUV = vUV[i];
      gNormal = faceNormal;

      gl_Position = vec4(projectedPosition, 1.0f);
      EmitVertex();
   }
   EndPrimitive();

}