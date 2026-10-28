#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/keyboard.h>

#include "funcoes.h"

/*  PREDEFINIÇOES */
#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define FPS 60.0

#define CHAO_Y 600.0f

#define VELOCIDADE_PROJETIL 8.0f
#define RAIO_PROJETIL 12.0f

#define PROJ_FRAME 128
#define PROJ_FRAMES 11
#define PROJ_BOLA_RAIO 28.0f
#define PROJ_CENTRO_X 89.0f
#define PROJ_CENTRO_Y 64.0f

#define MAGO_FRAME_LARGURA 384
#define MAGO_FRAME_ALTURA 320
#define MAGO_FRAMES 5
#define MAGO_LARGURA 230.0f
#define MAGO_ALTURA 200.0f

#define ALVO_SRC_POS 128
#define ALVO_SRC_TAM 800
#define ALVO_DIAMETRO 130.0f

#define COR_MADEIRA_ESCURA al_map_rgb(105, 65, 30)
#define COR_MADEIRA_MEDIA al_map_rgb(160, 105, 55)
#define COR_MADEIRA_CLARA al_map_rgb(205, 155, 95)
#define COR_VEIO_ESCURO al_map_rgb(80, 48, 22)
#define COR_VEIO_MEDIO al_map_rgb(130, 82, 40)
#define COR_VEIO_CLARO al_map_rgb(175, 125, 70)
#define COR_BORDA al_map_rgb(55, 32, 14)
#define COR_CREME al_map_rgb(250, 235, 200)
#define COR_TEXTO al_map_rgb(50, 28, 12)
#define COR_FORMULA al_map_rgb(100, 62, 28)
#define COR_CORDA al_map_rgb(200, 170, 110)

/*  ESTRUTURAS E ESTADOS */
typedef enum
{
    TELA_ESCOLHA_FUNCAO,
    TELA_PARAMETROS,
    TELA_JOGO
} EstadoJogo;

typedef struct
{
    float x;
    float y;
    float distancia;
    float angulo;
    bool ativo;
} Projetil;

/*  FUNCOES AUXILIARES */
static bool checar_colisao(float x1, float y1, float r1,
    float x2, float y2, float r2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;

    return dx * dx + dy * dy < (r1 + r2) * (r1 + r2);
}

static void resetar_projetil(Projetil* p)
{
    p->x = 0;
    p->y = 0;
    p->distancia = 0;
    p->angulo = 0;
    p->ativo = false;
}

static const char* nome_funcao(TipoFuncao tipo)
{
    switch (tipo)
    {
    case FUNCAO_LINEAR:
        return "Linear";

    case FUNCAO_QUADRATICA:
        return "Quadratica";

    case FUNCAO_LOGARITMICA:
        return "Exponencial";

    case FUNCAO_TRIGONOMETRICA:
        return "Trigonometrica";

    default:
        return "Desconhecida";
    }
}

static const char* formula_funcao(TipoFuncao tipo)
{
    switch (tipo)
    {
    case FUNCAO_LINEAR:
        return "y = ax + b";

    case FUNCAO_QUADRATICA:
        return "y = ax^2 + bx + c";

    case FUNCAO_LOGARITMICA:
        return "y = a^x";

    case FUNCAO_TRIGONOMETRICA:
        return "y = a sen(x) + b";

    default:
        return "";
    }
}

static void adicionar_caractere(char* texto, size_t capacidade, int unichar)
{
    char c = (char)unichar;
    size_t tamanho = strlen(texto);

    if (tamanho + 1 >= capacidade)
        return;

    if (c == '-' && tamanho != 0)
        return;

    if (c == '.' && strchr(texto, '.'))
        return;

    if ((c >= '0' && c <= '9') || c == '.' || c == '-')
    {
        texto[tamanho] = c;
        texto[tamanho + 1] = '\0';
    }
}

static void apagar_ultimo(char* texto)
{
    size_t tamanho = strlen(texto);

    if (tamanho > 0)
        texto[tamanho - 1] = '\0';
}

