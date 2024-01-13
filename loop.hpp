#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <vector>

// using namespace std;
using namespace glm;

class Loop {
private:
    static constexpr float mu_0 = 4 * pi<float>() * 1e-7; // N/A^2
    float current; // amps
    std::vector<vec3> points;
public:
    static const Loop create_circle(vec3 center, vec3 norm, float radius, float current, int segments) {
        Loop result;
        vec3 perp1;
        vec3 perp2;
        norm = normalize(norm);
        if (abs(norm.x) < 0.5) {
            perp1 = normalize(cross(norm, vec3(1, 0, 0)));
        } else {
            perp1 = normalize(cross(norm, vec3(0, 1, 0)));
        }
        perp2 = cross(perp1, norm);
        for(int i = 0; i < segments; i++) {
            vec3 p = center + perp1 * cos(two_pi<float>() * i / segments) * radius + perp2 * sin(two_pi<float>() * i / segments) * radius;
            result.add_point(p);
            std::cout << p.x << ":" << p.y << ":" << p.z << std::endl;
        }
        result.set_current(current);
        return result;
    }
    int gl_generate(GLfloat **vectors, GLfloat **colors) {
        *vectors = new GLfloat[3 * points.size()];
        *colors = new GLfloat[3 * points.size()];
        for(int i = 0; i < points.size(); i++) {
            (*vectors)[i*3+0] = points[i].x;
            (*vectors)[i*3+1] = points[i].y;
            (*vectors)[i*3+2] = points[i].z;
            (*colors)[i*3+0] = 1.0;
            (*colors)[i*3+1] = 1.0;
            (*colors)[i*3+2] = 1.0;
        }
        return points.size();
    }
    void add_point(vec3 point) { points.push_back(point); }
    void set_current(float c) { current = c; }
    vec3 biot_savart(vec3 r) {
        vec3 result(0, 0, 0);
        for(int i = 0; i < points.size(); i++) {
            result += cross(points[(i+1)%points.size()]-points[i], normalize(r - points[i])) / length(r - points[i]) / length(r - points[i]);
        }
        return result * mu_0 * current / 4.0f / pi<float>();
    }
    Loop() {}
};
