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

const float CONE_OFFSET = uVoxelDims.y * sqrt(2.0f);
const float MAX_DISTANCE = 128.0f;

vec3 ToVoxelSpace(vec3 p, float halfSize) {
   return (p / halfSize) * 0.5f + 0.5f;
}

const float E = 0.001;
bool IsInsideCube(vec3 uv) {
    const float edge = 1.0f + E;
    return abs(uv.x) < edge && abs(uv.y) < edge && abs(uv.z) < edge;
}

vec3 coneTrace(vec3 direction, float aperture) {
   float invVoxelDims = 1.0f / uVoxelDims.y;
   direction = normalize(direction);
   float stepSize = uVoxelDims.y;
   vec3 start = vWorldPos + CONE_OFFSET * direction;

   float dist = uVoxelDims.y;
   const float coneCoefficient = 2.0f * tan(aperture *	0.5f);

   vec4 Lv = vec4(0.0f);
   const float maxDistance = MAX_DISTANCE * uVoxelDims.y;
   const float halfSize = uVoxelDims.x * uVoxelDims.y * 0.5f;
   while(dist < maxDistance && Lv.a <= 1.0f) {
      float diameter = max(uVoxelDims.y, dist * coneCoefficient);
      float mip = log2(diameter * invVoxelDims);

	  vec3 position	= start	+ dist * direction;
      vec3 uv = ToVoxelSpace(position, halfSize);
      if(!IsInsideCube(uv) || mip > 5.0f) break;

      vec4 sam = textureLod(uVolumeTexture, uv, mip);
      if(sam.a > 0.0f) {
        float a = 1.0f - Lv.a;
		Lv.rgb += a	* sam.rgb;
		Lv.a +=	a *	sam.a;
      }
      dist += diameter * stepSize * 0.5f;
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
   col += calculateSpecularReflection(viewDir, material.roughness) * material.metallic;
   col /= (1.0f + col);
   col = pow(col, vec3(0.4545));
   fragColor = vec4(col, 1.0f);
}
