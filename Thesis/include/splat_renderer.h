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
	SplatRenderer() = default;
	SplatRenderer(const SplatRenderer&) = delete;
	SplatRenderer& operator=(const SplatRenderer&) = delete;
private:

	void initGL();

	unsigned int VAO = 0;
	unsigned int EBO = 0;
	unsigned int quadVBO = 0;
	unsigned int splatSSBO = 0;
	unsigned int indexSSBO = 0;
	std::vector<Splat> splatsVector;
	std::vector<uint32_t> indices;
	std::vector<float> depths;
	glm::mat4 model{glm::mat4(1,0,0,0, 0,-1,0,0, 0,0,-1,0, 0,0,0,1) };
};