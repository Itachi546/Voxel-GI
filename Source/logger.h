#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <iostream>

namespace logger {
	static std::vector<std::string> gLogs;

	static void AddLog(const std::string& logLevel, const std::string& message) {
		gLogs.emplace_back(std::string{"[" + logLevel+ "]: " + message});
		std::cout << "[" << logLevel << "]: " << message << std::endl;
	}

	inline void Debug(const std::string log) {
		AddLog("DEBUG", log);
	}

	inline void Warn(const std::string log) {
		AddLog("WARN", log);
	}

	inline void Error(const std::string log) {
		AddLog("ERROR", log);
		assert(0);
	}

	inline std::vector<std::string>& GetAllLogs() {
		return gLogs;
	}

};