#ifndef FUNCOES_H
#define FUNCOES_H

#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

typedef enum {
    FUNCAO_LINEAR,
    FUNCAO_QUADRATICA,
    FUNCAO_EXPONENCIAL,
    FUNCAO_TRIGONOMETRICA
} TipoFuncao;

/* Compatibilidade com versões anteriores do projeto. */
#define FUNCAO_LOGARITMICA FUNCAO_EXPONENCIAL

typedef struct {
    TipoFuncao tipo;
    float a;
    float b;
    float c;
} ParametrosFuncao;

/* Escalas independentes: mais resolução horizontal deixa as curvas visíveis. */
#define ESCALA_X 35.0f
#define ESCALA_Y 50.0f
#define LIMITE_Y_UNIDADES 10.0f

float calcular_y(float x_pixels, ParametrosFuncao f);
bool validar_parametros(ParametrosFuncao f, float alcance_pixels, const char** motivo);
void desenhar_trajetoria(float origem_x, float origem_y, float alcance_max,
    ParametrosFuncao f, ALLEGRO_COLOR cor);

#endif
