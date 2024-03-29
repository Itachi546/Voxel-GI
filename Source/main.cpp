#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"
#include "utils.h"
#include "camera.h"
#include "mesh.h"
#include "debug-draw.h"
#include "gpu-query.h"

#include "voxel-raytracing/voxelizer.h"
#include <iostream>

#include "depth-prepass.h"

struct WindowProps {
	GLFWwindow* window;
	int width;
	int height;

	float mouseX = 0.0f;
	float mouseY = 0.0f;

	float mDx = 0.0f;
	float mDy = 0.0f;

	bool mouseDown = false;
};

Camera gCamera;

WindowProps gWindowProps = {
	nullptr, 1360, 769
};

uint32_t gFBOWidth = 1920;
uint32_t gFBOHeight = 1080;

static void on_window_resize(GLFWwindow* window, int width, int height) {
	gWindowProps.width = std::max(width, 2);
	gWindowProps.height = std::max(height, 2);

	gCamera.SetAspect(float(width) / float(height));
}

static void on_key_press(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, true);
}

static void on_mouse_move(GLFWwindow* window, double mouseX, double mouseY) {
	float x = static_cast<float>(mouseX);
	float y = static_cast<float>(mouseY);
	gWindowProps.mDx = x - gWindowProps.mouseX;
	gWindowProps.mDy = y - gWindowProps.mouseY;

	gWindowProps.mouseX = x;
	gWindowProps.mouseY = y;
}

static void on_mouse_down(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_RELEASE) {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gWindowProps.mouseDown = false;
	}
	else {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gWindowProps.mouseDown = true;
	}
}

void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (type != GL_DEBUG_TYPE_ERROR) return;
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

