#include "voxelizer.h"

#include "gl-utils.h"
#include "camera.h"
#include "imgui-service.h"
#include "logger.h"
#include "utils.h"
#include "gpu-query.h"

void Voxelizer::Init(uint32_t voxelDims, float unitVoxelSize)
{
	mVoxelDims = voxelDims;
	mUnitVoxelSize = unitVoxelSize;
	{
		mProgram = std::make_unique<GLProgram>();
		GLShader vs("Assets/Shaders/voxelizer.vert");
		GLShader fs("Assets/Shaders/voxelizer.frag");
		GLShader gs("Assets/Shaders/voxelizer.geom");
		mProgram->init(vs, fs, gs);
	}

	{
		mClearTextureProgram = std::make_unique<GLComputeProgram>();
		mClearTextureProgram->init(GLShader{ "Assets/Shaders/clear-texture.comp" });

		mVisualizerProgram = std::make_unique<GLProgram>();
		mVisualizerProgram->init(GLShader{ "Assets/Shaders/visualizer.vert" }, GLShader{ "Assets/Shaders/visualizer.frag" });

		mDrawCallGeneratorProgram = std::make_unique<GLComputeProgram>();
		mDrawCallGeneratorProgram->init(GLShader{ "Assets/Shaders/draw-call.comp" });
	}

	mDrawCommandBuffer = std::make_unique<GLBuffer>();
	// Support only 10'000 voxels
	uint32_t bufferSize = sizeof(float) * 3 * MAX_VOXELS_ALLOCATED;
	mDrawCommandBuffer->init(nullptr, bufferSize, 0);

	mDrawCountBuffer = std::make_unique<GLBuffer>();
	mDrawCountBuffer->init(0, sizeof(uint32_t), GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	TextureCreateInfo colorAttachment{ voxelDims, voxelDims };
	GLFramebuffer mainFBO;
	mainFBO.init({ Attachment{ 0, &colorAttachment } }, nullptr);
	framebuffer = std::make_unique<GLFramebuffer>();
	framebuffer->init({ Attachment{0, &colorAttachment} }, nullptr);

	TextureCreateInfo volumeTextureCreateInfo{ voxelDims, voxelDims, voxelDims, GL_RGBA, GL_RGBA8, GL_TEXTURE_3D, GL_UNSIGNED_BYTE};
	volumeTextureCreateInfo.mipLevels = 6;
	volumeTextureCreateInfo.wrapType = GL_CLAMP_TO_EDGE;
	volumeTextureCreateInfo.minFilterType = GL_LINEAR_MIPMAP_LINEAR;

	voxelTexture = std::make_unique<GLTexture>();
	voxelTexture->init(&volumeTextureCreateInfo);

	mCubeMesh = std::make_unique<GLMesh>();
	InitializeCubeMesh(mCubeMesh.get());
}

void Voxelizer::Generate(Scene* scene)
{
	if (mRegenerateVoxelData == false) return;
	mRegenerateVoxelData = false;

	GpuProfiler::Begin("Clear Voxel Texture");

	mClearTextureProgram->bind();
	mClearTextureProgram->setTexture(0, voxelTexture->handle, GL_WRITE_ONLY, voxelTexture->internalFormat, true);
	uint32_t workGroupSize = (mVoxelDims + 7) / 8;
	mClearTextureProgram->dispatch(workGroupSize, workGroupSize, workGroupSize);
	mClearTextureProgram->unbind();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	GpuProfiler::End();

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	GpuProfiler::Begin("Voxelize Pass");
	framebuffer->bind();
	framebuffer->setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	framebuffer->setViewport(mVoxelDims, mVoxelDims);
	framebuffer->clear();

	Camera* camera = scene->camera;

	mProgram->bind();
	glm::mat4 VP = camera->GetViewProjectionMatrix();
	glm::mat4 V = camera->GetViewMatrix();
	glm::vec2 voxelDims{ mVoxelDims, mUnitVoxelSize };
	mProgram->setVec2("uVoxelDims", &voxelDims[0]);
	mProgram->setVec3("uLightPosition", &scene->lightPosition[0]);
	mProgram->setUAVTexture(0, voxelTexture->handle, GL_WRITE_ONLY,  voxelTexture->internalFormat, true);

	for (auto& mesh : scene->meshGroup) {
		mesh.Draw(mProgram.get());
	}

	mProgram->unbind();
	framebuffer->unbind();

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	GpuProfiler::End();

	GpuProfiler::Begin("Texture Mipmap Generation");
	glBindTexture(GL_TEXTURE_3D, voxelTexture->handle);
	glGenerateMipmap(GL_TEXTURE_3D);
	GpuProfiler::End();

}

void Voxelizer::Visualize(Camera* camera)
{
	GpuProfiler::Begin("Voxel Instance Data Generation");
	int voxelDims = mVoxelDims >> mDebugMipLevel;
	float unitVoxelSize = (float)(mUnitVoxelSize * std::pow(2.0f, mDebugMipLevel));

	uint32_t workGroupSize = (voxelDims + 7) / 8;
	mDrawCallGeneratorProgram->bind();
	mDrawCallGeneratorProgram->setInt("uVoxelDims", voxelDims);
	mDrawCallGeneratorProgram->setFloat("uVoxelSpan", mUnitVoxelSize * mVoxelDims * 0.5f);
	mDrawCallGeneratorProgram->setFloat("uUnitVoxelSize", unitVoxelSize);
	mDrawCallGeneratorProgram->setVec4("frustumPlanes", (float*)camera->frustumPlanes.data(), 6);

	mDrawCallGeneratorProgram->setTexture(0, voxelTexture->handle, GL_READ_ONLY, voxelTexture->internalFormat, true, mDebugMipLevel);
	mDrawCallGeneratorProgram->setBuffer(1, mDrawCommandBuffer->handle);
	mDrawCallGeneratorProgram->setAtomicCounterBuffer(2, mDrawCountBuffer->handle);

	mDrawCallGeneratorProgram->dispatch(workGroupSize, workGroupSize, workGroupSize);
	mDrawCallGeneratorProgram->unbind();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
	GpuProfiler::End();

	mVisualizerProgram->bind();

	uint32_t* count = (uint32_t*)glMapNamedBuffer(mDrawCountBuffer->handle, GL_READ_WRITE);
	mTotalVoxels = count[0];
	// Reset the count for next frame
	count[0] = 0;
	glUnmapNamedBuffer(mDrawCountBuffer->handle);

	glm::mat4 VP = camera->GetViewProjectionMatrix();
	mVisualizerProgram->setMat4("uVP", &VP[0][0]);

	mVisualizerProgram->setInt("uVoxelDims", voxelDims);
	mVisualizerProgram->setFloat("uVoxelSpan", mUnitVoxelSize * mVoxelDims * 0.5f);
	mVisualizerProgram->setFloat("uUnitVoxelSize", unitVoxelSize);

	mVisualizerProgram->setBuffer(0, mDrawCommandBuffer->handle);
	mVisualizerProgram->setUAVTexture(0, voxelTexture->handle, GL_READ_ONLY, voxelTexture->internalFormat, true, mDebugMipLevel);
	mCubeMesh->drawInstanced(mTotalVoxels);
	mVisualizerProgram->unbind();
}

void Voxelizer::AddUI()
{
	static float layer = 0.0f;
	static int channel = 4;
	ImGui::Text("Voxel Count: %d", mTotalVoxels);
	if (mTotalVoxels > MAX_VOXELS_ALLOCATED) {
		static const ImVec4 RED{ 1.0f, 0.0f, 0.0f, 1.0f };
		ImGui::TextColored(RED, "Insufficient memory to render additional: %d voxels.", mTotalVoxels - MAX_VOXELS_ALLOCATED);
		ImGui::TextColored(RED, "Select lower mip level");
	}

	if (ImGui::DragFloat("VoxelSize", &mUnitVoxelSize, 0.01f, 0.01f, 1.0f)) {
		mRegenerateVoxelData = true;
	}

	ImGui::SliderInt("Debug MipLevel", &mDebugMipLevel, 0, 5);
	ImGui::SliderFloat("Mip Interpolation", &mDebugMipInterpolation, 0.0f, 5.0f);

	ImGui::Checkbox("Show Voxels", &enableDebugVoxel);

	static bool showTexture = false;
	ImGui::Checkbox("Show Texture", &showTexture);
	if (showTexture) {
		ImGui::Text("3D Texture");
		ImGuiService::SelectableTexture3D(voxelTexture->handle, ImVec2{ (float)mVoxelDims, (float)mVoxelDims }, &layer, &channel, 4);
	}
}

void Voxelizer::Destroy()
{
	mDrawCommandBuffer->destroy();
	mDrawCountBuffer->destroy();
	mDrawCallGeneratorProgram->destroy();
	mProgram->destroy();
	mVisualizerProgram->destroy();
	mClearTextureProgram->destroy();
	framebuffer->destroy();
	voxelTexture->destroy();
}
