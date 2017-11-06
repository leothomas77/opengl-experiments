# Projeto 1 - Implementacao do Raycasting
## Descrição da Cena
Um plano de chão, dois pontos de luz, um conjunto de 5 esferas flutuando. Uma esfera vermelha ao centro.

Pelo menos uma esfera reflexiva

Pelo menos uma esfera translúcida com material refratário vidro (índice 1.5)

A câmera em movimento de rotação em relação ao eixo Y com uma esfera ao centro, dando a sensação de corpos em órbita da esfera central
## Arquivos
```
raytracing
├─ README.md - instruções do programa - você está este arquivo :)
├─ ETAPAS.md - etapas de desenvolvimento do programa
├─ objetos.h - definição das classes do programa
├─ globals.h - variáveis globais
├─ main.cpp - programa principal
├─ raycasting.h - definição das funções do raycasting
├─ raycasting.cpp - função recursiva para traçar raios e cálculo da cor
├─ Makefile.linux64 - makefile para Linux
├─ Makefile.MacOS - makefile para Mac
├─ makelinux.sh - script para compilação Linux
└─ makemac.sh - script para compilação Mac
```
## Compilação
Para Linux
```
./makelinux.sh
```
Para MacOS
```
./makemac.sh
```
##Utilizando o programa
Para executar o programa
```
./main
```
Aparecerá a quantidade de FPS na barra superior da janela do programa

Para ligar desligar fontes de luz

Teclar L ou l. Serão ligados e desligados os pontos de luz em uma sequência programada

Para selecionar uma esfera

Digitar um número de 0 a 5

Para movimentar uma esfera

Utilizar uma das setas direcionais Left, Right, Up e Down

OBS: esta opção não se mostrou performática. Tem que dar toques contínuos para funcionar

