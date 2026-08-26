#version 460 core


layout(rgba32f, binding = 0) uniform writeonly image2D outputImage;

layout(local_size_x = 16, local_size_y = 16) in;

uniform vec2 screenResolution;
//uniform int maxStack;

uniform vec3 camPos, camDir, camUp, camRight;
uniform float camFovTangent;

uniform mat4 invModelMatrix;
uniform mat3 normalModelMatrix;


// SSBOs
// Tri/Quad Indices: May use 3 or 4
layout(std430, binding=0) buffer Indices { int indices[]; };
// Tri/Quad data structures (std430 padded to vec4 algnment)
layout(std430, binding=1) buffer Vertices { vec4 vertices[]; };


// BVH data (using SSBOs)
// below was vec3 and hence BVH version could not render properly.
// Now correct to be vec4
struct BVHNode {
	vec4 minBounds, maxBounds; // AABB of this node
	int left;  // index of left child (or -1 for leaf)
	int right; // index of right child
	int firstTri, triCount; // if leaf: range of triangle indices
};
layout(std430, binding=2) buffer BVH { BVHNode nodes[]; };

const int MAX_STACK=128;

// Fast AABB Ray Intersection Test
// This is the Slab Intersection Method
bool intersectAABB(
	vec3 origin, vec3 invDir,
	vec3 boxMin, vec3 boxMax,
	out float tNear, out float tFar
) {
	vec3 t1 = (boxMin - origin) * invDir;
	vec3 t2 = (boxMax - origin) * invDir;

	vec3 tmin = min(t1, t2);
	vec3 tmax = max(t1, t2);

	tNear = max(max(tmin.x, tmin.y), tmin.z);
	tFar = min(min(tmax.x, tmax.y), tmax.z);
	
	// tFar > 0.0 removes nodes completely behind the ray.
	return (tNear <= tFar) && (tFar > 0.0);
}


// Möller-Trumbore Ray-Triangle Intersection
bool intersectRayTriangle(
	vec3 origin, vec3 dir,
	vec3 v0, vec3 v1, vec3 v2,
	float maxT, out float t,
	out vec3 bary
) {
	const float EPS = 1e-6;
	vec3 e1 = v1 - v0;
	vec3 e2 = v2 - v0;
	vec3 p = cross(dir, e2);
	float det = dot(e1, p);
	if (abs(det) < EPS) return false; // parallel or degenerate
	float invDet = 1.0 / det;
	vec3 tvec = origin - v0;
	float u = dot(tvec, p) * invDet;
	if (u < 0.0 || u > 1.0) return false;
	vec3 q = cross(tvec, e1);
	float v = dot(dir, q) * invDet;
	if (v < 0.0 || u + v > 1.0) return false;
	t = dot(e2, q) * invDet;
	if (t <= EPS) return false;
	if (t > maxT) return false;
	bary = vec3(u, v, 1.0 - u - v);
	return true;
}


void pushNode(
	inout int stack[MAX_STACK],
	inout float stackNear[MAX_STACK],
	inout int sp, int nodeIdx, float near
) {
	if (sp < MAX_STACK) {
		stack[sp] = nodeIdx;
		stackNear[sp] = near;
		sp++;
	}
	// else: dropped -- dropping should not happen if MAX_STACK
	// is sized to the real tree depth; consider tracking
	// max build-time depth on the CPU (self.max_depth) and
	// asserting MAX_STACK > max_depth + 1, or increasing MAX_STACK.
}



