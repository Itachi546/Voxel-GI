#include "imgui-service.h"

namespace ImGuiService {

	struct RenderData {
		uint32_t textureId;
		float layer;
		int channel;
		uint32_t vertexOffset;
	};

	const int MAX_RECT_COUNT = 100;
	struct Vertex {
		ImVec2 position;
		ImVec2 uv;
		ImColor color;
	};

	struct GuiState {
		ImGuiID dockSpaceId = 0;
		bool firstFrame = true;
		GLProgram draw3DTextureProgram;
		glm::mat4 projectionMatrix;
		std::vector<RenderData> textureData;

		GLBuffer buffer;
		Vertex* bufferPtr;
		uint32_t bufferVertexOffset;
	};

	static GuiState gState = {};

	void Initialize3DTextureProgram() {
		const char* vsCode = "#version 410 core\n"
			"layout (location = 0) in vec2 Position;\n"
			"layout (location = 1) in vec2 UV;\n"
			"layout (location = 2) in vec4 Color;\n"
			"uniform mat4 ProjMtx;\n"
			"out vec2 Frag_UV;\n"
			"out vec4 Frag_Color;\n"
			"void main()\n"
			"{\n"
			"    Frag_UV = UV;\n"
			"    Frag_Color = Color;\n"
			"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
			"}\n";
		const char* fsCode = "#version 410 core\n"
			"in vec2 Frag_UV;\n"
			"in vec4 Frag_Color;\n"
			"uniform sampler3D Texture;\n"
			"uniform float Layer;\n"
			"uniform int Channel;\n"
			"layout (location = 0) out vec4 Out_Color;\n"
			"void main()\n"
			"{\n"
			"vec4 color = texture(Texture, vec3(Frag_UV, Layer));\n"
			"if(Channel == 4) Out_Color = Frag_Color * color;\n"
			"else Out_Color = Frag_Color * vec4(vec3(color[Channel]), 1.0f);\n"
			"}\n";

		GLShader vs(GL_VERTEX_SHADER, vsCode);
		GLShader fs(GL_FRAGMENT_SHADER, fsCode);

		gState.draw3DTextureProgram.init(vs, fs);
	}


	static void AddRect(const ImVec2& min, const ImVec2& max, const ImColor& color = { 1.0f, 1.0f, 1.0f, 1.0f }) {
		Vertex* vertex = gState.bufferPtr + gState.bufferVertexOffset;

		vertex->position = min;
		vertex->color = color;
		vertex->uv = { 0.0f, 1.0f };
		vertex++;

		vertex->position = max;
		vertex->color = color;
		vertex->uv = ImVec2{ 1.0f, 0.0f };
		vertex++;

		vertex->position = { min.x, max.y };
		vertex->color = color;
		vertex->uv = { 0.0f, 0.0f };
		vertex++;

		vertex->position = { max.x, min.y };
		vertex->color = color;
		vertex->uv = { 1.0f, 1.0f };
		vertex++;

		vertex->position = max;
		vertex->color = color;
		vertex->uv = ImVec2{ 1.0f, 0.0f };
		vertex++;

		vertex->position = min;
		vertex->color = color;
		vertex->uv = { 0.0f, 1.0f };
		vertex++;

		gState.bufferVertexOffset += 6;
	}

	// https://gist.github.com/AidanSun05/953f1048ffe5699800d2c92b88c36d9f
	void BeginDraw3DTex(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
		GLProgram& program = gState.draw3DTextureProgram;
		program.bind();

		RenderData* renderData = reinterpret_cast<RenderData*>(cmd->UserCallbackData);
		program.setFloat("Layer", renderData->layer);
		program.setInt("Channel", renderData->channel);
		program.setMat4("ProjMtx", &gState.projectionMatrix[0][0]);
		program.setInt("Texture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, renderData->textureId);

		glBindBuffer(GL_ARRAY_BUFFER, gState.buffer.handle);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
		glDrawArrays(GL_TRIANGLES, renderData->vertexOffset, 6);

		program.unbind();
	}


	void Image3D(ImTextureID user_texture_id, const ImVec2& size, float layer, int channel, const ImVec4& border_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		if (window->SkipItems)
			return;

		ImRect bb(window->DC.CursorPos, ImVec2{ window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y });
		if (border_col.w > 0.0f) {
			bb.Max.x += 2;
			bb.Max.y += 2;
		}

		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, 0))
			return;

