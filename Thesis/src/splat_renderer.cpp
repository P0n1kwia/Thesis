#include <splat_renderer.h>
#include <glad/gl.h>
void SplatRenderer::upload(const std::vector<Splat>& splats)
{
	splat_count = splats.size();
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Splat) * splat_count, splats.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 56, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 56, (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 56, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 56, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 56, (void*)(10 * sizeof(float)));
	glEnableVertexAttribArray(4);
	
	glBindVertexArray(0);
}

void SplatRenderer::draw(Shader& shader, Camera& camera)
{
	
	shader.setMat4("uView", camera.getViewMatrix());
	shader.setMat4("uProj", camera.getProjMatrix());
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, splat_count);
}

SplatRenderer::~SplatRenderer()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}
