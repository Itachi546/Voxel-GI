#pragma once

#include <memory>

#include "mesh.h"

class GLProgram;
class GLComputeProgram;
struct GLMesh;
struct GLFramebuffer;
class Camera;

class Voxelizer {
	
public:
	void Init(uint32_t voxelDims, uint32_t voxelSize = 1);

	void Render(Camera* camera, std::vector<Mesh>& meshes);

	void AddUI();

	void Destroy();

	std::unique_ptr<GLFramebuffer> framebuffer;
	std::unique_ptr<GLTexture> voxelTexture;
private:
	std::unique_ptr<GLProgram> mProgram;
	std::unique_ptr<GLComputeProgram> mClearTextureProgram;
	uint32_t mVoxelDims;
	uint32_t mVoxelSize;
	glm::mat4 mVoxelSpaceTransform;

};