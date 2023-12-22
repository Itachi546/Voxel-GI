#pragma once

#include "glm-includes.h"
#include "gl-utils.h"
#include <string>
#include <vector>

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

struct Material {
	glm::vec4 albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 emissive{ 0.0f, 0.0f, 0.0f, 0.0f };

	float metallic = 0.1f;
	float roughness = 0.5f;
	float ao = 1.0f;
	float transparency = 1.0f;

	uint32_t padding;
	uint32_t albedoMap = 0;
	uint32_t normalMap = 0;
	uint32_t emissiveMap = 0;

	uint32_t metallicMap = 0;
	uint32_t roughnessMap = 0;
	uint32_t ambientOcclusionMap = 0;
	uint32_t opacityMap = 0;
};

struct MeshGroup {
	GLBuffer vertexBuffer;
	GLBuffer indexBuffer;
	GLBuffer drawIndirectBuffer;
	GLBuffer transformBuffer;
	GLBuffer materialBuffer;

	GLuint vao;

	std::vector<glm::mat4> transforms;
	std::vector<AABB> aabbs;
	std::vector<DrawElementsIndirectCommand> drawCommands;
	std::vector<Material> materials;
	std::vector<std::string> names;

	void updateTransforms();
	void updateMaterials();

	void Draw(GLProgram* program);
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

class Camera;
struct Scene {
	std::vector<MeshGroup> meshGroup;
	glm::vec3 lightPosition;
	Camera* camera;
	GLMesh mLightMesh;
};

void LoadMesh(const std::string& filename, MeshGroup* meshGroup);
void InitializePlaneMesh(GLMesh* mesh, int width, int height);
void InitializeCubeMesh(GLMesh* mesh);