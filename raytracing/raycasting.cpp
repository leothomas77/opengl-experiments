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
#define REFRACAO_VIDRO 1.0f / 1.55f //indice de refracao do vidro
#define REFRACAO_AR 1.0 

int nanoToMili(double nanoseconds) {
    return (int)(nanoseconds*0x431BDE82)>>18;
}

vec3 calcularRaio(vec3 origem, vec3 direcao) {
    return vec3(0, 0, 0);
}

void shade(vec3 origemRaio, vec3 direcaoRaio, float t) {
    return;
}


vec3 tracarRaio(vec3 origem, vec3 direcao, vector<ObjetoImplicito*> objetos, vec3 posicaoLuz, unsigned int nivel) {
    float t = INFINITO;
    vec3 especular = vec3(1.0);
    vec3 difusa = vec3(1.0);
    vec3 ambiente = vec3(1.0);
    float atenuacao = 0.68f;
    ObjetoImplicito* objeto = NULL;
    bool tocou = false;
    float indiceRefracao = REFRACAO_AR;
    //vector<IntersecaoObjeto> intersecoesObjetos;

    vec3 cor; 
    vec3 normal = vec3(0); 
    vec3 vertice = vec3(0);

    //IntersecaoObjeto intersecaoObjeto;

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

    //Ordena intersecoes e retorna o objeto com menor t
    /*
    for (unsigned i=0; i)
    std::sort(intersecoesObjetos.begin(), intersecoesObjetos.end());
    t = intersecoesObjetos.at(0).tIntersecao;
    objeto = objetos.at(intersecoesObjetos.at(0).indiceObjeto);

    intersecoesObjetos.clear();
    */
    
    if (objeto != NULL) {
   
        cor = vec3(0);
        vertice = origem + (direcao * t);
        normal = objeto->calcularNormal(origem, direcao, t);
        if (objeto->superficie.tipoSuperficie == refrataria && dot(direcao, normal) > 0) {
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
            cor = tracarRaio(vertice, raioRefletido, objetos, posicaoLuz, nivel + 1);
        } else if (objeto->superficie.tipoSuperficie == refrataria && nivel < MAX_RECURSOES) { 
            //calculo da refracao
            if (tocou) {
                //cout << "refrata vidro" << endl;
                indiceRefracao = REFRACAO_VIDRO;
            } else {
                //cout << "refrata ar" << endl;
                indiceRefracao = REFRACAO_AR;
            } 
            vec3 raioRefratado = normalize(refract(vertice, normal, indiceRefracao));
            cor = tracarRaio(vertice + raioRefratado * 0.1f, raioRefratado, objetos, posicaoLuz, nivel + 1);
        } else {
            cor = objeto->superficie.corRGB;
            ambiente = objeto->superficie.ambienteRGB;
            difusa = objeto->superficie.difusaRGB;
            especular = objeto->superficie.especularRGB;

            //vec3 phong = calcularPhong(origem, direcao, posicaoLuz, normal, vertice, difusa, especular);
            vec3 direcaoLuz = normalize(posicaoLuz - vertice);
            cor =  cor * ( 
                calcularDifusa(direcaoLuz, normal, difusa) +
                calcularEspecular(direcao, posicaoLuz, vertice, normal, especular)
            );
         
        }
    }
    if (temSombra(vertice, posicaoLuz, objetos)) {
        cor = cor * atenuacao;
    }

    return cor;
}

vec3 calcularDifusa(vec3 direcaoLuz, vec3 normal, vec3 difusa) {
    float teta = std::max(dot(direcaoLuz, normal), 0.0f);
    return difusa * teta;    
}

vec3 calcularEspecular(vec3 direcao, vec3 direcaoLuz, vec3 vertice, vec3 normal, vec3 especularRGB) {
    vec3 v = normalize(direcao - vertice);
    vec3 r = normalize(reflect(-direcaoLuz, normal));
    float omega = std::max(dot(v, r), 0.0f);
    return especularRGB * (float)pow(omega, 30);
}

bool temSombra(vec3 vertice, vec3 posicaoLuz, vector<ObjetoImplicito*> objetos) {
    float t1 = INFINITO, t0 = INFINITO;
    bool retorno = false;
    vec3 direcaoLuz = normalize(posicaoLuz - vertice);
    for(unsigned i=0; i < objetos.size(); i++) {

        if (objetos.at(i)->intersecao(vertice, direcaoLuz, t1, t0)) {
           // cout << "tem sombra" << endl;
            retorno = true;
            break;
        }
    }

    return retorno;
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