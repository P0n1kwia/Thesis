#include <splat_renderer.h>
#include <glad/gl.h>
#include <algorithm>
#include <execution>
#include <utils.h>
#include <bit>
#include <iostream>
#include <array>

namespace
{
	constexpr unsigned int SPLAT_SSBO_BINDING = 0;
	constexpr unsigned int INDEX_SSBO_BINDING = 1;
	constexpr unsigned int PREPROC_SSBO_BINDING = 2;
	constexpr unsigned int VISIBLE_COUNT_SSBO_BINDING = 3;
	constexpr unsigned int VISIBLE_INDEX_SSBO_BINDING = 4;


	constexpr unsigned int KEYS_A_SSBO_BINDING = 5;
	constexpr unsigned int KEYS_B_SSBO_BINDING = 6;
	constexpr unsigned int INDEX_B_SSBO_BINDING = 7;
	constexpr unsigned int WG_HISTOGRAMS_SSBO_BINDING = 8;
	constexpr unsigned int BIN_OFFSETS_SSBO_BINDING = 9;
	constexpr uint32_t ELEMENTS_PER_WORKGROUP = 1024u;
	constexpr uint32_t RADIX_SIZE = 16u;
	constexpr uint32_t DEPTH_ULP_TOLERANCE = 8u;
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

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 3 * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleCountSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_COUNT_SSBO_BINDING, visibleCountSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleIndexSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);

	maxWorkgroups = (splatCount + ELEMENTS_PER_WORKGROUP - 1u) / ELEMENTS_PER_WORKGROUP;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, keysSSBO_A);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, keysSSBO_B);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_B_SSBO_BINDING, keysSSBO_B);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO_B);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * splatCount, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_B_SSBO_BINDING, indexSSBO_B);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, wgHistogramsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * RADIX_SIZE * std::max<uint32_t>(maxWorkgroups, 1u), nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, binOffsetsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * RADIX_SIZE, nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SplatRenderer::draw(Shader& shader, Camera& camera,const glm::vec2& screenSize)
{

	shader.setVec2("uScreenSize", screenSize);
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, drawCount);
}

void SplatRenderer::sort(Camera& camera, SortMethod method, Shader& gatherShader, Shader& histogramShader,
	Shader& scanWorkgroupsShader, Shader& scanBinsShader, Shader& scatterShader)
{
	if (method == SortMethod::CPU)
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
	}
	else
	{
		gpuRadixSort(gatherShader, histogramShader, scanWorkgroupsShader, scanBinsShader, scatterShader);
	}


	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);
	drawCount = static_cast<uint32_t>(visibleIndices.size());
}

void SplatRenderer::gpuRadixSort(Shader& gatherShader, Shader& histogramShader,
	Shader& scanWorkgroupsShader, Shader& scanBinsShader, Shader& scatterShader)
{
	if (visibleIndices.empty()) return;

	uint32_t visibleCount = static_cast<uint32_t>(visibleIndices.size());
	uint32_t numWorkgroups = (visibleCount + ELEMENTS_PER_WORKGROUP - 1u) / ELEMENTS_PER_WORKGROUP;


	gatherShader.use();
	gatherShader.setUInt("uVisibleCount", visibleCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);
	glDispatchCompute((visibleCount + 255u) / 256u, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	constexpr uint32_t NUM_PASSES = 8u;
	for (uint32_t pass = 0; pass < NUM_PASSES; ++pass)
	{
		uint32_t shift = pass * 4u;
		bool srcIsA = (pass % 2u == 0u);
		unsigned int idsSrc = srcIsA ? indexSSBO : indexSSBO_B;
		unsigned int keysSrc = srcIsA ? keysSSBO_A : keysSSBO_B;
		unsigned int idsDst = srcIsA ? indexSSBO_B : indexSSBO;
		unsigned int keysDst = srcIsA ? keysSSBO_B : keysSSBO_A;

		histogramShader.use();
		histogramShader.setUInt("uCount", visibleCount);
		histogramShader.setUInt("uShift", shift);
		histogramShader.setUInt("uMaxWorkgroups", maxWorkgroups);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSrc);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
		glDispatchCompute(numWorkgroups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		scanWorkgroupsShader.use();
		scanWorkgroupsShader.setUInt("uNumWorkgroups", numWorkgroups);
		scanWorkgroupsShader.setUInt("uMaxWorkgroups", maxWorkgroups);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
		glDispatchCompute(RADIX_SIZE, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		scanBinsShader.use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		scatterShader.use();
		scatterShader.setUInt("uCount", visibleCount);
		scatterShader.setUInt("uShift", shift);
		scatterShader.setUInt("uMaxWorkgroups", maxWorkgroups);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, idsSrc);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSrc);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_B_SSBO_BINDING, idsDst);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_B_SSBO_BINDING, keysDst);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
		glDispatchCompute(numWorkgroups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}

