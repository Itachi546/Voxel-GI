#pragma once

#include <string>

namespace GpuProfiler {
	void Initialize();

	void Begin(std::string name);

	void End();

	void AddUI();

	void Reset();

	void Shutdown();
};