static bool converter_float(const char* texto, float* valor)
{
    char* fim;
    double numero;

    if (!texto[0] ||
        !strcmp(texto, "-") ||
        !strcmp(texto, "."))
    {
        return false;
    }

    numero = strtod(texto, &fim);

    if (*fim != '\0')
        return false;

    *valor = (float)numero;

    return true;
}

/*  ENTRADA DE PARAMETROS */
static void desenhar_prego(float x, float y)
{
    al_draw_filled_circle(
        x,
        y,
        3.5f,
        al_map_rgb(60, 60, 65)
    );

    al_draw_filled_circle(
        x - 1,
        y - 1,
        1.5f,
        al_map_rgb(180, 180, 190)
    );
}

static void desenhar_tabua(
    float x1,
    float y1,
    float x2,
    float y2,
    ALLEGRO_COLOR base,
    ALLEGRO_COLOR veio,
    int semente)
{
    al_draw_filled_rounded_rectangle(
        x1,
        y1,
        x2,
        y2,
        6,
        6,
        base
    );

    for (float y = y1 + 4; y < y2 - 3; y += 5)
    {
        int v = (semente * 37 + (int)y * 13) % 100;

        float inicio = x1 + 6 + (v % 20);
        float fim = x2 - 6 - ((v * 7) % 30);

        al_draw_line(
            inicio,
            y,
            fim,
            y + (v % 3) - 1,
            veio,
            1.0f
        );
    }

    float nx = x1 +
        (x2 - x1) *
        (0.2f + (semente % 6) * 0.12f);

    float ny = (y1 + y2) / 2.0f;

    al_draw_ellipse(
        nx,
        ny,
        6,
        3,
        veio,
        1.0f
    );

    al_draw_ellipse(
        nx,
        ny,
        3,
        1.5f,
        veio,
        1.0f
    );

    al_draw_rounded_rectangle(
        x1,
        y1,
        x2,
        y2,
        6,
        6,
        COR_BORDA,
        2.0f
    );
}

static void desenhar_placa(
    float x1,
    float y1,
    float x2,
    float y2)
{
    float c1 = x1 + 55;
    float c2 = x2 - 55;

    al_draw_line(
        c1,
        0,
        c1,
        y1 + 4,
        COR_CORDA,
        4.0f
    );

    al_draw_line(
        c2,
        0,
        c2,
        y1 + 4,
        COR_CORDA,
        4.0f
    );

    al_draw_line(
        c1,
        0,
        c1,
        y1 + 4,
        al_map_rgb(150, 120, 70),
        1.0f
    );

    al_draw_line(
        c2,
        0,
        c2,
        y1 + 4,
        al_map_rgb(150, 120, 70),
        1.0f
    );

    al_draw_filled_rounded_rectangle(
        x1 + 6,
        y1 + 6,
        x2 + 6,
        y2 + 6,
        12,
        12,
        al_map_rgba(0, 0, 0, 90)
    );

    desenhar_tabua(
        x1,
        y1,
        x2,
        y2,
        COR_MADEIRA_ESCURA,
        COR_VEIO_ESCURO,
        1
    );

    al_draw_rounded_rectangle(
        x1,
        y1,
        x2,
        y2,
        12,
        12,
        COR_BORDA,
        4.0f
    );

    desenhar_prego(x1 + 10, y1 + 10);
    desenhar_prego(x2 - 10, y1 + 10);
    desenhar_prego(x1 + 10, y2 - 10);
    desenhar_prego(x2 - 10, y2 - 10);
}

static void desenhar_medalhao(
    ALLEGRO_FONT* font,
    float cx,
    float cy,
    const char* texto)
{
    al_draw_filled_circle(
        cx,
        cy,
        13,
        al_map_rgb(60, 60, 65)
    );

    al_draw_filled_circle(
        cx,
        cy,
        11,
        al_map_rgb(95, 95, 105)
    );

    al_draw_circle(
        cx,
        cy,
        11,
        al_map_rgb(190, 190, 200),
        1.0f
    );

    al_draw_text(
        font,
        COR_CREME,
        cx,
        cy - 4,
        ALLEGRO_ALIGN_CENTER,
        texto
    );
}

