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

#include <glm/vec3.hpp> 
#include <glm/glm.hpp>

#include "raycasting.h"
#include "objetos.h"

using namespace std;



glm::vec3 calcularRaio(glm::vec3 origem, glm::vec3 direcao) {
    return glm::vec3(0, 0, 0);
}

void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t) {
    return;
}

glm::vec3 tracarRaio(glm::vec3 origem, glm::vec3 posicao, float t, Esfera *objeto) {
    Esfera *objetoTocado = new Esfera();
    t = INFINITO; 
    float menorT = INFINITO;
    //cout << "intersecao..." << endl;
    
    if (objeto->intersecao(origem, posicao, t)) {
        if (t < menorT) {
            menorT = t;
            objetoTocado = objeto;
            cout << "objeto tocado..." << endl;
        }
    }
    return objetoTocado->superficie.corRGB;
}



void moveCamera() {};
void moveObject() {};