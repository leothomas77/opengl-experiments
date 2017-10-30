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


using namespace std;
using namespace glm;

#define MAX_RECURSOES	5
#define REFRACAO_VIDRO 1/3.0f //indice de refracao do vidro
#define REFRACAO_AR 1.0f
#define DESVIO 0.01f
#define ATENUACAO 0.30f

int nanoToMili(double nanoseconds) {
    return (int)(nanoseconds*0x431BDE82)>>18;
}

vec3 calcularRaio(vec3 origem, vec3 direcao) {
    return vec3(0, 0, 0);
}

void shade(vec3 origemRaio, vec3 direcaoRaio, float t) {
    return;
}


vec3 tracarRaio(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, 
    vector<PontoDeLuz> pontosDeLuz, unsigned int nivel) {
    float t = INFINITO;
    ObjetoImplicito* objeto = NULL;
    vec3 cor = BACKGROUND; 
    vec3 normal = vec3(0); 
    vec3 vertice = vec3(0);

    for(unsigned int i = 0; i < objetos.size(); i++) {
        float t1 = INFINITO;
        float t0 = INFINITO; 
        //cout << "objeto processando..." << endl;
        if (objetos.at(i)->intersecao(origem, direcao, t0, t1)) {
            //cout << "intersecao... t0" << t0 << " t1" << t1 << endl;
            if (t0 < 0) {
                t0 = t1;
            }
            if (t0 < t) {
                t = t0;
                //intersecaoObjeto.tIntersecao = t;
                //intersecaoObjeto.indiceObjeto = i;
                //intersecoesObjetos.push_back(intersecaoObjeto);
                objeto = objetos.at(i);
            }
        } 
    }

    if (objeto != NULL) {
        nivel++;//nivel de recursao
        bool tocou = false;
        vec3 ambiente = objeto->superficie.ambienteRGB;
        vertice = origem + (direcao * t);
        normal = objeto->calcularNormal(origem, direcao, t);

        if (dot(direcao, normal) > 0) {
            //cout << "Normal maior zero = obliquo" << endl;
            normal = -normal;
            tocou = true;
        }
        //cout << "Objeto tocado" << endl;
        //cout << "Objeto espelhamento" << objeto->superficie.espelhamento << endl;
        //cout << "Recursoes" << objeto->superficie.espelhamento << endl;
        if (objeto->superficie.tipoSuperficie == reflexiva && nivel < MAX_RECURSOES) {
            //cout << "Objeto espelhado" << endl;
            vec3 raioRefletido = reflect(direcao, normal);
            raioRefletido = normalize(raioRefletido); 
            cor = tracarRaio(vertice , raioRefletido, objetos, pontosDeLuz, nivel);
        } else if (objeto->superficie.tipoSuperficie == refrataria && nivel < MAX_RECURSOES) { 
            float indiceRefracao ;
            //calculo da refracao
            if (tocou) {
                //cout << "refrata vidro" << endl;
                indiceRefracao = REFRACAO_VIDRO;
            } else {
                //cout << "refrata ar" << endl;
                indiceRefracao = REFRACAO_AR;
            } 
            vec3 raioRefratado = normalize(refract(vertice, normal, indiceRefracao));
            cor = tracarRaio(vertice - normal * DESVIO, raioRefratado, objetos, pontosDeLuz, nivel);
        } else {
            cor = objeto->superficie.corRGB;
            vec3 especular = objeto->superficie.especularRGB;
            vec3 difusa = objeto->superficie.difusaRGB;
            float atenuacao = 0.68f;
            unsigned expoente = objeto->superficie.expoente;
            float r = 0.0f, g = 0.0f, b = 0.0f;
            //calcula a contribuicao de cada luz na cor
            unsigned luzesLigadas = 0;
            
            for (unsigned k=0; k < pontosDeLuz.size(); k++) {
                if (pontosDeLuz.at(k).estado == LIGADA) {
                    luzesLigadas++;
                    vec3 direcaoLuz = normalize(pontosDeLuz.at(k).posicao - vertice);
                    vec3 difusaEspecular = 
                        calcularDifusa(direcaoLuz, normal, difusa) +
                        calcularEspecular(direcao, pontosDeLuz.at(k).posicao, vertice, normal, especular, expoente);
                    r += difusaEspecular.r;
                    g += difusaEspecular.g;
                    b += difusaEspecular.b;
     
                }
            }
            //cout << "luzes ligadas:" << luzesLigadas << endl;
            luzesLigadas == 0 ? luzesLigadas = 1 : luzesLigadas = luzesLigadas; //evita divisao por zero
            vec3 mediasComponentes = vec3(r / luzesLigadas, g / luzesLigadas, b / luzesLigadas);
             
            cor = cor * mediasComponentes;
         
        }
        if (temSombra(vertice, pontosDeLuz , objetos, objeto)) {
            cor = cor * ATENUACAO;
        }
    }

    return cor;
}

vec3 calcularDifusa(vec3 direcaoLuz, vec3 normal, vec3 difusa) {
    float teta = std::max(dot(direcaoLuz, normal), 0.0f);
    return difusa * teta;    
}

vec3 calcularEspecular(vec3 direcao, vec3 direcaoLuz, vec3 vertice, vec3 normal, vec3 especularRGB, unsigned expoente) {
    vec3 v = normalize(direcao - vertice);
    vec3 r = normalize(reflect(-direcaoLuz, normal));
    float omega = std::max(dot(v, r), 0.0f);
    return especularRGB * (float)pow(omega, expoente);
}

vec3 calcularDirecaoLuz(vec3 vertice, vec3 posicaoLuz) {
    return normalize(posicaoLuz - vertice);
}

/*
Verifica na cena se do ponto tocado ao ponto de luz existe intersecao com algum outro objeto da cena
*/
bool temSombra(vec3 vertice, vector<PontoDeLuz> pontosDeLuz, vector<ObjetoImplicito*> objetos, ObjetoImplicito* objetoTocado) {
    float t1 = INFINITO, t0 = INFINITO, t = INFINITO;
    bool retorno = false;
    for (unsigned k=0; k < pontosDeLuz.size(); k++) {
        if (pontosDeLuz.at(k).estado == LIGADA) {
            for(unsigned i=0; i < objetos.size(); i++) {
                if (objetos.at(i) != objetoTocado) {
                    vec3 direcaoLuz = calcularDirecaoLuz(vertice, pontosDeLuz.at(k).posicao);
                    if (objetos.at(i)->intersecao(vertice, direcaoLuz, t1, t0)) {
                        return true;// saida do loop duplo
                    } else {
                        return false;
                    }
                } 
            }
        }
    }
    return retorno;
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