static void texto_com_sombra(
    ALLEGRO_FONT* font,
    float x,
    float y,
    int alinhamento,
    const char* texto)
{
    al_draw_text(
        font,
        COR_TEXTO,
        x + 1,
        y + 1,
        alinhamento,
        texto
    );

    al_draw_text(
        font,
        COR_CREME,
        x,
        y,
        alinhamento,
        texto
    );
}

/* BACKGROUND */
static void desenhar_fundo(ALLEGRO_BITMAP* background)
{
    al_clear_to_color(al_map_rgb(25, 25, 35));

    if (background)
    {
        al_draw_scaled_bitmap(
            background,
            0,
            0,
            al_get_bitmap_width(background),
            al_get_bitmap_height(background),
            0,
            0,
            LARGURA_TELA,
            ALTURA_TELA,
            0
        );
    }
}

/* INTERFACE */
static void desenhar_menu_escolha(ALLEGRO_FONT* font)
{
    static const TipoFuncao ordem[4] =
    {
        FUNCAO_LINEAR,
        FUNCAO_QUADRATICA,
        FUNCAO_LOGARITMICA,
        FUNCAO_TRIGONOMETRICA
    };

    static const char* numeros[4] =
    {
        "1",
        "2",
        "3",
        "4"
    };

    desenhar_placa(
        20,
        30,
        380,
        285
    );

    desenhar_tabua(
        40,
        42,
        360,
        80,
        COR_MADEIRA_MEDIA,
        COR_VEIO_MEDIO,
        2
    );

    texto_com_sombra(
        font,
        200,
        56,
        ALLEGRO_ALIGN_CENTER,
        "ESCOLHA A FUNCAO"
    );

    for (int i = 0; i < 4; i++)
    {
        float y1 = 92 + i * 45;
        float cy = y1 + 19;

        desenhar_tabua(
            40,
            y1,
            360,
            y1 + 38,
            COR_MADEIRA_CLARA,
            COR_VEIO_CLARO,
            10 + i
        );

        desenhar_medalhao(
            font,
            62,
            cy,
            numeros[i]
        );

        al_draw_text(
            font,
            COR_TEXTO,
            86,
            cy - 11,
            0,
            nome_funcao(ordem[i])
        );

        al_draw_text(
            font,
            COR_FORMULA,
            86,
            cy + 1,
            0,
            formula_funcao(ordem[i])
        );
    }
}

static void desenhar_menu_parametros(
    ALLEGRO_FONT* font,
    TipoFuncao tipo,
    int campo,
    char entradas[3][32],
    bool erro_ativo,
    const char* erro)
{
    static const char* letras[3] =
    {
        "A",
        "B",
        "C"
    };

    static const char* rotulos[3] =
    {
        "Valor de A",
        "Valor de B",
        "Valor de C"
    };

    char titulo[64];

    desenhar_placa(
        20,
        30,
        380,
        325
    );

    desenhar_tabua(
        40,
        42,
        360,
        80,
        COR_MADEIRA_MEDIA,
        COR_VEIO_MEDIO,
        2
    );

    snprintf(
        titulo,
        sizeof(titulo),
        "FUNCAO: %s",
        nome_funcao(tipo)
    );

    texto_com_sombra(
        font,
        56,
        48,
        0,
        titulo
    );

    texto_com_sombra(
        font,
        56,
        62,
        0,
        formula_funcao(tipo)
    );

    for (int i = 0; i < 3; i++)
    {
        float y1 = 92 + i * 45;
        float y2 = y1 + 38;

        desenhar_tabua(
            40,
            y1,
            360,
            y2,
            COR_MADEIRA_CLARA,
            COR_VEIO_CLARO,
            20 + i
        );

        if (i == campo)
        {
            al_draw_rounded_rectangle(
                40,
                y1,
                360,
                y2,
                6,
                6,
                al_map_rgb(255, 230, 120),
                3.0f
            );
        }

        desenhar_medalhao(
            font,
            62,
            y1 + 19,
            letras[i]
        );

        al_draw_text(
            font,
            COR_TEXTO,
            86,
            y1 + 6,
            0,
            rotulos[i]
        );

        al_draw_textf(
            font,
            COR_FORMULA,
            86,
            y1 + 21,
            0,
            "%s%s",
            entradas[i],
            i == campo ? "_" : ""
        );
    }

    texto_com_sombra(
        font,
        40,
        236,
        0,
        "ENTER = confirmar"
    );

    texto_com_sombra(
        font,
        40,
        252,
        0,
        "TAB = trocar campo"
    );

    texto_com_sombra(
        font,
        40,
        268,
        0,
        "ESC = voltar"
    );

    if (erro_ativo)
    {
        al_draw_filled_rounded_rectangle(
            40,
            284,
            360,
            312,
            6,
            6,
            al_map_rgb(255, 225, 225)
        );

        al_draw_rounded_rectangle(
            40,
            284,
            360,
            312,
            6,
            6,
            al_map_rgb(150, 30, 30),
            2.0f
        );

        al_draw_text(
            font,
            al_map_rgb(200, 30, 30),
            200,
            294,
            ALLEGRO_ALIGN_CENTER,
            erro
        );
    }
}

