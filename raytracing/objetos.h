#ifndef __OBJETOS__	
#define __OBJETOS__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cfloat>
#include <cstdlib>
#include <cstdio>

using namespace std;
struct Superficie {
    glm::vec3 corRGB; 
    glm::vec3 especular;
    glm::vec3 difusa;
    float solidez, espelhamento, transparencia;
};

class ObjetoImplicito { 
 public: 
    Superficie superficie;  
    ObjetoImplicito(){
        superficie.corRGB = glm::vec3(0,0,0);
    }; 
    virtual ~ObjetoImplicito(){};
    virtual bool intersecao(const glm::vec3 origem, const glm::vec3 direcao, float &t) = 0;

}; 

class Plano: public ObjetoImplicito {
public:
    glm::vec3 normal = glm::vec3(0);
    glm::vec3 p0 = glm::vec3(0);

    Plano(const glm::vec3 p0, glm::vec3 normal) {
        this->p0 = p0;
        this->normal = normal;
    }
    ~Plano() {} 

    bool intersecao(const glm::vec3 origem, const glm::vec3 direcao, float &t) {
    
        float denominador = glm::dot(this->normal, direcao); 
        if (denominador > INFINITO) { 
            glm::vec3 distancia = this->p0 - origem;
            t = glm::dot(distancia,  this->normal) / denominador; 
            return (t >= 0); 
        } 
            return false; 
        }
};

class Esfera:  public ObjetoImplicito {
public:
    float raio; 
    glm::vec3 centro;
    Esfera(){};
    Esfera(float raio, glm::vec3 centro, glm::vec3 corRGB) {
        this->raio = raio;
        this->centro = centro;
        this->superficie.corRGB = corRGB;
    }  
    ~Esfera() {} 
        bool intersecao(const glm::vec3 origem, const glm::vec3 direcao, float &t) {
    
        bool retorno = false;
        float t0 = 0, t1 = 0; 
        cout << "aqui..." << endl;
        glm::vec3 distancia = origem - centro; 
        float a = glm::dot(distancia, distancia);
        float b = 2 * glm::dot(direcao, distancia); 
        float c = glm::dot(distancia, distancia) - raio * raio; 
        cout << "intersecao..." << endl;
    
        if (calcularRaizesEquacao(a, b, c, t0, t1) && (t0 > 0 || t1 > 0)) {
            retorno = true;
        }

        if (t0 > t1) {
            float aux = t0;
            t0 = t1;
            t1 = aux;
        }

        t = t0;

        return retorno; 
 
    }

    bool calcularRaizesEquacao(float a, float b, float c, float &t0, float &t1) { 
        float delta = b * b - 4 * a * c;
        bool retorno = false; 
		cout << "delta: " << delta << endl;
        
        if (delta == 0) { 
            t0 = t1 = -b / (2 * a);
            retorno = true;
        } 
        else if (delta > 0) {
            float raizDelta = sqrt(delta);
            t0 = (-b + raizDelta) / (2 * a);
            t1 = (-b - raizDelta) / (2 * a);
            retorno = true;
        } 
        return retorno; 
    } 

};

#endif //__OBJETOS__	