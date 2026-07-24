#pragma once

#include <limits>
#include <numeric>
#include <glm/glm.hpp>

struct AABB {
	glm::vec3 min{ std::numeric_limits<float>::max() };
	glm::vec3 max{ -std::numeric_limits<float>::max() };

	void Expand(const glm::vec3& p)
	{
		min = glm::min(min, p);
		max = glm::max(max, p);
	}

	int LongestAxis() const
	{
		glm::vec3 d = max - min;
		if (d.x >= d.y && d.x >= d.z) return 0;
		if (d.y >= d.z) return 1;
		return 2;
	}

};

class BVHBuilder
{
public:
	struct BVHNode
	{
		AABB bounds;
		int left{-1}, right{-1}; // child node indices, -1 if leaf
		int start{0}, count{0}; // corresponds to firstTri, triCount -- identifies range into triIndex
	};

	std::vector<BVHNode> nodes;
	std::vector<int> triIndex;
	int maxDepthSeen {0};

	int Build(const Mesh& mesh)
	{
		int numTris = (int)mesh.VertexIndices.size() / 3;
		triIndex.resize(numTris);
		std::iota(triIndex.begin(), triIndex.end(), 0);
		nodes.clear();
		nodes.reserve(numTris * 2); // A binary tree over N leaves had < 2N nodes
		return BuildRecursive(mesh, 0, numTris, 0);
	}
private:
	static constexpr int MAX_LEAF_SIZE {4}; // see "improvements" below
	static constexpr int MAX_DEPTH {40};    // safety valve, see Step 3

	glm::vec3 Centroid(const Mesh& mesh, int triId) const {
		// Finds and returns center of a triangle.
		uint32_t i0 = mesh.VertexIndices[triId * 3 + 0];
		uint32_t i1 = mesh.VertexIndices[triId * 3 + 1];
		uint32_t i2 = mesh.VertexIndices[triId * 3 + 2];
		return (glm::vec3(mesh.Vertices[i0]) +
				glm::vec3(mesh.Vertices[i1]) +
				glm::vec3(mesh.Vertices[i2])
		) / 3.0f;
	}

	void ComputeBounds(
		const Mesh& mesh, int first,
		int count, AABB& out
	) const
	{
		out = AABB{};
		// Loops through each triangle face and used the corners
		// to expand the AABB's min/max bounds
		for (int i =0; i < count; ++i) {
			int triId = triIndex[first + i];
			out.Expand(glm::vec3(mesh.Vertices[mesh.VertexIndices[triId * 3 + 0]]));
			out.Expand(glm::vec3(mesh.Vertices[mesh.VertexIndices[triId * 3 + 1]]));
			out.Expand(glm::vec3(mesh.Vertices[mesh.VertexIndices[triId * 3 + 2]]));
		}
	}

	int BuildRecursive(const Mesh& mesh, int first, int numTris, int depth)
	{
		int nodeIdx = (int)nodes.size();
		nodes.emplace_back();
		maxDepthSeen = std::max(maxDepthSeen, depth);

		nodes[nodeIdx].start = first;
		nodes[nodeIdx].count = numTris;
		ComputeBounds(mesh, first, numTris, nodes[nodeIdx].bounds);

		if (numTris <= MAX_LEAF_SIZE || depth >= MAX_DEPTH) {
			return nodeIdx; // leaf
		}

		AABB bounds = nodes[nodeIdx].bounds; // copy: safe across recursion below
		int axis = bounds.LongestAxis();
		float splitPos = 0.5f * (bounds.min[axis] + bounds.max[axis]);

		int i = first, j = first + numTris - 1;
		while (i <= j) {
			float c = Centroid(mesh, triIndex[i])[axis];
			if (c < splitPos) ++i;
			else std::swap(triIndex[i], triIndex[j--]);
		}
		int leftCount = i - first;
		if (leftCount == 0 || leftCount == numTris) {
			leftCount = numTris / 2; // degenerate split fallback
		}

		int leftIdx = BuildRecursive(mesh, first, leftCount, depth + 1);
		int rightIdx = BuildRecursive(mesh, first + leftCount, numTris - leftCount, depth + 1);

		// Index back in - do NOT reuse an earlier reference to nodes[nodeIdx]
		nodes[nodeIdx].left = leftIdx;
		nodes[nodeIdx].right = rightIdx;
		return nodeIdx;
	}
};

struct GPUBVHNode {
	glm::vec4 minBounds; // .xyz used, .w unused padding
	glm::vec4 maxBounds; // .xyz used, .w unused padding
	int left, right, firstTri, triCount;
};
static_assert(sizeof(GPUBVHNode)==48, "GPUBVHNode must match std430 layout exactly");

std::vector<uint32_t> ReorderIndices(
	const Mesh& mesh,
	const std::vector<int>& triIndex
)
{
	std::vector<uint32_t> reordered;
	reordered.reserve(mesh.VertexIndices.size());
	for (int triId : triIndex) {
		reordered.push_back(mesh.VertexIndices[triId * 3 + 0]);
		reordered.push_back(mesh.VertexIndices[triId * 3 + 1]);
		reordered.push_back(mesh.VertexIndices[triId * 3 + 2]);
	}
	return reordered;
}

std::vector<GPUBVHNode> ToGPUNodes(
	const std::vector<BVHBuilder::BVHNode>& cpuNodes
)
{
	std::vector<GPUBVHNode> gpuNodes(cpuNodes.size());
	for (size_t i=0; i < cpuNodes.size(); ++i) {
		const auto& n = cpuNodes[i];
		gpuNodes[i].minBounds = glm::vec4(n.bounds.min, 0.0f);
		gpuNodes[i].maxBounds = glm::vec4(n.bounds.max, 0.0f);
		gpuNodes[i].left 	  = n.left;
		gpuNodes[i].right 	  = n.right;
		// lead nodes: n.left == -1, so firstTri/triCount matter;
		// interior nodes: triCount == 0 tells the shader to descend instead
		gpuNodes[i].firstTri = n.start;
		gpuNodes[i].triCount = (n.left == -1) ? n.count : 0;
	}
	return gpuNodes;
}