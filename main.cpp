#include "loadShader.hpp"
#include "line.hpp"

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

GLfloat *g_compiled_vertex_data = new GLfloat[0];
GLfloat *g_compiled_color_data = new GLfloat[0];
int compiled_length = 0;
int compiled_vertices = 0;

glm::vec3 pseudo_bs(Line *wire, int wires, int len, glm::vec3 r) {
    glm::vec3 result(0, 0, 0);
    for(int w = 0; w < wires; w++) {
        for(int i = 0; i < len; i++) {
            int index = w * len + i;
            result += glm::cross(wire[index].stroke(), glm::normalize(r - wire[index].start)) / glm::length(r - wire[index].start) / glm::length(r - wire[index].start);
        }
    }
    return result;
}

glm::vec3 hsvToRgb(glm::vec3 hsv)
{
    double      hh, p, q, t, ff;
    long        i;
    glm::vec3         out;

    if(hsv.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.x = hsv.z;
        out.y = hsv.z;
        out.z = hsv.z;
        return out;
    }
    hh = hsv.x;
    if(hh >= 1.0) hh = 0.0;
    hh *= 6;
    i = (long)hh;
    ff = hh - i;
    p = hsv.z * (1.0 - hsv.y);
    q = hsv.z * (1.0 - (hsv.y * ff));
    t = hsv.z * (1.0 - (hsv.y * (1.0 - ff)));

    switch(i) {
    case 0:
        out.x = hsv.z;
        out.y = t;
        out.z = p;
        break;
    case 1:
        out.x = q;
        out.y = hsv.z;
        out.z = p;
        break;
    case 2:
        out.x = p;
        out.y = hsv.z;
        out.z = t;
        break;

    case 3:
        out.x = p;
        out.y = q;
        out.z = hsv.z;
        break;
    case 4:
        out.x = t;
        out.y = p;
        out.z = hsv.z;
        break;
    case 5:
    default:
        out.x = hsv.z;
        out.y = p;
        out.z = q;
        break;
    }
    return out;     
}