static void desenhar_painel_jogo(
    ALLEGRO_FONT* font,
    TipoFuncao tipo,
    int vidas)
{
    desenhar_placa(
        20,
        30,
        380,
        140
    );

    desenhar_tabua(
        40,
        44,
        360,
        126,
        COR_MADEIRA_CLARA,
        COR_VEIO_CLARO,
        30
    );

    al_draw_textf(
        font,
        COR_TEXTO,
        56,
        56,
        0,
        "Funcao: %s",
        nome_funcao(tipo)
    );

    al_draw_textf(
        font,
        COR_TEXTO,
        56,
        76,
        0,
        "Vidas do alvo: %d",
        vidas
    );

    al_draw_text(
        font,
        COR_FORMULA,
        56,
        102,
        0,
        "ESPACO = disparar"
    );

    al_draw_text(
        font,
        COR_FORMULA,
        210,
        102,
        0,
        "R = nova funcao"
    );
}

/*  MAGO */
static void desenhar_mago(
    ALLEGRO_BITMAP* mage,
    float mago_x,
    float mago_y,
    float frame)
{
    if (!mage)
        return;

    int quadro = (int)frame % MAGO_FRAMES;

    al_draw_scaled_bitmap(
        mage,
        quadro * MAGO_FRAME_LARGURA,
        0,
        MAGO_FRAME_LARGURA,
        MAGO_FRAME_ALTURA,
        mago_x,
        mago_y,
        MAGO_LARGURA,
        MAGO_ALTURA,
        0
    );
}

/*  ALVO */
static void desenhar_alvo(
    ALLEGRO_BITMAP* alvo_bmp,
    float alvo_cx,
    float alvo_cy,
    float alvo_raio)
{
    if (!alvo_bmp)
        return;

    al_draw_scaled_bitmap(
        alvo_bmp,
        ALVO_SRC_POS,
        ALVO_SRC_POS,
        ALVO_SRC_TAM,
        ALVO_SRC_TAM,
        alvo_cx - alvo_raio,
        alvo_cy - alvo_raio,
        ALVO_DIAMETRO,
        ALVO_DIAMETRO,
        0
    );
}

