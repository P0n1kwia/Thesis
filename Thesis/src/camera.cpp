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
