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

#define LARGURA_INTERNA 1280.0f
#define ALTURA_INTERNA  720.0f

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
    TELA_PAUSA,
    TELA_FASE_CONCLUIDA
} EstadoJogo;

typedef struct {
    float x, y, distancia, angulo;
    bool ativo;
} Projetil;

typedef struct {
    TipoFuncao tipo;
    const char* nome;
    const char* descricao;
    const char* formula;
    float alvo_x, alvo_y;
    bool tem_obstaculo;
    float obs_x, obs_y, obs_w, obs_h;
    bool tem_obstaculo2;
    float obs2_x, obs2_y, obs2_w, obs2_h;
} DadosFase;

const DadosFase fases[4] = {
    { FUNCAO_LINEAR, "FASE 1: FUNCAO LINEAR", "Desafio da Linha Reta", "y = ax + b", 1040.0f, CHAO_Y - (ALVO_DIAMETRO / 2.0f) - 60.0f, false, 0, 0, 0, 0, false, 0, 0, 0, 0 },
    { FUNCAO_QUADRATICA, "FASE 2: FUNCAO QUADRATICA", "Salte por cima da Muralha!", "y = ax^2 + bx + c", 1040.0f, CHAO_Y - (ALVO_DIAMETRO / 2.0f) - 60.0f, true, 580.0f, 300.0f, 60.0f, 300.0f, false, 0, 0, 0, 0 },
    { FUNCAO_EXPONENCIAL, "FASE 3: FUNCAO EXPONENCIAL", "Curva de Subida Veloz", "y = a^x", 980.0f, 250.0f, false, 0, 0, 0, 0, false, 0, 0, 0, 0 },
    { FUNCAO_TRIGONOMETRICA, "FASE 4: ONDA SENOIDAL", "Ziguezague entre duas muralhas!", "y = a sen(bx) + c", 1080.0f, 420.0f, true, 480.0f, 340.0f, 50.0f, 260.0f, true, 740.0f, 80.0f, 60.0f, 300.0f }
};

float calcular_y_ajustado(float x, ParametrosFuncao f) {
    if (f.tipo == FUNCAO_LINEAR) {
        return (f.a * x) + f.b;
    }
    else if (f.tipo == FUNCAO_QUADRATICA) {
        return ((f.a / 1000.0f) * x * x) + (f.b * x) + f.c;
    }
    else if (f.tipo == FUNCAO_EXPONENCIAL) {
        if (f.a <= 0.0f) return 0.0f;
        float taxa = (f.a * 0.0012f);
        return 20.0f * (expf(taxa * x) - 1.0f);
    }
    else if (f.tipo == FUNCAO_TRIGONOMETRICA) {
        float freq = f.b / 1000.0f;
        return (f.a * sinf(freq * x)) + f.c;
    }
    return 0.0f;
}

bool validar_parametros_local(ParametrosFuncao f, const char** motivo) {
    float y_zero = calcular_y_ajustado(0, f);
    if (!isfinite(y_zero)) {
        if (motivo) *motivo = "Erro matematico na origem.";
        return false;
    }
    return true;
}

void desenhar_trajetoria_local(float ox, float oy, float max_x, ParametrosFuncao f, ALLEGRO_COLOR cor) {
    float x_ant = ox;
    float y_base = calcular_y_ajustado(0, f);
    float y_ant = oy;

    for (float d = 2.0f; d <= max_x; d += 4.0f) {
        float px = ox + d;
        float py = oy - (calcular_y_ajustado(d, f) - y_base);

        if (!isfinite(py)) break;

        if (py > -8000.0f && py < 8000.0f && y_ant > -8000.0f && y_ant < 8000.0f) {
            al_draw_line(x_ant, y_ant, px, py, cor, 2.0f);
        }

        x_ant = px;
        y_ant = py;
    }
}

