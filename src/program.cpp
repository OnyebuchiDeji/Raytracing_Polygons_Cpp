#include <iostream>


#include "program.h"
#include "bvh.h"


Program::Program(
	int width, int height, const char* title,
	bool useTriQuad, bool useCompute
) :
m_WindowWidth(width), m_WindowHeight (height), m_Title(title),
m_UtilizeTriQuad(useTriQuad), m_UtilizeCompute(useCompute),
m_LastTime((float)glfwGetTime()), m_SecondsTimer(0.0f), m_FPS(0)
{
	InitializeWindowAndOpenGL();
	InitializeVertexDisplayStructures();
	PrepareModelRaytraceRender();
	Run();
}

void Program::CleanUpBuffers()
{
	glDeleteBuffers(1, &m_IdxSSBO);
	glDeleteBuffers(1, &m_VertSSBO);
	glDeleteBuffers(1, &m_BvhSSBO);
}

Program::~Program()
{
	//	Clean up GPU assets systematically to prevent leaks
	glDeleteProgram(m_Shader);
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	if (!m_UtilizeTriQuad) glDeleteBuffers(1, &m_IBO);
	CleanUpBuffers();
	std::cout << "Program " << m_Title << " Closed!\n";
}

void Program::ErrorCallback(int error, const char* description)
{
	std::cerr << "OpenGL/GLFW Error (" << error << "): " << description << "\n";
}


void Program::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	// if (key == GLFW_KEY_R && action==GLFW_RELEASE)
	// 	this->ReloadShader(); // This now safely works!

	//	Camera Controls
	// Switch-Case statements produce non-continuous/jittery movement
	// Hence, if statements are the best.
	// However, KeyCallback only finds one key at a time.
	this->m_KeysReleased[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
	/**
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch(key) {
			//	WASDQEIO
			case GLFW_KEY_W:
				m_Camera.Move(m_DeltaTime, {0.0, 0.0, 1.0}); break;
			case GLFW_KEY_S:
				m_Camera.Move(m_DeltaTime, {0.0, 0.0, -1.0}); break;
			case GLFW_KEY_A:
				m_Camera.Move(m_DeltaTime, {-1.0, 0.0, 0.0}); break;
			case GLFW_KEY_D:
				m_Camera.Move(m_DeltaTime, {1.0, 0.0, 0.0}); break;
			case GLFW_KEY_Q:
				m_Camera.Move(m_DeltaTime, {0.0, 1.0, 0.0}); break;
			case GLFW_KEY_E:
				m_Camera.Move(m_DeltaTime, {0.0, -1.0, 0.0}); break;
			case GLFW_KEY_I:
				m_Camera.Zoom(-1.0f * m_DeltaTime); break;
			case GLFW_KEY_O:
				m_Camera.Zoom(1.0f * m_DeltaTime); break;


			//	Model (Matrix) Control -- Arraw Keys
			case GLFW_KEY_UP:
				m_Mesh.Model.Move(m_DeltaTime, {0.0, 0.0, 1.0}); break;
			case GLFW_KEY_DOWN:
				m_Mesh.Model.Move(m_DeltaTime, {0.0, 0.0, -1.0}); break;
			case GLFW_KEY_LEFT:
				m_Mesh.Model.Move(m_DeltaTime, {-1.0, 0.0, 0.0}); break; 
			case GLFW_KEY_RIGHT:
				m_Mesh.Model.Move(m_DeltaTime, {1.0, 0.0, 0.0}); break;
			case GLFW_KEY_PAGE_UP:
				m_Mesh.Model.Move(m_DeltaTime, {0.0, 1.0, 0.0}); break;
			case GLFW_KEY_PAGE_DOWN:
				m_Mesh.Model.Move(m_DeltaTime, {0.0, -1.0, 0.0}); break;
		}
	}
	*/

	// Model Change Key Controls
	if (action == GLFW_RELEASE) {
		int targetKeys[] {GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9};
		for (int idx=0; idx<sizeof(targetKeys)/sizeof(targetKeys[0]); idx++) {
			if (key == targetKeys[idx]) {
				m_ModelIdx = idx;
				this->ReloadShader();
				this->CleanUpBuffers();
				this->PrepareModelRaytraceRender();
				break;
			}
		}		
	}

}


