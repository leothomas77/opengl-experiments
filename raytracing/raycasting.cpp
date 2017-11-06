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
#include <iomanip>

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

#define MAX_RECURSOES	4
#define REFRACAO_VIDRO 1.5f //indice de refracao do vidro
#define REFRACAO_AR 1.0f
#define DESVIO 0.001f
#define SOMBREAMENTO 0.30f

int nanoToMili(double nanoseconds) {
    return (int)(nanoseconds*0x431BDE82)>>18;
}

float interceptarObjetos(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, ObjetoImplicito* objeto) {
    float t = INFINITO;
    for(unsigned int i = 0; i < objetos.size(); i++) {
        float t1 = INFINITO;
        float t0 = INFINITO; 
        if (objetos.at(i)->intersecao(origem, direcao, t0, t1)) {
            if (t0 < 0) {
                t0 = t1;
            }
            if (t0 < t) {
                t = t0;
                objeto = objetos.at(i);
                //cout << "interceptou" << endl;
            }
        } 
    }
    return t;
}

vec3 tracarRaio(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, vector<PontoDeLuz> pontosDeLuz, unsigned int nivel) {
    float t = INFINITO;
    ObjetoImplicito* objeto = NULL;
    vec3 cor = BACKGROUND; 
    vec3 normal = vec3(0); 
    vec3 vertice = vec3(0);
    //cout << "Recursao nivel R" << nivel << endl;
    for(unsigned int i = 0; i < objetos.size(); i++) {
        //cout << "Intersecao objeto " << i << endl;
        
        float t1 = INFINITO;
        float t0 = INFINITO; 
        if (objetos.at(i)->intersecao(origem, direcao, t0, t1)) {
            if (t0 < 0) {
                t0 = t1;
            }
            if (t0 < t) {
                t = t0;
                objeto = objetos.at(i);
            }
        } 
    }

    //cout << "Objeto " << objeto << endl;      

    if (objeto != NULL) {
        vertice = origem + (direcao * t);
        normal = objeto->calcularNormal(origem, direcao, t);
        nivel++; //nivel de recursao
 
        //cout << "Objeto tocado" << endl;
        //cout << "Objeto espelhamento" << objeto->superficie.espelhamento << endl;
        //cout << "Recursoes" << objeto->superficie.espelhamento << endl;
        if (nivel < MAX_RECURSOES) {
            cor = objeto->superficie.difusaRGB; //cor de partida (nunca pode ser zero)
            
            bool temSombra = calcularSombras(vertice, normal, pontosDeLuz, objetos, objeto);
            if (temSombra) {

                cor *= SOMBREAMENTO; //cor nao pode ser negativa
                
            } else if (objeto->superficie.tipoSuperficie == reflexiva) { 
                //cout << "refl" << endl;
                vec3 raioRefletido = normalize(reflect(direcao, normal));
                vec3 corRefletida = tracarRaio(vertice + normal * DESVIO, raioRefletido, objetos, pontosDeLuz, nivel);
                //cout << "Retornou corRefletida " << endl;
                cor = corRefletida;
            } else if (objeto->superficie.tipoSuperficie == refrataria) {
                //cout << "refr" << endl;
                vec3 corRefratada = vec3(0.0f, 0.0f, 0.0f);

                vec3 desvio = normal * DESVIO;
                vec3 verticeRefracao, raioRefratado;
                float cosi = dot(direcao, normal);
                if (cosi < 0) {
                    raioRefratado = calcularRaioRefratado(direcao, normal, REFRACAO_AR, REFRACAO_VIDRO, cosi);
                    verticeRefracao =  vertice - desvio;
                } else {
                    raioRefratado = calcularRaioRefratado(direcao, normal, REFRACAO_AR, REFRACAO_VIDRO, cosi);
                    verticeRefracao = vertice + desvio;
                }
                corRefratada = tracarRaio(verticeRefracao , normalize(raioRefratado), objetos, pontosDeLuz, nivel);
                
                cor =  corRefratada;
            } else {
                //cout << "phong" << endl;
                cor = calcularContribuicoesLuzes(pontosDeLuz, vertice, normal, direcao, objeto);
                //cout << "Retornou corSolida" << endl;
            }

 
        }
    }   
    //cout << "Retorno cor (r: "<< cor.x*255 <<" g:"<< cor.y*255 << " b: "<< cor.z*255 << ")"<< endl;
    return cor;
}

vec3 calcularDifusa(vec3 direcaoLuz, vec3 normal, vec3 difusaRGB) {
    float teta = std::max(dot(direcaoLuz, normal), 0.0f);
    return difusaRGB * teta;    
}

vec3 calcularEspecular(vec3 direcao, vec3 direcaoLuz, vec3 vertice, vec3 normal, vec3 especularRGB, unsigned expoente) {
    vec3 v = normalize(direcao - vertice);
    vec3 r = normalize(reflect(direcaoLuz, normal));
    float omega = std::max(dot(v, r), 0.0f);
    return especularRGB * (float)pow(omega, expoente);
}

