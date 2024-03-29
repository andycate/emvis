#include "loadShader.hpp"
#include "line.hpp"
#include "loop.hpp"
#include "trace.hpp"
#include "PCStellarator.hpp"

#define GLFW_INCLUDE_GLCOREARB
// #include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#include <boost/functional/hash.hpp>

#include <ctime>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <math.h>
#include <limits>

/* structures */
struct Point
{
    float x, y;
    Point()
    {
        x = 0.0;
        y = 0.0;
    }

    Point(float _x, float _y) : x(_x), y(_y) {}
};

struct View
{
    float fov, vert, horiz, transx, transy, zoom;
    View()
    {
        fov = 45.0;
        vert = 0.0;
        horiz = 0.0;
        transx = 0.0;
        transy = 0.0;
        zoom = 5.0;
    }
};

/* global variables */
int width = 800;
int height = 600;
bool ortho_view = false;
Point *lastMousePt = new Point;
Point *delta = new Point;
int drag = 0;
float closest_dist = std::numeric_limits<float>::max();
int closest_index = -1;
int closest_face = 0;
View *view = new View;
GLuint vertexbuffer;
GLuint colorbuffer;
glm::mat4 Projection = glm::perspective(glm::radians(view->fov), (float)width / (float)height, 0.1f, 100.0f);

std::vector<GLfloat*> g_compiled_vertex_data_loops;
std::vector<GLfloat*> g_compiled_color_data_loops;
std::vector<int> compiled_vertices_loops;

std::vector<GLfloat*> g_compiled_vertex_data_traces;
std::vector<GLfloat*> g_compiled_color_data_traces;
std::vector<int> compiled_vertices_traces;

void compile_em()
{
    std::vector<Loop> loops;
    std::vector<Trace> traces;

    pcs::compile_em_bottle(&loops, &traces);

    // GL Processing Below
    GLfloat *vtmp, *ctmp;
    for(int i = 0; i < loops.size(); i++) {
        int cvert = loops[i].gl_generate(&vtmp, &ctmp);
        g_compiled_vertex_data_loops.push_back(vtmp);
        g_compiled_color_data_loops.push_back(ctmp);
        compiled_vertices_loops.push_back(cvert);
    }
    for(int i = 0; i < traces.size(); i++) {
        int cvert = traces[i].gl_generate(&vtmp, &ctmp);
        g_compiled_vertex_data_traces.push_back(vtmp);
        g_compiled_color_data_traces.push_back(ctmp);
        compiled_vertices_traces.push_back(cvert);
    }
}

/* input callbacks */
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            drag = 1;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastMousePt->x = xpos;
            lastMousePt->y = ypos;
            delta->x = 0;
            delta->y = 0;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            drag = 2;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastMousePt->x = xpos;
            lastMousePt->y = ypos;
            delta->x = 0;
            delta->y = 0;
        }
    }
    else if (action == GLFW_RELEASE)
    {
        drag = 0;
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    delta->x = xpos - lastMousePt->x;
    delta->y = ypos - lastMousePt->y;
    lastMousePt->x = xpos;
    lastMousePt->y = ypos;
    if (drag == 1)
    {
        view->horiz -= delta->x * 0.01;
        view->vert -= delta->y * 0.01;
    }
    else if (drag == 2)
    {
        view->transx += delta->x * 0.01;
        view->transy += delta->y * 0.01;
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    view->zoom *= pow(1.01, yoffset);
    view->fov += xoffset * 0.1;
    Projection = glm::perspective(glm::radians(view->fov), (float)width / (float)height, 0.1f, 100.0f);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_F && action == GLFW_RELEASE) {
        view->horiz = 0;
        view->vert = 0;
    } else if(key == GLFW_KEY_O && action == GLFW_RELEASE) {
        ortho_view = !ortho_view;
    } else if(key == GLFW_KEY_UP && action == GLFW_RELEASE) {
        view->vert += glm::two_pi<float>()/48.0;
    } else if(key == GLFW_KEY_DOWN && action == GLFW_RELEASE) {
        view->vert -= glm::two_pi<float>()/48.0;
    } else if(key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
        view->horiz += glm::two_pi<float>()/48.0;
    } else if(key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
        view->horiz -= glm::two_pi<float>()/48.0;
    }
}

void window_size_callback(GLFWwindow *window, int w, int h)
{
    width = w;
    height = h;
    Projection = glm::perspective(glm::radians(view->fov), (float)width / (float)height, 0.1f, 100.0f);
}

