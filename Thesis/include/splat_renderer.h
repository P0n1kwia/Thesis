#pragma once
#include <splat.h>
#include <vector>
#include <camera.h>
#include <shader.h>

struct RenderParams
{
	float minOpacity = 0.0039f;
	float scaleMultiplier = 1.0f;
	float dilation = 0.3f;
	float maxRadiusPx = 1024.0f;
	int shDegree = 2;

	bool operator==(const RenderParams&) const = default;
};

class SplatRenderer
{
public:
	void upload(const std::vector<Splat>& splats);
	void draw(Shader& shader,  Camera& camera, const glm::vec2& screenSize);
	void sort(Camera& camera);
	void preprocess(Shader& computeShader, Camera& camera, const glm::vec2& screenSize, const RenderParams& params);
	size_t getEstimatedVramBytes() const;
	uint32_t getSplatCount() const;
	uint32_t getDrawCount() const;
	const std::vector<Splat>& getSplats() const;
	const std::vector<uint32_t>& getVisibleIndices() const;
	std::vector<glm::vec2> fetchVisibleScreenExtents() const;

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
	std::vector<Splat> splatsVector;
	std::vector<float> depths;
	glm::mat4 model{glm::mat4(1,0,0,0, 0,-1,0,0, 0,0,-1,0, 0,0,0,1) };

	glm::mat4 lastPreprocessView{};
	glm::mat4 lastPreprocessProj{};
	glm::vec3 lastPreprocessCamPos{};
	glm::vec2 lastPreprocessScreenSize{};
	RenderParams lastPreprocessParams{};
	bool hasValidPreprocess = false;

	std::vector<uint32_t> visibleIndices;
	uint32_t drawCount = 0;
};