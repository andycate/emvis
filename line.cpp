#include "line.hpp"

int Line::append_vertices(GLfloat* varr, size_t pos) {
    varr[pos+0] = start.x;
    varr[pos+1] = start.y;
    varr[pos+2] = start.z;
    varr[pos+3] = end.x;
    varr[pos+4] = end.y;
    varr[pos+5] = end.z;

    return 6;
}

int Line::append_colors(GLfloat* carr, size_t pos) {
    carr[pos+0] = 1;
    carr[pos+1] = 1;
    carr[pos+2] = 1;
    carr[pos+3] = 1;
    carr[pos+4] = 1;
    carr[pos+5] = 1;

    return 6;
}

glm::vec3 Line::stroke() {
    return end - start;
}

Line::Line(glm::vec3 start, glm::vec3 end): start(start), end(end) {
}

Line::Line(): start(0, 0, 0), end(0, 0, 0) {};
