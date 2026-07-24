#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <print>


#include "shader.h"
#include "renderer.h"
#include "model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const float g_Sensitivity {0.07f};
const float g_Speed {3.0f};

struct CameraState {
	float AspectRatio{0.0f};
	glm::vec3 Position{0.0f, 0.0f, 0.0f};
	// glm::vec3 Orientation{0.0f, 0.0f, 0.0f};
	glm::vec3 Up{0.0f, 1.0f, 0.0f};
	glm::vec3 Right{1.0f, 0.0f, 0.0f};
	glm::vec3 Forward{0.0f, 0.0f, -1.0f};
	float Yaw {0.0f};
	float Pitch {0.0f};
	float FOV {0.0f};
	float FOVTangent{0.0f};
	glm::mat4 ViewMatrix{1.0f};

	CameraState() {}

	CameraState(
		float aspectRatio, glm::vec3 pos={0.0f, 0.0f, 4.0f}, float yaw=-90.0f, float pitch=0.0f, float fov=100.0f)
	:
	AspectRatio(aspectRatio), Position(pos),
	Yaw(yaw), Pitch(pitch), FOV(fov)
	{
		SolveFov();
		UpdateVectors(); // <- derive Forward/Right/Up/ViewMatrix from Yaw/Pitch Explicitely
	}

	void SolveFov()
	{
		FOVTangent = glm::tan(glm::radians(FOV/2.0f));
	}

	void Rotate(float deltaYaw, float deltaPitch)
	{
		Yaw += deltaYaw * g_Sensitivity;
		Pitch -= deltaPitch * g_Sensitivity;
		//  Limiting pitch movement to prevent unnatural movements up and down from Gimbal Lock
        Pitch = glm::max(-89.0f, glm::min(89.0f, Pitch));
        UpdateVectors();
	}

	void Move(float dt, glm::vec3 dPos)
	{
		// Extract a ground-only forward vector for realistic FPS movement
		// glm::vec3 fpsForward = glm::normalize(glm::vec3(Forward.x, 0.0f, Forward.z));

		Position += Up * dPos.y * g_Speed * dt; // Jumping and Crouching if Used
		Position += Right * dPos.x * g_Speed * dt; // Strafing Left/Right
		// for FPS, replace Forward with fpsForward
		Position += Forward * dPos.z * g_Speed * dt; // Walking Forward/Baclward on Ground
		UpdateVectors();
	}

	void Zoom(float dFov)
	{
		FOV += dFov * g_Speed;
		FOV = glm::clamp(FOV, 1.0f, 120.0f); // Prevent FOV inversion or extreme values
		SolveFov();	// Update FOVTangent to match new zoom level
	}

	void UpdateVectors()
	{
		float yaw = glm::radians(Yaw);
		float pitch = glm::radians(Pitch);
		Forward.x = glm::cos(yaw) * glm::cos(pitch);
		Forward.y = glm::sin(pitch);
		Forward.z = glm::sin(yaw) * glm::cos(pitch);

		Forward = glm::normalize(Forward);
		Right = glm::normalize(glm::cross(Forward, {0.0f, 1.0f, 0.0f}));
		Up = glm::normalize(glm::cross(Right, Forward));

		ViewMatrix = glm::lookAt(Position, Position + Forward, Up);
	}

	void Debug()
	{
		std::print(
			"Camera State Object\n | Aspect Ratio: {0}, \n\
			FOV: {1}, FOVTangent: {2}, \n\
			Position (x, y, z): [{3}, {4}, {5}], \
			\n",
			AspectRatio,
		 	FOV, FOVTangent,
		 	Position.x,Position.y, Position.z);
	}

};


class Program
{
private:
	int m_WindowWidth {0}, m_WindowHeight {0};
	const char* m_Title;
	// const char* m_Description;
	bool m_UtilizeTriQuad {false};
	bool m_UtilizeCompute {false};
	float m_LastTime;
	float m_SecondsTimer;
	float m_DeltaTime;
	int m_FPS;

	GLFWwindow* m_Window = nullptr;
	Texture m_DisplayTexture;
	Framebuffer m_DisplayBuffer;
	GLuint m_Shader;
	Mesh m_Mesh;
	CameraState m_Camera;
	GLuint m_VBO;
	GLuint m_IBO; // Index/Element Buffer Object
	GLuint m_VAO;
	GLuint m_VertSSBO, m_IdxSSBO, m_BvhSSBO;

	const std::vector<const char*> vertexShadersPath {
		"src/shaders/shader1-rect_quad.vert",
		"src/shaders/shader1-tri_quad.vert"
	};
	const std::vector<const char*> fragmentShadersPath {
		"src/shaders/shader1.frag",
		"src/shaders/shader1-ssbos.frag",
		"src/shaders/shader1-ssbos_bvh.frag",
		"src/shaders/shader1-1dtexture_buffer.frag",
		"src/shaders/shader1-2dtexture_buffer.frag"
	};
	const char* computeShaderPath {"src/shaders/shader2-compute.glsl"};
	// uint32_t m_cvsIdx {0}; // current vertex shader idx
	uint32_t m_cfsIdx {2}; // current fragment shader idx
	std::unordered_map<std::string, uint32_t> m_UniformLocations;
	std::unordered_map<int, bool> m_KeysReleased;
	const std::vector<const char*> m_ModelPaths {
		"models/cube.obj",
		"models/axis.obj",
		"models/baryonx.obj",
		"models/deino.obj",
		"models/ground.obj",
		"models/mountains.obj",
		"models/tank.obj",
		"models/teapot.obj",
		"models/wall.obj",
		"models/VideoShip.obj"
	};
	int m_ModelIdx {0};


public:
	Program() {};
	Program(int width, int height, const char* title,
			bool useTriQuad=false, bool useCompute=false);
	~Program();

	void Run();

private:
	static void ErrorCallback(int error, const char* description);
	
	// Actual, Non-Static Functions
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	//	Static Wrapper Function
	static void StaticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void ReloadShader();
	void InitializeWindowAndOpenGL();
	void InitializeVertexDisplayStructures();
	void RegisterUniformLocations();
	void PrepareModelRaytraceRender();
	void SenseKeyInput();
	void UpdateUniforms();
	void OnRender();
	void OnUpdate();
	void CleanUpBuffers();
};