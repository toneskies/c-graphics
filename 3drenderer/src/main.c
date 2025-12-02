#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "display.h"

/*
    TODO: Fix state for drawing and painting

*/

bool is_running = false;
bool is_painting = false;
bool is_erasing = false;
int mouse_x = 0;
int mouse_y = 0;
int prev_mouse_x = 0;
int prev_mouse_y = 0;

int BRUSH_THICKNESS = 2;

void draw_filled_circle(int center_x, int center_y, int radius,
                        uint32_t color) {
    for (int y = center_y - radius; y <= center_y + radius; y++) {
        for (int x = center_x - radius; x <= center_x + radius; x++) {
            int dx = x - center_x;
            int dy = y - center_y;

            if ((dx * dx + dy * dy) <= (radius * radius)) {
                if (x >= 0 && x < window_width && y >= 0 && y < window_height) {
                    draw_pixel(x, y, color);
                }
            }
        }
    }
}

void draw_line_segment(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 - x0);
    int dy = (y1 - y0);

    int steps = abs(dx) >= abs(dy) ? abs(dx) : abs(dy);

    float x_inc = (float)(x1 - x0) / steps;
    float y_inc = (float)(y1 - y0) / steps;

    float current_x = (float)x0;
    float current_y = (float)y0;

    for (int i = 0; i <= steps; i++) {
        int pixel_x = (int)round(current_x);
        int pixel_y = (int)round(current_y);

        if (pixel_x >= 0 && pixel_x < window_width && pixel_y >= 0 &&
            pixel_y < window_height) {
            draw_filled_circle(pixel_x, pixel_y, BRUSH_THICKNESS, color);
            current_x += x_inc;
            current_y += y_inc;
        }
    }
}

void setup(void) {
    // allocate the required memory in bytes to hold the color buffer
    color_buffer =
        (uint32_t*)malloc(window_width * window_height * sizeof(uint32_t));

    if (!color_buffer) {
        /* if it's a null pointer, fix this!*/
    } else {
        /* success, continue with your code.*/
    }

    // create a buffer texture for SDL that is used to hold the color buffer
    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             window_width, window_height);
}

void process_input(void) {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) is_running = false;
            if (event.key.keysym.sym == SDLK_c) clear_color_buffer(0xFF000000);
            if (event.key.keysym.sym == SDLK_EQUALS) BRUSH_THICKNESS++;
            if (event.key.keysym.sym == SDLK_MINUS) {
                if (BRUSH_THICKNESS > 1) BRUSH_THICKNESS--;
            }
            if (event.key.keysym.sym == SDLK_e) {
                is_erasing = true;
                is_painting = false;
            }
            if (event.key.keysym.sym == SDLK_d) {
                is_painting = true;
                is_erasing = false;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (is_erasing) {
                    is_painting = false;
                    prev_mouse_x = event.button.x;
                    prev_mouse_y = event.button.y;
                    printf("Erasing!\n");
                } else {
                    is_painting = true;
                    prev_mouse_x = event.button.x;
                    prev_mouse_y = event.button.y;
                }
                break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        is_painting = false;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                    printf("Mouse: (%d, %d)\n", mouse_x, mouse_y);
                    break;
            }
    }
}

void update(void) {
    if (is_painting || is_erasing) {
        uint32_t color = is_erasing ? 0xFF000000 : 0xFFFF0000;
        draw_line_segment(prev_mouse_x, prev_mouse_y, mouse_x, mouse_y, color);

        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;
    }
}

void render(void) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    // exercise 1
    draw_grid(0xFF333333, 10);

    // exercise 2
    draw_rect(100, 100, 100, 100, 0xFFFFC0CB);
    draw_pixel(100, 100, 0xFFFF0000);

    render_color_buffer();
    // clear_color_buffer(0xFF000000);

    SDL_RenderPresent(renderer);
}

int main(void) {
    // 1. initialize window
    is_running = initialize_window();

    // 2. set up buffer for game
    setup();

    // 3. game loop
    while (is_running) {
        // 3a. process user input
        process_input();

        // 3b. update timestep
        update();

        // 3c. render data to screen
        render();
    }

    // 4. collect garbage
    destroy_window();

    return 0;
}