#include <gpu_timer.h>
#include <glad/gl.h>

void GpuTimer::initGL()
{
	glGenQueries(2, startQueries);
	glGenQueries(2, endQueries);
	initialized = true;
}

void GpuTimer::begin()
{
	if (!initialized) initGL();
	glQueryCounter(startQueries[frameIndex % 2], GL_TIMESTAMP);
}

void GpuTimer::end()
{
	glQueryCounter(endQueries[frameIndex % 2], GL_TIMESTAMP);
	pending[frameIndex % 2] = true;
	frameIndex++;
}

bool GpuTimer::tryGetResultMs(float& outMs)
{
	int slot = frameIndex % 2;
	if (!pending[slot]) return false;

	GLint available = 0;
	glGetQueryObjectiv(endQueries[slot], GL_QUERY_RESULT_AVAILABLE, &available);
	if (!available) return false;

	GLuint64 startTime = 0, endTime = 0;
	glGetQueryObjectui64v(startQueries[slot], GL_QUERY_RESULT, &startTime);
	glGetQueryObjectui64v(endQueries[slot], GL_QUERY_RESULT, &endTime);
	outMs = static_cast<float>(endTime - startTime) / 1000000.0f;
	pending[slot] = false;
	return true;
}

float GpuTimer::endAndWaitMs()
{
	int slot = frameIndex % 2;
	glQueryCounter(endQueries[slot], GL_TIMESTAMP);

	GLuint64 startTime = 0, endTime = 0;
	glGetQueryObjectui64v(startQueries[slot], GL_QUERY_RESULT, &startTime); // blocks until available
	glGetQueryObjectui64v(endQueries[slot], GL_QUERY_RESULT, &endTime);
	pending[slot] = false;
	frameIndex++;

	return static_cast<float>(endTime - startTime) / 1000000.0f;
}

GpuTimer::~GpuTimer()
{
	if (initialized)
	{
		glDeleteQueries(2, startQueries);
		glDeleteQueries(2, endQueries);
	}
}
