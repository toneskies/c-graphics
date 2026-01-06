#include "display.h"

#include <stdio.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
uint32_t* color_buffer = NULL;
SDL_Texture* color_buffer_texture = NULL;
int window_height = 600;
int window_width = 800;

bool initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Use SDL to query what is the fullscreen max. width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    window_width = 800;   // display_mode.w;
    window_height = 600;  // display_mode.h;

    // Created a SDL Window
    window =
        SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         window_width, window_height, SDL_WINDOW_BORDERLESS);

    if (!window) {
        fprintf(stderr, "Error creating SDL window.\n");
        return false;
    }

    // Create a SDL renderer
    renderer = SDL_CreateRenderer(window, -1, 0);
    // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    if (!renderer) {
        fprintf(stderr, "Error creating SDL renderer.\n");
        return false;
    }

    return true;
}

void render_color_buffer() {
    SDL_UpdateTexture(color_buffer_texture, NULL, color_buffer,
                      (int)(window_width * sizeof(uint32_t)));
    SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
}

void clear_color_buffer(uint32_t color) {
    for (int i = 0, buffer_size = window_height * window_width; i < buffer_size;
         i++) {
        color_buffer[i] = color;
    }
}

void draw_grid(uint32_t color, int cell_size) {
    if (cell_size % 10 != 0) cell_size = 100;

    for (int y = 0; y < window_height; y += cell_size) {
        for (int x = 0; x < window_width; x += cell_size) {
            int idx = (window_width * y) + x;
            color_buffer[idx] = color;
        }
    }
}

void draw_pixel(int x_pos, int y_pos, uint32_t color) {
    if (x_pos >= 0 && x_pos < window_width && y_pos >= 0 &&
        y_pos < window_height)
        color_buffer[(window_width * y_pos) + x_pos] = color;
}

void draw_rect(int x_pos, int y_pos, int width, int height, uint32_t color) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            int current_x = x_pos + i;
            int current_y = y_pos + j;
            draw_pixel(current_x, current_y, color);
        }
    }
}

// @brief function to draw a line using the DDA algorithm
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int delta_x = (x1 - x0);
    int delta_y = (y1 - y0);

    int longest_side_length =
        (abs(delta_x) >= abs(delta_y)) ? abs(delta_x) : abs(delta_y);

    float x_inc = delta_x / (float)longest_side_length;
    float y_inc = delta_y / (float)longest_side_length;

    float current_x = x0;
    float current_y = y0;

    for (int i = 0; i <= longest_side_length; i++) {
        draw_pixel(round(current_x), round(current_y), color);
        current_x += x_inc;
        current_y += y_inc;
    }
}

void destroy_window(void) {
    SDL_DestroyTexture(color_buffer_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}