glm::mat4 get_mvp(float zoom, float y_angle, float vert_angle, float transx, float transy)
{

    // Or, for an ortho camera :
    if(ortho_view) {
        float aspect = (float)width / (float)height;
        Projection = glm::ortho(-zoom*aspect,zoom*aspect,-zoom,zoom,0.0f,100.0f); // In world coordinates
    } else {
        Projection = glm::perspective(glm::radians(view->fov), (float)width / (float)height, 0.1f, 100.0f);
    }

    glm::vec3 cam = glm::vec3(glm::rotate(glm::mat4(1.0f), y_angle, glm::vec3(0, 1, 0)) *
                              glm::rotate(glm::mat4(1.0f), vert_angle, glm::vec3(1, 0, 0)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(zoom, zoom, zoom)) *
                              glm::vec4(0, 0, 1, 1));
    glm::vec3 look = glm::vec3(0, 0, 0); // glm::cross(cam, glm::vec3(0, 1, 0)) * transx;

    // Camera matrix
    glm::mat4 View = glm::lookAt(
        cam,               // Camera is at (4,3,3), in World Space
        look,              // and looks at the origin
        glm::vec3(0, 1, 0) // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around
    return mvp;
}

int main(int argc, char **argv)
{
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

#ifdef __APPLE__
    /* We need to explicitly ask for a 4.5 context on OS X */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width, height, "3D EM Vis", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Setup input handling */
    // glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    // configure for display
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
    glGenBuffers(1, &vertexbuffer);
    glGenBuffers(1, &colorbuffer);

    // generate axis markers
    GLfloat axis_vertices[] = {
        0.0f, 0.0f, 0.0f, // x
        0.1f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, // y
        0.0f, 0.1f, 0.0f,
        0.0f, 0.0f, 0.0f, // z
        0.0f, 0.0f, 0.1f};

    GLfloat axis_colors[] = {
        1.0f, 0.0f, 0.0f, // x
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, // y
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, // z
        0.0f, 0.0f, 1.0f};

    compile_em();

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");
    glUseProgram(programID);

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glm::mat4 mvp = get_mvp(view->zoom, view->horiz, view->vert, view->transx, view->transy);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers

        // draw the axis markers
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), &axis_vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(
            0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void *)0 // array buffer offset
        );
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), &axis_colors, GL_STATIC_DRAW);
        glVertexAttribPointer(
            1,        // attribute. No particular reason for 1, but must match the layout in the shader.
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void *)0 // array buffer offset
        );
        glDrawArrays(GL_LINES, 0, 6); // Starting from vertex 0; 3 vertices total -> 1 triangle
        glDisableVertexAttribArray(0);

        for(int i = 0; i < g_compiled_vertex_data_loops.size(); i++) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
            glBufferData(GL_ARRAY_BUFFER, compiled_vertices_loops[i] * 3 * sizeof(GLfloat), g_compiled_vertex_data_loops[i], GL_STATIC_DRAW);
            glVertexAttribPointer(
                0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
                3,        // size
                GL_FLOAT, // type
                GL_FALSE, // normalized?
                0,        // stride
                (void *)0 // array buffer offset
            );
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
            glBufferData(GL_ARRAY_BUFFER, compiled_vertices_loops[i] * 3 * sizeof(GLfloat), g_compiled_color_data_loops[i], GL_STATIC_DRAW);
            glVertexAttribPointer(
                1,        // attribute. No particular reason for 1, but must match the layout in the shader.
                3,        // size
                GL_FLOAT, // type
                GL_FALSE, // normalized?
                0,        // stride
                (void *)0 // array buffer offset
            );
            glDrawArrays(GL_LINE_LOOP, 0, compiled_vertices_loops[i]); // Starting from vertex 0; 3 vertices total -> 1 triangle
            glDisableVertexAttribArray(0);
        }
        
        for(int i = 0; i < g_compiled_vertex_data_traces.size(); i++) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
            glBufferData(GL_ARRAY_BUFFER, compiled_vertices_traces[i] * 3 * sizeof(GLfloat), g_compiled_vertex_data_traces[i], GL_STATIC_DRAW);
            glVertexAttribPointer(
                0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
                3,        // size
                GL_FLOAT, // type
                GL_FALSE, // normalized?
                0,        // stride
                (void *)0 // array buffer offset
            );
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
            glBufferData(GL_ARRAY_BUFFER, compiled_vertices_traces[i] * 3 * sizeof(GLfloat), g_compiled_color_data_traces[i], GL_STATIC_DRAW);
            glVertexAttribPointer(
                1,        // attribute. No particular reason for 1, but must match the layout in the shader.
                3,        // size
                GL_FLOAT, // type
                GL_FALSE, // normalized?
                0,        // stride
                (void *)0 // array buffer offset
            );
            glDrawArrays(GL_LINE_STRIP, 0, compiled_vertices_traces[i]); // Starting from vertex 0; 3 vertices total -> 1 triangle
            glDisableVertexAttribArray(0);
        }

        glFlush();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwWaitEvents();
    }

    glfwTerminate();
    return 0;
}
