#pragma once

#include <memory>

#include "mesh.h"

class GLProgram;
class GLComputeProgram;
struct GLMesh;
struct GLFramebuffer;
struct GLBuffer;

class Voxelizer {
	
public:
	void Init(uint32_t voxelDims, float unitVoxelSize = 0.05f);

	void Generate(Scene* scene);

	void Visualize(Camera* camera);

	void AddUI();

	void Destroy();

	std::unique_ptr<GLFramebuffer> framebuffer;
	std::unique_ptr<GLTexture> voxelTexture;
	bool enableDebugVoxel = false;
	uint32_t mVoxelDims;
	float mUnitVoxelSize;
	float mDebugMipInterpolation = 0.0f;
	bool mRegenerateVoxelData = true;
private:
	std::unique_ptr<GLProgram> mProgram, mVisualizerProgram;
	std::unique_ptr<GLComputeProgram> mClearTextureProgram, mDrawCallGeneratorProgram;
	std::unique_ptr<GLBuffer> mDrawCommandBuffer, mDrawCountBuffer;

	const uint32_t MAX_VOXELS_ALLOCATED = 1'000'000;
	std::unique_ptr<GLMesh> mCubeMesh;
	uint32_t mTotalVoxels = 0;
	int mDebugMipLevel = 0;
};