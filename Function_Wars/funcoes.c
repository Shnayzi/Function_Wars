#include <math.h>
#include "funcoes.h"

float calcular_y(float x, ParametrosFuncao f)
{
    switch (f.tipo)
    {
    case FUNCAO_LINEAR:
        return (f.a * x) + f.b;

    case FUNCAO_QUADRATICA:
        return (f.a * x * x) + (f.b * x) + f.c;

    case FUNCAO_LOGARITMICA:
        /* Mantido o comportamento do projeto original: a^x. */
        if (f.a <= 0.0f)
            return 0.0f;
        return powf(f.a, x);

    case FUNCAO_TRIGONOMETRICA:
        return (f.a * sinf(x)) + f.b;

    default:
        return 0.0f;
    }
}

void desenhar_trajetoria(
    float origem_x,
    float origem_y,
    float alcance_max,
    ParametrosFuncao f,
    ALLEGRO_COLOR cor)
{
    const float passo = 2.0f;

    float x_anterior = origem_x;
    float y_anterior = origem_y - calcular_y(0.0f, f);

    for (float x = passo; x <= alcance_max; x += passo)
    {
        float y_calculado = calcular_y(x, f);
        float x_atual = origem_x + x;
        float y_atual = origem_y - y_calculado;

        if (isfinite(y_anterior) && isfinite(y_atual))
        {
            al_draw_line(
                x_anterior, y_anterior,
                x_atual, y_atual,
                cor, 2.0f
            );
        }

        x_anterior = x_atual;
        y_anterior = y_atual;
    }
}
