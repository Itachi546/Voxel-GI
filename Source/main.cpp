#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"
#include "utils.h"
#include "camera.h"
#include "mesh.h"
#include "debug-draw.h"

#include "voxel-raytracing/voxelizer.h"
#include <iostream>

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

	float walkSpeed = dt * 5.0f;
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

	float startTime = (float)glfwGetTime();
	float dt = 1.0f / 60.0f;

	gCamera.SetAspect(float(gWindowProps.width) / float(gWindowProps.height));
	gCamera.SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
	gCamera.SetRotation(glm::vec3(0.0f, glm::pi<float>() * 0.5f, 0.0f));

	std::vector<MeshGroup> scene;
	scene.push_back(MeshGroup{});
	LoadMesh("C:/Users/Dell/OneDrive/Documents/3D-Assets/Models/Sponza/Sponza.gltf", &scene.back());
	const AABB& debugAABB = scene[0].aabbs[0];

	Voxelizer voxelizer;
	voxelizer.Init(512);

	TextureCreateInfo colorAttachment = { gFBOWidth, gFBOHeight };
	TextureCreateInfo depthAttachment;
	InitializeDepthTexture(&depthAttachment, gFBOWidth, gFBOHeight);

	GLFramebuffer mainFBO;
	mainFBO.init({ Attachment{ 0, &colorAttachment } }, &depthAttachment);

	GLProgram mainProgram;
	mainProgram.init(GLShader("Assets/Shaders/mesh.vert"), GLShader("Assets/Shaders/mesh.frag"));

	bool wireframeMode = false;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		gCamera.Update(dt);

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		voxelizer.Generate(&gCamera, scene);

		if (wireframeMode) 
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glm::mat4 VP = gCamera.GetViewProjectionMatrix();
		mainFBO.bind();
		mainFBO.setClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		mainFBO.setViewport(gFBOWidth, gFBOHeight);
		mainFBO.clear(true);

		voxelizer.Visualize(&gCamera);

		if (!voxelizer.enableDebugVoxel) {
			mainProgram.bind();
			mainProgram.setMat4("uVP", &VP[0][0]);
			for (auto& meshGroup : scene)
				meshGroup.Draw(&mainProgram);
			mainProgram.unbind();
		}

		DebugDraw::Render(VP);
		mainFBO.unbind();
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
		ImGui::Checkbox("Wireframe", &wireframeMode);

		voxelizer.AddUI();
		ImGui::End();

		ImGuiService::Render(window);

		glfwSwapBuffers(window);

		float endTime = (float)glfwGetTime();
		dt = endTime - startTime;
		startTime = endTime;

		gWindowProps.mDx = 0.0f;
		gWindowProps.mDy = 0.0f;
	}
	DebugDraw::Shutdown();
	ImGuiService::Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}