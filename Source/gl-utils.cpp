#include "gl-utils.h"
#include "logger.h"

#include <string>
#include <fstream>
#include <iostream>
#include <optional>

/*****************************************************************************************************************************************/
// Shader

static int EndWith(const char* s, const char* part)
{
	return (strstr(s, part) - s) == (strlen(s) - strlen(part));
}

/*****************************************************************************************************************************************/

static GLenum GetShaderTypeFromFile(const char* filename)
{
	if (EndWith(filename, ".vert"))
		return GL_VERTEX_SHADER;
	if (EndWith(filename, ".frag"))
		return GL_FRAGMENT_SHADER;
	if (EndWith(filename, ".geom"))
		return GL_GEOMETRY_SHADER;
	if (EndWith(filename, ".comp"))
		return GL_COMPUTE_SHADER;
	if (EndWith(filename, ".tesc"))
		return GL_TESS_CONTROL_SHADER;
	if (EndWith(filename, ".tese"))
		return GL_TESS_EVALUATION_SHADER;

	logger::Error("Invalid file format: " + std::string(filename));
	return 0;
}

/*****************************************************************************************************************************************/

static std::optional<std::string> ReadShaderFile(const char* filename)
{
	std::ifstream inFile(filename);
	if (!inFile)
	{
		logger::Error("Failed to read file: " + std::string(filename));
		return {};
	}
	return std::string{
		(std::istreambuf_iterator<char>(inFile)),
		std::istreambuf_iterator<char>()
	};
}

/*****************************************************************************************************************************************/

GLShader::GLShader(const char* filename) :
	GLShader(GetShaderTypeFromFile(filename), ReadShaderFile(filename).value().c_str())
{
}

GLShader::GLShader(GLenum type, const char* shaderCode) :
	type_(type),
	handle_(glCreateShader(type_))
{
	glShaderSource(handle_, 1, &shaderCode, nullptr);
	glCompileShader(handle_);

	char buffer[8192];
	GLsizei length = 0;
	glGetShaderInfoLog(handle_, sizeof(buffer), &length, buffer);
	if (length)
	{
		printf("%s\n", buffer);
		printf("%s\n", shaderCode);
		assert(0);
	}
}

/*****************************************************************************************************************************************/

// Shader Program

static
void printProgramInfoLog(GLuint handle_)
{
	char buffer[8192];
	GLsizei length = 0;

	glGetProgramInfoLog(handle_, sizeof(buffer), &length, buffer);
	if (length > 0)
	{
		printf("%s\n", buffer);
		assert(false);
	}
}

/*****************************************************************************************************************************************/

void GLProgram::init(GLShader a, GLShader b)
{
	handle_ = glCreateProgram();
	glAttachShader(handle_, a.getHandle());
	glAttachShader(handle_, b.getHandle());
	glLinkProgram(handle_);

	printProgramInfoLog(handle_);
}

void GLProgram::init(GLShader a, GLShader b, GLShader c)
{
	handle_ = glCreateProgram();
	glAttachShader(handle_, a.getHandle());
	glAttachShader(handle_, b.getHandle());
	glAttachShader(handle_, c.getHandle());
	glLinkProgram(handle_);

	printProgramInfoLog(handle_);
}


void GLProgram::setTexture(const std::string& name, int binding, unsigned int textureId, bool layered)
{
	setInt(name, binding);
	glActiveTexture(GL_TEXTURE0 + binding);
	if(layered)
		glBindTexture(GL_TEXTURE_3D, textureId);
	else
		glBindTexture(GL_TEXTURE_2D, textureId);
}

void GLProgram::setUAVTexture(int binding, uint32_t textureId, GLenum access, GLenum format, bool layered, int mipLevel)
{
	glBindImageTexture(binding, textureId, mipLevel, layered ? GL_TRUE : GL_FALSE, 0, access, format);
}

void GLProgram::setInt(const std::string& name, int val)
{
	glUniform1i(glGetUniformLocation(handle_, name.c_str()), val);
}

void GLProgram::setFloat(const std::string& name, float val)
{
	glUniform1f(glGetUniformLocation(handle_, name.c_str()), val);
}

void GLProgram::setVec2(const std::string& name, float* val)
{
	glUniform2fv(glGetUniformLocation(handle_, name.c_str()), 1, val);
}

void GLProgram::setVec3(const std::string& name, float* val)
{
	glUniform3fv(glGetUniformLocation(handle_, name.c_str()), 1, val);
}

