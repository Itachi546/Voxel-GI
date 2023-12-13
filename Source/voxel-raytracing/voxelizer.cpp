#include "voxelizer.h"

#include "gl-utils.h"
#include "camera.h"
#include "imgui-service.h"


void Voxelizer::Init(uint32_t voxelDims, uint32_t voxelSize)
{
	mVoxelDims = voxelDims;
	mVoxelSize = voxelSize;

	mProgram = std::make_unique<GLProgram>();
	GLShader vs("Assets/Shaders/voxelizer.vert");
	GLShader fs("Assets/Shaders/voxelizer.frag");
	GLShader gs("Assets/Shaders/voxelizer.geom");
	mProgram->init(vs, fs, gs);

	GLShader cs("Assets/Shaders/clear-texture.comp");
	mClearTextureProgram = std::make_unique<GLComputeProgram>();
	mClearTextureProgram->init(cs);

	TextureCreateInfo colorAttachment{ voxelDims, voxelDims };
	GLFramebuffer mainFBO;
	mainFBO.init({ Attachment{ 0, &colorAttachment } }, nullptr);
	framebuffer = std::make_unique<GLFramebuffer>();
	framebuffer->init({ Attachment{0, &colorAttachment} }, nullptr);

	TextureCreateInfo volumeTextureCreateInfo{ voxelDims, voxelDims, voxelDims, GL_RGBA, GL_RGBA8, GL_TEXTURE_3D, GL_UNSIGNED_BYTE};
	voxelTexture = std::make_unique<GLTexture>();
	voxelTexture->init(&volumeTextureCreateInfo);

	float halfDims = voxelDims * 0.5f;
	mVoxelSpaceTransform = glm::ortho(-halfDims, halfDims, -halfDims, halfDims, -halfDims, halfDims);
}

void Voxelizer::Render(Camera* camera, std::vector<Mesh>& meshes)
{
	mClearTextureProgram->bind();
	mClearTextureProgram->setTexture(0, voxelTexture->handle, GL_WRITE_ONLY, voxelTexture->internalFormat, true);
	uint32_t workGroupSize = (mVoxelDims + 7) / 8;
	mClearTextureProgram->dispatch(workGroupSize, workGroupSize, workGroupSize);
	mClearTextureProgram->unbind();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	framebuffer->bind();
	framebuffer->setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	framebuffer->setViewport(mVoxelDims, mVoxelDims);
	framebuffer->clear();

	mProgram->bind();
	glm::mat4 VP = camera->GetViewProjectionMatrix();
	glm::mat4 V = camera->GetViewMatrix();
	mProgram->setMat4("uVP", &VP[0][0]);
	mProgram->setMat4("uView", &V[0][0]);
	mProgram->setMat4("uVoxelSpaceTransform", &mVoxelSpaceTransform[0][0]);
	mProgram->setInt("uVoxelDims", mVoxelDims);
	mProgram->setUAVTexture(0, voxelTexture->handle, GL_READ_WRITE,  voxelTexture->internalFormat, true);
	for (auto& mesh : meshes) {
		mProgram->setMat4("uModel", &mesh.modalMatrix[0][0]);
		mesh.glMesh.draw();
	}

	mProgram->unbind();
	framebuffer->unbind();

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Voxelizer::AddUI()
{
	static float layer = 0.0f;
	static int channel = 4;
	ImGuiService::SelectableTexture3D(voxelTexture->handle, ImVec2{ 512, 512 }, &layer, &channel, 4);
}

void Voxelizer::Destroy()
{
}