bool SplatRenderer::debugValidateGatherStage(Shader& gatherShader)
{
	if (visibleIndices.empty()) return true;

	uint32_t visibleCount = static_cast<uint32_t>(visibleIndices.size());

	gatherShader.use();
	gatherShader.setUInt("uVisibleCount", visibleCount);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);

	glDispatchCompute((visibleCount + 255u) / 256u, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	std::vector<uint32_t> gpuIds(visibleCount);
	std::vector<uint32_t> gpuKeys(visibleCount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, gpuIds.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, keysSSBO_A);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, gpuKeys.data());

	std::vector<glm::vec4> preprocAll(3 * static_cast<size_t>(splatCount));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * preprocAll.size(), preprocAll.data());

	size_t idMismatches = 0;
	size_t keyMismatches = 0;
	for (uint32_t k = 0; k < visibleCount; ++k)
	{
		uint32_t expectedId = visibleIndices[k];
		if (gpuIds[k] != expectedId)
		{
			++idMismatches;
			continue;
		}

		float tz = preprocAll[3u * expectedId + 1u].w;
		uint32_t expectedKey = 0xFFFFFFFFu - std::bit_cast<uint32_t>(tz);

		if (gpuKeys[k] != expectedKey)
			++keyMismatches;
	}

	if (idMismatches > 0 || keyMismatches > 0)
	{
		std::cerr << "[radix_gather validation] id mismatches: " << idMismatches
			<< ", key mismatches: " << keyMismatches << " / " << visibleCount << "\n";
		return false;
	}
	return true;
}

bool SplatRenderer::debugValidateHistogramScanStage(Shader& gatherShader, Shader& histogramShader,
	Shader& scanWorkgroupsShader, Shader& scanBinsShader)
{
	if (visibleIndices.empty()) return true;

	uint32_t visibleCount = static_cast<uint32_t>(visibleIndices.size());
	uint32_t numWorkgroups = (visibleCount + ELEMENTS_PER_WORKGROUP - 1u) / ELEMENTS_PER_WORKGROUP;


	gatherShader.use();
	gatherShader.setUInt("uVisibleCount", visibleCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);
	glDispatchCompute((visibleCount + 255u) / 256u, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	histogramShader.use();
	histogramShader.setUInt("uCount", visibleCount);
	histogramShader.setUInt("uShift", 0u);
	histogramShader.setUInt("uMaxWorkgroups", maxWorkgroups);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
	glDispatchCompute(numWorkgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	std::vector<uint32_t> rawHist(static_cast<size_t>(RADIX_SIZE) * maxWorkgroups);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, wgHistogramsSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * rawHist.size(), rawHist.data());

	std::vector<uint32_t> keysA(visibleCount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, keysSSBO_A);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, keysA.data());

	std::array<uint64_t, RADIX_SIZE> cpuHist{};
	for (uint32_t k = 0; k < visibleCount; ++k)
		++cpuHist[keysA[k] & 0xFu];

	bool histogramOk = true;
	for (uint32_t d = 0; d < RADIX_SIZE; ++d)
	{
		uint64_t gpuSum = 0;
		for (uint32_t wg = 0; wg < numWorkgroups; ++wg)
			gpuSum += rawHist[static_cast<size_t>(d) * maxWorkgroups + wg];
		if (gpuSum != cpuHist[d])
		{
			std::cerr << "[radix_histogram validation] digit " << d << ": GPU=" << gpuSum << " CPU=" << cpuHist[d] << "\n";
			histogramOk = false;
		}
	}

	scanWorkgroupsShader.use();
	scanWorkgroupsShader.setUInt("uNumWorkgroups", numWorkgroups);
	scanWorkgroupsShader.setUInt("uMaxWorkgroups", maxWorkgroups);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
	glDispatchCompute(RADIX_SIZE, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	std::array<uint32_t, RADIX_SIZE> binTotal{};
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, binOffsetsSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * RADIX_SIZE, binTotal.data());

	uint64_t totalSum = 0;
	for (uint32_t d = 0; d < RADIX_SIZE; ++d) totalSum += binTotal[d];
	bool totalOk = (totalSum == visibleCount);
	if (!totalOk)
		std::cerr << "[radix_scan validation] sum(binTotal) = " << totalSum << ", expected " << visibleCount << "\n";


	scanBinsShader.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	std::array<uint32_t, RADIX_SIZE> binOffset{};
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, binOffsetsSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * RADIX_SIZE, binOffset.data());

	bool monotonicOk = (binOffset[0] == 0u);
	for (uint32_t d = 1; d < RADIX_SIZE && monotonicOk; ++d)
		if (binOffset[d] < binOffset[d - 1]) monotonicOk = false;
	if (!monotonicOk)
		std::cerr << "[radix_scan validation] binOffset[] not monotonic or binOffset[0] != 0\n";

	bool closureOk = (binOffset[RADIX_SIZE - 1] + binTotal[RADIX_SIZE - 1] == visibleCount);
	if (!closureOk)
		std::cerr << "[radix_scan validation] binOffset[15] + binTotal[15] = "
			<< (binOffset[RADIX_SIZE - 1] + binTotal[RADIX_SIZE - 1]) << ", expected " << visibleCount << "\n";

	return histogramOk && totalOk && monotonicOk && closureOk;
}

