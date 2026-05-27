#include <splat_renderer.h>
#include <glad/gl.h>
#include <algorithm>
void SplatRenderer::upload(const std::vector<Splat>& splats)
{
	splatsVector = splats;
	const float quad[] = {
		 1.0f,  1.0f, 0.0f,   
		 1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};
	const int indices[] = {
		0,1,3,
		1,2,3
	};


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
	glEnableVertexAttribArray(5);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);



	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Splat) * splatsVector.size(), splats.data(), GL_STATIC_DRAW);
	
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 56, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 56, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 56, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 56, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 56, (void*)(10 * sizeof(float)));
	glEnableVertexAttribArray(4);

	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	

	
	
	glBindVertexArray(0);
}

void SplatRenderer::draw(Shader& shader, Camera& camera)
{
	
	shader.setMat4("uView", camera.getViewMatrix());
	shader.setMat4("uProj", camera.getProjMatrix());
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, splatsVector.size());
}

void SplatRenderer::sort(Camera& camera)
{
	uint32_t n = splatsVector.size();
	std::vector<float> depths(n);
	std::vector<uint32_t> indices(n);
	glm::mat4 view = camera.getViewMatrix();
	for (uint32_t i = 0; i < n; i++)
	{
		glm::vec3 viewPos = glm::vec3(view * glm::vec4(splatsVector[i].position[0], -splatsVector[i].position[1], splatsVector[i].position[2], 1.0f));
		depths[i] = -viewPos.z;
		indices[i] = i;
	}
	std::sort(indices.begin(), indices.end(), [&depths](int a, int b) {
		return depths[a] > depths[b];
		});
	std::vector<Splat> sortedSplats(n);
	for (uint32_t i = 0; i < n; i++)
	{
		int original = indices[i];
		sortedSplats[i] = splatsVector[original];
	}
	splatsVector = sortedSplats;
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sortedSplats.size() * sizeof(Splat), sortedSplats.data());
}

SplatRenderer::~SplatRenderer()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &quadVBO);
}
