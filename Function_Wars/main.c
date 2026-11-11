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

#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define FPS 60.0
#define CHAO_Y 600.0f
#define VELOCIDADE_PROJETIL 12.0f
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

#define COR_MADEIRA_ESCURA al_map_rgb(105,65,30)
#define COR_MADEIRA_MEDIA al_map_rgb(160,105,55)
#define COR_MADEIRA_CLARA al_map_rgb(205,155,95)
#define COR_VEIO_ESCURO al_map_rgb(80,48,22)
#define COR_VEIO_MEDIO al_map_rgb(130,82,40)
#define COR_VEIO_CLARO al_map_rgb(175,125,70)
#define COR_BORDA al_map_rgb(55,32,14)
#define COR_CREME al_map_rgb(250,235,200)
#define COR_TEXTO al_map_rgb(50,28,12)
#define COR_FORMULA al_map_rgb(100,62,28)

typedef enum {
    TELA_INICIAL,
    TELA_TUTORIAL,
    TELA_ESCOLHA_FUNCAO,
    TELA_PARAMETROS,
    TELA_JOGO,
    TELA_PAUSA
} EstadoJogo;

typedef struct {
    float x, y, distancia, angulo;
    bool ativo;
} Projetil;

static const TipoFuncao tipos[4] = {
    FUNCAO_LINEAR,
    FUNCAO_QUADRATICA,
    FUNCAO_EXPONENCIAL,
    FUNCAO_TRIGONOMETRICA
};

/* --- LOGICA MATEMATICA E FISICA --- */
static float calcular_y_ajustado(float x, ParametrosFuncao f) {
    if (f.tipo == FUNCAO_LINEAR) {
        return (f.a * x) + f.b;
    }
    else if (f.tipo == FUNCAO_QUADRATICA) {
        return ((f.a / 1000.0f) * x * x) + (f.b * x) + f.c;
    }
    else if (f.tipo == FUNCAO_EXPONENCIAL) {
        float taxa = f.b / 1000.0f;
        return f.a * expf(taxa * x);
    }
    else if (f.tipo == FUNCAO_TRIGONOMETRICA) {
        float freq = f.b / 1000.0f;
        return (f.a * sinf(freq * x)) + f.c;
    }
    return 0.0f;
}

static bool validar_parametros_local(ParametrosFuncao f, const char** motivo) {
    float y_zero = calcular_y_ajustado(0, f);
    if (!isfinite(y_zero)) {
        if (motivo) *motivo = "Erro matematico na origem.";
        return false;
    }
    // Liberdade total para o jogador testar trajetorias insanas!
    return true;
}

static void desenhar_trajetoria_local(float ox, float oy, float max_x, ParametrosFuncao f, ALLEGRO_COLOR cor) {
    float x_ant = ox;
    float y_base = calcular_y_ajustado(0, f);
    float y_ant = oy;

    for (float d = 2.0f; d <= max_x; d += 4.0f) {
        float px = ox + d;
        float py = oy - (calcular_y_ajustado(d, f) - y_base);

        if (!isfinite(py)) break;

        // Desenha apenas se estiver em coordenadas razoaveis para o Allegro renderizar,
        // mas CONTINUA O LACO para calcular a volta da parabola mesmo que ultrapasse o teto!
        if (py > -8000.0f && py < 8000.0f && y_ant > -8000.0f && y_ant < 8000.0f) {
            al_draw_line(x_ant, y_ant, px, py, cor, 2.0f);
        }

        x_ant = px;
        y_ant = py;
    }
}

/* --- FUNCOES BASE --- */
static bool colisao_segmento_circulo(float x1, float y1, float x2, float y2, float cx, float cy, float raio) {
    float dx = x2 - x1; float dy = y2 - y1;
    float d = dx * dx + dy * dy; float t = 0;
    if (d > 0.0001f) t = ((cx - x1) * dx + (cy - y1) * dy) / d;
    if (t < 0) t = 0; if (t > 1) t = 1;
    float px = x1 + t * dx; float py = y1 + t * dy;
    float ex = cx - px; float ey = cy - py;
    return ex * ex + ey * ey <= raio * raio;
}

