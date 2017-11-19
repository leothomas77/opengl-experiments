#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <GLFW/glfw3.h>
					
using namespace std;

bool inside = false;

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void error_callback(int error, const char* description) {
    cout << "Error: " << description << endl;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void window_size_callback(GLFWwindow* window, int width, int height) {
    cout << "Resized window: " << width << " , " << height << endl;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {

	int w, h;

	if (inside) {
		glfwGetWindowSize(window, &w, &h);
		glClearColor(xpos / w, ypos / h, 1.0 - xpos / h, 0.0);	
	}

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void cursor_enter_callback(GLFWwindow* window, int entered) {

	if (entered) {
		cout << "mouse: ENTER" << endl;	
		inside = true;
		}
	else {
		cout << "mouse: LEAVE" << endl;	
		inside = false;
		}

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	if (action == GLFW_PRESS) 
		switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT	:	cout << "mouse rigth button PRESSED" << endl;	
												break;
			case GLFW_MOUSE_BUTTON_MIDDLE	:	cout << "mouse middle button PRESSED" << endl;	
												break;
			case GLFW_MOUSE_BUTTON_LEFT		:	cout << "mouse left button PRESSED" << endl;	
												break;
			}
	else
		cout << "mouse button: RELEASED" << endl;	

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (action == GLFW_PRESS)										
		switch (key) {	
			case GLFW_KEY_ESCAPE  	: 	glfwSetWindowShouldClose(window, true);
										break;
			case GLFW_KEY_UP		: 	cout << "UP" << endl;
										break;
			case GLFW_KEY_DOWN		: 	cout << "DOWN" << endl;
										break;
			case GLFW_KEY_LEFT		: 	cout << "LEFT" << endl;
										break;
			case GLFW_KEY_RIGHT		: 	cout << "RIGHT" << endl;
										break;
			}

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static GLFWwindow* initGLFW(char* nameWin, int w, int h) {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
	    exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(w, h, nameWin, NULL, NULL);
	if (!window) {
	    glfwTerminate();
	    exit(EXIT_FAILURE);
		}

	glfwSetWindowSizeCallback(window, window_size_callback);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	return (window);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void initGL(GLFWwindow* window) {

    int width, height;

	glClearColor(0.0, 0.0, 0.0, 0.0);
    
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);

}


/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void display() {

	glClear(GL_COLOR_BUFFER_BIT);

}

 
/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void GLFW_MainLoop(GLFWwindow* window) {

   while (!glfwWindowShouldClose(window)) {

   		display();
   		
        glfwSwapBuffers(window);
        glfwPollEvents();
    	}
}

/* ************************************************************************* */
/* ************************************************************************* */
/* *****                                                               ***** */
/* ************************************************************************* */
/* ************************************************************************* */

int main(int argc, char *argv[]) { 

    GLFWwindow* window;

    window = initGLFW(argv[0], 500, 500);

    initGL(window);

    GLFW_MainLoop(window);

    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
	}
