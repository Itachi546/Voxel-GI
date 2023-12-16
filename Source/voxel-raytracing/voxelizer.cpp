#include "voxelizer.h"

#include "gl-utils.h"
#include "camera.h"
#include "imgui-service.h"
#include "logger.h"
#include "utils.h"

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
	uint32_t bufferSize = sizeof(float) * 3 * 2'000'000;
	mDrawCommandBuffer->init(nullptr, bufferSize, 0);

	mDrawCountBuffer = std::make_unique<GLBuffer>();
	mDrawCountBuffer->init(0, sizeof(uint32_t), GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	TextureCreateInfo colorAttachment{ voxelDims, voxelDims };
	GLFramebuffer mainFBO;
	mainFBO.init({ Attachment{ 0, &colorAttachment } }, nullptr);
	framebuffer = std::make_unique<GLFramebuffer>();
	framebuffer->init({ Attachment{0, &colorAttachment} }, nullptr);

	TextureCreateInfo volumeTextureCreateInfo{ voxelDims, voxelDims, voxelDims, GL_RED_INTEGER, GL_R32UI, GL_TEXTURE_3D, GL_UNSIGNED_INT};
	voxelTexture = std::make_unique<GLTexture>();
	voxelTexture->init(&volumeTextureCreateInfo);

	mCubeMesh = std::make_unique<GLMesh>();
	InitializeCubeMesh(mCubeMesh.get());
}

void Voxelizer::Generate(Camera* camera, std::vector<MeshGroup>& meshes)
{
	if (mRegenerateVoxelData == false) return;
	mRegenerateVoxelData = false;

	mClearTextureProgram->bind();
	mClearTextureProgram->setTexture(0, voxelTexture->handle, GL_WRITE_ONLY, voxelTexture->internalFormat, true);
	uint32_t workGroupSize = (mVoxelDims + 7) / 8;
	mClearTextureProgram->dispatch(workGroupSize, workGroupSize, workGroupSize);
	mClearTextureProgram->unbind();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	framebuffer->bind();
	framebuffer->setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	framebuffer->setViewport(mVoxelDims, mVoxelDims);
	framebuffer->clear();

	mProgram->bind();
	glm::mat4 VP = camera->GetViewProjectionMatrix();
	glm::mat4 V = camera->GetViewMatrix();
	glm::vec2 voxelDims{ mVoxelDims, mUnitVoxelSize };
	mProgram->setVec2("uVoxelDims", &voxelDims[0]);

	mProgram->setUAVTexture(0, voxelTexture->handle, GL_WRITE_ONLY,  voxelTexture->internalFormat, true);

	for (auto& mesh : meshes) {
		mesh.Draw(mProgram.get());
	}

	mProgram->unbind();
	framebuffer->unbind();

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindTexture(GL_TEXTURE_3D, voxelTexture->handle);
	glGenerateMipmap(GL_TEXTURE_3D);
}

void Voxelizer::Visualize(Camera* camera)
{
	if (enableDebugVoxel) {

		uint32_t workGroupSize = (mVoxelDims + 7) / 8;
		mDrawCallGeneratorProgram->bind();
		mDrawCallGeneratorProgram->setInt("uVoxelDims", mVoxelDims);
		mDrawCallGeneratorProgram->setFloat("uVoxelSpan", mUnitVoxelSize * mVoxelDims * 0.5f);
		mDrawCallGeneratorProgram->setFloat("uUnitVoxelSize", mUnitVoxelSize);
		mDrawCallGeneratorProgram->setVec4("frustumPlanes", (float*)camera->frustumPlanes.data(), 6);

		mDrawCallGeneratorProgram->setTexture(0, voxelTexture->handle, GL_READ_ONLY, voxelTexture->internalFormat, true);
		mDrawCallGeneratorProgram->setBuffer(1, mDrawCommandBuffer->handle);
		mDrawCallGeneratorProgram->setAtomicCounterBuffer(2, mDrawCountBuffer->handle);

		mDrawCallGeneratorProgram->dispatch(workGroupSize, workGroupSize, workGroupSize);
		mDrawCallGeneratorProgram->unbind();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

		mVisualizerProgram->bind();

		uint32_t* count = (uint32_t*)glMapNamedBuffer(mDrawCountBuffer->handle, GL_READ_WRITE);
		mTotalVoxels = count[0];
		// Reset the count for next frame
		count[0] = 0;
		glUnmapNamedBuffer(mDrawCountBuffer->handle);

		glm::mat4 VP = camera->GetViewProjectionMatrix();
		mVisualizerProgram->setMat4("uVP", &VP[0][0]);

		mVisualizerProgram->setInt("uVoxelDims", mVoxelDims);
		mVisualizerProgram->setFloat("uVoxelSpan", mUnitVoxelSize * mVoxelDims * 0.5f);
		mVisualizerProgram->setFloat("uUnitVoxelSize", mUnitVoxelSize);

		mVisualizerProgram->setBuffer(0, mDrawCommandBuffer->handle);
		mVisualizerProgram->setUAVTexture(0, voxelTexture->handle, GL_READ_ONLY, voxelTexture->internalFormat, true);
		mCubeMesh->drawInstanced(mTotalVoxels);
		mVisualizerProgram->unbind();
	}
}

void Voxelizer::AddUI()
{
	static float layer = 0.0f;
	static int channel = 0;
	ImGui::Text("Voxel Count: %d", mTotalVoxels);
	if (ImGui::SliderFloat("VoxelSize", &mUnitVoxelSize, 0.01f, 1.0f)) {
		mRegenerateVoxelData = true;
	}

	ImGui::Checkbox("Show Voxels", &enableDebugVoxel);

	static bool showTexture = false;
	ImGui::Checkbox("Show Texture", &showTexture);
	if (showTexture) {
		ImGui::Text("3D Texture");
		ImGuiService::SelectableTexture3D(voxelTexture->handle, ImVec2{ 512, 512 }, &layer, &channel, 1);
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