		RenderData data = { (uint32_t)(uint64_t)user_texture_id, layer, channel, gState.bufferVertexOffset};
		gState.textureData.push_back(data);

		ImVec2 bbMin = bb.Min;
		ImVec2 bbMax = bb.Max;
		if (border_col.w > 0.0f)
		{
			window->DrawList->AddRect(bbMin, bbMax, ImGui::GetColorU32(border_col), 0.0f);
			bbMin.x += 1.0f; bbMin.y += 1.0f;
			bbMax.x -= 1.0f; bbMax.y -= 1.0f;
		}

		window->DrawList->AddCallback(BeginDraw3DTex, &gState.textureData.back());
		AddRect(bbMin, bbMax);
		window->DrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
	}

	void SetEditorLayoutPreset() {
		ImVec2 workCenter = ImGui::GetMainViewport()->GetCenter();
		ImGui::DockBuilderRemoveNode(gState.dockSpaceId);
		ImGui::DockBuilderAddNode(gState.dockSpaceId);

		ImGuiID mainWindowDoc = ImGui::DockBuilderSplitNode(gState.dockSpaceId, ImGuiDir_Left, 0.85f, nullptr, &gState.dockSpaceId);
		ImGuiID optionWindowDoc = ImGui::DockBuilderSplitNode(gState.dockSpaceId, ImGuiDir_Right, 0.1f, nullptr, &gState.dockSpaceId);
		ImGuiID logDoc = ImGui::DockBuilderSplitNode(mainWindowDoc, ImGuiDir_Down, 0.2f, nullptr, &mainWindowDoc);

		ImGui::DockBuilderDockWindow(gCustomWindowName[int(ImGuiCustomWindow::MainWindow)], mainWindowDoc);
		ImGui::DockBuilderDockWindow(gCustomWindowName[int(ImGuiCustomWindow::Options)], optionWindowDoc);
		ImGui::DockBuilderDockWindow(gCustomWindowName[int(ImGuiCustomWindow::Logs)], logDoc);
		ImGui::DockBuilderFinish(gState.dockSpaceId);
	}

	void Initialize(GLFWwindow* window) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init();

		// Initialize 3D Texture related resources
		Initialize3DTextureProgram();

		uint32_t dataSize = sizeof(Vertex) * 6 * MAX_RECT_COUNT;
		gState.buffer.init(nullptr, dataSize, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT);
	}

	void NewFrame() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		ImVec2 size = io.DisplaySize;
		ImVec2 pos = ImGui::GetMainViewport()->Pos;
		gState.projectionMatrix = glm::ortho(pos.x, pos.x + size.x, pos.y + size.y, pos.y);
		gState.bufferPtr = reinterpret_cast<Vertex*>(glMapNamedBuffer(gState.buffer.handle, GL_WRITE_ONLY));
	}

	void RenderDockSpace() {
		gState.dockSpaceId = ImGui::DockSpaceOverViewport();
		if (gState.firstFrame) {
			SetEditorLayoutPreset();
			gState.firstFrame = false;
		}
	}

	void Render(GLFWwindow* window) {
		glUnmapNamedBuffer(gState.buffer.handle);
		gState.bufferPtr = 0;
		gState.bufferVertexOffset = 0;

		ImGui::Render();

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
		gState.textureData.clear();
	}

	void Shutdown() {
		gState.draw3DTextureProgram.destroy();
		gState.buffer.destroy();
		gState.bufferVertexOffset = 0;
		gState.bufferPtr = nullptr;

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}