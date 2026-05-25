#include <shader.h>
#include <glad/gl.h>
#include <iostream>
#include <fstream>
#include <sstream>
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
	std::string vertexCode, fragmentCode;
	std::fstream vertexFile, fragmentFile;
	vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		std::stringstream vertexSS, fragmentSS;
		vertexFile.open(vertexPath);
		vertexSS << vertexFile.rdbuf();
		vertexFile.close();

		fragmentFile.open(fragmentPath);
		fragmentSS << fragmentFile.rdbuf();
		fragmentFile.close();

		vertexCode = vertexSS.str();
		fragmentCode = fragmentSS.str();

	}
	catch (std::ifstream::failure e) 
	{
		std::cerr << "Failed  to open shader File! " << e.what() << "\n";
	}
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
			std::cerr << "Failed to link program!\n";
			glGetProgramInfoLog(ID, 1024, nullptr, log);
			std::cerr << log << std::endl;
		}
		
	}
	else if (type == "VERTEX")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			std::cerr << "Failed to compile vertex shader!\n";
			glGetShaderInfoLog(shader, 1024, nullptr, log);
			std::cerr << log << std::endl;
		}
	}
	else
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			std::cerr << "Failed to compile fragment shader!\n";
			glGetShaderInfoLog(shader, 1024, nullptr, log);
			std::cerr << log << std::endl;
		}
	}
}

void Shader::setFloat(const std::string& name, float value)
{
	glUniform1f(getUniformLocation(name), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
	glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string name, const glm::vec3& vec)
{
	glUniform3fv(getUniformLocation(name), 1, &vec[0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat)
{
	glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

Shader::~Shader()
{
	glDeleteProgram(ID);
}

unsigned int Shader::getUniformLocation(const std::string& name)
{
	if (uniformLocation.find(name) != uniformLocation.end())
	{
		return uniformLocation[name];
	}
	unsigned int loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "Failed to find uniform location: " << name << "!\n";
	}
	uniformLocation[name] = loc;
	return loc;
}