/*  PROJETIL */
static void desenhar_projetil(
    ALLEGRO_BITMAP* proj_bmp,
    Projetil* projetil)
{
    if (!projetil->ativo)
        return;

    if (!proj_bmp)
    {
        al_draw_filled_circle(
            projetil->x,
            projetil->y,
            RAIO_PROJETIL,
            al_map_rgb(255, 60, 30)
        );

        return;
    }

    float pulso =
        1.0f +
        0.12f *
        sinf(projetil->distancia * 0.15f);

    float escala =
        RAIO_PROJETIL /
        PROJ_BOLA_RAIO *
        pulso;

    int atual =
        (int)(
            projetil->distancia /
            VELOCIDADE_PROJETIL /
            2.0f
            ) % PROJ_FRAMES;

    al_draw_tinted_scaled_rotated_bitmap_region(
        proj_bmp,
        atual * PROJ_FRAME,
        0,
        PROJ_FRAME,
        PROJ_FRAME,
        al_map_rgb(255, 255, 255),
        PROJ_CENTRO_X,
        PROJ_CENTRO_Y,
        projetil->x,
        projetil->y,
        escala,
        escala,
        projetil->angulo,
        0
    );
}

/*  FUNCOES MATEMATICAS */
static void desenhar_trajetoria_atual(
    float origem_x,
    float origem_y,
    ParametrosFuncao funcao)
{
    desenhar_trajetoria(
        origem_x,
        origem_y,
        LARGURA_TELA - origem_x,
        funcao,
        al_map_rgb(80, 150, 255)
    );
}

