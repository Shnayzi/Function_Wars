#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/keyboard.h>

// largura do sprite do mago: 424 / 4 = 106
// altura do sprite do mago: 632 / 4 = 158

int main() {
    al_init();
    al_init_font_addon();
    al_init_image_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY* display = al_create_display(1280, 720);
    al_set_window_position(display, 200, 200);
    al_set_window_title(display, "Function Wars");

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);

    ALLEGRO_BITMAP* mage = al_load_bitmap("./mage_spritesheet.png");
	ALLEGRO_BITMAP* background1 = al_load_bitmap("./background1.png");

    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    float frame = 0.0f;
    int pos_x = 70, pos_y = 450;
    int current_frame_y = 0;
    bool redraw = true;

    bool keys[ALLEGRO_KEY_MAX] = { false };

    while (true) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            keys[event.keyboard.keycode] = true;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            keys[event.keyboard.keycode] = false;
        }
        else if (event.type == ALLEGRO_EVENT_TIMER) {
            bool moving = false;

            if (keys[ALLEGRO_KEY_UP]) {
                current_frame_y = 158 * 3;
                pos_y -= 5;
                moving = true;
            }
            if (keys[ALLEGRO_KEY_DOWN]) {
                current_frame_y = 0;
                pos_y += 5;
                moving = true;
            }
            if (keys[ALLEGRO_KEY_LEFT]) {
                current_frame_y = 158;
                pos_x -= 5;
                moving = true;
            }
            if (keys[ALLEGRO_KEY_RIGHT]) {
                current_frame_y = 158 * 2;
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

            redraw = true;
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            al_clear_to_color(al_map_rgb(255, 255, 255));
            al_draw_bitmap(background1, 0, 0, 0);
            al_draw_bitmap_region(mage, 106 * (int)frame, current_frame_y, 106, 158, pos_x, pos_y, 0);
            al_draw_text(font, al_map_rgb(0, 0, 0), 5, 5, 0, "Function Wars");
            al_flip_display();
        }
    }

    al_destroy_bitmap(mage);
    al_destroy_timer(timer);
    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);

    return 0;
}