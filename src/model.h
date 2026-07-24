#pragma once

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float g_ModelSpeed {1.5f};

struct Model
{
	glm::vec3 Position = glm::vec3{0.0f, 0.0f, 0.0f};
	glm::vec3 Orientation = glm::vec3{0.0f, 0.0f, 0.0f};
	glm::vec3 Scale = glm::vec3{1.0f, 1.0f, 1.0f};
	glm::mat4 Matrix = glm::mat4{}; // Zero-initialized
	// glm::mat4 InverseMatrix = glm::mat4{};

	Model(
		glm::vec3 pos={0.0f, 0.0f, 0.0f}, 
		glm::vec3 orientation={0.0f, 0.0f, 0.0f}, 
		glm::vec3 scale={1.0f, 1.0f, 1.0f}
	)
	: Position(pos), Orientation(orientation), Scale(scale)
	{
		UpdateMatrix();
	}

	void UpdateMatrix()
	{
		Matrix = glm::mat4{1.0f}; // Must start from identity matrix
		Matrix = glm::translate(Matrix, Position);
		Matrix = glm::rotate(Matrix, Orientation.x, glm::vec3(0.0f, 0.0f, 1.0f));
		Matrix = glm::rotate(Matrix, Orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		Matrix = glm::rotate(Matrix, Orientation.z, glm::vec3(1.0f, 0.0f, 0.0f));
		Matrix = glm::scale(Matrix, Scale);
		// InverseMatrix = glm::inverse(Matrix)
	}

	void Move(float dt, glm::vec3 dPos)
	{
		Position += dPos * g_ModelSpeed * dt;
		UpdateMatrix();
	}

};

struct Mesh
{
	std::vector<glm::vec4> Vertices;
	std::vector<glm::vec2> TextureUvs;
	std::vector<glm::vec3> VertexNormals;
	std::vector<uint32_t> VertexIndices;
	std::vector<uint32_t> TextureIndices;
	std::vector<uint32_t> NormalIndices;
	Model Model;
};

Mesh CreateMesh(const std::filesystem::path& path);