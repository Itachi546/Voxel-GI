#include "utils.h"


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm>

#include "gl-utils.h"
#include "logger.h"

namespace Utils {
    bool RayBoxIntersection(const Ray& ray, const glm::vec3& min, const glm::vec3& max, glm::vec2& t)
    {
        glm::vec3 invRd = 1.0f / ray.direction;
        glm::vec3 t0 = (min - ray.origin) * invRd;
        glm::vec3 t1 = (max - ray.origin) * invRd;

        float tMin = t0.x;
        float tMax = t1.x;
        if (tMin > tMax) std::swap(tMin, tMax);

        if (t0.y > t1.y) std::swap(t0.y, t1.y);

        if (t0.y > tMax || tMin > t1.y) return false;
        tMin = std::max(t0.y, tMin);
        tMax = std::min(t1.y, tMax);

        if (t0.z > t1.z) std::swap(t0.z, t1.z);

        if (t0.z > tMax || tMin > t1.z) return false;
        tMin = std::max(t0.z, tMin);
        tMax = std::min(t1.z, tMax);

        t = { tMin, tMax };
        return true;
    }

    glm::vec3 GetRayDir(const glm::mat4& P, const glm::mat4& V, const glm::vec2& ndcCoord)
    {
        glm::vec4 ndc{ ndcCoord, -1.0f, 1.0f };
        glm::vec4 viewCoord = glm::inverse(P) * ndc;
        viewCoord.z = -1.0f;
        viewCoord.w = 0.0f;

        glm::vec4 worldCoord = glm::inverse(V) * viewCoord;
        return glm::normalize(glm::vec3(worldCoord));
    }

    unsigned char* LoadImage(const char* filename, int* width, int* height, int* nChannel)
    {
        unsigned char* image = stbi_load(filename, width, height, nChannel, 0);
        if (image == nullptr)
            logger::Error("Failed to load image: " + std::string(filename));

        logger::Debug("Image Loaded from file: " + std::string(filename));
        return image;
    }

    float* LoadImageFloat(const char* filename, int* width, int* height, int* nChannel)
    {
        float* image = stbi_loadf(filename, width, height, nChannel, 0);
        if (image == nullptr)
            logger::Error("Failed to load image: " + std::string(filename));

        logger::Debug("Image Loaded from file: " + std::string(filename));
        return image;
    }

    void FreeImage(void* buffer)
    {
        stbi_image_free(buffer);
    }

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

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

	static uint8_t* getBufferPtr(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		return model->buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;
	}

	void LoadMesh(const std::string filename, GLMesh* mesh) {
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
			uint32_t indexCount = (uint32_t)indicesAccessor.count;
			if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
				uint32_t* indicesPtr = (uint32_t*)getBufferPtr(&model, indicesAccessor);
				indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
			}
			else if(indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				uint16_t* indicesPtr = (uint16_t*)getBufferPtr(&model, indicesAccessor);
				indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
			}
		}

		uint32_t vertexCount = static_cast<uint32_t>(vertices.size() * 8);
		uint32_t indexCount = static_cast<uint32_t>(indices.size());
		mesh->init((float*)vertices.data(), vertexCount, indices.data(), indexCount);
	}



}