/*  INICIALIZACAO */
int main(void)
{
    static const TipoFuncao tipos[4] =
    {
        FUNCAO_LINEAR,
        FUNCAO_QUADRATICA,
        FUNCAO_LOGARITMICA,
        FUNCAO_TRIGONOMETRICA
    };

    static const char* erros_param[3] =
    {
        "Valor de A invalido.",
        "Valor de B invalido.",
        "Valor de C invalido."
    };

    if (!al_init())
        return 1;

    al_init_font_addon();
    al_init_image_addon();
    al_init_primitives_addon();

    if (!al_install_keyboard())
        return 1;

    ALLEGRO_DISPLAY* display =
        al_create_display(
            LARGURA_TELA,
            ALTURA_TELA
        );

    ALLEGRO_FONT* font =
        al_create_builtin_font();

    ALLEGRO_TIMER* timer =
        al_create_timer(1.0 / FPS);

    ALLEGRO_EVENT_QUEUE* fila =
        al_create_event_queue();

    if (!display ||
        !font ||
        !timer ||
        !fila)
    {
        return 1;
    }

    al_set_new_bitmap_flags(
        ALLEGRO_MIN_LINEAR |
        ALLEGRO_MAG_LINEAR
    );

    ALLEGRO_BITMAP* mage =
        al_load_bitmap(
            "./mage_spritesheet.png"
        );

    ALLEGRO_BITMAP* alvo_bmp =
        al_load_bitmap(
            "./alvo.png"
        );

    ALLEGRO_BITMAP* background =
        al_load_bitmap(
            "./background1.png"
        );

    ALLEGRO_BITMAP* proj_bmp =
        al_load_bitmap(
            "./projetil.png"
        );

    al_set_window_title(
        display,
        "Function Wars"
    );

    al_register_event_source(
        fila,
        al_get_display_event_source(display)
    );

    al_register_event_source(
        fila,
        al_get_timer_event_source(timer)
    );

    al_register_event_source(
        fila,
        al_get_keyboard_event_source()
    );

    al_start_timer(timer);

    /* ESTADO DO JOGO */
    const float alvo_raio =
        ALVO_DIAMETRO / 2.0f;

    const float alvo_cx =
        1040.0f;

    const float alvo_cy =
        CHAO_Y -
        alvo_raio -
        60.0f;

    const float mago_x =
        100.0f;

    const float mago_y =
        CHAO_Y -
        MAGO_ALTURA;

    const float origem_x =
        mago_x +
        MAGO_LARGURA -
        25.0f;

    const float origem_y =
        mago_y +
        80.0f;

    EstadoJogo estado =
        TELA_ESCOLHA_FUNCAO;

    TipoFuncao tipo_escolhido =
        FUNCAO_QUADRATICA;

    char entradas[3][32] =
    {
        "",
        "",
        ""
    };

    int campo = 0;

    ParametrosFuncao funcao =
    {
        FUNCAO_QUADRATICA,
        0.001f,
        0.0f,
        0.0f
    };

    Projetil projetil;

    resetar_projetil(
        &projetil
    );

    int vidas = 3;

    float frame = 0.0f;

    bool msg_ativa = false;

    float msg_tempo = 0.0f;

    const char* msg = "";

    bool executando = true;
    bool redraw = true;

    /*  LOOP PRINCIPAL */
    while (executando)
    {
        ALLEGRO_EVENT ev;

        al_wait_for_event(
            fila,
            &ev
        );

        if (ev.type ==
            ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            executando = false;
        }
        else if (ev.type ==
            ALLEGRO_EVENT_TIMER)
        {
            frame += 0.2f;

            if (frame >= MAGO_FRAMES)
                frame = 0.0f;

            if (msg_ativa)
            {
                msg_tempo -= 1.0f / FPS;

                if (msg_tempo <= 0)
                {
                    msg_ativa = false;
                    msg = "";
                }
            }

            if (projetil.ativo)
            {
                float x_ant =
                    projetil.x;

                float y_ant =
                    projetil.y;

                projetil.distancia +=
                    VELOCIDADE_PROJETIL;

                projetil.x =
                    origem_x +
                    projetil.distancia;

                projetil.y =
                    origem_y -
                    calcular_y(
                        projetil.distancia,
                        funcao
                    );

                projetil.angulo =
                    atan2f(
                        projetil.y - y_ant,
                        projetil.x - x_ant
                    );

                if (checar_colisao(
                    projetil.x,
                    projetil.y,
                    RAIO_PROJETIL,
                    alvo_cx,
                    alvo_cy,
                    alvo_raio))
                {
                    projetil.ativo = false;

                    vidas--;

                    if (vidas <= 0)
                    {
                        msg =
                            "ALVO DERROTADO!";

                        msg_tempo = 3.0f;
                        vidas = 3;
                    }
                    else
                    {
                        msg =
                            "ACERTO!";

                        msg_tempo = 2.0f;
                    }

                    msg_ativa = true;
                }

                if (projetil.x >
                    LARGURA_TELA + 50)
                {
                    projetil.ativo = false;
                }
            }

            redraw = true;
        }
        else if (ev.type ==
            ALLEGRO_EVENT_KEY_CHAR)
        {
            int tecla =
                ev.keyboard.keycode;

            if (estado ==
                TELA_ESCOLHA_FUNCAO)
            {
                if (tecla >= ALLEGRO_KEY_1 &&
                    tecla <= ALLEGRO_KEY_4)
                {
                    tipo_escolhido =
                        tipos[
                            tecla -
                                ALLEGRO_KEY_1
                        ];

                    entradas[0][0] = '\0';
                    entradas[1][0] = '\0';
                    entradas[2][0] = '\0';

                    campo = 0;

                    msg_ativa = false;

                    estado =
                        TELA_PARAMETROS;
                }
            }
            else if (estado ==
                TELA_PARAMETROS)
            {
                if (tecla ==
                    ALLEGRO_KEY_ESCAPE)
                {
                    estado =
                        TELA_ESCOLHA_FUNCAO;

                    msg_ativa = false;
                }
                else if (tecla ==
                    ALLEGRO_KEY_BACKSPACE)
                {
                    apagar_ultimo(
                        entradas[campo]
                    );
                }
                else if (tecla ==
                    ALLEGRO_KEY_TAB)
                {
                    campo =
                        (campo + 1) % 3;
                }
                else if (tecla ==
                    ALLEGRO_KEY_ENTER)
                {
                    float* destino[3] =
                    {
                        &funcao.a,
                        &funcao.b,
                        &funcao.c
                    };

                    bool opcional[3] =
                    {
                        false,
                        tipo_escolhido ==
                        FUNCAO_LOGARITMICA,
                        tipo_escolhido !=
                        FUNCAO_QUADRATICA
                    };

                    bool valido = true;

                    for (int i = 0;
                        i < 3 && valido;
                        i++)
                    {
                        if (converter_float(
                            entradas[i],
                            destino[i]))
                        {
                            continue;
                        }

                        if (opcional[i])
                        {
                            *destino[i] =
                                0.0f;
                        }
                        else
                        {
                            valido = false;
                            msg =
                                erros_param[i];
                        }
                    }

                    if (valido)
                    {
                        funcao.tipo =
                            tipo_escolhido;

                        resetar_projetil(
                            &projetil
                        );

                        vidas = 3;
                        msg_ativa = false;

                        estado =
                            TELA_JOGO;
                    }
                    else
                    {
                        msg_ativa = true;
                        msg_tempo = 2.0f;
                    }
                }
                else
                {
                    adicionar_caractere(
                        entradas[campo],
                        sizeof(entradas[campo]),
                        ev.keyboard.unichar
                    );
                }
            }
            else if (estado ==
                TELA_JOGO)
            {
                if (tecla ==
                    ALLEGRO_KEY_R)
                {
                    estado =
                        TELA_ESCOLHA_FUNCAO;

                    resetar_projetil(
                        &projetil
                    );

                    msg_ativa = false;
                }
                else if (
                    tecla == ALLEGRO_KEY_SPACE &&
                    !projetil.ativo)
                {
                    projetil.ativo = true;
                    projetil.distancia = 0.0f;
                    projetil.angulo = 0.0f;
                    projetil.x = origem_x;
                    projetil.y = origem_y;
                }
            }

            redraw = true;
        }

        /*  RENDERIZACAO */
        if (redraw &&
            al_is_event_queue_empty(fila))
        {
            redraw = false;

            desenhar_fundo(
                background
            );

            if (estado ==
                TELA_JOGO ||
                estado ==
                TELA_PARAMETROS)
            {
                ParametrosFuncao previa;

                if (estado ==
                    TELA_JOGO)
                {
                    previa = funcao;
                }
                else
                {
                    previa =
                        (ParametrosFuncao)
                    {
                        tipo_escolhido,
                        0.001f,
                        0.0f,
                        0.0f
                    };
                }

                desenhar_trajetoria_atual(
                    origem_x,
                    origem_y,
                    previa
                );
            }

            desenhar_mago(
                mage,
                mago_x,
                mago_y,
                frame
            );

            desenhar_alvo(
                alvo_bmp,
                alvo_cx,
                alvo_cy,
                alvo_raio
            );

            desenhar_projetil(
                proj_bmp,
                &projetil
            );

            if (estado ==
                TELA_ESCOLHA_FUNCAO)
            {
                desenhar_menu_escolha(
                    font
                );
            }
            else if (estado ==
                TELA_PARAMETROS)
            {
                desenhar_menu_parametros(
                    font,
                    tipo_escolhido,
                    campo,
                    entradas,
                    msg_ativa,
                    msg
                );
            }
            else
            {
                desenhar_painel_jogo(
                    font,
                    funcao.tipo,
                    vidas
                );
            }

            if (estado ==
                TELA_JOGO &&
                msg_ativa)
            {
                al_draw_filled_rounded_rectangle(
                    506,
                    106,
                    786,
                    176,
                    8,
                    8,
                    al_map_rgba(0, 0, 0, 90)
                );

                desenhar_tabua(
                    500,
                    100,
                    780,
                    170,
                    COR_MADEIRA_MEDIA,
                    COR_VEIO_MEDIO,
                    40
                );

                desenhar_prego(
                    510,
                    110
                );

                desenhar_prego(
                    770,
                    110
                );

                desenhar_prego(
                    510,
                    160
                );

                desenhar_prego(
                    770,
                    160
                );

                texto_com_sombra(
                    font,
                    640,
                    131,
                    ALLEGRO_ALIGN_CENTER,
                    msg
                );
            }

            al_flip_display();
        }
    }

    /*  LIBERACAO DE RECURSOS */
    if (mage)
        al_destroy_bitmap(mage);

    if (alvo_bmp)
        al_destroy_bitmap(alvo_bmp);

    if (background)
        al_destroy_bitmap(background);

    if (proj_bmp)
        al_destroy_bitmap(proj_bmp);

    al_destroy_timer(timer);
    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_event_queue(fila);

    return 0;
}