vec3 calcularRaioRefratado(vec3 direcao, vec3 normal, float n1, float n2, float cosi) { 
    float etai, etat; 
    //cout << "Raio refratado cosi=" << cosi << endl;
    if (cosi > 0.0f) { // raio dentro -> fora
        //cout << "Raio refratado saiu" << endl;
        etai = n2;
        etat = n1;
        normal = -1.0f * normal; 
    } else { // raio raio fora -> dentro
        //cout << "Raio refratado entrou" << endl;
        etai = n1;
        etat = n2;
        cosi = -cosi; 
    } 
    float eta = etai / etat; 
    float k = 1 - eta * eta * (1 - cosi * cosi); 
    if (k < 0) {
        return vec3(0);
    } else {
        //cout << "K = " << k << endl;
        return eta * direcao + (eta * cosi - sqrtf(k)) * normal;
    }

}

vec3 calcularDirecaoLuz(vec3 vertice, vec3 posicaoLuz) {
    return normalize(posicaoLuz - vertice);
}

vec3 calcularContribuicoesLuzes(vector<PontoDeLuz> pontosDeLuz, vec3 vertice, vec3 normal, vec3 direcao, ObjetoImplicito* objeto) {
    vec3 ambiente = objeto->superficie.ambienteRGB;
    vec3 especular = objeto->superficie.especularRGB;
    vec3 difusa = objeto->superficie.difusaRGB;
    unsigned expoente = objeto->superficie.expoente;
    float r = ambiente.r, g = ambiente.g, b = ambiente.b;

    //calcula a contribuicao de cada luz na cor
    unsigned luzesLigadas = 0;
    
    for (unsigned k=0; k < pontosDeLuz.size(); k++) {
        if (pontosDeLuz.at(k).estado == LIGADA) {
            luzesLigadas++;
            vec3 direcaoLuz = normalize(pontosDeLuz.at(k).posicao - vertice);
            float decaimentoLuz = 1 / length(direcaoLuz);
            vec3 difusaEspecular = 
                calcularDifusa(direcaoLuz, normal, difusa) +
                calcularEspecular(direcao, direcaoLuz, vertice, normal, especular, expoente);
            r += decaimentoLuz * difusaEspecular.r;
            g += decaimentoLuz * difusaEspecular.g;
            b += decaimentoLuz * difusaEspecular.b;

        }
    }
    //cout << "luzes ligadas:" << luzesLigadas << endl;
    luzesLigadas == 0 ? luzesLigadas = 1 : luzesLigadas = luzesLigadas; //evita divisao por zero
    return vec3(r / luzesLigadas, g / luzesLigadas, b / luzesLigadas );
}

/*
Verifica na cena se do ponto tocado ao ponto de luz existe intersecao com algum outro objeto da cena
*/
bool calcularSombras(vec3 vertice, vec3 normal, vector<PontoDeLuz> pontosDeLuz, vector<ObjetoImplicito*> objetos, ObjetoImplicito* objetoTocado) {
    for (unsigned k = 0; k < pontosDeLuz.size(); k++) {
        if (pontosDeLuz.at(k).estado == LIGADA) {
            vec3 raioObjetoLuz = normalize(pontosDeLuz.at(k).posicao - vertice);
            float distanciaLuz = length(raioObjetoLuz);
            float t = INFINITO;
            for (unsigned i = 0; i < objetos.size() && objetos.at(i) != objetoTocado; i++) {
                float t0 = INFINITO, t1 = INFINITO;
                if (objetos.at(i)->intersecao(vertice + normal * DESVIO, raioObjetoLuz, t0, t1) && objetos.at(i) != objetoTocado) {
                    return true;
                }
            }
        }
    }
    return false;
}

unsigned obterEstadoLuz(vector<PontoDeLuz> pontosDeLuz) {
    if (pontosDeLuz.size() == 2) {
        if (pontosDeLuz.at(0).estado == LIGADA    && pontosDeLuz.at(1).estado == DESLIGADA) return LUZ_1;
        if (pontosDeLuz.at(0).estado == DESLIGADA    && pontosDeLuz.at(1).estado == LIGADA) return LUZ_2;
        if (pontosDeLuz.at(0).estado == LIGADA && pontosDeLuz.at(1).estado == LIGADA) return LUZ_12;
    } else {
        cout << "Erro ao recuperar estado das luzes" << endl;
    }
    return LUZ_1;
}

void mudarEstadoLuz(unsigned &estado, vector<PontoDeLuz> &pontosDeLuz) {
    unsigned novoEstado;
    if (pontosDeLuz.size() == 2) {
        switch (estado) {
            case LUZ_1: novoEstado = LUZ_2;
                        //cout << "Novo estado: " << novoEstado << endl;
                        pontosDeLuz.at(0).estado = DESLIGADA;
                        pontosDeLuz.at(1).estado = LIGADA;
                        break;
            case LUZ_2: novoEstado = LUZ_12;
                        //cout << "Novo estado: " << novoEstado << endl;
                        pontosDeLuz.at(0).estado = LIGADA;
                        pontosDeLuz.at(1).estado = LIGADA;
                        break;
            case LUZ_12: novoEstado = LUZ_1;
                        //cout << "Novo estado: " << novoEstado << endl;
                        pontosDeLuz.at(0).estado = LIGADA;
                        pontosDeLuz.at(1).estado = DESLIGADA;
                        break;
        }
        estado = novoEstado;
    } else {
        cout << "Sem pontos de luz para definir estado" << endl;
    }
    cout << "Luz1: " << pontosDeLuz.at(0).estado << " Luz2: " << pontosDeLuz.at(1).estado << endl;
    return;
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
