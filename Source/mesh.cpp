#include "mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include "logger.h"

void InitializePlaneMesh(GLMesh* mesh, int width, int height) {

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

void InitializeCubeMesh(GLMesh* mesh)
{
	std::vector<Vertex> vertices = {
					Vertex{glm::vec3(-1.0f, +1.0f, +1.0f), glm::vec3(+0.0f, +1.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, +1.0f, +1.0f), glm::vec3(+0.0f, +1.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, +1.0f, -1.0f), glm::vec3(+0.0f, +1.0f, +0.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(-1.0f, +1.0f, -1.0f), glm::vec3(+0.0f, +1.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(-1.0f, +1.0f, -1.0f), glm::vec3(+0.0f, +0.0f, -1.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, +1.0f, -1.0f), glm::vec3(+0.0f, +0.0f, -1.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(+1.0f, -1.0f, -1.0f), glm::vec3(+0.0f, +0.0f, -1.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(+0.0f, +0.0f, -1.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, +1.0f, -1.0f), glm::vec3(+1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(+1.0f, +1.0f, +1.0f), glm::vec3(+1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, -1.0f, +1.0f), glm::vec3(+1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, -1.0f, -1.0f), glm::vec3(+1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(-1.0f, +1.0f, +1.0f), glm::vec3(-1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(-1.0f, +1.0f, -1.0f), glm::vec3(-1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(-1.0f, -1.0f, +1.0f), glm::vec3(-1.0f, +0.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, +1.0f, +1.0f), glm::vec3(+0.0f, +0.0f, +1.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(-1.0f, +1.0f, +1.0f), glm::vec3(+0.0f, +0.0f, +1.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(-1.0f, -1.0f, +1.0f), glm::vec3(+0.0f, +0.0f, +1.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, -1.0f, +1.0f), glm::vec3(+0.0f, +0.0f, +1.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, -1.0f, -1.0f), glm::vec3(+0.0f, -1.0f, +0.0f), glm::vec2(0.0f)},

					Vertex{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(+0.0f, -1.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(-1.0f, -1.0f, +1.0f), glm::vec3(+0.0f, -1.0f, +0.0f), glm::vec2(0.0f)},
					Vertex{glm::vec3(+1.0f, -1.0f, +1.0f), glm::vec3(+0.0f, -1.0f, +0.0f), glm::vec2(0.0f)},
	};

	std::vector<unsigned int> indices = {
		0,   1,  2,  0,  2,  3, // Top
		4,   5,  6,  4,  6,  7, // Front
		8,   9, 10,  8, 10, 11, // Right
		12, 13, 14, 12, 14, 15, // Left
		16, 17, 18, 16, 18, 19, // Back
		20, 22, 21, 20, 23, 22, // Bottom
	};

	uint32_t vertexCount = static_cast<uint32_t>(vertices.size()) * 8;
	uint32_t indexCount = static_cast<uint32_t>(indices.size());
	mesh->init((float*)vertices.data(), vertexCount, indices.data(), indexCount);
}

static uint8_t* getBufferPtr(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
	tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
	return model->buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;
}

static void parseMaterial(tinygltf::Model* model, Material* component, uint32_t matIndex) {
	if (matIndex == -1)
		return;

	tinygltf::Material& material = model->materials[matIndex];
	//component->alphaCutoff = (float)material.alphaCutoff;
	/*
	if (material.alphaMode == "BLEND")
		component->alphaMode = ALPHAMODE_BLEND;
	else if (material.alphaMode == "MASK")
		component->alphaMode = ALPHAMODE_MASK;
		*/

	// Parse Material value
	tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
	std::vector<double>& baseColor = pbr.baseColorFactor;
	component->albedo = glm::vec4((float)baseColor[0], (float)baseColor[1], (float)baseColor[2], (float)baseColor[3]);
	component->transparency = (float)baseColor[3];
	component->metallic = (float)pbr.metallicFactor;
	component->roughness = (float)pbr.roughnessFactor;
	std::vector<double>& emissiveColor = material.emissiveFactor;
	component->emissive = glm::vec4((float)emissiveColor[0], (float)emissiveColor[1], (float)emissiveColor[2], 1.0f);
	/*
	// Parse Material texture
	auto loadTexture = [&](uint32_t index) {
		tinygltf::Texture& texture = model->textures[index];
		tinygltf::Image& image = model->images[texture.source];
		const std::string& name = image.uri.length() == 0 ? image.name : image.uri;
		return TextureCache::LoadTexture(name, image.width, image.height, image.image.data(), image.component, true);
		};


	if (pbr.baseColorTexture.index >= 0)
		component->albedoMap = loadTexture(pbr.baseColorTexture.index);

	if (pbr.metallicRoughnessTexture.index >= 0)
		component->metallicMap = component->roughnessMap = loadTexture(pbr.metallicRoughnessTexture.index);

	if (material.normalTexture.index >= 0)
		component->normalMap = loadTexture(material.normalTexture.index);

	if (material.occlusionTexture.index >= 0)
		component->ambientOcclusionMap = loadTexture(material.occlusionTexture.index);

	if (material.emissiveTexture.index >= 0)
		component->emissiveMap = loadTexture(material.emissiveTexture.index);
	 */
}

static void parseMesh(tinygltf::Model* model, tinygltf::Mesh& mesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, MeshGroup* meshGroup, const glm::mat4& transform) {
	for (auto& primitive : mesh.primitives)
	{
		// Parse position
		const tinygltf::Accessor& positionAccessor = model->accessors[primitive.attributes["POSITION"]];
		float* positions = (float*)getBufferPtr(model, positionAccessor);
		uint32_t numPosition = (uint32_t)positionAccessor.count;

		// Parse normals
		float* normals = nullptr;
		auto normalAttributes = primitive.attributes.find("NORMAL");
		if (normalAttributes != primitive.attributes.end()) {
			const tinygltf::Accessor& normalAccessor = model->accessors[normalAttributes->second];
			assert(numPosition == normalAccessor.count);
			normals = (float*)getBufferPtr(model, normalAccessor);
		}

		// Parse tangents
		float* tangents = nullptr;
		auto tangentAttributes = primitive.attributes.find("TANGENT");
		if (tangentAttributes != primitive.attributes.end()) {
			const tinygltf::Accessor& tangentAccessor = model->accessors[normalAttributes->second];
			assert(numPosition == tangentAccessor.count);
			tangents = (float*)getBufferPtr(model, tangentAccessor);
		}

		// Parse UV
		float* uvs = nullptr;
		auto uvAttributes = primitive.attributes.find("TEXCOORD_0");
		if (uvAttributes != primitive.attributes.end()) {
			const tinygltf::Accessor& uvAccessor = model->accessors[uvAttributes->second];
			assert(numPosition == uvAccessor.count);
			uvs = (float*)getBufferPtr(model, uvAccessor);
		}


		uint32_t vertexOffset = (uint32_t)(vertices.size() * sizeof(Vertex));
		uint32_t indexOffset = (uint32_t)(indices.size() * sizeof(uint32_t));

		for (uint32_t i = 0; i < numPosition; ++i) {
			Vertex vertex;
			vertex.position = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };

			if (normals)
				vertex.normal = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
			else
				vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);

			if (uvs)
				vertex.uv = { uvs[i * 2 + 0],  1.0f - uvs[i * 2 + 1] };
			else
				vertex.uv = { 0.0f, 0.0f };

			vertices.push_back(std::move(vertex));
		}

		const tinygltf::Accessor& indicesAccessor = model->accessors[primitive.indices];
		uint32_t indexCount = (uint32_t)indicesAccessor.count;
		if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			uint32_t* indicesPtr = (uint32_t*)getBufferPtr(model, indicesAccessor);
			indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
		}
		else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			uint16_t* indicesPtr = (uint16_t*)getBufferPtr(model, indicesAccessor);
			indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
		}
		else
			logger::Error("Undefined indices componentType: " + std::to_string(indicesAccessor.componentType));

		glm::vec3 minExtent = glm::vec3(positionAccessor.minValues[0], positionAccessor.minValues[1], positionAccessor.minValues[2]);
		glm::vec3 maxExtent = glm::vec3(positionAccessor.maxValues[0], positionAccessor.maxValues[1], positionAccessor.maxValues[2]);

		meshGroup->transforms.push_back(transform);
		meshGroup->aabbs.push_back(AABB{ minExtent, maxExtent });
		DrawElementsIndirectCommand drawCommand = {};
		drawCommand.count_ = indexCount;
		drawCommand.instanceCount_ = 1;
		drawCommand.firstIndex_ = indexOffset / sizeof(uint32_t);
		drawCommand.baseVertex_ = vertexOffset / sizeof(Vertex);
		drawCommand.baseInstance_ = 0;
		meshGroup->drawCommands.push_back(std::move(drawCommand));

		Material material = {};
		std::string materialName = model->materials[primitive.material].name;
		meshGroup->names.push_back(materialName);
		parseMaterial(model, &material, primitive.material);
		meshGroup->materials.push_back(std::move(material));
	}
}

void parseNodeHierarchy(tinygltf::Model* model, int nodeIndex, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, MeshGroup* meshGroup) {
	tinygltf::Node& node = model->nodes[nodeIndex];

	// Create entity and write the transforms
	glm::mat4 translation = glm::mat4(1.0f);
	glm::mat4 rotation = glm::mat4(1.0f);
	glm::mat4 scale = glm::mat4(1.0f);
	if (node.translation.size() > 0)
		translation = glm::translate(glm::mat4(1.0f), glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
	if (node.rotation.size() > 0)
		rotation = glm::mat4_cast(glm::fquat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]));
	if (node.scale.size() > 0)
		scale = glm::scale(glm::mat4(1.0f), glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));

	glm::mat4 transform = translation * rotation * scale;
	// Update MeshData
	if (node.mesh >= 0) {
		tinygltf::Mesh& mesh = model->meshes[node.mesh];
		parseMesh(model, mesh, vertices, indices, meshGroup, transform);
	}

	for (auto child : node.children)
		parseNodeHierarchy(model, child, vertices, indices, meshGroup);
}

void parseScene(tinygltf::Model* model,
	tinygltf::Scene* scene,
	std::vector<Vertex>& vertices,
	std::vector<unsigned int>& indices,
	MeshGroup* meshGroup)
{
	for (auto node : scene->nodes)
		parseNodeHierarchy(model, node, vertices, indices, meshGroup);
}

void LoadMesh(const std::string& filename, MeshGroup* meshGroup) {
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string err, warn;
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!ret) {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
		if (!ret) {
			if (!warn.empty()) logger::Warn(warn);
			if (!err.empty()) logger::Error(err);
			logger::Error("Failed to load file: " + filename);
			return;
		}
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (auto& scene : model.scenes)
		parseScene(&model, &scene, vertices, indices, meshGroup);

	// Upload data
	uint32_t vertexSize = (uint32_t)(sizeof(Vertex) * vertices.size());
	meshGroup->vertexBuffer.init(vertices.data(), vertexSize, 0);

	uint32_t indexSize = (uint32_t)(indices.size() * sizeof(uint32_t));
	meshGroup->indexBuffer.init(indices.data(), indexSize, 0);

	uint32_t transformSize = (uint32_t)(meshGroup->transforms.size() * sizeof(glm::mat4));
	meshGroup->transformBuffer.init(meshGroup->transforms.data(), transformSize, GL_DYNAMIC_STORAGE_BIT);

	uint32_t drawCommandSize = (uint32_t)(meshGroup->drawCommands.size() * sizeof(DrawElementsIndirectCommand));
	meshGroup->drawIndirectBuffer.init(meshGroup->drawCommands.data(), drawCommandSize, GL_DYNAMIC_STORAGE_BIT);

	uint32_t materialSize = (uint32_t)(meshGroup->materials.size() * sizeof(Material));
	meshGroup->materialBuffer.init(meshGroup->materials.data(), materialSize, GL_DYNAMIC_STORAGE_BIT);

	glGenVertexArrays(1, &meshGroup->vao);
	glBindVertexArray(meshGroup->vao);

	glBindBuffer(GL_ARRAY_BUFFER, meshGroup->vertexBuffer.handle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshGroup->indexBuffer.handle);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, meshGroup->drawIndirectBuffer.handle);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));
	glBindVertexArray(0);
}

void MeshGroup::updateTransforms()
{
	uint32_t dataSize = (uint32_t)(transforms.size() * sizeof(glm::mat4));
	glNamedBufferSubData(transformBuffer.handle, 0, dataSize, transforms.data());
}

void MeshGroup::updateMaterials()
{
	uint32_t dataSize = (uint32_t)materials.size() * sizeof(Material);
	glNamedBufferSubData(materialBuffer.handle, 0, dataSize, materials.data());
	/*
	uint32_t dataSize = (uint32_t)materials.size() * sizeof(Material);
	Material* material = (Material*)glMapNamedBuffer(materialBuffer.handle, GL_WRITE_ONLY);
	std::memcpy(material, materials.data(), dataSize);
	glUnmapNamedBuffer(materialBuffer.handle);
	*/

}

void MeshGroup::Draw(GLProgram* program)
{
	glBindVertexArray(vao);
	program->setBuffer(1, transformBuffer.handle);
	program->setBuffer(2, materialBuffer.handle);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, (uint32_t)drawCommands.size(), 0);
	glBindVertexArray(0);
}
