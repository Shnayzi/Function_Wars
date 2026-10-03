#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/keyboard.h>
#include "funcoes.h"

bool checar_colisao(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    if (x1 + w1 > x2 &&
        x1 < x2 + w2 &&
        y1 + h1 > y2 &&
        y1 < y2 + h2) {
        return true;
    }
    return false;
}

int display_alt = 1280;
int display_larg = 720;

ParametrosFuncao funcao_jogador;

int coeficiente_selecionado = 0;
float valor_a = -0.004f;
float valor_b = 3;
float valor_c = 0;
char entrada_a[20] = "";
bool digitando_a = true;

int main() {
    al_init();
    al_init_font_addon();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();

    ALLEGRO_DISPLAY* display = al_create_display(display_alt, display_larg);
    al_set_window_position(display, 200, 200);
    al_set_window_title(display, "Function Wars");

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);

    ALLEGRO_BITMAP* mage = al_load_bitmap("./mage_spritesheet.png");
    ALLEGRO_BITMAP* alvo = al_load_bitmap("./alvo.png");
    ALLEGRO_BITMAP* background1 = al_load_bitmap("./background1.png");

    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_mouse_event_source());
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    bool redraw = true;

    //informacoes do mago
    float frame = 0.0f;
    int pos_x = 80, pos_y = 430;
    int current_frame_y = 0;
    int pontos_vida = 3;


    //informacoes do inimigo
    float inimigo_x = 980.0f;
    float inimigo_y = 400.0f;
    float inimigo_largura = 106.0f;
    float inimigo_altura = 159.0f;

    //essa parte toda é do projetil
    float tiro_hitbox_x = pos_x;
    float tiro_hitbox_y = pos_y;
    float tiro_hitbox_larg = 16;
    float tiro_hitbox_alt = 16;
    bool  projetil_ativo = false;
    float projetil_origem_x = 0.0f;
    float projetil_origem_y = 0.0f;
    float projetil_distancia = 0.0f;
    float projetil_velocidade = 10.0f;
    ParametrosFuncao funcao_do_disparo;
    funcao_do_disparo.tipo = FUNCAO_QUADRATICA;
    funcao_do_disparo.a = valor_a;
    funcao_do_disparo.b = valor_b;
    funcao_do_disparo.c = valor_c;



    bool keys[ALLEGRO_KEY_MAX] = { false };

    while (true) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }
        if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            keys[event.keyboard.keycode] = true;
        }
        // aqui faz a variavel do coeficiente a alterar de valor
        if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (digitando_a) {
                if (event.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
                    int tamanho = strlen(entrada_a);
                    if (tamanho > 0) {
                        entrada_a[tamanho - 1] = '\0';
                    }
                }
                else {
                    char c = (char)event.keyboard.unichar;
                    if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
                        int tamanho = strlen(entrada_a);
                        if (tamanho < 19) {
                            entrada_a[tamanho] = c;
                            entrada_a[tamanho + 1] = '\0';
                        }
                    }
                }
            }
        }

        // aqui faz o projetil ser lançado
        if (event.keyboard.keycode == ALLEGRO_KEY_SPACE && !projetil_ativo) {

            projetil_ativo = true;

            projetil_origem_x = pos_x + 90;
            projetil_origem_y = pos_y + 80;

            projetil_distancia = 0;
        }
        if (event.type == ALLEGRO_EVENT_KEY_UP) {
            keys[event.keyboard.keycode] = false;
        }
        if (event.type == ALLEGRO_EVENT_TIMER) {
            bool moving = false;

            if (keys[ALLEGRO_KEY_LEFT] || keys[ALLEGRO_KEY_A]) {
                current_frame_y = 159;
                pos_x -= 5;
                moving = true;
            }
            if (keys[ALLEGRO_KEY_RIGHT] || keys[ALLEGRO_KEY_D]) {
                current_frame_y = 159 * 2;
                pos_x += 5;
                moving = true;
            }

            if (moving) {
                frame += 0.2f;
                if (frame >= 4.0f) {
                    frame -= 4.0f;
                }
            }
            else {
                frame = 0.0f;
            }
            if (pos_x >= 300) {
                pos_x -= 5;
            }
            if (pos_x <= 0) {
                pos_x += 5;
            }

            redraw = true;

            //aqui faz o projetil se locomover de acordo com a funcao que ta ali em cima, faz a hitbox dele e checa a colisao com o inimigo
            if (projetil_ativo) {

                projetil_distancia += projetil_velocidade;

                float y = calcular_y(projetil_distancia, funcao_do_disparo);

                tiro_hitbox_x = projetil_origem_x + projetil_distancia;
                tiro_hitbox_y = projetil_origem_y - y;

                if (tiro_hitbox_x > display_alt) {
                    projetil_ativo = false;
                }
            }
            //se acertar o projetil desaparece e mostra no terminal que ele acertou
            if (projetil_ativo) {

                if (checar_colisao(tiro_hitbox_x - 8, tiro_hitbox_y - 8, tiro_hitbox_larg, tiro_hitbox_alt, inimigo_x, inimigo_y, inimigo_largura, inimigo_altura)) {
                    projetil_ativo = false;

                    printf("ACERTOU!\n");
                }
            }
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            al_clear_to_color(al_map_rgb(255, 255, 255));
            al_draw_bitmap(background1, 0, 0, 0);

            calcular_y(projetil_distancia, funcao_do_disparo);

            // Desenha a trajetória da função
            desenhar_trajetoria(pos_x + 90, pos_y + 80, 1000, funcao_do_disparo, al_map_rgb(30, 80, 220));

            // vai ta pequeno, mas quando voce abre e digita, vai ter um A: que recebe os numeros que o usuario digitar
            al_draw_text(font, al_map_rgb(0, 0, 0), 50, 50, 0, "A:");
            al_draw_text(font, al_map_rgb(0, 0, 0), 80, 50, 0, entrada_a);

            al_draw_bitmap_region(mage, 106 * (int)frame, current_frame_y, 106, 159, pos_x, pos_y, 0);
            //aqui desenha o projetil que é um circulo vermelho pequeno
            if (projetil_ativo) {
                al_draw_filled_circle(tiro_hitbox_x, tiro_hitbox_y, 8, al_map_rgb(255, 0, 0));
            }
            al_draw_bitmap_region(alvo, 0, 159, 106, 159, inimigo_x, inimigo_y, 0);

            //hitbox do inimigo
            al_draw_rectangle(inimigo_x, inimigo_y,
                inimigo_x + inimigo_largura,
                inimigo_y + inimigo_altura,
                al_map_rgb(255, 0, 0), 1.0f);

            //aqui vai ser incrementado mais pra frente, mas vai ter que colocar a mecanica de vida
            al_draw_textf(font, al_map_rgb(0, 0, 0), 5, 5, 0, "Pontos de vida %d", pontos_vida);
            al_draw_text(font, al_map_rgb(30, 30, 30), 500, 40, 0, "FASE 1: Ajuste a funcao e acerte o alvo!");

            al_flip_display();
        }
    }

    al_destroy_bitmap(mage);
    al_destroy_bitmap(alvo);
    al_destroy_bitmap(background1);
    al_destroy_timer(timer);
    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);

    al_shutdown_primitives_addon();
    al_shutdown_image_addon();
    al_shutdown_font_addon();

    return 0;
}