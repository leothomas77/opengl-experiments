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

#ifndef INFINITO
#define INFINITO 1e8
#endif

using namespace std;
using namespace glm;

enum TipoSuperficie {
    reflexiva, refrataria, solida
};

struct Superficie {
    vec3 corRGB; 
    vec3 ambienteRGB = vec3(0.18, 0.18, 0.18);
    vec3 especularRGB = vec3(1.0, 1.0, 1.0);
    vec3 difusaRGB = vec3(0.72, 0.72, 0.72);
    
    //bool espelhamento = false;
    //bool transparencia = false;
    TipoSuperficie tipoSuperficie = solida;
};

struct IntersecaoObjeto {
    float tIntersecao;
    unsigned int indiceObjeto;

    bool operator < (const IntersecaoObjeto& stIntersecao) const
    {
        return (tIntersecao < stIntersecao.tIntersecao);
    }
};

class ObjetoImplicito { 
 public: 

    Superficie superficie;  
    ObjetoImplicito(){
        superficie.corRGB = vec3(0,0,0);
    };
    void setSuperficie(vec3 difusa, vec3 especular) {
        this->superficie.especularRGB = especular;
        this->superficie.difusaRGB = difusa;
    }
    virtual ~ObjetoImplicito(){};
    virtual bool intersecao(const vec3 origem, const vec3 direcao, float &t0, float &t1) = 0;
    //virtual bool intersecao(const vec3 origem, const vec3 direcao, float &t) = 0; 
    virtual vec3 calcularNormal(vec3 origem, vec3 direcao, float tIntersecao) = 0;
}; 

class Triangulo: public ObjetoImplicito {
public:
    vec3 v1, v2, v3; 
    Triangulo(vec3 v1, vec3 v2, vec3 v3) {
        this->v1 = v1;
        this->v2 = v2;
        this->v3 = v3;
    } 
    ~Triangulo() {}
    float calculaS1(vec3 direcao) {
        vec3 e1 = this->v2 - this->v1;
        vec3 e2 = this->v3 - this->v1;
        vec3 s1 = cross(direcao, e2);
        float divisor = dot(s1, e1);
        if (divisor == 0.0) {
            return 1.0;//TODO verificar aqui
        }
        return (float)(1 / divisor);    
    }
    float calculaB1(vec3 direcao) {
        return 0.0;
    } 
    float calculaB2(vec3 direcao) {
        return 0.0;
    }
    bool intersecao(const vec3 origem, const vec3 direcao, float &t0, float &t1) {
        return false;
    }
    
};

class Plano: public ObjetoImplicito {
public:
    
    vec3 p0 = vec3(0,0,0);
    vec3 normal = vec3(0,0,0);
    float d = 0;
    Plano(const vec3 p0, vec3 normal, float d) {
        this->p0 = p0;
        this->normal = normal;
        this->d = d;
    }
    ~Plano() {} 

    bool intersecao(const vec3 origem, const vec3 direcao, float &t0, float &t1) {
        float t = (-1)* (this->d + dot(normal, origem)) / (dot(normal, direcao));

        if (t > 0) {
            t0 = t;
            t1 = t;
            return true;
        }

        return false; 
    }
    vec3 calcularNormal(vec3 origem, vec3 direcao, float tIntersecao) {
        return normalize(this->normal);
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
		//cout << "delta: " << delta << endl;
        
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
        vec3 ponto = origem + (direcao * tIntersecao);
        vec3 normalPonto = normalize(ponto - this->centro);
        return normalPonto;
    }

};

#endif //__OBJETOS__	