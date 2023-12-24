#version 450

in vec3 gNormal;
in vec3 gWorldPos;
in vec2 gUV;
in flat int gMaterialIndex;
in vec3 gLightDirection;

struct Material {
	vec4 albedo;
	vec4 emissive;

	float metallic;
	float roughness;
	float ao;
	float transparency;

	uint padding;
	uint albedoMap;
	uint normalMap;
	uint emissiveMap;

	uint metallicMap;
	uint roughnessMap;
	uint ambientOcclusionMap;
	uint opacityMap;
};

uniform vec2 uVoxelDims;

layout(rgba8, binding = 0) uniform image3D uVoxelTexture;
layout(binding = 2) readonly buffer MaterialData {
   Material materials[];
};


const float E = 0.001;
bool IsInsideCube(vec3 position) {
    const float edge = 1.0f + E;
    return abs(position.x) < edge && abs(position.y) < edge && abs(position.z) < edge;
}

void main() {
   vec3 n = normalize(gNormal);
   Material material = materials[gMaterialIndex];

   float lightDist = length(gLightDirection);
   vec3 lightDir = gLightDirection / lightDist;

   float attenuation = 1.0f / (lightDist * lightDist);
   float diffuse = max(dot(n, lightDir), 0.1f) * attenuation;
   vec3 col = diffuse * material.albedo.rgb;
   col += material.emissive.rgb;
   
   if(IsInsideCube(gWorldPos)) {
     ivec3 voxelCoord = ivec3(gWorldPos * uVoxelDims.x);
     imageStore(uVoxelTexture, voxelCoord, vec4(col, 1.0f));
   }
}