void MoveCamera(float dt, bool isWindowActive) {
	if (gWindowProps.mouseDown && isWindowActive)
		gCamera.Rotate(-gWindowProps.mDy, -gWindowProps.mDx, dt);

	float walkSpeed = dt * 1.0f;
	if (glfwGetKey(gWindowProps.window, GLFW_KEY_LEFT_SHIFT) != GLFW_RELEASE)
		walkSpeed *= 4.0f;

	if (glfwGetKey(gWindowProps.window, GLFW_KEY_W) != GLFW_RELEASE)
		gCamera.Walk(-walkSpeed);
	else if (glfwGetKey(gWindowProps.window, GLFW_KEY_S) != GLFW_RELEASE)
		gCamera.Walk(walkSpeed);

	if (glfwGetKey(gWindowProps.window, GLFW_KEY_A) != GLFW_RELEASE)
		gCamera.Strafe(-walkSpeed);
	else if (glfwGetKey(gWindowProps.window, GLFW_KEY_D) != GLFW_RELEASE)
		gCamera.Strafe(walkSpeed);

	if (glfwGetKey(gWindowProps.window, GLFW_KEY_1) != GLFW_RELEASE)
		gCamera.Lift(walkSpeed);
	else if (glfwGetKey(gWindowProps.window, GLFW_KEY_2) != GLFW_RELEASE)
		gCamera.Lift(-walkSpeed);
}
/*
bool AddGameObjectUI(Scene* scene) {
	bool needUpdate = false;
	for (auto& meshGroup : scene->meshGroup) {
		if (ImGui::CollapsingHeader("MeshGroup")) {
			bool changed = false;
			for (std::size_t i = 0; i < meshGroup.names.size(); ++i) {
				ImGui::PushID((int)i);
				if (ImGui::CollapsingHeader(meshGroup.names[i].c_str())) {
					changed |= ImGui::ColorEdit3("Albedo", &meshGroup.materials[i].albedo[0]);
					changed |= ImGui::ColorEdit3("Emissive", &meshGroup.materials[i].emissive[0]);
					changed |= ImGui::SliderFloat("Metallic", &meshGroup.materials[i].metallic, 0.0f, 1.0f);
					changed |= ImGui::SliderFloat("Roughness", &meshGroup.materials[i].roughness, 0.0f, 1.0f);
				}
				ImGui::PopID();
			}
			if (changed)
				meshGroup.updateMaterials();
			needUpdate |= changed;
		}
	}
	return needUpdate;
}
*/
void InitializeCornellBoxScene(Scene* scene) {
	scene->lightPosition = glm::vec3(0.0f, 1.0f, -.5f);
	scene->camera->SetPosition(glm::vec3(0.0f, 1.0f, 2.0f));
	scene->meshGroup.push_back(MeshGroup{});
	MeshGroup& cornellBox = scene->meshGroup.back();
	LoadMesh("C:/Users/Dell/OneDrive/Documents/3D-Assets/Models/cornell-box/cornell-dragon2.gltf", &cornellBox);

	for (uint32_t i = 0; i < cornellBox.names.size(); ++i) {
		if (cornellBox.names[i] == "light") {
			scene->lightPosition = glm::vec3(cornellBox.transforms[i] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			break;
		}
		else if (cornellBox.names[i] == "dragon") {
			cornellBox.materials[i].metallic = 0.5f;
			cornellBox.materials[i].roughness = 0.1f;
		}
	}
}

void InitializeSponzaScene(Scene* scene) {
	scene->lightPosition = glm::vec3(0.0f, 10.0f, -.5f);
	scene->camera->SetPosition(glm::vec3(0.0f, 1.0f, 2.0f));
	scene->meshGroup.push_back(MeshGroup{});
	MeshGroup& sponza = scene->meshGroup.back();
	LoadMesh("C:/Users/Dell/OneDrive/Documents/3D-Assets/Models/sponza/sponza.gltf", &sponza);
}

int main() {

	if (!glfwInit()) return 1;

	GLFWwindow* window = glfwCreateWindow(gWindowProps.width, gWindowProps.height, "Hello OpenGL", 0, 0);
	gWindowProps.window = window;

	if (window == nullptr)
	{
		std::cerr << "Failed to create Window" << std::endl;
		return 1;
	}
	logger::Debug("Initialized GLFW Window ...");

	glfwSetWindowSizeCallback(window, on_window_resize);
	glfwSetKeyCallback(window, on_key_press);
	glfwSetCursorPosCallback(window, on_mouse_move);
	glfwSetMouseButtonCallback(window, on_mouse_down);
	glfwSwapInterval(1);
	glfwMakeContextCurrent(window);

	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == -1) {
		std::cerr << "Failed to initialize OpenGL" << std::endl;
		return -1;
	}

	logger::Debug("Initialized GLAD ...");
	glEnable(GL_DEBUG_OUTPUT);
	logger::Debug("Enabled Debug Callback ...");
	glDebugMessageCallback(MessageCallback, 0);

	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	logger::Debug("Renderer: " + std::string(renderer));
	logger::Debug("Vendor: " + std::string(vendor));

	glEnable(GL_DEPTH_TEST);

	DebugDraw::Initialize();
	ImGuiService::Initialize(window);
	GpuProfiler::Initialize();

	float startTime = (float)glfwGetTime();
	float dt = 1.0f / 60.0f;

	gCamera.SetAspect(float(gWindowProps.width) / float(gWindowProps.height));
	gCamera.SetNearPlane(0.1f);
	Scene scene;
	scene.camera = &gCamera;

	InitializeCornellBoxScene(&scene);

	Voxelizer voxelizer;
	voxelizer.Init(64, 0.1f);

	TextureCreateInfo colorAttachment = { gFBOWidth, gFBOHeight };
	TextureCreateInfo depthAttachment;
	InitializeDepthTexture(&depthAttachment, gFBOWidth, gFBOHeight);

	DepthPrePass depthPrePass;
	depthPrePass.Initialize(gFBOWidth, gFBOHeight);

	GLFramebuffer mainFBO;
	mainFBO.init({ Attachment{ 0, &colorAttachment } }, depthPrePass.GetDepthAttachment());

	GLProgram mainProgram;
	mainProgram.init(GLShader("Assets/Shaders/mesh.vert"), GLShader("Assets/Shaders/mesh.frag"));

	bool wireframeMode = false;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		gCamera.Update(dt);

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		GpuProfiler::Begin("Total Time GPU");
		// Voxelizer Pass
		voxelizer.Generate(&scene);

		// Depth Prepass
		if(!voxelizer.enableDebugVoxel)
			depthPrePass.Render(&scene);

		// Main Pass
		if (wireframeMode) 
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		GpuProfiler::Begin("Final Pass");
		mainFBO.bind();
		mainFBO.setClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		mainFBO.setViewport(gFBOWidth, gFBOHeight);
		mainFBO.clear(voxelizer.enableDebugVoxel);

		glm::mat4 VP = gCamera.GetViewProjectionMatrix();
		if (voxelizer.enableDebugVoxel) {
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LEQUAL);
			voxelizer.Visualize(&gCamera);
		}
		else {
			if (!voxelizer.enableDebugVoxel) {
				glDepthMask(GL_FALSE);
				glDepthFunc(GL_EQUAL);
				mainProgram.bind();
				mainProgram.setMat4("uVP", &VP[0][0]);
				mainProgram.setTexture("uVolumeTexture", 0, voxelizer.voxelTexture->handle, true);
				glm::vec3 voxelDim{ (float)voxelizer.mVoxelDims, (float)voxelizer.mUnitVoxelSize, voxelizer.mDebugMipInterpolation };
				mainProgram.setVec3("uVoxelDims", &voxelDim[0]);

				glm::vec3 cameraPosition = gCamera.GetPosition();
				mainProgram.setVec3("uCameraPosition", &cameraPosition[0]);
				mainProgram.setVec3("uLightPosition", &scene.lightPosition[0]);
				for (auto& meshGroup : scene.meshGroup)
					meshGroup.Draw(&mainProgram);
				mainProgram.unbind();
				glDepthMask(GL_TRUE);
			}
		}
		DebugDraw::Render(VP);
		mainFBO.unbind();
		GpuProfiler::End();

		ImGui::Begin("MainWindow");

		MoveCamera(dt, ImGui::IsWindowFocused());
		ImVec2 dims = ImGui::GetContentRegionAvail();
		ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::GetWindowDrawList()->AddImage(
			(ImTextureID)(uint64_t)mainFBO.attachments[0],
			ImVec2(pos.x, pos.y),
			ImVec2(pos.x + dims.x, pos.y + dims.y),
			ImVec2(0, 1),
			ImVec2(1, 0));
		ImGui::End();

		ImGui::Begin("Logs");
		std::vector<std::string>& logs = logger::GetAllLogs();
		for (auto& log : logs)
			ImGui::Text("%s", log.c_str());
		ImGui::End();

		ImGui::Begin("Options");
		//bool needUpdate = AddGameObjectUI(&scene);
		//voxelizer.mRegenerateVoxelData = needUpdate;
		GpuProfiler::AddUI();
		ImGui::Checkbox("Wireframe", &wireframeMode);

		voxelizer.AddUI();
		ImGui::End();

		ImGuiService::Render(window);

		glfwSwapBuffers(window);

		GpuProfiler::End();

		GpuProfiler::Reset();

		float endTime = (float)glfwGetTime();
		dt = endTime - startTime;
		startTime = endTime;

		gWindowProps.mDx = 0.0f;
		gWindowProps.mDy = 0.0f;
	}
	mainProgram.destroy();
	mainFBO.destroy();
	DebugDraw::Shutdown();
	ImGuiService::Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}