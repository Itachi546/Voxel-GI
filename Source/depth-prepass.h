#pragma once

#include <memory>
#include <stdint.h>

#include "mesh.h"

struct GLFramebuffer;
class GLProgram;
class Camera;

class DepthPrePass {

public:
	void Initialize(uint32_t width, uint32_t height);

	void Render(Camera* camera, std::vector<MeshGroup>& scene);

	unsigned int GetDepthAttachment();

	void Destroy();

private:
	uint32_t mWidth, mHeight;

	GLuint mTimerQuery;
	std::unique_ptr<GLProgram> mShader;
	std::unique_ptr<GLFramebuffer> mFramebuffer;
};