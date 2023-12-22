#version 450

layout(location = 0) out vec4 fragColor;

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;
in flat int vMaterialIndex;

vec3 normal = normalize(vNormal);
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

layout(binding = 2) readonly buffer MaterialData {
   Material materials[];
};

uniform sampler3D uVolumeTexture;
uniform vec3 uVoxelDims;
uniform vec3 uCameraPosition;
uniform vec3 uLightPosition;

const float SCALING = uVoxelDims.y / uVoxelDims.y;
const float CONE_OFFSET = uVoxelDims.y * sqrt(3.0f) * SCALING;
const float STEP_SIZE = uVoxelDims.y * SCALING;
const float INV_VOXEL_DIMS = 1.0f / uVoxelDims.y;
const float	HALF_SIZE = uVoxelDims.x * uVoxelDims.y * 0.5f;

vec3 ToVoxelSpace(vec3 p) {
   return (p / HALF_SIZE);
}

const float E = 0.001;
bool IsInsideCube(vec3 uv) {
    const float edge = 1.0f + E;
    return abs(uv.x) < edge && abs(uv.y) < edge && abs(uv.z) < edge;
}

vec3 coneTrace(vec3 direction, float aperture) {
   vec3 origin = ToVoxelSpace(vWorldPos);
   origin += CONE_OFFSET * direction;

   float dist = STEP_SIZE;
   const float coneCoefficient = 2.0f * tan(aperture *	0.5f);
   vec4 Lv = vec4(0.0f);
   const float maxDistance = distance(origin, vec3(1.0f));

   while(dist < maxDistance && Lv.a < 1.0f) {
      float diameter = dist * coneCoefficient;
      float mip = log2(diameter * INV_VOXEL_DIMS);

	  vec3 position	= origin + dist * direction;
      if(!IsInsideCube(position) || mip > 5.0f) break;

      vec4 sam = textureLod(uVolumeTexture, position * 0.5 + 0.5, mip);
      if(sam.a > 0.0f) {
        float a = 1.0f - Lv.a;
		Lv.rgb += a	* sam.rgb;
		Lv.a +=	a *	sam.a;
      }
      dist += diameter * STEP_SIZE * 0.5f;
   }
   return max(Lv.rgb, 0.0);
}

#define PI 3.141592
vec3 calculateDiffuseIndirect() {
    vec3 N = normal;
    vec3 T = cross(N, vec3(0.0f, 1.0f, 0.0f));
    vec3 B = cross(T, N);
    vec3 Lo = vec3(0.0f);

    float aperture = PI / 3.0f;
    vec3 direction = normal;

    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N + 0.7071f * T;
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N + 0.7071f * (0.309f * T + 0.951f * B);
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N + 0.7071f * (-0.809f * T + 0.588f * B);
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N - 0.7071f * (-0.809f * T - 0.588f * B);
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N - 0.7071f * (0.309f * T - 0.951f * B);
    Lo += coneTrace(direction, aperture);
    return Lo / 6.0f;
}

vec3 calculateSpecularReflection(vec3 viewDir, float roughness) {
    vec3 R = reflect(viewDir, normal);
	float aperture = roughness * PI * 0.5f * 0.1f;
    vec3 radiance = coneTrace(R, aperture);
    return max(radiance, 0.0f);
}

void main() {
   Material material = materials[vMaterialIndex];

   vec3 lightDir = uLightPosition - vWorldPos;
   float lightDist = length(lightDir);
   lightDir /= lightDist;

   float attenuation = 1.0f / (lightDist * lightDist);
   float diffuse = max(dot(normal, lightDir), 0.0f) * attenuation;
   vec3 col = diffuse * material.albedo.rgb;
   col += material.emissive.rgb * 10.0f;
   col += calculateDiffuseIndirect().rgb * 0.3f;

   vec3 viewDir = normalize(vWorldPos - uCameraPosition);

   if(material.metallic > 0.001f) 
      col += calculateSpecularReflection(viewDir, material.roughness) * material.metallic;

   col /= (1.0f + col);
   col = pow(col, vec3(0.4545));
   fragColor = vec4(col, 1.0f);
}