static bool campo_usado(TipoFuncao tipo, int campo) {
    if (campo == 0) return true;
    if (tipo == FUNCAO_EXPONENCIAL) return false;
    if (tipo == FUNCAO_LINEAR && campo == 2) return false;
    return true;
}

static int proximo_campo(TipoFuncao tipo, int atual) {
    for (int i = 1; i <= 3; i++) {
        int c = (atual + i) % 3;
        if (campo_usado(tipo, c)) return c;
    }
    return 0;
}

static bool converter_float(const char* texto, float* valor) {
    char* fim;
    if (!texto[0] || !strcmp(texto, "-") || !strcmp(texto, ".")) return false;
    double numero = strtod(texto, &fim);
    if (*fim != '\0') return false;
    *valor = (float)numero; return true;
}

static bool montar_previa(TipoFuncao tipo, char entradas[3][32], ParametrosFuncao* f) {
    float* destino[3] = { &f->a, &f->b, &f->c };
    bool preenchido = false;
    f->tipo = tipo; f->a = f->b = f->c = 0;

    for (int i = 0; i < 3; i++) {
        if (!campo_usado(tipo, i)) continue;
        if (entradas[i][0] == '\0') { *destino[i] = 0; continue; }
        if (!converter_float(entradas[i], destino[i])) return false;
        preenchido = true;
    }
    return preenchido;
}

static void resetar_projetil(Projetil* p) {
    p->x = 0; p->y = 0; p->distancia = 0; p->angulo = 0; p->ativo = false;
}

static const char* nome_funcao(TipoFuncao tipo) {
    switch (tipo) {
    case FUNCAO_LINEAR: return "Linear";
    case FUNCAO_QUADRATICA: return "Quadratica";
    case FUNCAO_EXPONENCIAL: return "Exponencial";
    case FUNCAO_TRIGONOMETRICA: return "Trigonometrica";
    default: return "Desconhecida";
    }
}

static const char* formula_funcao(TipoFuncao tipo) {
    switch (tipo) {
    case FUNCAO_LINEAR: return "y = ax + b";
    case FUNCAO_QUADRATICA: return "y = ax^2 + bx + c";
    case FUNCAO_EXPONENCIAL: return "y = a^x";
    case FUNCAO_TRIGONOMETRICA: return "y = a sen(bx) + c";
    default: return "";
    }
}

static void adicionar_caractere(char* texto, size_t capacidade, int unichar) {
    if (unichar < 0 || unichar > 127) return;
    char c = (char)unichar; size_t tamanho = strlen(texto);
    if (tamanho + 1 >= capacidade) return;
    if (c == '-' && tamanho != 0) return;
    if (c == '.' && strchr(texto, '.')) return;
    if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
        texto[tamanho] = c; texto[tamanho + 1] = '\0';
    }
}

static void apagar_ultimo(char* texto) {
    size_t tamanho = strlen(texto);
    if (tamanho > 0) texto[tamanho - 1] = '\0';
}

static bool mouse_na_caixa(float mx, float my, float x1, float y1, float x2, float y2) {
    return mx >= x1 && mx <= x2 && my >= y1 && my <= y2;
}

/* --- FUNCOES DE DESENHO --- */
static void desenhar_prego(float x, float y) {
    al_draw_filled_circle(x, y, 3.5f, al_map_rgb(60, 60, 65));
    al_draw_filled_circle(x - 1, y - 1, 1.5f, al_map_rgb(180, 180, 190));
}

static void desenhar_tabua(float x1, float y1, float x2, float y2, ALLEGRO_COLOR base, ALLEGRO_COLOR veio, int semente) {
    al_draw_filled_rounded_rectangle(x1, y1, x2, y2, 6, 6, base);
    for (float y = y1 + 4; y < y2 - 3; y += 5) {
        int v = (semente * 37 + (int)y * 13) % 100;
        float inicio = x1 + 6 + (v % 20);
        float fim = x2 - 6 - ((v * 7) % 30);
        al_draw_line(inicio, y, fim, y + (v % 3) - 1, veio, 1);
    }
    float nx = x1 + (x2 - x1) * (0.2f + (semente % 6) * 0.12f);
    float ny = (y1 + y2) / 2;
    al_draw_ellipse(nx, ny, 6, 3, veio, 1);
    al_draw_ellipse(nx, ny, 3, 1.5f, veio, 1);
    al_draw_rounded_rectangle(x1, y1, x2, y2, 6, 6, COR_BORDA, 2);
}

