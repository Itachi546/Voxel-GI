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
	void Init(uint32_t voxelDims, float unitVoxelSize = 0.05f);

	void Generate(Camera* camera, std::vector<MeshGroup>& meshes);

	void Visualize(Camera* camera);

	void AddUI();

	void Destroy();

	std::unique_ptr<GLFramebuffer> framebuffer;
	std::unique_ptr<GLTexture> voxelTexture;
	bool enableDebugVoxel = false;
private:
	std::unique_ptr<GLProgram> mProgram, mVisualizerProgram;
	std::unique_ptr<GLComputeProgram> mClearTextureProgram, mDrawCallGeneratorProgram;
	std::unique_ptr<GLBuffer> mDrawCommandBuffer, mDrawCountBuffer;

	uint32_t mVoxelDims;
	float mUnitVoxelSize;

	std::unique_ptr<GLMesh> mCubeMesh;
	uint32_t mTotalVoxels = 0;

	bool mRegenerateVoxelData = true;
};