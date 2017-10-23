

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

using namespace glm;


vec3 calcularRaio(glm::vec3 origem, glm::vec3 direcao) {
    return glm::vec3(0, 0, 0);
}

void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t) {
    return;
}

vec3 tracarRaio(glm::vec3 origem, glm::vec3 direcao, vector<ObjetoImplicito*> objetos, 
    vector<GLfloat> &vertices, vector<GLfloat> &cores, vector<GLfloat> &normais) {
    float t = INFINITO;
    static unsigned int cont = 0;
    glm::vec3 cor = BACKGROUND;
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
                glm::vec3 vertice = origem + direcao * t;
                vertices.push_back(vertice.x);                 
                vertices.push_back(vertice.y);
                vertices.push_back(vertice.z);
  
                glm::vec3 normal = objetos.at(i)->calcularNormal(origem, direcao, t);
                normais.push_back(normal.x);                 
                normais.push_back(normal.y);
                normais.push_back(normal.z);
  
                cor = objetos.at(i)->superficie.corRGB;
 
                //cores.push_back(cor.x);                 
                //cores.push_back(cor.y);
                //cores.push_back(cor.z);              
                //cores.push_back(1.0);              
                
            }
        } 
    } 
       
    //cores.push_back(cor);
    //unsigned char retCor = 
    //(unsigned char)(std::min(float(1), cor.x) * 255) <<
    //(unsigned char)(std::min(float(1), cor.y) * 255) <<
    //(unsigned char)(std::min(float(1), cor.z) * 255);
    return cor;
}



void moveCamera() {};
void moveObject() {};