#ifndef __OBJETOS__	
#define __OBJETOS__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cfloat>
#include <cstdlib>
#include <cstdio>
#include <glm/vec3.hpp>

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
    virtual bool intersecao(const glm::vec3 origem, const glm::vec3 direcao, float &t0, float &t1) = 0;

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

    bool intersecao(const glm::vec3 origem, const glm::vec3 direcao, float &t0, float &t1) {
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
    bool intersecao(const glm::vec3 origem, const glm::vec3 direcao, float &t0, float &t1) {
    /* 
        bool retorno = false;
        //cout << "intersecao esfera..." << endl;
        glm::vec3 distancia = origem - centro; 
        float a = glm::dot(distancia, distancia);
        float b = 2 * glm::dot(direcao, distancia); 
        float c = glm::dot(distancia, distancia) - raio * raio; 
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
        cout << "c: " << c << endl;
        
        if (calcularRaizesEquacao(a, b, c, t0, t1)) {
            //cout << "posui raizes..." << endl;
            retorno = true;
        } 
        
        return retorno; 
*/
        glm::vec3 l = this->centro - origem;
        float tca = glm::dot(l, direcao);
        if (tca < 0) return false;
        float d2 = glm::dot(l, l) - tca * tca;
        if (d2 > this->raio * this-> raio) return false;
        float thc = sqrt(this->raio * this-> raio - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        return true;
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
            //cout << "delta>0 " << delta << endl;
            float raizDelta = sqrt(delta);
            t0 = (-b + raizDelta) / (2 * a);
            t1 = (-b - raizDelta) / (2 * a);
            retorno = true;
        } 
        return retorno; 
    }

    //glm::vec3 calcularNormal(glm::vec3 origem, glm::vec3 direcao, float tIntersecao) {
      //  glm::vec3 ponto = glm::dot((origem + direcao), tIntersecao);
      //  glm::vec3 normalPonto = glm::normalize(ponto - this->centro);
      //  return normalPonto;
    //}

};

#endif //__OBJETOS__	