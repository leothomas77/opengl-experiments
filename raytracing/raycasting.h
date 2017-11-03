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

#define DESLIGADA   false
#define LIGADA      true

#define LUZ_1       0
#define LUZ_2       1
#define LUZ_12      2


using namespace std;
using namespace glm;
/*
Funcoes para implementacao da tecnica de renderizacao por raycasting
*/
int nanoToMili(double nanoseconds);
	
vec3 tracarRaio(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, 
    vector<PontoDeLuz> pontosDeLuz, unsigned int nivel);
void salvarImagem(GLFWwindow* window,  vector<vec3> imagem, unsigned width, unsigned height);
vec3 calcularPhong(vec3 origem, vec3 direcao, vec3 posicaoLuz, 
	vec3 normal, vec3 vertice, vec3 difusa, vec3 especular);
vec3 calcularEspecular(vec3 direcao, vec3 direcaoLuz, vec3 vertice, vec3 normal, vec3 especularRGB, unsigned expoente);
float clip(float x, float min, float max);
vec3 calcularDifusa(vec3 direcaoLuz, vec3 normal, vec3 difusa);
float calcularSombras(vec3 vertice, vec3 normal, vector<PontoDeLuz> pontosDeLuz, vector<ObjetoImplicito*> objetos, ObjetoImplicito* objetoTocado);
void mudarEstadoLuz(unsigned &estado, vector<PontoDeLuz> &pontosDeLuz);	
vec3 calcularDirecaoLuz(vec3 vertice, vec3 posicaoLuz);
unsigned obterEstadoLuz(vector<PontoDeLuz> pontosDeLuz);
float mix(const float &a, const float &b, const float &mix);
float interceptarObjetos(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, ObjetoImplicito* objeto, float &t);
vec3 calcularRaioRefratado (vec3 direcao, vec3 normal);
vec3 calcularContribuicoesLuzes(vector<PontoDeLuz> pontosDeLuz, vec3 vertice, vec3 normal, vec3 direcao, ObjetoImplicito* objeto);
float calcularFresnel(vec3 direcao, vec3 normal);
	
	

#endif //__RAYCASTING__	