static void desenhar_placa(float x1, float y1, float x2, float y2) {
    al_draw_filled_rounded_rectangle(x1 + 6, y1 + 6, x2 + 6, y2 + 6, 12, 12, al_map_rgba(0, 0, 0, 90));
    desenhar_tabua(x1, y1, x2, y2, COR_MADEIRA_ESCURA, COR_VEIO_ESCURO, 1);
    al_draw_rounded_rectangle(x1, y1, x2, y2, 12, 12, COR_BORDA, 4);
    desenhar_prego(x1 + 10, y1 + 10); desenhar_prego(x2 - 10, y1 + 10);
    desenhar_prego(x1 + 10, y2 - 10); desenhar_prego(x2 - 10, y2 - 10);
}

static void desenhar_medalhao(ALLEGRO_FONT* font, float cx, float cy, const char* texto) {
    al_draw_filled_circle(cx, cy, 13, al_map_rgb(60, 60, 65));
    al_draw_filled_circle(cx, cy, 11, al_map_rgb(95, 95, 105));
    al_draw_circle(cx, cy, 11, al_map_rgb(190, 190, 200), 1);
    al_draw_text(font, COR_CREME, cx, cy - 4, ALLEGRO_ALIGN_CENTER, texto);
}

static void texto_com_sombra(ALLEGRO_FONT* font, float x, float y, int alinhamento, const char* texto) {
    al_draw_text(font, COR_TEXTO, x + 1, y + 1, alinhamento, texto);
    al_draw_text(font, COR_CREME, x, y, alinhamento, texto);
}

static ALLEGRO_BITMAP* carregar_bitmap_recurso(const char* nome) {
    ALLEGRO_PATH* caminho = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
    ALLEGRO_BITMAP* bitmap = NULL;
    if (caminho) {
        al_append_path_component(caminho, nome);
        bitmap = al_load_bitmap(al_path_cstr(caminho, ALLEGRO_NATIVE_PATH_SEP));
        al_destroy_path(caminho);
    }
    if (!bitmap) bitmap = al_load_bitmap(nome);
    return bitmap;
}

static void desenhar_menu_escolha(ALLEGRO_FONT* font) {
    desenhar_placa(20, 30, 380, 285);
    desenhar_tabua(40, 42, 360, 80, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 2);
    texto_com_sombra(font, 200, 56, ALLEGRO_ALIGN_CENTER, "ESCOLHA A FUNCAO");
    for (int i = 0; i < 4; i++) {
        float y = 92 + i * 45;
        desenhar_tabua(40, y, 360, y + 38, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 10 + i);
        const char* numero = i == 0 ? "1" : i == 1 ? "2" : i == 2 ? "3" : "4";
        desenhar_medalhao(font, 62, y + 19, numero);
        al_draw_text(font, COR_TEXTO, 86, y + 8, 0, nome_funcao(tipos[i]));
        al_draw_text(font, COR_FORMULA, 86, y + 22, 0, formula_funcao(tipos[i]));
    }
}

static void desenhar_menu_parametros(ALLEGRO_FONT* font, TipoFuncao tipo, int campo, char entradas[3][32], bool erro_ativo, const char* erro) {
    static const char* letras[3] = { "A", "B", "C" };
    static const char* rotulos[3] = { "Valor de A", "Valor de B", "Valor de C" };
    char titulo[64];

    desenhar_placa(20, 30, 380, 325);
    desenhar_tabua(40, 42, 360, 80, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 2);
    snprintf(titulo, sizeof(titulo), "FUNCAO: %s", nome_funcao(tipo));
    texto_com_sombra(font, 56, 48, 0, titulo);
    texto_com_sombra(font, 56, 62, 0, formula_funcao(tipo));

    for (int i = 0; i < 3; i++) {
        float y = 92 + i * 45;
        desenhar_tabua(40, y, 360, y + 38, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 20 + i);
        if (i == campo) al_draw_rounded_rectangle(40, y, 360, y + 38, 6, 6, al_map_rgb(255, 230, 120), 3);
        desenhar_medalhao(font, 62, y + 19, letras[i]);

        if (!campo_usado(tipo, i)) {
            al_draw_text(font, al_map_rgb(110, 90, 70), 86, y + 12, 0, "Nao utilizado");
        }
        else {
            al_draw_text(font, COR_TEXTO, 86, y + 6, 0, rotulos[i]);
            al_draw_textf(font, COR_FORMULA, 86, y + 21, 0, "%s%s", entradas[i], i == campo ? "_" : "");
        }
    }
    texto_com_sombra(font, 40, 236, 0, "ENTER = confirmar");
    texto_com_sombra(font, 40, 252, 0, "TAB = trocar campo");
    texto_com_sombra(font, 40, 268, 0, "ESC = voltar");

    if (erro_ativo) {
        al_draw_filled_rounded_rectangle(40, 284, 360, 312, 6, 6, al_map_rgb(255, 225, 225));
        al_draw_rounded_rectangle(40, 284, 360, 312, 6, 6, al_map_rgb(150, 30, 30), 2);
        al_draw_text(font, al_map_rgb(200, 30, 30), 200, 294, ALLEGRO_ALIGN_CENTER, erro);
    }
}