bool colisao_segmento_circulo(float x1, float y1, float x2, float y2, float cx, float cy, float raio) {
    float dx = x2 - x1; float dy = y2 - y1;
    float d = dx * dx + dy * dy; float t = 0;
    if (d > 0.0001f) t = ((cx - x1) * dx + (cy - y1) * dy) / d;
    if (t < 0) t = 0; if (t > 1) t = 1;
    float px = x1 + t * dx; float py = y1 + t * dy;
    float ex = cx - px; float ey = cy - py;
    return ex * ex + ey * ey <= raio * raio;
}

bool colisao_circulo_retangulo(float cx, float cy, float raio, float rx, float ry, float rw, float rh) {
    float px = (cx < rx) ? rx : (cx > rx + rw ? rx + rw : cx);
    float py = (cy < ry) ? ry : (cy > ry + rh ? ry + rh : cy);
    float dx = cx - px;
    float dy = cy - py;
    return (dx * dx + dy * dy) < (raio * raio);
}

bool campo_usado(TipoFuncao tipo, int campo) {
    if (campo == 0) return true;
    if (tipo == FUNCAO_EXPONENCIAL) return false;
    if (tipo == FUNCAO_LINEAR && campo == 2) return false;
    return true;
}

int proximo_campo(TipoFuncao tipo, int atual) {
    for (int i = 1; i <= 3; i++) {
        int c = (atual + i) % 3;
        if (campo_usado(tipo, c)) return c;
    }
    return 0;
}

bool converter_float(const char* texto, float* valor) {
    char* fim;
    if (!texto[0] || !strcmp(texto, "-") || !strcmp(texto, ".")) return false;
    double numero = strtod(texto, &fim);
    if (*fim != '\0') return false;
    *valor = (float)numero; return true;
}

bool montar_previa(TipoFuncao tipo, char entradas[3][32], ParametrosFuncao* f) {
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

void resetar_projetil(Projetil* p) {
    p->x = 0; p->y = 0; p->distancia = 0; p->angulo = 0; p->ativo = false;
}

void adicionar_caractere(char* texto, size_t capacidade, int unichar) {
    if (unichar < 0 || unichar > 127) return;
    char c = (char)unichar; size_t tamanho = strlen(texto);
    if (tamanho + 1 >= capacidade) return;
    if (c == '-' && tamanho != 0) return;
    if (c == '.' && strchr(texto, '.')) return;
    if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
        texto[tamanho] = c; texto[tamanho + 1] = '\0';
    }
}

void apagar_ultimo(char* texto) {
    size_t tamanho = strlen(texto);
    if (tamanho > 0) texto[tamanho - 1] = '\0';
}

bool mouse_na_caixa(float mx, float my, float x1, float y1, float x2, float y2) {
    return mx >= x1 && mx <= x2 && my >= y1 && my <= y2;
}

void desenhar_prego(float x, float y) {
    al_draw_filled_circle(x, y, 3.5f, al_map_rgb(60, 60, 65));
    al_draw_filled_circle(x - 1, y - 1, 1.5f, al_map_rgb(180, 180, 190));
}

