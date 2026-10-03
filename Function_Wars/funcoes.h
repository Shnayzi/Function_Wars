#ifndef FUNCOES_H
#define FUNCOES_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>

typedef enum {
    FUNCAO_LINEAR,
    FUNCAO_QUADRATICA,
    FUNCAO_LOGARITMICA,
    FUNCAO_TRIGONOMETRICA
} TipoFuncao;

typedef struct {
    TipoFuncao tipo;
    float a;
    float b;
    float c;
} ParametrosFuncao;

float calcular_y(float x, ParametrosFuncao f);

void desenhar_trajetoria(float origem_x, float origem_y, float alcance_max,
    ParametrosFuncao f, ALLEGRO_COLOR cor);

#endif
