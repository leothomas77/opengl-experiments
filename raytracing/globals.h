#ifndef __RTGLOBALS__	
#define __RTGLOBALS__

#include <iostream>
#include <cfloat>
#include <vector>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <fstream>
#include <vector>
#include <cassert>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifndef BUFFER_OFFSET 
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif
#ifndef MIN
#define MIN(x,y) (x<y?x:y)
#endif
#ifndef MAX
#define MAX(x,y) (y>x?y:x)
#endif

#define MAX_INTERSECOES 10
#define MAX_RECURSOES	5

#ifndef WIDTH
#define WIDTH 600
#endif

#ifndef HEIGHT
#define HEIGHT 600
#endif

using namespace std;

GLuint 	shaderAmbient,
		shaderGouraud,
		shaderPhong,
		shader;
GLuint 	axisVBO[3];
GLuint 	meshVBO[3];
GLuint 	meshSize;

bool carregou = false;
//Movimento da camera
glm::vec3 deslocamento(0);
#define PASSO_CAMERA 3.0f
float 	velocidade = 3.0f;
double 	ellapsed = 0; //tempo decorrido entre 2 amostras no loop principal
bool 	moveu = false;


double  last;

vector<GLfloat> vboVertices;
vector<GLfloat> vboNormals;
vector<GLfloat> vboColors;

unsigned winWidth 	= WIDTH, winHeight 	= HEIGHT;

float 	angleX 	= 	0.0f,
		angleY	= 	0.0f,
		angleZ	=	0.0f,
		distanciaCamera = 5.0f;

/* the global Assimp scene object */
const aiScene* scene = NULL;
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;


#endif //__RTGLOBALS__	