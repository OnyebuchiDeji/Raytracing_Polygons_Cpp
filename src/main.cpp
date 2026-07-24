// #include <glad/gl.h>
// #include <GLFW/glfw3.h>
// #include <imgui.h>


// GLM Headers
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>

// #include <iostream>

// My Includes
#include "program.h"

int main() {
	// Example math usage
	// glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 2.0f);
	// glm::mat4 projection = glm::perspective(
	// 	glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f
	// );

	// std::cout << "Printing Camera Position.\n";
	// std::cout << "Cam Pos: " << camPos.x << ", "  << camPos.y << "\n";

	Program prog1(1200, 675, "Raytracing BVH, Graphics, & Compute Optimized");

	return 0;
}