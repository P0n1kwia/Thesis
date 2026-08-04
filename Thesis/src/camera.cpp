#include "camera.h"

Camera::Camera(int viewportW, int viewportH, const CameraConfig& config)
{
	aspect = static_cast<float>(viewportW) / viewportH;
	nearPlane = config.nearPlane;
	farPlane = config.farPlane;
	fovY = config.fovY;
	target = config.target;
	radius = config.radius;
	yaw = config.yaw;
	pitch = config.pitch;
	recompute();
}
const glm::mat4& Camera::getViewMatrix() 
{
	if (dirty)
	{
		recompute();
	}
	return viewMatrix;
}

const glm::mat4& Camera::getProjMatrix() const { if (dirty) recompute(); return projMatrix; }

glm::vec3 Camera::getPosition() const { if (dirty) recompute(); return eye; }


glm::vec3 Camera::getForward() const { if (dirty) recompute(); return glm::normalize(target - eye); }

float Camera::getFovY() const
{
	return fovY;
}

float Camera::getAspect() const
{
	return aspect;
}

float Camera::getRadius() const
{
	return radius;
}

void Camera::orbit(float deltaYaw, float deltaPitch, float multi)
{
	yaw += deltaYaw * multi;
	pitch = glm::clamp(pitch + deltaPitch * multi, glm::radians(-89.0f), glm::radians(89.0f));
	dirty = true;
}

void Camera::zoom(float delta, float sensitivity)
{
	radius -= delta * radius * sensitivity;
	if (radius < 0.1f) radius = 0.1f;
	dirty = true;

}

void Camera::pan(float dx, float dy)
{
	glm::vec3 forward = getForward();
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), forward));
	glm::vec3 up = glm::normalize(glm::cross(forward, right));
	glm::vec3 offset = (right * dx) + (up * dy);
	target += offset;


	dirty = true;

}

void Camera::onViewportResize(int w, int h)
{
	aspect = static_cast<float>(w) / h;
	dirty = true;
}

CameraConfig Camera::getConfig() const
{
	CameraConfig cfg;
	cfg.fovY = fovY;
	cfg.nearPlane = nearPlane;
	cfg.farPlane = farPlane;
	cfg.target = target;
	cfg.radius = radius;
	cfg.yaw = yaw;
	cfg.pitch = pitch;
	return cfg;
}

void Camera::applyConfig(const CameraConfig& cfg)
{
	fovY = cfg.fovY;
	nearPlane = cfg.nearPlane;
	farPlane = cfg.farPlane;
	target = cfg.target;
	radius = cfg.radius;
	yaw = cfg.yaw;
	pitch = cfg.pitch;
	dirty = true;
}

bool Camera::needsSort() const
{
	return glm::distance(lastSortPos, getPosition()) > 0.002f * radius;
}

void Camera::onSortComplete()
{
	lastSortPos = getPosition();
}

void Camera::recompute() const
{
	float x = radius * glm::cos(yaw) * glm::cos(pitch);
	float y = radius * glm::sin(pitch);
	float z = radius * glm::sin(yaw) * glm::cos(pitch);
	eye = target + glm::vec3(x, y, z);
	viewMatrix = glm::lookAt(eye, target, glm::vec3(0.f, 1.f, 0.f));
	projMatrix = glm::perspective(fovY, aspect, nearPlane, farPlane);
	dirty = false;
}