void GLProgram::setVec4(const std::string& name, float* val)
{
	glUniform4fv(glGetUniformLocation(handle_, name.c_str()), 1, val);
}

void GLProgram::setMat4(const std::string& name, float* data)
{
	glUniformMatrix4fv(glGetUniformLocation(handle_, name.c_str()), 1, GL_FALSE, data);
}

void GLProgram::setBuffer(int binding, uint32_t bufferId)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, bufferId);
}

/*****************************************************************************************************************************************/

void GLComputeProgram::init(GLShader shader)
{
	handle_ = glCreateProgram();
	glAttachShader(handle_, shader.getHandle());
	glLinkProgram(handle_);

	printProgramInfoLog(handle_);
}

void GLComputeProgram::setTexture(int binding, uint32_t textureId, GLenum access, GLenum format, bool layered, int mipLevel)
{
	glBindImageTexture(binding, textureId, mipLevel, layered ? GL_TRUE : GL_FALSE, 0, access, format);
}

void GLComputeProgram::setBuffer(int binding, uint32_t bufferId)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, bufferId);
}

void GLComputeProgram::setAtomicCounterBuffer(int binding, uint32_t bufferId)
{
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, bufferId);
}

void GLComputeProgram::setVec2(const std::string& name, float* val) 
{
	glUniform2fv(glGetUniformLocation(handle_, name.c_str()), 1, val);
}

void GLComputeProgram::setInt(const std::string& name, int val)
{
	glUniform1i(glGetUniformLocation(handle_, name.c_str()), val);
}

void GLComputeProgram::setFloat(const std::string& name, float val)
{
	glUniform1f(glGetUniformLocation(handle_, name.c_str()), val);
}

void GLComputeProgram::setVec3(const std::string& name, float* val)
{
	glUniform3fv(glGetUniformLocation(handle_, name.c_str()), 1, val);
}

void GLComputeProgram::setVec4(const std::string& name, float* val, int count)
{
	glUniform4fv(glGetUniformLocation(handle_, name.c_str()), count, val);
}

void GLComputeProgram::dispatch(uint32_t workGroupX, uint32_t workGroupY, uint32_t workGroupZ) const
{
	glDispatchCompute(workGroupX, workGroupY, workGroupZ);
}

void GLBuffer::init(void* data, uint32_t size, GLbitfield flags)
{
	glCreateBuffers(1, &handle);
	glNamedBufferStorage(handle, size, data, flags);
}

void GLFramebuffer::init(const std::vector<Attachment>& attachments, TextureCreateInfo* depthAttachmentInfo)
{
	glGenFramebuffers(1, &handle);
	glBindFramebuffer(GL_FRAMEBUFFER, handle);

	this->attachments.resize(attachments.size());
	for (auto& attachment : attachments) {
		GLTexture texture;
		texture.init(attachment.attachmentInfo);
		this->attachments[attachment.index] = texture.handle;
		glFramebufferTexture2D(GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + attachment.index,
			attachment.attachmentInfo->target,
			texture.handle, 0);
	}

	if (depthAttachmentInfo) {
		GLTexture texture;
		texture.init(depthAttachmentInfo);
		depthAttachment = texture.handle;
		glFramebufferTexture2D(GL_FRAMEBUFFER,
			GL_DEPTH_STENCIL_ATTACHMENT,
			GL_TEXTURE_2D,
			texture.handle, 0);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		logger::Error("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void GLFramebuffer::destroy()
{
	glDeleteFramebuffers(1, &handle);
}

void GLTexture::init(TextureCreateInfo* createInfo, void* data)
{
	width = createInfo->width;
	height = createInfo->height;
	depth = createInfo->depth;
	internalFormat = createInfo->internalFormat;

	GLuint target = createInfo->target;
	glGenTextures(1, &handle);
	glBindTexture(target, handle);

	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, createInfo->minFilterType);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, createInfo->magFilterType);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, createInfo->wrapType);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, createInfo->wrapType);

	if (target == GL_TEXTURE_2D) {
		glTexImage2D(target,
			0,
			createInfo->internalFormat,
			createInfo->width,
			createInfo->height,
			0,
			createInfo->format,
			createInfo->dataType, data);
	}
	else {
		glTexParameteri(target, GL_TEXTURE_WRAP_R, createInfo->wrapType);
		glTexImage3D(target,
			0, 
			createInfo->internalFormat,
			createInfo->width,
			createInfo->height,
			createInfo->depth,
			0,
			createInfo->format,
			createInfo->dataType,
			data);
	}
	if (createInfo->generateMipmap)
		glGenerateMipmap(target);
}
