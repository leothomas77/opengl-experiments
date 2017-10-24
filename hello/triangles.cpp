#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <GL/glew.h>

#include <GLFW/glfw3.h>


#ifndef BUFFER_OFFSET 
	#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

using namespace std;

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(VAOs[Triangles]);
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
    glFlush(); 
}

void init(void)
{
    glGenVertexArrays(NumVAOs, VAOs);
    glBindVertexArray(VAOs[Triangles]);
    GLfloat vertices[NumVertices][2] = {
        {-0.90, -0.90}, // Triangle 1
        {0.85, -0.90},
        {-0.90, 0.85},
        {0.90, -0.85}, // Triangle 2
        {0.90, 0.90},
        {-0.85, 0.90}};
    glGenBuffers(NumBuffers, Buffers);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers[ArrayBuffer]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                 vertices, GL_STATIC_DRAW);
    ShaderInfo shaders[] = {
        {GL_VERTEX_SHADER, "triangles.vert"},
        {GL_FRAGMENT_SHADER, "triangles.frag"},
        {GL_NONE, NULL}};
    GLuint program = LoadShaders(shaders);
    glUseProgram(program);
    glVertexAttribPointer(vPosition, 2, GL_FLOAT,
                          GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vPosition);
}

int main(int argc, char *argv[]) { 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA);
    glutInitWindowSize(512, 512);
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutCreateWindow(argv[0]);
    if (glewInit())
    {
        cout << "Unable to initialize GLEW ... exiting" << endl;
        exit(EXIT_FAILURE);
    }
    init();
    glutDisplayFunc(display);
    glutMainLoop();
}