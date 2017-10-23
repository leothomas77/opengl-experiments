#ifndef __RAYCASTING__	
#define __RAYCASTING__ 1

#include <limits.h> 

#ifndef INFINITO
	#define INFINITO 1e8
#endif

#ifndef BACKGROUND
	#define BACKGROUND glm::vec3(0.0, 0.0, 0.0)
#endif

#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 
#include "raycasting.h"
#include "objetos.h"

/*
Funcoes para implementacao da tecnica de renderizacao por raycasting
*/
glm::vec3 tracarRaio(glm::vec3 origem, glm::vec3 direcao, vector<ObjetoImplicito*> objetos, 
    vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais);
void moveCamera();
void moveObject();
void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t);
void shade(glm::vec3 lightPos, glm::vec3 camPos, glm::mat4 MVP, glm::mat4 normalMat, glm::mat4 ModelMat);
void salvarImagem(GLFWwindow* window, glm::vec3 *pixels);
	

#endif //__RAYCASTING__	