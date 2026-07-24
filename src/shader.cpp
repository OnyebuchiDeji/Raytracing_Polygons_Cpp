#include "shader.h"
#include <iostream>
#include <fstream>
#include <glad/gl.h>


static std::string ReadTextFile(const std::filesystem::path& path)
{
	std::ifstream file(path);

	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << path.string() << "\n";
		return {};
	}

	std::ostringstream contentStream;
	contentStream << file.rdbuf();
	return contentStream.str();
}

static void ShaderInfoLog(GLuint shaderHandle)
{
	GLint maxLength = 0;
	glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

	std::vector<GLchar> infoLog(maxLength);
	glGetShaderInfoLog(shaderHandle, maxLength, &maxLength, &infoLog[0]);

	std::cerr << infoLog.data() << "\n";
}

static void ProgramInfoLog(GLuint programHandle)
{
	GLint maxLength = 0;
	glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &maxLength);

	std::vector<GLchar> infoLog(maxLength);
	glGetProgramInfoLog(programHandle, maxLength, &maxLength, &infoLog[0]);

	std::cerr << infoLog.data() << "\n";
}

uint32_t CreateComputeShader(const std::filesystem::path& path)
{
	std::string shaderSource = ReadTextFile(path);
	GLuint shaderHandle = glCreateShader(GL_COMPUTE_SHADER);

	const GLchar* source =(const GLchar*)shaderSource.c_str();
	glShaderSource(shaderHandle, 1, &source, 0);

	glCompileShader(shaderHandle);

	GLint isCompiled = 0;
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {

		ShaderInfoLog(shaderHandle);

		glDeleteShader(shaderHandle);

		return -1;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, shaderHandle);
	glLinkProgram(program);

	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE) {
		ProgramInfoLog(program);
		glDeleteProgram(program);
		glDeleteShader(shaderHandle);
		return -1;
	}

	glDetachShader(program, shaderHandle);
	return program;
}

uint32_t ReloadComputeShader(uint32_t shaderHandle, const std::filesystem::path& path)
{
	uint32_t newShaderHandle = CreateComputeShader(path);

	// Return old shader if compilation failed
	if (newShaderHandle == -1) return shaderHandle;

	glDeleteProgram(shaderHandle);
	return newShaderHandle;
}

uint32_t CreateGraphicsShader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
{
	std::string vertexShaderSource = ReadTextFile(vertexPath);
	std::string fragmentShaderSource = ReadTextFile(fragmentPath);

	// Vertex Shader

	GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* source = (const GLchar*)vertexShaderSource.c_str();
	glShaderSource(vertexShaderHandle, 1, &source, 0);

	glCompileShader(vertexShaderHandle);

	GLint isCompiled = 0;
	glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {
		ShaderInfoLog(vertexShaderHandle);
		glDeleteShader(vertexShaderHandle);
		return -1;
	}

	// Fragment shader

	GLuint fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

	source = (const GLchar*)fragmentShaderSource.c_str();
	glShaderSource(fragmentShaderHandle, 1, &source, 0);

	glCompileShader(fragmentShaderHandle);

	isCompiled = 0;
	glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {
		ShaderInfoLog(fragmentShaderHandle);
		glDeleteShader(fragmentShaderHandle);
		return -1;
	}

	// Program linking

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShaderHandle);
	glAttachShader(program, fragmentShaderHandle);
	glLinkProgram(program);

	GLint isLinked=  0;
	glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE) {
		ProgramInfoLog(program);
		glDeleteProgram(program);
		glDeleteShader(vertexShaderHandle);
		glDeleteShader(fragmentShaderHandle);

		return -1;
	}

	glDetachShader(program, vertexShaderHandle);
	glDetachShader(program, fragmentShaderHandle);
	return program;
}

uint32_t ReloadGraphicsShader(uint32_t shaderHandle,
	const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
{
	uint32_t newShaderHandle = CreateGraphicsShader(vertexPath, fragmentPath);

	// Return old shader if compilation failed
	if (newShaderHandle == -1) return shaderHandle;

	glDeleteProgram(shaderHandle);
	return newShaderHandle;
}