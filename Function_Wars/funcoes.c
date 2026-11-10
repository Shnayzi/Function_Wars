#include <math.h>
#include <stdio.h>
#include "funcoes.h"

static bool numero_finito(float v) { return isfinite(v); }

/* X chega em pixels; a matemática usa unidades e Y volta para pixels. */
float calcular_y(float x_pixels, ParametrosFuncao f)
{
    const float x = x_pixels / ESCALA_X;
    float y = 0.0f;

    switch (f.tipo) {
    case FUNCAO_LINEAR:
        y = f.a * x + f.b;
        break;

    case FUNCAO_QUADRATICA:
        y = f.a * x * x + f.b * x + f.c;
        break;

    case FUNCAO_EXPONENCIAL:
        if (f.a <= 0.0f) return NAN;
        y = powf(f.a, x);
        break;

    case FUNCAO_TRIGONOMETRICA:
        y = f.a * sinf(f.b * x) + f.c;
        break;

    default:
        return NAN;
    }

    if (!numero_finito(y) || fabsf(y) > LIMITE_Y_UNIDADES)
        return NAN;

    return y * ESCALA_Y;
}

bool validar_parametros(ParametrosFuncao f, float alcance_pixels, const char** motivo)
{
    if (motivo) *motivo = NULL;

    if (!numero_finito(f.a) || !numero_finito(f.b) || !numero_finito(f.c)) {
        if (motivo) *motivo = "Digite somente numeros validos.";
        return false;
    }

    /* Limites ampliados, mas sem aceitar curvas degeneradas ou explosivas. */
    switch (f.tipo) {
    case FUNCAO_LINEAR:
        if (fabsf(f.a) > 3.0f || fabsf(f.b) > 10.0f) {
            if (motivo) *motivo = "Linear: A entre -3 e 3; B entre -10 e 10.";
            return false;
        }
        break;

    case FUNCAO_QUADRATICA:
        if (fabsf(f.a) > 0.8f || fabsf(f.b) > 3.0f || fabsf(f.c) > 6.0f) {
            if (motivo) *motivo = "Quadratica: A +/-0.8, B +/-3 e C +/-6.";
            return false;
        }
        if (fabsf(f.a) < 0.0001f) {
            if (motivo) *motivo = "Na quadratica, A nao pode ser zero.";
            return false;
        }
        break;

    case FUNCAO_EXPONENCIAL:
        if (f.a < 0.05f || f.a > 2.0f) {
            if (motivo) *motivo = "Exponencial: a base A deve ficar entre 0.05 e 2.";
            return false;
        }
        if (fabsf(f.a - 1.0f) < 0.001f) {
            if (motivo) *motivo = "A base 1 gera uma linha constante; use outro valor.";
            return false;
        }
        break;

    case FUNCAO_TRIGONOMETRICA:
        if (fabsf(f.a) > 6.0f || fabsf(f.b) > 3.0f || fabsf(f.c) > 6.0f) {
            if (motivo) *motivo = "Trigonometrica: A/C +/-6 e B +/-3.";
            return false;
        }
        if (fabsf(f.a) < 0.0001f || fabsf(f.b) < 0.0001f) {
            if (motivo) *motivo = "Na trigonometrica, A e B nao podem ser zero.";
            return false;
        }
        break;

    default:
        if (motivo) *motivo = "Tipo de funcao desconhecido.";
        return false;
    }

    if (alcance_pixels <= 0.0f || !numero_finito(alcance_pixels)) {
        if (motivo) *motivo = "Alcance da trajetoria invalido.";
        return false;
    }

    /* Testa toda a trajetoria, evitando valores fora da area vertical jogavel. */
    for (float x = 0.0f; x <= alcance_pixels; x += 2.0f) {
        if (!numero_finito(calcular_y(x, f))) {
            if (motivo) *motivo = "A curva sai da area jogavel. Ajuste os parametros.";
            return false;
        }
    }
    if (!numero_finito(calcular_y(alcance_pixels, f))) {
        if (motivo) *motivo = "A curva sai da area jogavel. Ajuste os parametros.";
        return false;
    }
    return true;
}

void desenhar_trajetoria(float origem_x, float origem_y, float alcance_max,
    ParametrosFuncao f, ALLEGRO_COLOR cor)
{
    const float passo = 2.0f;
    float x_anterior = origem_x;
    float y0 = calcular_y(0.0f, f);
    if (!isfinite(y0)) return;

    float y_anterior = origem_y;
    bool anterior_visivel = true;

    for (float x = passo; x <= alcance_max; x += passo) {
        float y = calcular_y(x, f);
        float px = origem_x + x;
        float py = origem_y - (y - y0);
        bool visivel = isfinite(y) && py >= 20.0f && py <= 680.0f;

        if (visivel && anterior_visivel)
            al_draw_line(x_anterior, y_anterior, px, py, cor, 2.0f);

        x_anterior = px;
        y_anterior = py;
        anterior_visivel = visivel;
    }
}
