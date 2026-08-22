#include "model.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype> // for std::isspace
#include <algorithm>
#include <sstream>

// Helper to check if a string is entirely whitespace or empty
bool IsEmptyOrWhitespace(const std::string& str)
{
	if (str.empty()) return true;
	return std::all_of(str.begin(), str.end(), [](unsigned char ch) {
		return std::isspace(ch);
	});
}

void PrintStdVectorInt(std::vector<int>& vect, const char* name)
{
	std::cout << "\n";
	std::cout << "Printing Vector: " << name << "\n";
	std::cout << "Length of Vector: " << vect.size() << "\n";
	for (int i=0; i<vect.size();i++) {
		std::cout << vect[i] << ", ";
		if (i>0 && i%3 == 0) std::cout << "\n";
	}
	std::cout << "\n";
}

std::vector<std::string> Split(std::string& line, const std::string delimiter)
{
	std::vector<std::string> splitLine;

	// Handle empty delimiter edge case to prevent infinite loops
	if (delimiter.empty()) {
		if (!IsEmptyOrWhitespace(line)) {
			splitLine.push_back(line);
		}
		return splitLine;
	}
	size_t pos = 0;
	std::string token;
	while ((pos = line.find(delimiter)) != std::string::npos) {
		token = line.substr(0, pos);
		if (!IsEmptyOrWhitespace(token)) {
			splitLine.push_back(token);
		}
		line.erase(0, pos + delimiter.length());
	}

	// Handle the remaining trailing part of the string.
	if (!IsEmptyOrWhitespace(line)) {
		splitLine.push_back(line);
	}

	// std::cout << "Final Split Line: ";
	// for (const auto& s : splitLine){
	// 	std::cout << s << ", ";
	// }
	// std::cout << "\n";

	return splitLine;
}

// Strict split specifically for slashes that Preserves empty fields
std::vector<std::string> StrictSplit(const std::string& str, char delimiter)
{
	// std::cout << "Inside Strict Split!\n";
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(str);
	while (std::getline(tokenStream, token, delimiter)) {
		// std::cout << "Got Token: " << token << "\n";
		tokens.push_back(token);
	}
	// If the string ends with a delimiter (e.g. "1/1/"), add a final empty token
	if (!str.empty() && str.back() == delimiter) {
		tokens.push_back("");
	}
	return tokens;
}

void ConditionalExtend(std::vector<uint32_t>& targetArray, std::vector<int> subjectArray)
{
	if (subjectArray[0] != -1) {
		// M0: Push_Back
		// for (int i : subjectArray) targetArray.push_back(i);

		// M1: .Insert
		// targetArray.insert(targetArray.end(), subjectArray.begin(), subjectArray.end());

		// M2: std::copy
		// std::copy(subjectArray.begin(), subjectArray.end(), std::back_inserter(targetArray));

		// M3: std::move --- best
		std::move(subjectArray.begin(), subjectArray.end(), std::back_inserter(targetArray));

	} 
}

