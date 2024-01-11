#pragma once

#include <glm/glm.hpp>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

class Line {
    private:
        glm::vec3 start;
        glm::vec3 end;

    public:
        int append_vertices(GLfloat* varr, size_t pos);
        int append_colors(GLfloat* carr, size_t pos);
        Line(glm::vec3 start, glm::vec3 end);
        Line();
};
