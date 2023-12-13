#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "glm-includes.h"
#include "gl-utils.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <sstream>
#include <chrono>
#include <ctime>

struct GLFWwindow;

enum class ImGuiCustomWindow {
	MainWindow = 0,
	Options = 1,
	Logs = 2,
};

static const char* gCustomWindowName[] = {
	"MainWindow",
	"Options",
	"Logs"
};

namespace ImGuiService {
	void Initialize(GLFWwindow* window);

	void NewFrame();

	void RenderDockSpace();

	void Image3D(ImTextureID user_texture_id,
		const ImVec2& size,
		float layer = 0,
		int channel = 0,
		const ImVec4& border_col = { 0.5f, 0.5f, 0.5f, 1.0f });

	static const char* CHANNELS_DROPDOWN[] = {
		"R\0",
		"R\0G\0",
		"R\0G\0B\0",
		"R\0G\0B\0A\0All\0",
	};
	
	inline void SelectableTexture3D(GLuint textureHandle, const ImVec2& size, float* layer, int* channel, int numChannel) {
		ImGui::SliderFloat("Layer", layer, 0.0f, 1.0f);
		ImGui::Combo("Channel", channel, CHANNELS_DROPDOWN[numChannel - 1]);
		ImGuiService::Image3D((ImTextureID)(uint64_t)textureHandle, size, *layer, *channel);
	}

	void Render(GLFWwindow* window);

	void Shutdown();
};