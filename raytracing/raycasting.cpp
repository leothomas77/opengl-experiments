/* ************************************************************************* */
/*     Implementacoes das funcoes                                                                      */
/* ************************************************************************* */
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
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "raycasting.h"
#include "objetos.h"


using namespace std;
using namespace glm;


int nanoToMili(double nanoseconds) {
    return (int)(nanoseconds*0x431BDE82)>>18;
}

vec3 calcularRaio(vec3 origem, vec3 direcao) {
    return vec3(0, 0, 0);
}

void shade(vec3 origemRaio, vec3 direcaoRaio, float t) {
    return;
}


vec3 tracarRaio(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, 
    vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais, 
    vec3 posicaoLuz) {
    float t = INFINITO;
    static unsigned int cont = 0;
    vec3 cor = BACKGROUND; vec3 normal = vec3(0); vec3 vertice = vec3(0);
    static unsigned totalVertices = 0;
    for(unsigned i = 0; i < objetos.size(); i++) {
        float t1 = INFINITO;
        float t0 = INFINITO; 
        //cout << "objeto processando..." << endl;

        if (objetos.at(i)->intersecao(origem, direcao, t0, t1)) {
            ///cout << "intersecao... t0" << t0 << " t1" << t1 << endl;
            
            if (t0 < 0) {
                t0 = t1;
            }
            if (t0 < t) {
                t = t0;
                totalVertices++;
                //cout << "objeto tocado..." << totalVertices << " vezes" << endl;
                vertice = origem + (direcao * t);
                vertices.push_back(vertice.x);                 
                vertices.push_back(vertice.y);
                vertices.push_back(vertice.z);
  
                normal = objetos.at(i)->calcularNormal(origem, direcao, t);
                normais.push_back(normal.x);                 
                normais.push_back(normal.y);
                normais.push_back(normal.z);
  
                cor = objetos.at(i)->superficie.corRGB;
      
            }
        } 
    } 

    float phong = calcularPhong(cor, origem, direcao, posicaoLuz, normal, vertice);
    float atenuacao = 0.7;
    float ambiente = 1.0;
    return cor * atenuacao * phong * ambiente;
}

float calcularPhong(vec3 cor, vec3 origem, vec3 direcao, vec3 posicaoLuz, 
  vec3 normal, vec3 vertice) {
    vec3 l = normalize(posicaoLuz - vertice);
    float kDifusa = 0.8;
    float teta = std::max(dot(l, normal), 0.0f);

    vec3 v = normalize(direcao - vertice);
    vec3 refletido = reflect(l*(-1.0f), normal);
    vec3 r = normalize(refletido);
    float kEspecular = 1.0;
    float omega = std::max(dot(v, r), 0.0f); 

    return kDifusa * teta + kEspecular * pow(omega, 56);
}

void salvarImagem(GLFWwindow* window,  vector<vec3> imagem, unsigned width, unsigned height) {
	cout << "Salvando imagem " << endl;
	
 	ofstream ofs("./imagem.ppm", ios::out | ios::binary);
 	ofs << "P6\n" << width << " " << height << "\n255\n";
	for (unsigned i = 0; i < imagem.size(); ++i) {
		ofs << (unsigned char)(std::min(float(1), imagem[i].x) * 255) <<
		(unsigned char)(std::min(float(1), imagem[i].y) * 255) <<
		(unsigned char)(std::min(float(1), imagem[i].z) * 255);
	}
	ofs.close();
}