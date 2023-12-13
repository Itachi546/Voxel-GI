#include "debug-draw.h"

namespace DebugDraw {

	static GLBuffer gLineBuffer;
	static uint32_t gLineBufferOffset = 0;
	static Line* gLineBufferPtr = nullptr;
	static GLProgram gLineProgram;
	static const int MAX_LINE_COUNT = 100;

	void Initialize() {
		uint32_t bufferSize = MAX_LINE_COUNT * sizeof(Line);
		gLineBuffer.init(nullptr, bufferSize, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT);

		GLShader vs("Assets/Shaders/line.vert");
		GLShader fs("Assets/Shaders/line.frag");
		gLineProgram.init(vs, fs);
	}

	void InitializeBufferPtr() {
		if (gLineBufferPtr == nullptr) {
			gLineBufferPtr = reinterpret_cast<Line*>(glMapNamedBuffer(gLineBuffer.handle, GL_WRITE_ONLY));
		}
	}

	void AddLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color) {
		InitializeBufferPtr();

		Line* line = (gLineBufferPtr + gLineBufferOffset);
		line->p0 = p0;
		line->p1 = p1;
		line->c0 = line->c1 = color;
		gLineBufferOffset += 1;
	}

	void AddRect(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) {
		InitializeBufferPtr();

		Line* line = (gLineBufferPtr + gLineBufferOffset);

		glm::vec3 size = max - min;
		// Bottom
		line->p0 = min;
		line->p1 = { min.x + size.x, min.y, min.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x + size.x, min.y, min.z };
		line->p1 = { min.x + size.x, min.y, min.z + size.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x + size.x, min.y, min.z + size.z }; 
		line->p1 = { min.x, min.y, min.z + size.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x, min.y, min.z + size.z };
		line->p1 = { min.x, min.y, min.z };
		line->c0 = line->c1 = color;
		line++;

		// Top
		line->p0 = { min.x, max.y, min.z };
		line->p1 = { min.x + size.x, max.y, min.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x + size.x, max.y, min.z };
		line->p1 = { min.x + size.x, max.y, min.z + size.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x + size.x, max.y, min.z + size.z };
		line->p1 = { min.x, max.y, min.z + size.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x, max.y, min.z + size.z };
		line->p1 = { min.x, max.y, min.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x, min.y, min.z };
		line->p1 = { min.x, min.y + size.y, min.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x + size.x, min.y, min.z };
		line->p1 = { min.x + size.x, min.y + size.y, min.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x + size.x, min.y, min.z + size.z };
		line->p1 = { min.x + size.x, min.y + size.y, min.z + size.z };
		line->c0 = line->c1 = color;
		line++;

		line->p0 = { min.x, min.y, min.z + size.z };
		line->p1 = { min.x, min.y + size.y, min.z + size.z };
		line->c0 = line->c1 = color;
		line++;

		gLineBufferOffset += 12;
	}

	void Render(glm::mat4 VP, glm::vec2 windowSize) {
		if (gLineBufferOffset == 0) return;
		glUnmapNamedBuffer(gLineBuffer.handle);
		gLineBufferPtr = nullptr;

		uint32_t numLine = gLineBufferOffset;
		gLineBufferOffset = 0;

		// Draw Line
		gLineProgram.bind();
		gLineProgram.setMat4("VP", &VP[0][0]);

		glLineWidth(2.0f);
		glBindBuffer(GL_ARRAY_BUFFER, gLineBuffer.handle);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));

		glDrawArrays(GL_LINES, 0, numLine * 2);

		glLineWidth(1.0f);
		gLineProgram.unbind();
	}

	void Shutdown() {
		gLineBuffer.destroy();
		gLineProgram.destroy();
	}

}