void compile_em()
{
    // Wires
    int wires = 4;
    int segments = 80;
    Line wire[wires][segments];
    for (int i = 0; i < segments; i++)
    {
        wire[0][i] = Line(glm::vec3(glm::cos(glm::two_pi<float>() * i / segments)-1.4, glm::sin(glm::two_pi<float>() * i / segments), 0.0), glm::vec3(glm::cos(glm::two_pi<float>() * (i+1) / segments)-1.4, glm::sin(glm::two_pi<float>() * (i+1) / segments), 0.0));
        wire[1][i] = Line(glm::vec3(glm::cos(glm::two_pi<float>() * i / segments)+1.4, -glm::sin(glm::two_pi<float>() * i / segments), 0.0), glm::vec3(glm::cos(glm::two_pi<float>() * (i+1) / segments)+1.4, -glm::sin(glm::two_pi<float>() * (i+1) / segments), 0.0));
        wire[2][i] = Line(glm::vec3(0.0, glm::sin(glm::two_pi<float>() * i / segments), glm::cos(glm::two_pi<float>() * i / segments)-1.4), glm::vec3(0.0, glm::sin(glm::two_pi<float>() * (i+1) / segments), glm::cos(glm::two_pi<float>() * (i+1) / segments)-1.4));
        wire[3][i] = Line(glm::vec3(0.0, -glm::sin(glm::two_pi<float>() * i / segments), glm::cos(glm::two_pi<float>() * i / segments)+1.4), glm::vec3(0.0, -glm::sin(glm::two_pi<float>() * (i+1) / segments), glm::cos(glm::two_pi<float>() * (i+1) / segments)+1.4));
    }

    int samples = 50;
    int tracks = 50;
    int layers = 10;

    compiled_vertices = 3 * 2 * 3 + wires * segments * 2 * 3 + samples * 2 * 3 * tracks * layers;
    compiled_length = compiled_vertices * sizeof(GLfloat);
    delete[] g_compiled_vertex_data;
    delete[] g_compiled_color_data;
    g_compiled_vertex_data = new GLfloat[compiled_vertices];
    g_compiled_color_data = new GLfloat[compiled_vertices];
    int tmp_cnt = 0;
    // Axis markers
    g_compiled_vertex_data[0] = 0.0;
    g_compiled_vertex_data[1] = 0.0;
    g_compiled_vertex_data[2] = 0.0;
    g_compiled_vertex_data[3] = 0.1;
    g_compiled_vertex_data[4] = 0.0;
    g_compiled_vertex_data[5] = 0.0;

    g_compiled_vertex_data[6 + 0] = 0.0;
    g_compiled_vertex_data[6 + 1] = 0.0;
    g_compiled_vertex_data[6 + 2] = 0.0;
    g_compiled_vertex_data[6 + 3] = 0.0;
    g_compiled_vertex_data[6 + 4] = 0.1;
    g_compiled_vertex_data[6 + 5] = 0.0;

    g_compiled_vertex_data[2*6 + 0] = 0.0;
    g_compiled_vertex_data[2*6 + 1] = 0.0;
    g_compiled_vertex_data[2*6 + 2] = 0.0;
    g_compiled_vertex_data[2*6 + 3] = 0.0;
    g_compiled_vertex_data[2*6 + 4] = 0.0;
    g_compiled_vertex_data[2*6 + 5] = 0.1;

    g_compiled_color_data[0] = 1.0;
    g_compiled_color_data[1] = 0.0;
    g_compiled_color_data[2] = 0.0;
    g_compiled_color_data[3] = 1.0;
    g_compiled_color_data[4] = 0.0;
    g_compiled_color_data[5] = 0.0;

    g_compiled_color_data[6 + 0] = 0.0;
    g_compiled_color_data[6 + 1] = 1.0;
    g_compiled_color_data[6 + 2] = 0.0;
    g_compiled_color_data[6 + 3] = 0.0;
    g_compiled_color_data[6 + 4] = 1.0;
    g_compiled_color_data[6 + 5] = 0.0;

    g_compiled_color_data[2*6 + 0] = 0.0;
    g_compiled_color_data[2*6 + 1] = 0.0;
    g_compiled_color_data[2*6 + 2] = 1.0;
    g_compiled_color_data[2*6 + 3] = 0.0;
    g_compiled_color_data[2*6 + 4] = 0.0;
    g_compiled_color_data[2*6 + 5] = 1.0;

    tmp_cnt = 18;

    // Current wires
    for(int w = 0; w < wires; w++) {
        for(int i = 0; i < segments; i++) {
            wire[w][i].append_vertices(g_compiled_vertex_data, tmp_cnt);
            wire[w][i].append_colors(g_compiled_color_data, tmp_cnt);
            tmp_cnt += 6;
        }
    }

    // B-field lines
    for(int l = 0; l < layers; l++) {
        for(int t = 0; t < tracks; t++) {
            float len = 0.05;
            glm::vec3 track1(glm::cos(glm::two_pi<float>() * t / 50.0) * (((float)l+0.5) / (float)(layers+1)) * 4.0, glm::sin(glm::two_pi<float>() * t / 50.0) * (((float)l+0.5) / (float)(layers+1)) * 4.0, 0.0);
            glm::vec3 track2(glm::cos(glm::two_pi<float>() * t / 50.0) * (((float)l+0.5) / (float)(layers+1)) * 4.0, glm::sin(glm::two_pi<float>() * t / 50.0) * (((float)l+0.5) / (float)(layers+1)) * 4.0, 0.0);
            for(int i = 0; i < samples / 2; i++) {
                glm::vec b1 = pseudo_bs(&wire[0][0], wires, segments, track1);
                glm::vec b2 = pseudo_bs(&wire[0][0], wires, segments, track2);
                float bstrength1 = glm::length(b1);
                float bstrength2 = glm::length(b2);
                b1 = glm::normalize(b1) * len;
                b2 = glm::normalize(b2) * len;

                g_compiled_vertex_data[tmp_cnt + 0] = track1.x;
                g_compiled_vertex_data[tmp_cnt + 1] = track1.y;
                g_compiled_vertex_data[tmp_cnt + 2] = track1.z;
                g_compiled_vertex_data[tmp_cnt + 3] = track1.x + b1.x;
                g_compiled_vertex_data[tmp_cnt + 4] = track1.y + b1.y;
                g_compiled_vertex_data[tmp_cnt + 5] = track1.z + b1.z;
                
                //glm::log(bstrength1 / 15.0f)+0.6
                glm::vec3 c1 = glm::clamp(hsvToRgb(glm::clamp(glm::vec3(((float)l+0.5) / (float)(layers+1), 1.0f, glm::log(bstrength1 * 10000)/8.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0))), glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0));
                // std::cout << bstrength1 / 15.0f << std::endl;

                g_compiled_color_data[tmp_cnt + 0] = c1.x;
                g_compiled_color_data[tmp_cnt + 1] = c1.y;
                g_compiled_color_data[tmp_cnt + 2] = c1.z;
                g_compiled_color_data[tmp_cnt + 3] = c1.x;
                g_compiled_color_data[tmp_cnt + 4] = c1.y;
                g_compiled_color_data[tmp_cnt + 5] = c1.z;
                // g_compiled_color_data[tmp_cnt + 0] = 1;
                // g_compiled_color_data[tmp_cnt + 1] = 1;
                // g_compiled_color_data[tmp_cnt + 2] = 1;
                // g_compiled_color_data[tmp_cnt + 3] = 1;
                // g_compiled_color_data[tmp_cnt + 4] = 1;
                // g_compiled_color_data[tmp_cnt + 5] = 1;

                g_compiled_vertex_data[tmp_cnt + 6] = track2.x;
                g_compiled_vertex_data[tmp_cnt + 7] = track2.y;
                g_compiled_vertex_data[tmp_cnt + 8] = track2.z;
                g_compiled_vertex_data[tmp_cnt + 9] = track2.x - b2.x;
                g_compiled_vertex_data[tmp_cnt + 10] = track2.y - b2.y;
                g_compiled_vertex_data[tmp_cnt + 11] = track2.z - b2.z;

                //glm::log(bstrength2 / 15.0f)+0.6
                glm::vec3 c2 = glm::clamp(hsvToRgb(glm::clamp(glm::vec3(((float)l+0.5) / (float)(layers+1), 1.0f, glm::log(bstrength2 * 10000)/8.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0))), glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0));

                g_compiled_color_data[tmp_cnt + 6] = c2.x;
                g_compiled_color_data[tmp_cnt + 7] = c2.y;
                g_compiled_color_data[tmp_cnt + 8] = c2.z;
                g_compiled_color_data[tmp_cnt + 9] = c2.x;
                g_compiled_color_data[tmp_cnt + 10] = c2.y;
                g_compiled_color_data[tmp_cnt + 11] = c2.z;
                // g_compiled_color_data[tmp_cnt + 6] = 1;
                // g_compiled_color_data[tmp_cnt + 7] = 1;
                // g_compiled_color_data[tmp_cnt + 8] = 1;
                // g_compiled_color_data[tmp_cnt + 9] = 1;
                // g_compiled_color_data[tmp_cnt + 10] = 1;
                // g_compiled_color_data[tmp_cnt + 11] = 1;

                tmp_cnt += 12;
                track1 += b1;
                track2 -= b2;
            }
        }
    }

    std::cout << tmp_cnt << ":" << compiled_vertices << std::endl;

    // for(int i = 0; i < tmp_cnt; i+=3) {
    //     if(glm::abs(g_compiled_color_data[i])<=0.01f && glm::abs(g_compiled_color_data[i+1])<=0.01f && glm::abs(g_compiled_color_data[i+2])<=0.01f) {
    //         std::cout << i << std::endl;
    //     }
    // }

    // int tmp_cnt = 0;
    // for(float x = -2; x <= 2; x+= 0.1) {
    //     for(float y = -2; y <= 2; y+= 0.1) {
    //         for(float z = -2; z <= 2; z+= 0.1) {
    //             glm::vec b = pseudo_bs(wire, segments, glm::vec3(x, y, z));
    //             float bstrength = glm::length(b);
    //             b = glm::normalize(b) / 40.0f;
    //             g_compiled_vertex_data[tmp_cnt + 0] = x + b.x;
    //             g_compiled_vertex_data[tmp_cnt + 1] = y + b.y;
    //             g_compiled_vertex_data[tmp_cnt + 2] = z + b.z;
    //             g_compiled_vertex_data[tmp_cnt + 3] = x - b.x;
    //             g_compiled_vertex_data[tmp_cnt + 4] = y - b.y;
    //             g_compiled_vertex_data[tmp_cnt + 5] = z - b.z;

    //             g_compiled_color_data[tmp_cnt + 0] = bstrength / 15.0f;
    //             g_compiled_color_data[tmp_cnt + 1] = 0.0f;
    //             g_compiled_color_data[tmp_cnt + 2] = 0.0f;
    //             g_compiled_color_data[tmp_cnt + 3] = bstrength / 15.0f;
    //             g_compiled_color_data[tmp_cnt + 4] = 0.0f;
    //             g_compiled_color_data[tmp_cnt + 5] = 0.0f;

    //             tmp_cnt += 6;
    //         }
    //     }
    // }

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, compiled_length, g_compiled_vertex_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, compiled_length, g_compiled_color_data, GL_STATIC_DRAW);
    glFlush();
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

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
            0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void *)0 // array buffer offset
        );
        // 2nd attribute buffer : colors
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(
            1,        // attribute. No particular reason for 1, but must match the layout in the shader.
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void *)0 // array buffer offset
        );
        // Draw the triangle !
        glDrawArrays(GL_LINES, 0, compiled_vertices / 2); // Starting from vertex 0; 3 vertices total -> 1 triangle
        glDisableVertexAttribArray(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwWaitEvents();
    }

    glfwTerminate();
    return 0;
}
