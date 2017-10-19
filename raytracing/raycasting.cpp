/* ************************************************************************* */
/*     Implementacoes das funcoes                                                                      */
/* ************************************************************************* */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <glm/vec3.hpp> 
#include <glm/glm.hpp>

#include "raycasting.h"


class ObjetoImplicito { 
 public: 
    ObjetoImplicito() {} 
    virtual ~ObjetoImplicito() {} 
    virtual bool intersecao(const glm::vec3 &, const glm::vec3 &, float &) const = 0; 
}; 

class Plano: ObjetoImplicito {
public:
    glm::vec3 normal;
    glm::vec3 p0;
  
    bool intercecao(const glm::vec3 &origem, const glm::vec3 &direcao, float &t) {
 
        float denominador = glm::dot(normal, direcao); 
        if (denominador > INFINITO) { 
            glm::vec3 distancia = p0 - origem;
            t = glm::dot(distancia,  normal) / denominador; 
            return (t >= 0); 
        } 
            return false; 
        }
};

class Esfera: ObjetoImplicito {
public:
    float raio; 
    glm::vec3 centro;
    
    Esfera(float raio, glm::vec3 centro) {
        this->raio = raio;
        this->centro = centro;
    }  
    bool intercecao(const glm::vec3 &origem, const glm::vec3 direcao, float &t) {
        bool retorno = false;
        float t0 = 0, t1 = 0; 
        glm::vec3 distancia = origem - centro; 
        float a = glm::dot(distancia, distancia);
        float b = 2 * glm::dot(direcao, distancia); 
        float c = glm::dot(distancia, distancia) - raio * raio; 

        retorno = !resolveEquacao2oGrau(a, b, c, t0, t1) || (t0 < 0 && t1 < 0);

        if (t0 > t1) {
            float aux = t0;
            t0 = t1;
            t1 = aux;
        }

        return retorno; 
 
    }

    bool resolveEquacao2oGrau(float a, float b, float c, float &t0, float &t1) { 
        float delta = b * b - 4 * a * c; 
        bool retorno = false; 
        if (delta == 0) { 
            t0 = t1 = - b / (2 * a);
            retorno = true;
        } 
        else if (delta > 0) { 
            float q = (b > 0) ? -0.5 * (b + sqrt(delta)) : -0.5 * (b - sqrt(delta)); 
            t0 = q / a; 
            t1 = c / q; 
            retorno = true;
        } 
        return retorno; 
    } 

};


glm::vec3 calcularRaio(glm::vec3 origem, glm::vec3 direcao) {
    return glm::vec3(0, 0, 0);
}

void shade(glm::vec3 origemRaio, glm::vec3 direcaoRaio, float t) {
    return;
}

bool tracarRaio(glm::vec3 origem, glm::vec3 posicao, float t) {
    return false;
}



void moveCamera() {};
void moveObject() {};