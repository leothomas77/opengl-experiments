/* ************************************************************************* */
/*     Implementacoes das funcoes                                                                      */
/* ************************************************************************* */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cfloat>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include <glm/vec3.hpp> 
#include <glm/glm.hpp>

#include "raycasting.h"
#include "objetos.h"

using namespace std;
using namespace glm;


vec3 calcularRaio(glm::vec3 origem, glm::vec3 direcao) {
    return glm::vec3(0, 0, 0);
}

void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t) {
    return;
}

glm::vec3 tracarRaio(glm::vec3 origem, glm::vec3 direcao, std::vector<ObjetoImplicito*> objetos) {
 
    float t = INFINITO;
    static unsigned int cont = 0;
    glm::vec3 cor = BACKGROUND;
    
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
                cont++;
                //cout << "objeto tocado..." << cont << " vezes" << endl;
                cor = objetos.at(i)->superficie.corRGB;
            }
        }
        
    }
 

    return cor;
}



void moveCamera() {};
void moveObject() {};