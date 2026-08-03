#include <splat_renderer.h>
#include <glad/gl.h>
#include <algorithm>
#include <execution>
#include <numeric>
#include <utils.h>

namespace
{
	constexpr unsigned int SPLAT_SSBO_BINDING = 0;
	constexpr unsigned int INDEX_SSBO_BINDING = 1;
	constexpr unsigned int PREPROC_SSBO_BINDING = 2;
}
void SplatRenderer::upload(const std::vector<Splat>& splats)
{
	if (VAO == 0) initGL();
	splatsVector = splats;
	splatCount = static_cast<uint32_t>(splatsVector.size());
	hasValidPreprocess = false;

	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, splatSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Splat) * splatCount, splatsVector.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPLAT_SSBO_BINDING, splatSSBO);
	

	indices.resize(splatCount);
	std::iota(indices.begin(), indices.end(), 0u);
	depths.resize(splatCount);

	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, indices.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 3 * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SplatRenderer::draw(Shader& shader, Camera& camera,const glm::vec2& screenSize)
{
	
	shader.setVec2("uScreenSize", screenSize);
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, splatsVector.size());
}

void SplatRenderer::sort(Camera& camera)
{
	splatCount = splatsVector.size();
	glm::mat4 MV = camera.getViewMatrix() * model;
	
	for (uint32_t i = 0; i < splatCount; i++)
	{
		glm::vec3 viewPos = glm::vec3(MV * glm::vec4(splatsVector[i].position[0], splatsVector[i].position[1], splatsVector[i].position[2], 1.0f));
		depths[i] = -viewPos.z;

	}
	std::sort(std::execution::par_unseq, indices.begin(), indices.end(), [this](int a, int b) {
		return depths[a] > depths[b];
		});


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
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

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPLAT_SSBO_BINDING, splatSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);

	glDispatchCompute((splatCount + 255u) / 256u, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

SplatRenderer::~SplatRenderer()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &splatSSBO);
	glDeleteBuffers(1, &indexSSBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &quadVBO);
	glDeleteBuffers(1, &preprocSSBO);
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
}
