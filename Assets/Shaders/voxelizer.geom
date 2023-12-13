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

void main() {
   vec3 e1 = normalize(vWorldPos[0] - vWorldPos[1]);
   vec3 e2 = normalize(vWorldPos[2] - vWorldPos[1]);

   vec3 faceNormal = cross(e1, e2);

   uint dominantAxis = faceNormal[1] > faceNormal[0] ? 1 : 0;
   dominantAxis = faceNormal[2] > faceNormal[dominantAxis] ? 2 : dominantAxis;

   for(int i = 0; i < 3; ++i) {
      vec3 position = vWorldPos[i];

      if(dominantAxis == 0)
        position = position.zyx;
      else if(dominantAxis == 1)
        position = position.xzy;
      position.z = 0.0f;

      gWorldPos = vec3(uVoxelSpaceTransform * uView * vec4(vWorldPos[i], 1.0f)) * 0.5 + 0.5;
      gUV = vUV[i];
      gNormal = faceNormal;

      gl_Position = uVoxelSpaceTransform * uView * vec4(position, 1.0f);
      EmitVertex();
   }
   EndPrimitive();

}