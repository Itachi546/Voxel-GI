#pragma once

#include <memory>

#include "mesh.h"

class GLProgram;
class GLComputeProgram;
struct GLMesh;
struct GLFramebuffer;
class Camera;
struct GLBuffer;

class Voxelizer {
	
public:
	void Init(uint32_t voxelDims, uint32_t voxelSize = 1);

	void Generate(Camera* camera, std::vector<Mesh>& meshes);

	void Visualize(Camera* camera);

	void AddUI();

	void Destroy();

	std::unique_ptr<GLFramebuffer> framebuffer;
	std::unique_ptr<GLTexture> voxelTexture;
	bool enableDebugVoxel = true;
private:
	std::unique_ptr<GLProgram> mProgram, mVisualizerProgram;
	std::unique_ptr<GLComputeProgram> mClearTextureProgram, mDrawCallGeneratorProgram;
	std::unique_ptr<GLBuffer> mDrawCommandBuffer, mDrawCountBuffer;

	uint32_t mVoxelDims;
	//uint32_t mVoxelSize;
	glm::mat4 mVoxelSpaceTransform;

	std::unique_ptr<GLMesh> mCubeMesh;
	uint32_t mTotalVoxels = 0;

};