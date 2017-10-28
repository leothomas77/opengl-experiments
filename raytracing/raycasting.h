#ifndef __RAYCASTING__	
#define __RAYCASTING__ 1

#include <limits.h> 

#ifndef INFINITO
	#define INFINITO 1e8
#endif

#ifndef BACKGROUND
	#define BACKGROUND vec3(0.0, 0.0, 0.0)
#endif

#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 
#include "raycasting.h"
#include "objetos.h"


using namespace std;
using namespace glm;
/*
Funcoes para implementacao da tecnica de renderizacao por raycasting
*/
int nanoToMili(double nanoseconds);
	
vec3 tracarRaio(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, 
    vec3 posicaoLuz, unsigned int nivel);
void moveCamera();
void moveObject();
void shade(vec3 origemRaio, vec3 direcaoRaio, float t);
void shade(vec3 lightPos, vec3 camPos, mat4 MVP, mat4 normalMat, mat4 ModelMat);
void salvarImagem(GLFWwindow* window,  vector<vec3> imagem, unsigned width, unsigned height);
vec3 calcularPhong(vec3 origem, vec3 direcao, vec3 posicaoLuz, 
	vec3 normal, vec3 vertice, vec3 difusa, vec3 especular);
vec3 calcularEspecular(vec3 direcao, vec3 posicaoLuz, vec3 vertice, vec3 normal, vec3 especularRGB);
float clip(float x, float min, float max);
vec3 calcularDifusa(vec3 direcaoLuz, vec3 normal, vec3 difusa);
bool temSombra(vec3 vertice, vec3 posicaoLuz, vector<ObjetoImplicito*> objetos);
	
	

#endif //__RAYCASTING__	