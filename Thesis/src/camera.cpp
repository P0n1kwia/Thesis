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

const glm::mat4& Camera::getProjMatrix() const
{
	return projMatrix;
}

glm::vec3 Camera::getPosition() const
{
	return eye;
}

glm::vec3 Camera::getForward() const
{
	return glm::normalize(target - eye);
}

float Camera::getFovY() const
{
	return fovY;
}

float Camera::getAspect() const
{
	return aspect;
}

void Camera::orbit(float deltYaw, float deltaPitch, float multi)
{
	yaw += deltYaw * multi;
	pitch += deltaPitch * multi;
	dirty = true;
}

void Camera::zoom(float delta, float sensitivity = 0.5f)
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
	eye += offset;
	target += offset;


	dirty = true;

}

void Camera::onViewportResize(int w, int h)
{
	aspect = static_cast<float>(w) / h;
	dirty = true;
}

bool Camera::needsSort() const
{
	float dist = glm::distance(lastSortPos, getPosition());
	return dist > 1e-2f;
}

void Camera::onSortComplete()
{
	lastSortPos = getPosition();
}

void Camera::recompute()
{
	pitch = glm::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));
	float x = radius * glm::cos(yaw) * glm::cos(pitch) ;
	float y = radius * glm::sin(pitch) ;
	float z = radius * glm::sin(yaw)  * glm::cos(pitch);
	eye = target + glm::vec3(x, y, z);

	viewMatrix = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
	projMatrix = glm::perspective(fovY, aspect, nearPlane, farPlane);


	dirty = false;
}