static void desenhar_estado(EstadoJogo estado, ALLEGRO_FONT* font, ALLEGRO_BITMAP* mage, ALLEGRO_BITMAP* alvo_bmp, ALLEGRO_BITMAP* bg, ALLEGRO_BITMAP* intro, ALLEGRO_BITMAP* proj_bmp, TipoFuncao tipo, char entradas[3][32], int campo, ParametrosFuncao funcao, Projetil* projetil, int vidas, float frame, bool msg_ativa, const char* msg, float ox, float oy, float mx, float my, float acx, float acy, float araio) {
    if (estado == TELA_INICIAL) {
        al_clear_to_color(al_map_rgb(25, 25, 35));
        if (intro) al_draw_scaled_bitmap(intro, 0, 0, al_get_bitmap_width(intro), al_get_bitmap_height(intro), 0, 0, LARGURA_TELA, ALTURA_TELA, 0);
        texto_com_sombra(font, 640, 70, ALLEGRO_ALIGN_CENTER, "A magia da matematica esta em suas maos");
        desenhar_placa(690, 205, 1235, 315); desenhar_placa(720, 365, 1205, 465); desenhar_placa(795, 520, 1140, 605);
        texto_com_sombra(font, 962, 248, ALLEGRO_ALIGN_CENTER, "JOGAR"); texto_com_sombra(font, 962, 280, ALLEGRO_ALIGN_CENTER, "ENTER - iniciar");
        texto_com_sombra(font, 962, 403, ALLEGRO_ALIGN_CENTER, "COMO JOGAR"); texto_com_sombra(font, 962, 435, ALLEGRO_ALIGN_CENTER, "T - instrucoes");
        texto_com_sombra(font, 967, 553, ALLEGRO_ALIGN_CENTER, "SAIR - ESC");
        if (mage) al_draw_scaled_bitmap(mage, ((int)frame % MAGO_FRAMES) * MAGO_FRAME_LARGURA, 0, MAGO_FRAME_LARGURA, MAGO_FRAME_ALTURA, 35, 183, 520, 417, 0);
        return;
    }

    al_clear_to_color(al_map_rgb(25, 25, 35));
    if (bg) al_draw_scaled_bitmap(bg, 0, 0, al_get_bitmap_width(bg), al_get_bitmap_height(bg), 0, 0, LARGURA_TELA, ALTURA_TELA, 0);

    if (estado == TELA_TUTORIAL) {
        desenhar_placa(250, 115, 1030, 610);
        texto_com_sombra(font, 640, 145, ALLEGRO_ALIGN_CENTER, "COMO JOGAR");
        texto_com_sombra(font, 300, 220, 0, "1. Escolha uma funcao matematica.");
        texto_com_sombra(font, 300, 275, 0, "2. Digite valores. Experimente numeros maiores (ex: 5, 10, 20).");
        texto_com_sombra(font, 300, 330, 0, "3. Veja a trajetoria antes de confirmar.");
        texto_com_sombra(font, 300, 385, 0, "4. ESPACO dispara; P pausa; R recomeca.");
        texto_com_sombra(font, 640, 535, ALLEGRO_ALIGN_CENTER, "ENTER ou ESC para voltar");
        return;
    }

    if (mage) al_draw_scaled_bitmap(mage, ((int)frame % MAGO_FRAMES) * MAGO_FRAME_LARGURA, 0, MAGO_FRAME_LARGURA, MAGO_FRAME_ALTURA, mx, my, MAGO_LARGURA, MAGO_ALTURA, 0);
    if (alvo_bmp) al_draw_scaled_bitmap(alvo_bmp, ALVO_SRC_POS, ALVO_SRC_POS, ALVO_SRC_TAM, ALVO_SRC_TAM, acx - araio, acy - araio, ALVO_DIAMETRO, ALVO_DIAMETRO, 0);

    if (estado == TELA_ESCOLHA_FUNCAO) {
        desenhar_menu_escolha(font);
    }
    else if (estado == TELA_PARAMETROS) {
        ParametrosFuncao previa;
        if (!montar_previa(tipo, entradas, &previa)) previa = (ParametrosFuncao){ tipo, 0, 0, 0 };

        // Desenha a linha SEMPRE, permitindo testes loucos
        if (isfinite(calcular_y_ajustado(0, previa))) {
            desenhar_trajetoria_local(ox, oy, LARGURA_TELA - ox + 150, previa, al_map_rgb(80, 150, 255));
        }
        desenhar_menu_parametros(font, tipo, campo, entradas, msg_ativa, msg);
    }
    else {
        desenhar_trajetoria_local(ox, oy, LARGURA_TELA - ox + 150, funcao, al_map_rgb(80, 150, 255));

        if (projetil->ativo) {
            float pulso = 1.0f + 0.12f * sinf(projetil->distancia * 0.15f);
            float escala = RAIO_PROJETIL / PROJ_BOLA_RAIO * pulso;
            int atual = (int)(projetil->distancia / VELOCIDADE_PROJETIL / 2.0f) % PROJ_FRAMES;
            al_draw_tinted_scaled_rotated_bitmap_region(proj_bmp, atual * PROJ_FRAME, 0, PROJ_FRAME, PROJ_FRAME,
                al_map_rgb(255, 255, 255), PROJ_CENTRO_X, PROJ_CENTRO_Y, projetil->x, projetil->y, escala, escala, projetil->angulo, 0);
        }

        desenhar_placa(20, 30, 380, 140);
        desenhar_tabua(40, 44, 360, 126, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 30);
        al_draw_textf(font, COR_TEXTO, 56, 56, 0, "Funcao: %s", nome_funcao(funcao.tipo));
        al_draw_textf(font, COR_TEXTO, 56, 76, 0, "Vidas do alvo: %d", vidas);
        al_draw_text(font, COR_FORMULA, 56, 102, 0, "ESPACO = disparar  |  R = nova");
    }

    if (estado == TELA_PAUSA) {
        al_draw_filled_rounded_rectangle(430, 235, 850, 475, 12, 12, al_map_rgba(0, 0, 0, 190));
        desenhar_placa(440, 245, 840, 465);
        texto_com_sombra(font, 640, 285, ALLEGRO_ALIGN_CENTER, "JOGO PAUSADO");
        texto_com_sombra(font, 640, 350, ALLEGRO_ALIGN_CENTER, "ENTER / P / ESC = continuar");
    }

    if (estado == TELA_JOGO && msg_ativa) {
        al_draw_filled_rounded_rectangle(506, 106, 786, 176, 8, 8, al_map_rgba(0, 0, 0, 90));
        desenhar_tabua(500, 100, 780, 170, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 40);
        texto_com_sombra(font, 640, 131, ALLEGRO_ALIGN_CENTER, msg);
    }
}

