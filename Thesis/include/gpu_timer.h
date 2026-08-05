#pragma once

class GpuTimer
{
public:
	void begin();
	void end();
	bool tryGetResultMs(float& outMs);

	~GpuTimer();
	GpuTimer() = default;
	GpuTimer(const GpuTimer&) = delete;
	GpuTimer& operator=(const GpuTimer&) = delete;

private:
	void initGL();

	unsigned int startQueries[2] = { 0, 0 };
	unsigned int endQueries[2] = { 0, 0 };
	bool pending[2] = { false, false };
	int frameIndex = 0;
	bool initialized = false;
};