bool SplatRenderer::debugValidateScatterStage(Shader& gatherShader, Shader& histogramShader,
	Shader& scanWorkgroupsShader, Shader& scanBinsShader, Shader& scatterShader)
{
	if (visibleIndices.empty()) return true;

	uint32_t visibleCount = static_cast<uint32_t>(visibleIndices.size());
	uint32_t numWorkgroups = (visibleCount + ELEMENTS_PER_WORKGROUP - 1u) / ELEMENTS_PER_WORKGROUP;
	const uint32_t shift = 0u;


	gatherShader.use();
	gatherShader.setUInt("uVisibleCount", visibleCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREPROC_SSBO_BINDING, preprocSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_INDEX_SSBO_BINDING, visibleIndexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);
	glDispatchCompute((visibleCount + 255u) / 256u, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);


	histogramShader.use();
	histogramShader.setUInt("uCount", visibleCount);
	histogramShader.setUInt("uShift", shift);
	histogramShader.setUInt("uMaxWorkgroups", maxWorkgroups);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
	glDispatchCompute(numWorkgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);


	scanWorkgroupsShader.use();
	scanWorkgroupsShader.setUInt("uNumWorkgroups", numWorkgroups);
	scanWorkgroupsShader.setUInt("uMaxWorkgroups", maxWorkgroups);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
	glDispatchCompute(RADIX_SIZE, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);


	scanBinsShader.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	scatterShader.use();
	scatterShader.setUInt("uCount", visibleCount);
	scatterShader.setUInt("uShift", shift);
	scatterShader.setUInt("uMaxWorkgroups", maxWorkgroups);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, indexSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_A_SSBO_BINDING, keysSSBO_A);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_B_SSBO_BINDING, indexSSBO_B);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, KEYS_B_SSBO_BINDING, keysSSBO_B);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WG_HISTOGRAMS_SSBO_BINDING, wgHistogramsSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIN_OFFSETS_SSBO_BINDING, binOffsetsSSBO);
	glDispatchCompute(numWorkgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

	std::vector<uint32_t> outIds(visibleCount);
	std::vector<uint32_t> outKeys(visibleCount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO_B);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, outIds.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, keysSSBO_B);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, outKeys.data());

	bool monotonicOk = true;
	for (uint32_t k = 1; k < visibleCount; ++k)
	{
		uint32_t prevDigit = (outKeys[k - 1] >> shift) & 0xFu;
		uint32_t curDigit = (outKeys[k] >> shift) & 0xFu;
		if (curDigit < prevDigit) { monotonicOk = false; break; }
	}
	if (!monotonicOk)
		std::cerr << "[radix_scatter validation] output digits not monotonic\n";

	std::vector<uint32_t> inputRank(splatCount, 0u);
	for (uint32_t k = 0; k < visibleCount; ++k)
		inputRank[visibleIndices[k]] = k;

	bool stableOk = true;
	for (uint32_t k = 1; k < visibleCount; ++k)
	{
		uint32_t prevDigit = (outKeys[k - 1] >> shift) & 0xFu;
		uint32_t curDigit = (outKeys[k] >> shift) & 0xFu;
		if (curDigit == prevDigit && inputRank[outIds[k]] < inputRank[outIds[k - 1]])
		{
			stableOk = false;
			break;
		}
	}
	if (!stableOk)
		std::cerr << "[radix_scatter validation] stability violated within a digit bucket\n";

	std::vector<uint32_t> sortedOut = outIds;
	std::vector<uint32_t> sortedIn = visibleIndices;
	std::sort(sortedOut.begin(), sortedOut.end());
	std::sort(sortedIn.begin(), sortedIn.end());
	bool permutationOk = (sortedOut == sortedIn);
	if (!permutationOk)
		std::cerr << "[radix_scatter validation] output is not a permutation of the input ids\n";

	return monotonicOk && stableOk && permutationOk;
}