void main() {
	// Setup normalized camera coordinates
	//vec2 uv = gl_FragCoord.xy / screenResolution;
	ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
	if (pixel_coord.x >= imageSize(outputImage).x ||
		pixel_coord.y >= imageSize(outputImage).y)
		return;

	vec2 uv = vec2(pixel_coord) / screenResolution;
	vec2 uv_ndc = (uv - 0.5) * 2;
	float aspect = screenResolution.x / screenResolution.y;

	// Generate world space ray
	vec3 rayDir = normalize(camDir +
		uv_ndc.x * aspect * camFovTangent * camRight +
		uv_ndc.y * camFovTangent * camUp
	);

	vec3 rayOrigin = camPos;

	// Transform ray to object;s local space
	vec3 localOrigin = (invModelMatrix * vec4(rayOrigin, 1.0)).xyz;
	vec3 localDir = normalize((invModelMatrix * vec4(rayDir, 0.0)).xyz);

	// Modified by 'intersectAABB'
	float tNear, tFar;

	// Used to determine node traversal
	// based on which node is closer between
	// left or right
	int left;
	int right;
	float leftNear, leftFar;
	float rightNear, rightFar;
	bool hitLeft = false;
	bool hitRight = false;

	// clamp very small directions to prevent
	// cases where invDir could become NaN
	// such as when origin.x == boxMin.x
	const float EPS = 1e-8;
	vec3 safeDir = sign(localDir) * max(abs(localDir), vec3(EPS));
	vec3 invDir = 1.0 / safeDir;

	int stack[MAX_STACK];
	float stackNear[MAX_STACK];

	int sp = 0;
	stack[sp] = 0; // root idx
	stackNear[sp] = 0.0; // initialize the stackNear also
	sp++;

	float closestT = 1e30;
	vec3 hitNormalLocal = vec3(0.0);


	while (sp > 0) {
		int nodeIdx = stack[--sp];
		tNear = stackNear[sp];

		// Adding const makes drivers optimize better.
		const BVHNode node = nodes[nodeIdx];

		if (!intersectAABB(localOrigin, invDir,
			node.minBounds.xyz, node.maxBounds.xyz, tNear, tFar) || tNear > closestT) {
			continue;
		}


		if (node.triCount > 0) {
			for (int i = 0; i < node.triCount; ++i) {
				//	Fetch the triangle vertex indices and then positions
				int i3 = (node.firstTri + i) * 3;
				int i0 = indices[i3 + 0];
				int i1 = indices[i3 + 1];
				int i2 = indices[i3 + 2];
				vec3 v0 = vertices[i0].xyz;
				vec3 v1 = vertices[i1].xyz;
				vec3 v2 = vertices[i2].xyz;

				
				float maxT = closestT;
				float t; vec3 bary;

				// begin march at start and ensure to end at maxT
				if (intersectRayTriangle(localOrigin, localDir,
					v0, v1,v2, maxT, t, bary)) {
					if (t < closestT) {
						closestT = t;
						hitNormalLocal = cross(v1 - v0, v2 - v0);
					}
				}
			}

		} else {
			//	reset before testing children.
			hitLeft = false;
			hitRight = false;

			left = node.left;
			right = node.right;

			if (left >= 0) {
				BVHNode ln = nodes[left];
				hitLeft = intersectAABB(
					localOrigin, invDir,
					ln.minBounds.xyz, ln.maxBounds.xyz,
					leftNear, leftFar
				);
				hitLeft = hitLeft && (leftNear <= closestT);
			}

			if (right >= 0) {
				BVHNode rn = nodes[right];
				hitRight = intersectAABB(
					localOrigin, invDir,
					rn.minBounds.xyz, rn.maxBounds.xyz,
					rightNear, rightFar
				);
				hitRight = hitRight && (rightNear <= closestT);
			}

			if (!hitLeft && !hitRight) { continue; }

			if (hitLeft && hitRight)
			{
				if (leftNear < rightNear)
				{
					pushNode( stack, stackNear,
						sp, right, rightNear
					);
					
					pushNode( stack, stackNear,
						sp, left, leftNear
					);
				}
				else {
					pushNode( stack, stackNear,
						sp, left, leftNear
					);
					pushNode( stack, stackNear,
						sp, right, rightNear
					);
				}
			}
			else if (hitLeft) {
				pushNode( stack, stackNear,
					sp, left, leftNear
				);
			}
			else if (hitRight) { 
				pushNode( stack, stackNear,
					sp, right, rightNear
				);
			}

		}
	
	}
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	// Shading and Output
	if (closestT < 1e29) {
		// Transform normal to World Space using the Normal Matrix
		vec3 hitNormalWorld = normalize(normalModelMatrix * hitNormalLocal);

		// Simple normal visualization shading
		color = vec4(0.5 * (hitNormalWorld + vec3(1.0)), 1.0);
	}
	
	imageStore(outputImage, pixel_coord, color);
}