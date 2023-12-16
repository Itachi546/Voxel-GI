#pragma once

#include "glm-includes.h"

#include <string>

struct GLMesh;
struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

namespace Utils {

	bool RayBoxIntersection(const Ray& ray, const glm::vec3& min, const glm::vec3& max, glm::vec2& t);

	glm::vec3 GetRayDir(const glm::mat4& P, const glm::mat4& V, const glm::vec2& mouseCoord);

	unsigned char* LoadImage(const char* filename, int* width, int* height, int* nChannel);

	float* LoadImageFloat(const char* filename, int* width, int* height, int* nChannel);

	bool FrustumBoxIntersection(const glm::vec3& min, const glm::vec3& max, glm::vec4* frustumPlanes);
	void FreeImage(void* buffer);
}