void desenhar_tabua(float x1, float y1, float x2, float y2, ALLEGRO_COLOR base, ALLEGRO_COLOR veio, int semente) {
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

void desenhar_placa(float x1, float y1, float x2, float y2) {
    al_draw_filled_rounded_rectangle(x1 + 6, y1 + 6, x2 + 6, y2 + 6, 12, 12, al_map_rgba(0, 0, 0, 90));
    desenhar_tabua(x1, y1, x2, y2, COR_MADEIRA_ESCURA, COR_VEIO_ESCURO, 1);
    al_draw_rounded_rectangle(x1, y1, x2, y2, 12, 12, COR_BORDA, 4);
    desenhar_prego(x1 + 10, y1 + 10); desenhar_prego(x2 - 10, y1 + 10);
    desenhar_prego(x1 + 10, y2 - 10); desenhar_prego(x2 - 10, y2 - 10);
}

void desenhar_medalhao(ALLEGRO_FONT* font, float cx, float cy, const char* texto) {
    al_draw_filled_circle(cx, cy, 18, al_map_rgb(60, 60, 65));
    al_draw_filled_circle(cx, cy, 15, al_map_rgb(95, 95, 105));
    al_draw_circle(cx, cy, 15, al_map_rgb(190, 190, 200), 1.5f);
    al_draw_text(font, COR_CREME, cx, cy - 5, ALLEGRO_ALIGN_CENTER, texto);
}

void texto_com_sombra(ALLEGRO_FONT* font, float x, float y, int alinhamento, const char* texto) {
    al_draw_text(font, COR_TEXTO, x + 1, y + 1, alinhamento, texto);
    al_draw_text(font, COR_CREME, x, y, alinhamento, texto);
}

ALLEGRO_BITMAP* carregar_bitmap_recurso(const char* nome) {
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

void desenhar_tela_selecao_completa(ALLEGRO_FONT* font, ALLEGRO_BITMAP* bg_selecao) {
    al_clear_to_color(al_map_rgb(25, 25, 35));

    if (bg_selecao) {
        al_draw_scaled_bitmap(bg_selecao, 0, 0,
            al_get_bitmap_width(bg_selecao),
            al_get_bitmap_height(bg_selecao),
            0, 0, LARGURA_TELA, ALTURA_TELA, 0);
    }

    desenhar_placa(180, 30, 1100, 120);
    desenhar_tabua(200, 42, 1080, 108, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 3);
    texto_com_sombra(font, 640, 52, ALLEGRO_ALIGN_CENTER, "FUNCTION WARS - SELECAO DE FASES");
    texto_com_sombra(font, 640, 78, ALLEGRO_ALIGN_CENTER, "Escolha um desafio para jogar (Clique ou use as teclas 1, 2, 3 ou 4):");

    float pos_fases[4][4] = {
        { 160, 160, 600, 380 },
        { 680, 160, 1120, 380 },
        { 160, 420, 600, 640 },
        { 680, 420, 1120, 640 }
    };

    const char* nums[4] = { "1", "2", "3", "4" };

    for (int i = 0; i < 4; i++) {
        float x1 = pos_fases[i][0];
        float y1 = pos_fases[i][1];
        float x2 = pos_fases[i][2];
        float y2 = pos_fases[i][3];

        desenhar_placa(x1, y1, x2, y2);
        desenhar_tabua(x1 + 15, y1 + 15, x2 - 15, y2 - 15, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 12 + i);

        desenhar_medalhao(font, x1 + 50, y1 + 55, nums[i]);

        texto_com_sombra(font, x1 + 90, y1 + 38, 0, fases[i].nome);
        al_draw_text(font, COR_TEXTO, x1 + 90, y1 + 65, 0, fases[i].descricao);
        al_draw_textf(font, COR_FORMULA, x1 + 90, y1 + 100, 0, "Formula: %s", fases[i].formula);

        desenhar_tabua(x1 + 90, y1 + 140, x2 - 40, y1 + 185, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 50 + i);
        texto_com_sombra(font, (x1 + 90 + x2 - 40) / 2.0f, y1 + 155, ALLEGRO_ALIGN_CENTER, "JOGAR FASE");
    }

    texto_com_sombra(font, 640, 675, ALLEGRO_ALIGN_CENTER, "ESC = Voltar ao Menu Inicial");
}

void desenhar_menu_parametros(ALLEGRO_FONT* font, int fase_idx, int campo, char entradas[3][32], bool erro_ativo, const char* erro) {
    static const char* letras[3] = { "A", "B", "C" };
    static const char* rotulos[3] = { "Valor de A", "Valor de B", "Valor de C" };
    char titulo[64];
    TipoFuncao tipo = fases[fase_idx].tipo;

    desenhar_placa(20, 30, 400, 335);
    desenhar_tabua(40, 42, 380, 80, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 2);
    snprintf(titulo, sizeof(titulo), "%s", fases[fase_idx].nome);
    texto_com_sombra(font, 56, 48, 0, titulo);
    texto_com_sombra(font, 56, 62, 0, fases[fase_idx].formula);

    for (int i = 0; i < 3; i++) {
        float y = 92 + i * 45;
        desenhar_tabua(40, y, 380, y + 38, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 20 + i);
        if (i == campo && campo_usado(tipo, i)) al_draw_rounded_rectangle(40, y, 380, y + 38, 6, 6, al_map_rgb(255, 230, 120), 3);
        desenhar_medalhao(font, 62, y + 19, letras[i]);

        if (!campo_usado(tipo, i)) {
            al_draw_text(font, al_map_rgb(110, 90, 70), 86, y + 12, 0, "Nao utilizado nesta fase");
        }
        else {
            al_draw_text(font, COR_TEXTO, 86, y + 6, 0, rotulos[i]);
            al_draw_textf(font, COR_FORMULA, 86, y + 21, 0, "%s%s", entradas[i], i == campo ? "_" : "");
        }
    }
    texto_com_sombra(font, 40, 246, 0, "ENTER = confirmar parametros");
    texto_com_sombra(font, 40, 264, 0, "TAB = trocar campo");
    texto_com_sombra(font, 40, 282, 0, "ESC = voltar para selecao de fases");

    if (erro_ativo) {
        al_draw_filled_rounded_rectangle(40, 298, 380, 326, 6, 6, al_map_rgb(255, 225, 225));
        al_draw_rounded_rectangle(40, 298, 380, 326, 6, 6, al_map_rgb(150, 30, 30), 2);
        al_draw_text(font, al_map_rgb(200, 30, 30), 210, 308, ALLEGRO_ALIGN_CENTER, erro);
    }
}

void desenhar_estado(EstadoJogo estado, ALLEGRO_FONT* font, ALLEGRO_BITMAP* mage, ALLEGRO_BITMAP* alvo_bmp, ALLEGRO_BITMAP* bg_fases[4], ALLEGRO_BITMAP* intro, ALLEGRO_BITMAP* bg_selecao, ALLEGRO_BITMAP* proj_bmp, int fase_idx, char entradas[3][32], int campo, ParametrosFuncao funcao, Projetil* projetil, int vidas, float frame, bool msg_ativa, const char* msg, float ox, float oy, float mx, float my) {
    if (estado == TELA_INICIAL) {
        al_clear_to_color(al_map_rgb(25, 25, 35));
        if (intro) al_draw_scaled_bitmap(intro, 0, 0, al_get_bitmap_width(intro), al_get_bitmap_height(intro), 0, 0, LARGURA_TELA, ALTURA_TELA, 0);

        desenhar_placa(380, 25, 900, 110);
        desenhar_tabua(400, 35, 880, 100, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 7);
        texto_com_sombra(font, 640, 48, ALLEGRO_ALIGN_CENTER, "FUNCTION WARS");
        texto_com_sombra(font, 640, 72, ALLEGRO_ALIGN_CENTER, "A magia da matematica esta em suas maos");

        desenhar_placa(690, 205, 1235, 315); desenhar_placa(720, 365, 1205, 465); desenhar_placa(795, 520, 1140, 605);
        texto_com_sombra(font, 962, 248, ALLEGRO_ALIGN_CENTER, "JOGAR"); texto_com_sombra(font, 962, 280, ALLEGRO_ALIGN_CENTER, "ENTER / Clique - iniciar");
        texto_com_sombra(font, 962, 403, ALLEGRO_ALIGN_CENTER, "COMO JOGAR"); texto_com_sombra(font, 962, 435, ALLEGRO_ALIGN_CENTER, "T - instrucoes");
        texto_com_sombra(font, 967, 553, ALLEGRO_ALIGN_CENTER, "SAIR - ESC");
        if (mage) al_draw_scaled_bitmap(mage, ((int)frame % MAGO_FRAMES) * MAGO_FRAME_LARGURA, 0, MAGO_FRAME_LARGURA, MAGO_FRAME_ALTURA, 35, 183, 520, 417, 0);
        return;
    }

    if (estado == TELA_ESCOLHA_FUNCAO) {
        desenhar_tela_selecao_completa(font, bg_selecao);
        return;
    }

    al_clear_to_color(al_map_rgb(25, 25, 35));
    if (bg_fases[fase_idx]) {
        al_draw_scaled_bitmap(bg_fases[fase_idx], 0, 0, al_get_bitmap_width(bg_fases[fase_idx]), al_get_bitmap_height(bg_fases[fase_idx]), 0, 0, LARGURA_TELA, ALTURA_TELA, 0);
    }

    if (estado == TELA_TUTORIAL) {
        desenhar_placa(250, 115, 1030, 610);
        texto_com_sombra(font, 640, 145, ALLEGRO_ALIGN_CENTER, "COMO JOGAR");
        texto_com_sombra(font, 300, 220, 0, "1. Escolha a fase no menu (Linear, Quadratica, etc).");
        texto_com_sombra(font, 300, 275, 0, "2. Cada fase possui um cenario e uma funcao especifica.");
        texto_com_sombra(font, 300, 330, 0, "3. As fases com barreiras exigem curvas calculadas!");
        texto_com_sombra(font, 300, 385, 0, "4. ESPACO dispara; P pausa; R volta para selecao de fases.");
        texto_com_sombra(font, 640, 535, ALLEGRO_ALIGN_CENTER, "ENTER ou ESC para voltar");
        return;
    }

    if (mage) al_draw_scaled_bitmap(mage, ((int)frame % MAGO_FRAMES) * MAGO_FRAME_LARGURA, 0, MAGO_FRAME_LARGURA, MAGO_FRAME_ALTURA, mx, my, MAGO_LARGURA, MAGO_ALTURA, 0);

    float acx = fases[fase_idx].alvo_x;
    float acy = fases[fase_idx].alvo_y;
    float araio = ALVO_DIAMETRO / 2.0f;
    if (alvo_bmp) al_draw_scaled_bitmap(alvo_bmp, ALVO_SRC_POS, ALVO_SRC_POS, ALVO_SRC_TAM, ALVO_SRC_TAM, acx - araio, acy - araio, ALVO_DIAMETRO, ALVO_DIAMETRO, 0);

    if (fases[fase_idx].tem_obstaculo) {
        desenhar_tabua(fases[fase_idx].obs_x, fases[fase_idx].obs_y,
            fases[fase_idx].obs_x + fases[fase_idx].obs_w,
            fases[fase_idx].obs_y + fases[fase_idx].obs_h,
            COR_MADEIRA_ESCURA, COR_VEIO_ESCURO, 15);
        desenhar_prego(fases[fase_idx].obs_x + 10, fases[fase_idx].obs_y + 15);
        desenhar_prego(fases[fase_idx].obs_x + 10, fases[fase_idx].obs_y + fases[fase_idx].obs_h - 15);
    }

    if (fases[fase_idx].tem_obstaculo2) {
        desenhar_tabua(fases[fase_idx].obs2_x, fases[fase_idx].obs2_y,
            fases[fase_idx].obs2_x + fases[fase_idx].obs2_w,
            fases[fase_idx].obs2_y + fases[fase_idx].obs2_h,
            COR_MADEIRA_ESCURA, COR_VEIO_ESCURO, 28);
        desenhar_prego(fases[fase_idx].obs2_x + 10, fases[fase_idx].obs2_y + 15);
        desenhar_prego(fases[fase_idx].obs2_x + 10, fases[fase_idx].obs2_y + fases[fase_idx].obs2_h - 15);
    }

    if (estado == TELA_PARAMETROS) {
        ParametrosFuncao previa;
        if (!montar_previa(fases[fase_idx].tipo, entradas, &previa)) previa = (ParametrosFuncao){ fases[fase_idx].tipo, 0, 0, 0 };

        if (isfinite(calcular_y_ajustado(0, previa))) {
            desenhar_trajetoria_local(ox, oy, LARGURA_TELA - ox + 150, previa, al_map_rgb(80, 150, 255));
        }
        desenhar_menu_parametros(font, fase_idx, campo, entradas, msg_ativa, msg);
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

        desenhar_placa(20, 30, 390, 140);
        desenhar_tabua(40, 44, 370, 126, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 30);
        al_draw_textf(font, COR_TEXTO, 56, 56, 0, "%s", fases[fase_idx].nome);
        al_draw_textf(font, COR_TEXTO, 56, 76, 0, "Vidas do alvo: %d", vidas);
        al_draw_text(font, COR_FORMULA, 56, 102, 0, "ESPACO = disparar  |  R = fases");
    }

    if (estado == TELA_PAUSA) {
        al_draw_filled_rounded_rectangle(430, 235, 850, 475, 12, 12, al_map_rgba(0, 0, 0, 190));
        desenhar_placa(440, 245, 840, 465);
        texto_com_sombra(font, 640, 285, ALLEGRO_ALIGN_CENTER, "JOGO PAUSADO");
        texto_com_sombra(font, 640, 350, ALLEGRO_ALIGN_CENTER, "ENTER / P / ESC = continuar");
    }

    if (estado == TELA_FASE_CONCLUIDA) {
        al_draw_filled_rounded_rectangle(340, 180, 940, 540, 12, 12, al_map_rgba(0, 0, 0, 210));
        desenhar_placa(360, 200, 920, 520);
        desenhar_tabua(380, 220, 900, 290, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 10);
        texto_com_sombra(font, 640, 245, ALLEGRO_ALIGN_CENTER, "VITORIA! FASE CONCLUIDA!");

        texto_com_sombra(font, 640, 330, ALLEGRO_ALIGN_CENTER, fases[fase_idx].nome);
        texto_com_sombra(font, 640, 360, ALLEGRO_ALIGN_CENTER, "O alvo foi derrotado com exito matematico!");

        desenhar_tabua(450, 410, 830, 470, COR_MADEIRA_CLARA, COR_VEIO_CLARO, 35);
        texto_com_sombra(font, 640, 430, ALLEGRO_ALIGN_CENTER, "Clique ou ENTER / ESPACO para Selecao de Fases");
    }

    if (estado == TELA_JOGO && msg_ativa) {
        al_draw_filled_rounded_rectangle(490, 106, 790, 176, 8, 8, al_map_rgba(0, 0, 0, 90));
        desenhar_tabua(484, 100, 784, 170, COR_MADEIRA_MEDIA, COR_VEIO_MEDIO, 40);
        texto_com_sombra(font, 634, 131, ALLEGRO_ALIGN_CENTER, msg);
    }
}

void atualizar_projetil(Projetil* p, ParametrosFuncao funcao, int fase_idx, float ox, float oy, int* vidas, bool* m_ativa, float* m_tempo, const char** msg, EstadoJogo* estado) {
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

    if (!isfinite(p->y) || p->y > CHAO_Y + 100 || p->x > LARGURA_TELA + 100 || p->x < -200) {
        p->ativo = false;
    }
    p->angulo = atan2f(p->y - y_ant, p->x - x_ant);

    float acx = fases[fase_idx].alvo_x;
    float acy = fases[fase_idx].alvo_y;
    float araio = ALVO_DIAMETRO / 2.0f;

    if (p->ativo && colisao_segmento_circulo(x_ant, y_ant, p->x, p->y, acx, acy, araio + RAIO_PROJETIL)) {
        p->ativo = false; (*vidas)--;
        if (*vidas <= 0) {
            *vidas = 3;
            *estado = TELA_FASE_CONCLUIDA;
        }
        else {
            *msg = "ACERTO NO ALVO!";
            *m_tempo = 2;
            *m_ativa = true;
        }
    }

    if (p->ativo && fases[fase_idx].tem_obstaculo) {
        if (colisao_circulo_retangulo(p->x, p->y, RAIO_PROJETIL,
            fases[fase_idx].obs_x, fases[fase_idx].obs_y,
            fases[fase_idx].obs_w, fases[fase_idx].obs_h))
        {
            p->ativo = false;
            *msg = "BATEU NA MURALHA! AJUSTE A CURVA!";
            *m_tempo = 2.5f;
            *m_ativa = true;
        }
    }

    if (p->ativo && fases[fase_idx].tem_obstaculo2) {
        if (colisao_circulo_retangulo(p->x, p->y, RAIO_PROJETIL,
            fases[fase_idx].obs2_x, fases[fase_idx].obs2_y,
            fases[fase_idx].obs2_w, fases[fase_idx].obs2_h))
        {
            p->ativo = false;
            *msg = "BATEU NO TETO SUSPENSO! ONDULE POR BAIXO!";
            *m_tempo = 2.5f;
            *m_ativa = true;
        }
    }
}

int main(void) {
    if (!al_init()) return 1;
    al_init_font_addon(); al_init_image_addon(); al_init_primitives_addon();
    al_install_keyboard(); al_install_mouse();
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    ALLEGRO_DISPLAY* display = al_create_display(1920, 1080);
    if (!display) {
        display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    }

    int disp_w = al_get_display_width(display);
    int disp_h = al_get_display_height(display);
    float escala_x = (float)disp_w / LARGURA_INTERNA;
    float escala_y = (float)disp_h / ALTURA_INTERNA;

    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_scale_transform(&transform, escala_x, escala_y);
    al_use_transform(&transform);

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    al_set_window_title(display, "Function Wars");

    ALLEGRO_BITMAP* mage = carregar_bitmap_recurso("mage_spritesheet.png");
    ALLEGRO_BITMAP* alvo_bmp = carregar_bitmap_recurso("alvo.png");
    ALLEGRO_BITMAP* intro = carregar_bitmap_recurso("Tela_inicial.png");
    ALLEGRO_BITMAP* bg_selecao = carregar_bitmap_recurso("Tela_inicial.png");
    ALLEGRO_BITMAP* proj_bmp = carregar_bitmap_recurso("projetil.png");

    ALLEGRO_BITMAP* bg_fases[4];
    bg_fases[0] = carregar_bitmap_recurso("background1.png");
    bg_fases[1] = carregar_bitmap_recurso("background1.png");
    bg_fases[2] = carregar_bitmap_recurso("background3.jpeg");
    bg_fases[3] = carregar_bitmap_recurso("background4.jpeg");

    al_register_event_source(fila, al_get_display_event_source(display));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_mouse_event_source());
    al_start_timer(timer);

    EstadoJogo estado = TELA_INICIAL;
    int fase_idx = 0;
    char entradas[3][32] = { "", "", "" };
    int campo = 0, vidas = 3;
    ParametrosFuncao funcao = { FUNCAO_LINEAR, 0, 0, 0 };
    Projetil projetil; resetar_projetil(&projetil);

    float frame = 0, msg_tempo = 0;
    bool msg_ativa = false, executando = true, redraw = true;
    const char* msg = "";

    float pos_fases[4][4] = {
        { 160, 160, 600, 380 },
        { 680, 160, 1120, 380 },
        { 160, 420, 600, 640 },
        { 680, 420, 1120, 640 }
    };

    while (executando) {

        float mx = (fase_idx == 2) ? 200.0f : 100.0f;
        float my = (fase_idx == 2) ? 350.0f : (CHAO_Y - MAGO_ALTURA);
        float ox = mx + MAGO_LARGURA - 25.0f;
        float oy = my + 80.0f;

        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) executando = false;
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            frame += 0.2f;
            if (msg_ativa && (msg_tempo -= 1.0f / FPS) <= 0) msg_ativa = false;
            if (projetil.ativo && estado == TELA_JOGO) {
                atualizar_projetil(&projetil, funcao, fase_idx, ox, oy, &vidas, &msg_ativa, &msg_tempo, &msg, &estado);
            }
            redraw = true;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP && ev.mouse.button == 1) {
            float mouse_x = (float)ev.mouse.x / escala_x;
            float mouse_y = (float)ev.mouse.y / escala_y;

            if (estado == TELA_INICIAL) {
                if (mouse_na_caixa(mouse_x, mouse_y, 690, 205, 1235, 315)) estado = TELA_ESCOLHA_FUNCAO;
                else if (mouse_na_caixa(mouse_x, mouse_y, 720, 365, 1205, 465)) estado = TELA_TUTORIAL;
                else if (mouse_na_caixa(mouse_x, mouse_y, 795, 520, 1140, 605)) executando = false;
            }
            else if (estado == TELA_ESCOLHA_FUNCAO) {
                for (int i = 0; i < 4; i++) {
                    if (mouse_na_caixa(mouse_x, mouse_y, pos_fases[i][0], pos_fases[i][1], pos_fases[i][2], pos_fases[i][3])) {
                        fase_idx = i;
                        entradas[0][0] = entradas[1][0] = entradas[2][0] = '\0';
                        campo = 0;
                        msg_ativa = false;
                        estado = TELA_PARAMETROS;
                        break;
                    }
                }
            }
            else if (estado == TELA_PARAMETROS) {
                for (int i = 0; i < 3; i++) {
                    float y = 92 + i * 45;
                    if (mouse_na_caixa(mouse_x, mouse_y, 40, y, 380, y + 38)) {
                        if (campo_usado(fases[fase_idx].tipo, i)) {
                            campo = i;
                        }
                    }
                }
            }
            else if (estado == TELA_FASE_CONCLUIDA) {
                estado = TELA_ESCOLHA_FUNCAO;
                resetar_projetil(&projetil);
                msg_ativa = false;
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
                int idx = (key >= ALLEGRO_KEY_1 && key <= ALLEGRO_KEY_4) ? key - ALLEGRO_KEY_1 :
                    ((key >= ALLEGRO_KEY_PAD_1 && key <= ALLEGRO_KEY_PAD_4) ? key - ALLEGRO_KEY_PAD_1 : -1);
                if (idx >= 0 && idx < 4) {
                    fase_idx = idx;
                    entradas[0][0] = entradas[1][0] = entradas[2][0] = '\0';
                    campo = 0;
                    msg_ativa = false;
                    estado = TELA_PARAMETROS;
                }
            }
            else if (estado == TELA_PARAMETROS) {
                if (key == ALLEGRO_KEY_ESCAPE) { estado = TELA_ESCOLHA_FUNCAO; msg_ativa = false; }
                else if (key == ALLEGRO_KEY_BACKSPACE) apagar_ultimo(entradas[campo]);
                else if (key == ALLEGRO_KEY_TAB) campo = proximo_campo(fases[fase_idx].tipo, campo);
                else if (key == ALLEGRO_KEY_ENTER) {
                    ParametrosFuncao cand; const char* mot = NULL;
                    if (montar_previa(fases[fase_idx].tipo, entradas, &cand) && validar_parametros_local(cand, &mot)) {
                        funcao = cand;
                        resetar_projetil(&projetil);
                        vidas = 3;
                        msg_ativa = false;
                        estado = TELA_JOGO;
                    }
                    else {
                        msg = mot ? mot : "Preencha os campos!";
                        msg_ativa = true;
                        msg_tempo = 4;
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
            else if (estado == TELA_FASE_CONCLUIDA) {
                if (key == ALLEGRO_KEY_ENTER || key == ALLEGRO_KEY_SPACE || key == ALLEGRO_KEY_ESCAPE) {
                    estado = TELA_ESCOLHA_FUNCAO;
                    resetar_projetil(&projetil);
                    msg_ativa = false;
                }
            }
        }

        if (redraw && al_is_event_queue_empty(fila)) {
            redraw = false;
            desenhar_estado(estado, font, mage, alvo_bmp, bg_fases, intro, bg_selecao, proj_bmp, fase_idx, entradas, campo, funcao, &projetil, vidas, frame, msg_ativa, msg, ox, oy, mx, my);
            al_flip_display();
        }
    }

    if (bg_selecao) al_destroy_bitmap(bg_selecao);
    if (mage) al_destroy_bitmap(mage);
    if (intro) al_destroy_bitmap(intro);
    if (alvo_bmp) al_destroy_bitmap(alvo_bmp);
    for (int i = 0; i < 4; i++) {
        if (bg_fases[i]) al_destroy_bitmap(bg_fases[i]);
    }
    if (proj_bmp) al_destroy_bitmap(proj_bmp);

    al_destroy_timer(timer); al_destroy_font(font); al_destroy_display(display); al_destroy_event_queue(fila);
    return 0;
}