void Program::StaticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Retrieve the C++ object instance pointer that was stored in the GLFW window
	Program* programInstance = static_cast<Program*>(glfwGetWindowUserPointer(window));

	// Safely forward the call to the non-static member function
	if (programInstance) {
		programInstance->KeyCallback(window, key, scancode, action, mods);
	}
}

void Program::RegisterUniformLocations()
{
	glUseProgram(m_Shader);

	auto registerUniform = [&] (const std::string& name) {
		GLint loc = glGetUniformLocation(m_Shader, name.c_str());
		if (loc == -1)
			std::cerr << "Warning: uniform " << name << " not found or optimized out\n";
		m_UniformLocations[name] = loc;
	};

	registerUniform("screenResolution");

	// Cannot use uniform to initialize shader array.
	// Alternative is to inject maxStack as a #define value
	// before compiling the shader!
	// But I overlooked this to just use a static value.
	// registerUniform("maxStack");
	registerUniform("primitiveShapeCount");

	// Register Model Uniforms
	registerUniform("invModelMatrix");
	registerUniform("normalModelMatrix");

	// Prepare Camera and Upload Camera Data
	registerUniform("camPos");
	registerUniform("camDir");
	registerUniform("camUp");
	registerUniform("camRight");
	registerUniform("camFovTangent");
}

void Program::ReloadShader()
{
	if (m_UtilizeCompute) 
		m_Shader = ReloadComputeShader(
			m_Shader, computeShaderPath
		);
	else
		// std::cout << "Reload Graphics Shader Called!\n";
		m_Shader = ReloadGraphicsShader(
			m_Shader,
			vertexShadersPath[(int)m_UtilizeTriQuad],
			fragmentShadersPath[m_cfsIdx]
		);


	// Refresh Resolution Mapping for the New Program Context
	RegisterUniformLocations();
	std::cout << "Reloaded Shader Successfully.\n";
}

/**
 * The callback functions used in `glfwSetErrorCallback`
 * and `glfwSetKeyCallback` must be static.
 * This is because a standard C++ member function cannot be used
 * directly as a C-style callback function pointer like GLFW expects.
 * Hence, if not static, GLFW cannot talk to them as GLFW is a C library.
 * It expects a normal function pointer with the exact signature:
 * 	`void (*)(GLFWwindow*, int, int, int, int)`.
 * But the non-static Program::ErrorCallback and Program::KeyCallback
 * have a hidden, implicit, first parameter: the `this` pointer.
 * Hence, for example, their signatures look like:
 * 	`void KeyCallback(Program* this, GLFWwindow* window, ...)`.
 * Because that signature and the signature required by GLFW do not match,
 * the compiler rejects it.
 * 
 * The issue is that static functions cannot be used to access/modify member variables.
 * But `KeyCallback` needs to do this.
 * 
 * Solution: Use static versions of ErrorCallback and KeyCallback as shown. 
 * */
