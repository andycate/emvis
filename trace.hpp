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
    vector<float> b;
    vector<vec3> points;
public:
    void generate_path(vector<Loop> loops, float step, int segments, bool forward) {
        for(int i = 0; i < segments; i++) {
            vec3 bvec(0, 0, 0);
            for(int l = 0; l < loops.size(); l++) {
                bvec += loops[l].biot_savart(points[i]);
            }
            float bfield = length(bvec);
            vec3 delta = normalize(bvec) * step;
            points.push_back(forward ? points[i] + delta : points[i] - delta);
            b.push_back(length(bvec));
        }
    }
    Trace(vec3 start) { points.push_back(start); }
};
