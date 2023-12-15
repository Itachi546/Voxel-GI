#pragma once

#include "glm-includes.h"
#include "gl-utils.h"
#include <string>
#include <vector>

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

struct MeshGroup {
	GLBuffer vertexBuffer;
	GLBuffer indexBuffer;
	GLBuffer drawIndirectBuffer;
	GLBuffer transformBuffer;

	GLuint vao;

	std::vector<glm::mat4> transforms;
	std::vector<AABB> aabbs;
	std::vector<DrawElementsIndirectCommand> drawCommands;

	void Draw(GLProgram* program);
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

void LoadMesh(const std::string& filename, MeshGroup* meshGroup);
void InitializePlaneMesh(GLMesh* mesh, int width, int height);
void InitializeCubeMesh(GLMesh* mesh);