#pragma once
#include <splat.h>
#include <vector>
#include <camera.h>
#include <shader.h>
class SplatRenderer
{
public:
	void upload(const std::vector<Splat>& splats);
	void draw(Shader& shader,  Camera& camera, const glm::vec2& screenSize);
	void sort(Camera& camera);
	void preprocess(Shader& computeShader, Camera& camera, const glm::vec2& screenSize);
	size_t getEstimatedVramBytes() const;
	glm::vec3 getBboxCenter() const;
	float getBoundingRadius() const;

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
	unsigned int preprocSSBO = 0;
	unsigned int visibleCountSSBO = 0;
	unsigned int visibleIndexSSBO = 0;
	unsigned int splatCount = 0;
	glm::vec3 bboxMin{ 0.0f };
	glm::vec3 bboxMax{ 0.0f };
	std::vector<Splat> splatsVector;
	std::vector<float> depths;
	glm::mat4 model{glm::mat4(1,0,0,0, 0,-1,0,0, 0,0,-1,0, 0,0,0,1) };

	glm::mat4 lastPreprocessView{};
	glm::mat4 lastPreprocessProj{};
	glm::vec3 lastPreprocessCamPos{};
	glm::vec2 lastPreprocessScreenSize{};
	bool hasValidPreprocess = false;

	std::vector<uint32_t> visibleIndices;
	uint32_t drawCount = 0;
};