/* --- LOOP FÍSICO --- */
static void atualizar_projetil(Projetil* p, ParametrosFuncao funcao, float ox, float oy, float acx, float acy, float araio, int* vidas, bool* m_ativa, float* m_tempo, const char** msg) {
    if (!p->ativo) return;

    float x_ant = p->x; float y_ant = p->y;
    float d_atual = p->distancia;
    float y_atual = calcular_y_ajustado(d_atual, funcao);
    float y_prox = calcular_y_ajustado(d_atual + 1.0f, funcao);

    float derivada = y_prox - y_atual;
    float passo_x = VELOCIDADE_PROJETIL / sqrtf(1.0f + derivada * derivada);

    p->distancia += passo_x;
    p->x = ox + p->distancia;
    p->y = oy - (calcular_y_ajustado(p->distancia, funcao) - calcular_y_ajustado(0, funcao));

    // O Projetil NÃO morre mais se for muito alto. So morre se bater no chao, ou sair da tela pelos lados
    if (!isfinite(p->y) || p->y > CHAO_Y + 100 || p->x > LARGURA_TELA + 100 || p->x < -200) {
        p->ativo = false;
    }
    p->angulo = atan2f(p->y - y_ant, p->x - x_ant);

    if (p->ativo && colisao_segmento_circulo(x_ant, y_ant, p->x, p->y, acx, acy, araio + RAIO_PROJETIL)) {
        p->ativo = false; (*vidas)--;
        if (*vidas <= 0) { *msg = "ALVO DERROTADO!"; *m_tempo = 3; *vidas = 3; }
        else { *msg = "ACERTO!"; *m_tempo = 2; }
        *m_ativa = true;
    }
}