void Program::InitializeWindowAndOpenGL()
{
	glfwSetErrorCallback(&Program::ErrorCallback);


	if (!glfwInit()) exit(EXIT_FAILURE);

	// Request an explicit core profile so the dev environment matches
	// the spec you're writing against, instead of getting NVIDIA's
	// lenient compatibility fallback that masks exactly this class of bug:
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // macOS requires this; harmless elsewhere

	m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, "Raytrace", NULL, NULL);

	if (!m_Window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Store the ``this`` pointer inside the GLFW windoe object 
	glfwSetWindowUserPointer(m_Window, this);


	// Pass the STATIC wrapper function to GLFW
	glfwSetKeyCallback(m_Window, &Program::StaticKeyCallback);

	glfwMakeContextCurrent(m_Window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	if (m_UtilizeCompute) 
		m_Shader = CreateComputeShader(computeShaderPath);
	else
		m_Shader = CreateGraphicsShader(
			vertexShadersPath[(int)m_UtilizeTriQuad], fragmentShadersPath[m_cfsIdx]
		);

	if (m_Shader == -1) {
		std::cerr << "Shader Preparation Failed\n";
	}

	RegisterUniformLocations();

	m_DisplayTexture = CreateTexture(m_WindowWidth, m_WindowHeight);
	m_DisplayBuffer = CreateFramebufferWithTexture(m_DisplayTexture);
}

void Program::InitializeVertexDisplayStructures()
{
	// Using Rectangular Quad Vertices for GPU Display Surface
	// And Indices - Utilizing Rect Quad Vertex Shader

	// Initialize m_VBO and m_VAO
	glCreateVertexArrays(1, &m_VAO);
	glCreateBuffers(1, &m_VBO);

	if (!m_UtilizeTriQuad){
		// For Rect Quad, Use IBO
		glCreateBuffers(1, &m_IBO);

		float vertices[] {
			-1.0f, -1.0f, // bottom left
			 1.0f, -1.0f, // bottom right
			-1.0f,  1.0f, // top left
			 1.0f,  1.0f  // top right
		};

		uint32_t indices[] {
			0, 1, 2, 3
		};

		glNamedBufferData(m_VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glNamedBufferData(m_IBO, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexArrayElementBuffer(m_VAO, m_IBO);
	}
	else {
		// Using Right-Angled Triangle
		// No indices/IBO
		float vertices[] {
			-1.0f, -1.0f, // Bottom-Left
			 3.0f, -1.0f, // Bottom-Right
			 -1.0f, 3.0f, // Top-Left
		};

		glNamedBufferData(m_VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
	}

	// Define size of a vertex
	// Currently, vertices only carry: x, y -- 2 data components that make up vertex
	// It could also carry: x, y, u, v -- 4 components.
	// for the latter, the last argument below till be `sizeof(float)*4`
	glVertexArrayVertexBuffer(m_VAO, 0, m_VBO, 0, sizeof(float)*2);

	// Defining vertex data format.
	// Because only x, y vertex components are used,
	// Hence, only one attribute type is used.
	// If a vertex was `x, y, u, v`, two Array Attribute Ids will exist
	// And for each, a particular format will be specified since
	// one will want to use them as separate attribute variables in the GPU
	// and each will be given its own GPU shader location

	glEnableVertexArrayAttrib(m_VAO, 0); // first attribute id
	// args: [0] vao, [1] attrib id, [2] count of values, [3] type to be interpreted as in GPU
	// [4] should be normalized?, [5] stride from start of vertex buffer flat buffer
	glVertexArrayAttribFormat(m_VAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

	// Bind the specific vertex attribute of id 0 to a GPU shader location ID, 0 
	glVertexArrayAttribBinding(m_VAO, 0, 0);
}

void Program::PrepareModelRaytraceRender()
{
	std::cout << "Preparing Models for Raytrace...\n";
	m_Mesh = CreateMesh(m_ModelPaths[m_ModelIdx]);
	std::cout << "Done Creating Mesh\n";
	m_Camera = CameraState(static_cast<float>(m_WindowWidth)/static_cast<float>(m_WindowHeight));

	// Upload Mesh Geometry Using Modern DSA SSBO Methods

	BVHBuilder builder;
	int root = builder.Build(m_Mesh); // root should be 0 if Build is only called once
	std::vector<uint32_t> reorderedIndices  = ReorderIndices(m_Mesh, builder.triIndex);
	std::vector<GPUBVHNode> gpuNodes = ToGPUNodes(builder.nodes);


	//	Modern Method
	glCreateBuffers(1, &m_IdxSSBO);
	// Index Buffer (binding = 0) was
	if (m_cfsIdx == 1) {
		glNamedBufferStorage(
			m_IdxSSBO,
			m_Mesh.VertexIndices.size() * sizeof(uint32_t),
			m_Mesh.VertexIndices.data(), 0
		);
	}else {
		//	Now the reordered version
		glNamedBufferStorage(
			m_IdxSSBO,
			reorderedIndices.size() * sizeof(uint32_t),
			reorderedIndices.data(), 0
		);
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_IdxSSBO);

	// Vertex buffer (binding=1) - Unchanged, indices above still refer into this
	glCreateBuffers(1, &m_VertSSBO);
	glNamedBufferStorage(
		m_VertSSBO,
		m_Mesh.Vertices.size() * sizeof(glm::vec4),
		m_Mesh.Vertices.data(), 0
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_VertSSBO);


	if (m_cfsIdx == 2) { // using bvh shader
		//	BVH nodes (binding = 2)
		glCreateBuffers(1, &m_BvhSSBO);
		glNamedBufferStorage(
			m_BvhSSBO,
			gpuNodes.size() * sizeof(GPUBVHNode),
			gpuNodes.data(), 0
		);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_BvhSSBO);
	}


	// Register Base Uniforms
	glUseProgram(m_Shader);
	
	glUniform2f(
		m_UniformLocations.at("screenResolution"),
		m_WindowWidth, m_WindowHeight
	);

	glUniform1i(
		m_UniformLocations.at("primitiveShapeCount"),
		(int)(m_Mesh.VertexIndices.size() / 3)
	);

	// Cannot use uniform to initialize shader array.
	// Alternative is to inject maxStack as a #define value
	// before compiling the shader!
	// glUniform1i(
	// 	m_UniformLocations.at("maxStack"),
	// 	(int)(builder.maxDepthSeen + 1)
	// );

	// std::cout << "Primitive Count: " << m_Mesh.VertexIndices.size() / 3 << "\n";

	UpdateUniforms();
	std::cout << "Done Preparing Models for Raytrace!\n";
}

void Program::UpdateUniforms()
{
	glUseProgram(m_Shader);

	m_Mesh.Model.UpdateMatrix();

	//	Update Model Uniforms
	glm::mat3 normalMatrix = glm::transpose(
		glm::inverse(glm::mat3(m_Mesh.Model.Matrix))
	);

	// Use .at() instead of [] to raise error if key does not exist
	glUniformMatrix4fv(
		m_UniformLocations.at("invModelMatrix"),
		1, GL_FALSE, glm::value_ptr(glm::inverse(m_Mesh.Model.Matrix))
	);
	glUniformMatrix3fv(
		m_UniformLocations.at("normalModelMatrix"),
		1, GL_FALSE, glm::value_ptr(normalMatrix)
	);

	//	Update Camera Uniforms
	glUniform3fv(m_UniformLocations.at("camPos"),
		1, glm::value_ptr(m_Camera.Position)
	);
	glUniform3fv(m_UniformLocations.at("camDir"),
		1, glm::value_ptr(m_Camera.Forward)
	);
	glUniform3fv(m_UniformLocations.at("camUp"),
		1, glm::value_ptr(m_Camera.Up)
	);
	glUniform3fv(m_UniformLocations.at("camRight"),
		1, glm::value_ptr(m_Camera.Right)
	);
	glUniform1f(m_UniformLocations.at("camFovTangent"), m_Camera.FOVTangent);
}

/**
 * Better than using switch statement in the KeyCallback method
 * as this way enables detecting and applying effect of multiple
 * keys pressed.
 * */
void Program::SenseKeyInput()
{
	std::unordered_map<int, bool>& keys = m_KeysReleased;
	// Controls
	//	WASDQEIO
	if (keys.contains(GLFW_KEY_W) && keys[GLFW_KEY_W]) {
		m_Camera.Move(m_DeltaTime, {0.0, 0.0, 1.0});
	}
	if (keys.contains(GLFW_KEY_S) && keys[GLFW_KEY_S]) {
		m_Camera.Move(m_DeltaTime, {0.0, 0.0, -1.0});
	}
	if (keys.contains(GLFW_KEY_A) && keys[GLFW_KEY_A]) {
		m_Camera.Move(m_DeltaTime, {-1.0, 0.0, 0.0});
	}
	if (keys.contains(GLFW_KEY_D) && keys[GLFW_KEY_D]) {
		m_Camera.Move(m_DeltaTime, {1.0, 0.0, 0.0});
	}
	if (keys.contains(GLFW_KEY_Q) && keys[GLFW_KEY_Q]) {
		m_Camera.Move(m_DeltaTime, {0.0, 1.0, 0.0});
	}
	if (keys.contains(GLFW_KEY_E) && keys[GLFW_KEY_E]) {
		m_Camera.Move(m_DeltaTime, {0.0, -1.0, 0.0});
	}
	if (keys.contains(GLFW_KEY_I) && keys[GLFW_KEY_I]) {
		m_Camera.Zoom(-1.0f * m_DeltaTime);
	}
	if (keys.contains(GLFW_KEY_O) && keys[GLFW_KEY_O]) {
		m_Camera.Zoom(1.0f * m_DeltaTime);
	}


	//	Model (Matrix) Control -- Arraw Keys
	if (keys.contains(GLFW_KEY_UP) && keys[GLFW_KEY_UP]) {
		m_Mesh.Model.Move(m_DeltaTime, {0.0, 0.0, 1.0});
	}
	if (keys.contains(GLFW_KEY_DOWN) && keys[GLFW_KEY_DOWN]) {
		m_Mesh.Model.Move(m_DeltaTime, {0.0, 0.0, -1.0});
	}
	if (keys.contains(GLFW_KEY_LEFT) && keys[GLFW_KEY_LEFT]) {
		m_Mesh.Model.Move(m_DeltaTime, {-1.0, 0.0, 0.0}); 
	}
	if (keys.contains(GLFW_KEY_RIGHT) && keys[GLFW_KEY_RIGHT]) {
		m_Mesh.Model.Move(m_DeltaTime, {1.0, 0.0, 0.0});
	}
	if (keys.contains(GLFW_KEY_PAGE_UP) && keys[GLFW_KEY_PAGE_UP]) {
		m_Mesh.Model.Move(m_DeltaTime, {0.0, 1.0, 0.0});
	}
	if (keys.contains(GLFW_KEY_PAGE_DOWN) && keys[GLFW_KEY_PAGE_DOWN]) {
		m_Mesh.Model.Move(m_DeltaTime, {0.0, -1.0, 0.0});
	}

}

void Program::OnUpdate()
{
	glfwGetFramebufferSize(m_Window, &m_WindowWidth, &m_WindowHeight);

	// Get FPS
	float currentTime = (float)glfwGetTime();
	m_DeltaTime = currentTime - m_LastTime;
	m_LastTime = currentTime;

	m_SecondsTimer += m_DeltaTime;
	if (m_SecondsTimer >= 1.0f)
	{
		std::string title = std::format("{} - {} fps", m_Title, m_FPS);
		glfwSetWindowTitle(m_Window, title.c_str());

		m_SecondsTimer = 0.0f;
		m_FPS = 0;
	}

	// Resize texture
	if (m_WindowWidth != m_DisplayTexture.Width || m_WindowHeight != m_DisplayTexture.Height) {
		glDeleteTextures(1, &m_DisplayTexture.Handle);
		m_DisplayTexture = CreateTexture(m_WindowWidth, m_WindowHeight);
		AttachTextureToFramebuffer(m_DisplayBuffer, m_DisplayTexture);
		m_Camera.AspectRatio = m_WindowWidth/m_WindowHeight;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(m_Window, &mouseX, &mouseY);
	glfwSetCursorPos(m_Window,
		static_cast<double>(m_WindowWidth / 2),
		static_cast<double>(m_WindowHeight / 2)
	);

	m_Camera.Rotate(
		static_cast<float>(mouseX) - m_WindowWidth/2,
		static_cast<float>(mouseY) - m_WindowHeight/2
	);

	SenseKeyInput();

	UpdateUniforms();

	OnRender();

	glfwPollEvents();

	m_FPS++;

}


void Program::OnRender()
{
	// Render
	if (m_UtilizeCompute) // Compute Shader Route
	{
		glBindImageTexture(
			0, m_DisplayBuffer.Handle, 0, GL_FALSE,
			0, GL_WRITE_ONLY, GL_RGBA32F
		);

		const GLuint workGroupSizeX = 16;
		const GLuint workGroupSizeY = 16;
		GLuint numGroupsX = (m_WindowWidth + workGroupSizeX - 1) / workGroupSizeX;
		GLuint numGroupsY = (m_WindowHeight + workGroupSizeY - 1) / workGroupSizeY;

		glDispatchCompute(numGroupsX, numGroupsY, 1);

		// Ensure all writes to the image are complete
		// by waiting for memory writes to settle before
		// blitting or drawing.
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	else // Graphics Shader Route
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_DisplayBuffer.Handle);
		// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(m_VAO);
		if (!m_UtilizeTriQuad) {
			// glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
			glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, nullptr);
		}
		else {
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Blit
	{
		BlitFramebufferToSwapchain(m_DisplayBuffer);
	}

	glfwSwapBuffers(m_Window);
}

void Program::Run()
{
	while (!glfwWindowShouldClose(m_Window)) {
		OnUpdate();
		OnRender();
	}

	glfwDestroyWindow(m_Window);
	glfwTerminate();
}