#include "depth-prepass.h"

#include "gl-utils.h"
#include "camera.h"
#include "gpu-query.h"

void DepthPrePass::Initialize(uint32_t width, uint32_t height) 
{
	mWidth = width;
	mHeight = height;

	mFramebuffer = std::make_unique<GLFramebuffer>();

	TextureCreateInfo createInfo = {};
	InitializeDepthTexture(&createInfo, width, height);
	mFramebuffer->init({}, &createInfo);

	mShader = std::make_unique<GLProgram>();
	mShader->init(GLShader("Assets/Shaders/depth-prepass.vert"), GLShader("Assets/Shaders/depth-prepass.frag"));

	glGenQueries(1, &mTimerQuery);
}

void DepthPrePass::Render(Camera* camera, std::vector<MeshGroup>& scene)
{
	GpuProfiler::Begin("Depth Prepass");
	mFramebuffer->bind();
	mFramebuffer->setViewport(mWidth, mHeight);
	mFramebuffer->clear(true);

	mShader->bind();
	glm::mat4 VP = camera->GetViewProjectionMatrix();
	mShader->setMat4("uVP", &VP[0][0]);
	for (auto& meshGroup : scene)
		meshGroup.Draw(mShader.get());
	mShader->unbind();
	mFramebuffer->unbind();
	GpuProfiler::End();
}

unsigned int DepthPrePass::GetDepthAttachment()
{
	return mFramebuffer->depthAttachment;
}

void DepthPrePass::Destroy()
{
	mFramebuffer->destroy();
	mShader->destroy();
}