bool SplatRenderer::debugCompareSortMethods(Camera& camera, Shader& gatherShader, Shader& histogramShader,
	Shader& scanWorkgroupsShader, Shader& scanBinsShader, Shader& scatterShader)
{
	if (visibleIndices.empty()) return true;

	uint32_t visibleCount = static_cast<uint32_t>(visibleIndices.size());

	glm::mat4 MV = camera.getViewMatrix() * model;
	std::vector<float> cpuDepths(splatCount);
	for (uint32_t id : visibleIndices)
	{
		glm::vec3 viewPos = glm::vec3(MV * glm::vec4(splatsVector[id].position[0], splatsVector[id].position[1], splatsVector[id].position[2], 1.0f));
		cpuDepths[id] = -viewPos.z;
	}
	std::vector<uint32_t> cpuSorted = visibleIndices;
	std::sort(std::execution::par_unseq, cpuSorted.begin(), cpuSorted.end(), [&cpuDepths](uint32_t a, uint32_t b) {
		return cpuDepths[a] > cpuDepths[b];
		});

	gpuRadixSort(gatherShader, histogramShader, scanWorkgroupsShader, scanBinsShader, scatterShader);

	std::vector<uint32_t> gpuSorted(visibleCount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * visibleCount, gpuSorted.data());

	std::vector<glm::vec4> preprocAll(3 * static_cast<size_t>(splatCount));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * preprocAll.size(), preprocAll.data());

	size_t mismatches = 0;
	for (uint32_t k = 0; k < visibleCount; ++k)
	{
		if (cpuSorted[k] == gpuSorted[k]) continue;
		float tzCpu = preprocAll[3u * cpuSorted[k] + 1u].w;
		float tzGpu = preprocAll[3u * gpuSorted[k] + 1u].w;
		uint32_t bitsCpu = std::bit_cast<uint32_t>(tzCpu);
		uint32_t bitsGpu = std::bit_cast<uint32_t>(tzGpu);
		uint32_t ulpDist = bitsCpu > bitsGpu ? bitsCpu - bitsGpu : bitsGpu - bitsCpu;
		if (ulpDist > DEPTH_ULP_TOLERANCE)
			++mismatches;
	}

	if (mismatches > 0)
	{
		std::cerr << "[CPU vs GPU sort] " << mismatches << " / " << visibleCount
			<< " positions differ at non-tied depth\n";
		return false;
	}
	return true;
}

bool SplatRenderer::debugCheckSortMonotonic() const
{
	if (drawCount == 0) return true;

	std::vector<uint32_t> ids(drawCount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * drawCount, ids.data());

	std::vector<glm::vec4> preprocAll(3 * static_cast<size_t>(splatCount));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * preprocAll.size(), preprocAll.data());

	for (uint32_t k = 1; k < drawCount; ++k)
	{
		float tzPrev = preprocAll[3u * ids[k - 1] + 1u].w;
		float tzCur = preprocAll[3u * ids[k] + 1u].w;
		if (tzCur <= tzPrev) continue;


		uint32_t bitsPrev = std::bit_cast<uint32_t>(tzPrev);
		uint32_t bitsCur = std::bit_cast<uint32_t>(tzCur);
		if (bitsCur - bitsPrev > DEPTH_ULP_TOLERANCE)
		{
			std::cerr << "[sort monotonic check] violation at k=" << k
				<< ": tz[" << (k - 1) << "]=" << tzPrev << " < tz[" << k << "]=" << tzCur << "\n";
			return false;
		}
	}
	return true;
}

