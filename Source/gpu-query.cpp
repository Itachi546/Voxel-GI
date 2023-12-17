#include "gpu-query.h"
#include "gl-utils.h"
#include "logger.h"
#include "imgui-service.h"

#include <stack>
#include <map>

namespace GpuProfiler {
	static const uint32_t QUERY_COUNT = 64;
	GLuint queries[QUERY_COUNT];

	struct QueryRange {
		GLuint startQuery;
		GLuint endQuery;
	};

	std::map<std::string, QueryRange> frameMap[2];
	std::stack<std::string> queryStack;

	uint32_t currentFrame = 0;
	uint32_t currentIndex = 0;

	void Initialize()
	{
		glGenQueries(QUERY_COUNT, queries);
	}

	static uint32_t GetCurrentQuery() {
		GLuint query = queries[currentFrame * (QUERY_COUNT / 2) + currentIndex];
		currentIndex++;
		return query;
	}

	void Begin(std::string name) {
		GLuint query = GetCurrentQuery();
		frameMap[currentFrame].insert(std::make_pair(name, QueryRange{ query, 0 }));
		queryStack.push(name);
		glQueryCounter(query, GL_TIMESTAMP);
	}

	void End() {
		std::string currentBlockName = queryStack.top();
		queryStack.pop();

		auto found = frameMap[currentFrame].find(currentBlockName);
		if (found == frameMap[currentFrame].end())
			logger::Error("Undefined profile block: " + currentBlockName);

		GLuint query = GetCurrentQuery();
		glQueryCounter(query, GL_TIMESTAMP);
		found->second.endQuery = query;
	}

	void AddUI() {
		uint32_t lastFrame = 1 - currentFrame;
		if (frameMap[lastFrame].size() == 0) return;

		ImGui::Text("Perfomance");
		ImGui::Separator();
		const uint32_t HALF_QUERY_COUNT = QUERY_COUNT / 2;
		for (auto& [key, val] : frameMap[lastFrame]) {

			uint64_t startTime, endTime;

			glGetQueryObjectui64v(val.startQuery,
				GL_QUERY_RESULT, &startTime);

			glGetQueryObjectui64v(val.endQuery,
				GL_QUERY_RESULT, &endTime);
			ImGui::Text("%s: %.2fms", key.c_str(), (endTime - startTime) / 1000000.0f);
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

	void Reset() {
		while (!queryStack.empty())
			queryStack.pop();

		currentFrame = 1 - currentFrame;
		frameMap[currentFrame].clear();
		currentIndex = 0;
	}

	void Shutdown() {
		glDeleteQueries(QUERY_COUNT, queries);
	}
}
