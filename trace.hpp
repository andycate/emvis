#pragma once

#include "loop.hpp"

#include <glm/glm.hpp>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <vector>

using namespace std;
using namespace glm;

class Trace {
private:
    static vec3 hsvToRgb(vec3 hsv) {
        double h = hsv.x, s = hsv.y, v = hsv.z;
        double r, g, b;

        int i = static_cast<int>(h * 6);
        double f = h * 6 - i;
        double p = v * (1 - s);
        double q = v * (1 - f * s);
        double t = v * (1 - (1 - f) * s);

        switch (i % 6) {
            case 0: r = v, g = t, b = p; break;
            case 1: r = q, g = v, b = p; break;
            case 2: r = p, g = v, b = t; break;
            case 3: r = p, g = q, b = v; break;
            case 4: r = t, g = p, b = v; break;
            case 5: r = v, g = p, b = q; break;
        }

        return vec3(r, g, b);
    }
    vector<float> b;
    vector<vec3> points;
    bool forward;
public:
    void generate_path(vector<Loop> *loops, float step, int segments) {
        for(int i = 0; i < segments; i++) {
            vec3 bvec(0, 0, 0);
            for(int l = 0; l < loops->size(); l++) {
                bvec += (*loops)[l].biot_savart(points[i]);
            }
            float bfield = length(bvec);
            vec3 delta = normalize(bvec) * step;
            points.push_back(forward ? points[i] + delta : points[i] - delta);
            b.push_back(length(bvec));
        }
    }
    void generate_dynamic_path(vector<Loop> *loops, int segments) {
        for(int i = 0; i < segments; i++) {
            vec3 bvec(0, 0, 0);
            for(int l = 0; l < loops->size(); l++) {
                bvec += (*loops)[l].biot_savart(points[i]);
            }
            float bfield = length(bvec);
            vec3 delta = normalize(bvec) * ((i == 0) ? 0.01f : std::abs(dot(normalize(bvec), points[i]-points[i-1])));
            points.push_back(forward ? points[i] + delta : points[i] - delta);
            b.push_back(length(bvec));
        }
    }
    int gl_generate(GLfloat **vectors, GLfloat **colors) {
        *vectors = new GLfloat[3 * points.size()];
        *colors = new GLfloat[3 * points.size()];
        for(int i = 0; i < points.size(); i++) {
            (*vectors)[i*3+0] = points[i].x;
            (*vectors)[i*3+1] = points[i].y;
            (*vectors)[i*3+2] = points[i].z;
            vec3 rgb = hsvToRgb(vec3(glm::clamp(log(b[i]*10000000)/4, 0.0f, 1.0f), 1.0f, 1.0f));
            (*colors)[i*3+0] = rgb.x;
            (*colors)[i*3+1] = rgb.y;
            (*colors)[i*3+2] = rgb.z;
        }
        return points.size();
    }
    Trace(vec3 start, bool f): forward(f) { points.push_back(start); }
};
