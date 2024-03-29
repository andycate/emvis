#pragma once

#include <glm/glm.hpp>

bool rayIntersectsTriangle(glm::vec3 ray_origin, glm::vec3 ray_vector, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& out_dist);
