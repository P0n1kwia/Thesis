#include <splat_renderer.h>
#include <glad/gl.h>
#include <algorithm>
#include <numeric>

namespace
{
	constexpr unsigned int SPLAT_SSBO_BINDING = 0;
	constexpr unsigned int INDEX_SSBO_BINDING = 1;
}
void SplatRenderer::upload(const std::vector<Splat>& splats)
{
	if (VAO == 0) initGL();
	splatsVector = splats;
	uint32_t n = static_cast<uint32_t>(splatsVector.size());
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, splatSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Splat) * n, splatsVector.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPLAT_SSBO_BINDING, splatSSBO);
	

	indices.resize(n);
	std::iota(indices.begin(), indices.end(), 0u);
	depths.resize(n);

	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * n, indices.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SplatRenderer::draw(Shader& shader, Camera& camera)
{
	
	shader.setMat4("uView", camera.getViewMatrix());
	shader.setMat4("uProj", camera.getProjMatrix());
	shader.setMat4("uModel", model);
	shader.setVec3("uCamPos", camera.getPosition());
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, splatsVector.size());
}

void SplatRenderer::sort(Camera& camera)
{
	uint32_t n = splatsVector.size();
	glm::mat4 MV = camera.getViewMatrix() * model;
	
	for (uint32_t i = 0; i < n; i++)
	{
		glm::vec3 viewPos = glm::vec3(MV * glm::vec4(splatsVector[i].position[0], splatsVector[i].position[1], splatsVector[i].position[2], 1.0f));
		depths[i] = -viewPos.z;

	}
	std::sort(indices.begin(), indices.end(), [this](int a, int b) {
		return depths[a] > depths[b];
		});


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
}

SplatRenderer::~SplatRenderer()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &splatSSBO);
	glDeleteBuffers(1, &indexSSBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &quadVBO);
}

void SplatRenderer::initGL()
{
	const float quad[] = {
		 1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};
	const int quadIndices[] = {
		0,1,3,
		1,2,3
	};


	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(5);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	glGenBuffers(1, &splatSSBO);
	glGenBuffers(1, &indexSSBO);
}
