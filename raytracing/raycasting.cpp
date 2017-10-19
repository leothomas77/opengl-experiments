/* ************************************************************************* */
/*     Implementacoes das funcoes                                                                      */
/* ************************************************************************* */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <glm/vec3.hpp> 
#include <glm/glm.hpp>

#include "raycasting.h"
#include "objetos.h"




glm::vec3 calcularRaio(glm::vec3 origem, glm::vec3 direcao) {
    return glm::vec3(0, 0, 0);
}

void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t) {
    return;
}

bool tracarRaio(glm::vec3 origem, glm::vec3 posicao, float t, ObjetoImplicito* objeto) {
    return false;
}



void moveCamera() {};
void moveObject() {};