Mesh CreateMesh(const std::filesystem::path& path)
{
	std::ifstream file(path);

	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << path.string() << "\n";
		return {};
	}

	size_t vertexCount = 0;
	size_t texCoordCount = 0;
	size_t normalCount = 0;
	size_t triangleCount = 0;

	std::string line;
	std::vector<std::string> words;

	while (std::getline(file, line)) {
		words = Split(line, " ");
		if (words.size() == 0) {
			continue;
		}
		if (!words[0].compare("v"))
			++vertexCount;

		else if (!words[0].compare("vt")) 
			++texCoordCount;

		else if (!words[0].compare("vn"))
			++normalCount;

		else if (!words[0].compare("f")) {
			// word size of 4 (including the 'f' means there's
			// 3 vertices/corners --- triangleCount = 1.
			// some 3d models have quad faces of 4 or greater corners.
			// For these word size may be 5 or greater
			// word count of 5 -> triangleCount = 5 - 3 = 2 
			// word count of 6 -> triangleCount = 6 - 3 = 3
			// Hence, the count is appropriate
			triangleCount += words.size() - 3;
		}
	}
	file.close();

	Mesh mesh;
	mesh.Vertices.reserve(vertexCount);
	mesh.TextureUvs.reserve(texCoordCount);
	mesh.VertexNormals.reserve(normalCount);
	// * 3 because each triangle has three corners
	mesh.VertexIndices.reserve(triangleCount * 3);
	mesh.TextureIndices.reserve(triangleCount * 3);
	mesh.NormalIndices.reserve(triangleCount * 3);

	// mesh.Vertices.reserve(triangleCount * 3 * 8);
	// indices.reserve(triangleCount * 3 * 8);

	file.open(path);

	while (std::getline(file, line)) {

		words = Split(line, " ");
		if (words.size() == 0) {
			continue;
		}
		// std::cout << "Words: ";
		// for (int i=0; i<words.size();++i){
		// 	std::cout << words[i]  << ", ";
		// }
		// std::cout << "\n";

		if (!words[0].compare("v")) {
			// std::cout << "Comparing the V!\n";
			mesh.Vertices.emplace_back(
				glm::vec4(
					std::stof(words[1]), std::stof(words[2]),
					std::stof(words[3]), 1.0f
			));
		}
		else if (!words[0].compare("vt")) {
			// std::cout << "Comparing the VT!\n";
			mesh.TextureUvs.emplace_back(glm::vec2(std::stof(
				words[1]), std::stof(words[2])
			));
		}
		else if (!words[0].compare("vn")) {
			// std::cout << "Comparing the VN!\n";
			mesh.VertexNormals.emplace_back(glm::vec3(
				std::stof(words[1]), std::stof(words[2]), std::stof(words[3])
			));
		}
		else if (!words[0].compare("f")) {
			// std::cout << "Comparing the Face!\n";
			// word contains: "f", "corner1", "corner2", "corner3", ... "cornerN"
			// normally, a face has 3 corners
			// but there can be greater than 3 corners
			// Each word, without considering, 'f' has values called a Corner.
			// a Corner consists of 3 parts separated by '/': vertex_idx, texture_idx, normal_idx.
			int cornerCount = words.size() - 1;	// -1 to skip 'f'

			// Do `* 3` since each corner has 3 parts and the face_list_flat
			// stores each corner's data in a flat list,
			std::vector<int> face_list_flat;
			face_list_flat.reserve(cornerCount * 3);
			// std::cout << "Face Corner Count: " << cornerCount << "!\n";

			for (int idx=1; idx <= cornerCount; idx++) {
				std::vector<std::string> parts = StrictSplit(words[idx], '/');
				// std::cout << "Parts Size: " << parts.size() << "\n";
				// was iidx < parts.size()
				// but changed to 3 because most models should have 3 parts here
				// but some can have just 1 i.e., no /-separated values
				// but that's what the default -1 is for...
				for (int iidx=0; iidx < 3; iidx++) {
					// std::cout << parts[iidx] << ", "; 
					int val = -1;
					// Some face parts -- especially the texture part -- have no values.
					// So they should be -1 which will be skipped when adding to the final
					// indices array.
					if (iidx == 0 || iidx % parts.size() != 0) {
						if (parts[iidx].length() > 0)
							val = std::stol(parts[iidx]) - 1; // - 1 since indices should start from 0
					}

					face_list_flat.push_back(val);
				}
				// std::cout << "\n"; 
			}

			// PrintStdVectorInt(face_list_flat, "Face Flat List");

			std::vector<int> flat_vertex_indices;
			flat_vertex_indices.reserve(face_list_flat.size()/3);
			std::vector<int> flat_texture_indices;
			flat_texture_indices.reserve(face_list_flat.size()/3);
			std::vector<int> flat_normal_indices;
			flat_normal_indices.reserve(face_list_flat.size()/3);

			for (int idx=0; idx < face_list_flat.size(); idx+=3) {
				flat_vertex_indices.push_back(face_list_flat[idx]);
				flat_texture_indices.push_back(face_list_flat[idx+1]);
				flat_normal_indices.push_back(face_list_flat[idx+2]);
			}
			// PrintStdVectorInt(flat_vertex_indices, "vertex indices");
			// PrintStdVectorInt(flat_texture_indices, "texture indices");
			// PrintStdVectorInt(flat_normal_indices, "normal indices");
			if (cornerCount < 3) {
				std::cerr << "Skipping malformed face with  " << cornerCount << "corners\n";
				continue;
			}
			if (cornerCount == 3) {
				ConditionalExtend(mesh.VertexIndices, flat_vertex_indices);
				ConditionalExtend(mesh.TextureIndices, flat_texture_indices);
				ConditionalExtend(mesh.NormalIndices, flat_normal_indices);
			}
			else if (cornerCount == 4) {
				unsigned int index_list[] {0, 1, 2, 0, 2, 3};

				for (int idx=0; idx<sizeof(index_list)/sizeof(index_list[0]); idx+=3) {
					ConditionalExtend(mesh.VertexIndices, {
						flat_vertex_indices[index_list[idx]],
						flat_vertex_indices[index_list[idx + 1]],
						flat_vertex_indices[index_list[idx + 2]]
					});
					ConditionalExtend(mesh.TextureIndices, {
						flat_texture_indices[index_list[idx]],
						flat_texture_indices[index_list[idx + 1]],
						flat_texture_indices[index_list[idx + 2]]
					});
					ConditionalExtend(mesh.NormalIndices,{
						flat_normal_indices[index_list[idx]],
						flat_normal_indices[index_list[idx + 1]],
						flat_normal_indices[index_list[idx + 2]]
					});
				}
			}
			else if (cornerCount > 4) { 
				// Handles models with faces of cornerse > 4
				// Handles them as N-gons (Fans) faces
				// Adds a given number of triangles based on the number of corners
				// present on the face
				int vert_idx0 = flat_vertex_indices[0];
				int tex_idx0  = flat_texture_indices[0];
				int norm_idx0 = flat_normal_indices[0];
				for (int idx=1; idx < flat_vertex_indices.size() - 1; idx++) {
					ConditionalExtend(mesh.VertexIndices, {
						vert_idx0,
						flat_vertex_indices[idx],
						flat_vertex_indices[idx + 1]
					});
					ConditionalExtend(mesh.TextureIndices, {
						tex_idx0,
						flat_texture_indices[idx],
						flat_texture_indices[idx + 1]
					});
					ConditionalExtend(mesh.NormalIndices, {
						norm_idx0,
						flat_normal_indices[idx],
						flat_normal_indices[idx + 1]
					});
				}
			}
		}
	}
	file.close();

	std::cout << "No. of Polygons: " << mesh.VertexIndices.size() / 3 << "\n";
	std::cout << "No. of Vertices: " << mesh.Vertices.size() << "\n";

	// for (int idx=0; idx<mesh.Vertices.size();++idx) {
	// 	std::cout << "V [" << idx << "] : "
	// 		<< mesh.Vertices[idx].x << ", " << mesh.Vertices[idx].y << ", " << mesh.Vertices[idx].z << "\n";
	// }
	// std::cout << "No. of TextureUvs: " << mesh.TextureUvs / 3 << "\n";
	// std::cout << "No. of Normals: " << mesh.Normals / 3 << "\n";


	return mesh;
}