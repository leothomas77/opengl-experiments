#ifndef __RAYCASTING__	
#define __RAYCASTING__ 1

#include <limits.h> 

#ifndef INFINITO
	#define INFINITO 1e8
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
bool tracarRaio(glm::vec3 origem, glm::vec3 posicao, float t, ObjetoImplicito *objeto);
void moveCamera();
void moveObject();
void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t);
void shade(glm::vec3 lightPos, glm::vec3 camPos, glm::mat4 MVP, glm::mat4 normalMat, glm::mat4 ModelMat);


#endif //__RAYCASTING__	