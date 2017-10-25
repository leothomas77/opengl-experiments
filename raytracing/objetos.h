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
using namespace glm;

struct Superficie {
    vec3 corRGB; 
    vec3 especular;
    vec3 difusa;
    float solidez, espelhamento, transparencia;
};

class ObjetoImplicito { 
 public: 
    Superficie superficie;  
    ObjetoImplicito(){
        superficie.corRGB = vec3(0,0,0);
    }; 
    virtual ~ObjetoImplicito(){};
    virtual bool intersecao(const vec3 origem, const vec3 direcao, float &t0, float &t1) = 0;
    virtual vec3 calcularNormal(vec3 origem, vec3 direcao, float tIntersecao) = 0;
}; 

class Plano: public ObjetoImplicito {
public:
   Plano(const vec3 p0, vec3 normal) {
        
    }
    ~Plano() {} 

    bool intersecao(const vec3 origem, const vec3 direcao, float &t0, float &t1) {
        return false; 
    }
    vec3 calcularNormal(vec3 origem, vec3 direcao, float tIntersecao) {
        return vec3(0);
    }
    
};

class Esfera:  public ObjetoImplicito {
public:
    float raio; 
    vec3 centro;
    Esfera(){};
    Esfera(float raio, vec3 centro, vec3 corRGB) {
        this->raio = raio;
        this->centro = centro;
        this->superficie.corRGB = corRGB;
    }  
    ~Esfera() {} 
    bool intersecao(const vec3 origem, const vec3 direcao, float &t0, float &t1) {
    /* 
        bool retorno = false;
        //cout << "intersecao esfera..." << endl;
        vec3 distancia = origem - centro; 
        float a = dot(distancia, distancia);
        float b = 2 * dot(direcao, distancia); 
        float c = dot(distancia, distancia) - raio * raio; 
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
        cout << "c: " << c << endl;
        
        if (calcularRaizesEquacao(a, b, c, t0, t1)) {
            //cout << "posui raizes..." << endl;
            retorno = true;
        } 
        
        return retorno; 
*/
        vec3 l = this->centro - origem;
        float tca = dot(l, direcao);
        if (tca < 0) {
            return false;
        }
        float d2 = dot(l, l) - tca * tca;
        if (d2 > this->raio * this-> raio) {
            return false;
        }
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

    vec3 calcularNormal(vec3 origem, vec3 direcao, float tIntersecao) {
        vec3 ponto = origem + direcao * tIntersecao;
        vec3 normalPonto = normalize(ponto - this->centro);
        return normalPonto;
    }

};

#endif //__OBJETOS__	