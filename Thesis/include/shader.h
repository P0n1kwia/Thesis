#pragma once
#include <string>
#include <map>
#include <glm/glm.hpp>

class Shader
{
public:
	Shader(const std::string& vertexPath, const std::string& fragmentPath);
	void use();
	void setInt(const std::string& name, int value);
	void setFloat(const std::string& name, float value);
	void setMat4(const std::string& name, const glm::mat4& mat);
	void setVec3(const std::string& name, const glm::vec3& vec);
	void setMat3(const std::string& name, const glm::mat3& mat);

	~Shader();
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

private:
	void checkCompilationErrors(unsigned int shader, const std::string& type);

	unsigned int ID;
	std::map<std::string, int> uniformLocation;

	int getUniformLocation(const std::string& name);
	
};