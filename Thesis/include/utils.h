#pragma once
#include <array>
#include <glm/glm.hpp>

// Extracts the 6 world-space frustum planes from a combined view-projection
// matrix using the Gribb-Hartmann method. Each plane is (nx, ny, nz, d) with
// an inward-facing normal, normalized so dot(plane.xyz, p) + plane.w is the
// signed distance from point p to the plane, in world units.
inline std::array<glm::vec4, 6> extractFrustumPlanes(const glm::mat4& m)
{
	auto row = [&](int i) {return glm::vec4{ m[0][i], m[1][i], m[2][i], m[3][i] };};
	glm::vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
	std::array<glm::vec4, 6> p = {
		r0 + r3, //left
		r3 - r0,//right
		r3 + r1,//down
		r3 - r1,//up
		r3 + r2,//near
		r3 - r2,//far
	};

	for (auto& q : p) q /= glm::length(glm::vec3(q));
	return p;
}