int main(void) {
    if (!al_init()) return 1;
    al_init_font_addon(); al_init_image_addon(); al_init_primitives_addon();
    al_install_keyboard(); al_install_mouse();
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    al_set_window_title(display, "Function Wars");

    ALLEGRO_BITMAP* mage = carregar_bitmap_recurso("mage_spritesheet.png");
    ALLEGRO_BITMAP* alvo_bmp = carregar_bitmap_recurso("alvo.png");
    ALLEGRO_BITMAP* bg = carregar_bitmap_recurso("background1.png");
    ALLEGRO_BITMAP* intro = carregar_bitmap_recurso("Tela_inicial.png");
    ALLEGRO_BITMAP* proj_bmp = carregar_bitmap_recurso("projetil.png");

    al_register_event_source(fila, al_get_display_event_source(display));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_mouse_event_source());
    al_start_timer(timer);

    const float alvo_raio = ALVO_DIAMETRO / 2.0f;
    const float acx = 1040.0f, acy = CHAO_Y - alvo_raio - 60.0f;
    const float mx = 100.0f, my = CHAO_Y - MAGO_ALTURA;
    const float ox = mx + MAGO_LARGURA - 25.0f, oy = my + 80.0f;

    EstadoJogo estado = TELA_INICIAL;
    TipoFuncao tipo = FUNCAO_QUADRATICA;
    char entradas[3][32] = { "", "", "" };
    int campo = 0, vidas = 3;
    ParametrosFuncao funcao = { FUNCAO_QUADRATICA, 0, 0, 0 };
    Projetil projetil; resetar_projetil(&projetil);

    float frame = 0, msg_tempo = 0;
    bool msg_ativa = false, executando = true, redraw = true;

    while (executando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) executando = false;
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            frame += 0.2f;
            if (msg_ativa && (msg_tempo -= 1.0f / FPS) <= 0) msg_ativa = false;
            if (projetil.ativo && estado == TELA_JOGO) atualizar_projetil(&projetil, funcao, ox, oy, acx, acy, alvo_raio, &vidas, &msg_ativa, &msg_tempo, &msg);
            redraw = true;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP && ev.mouse.button == 1) {
            float mouse_x = ev.mouse.x, mouse_y = ev.mouse.y;
            if (estado == TELA_INICIAL) {
                if (mouse_na_caixa(mouse_x, mouse_y, 690, 205, 1235, 315)) estado = TELA_ESCOLHA_FUNCAO;
                else if (mouse_na_caixa(mouse_x, mouse_y, 720, 365, 1205, 465)) estado = TELA_TUTORIAL;
                else if (mouse_na_caixa(mouse_x, mouse_y, 795, 520, 1140, 605)) executando = false;
            }
            else if (estado == TELA_ESCOLHA_FUNCAO) {
                for (int i = 0; i < 4; i++) {
                    if (mouse_na_caixa(mouse_x, mouse_y, 40, 92 + i * 45, 360, 92 + i * 45 + 38)) {
                        tipo = tipos[i]; entradas[0][0] = entradas[1][0] = entradas[2][0] = '\0';
                        campo = 0; msg_ativa = false; estado = TELA_PARAMETROS; break;
                    }
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            int key = ev.keyboard.keycode;
            if (estado == TELA_INICIAL) {
                if (key == ALLEGRO_KEY_ESCAPE) executando = false;
                else if (key == ALLEGRO_KEY_T) estado = TELA_TUTORIAL;
                else if (key == ALLEGRO_KEY_ENTER || key == ALLEGRO_KEY_1) estado = TELA_ESCOLHA_FUNCAO;
            }
            else if (estado == TELA_TUTORIAL) {
                if (key == ALLEGRO_KEY_ENTER || key == ALLEGRO_KEY_ESCAPE) estado = TELA_INICIAL;
            }
            else if (estado == TELA_ESCOLHA_FUNCAO) {
                if (key == ALLEGRO_KEY_ESCAPE) estado = TELA_INICIAL;
                int idx = (key >= ALLEGRO_KEY_1 && key <= ALLEGRO_KEY_4) ? key - ALLEGRO_KEY_1 : ((key >= ALLEGRO_KEY_PAD_1 && key <= ALLEGRO_KEY_PAD_4) ? key - ALLEGRO_KEY_PAD_1 : -1);
                if (idx >= 0 && idx < 4) {
                    tipo = tipos[idx]; entradas[0][0] = entradas[1][0] = entradas[2][0] = '\0';
                    campo = 0; msg_ativa = false; estado = TELA_PARAMETROS;
                }
            }
            else if (estado == TELA_PARAMETROS) {
                if (key == ALLEGRO_KEY_ESCAPE) { estado = TELA_ESCOLHA_FUNCAO; msg_ativa = false; }
                else if (key == ALLEGRO_KEY_BACKSPACE) apagar_ultimo(entradas[campo]);
                else if (key == ALLEGRO_KEY_TAB) campo = proximo_campo(tipo, campo);
                else if (key == ALLEGRO_KEY_ENTER) {
                    ParametrosFuncao cand; const char* mot = NULL;
                    if (montar_previa(tipo, entradas, &cand) && validar_parametros_local(cand, &mot)) {
                        funcao = cand; resetar_projetil(&projetil); vidas = 3; msg_ativa = false; estado = TELA_JOGO;
                    }
                    else {
                        msg = mot ? mot : "Preencha os campos!"; msg_ativa = true; msg_tempo = 4;
                    }
                }
                else adicionar_caractere(entradas[campo], sizeof(entradas[campo]), ev.keyboard.unichar);
            }
            else if (estado == TELA_JOGO) {
                if (key == ALLEGRO_KEY_ESCAPE || key == ALLEGRO_KEY_P) estado = TELA_PAUSA;
                else if (key == ALLEGRO_KEY_R) { estado = TELA_ESCOLHA_FUNCAO; resetar_projetil(&projetil); msg_ativa = false; }
                else if (key == ALLEGRO_KEY_SPACE && !projetil.ativo) {
                    projetil.ativo = true; projetil.distancia = 0; projetil.angulo = 0; projetil.x = ox;
                }
            }
            else if (estado == TELA_PAUSA) {
                if (key == ALLEGRO_KEY_ENTER || key == ALLEGRO_KEY_P || key == ALLEGRO_KEY_ESCAPE) estado = TELA_JOGO;
                else if (key == ALLEGRO_KEY_R) { estado = TELA_ESCOLHA_FUNCAO; resetar_projetil(&projetil); msg_ativa = false; }
            }
        }

        if (redraw && al_is_event_queue_empty(fila)) {
            redraw = false;
            desenhar_estado(estado, font, mage, alvo_bmp, bg, intro, proj_bmp, tipo, entradas, campo, funcao, &projetil, vidas, frame, msg_ativa, msg, ox, oy, mx, my, acx, acy, alvo_raio);
            al_flip_display();
        }
    }

    if (mage) al_destroy_bitmap(mage);
    if (intro) al_destroy_bitmap(intro);
    if (alvo_bmp) al_destroy_bitmap(alvo_bmp);
    if (bg) al_destroy_bitmap(bg);
    if (proj_bmp) al_destroy_bitmap(proj_bmp);

    al_destroy_timer(timer); al_destroy_font(font); al_destroy_display(display); al_destroy_event_queue(fila);
    return 0;
}