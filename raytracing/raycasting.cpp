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

#define MAX_RECURSOES	6
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
            cor = objeto->superficie.corRGB; //cor de partida (nunca pode ser zero)
            
            float corSombreada = calcularSombras(vertice, normal, pontosDeLuz, objetos, objeto);
            cor -= corSombreada; //cor nao pode ser negativa

            if (objeto->superficie.tipoSuperficie == reflexiva) { 
                //cout << "refl" << endl;
                vec3 raioRefletido = normalize(reflect(direcao, normal));
                vec3 corRefletida = tracarRaio(vertice + normal * DESVIO, raioRefletido, objetos, pontosDeLuz, nivel);
                //cout << "Retornou corRefletida " << endl;
                cor *= corRefletida;
            } else if (objeto->superficie.tipoSuperficie == refrataria) {
                //cout << "refr" << endl;
                vec3 corRefratada = vec3(0.0f, 0.0f, 0.0f);

                vec3 desvio = normal * DESVIO;
                vec3 raioRefratado = calcularRaioRefratado2(direcao, normal, 2.4);
                vec3 verticeRefracao;
                if (dot(direcao, normal) < 0) {
                    verticeRefracao =  vertice - desvio;
                } else {
                    verticeRefracao = vertice + desvio;
                }
                corRefratada = tracarRaio(verticeRefracao , normalize(raioRefratado), objetos, pontosDeLuz, nivel);
                
               cor =  objeto->superficie.corRGB * corRefratada;
            } else {
                //cout << "phong" << endl;
                cor *= calcularContribuicoesLuzes(pontosDeLuz, vertice, normal, direcao, objeto);
                //cout << "Retornou corSolida" << endl;
            }

 
        }
    }   
    //cout << "Retorno cor (r: "<< cor.x*255 <<" g:"<< cor.y*255 << " b: "<< cor.z*255 << ")"<< endl;
    return cor;
}

vec3 calcularDifusa(vec3 direcaoLuz, vec3 normal, vec3 difusa) {
    float teta = std::max(dot(direcaoLuz, normal), 0.0f);
    return difusa * teta;    
}

vec3 calcularEspecular(vec3 direcao, vec3 direcaoLuz, vec3 vertice, vec3 normal, vec3 especularRGB, unsigned expoente) {
    vec3 v = normalize(direcao - vertice);
    vec3 r = normalize(reflect(direcaoLuz, normal));
    float omega = std::max(dot(v, r), 0.0f);
    return especularRGB * (float)pow(omega, expoente);
}

float clamp(float limiteInferior, float limiteSuperior, float valor) { 
    return std::max(limiteInferior, std::min(limiteSuperior, valor)); 
} 
//Calcular a parte do raio incidente que sera refratado
//Pela lei da conservacao, a parte restante sera refletida
float calcularFresnel(vec3 direcao, vec3 normal, float n1, float n2) {
    float retorno;
    float cosi = clamp(-1, 1, -1.0f * dot(direcao, normal)); //cosi = cos raio incidente
    //if (cosi > 0.0f) {//entrou no objeto
      //  std::swap(n1, n2);//inverte os indices
    //}
    float n = n1 / n2;
    //aplica Snell
    float sint2 = n * n * (1.0f - cosi * cosi);
    if (sint2 > 1.0f) {//reflexao total interna
        cout << "reflexao total interna" << endl;
        retorno = 1.0f;
    } else {
        float cost = sqrt(1.0f - sint2);
        float r0rth = (n1 * cosi - n2 * cost) / (n1 * cosi + n2 * cost);
        float rPar = (n2 * cosi - n1 * cost) / (n2 * cosi + n1 * cost);
        retorno = (r0rth * r0rth + rPar * rPar) / 2.0f;
    }
    return retorno;
}

vec3 calcularRaioRefratado2(vec3 direcao, vec3 normal, float indice) { 
    float cosi = dot(direcao, normal); 
    float etai, etat; 
    //cout << "Raio refratado cosi=" << cosi << endl;
    if (cosi > 0.0f) { // raio dentro > fora
        //cout << "Raio refratado saiu" << endl;
        etai = indice;
        etat = 1;
        normal = -1.0f * normal; 
    } else { // raio raio fora > dentro
        //cout << "Raio refratado entrou" << endl;
        etai = 1;
        etat = indice;
        cosi = -cosi; 
    } 
    float eta = etai / etat; 
    float k = 1 - eta * eta * (1 - cosi * cosi); 
    if (k < 0) {
        //cout << "TIR" << endl;
        return vec3(0);
    } else {
        //cout << "K = " << k << endl;
        return eta * direcao + (eta * cosi - sqrtf(k)) * normal;
    }

}

vec3 calcularRaioRefratado (vec3 direcao, vec3 normal) {
    float indice;
    //Normal > 0 = cos > 0  =entrou no objeto
    if (dot(direcao, normal) > 0) {
        normal = (-1.0f) * normal;
        indice = REFRACAO_VIDRO / REFRACAO_AR;
    } else {
     //Normal < 0 = cos < 0 = saiu do objeto
        indice = REFRACAO_AR / REFRACAO_VIDRO;
    }     
    return normalize(refract(direcao, normal, indice));
}

float mix(const float &a, const float &b, const float &mix) {
    return b * mix + a * (1 - mix);
}

float calcularFresnel(vec3 direcao, vec3 normal) {
    vec3 direcaoInversa = -1.0f * direcao;
    vec3 normalInversa = -1.0f * normal;
    float reflexaoFresnel = dot(direcao, normal);
    return mix(pow(1 - reflexaoFresnel, 3), 1, 0.1);
}

vec3 calcularDirecaoLuz(vec3 vertice, vec3 posicaoLuz) {
    return normalize(posicaoLuz - vertice);
}

vec3 calcularContribuicoesLuzes(vector<PontoDeLuz> pontosDeLuz, vec3 vertice, vec3 normal, vec3 direcao, ObjetoImplicito* objeto) {
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
    return vec3(r / luzesLigadas, g / luzesLigadas, b / luzesLigadas);
}

/*
Verifica na cena se do ponto tocado ao ponto de luz existe intersecao com algum outro objeto da cena
*/
float calcularSombras(vec3 vertice, vec3 normal, vector<PontoDeLuz> pontosDeLuz, vector<ObjetoImplicito*> objetos, ObjetoImplicito* objetoTocado) {
    for (unsigned k = 0; k < pontosDeLuz.size(); k++) {
        if (pontosDeLuz.at(k).estado == LIGADA) {
            vec3 raioObjetoLuz = normalize(pontosDeLuz.at(k).posicao - vertice);
            float distanciaLuz = length(raioObjetoLuz);
            float t = INFINITO;
            for (unsigned i = 0; i < objetos.size(); i++) {
                float t0 = INFINITO, t1 = INFINITO;
                if (objetos.at(i)->intersecao(vertice + normal * DESVIO, raioObjetoLuz, t0, t1) && objetos.at(i) != objetoTocado) {
                    return SOMBREAMENTO;
                }
            }
        }
    }
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
