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
	unsigned int VBO;
	unsigned int EBO;
	unsigned int quadVBO;
	std::vector<Splat> splatsVector;
};