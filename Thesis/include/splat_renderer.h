#pragma once
#include <splat.h>
#include <vector>
#include <camera.h>
#include <shader.h>
class SplatRenderer
{
public:
	void upload(const std::vector<Splat>& splats);
	void draw(Shader& shader,  Camera& camera);
	void sort(Camera& camera);

	~SplatRenderer();
private:
	unsigned int VAO;
	unsigned int EBO;
	unsigned int quadVBO;
	unsigned int splatSSBO;
	unsigned int indexSSBO;
	std::vector<Splat> splatsVector;
	std::vector<uint32_t> indices;
	std::vector<float> depths;
	glm::mat4 model{glm::mat4(1,0,0,0, 0,-1,0,0, 0,0,-1,0, 0,0,0,1) };
};