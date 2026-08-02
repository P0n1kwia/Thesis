#include <shader.h>
#include <glad/gl.h>
#include <iostream>
#include <fstream>
#include <sstream>
std::string Shader::readFile(const std::string& path)
{
	std::fstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		std::stringstream ss;
		file.open(path);
		ss << file.rdbuf();
		file.close();
		return ss.str();
	}
	catch (const std::ifstream::failure& e)
	{
		throw std::runtime_error("Failed to open shader file (" + path + "): " + e.what());
	}
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
	std::string vertexCode = readFile(vertexPath);
	std::string fragmentCode = readFile(fragmentPath);

	const char* vertexSource = vertexCode.c_str();
	const char* fragmentSource = fragmentCode.c_str();
	unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexSource, nullptr);
	glCompileShader(vertex);
	checkCompilationErrors(vertex, "VERTEX");

	unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentSource, nullptr);
	glCompileShader(fragment);
	checkCompilationErrors(fragment, "FRAGMENT");

	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);

	checkCompilationErrors(ID, "PROGRAM");
	

	glDeleteShader(vertex);
	glDeleteShader(fragment);

}

Shader::Shader(ComputeShader, const std::string& computePath)
{
	std::string computeCode = readFile(computePath);
	const char* computeSource = computeCode.c_str();
	unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute, 1, &computeSource, nullptr);
	glCompileShader(compute);
	checkCompilationErrors(compute, "COMPUTE");
	ID = glCreateProgram();
	glAttachShader(ID, compute);
	glLinkProgram(ID);
	checkCompilationErrors(ID, "PROGRAM");

	glDeleteShader(compute);
}

void Shader::use()
{
	glUseProgram(ID);
}

void Shader::checkCompilationErrors(unsigned int shader, const std::string& type)
{
	int success;
	char log[1024];
	if (type == "PROGRAM")
	{
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			GLint len = 0;
			glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &len);
			std::string log(static_cast<size_t>(len), '\0');
			glGetProgramInfoLog(shader, len, nullptr, log.data());
			throw std::runtime_error("Failed to link shader program:\n" + log);
		}
		
	}
	else
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLint len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
			std::string log(static_cast<size_t>(len), '\0');
			glGetShaderInfoLog(shader, len, nullptr, log.data());
			throw std::runtime_error("Failed to compile " + type + " shader:\n" + log);
		}
	}
}

void Shader::setInt(const std::string& name, int value)
{
	glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value)
{
	glUniform1f(getUniformLocation(name), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
	glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec)
{
	glUniform3fv(getUniformLocation(name), 1, &vec[0]);
}

void Shader::setVec2(const std::string& name, const glm::vec2& vec)
{
	glUniform2fv(getUniformLocation(name), 1, &vec[0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat)
{
	glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}


void Shader::setUInt(const std::string& name, unsigned int value)
{
	glUniform1ui(getUniformLocation(name), value);
}

Shader::~Shader()
{
	glDeleteProgram(ID);
}

int Shader::getUniformLocation(const std::string& name)
{
	if (uniformLocation.find(name) != uniformLocation.end())
	{
		return uniformLocation[name];
	}
	int loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "Failed to find uniform location: " << name << "!\n";
	}
	uniformLocation[name] = loc;
	return loc;
}


