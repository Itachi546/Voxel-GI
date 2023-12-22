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

void DepthPrePass::Render(Scene* scene)
{
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	GpuProfiler::Begin("Depth Prepass");
	mFramebuffer->bind();
	mFramebuffer->setViewport(mWidth, mHeight);
	mFramebuffer->clear(true);

	mShader->bind();
	glm::mat4 VP = scene->camera->GetViewProjectionMatrix();
	mShader->setMat4("uVP", &VP[0][0]);
	for (auto& meshGroup : scene->meshGroup)
		meshGroup.Draw(mShader.get());
	mShader->unbind();
	mFramebuffer->unbind();
	GpuProfiler::End();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
