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
    vec3 atenuacao = vec3(1.0);
    ObjetoImplicito* objeto = NULL;
    unsigned int recursoes = 0;
    vector<IntersecaoObjeto> intersecoesObjetos;

    vec3 cor; 
    vec3 normal = vec3(0); 
    vec3 vertice = vec3(0);

    IntersecaoObjeto intersecaoObjeto;

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
        cor = objeto->superficie.corRGB;
        vertice = origem + (direcao * t);
        normal = objeto->calcularNormal(origem, direcao, t);
        //cout << "Objeto tocado" << endl;
        //cout << "Objeto espelhamento" << objeto->superficie.espelhamento << endl;
        //cout << "Recursoes" << objeto->superficie.espelhamento << endl;
        if (objeto->superficie.espelhamento && nivel < MAX_RECURSOES) {
            //cout << "Objeto espelhado" << endl;
            vec3 raioRefletido = reflect(direcao, normal);
            raioRefletido = normalize(raioRefletido); 
            cor = tracarRaio(vertice, raioRefletido, objetos, posicaoLuz, nivel + 1);
        //} else if (objeto->superficie.transparencia && nivel < MAX_RECURSOES) { 
            //calculo da refracao

            //return cor;
        } else {
            cor = objeto->superficie.corRGB;
            ambiente = objeto->superficie.ambienteRGB;
            difusa = objeto->superficie.difusaRGB;
            especular = objeto->superficie.especularRGB;

            vec3 phong = calcularPhong(origem, direcao, posicaoLuz, normal, vertice, difusa, especular);

            cor =  cor * phong;
        }
    }

    return cor;
}

vec3 calcularPhong(vec3 origem, vec3 direcao, vec3 posicaoLuz, 
  vec3 normal, vec3 vertice, vec3 difusa, vec3 especular) {
    vec3 l = normalize(posicaoLuz - vertice);
    float teta = std::max(dot(l, normal), 0.0f);

    vec3 v = normalize(direcao - vertice);
    vec3 refletido = reflect(-l, normal);
    vec3 r = normalize(refletido);
    float omega = std::max(dot(v, r), 0.0f); 

    return  difusa * teta + especular * (float)pow(omega, 40);
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