void SplatRenderer::preprocess(Shader& computeShader, Camera& camera, const glm::vec2& screenSize, const RenderParams& params)
{
	const glm::mat4& view = camera.getViewMatrix();
	const glm::mat4& proj = camera.getProjMatrix();
	glm::vec3 camPos = camera.getPosition();


	if (hasValidPreprocess && view == lastPreprocessView && proj == lastPreprocessProj &&
		camPos == lastPreprocessCamPos && screenSize == lastPreprocessScreenSize &&
		params == lastPreprocessParams)
	{
		return;
	}
	lastPreprocessView = view;
	lastPreprocessProj = proj;
	lastPreprocessCamPos = camPos;
	lastPreprocessScreenSize = screenSize;
	lastPreprocessParams = params;
	hasValidPreprocess = true;

	computeShader.use();
	computeShader.setMat4("uView", view);
	computeShader.setMat4("uProj", proj);
	computeShader.setMat4("uModel", model);
	computeShader.setVec3("uCamPos", camPos);
	computeShader.setVec2("uScreenSize", screenSize);
	computeShader.setUInt("uCount", splatCount);
	computeShader.setFloat("uMinOpacity", params.minOpacity);
	computeShader.setFloat("uScaleMultiplier", params.scaleMultiplier);
	computeShader.setFloat("uDilation", params.dilation);
	computeShader.setFloat("uMaxRadiusPx", params.maxRadiusPx);
	computeShader.setInt("uSHDegree", params.shDegree);

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

size_t SplatRenderer::getEstimatedVramBytes() const
{
	size_t bytes = 0;
	bytes += sizeof(Splat) * splatCount;         // splatSSBO
	bytes += sizeof(uint32_t) * splatCount;       // indexSSBO
	bytes += sizeof(glm::vec4) * 3 * splatCount;  // preprocSSBO
	bytes += sizeof(GLuint);                      // visibleCountSSBO
	bytes += sizeof(uint32_t) * splatCount;       // visibleIndexSSBO
	bytes += sizeof(uint32_t) * splatCount;       // keysSSBO_A
	bytes += sizeof(uint32_t) * splatCount;       // keysSSBO_B
	bytes += sizeof(uint32_t) * splatCount;       // indexSSBO_B
	bytes += sizeof(uint32_t) * RADIX_SIZE * maxWorkgroups; // wgHistogramsSSBO
	bytes += sizeof(uint32_t) * RADIX_SIZE;       // binOffsetsSSBO
	return bytes;
}

uint32_t SplatRenderer::getSplatCount() const
{
	return splatCount;
}

uint32_t SplatRenderer::getDrawCount() const
{
	return drawCount;
}

const std::vector<Splat>& SplatRenderer::getSplats() const
{
	return splatsVector;
}

const std::vector<uint32_t>& SplatRenderer::getVisibleIndices() const
{
	return visibleIndices;
}

std::vector<glm::vec2> SplatRenderer::fetchVisibleScreenExtents() const
{
	std::vector<glm::vec2> extents;
	extents.reserve(visibleIndices.size());
	if (visibleIndices.empty()) return extents;

	std::vector<glm::vec4> preprocData(3 * static_cast<size_t>(splatCount));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preprocSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * preprocData.size(), preprocData.data());

	for (uint32_t idx : visibleIndices)
	{
		const glm::vec4& centerExtent = preprocData[3u * idx];
		extents.emplace_back(centerExtent.z, centerExtent.w);
	}
	return extents;
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
	glDeleteBuffers(1, &keysSSBO_A);
	glDeleteBuffers(1, &keysSSBO_B);
	glDeleteBuffers(1, &indexSSBO_B);
	glDeleteBuffers(1, &wgHistogramsSSBO);
	glDeleteBuffers(1, &binOffsetsSSBO);
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
	glGenBuffers(1, &keysSSBO_A);
	glGenBuffers(1, &keysSSBO_B);
	glGenBuffers(1, &indexSSBO_B);
	glGenBuffers(1, &wgHistogramsSSBO);
	glGenBuffers(1, &binOffsetsSSBO);
}
