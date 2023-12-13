#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"
#include "utils.h"
#include "camera.h"
#include "mesh.h"

#include "voxel-raytracing/voxelizer.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"

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

void MoveCamera(float dt) {
	if (gWindowProps.mouseDown)
		gCamera.Rotate(gWindowProps.mDy, -gWindowProps.mDx, dt);

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


struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

static void InitializePlaneMesh(GLMesh* mesh, int width, int height) {

	std::vector<Vertex> vertices;
	float invWidth = 1.0f / float(width);
	float invHeight = 1.0f / float(height);

	float tX = -width * 0.5f;
	float tY = -height * 0.5f;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			glm::vec3 position{ float(x) + tX, 0.0f, float(y) + tY };
			glm::vec2 uv{ position.x * invWidth, position.y * invHeight };
			vertices.emplace_back(Vertex{ position, glm::vec3{0.0f, 1.0f, 0.0f}, uv });
		}
	}

	std::vector<uint32_t> indices;
	for (int i = 0; i < height - 1; ++i) {
		for (int j = 0; j < width - 1; ++j) {
			int p0 = i * width + j;
			int p1 = p0 + 1;
			int p2 = (i + 1) * width + j;
			int p3 = p2 + 1;
			indices.push_back(p2);
			indices.push_back(p1);
			indices.push_back(p0);

			indices.push_back(p2);
			indices.push_back(p3);
			indices.push_back(p1);
		}
	}
	uint32_t vertexCount = static_cast<uint32_t>(vertices.size()) * 8;
	uint32_t indexCount = static_cast<uint32_t>(indices.size());
	mesh->init((float*)vertices.data(), vertexCount, indices.data(), indexCount);
}

static uint8_t* getBufferPtr(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
	tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
	return model->buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;
}

static void LoadMesh(const std::string filename, GLMesh* mesh) {
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string err, warn;
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!ret) {
		logger::Error("Failed to load file" + filename);
		return;
	}
	if (!warn.empty()) logger::Warn(warn);
	if (!err.empty()) logger::Error(err);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	for (auto& primitive : model.meshes[0].primitives)
	{
		// Parse position
		const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes["POSITION"]];
		float* positions = (float*)getBufferPtr(&model, positionAccessor);
		uint32_t numPosition = (uint32_t)positionAccessor.count;

		// Parse normals
		float* normals = nullptr;
		auto normalAttributes = primitive.attributes.find("NORMAL");
		if (normalAttributes != primitive.attributes.end()) {
			const tinygltf::Accessor& normalAccessor = model.accessors[normalAttributes->second];
			assert(numPosition == normalAccessor.count);
			normals = (float*)getBufferPtr(&model, normalAccessor);
		}

		// Parse UV
		float* uvs = nullptr;
		auto uvAttributes = primitive.attributes.find("TEXCOORD_0");
		if (uvAttributes != primitive.attributes.end()) {
			const tinygltf::Accessor& uvAccessor = model.accessors[uvAttributes->second];
			assert(numPosition == uvAccessor.count);
			uvs = (float*)getBufferPtr(&model, uvAccessor);
		}

		for (uint32_t i = 0; i < numPosition; ++i) {
			Vertex vertex;
			vertex.position = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };

			vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			if (normals)
				vertex.normal = glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]);

			vertex.uv = glm::vec2(0.0f);
			if (uvs)
				vertex.uv = glm::vec2(uvs[i * 2 + 0], 1.0f - uvs[i * 2 + 1]);

			vertices.push_back(vertex);
		}

		const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
		uint32_t* indicesPtr = (uint32_t*)getBufferPtr(&model, indicesAccessor);
		uint32_t indexCount = (uint32_t)indicesAccessor.count;
		indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
	}

	uint32_t vertexCount = static_cast<uint32_t>(vertices.size() * 8);
	uint32_t indexCount = static_cast<uint32_t>(indices.size());
	mesh->init((float*)vertices.data(), vertexCount, indices.data(), indexCount);
}


int main() {

	if(!glfwInit()) return 1;

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
	logger::Debug("Enabled Depth Test ...");

    ImGuiService::Initialize(window);

	float startTime = (float)glfwGetTime();
	float dt = 1.0f / 60.0f;

	gCamera.SetAspect(float(gWindowProps.width) / float(gWindowProps.height));
	gCamera.SetPosition(glm::vec3(0.0f, 2.0f, -6.0f));

	std::vector<Mesh> meshes;
	GLMesh bunny;
	LoadMesh("Assets/Models/bunny.gltf", &bunny);
	glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
	meshes.emplace_back(Mesh{ bunny, transform });

	Voxelizer voxelizer;
	voxelizer.Init(512);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		MoveCamera(dt);

		gCamera.Update(dt);

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		voxelizer.Render(&gCamera, meshes);

		ImGui::Begin("MainWindow");
		ImVec2 dims = ImGui::GetContentRegionAvail();
		ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::GetWindowDrawList()->AddImage(
			(ImTextureID)(uint64_t)voxelizer.framebuffer->attachments[0],
			ImVec2(pos.x, pos.y),
			ImVec2(pos.x + dims.x, pos.y + dims.y),
			ImVec2(0, 1),
			ImVec2(1, 0));
		ImGui::End();

		ImGui::Begin("Logs");
		std::vector<std::string>& logs = logger::GetAllLogs();
		for(auto& log : logs)
			ImGui::Text("%s", log.c_str());
		ImGui::End();

		ImGui::Begin("Options");
		ImGui::Text("Voxelize Texture");
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
    ImGuiService::Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}