#include <splat_renderer.h>
#include <glad/gl.h>
#include <algorithm>
#include <execution>
#include <utils.h>

namespace
{
	constexpr unsigned int SPLAT_SSBO_BINDING = 0;
	constexpr unsigned int INDEX_SSBO_BINDING = 1;
	constexpr unsigned int PREPROC_SSBO_BINDING = 2;
	constexpr unsigned int VISIBLE_COUNT_SSBO_BINDING = 3;
	constexpr unsigned int VISIBLE_INDEX_SSBO_BINDING = 4;
}
void SplatRenderer::upload(const std::vector<Splat>& splats)
{
	if (VAO == 0) initGL();
	splatsVector = splats;
	splatCount = static_cast<uint32_t>(splatsVector.size());
	hasValidPreprocess = false;
	visibleIndices.clear();
	drawCount = 0;


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, splatSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Splat) * splatCount, splatsVector.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPLAT_SSBO_BINDING, splatSSBO);


	depths.resize(splatCount);

	// indexSSBO's content is written by sort() (compacted + depth-sorted visible
	// indices) before it is ever consumed by draw(), so no initial data is needed
	// here -- only the capacity for the worst case (every splat visible).
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 3 * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleCountSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_COUNT_SSBO_BINDING, visibleCountSSBO);

	// Worst case every splat is visible, so this needs full splatCount capacity.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleIndexSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SplatRenderer::draw(Shader& shader, Camera& camera,const glm::vec2& screenSize)
{

	shader.setVec2("uScreenSize", screenSize);
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, drawCount);
}

void SplatRenderer::sort(Camera& camera)
{
	glm::mat4 MV = camera.getViewMatrix() * model;

	for (uint32_t id : visibleIndices)
	{
		glm::vec3 viewPos = glm::vec3(MV * glm::vec4(splatsVector[id].position[0], splatsVector[id].position[1], splatsVector[id].position[2], 1.0f));
		depths[id] = -viewPos.z;
	}
	std::sort(std::execution::par_unseq, visibleIndices.begin(), visibleIndices.end(), [this](uint32_t a, uint32_t b) {
		return depths[a] > depths[b];
		});

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, visibleIndices.size() * sizeof(uint32_t), visibleIndices.data());
	drawCount = static_cast<uint32_t>(visibleIndices.size());
}

void SplatRenderer::preprocess(Shader& computeShader, Camera& camera, const glm::vec2& screenSize)
{
	const glm::mat4& view = camera.getViewMatrix();
	const glm::mat4& proj = camera.getProjMatrix();
	glm::vec3 camPos = camera.getPosition();


	if (hasValidPreprocess && view == lastPreprocessView && proj == lastPreprocessProj &&
		camPos == lastPreprocessCamPos && screenSize == lastPreprocessScreenSize)
	{
		return;
	}
	lastPreprocessView = view;
	lastPreprocessProj = proj;
	lastPreprocessCamPos = camPos;
	lastPreprocessScreenSize = screenSize;
	hasValidPreprocess = true;

	computeShader.use();
	computeShader.setMat4("uView", view);
	computeShader.setMat4("uProj", proj);
	computeShader.setMat4("uModel", model);
	computeShader.setVec3("uCamPos", camPos);
	computeShader.setVec2("uScreenSize", screenSize);
	computeShader.setUInt("uCount", splatCount);

	auto frustumPlanes = extractFrustumPlanes(proj * view);
	computeShader.setVec4Array("uFrustum", frustumPlanes.data(), static_cast<unsigned int>(frustumPlanes.size()));

	GLuint zero = 0;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleCountSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPLAT_SSBO_BINDING, splatSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_COUNT_SSBO_BINDING, visibleCountSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);

	glDispatchCompute((splatCount + 255u) / 256u, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	GLuint visibleCount = 0;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleCountSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &visibleCount);

	visibleIndices.resize(visibleCount);
	if (visibleCount > 0)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleIndexSSBO);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, visibleIndices.data());
	}
}

SplatRenderer::~SplatRenderer()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &splatSSBO);
	glDeleteBuffers(1, &indexSSBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &quadVBO);
	glDeleteBuffers(1, &preprocSSBO);
	glDeleteBuffers(1, &visibleCountSSBO);
	glDeleteBuffers(1, &visibleIndexSSBO);
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
	glGenBuffers(1, &preprocSSBO);
	glGenBuffers(1, &visibleCountSSBO);
	glGenBuffers(1, &visibleIndexSSBO);
}
