#include "display.h"

#include <stdio.h>

static int render_method = 0;
static int cull_method = 0;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static uint32_t* color_buffer = NULL;
static float* z_buffer = NULL;

static SDL_Texture* color_buffer_texture = NULL;
static int window_height = 600;
static int window_width = 800;

int get_window_height() { return window_height; }
int get_window_width() { return window_width; }
void set_render_method(int method) { render_method = method; };
void set_cull_method(int method) { cull_method = method; };
bool is_cull_backface(void) { return cull_method == CULL_BACKFACE; };
bool should_render_filled_triangles(void) {
    return (render_method == RENDER_FILL_TRIANGLE ||
            render_method == RENDER_FILL_TRIANGLE_WIRE);
}
bool should_render_textured_triangles(void) {
    return (render_method == RENDER_TEXTURED ||
            render_method == RENDER_TEXTURED_WIRE);
}
bool should_render_wireframe(void) {
    return (render_method == RENDER_WIRE ||
            render_method == RENDER_WIRE_VERTEX ||
            render_method == RENDER_FILL_TRIANGLE_WIRE ||
            render_method == RENDER_TEXTURED_WIRE);
}
bool should_render_wire_vertex(void) {
    return render_method == RENDER_WIRE_VERTEX;
}

bool initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Use SDL to query what is the fullscreen max. width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    // int fullscreen_width = display_mode.w;
    // int fullscreen_height = display_mode.h;

    // window_width = fullscreen_width / 3;
    // window_height = fullscreen_height / 3;

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

    // allocate the required memory in bytes to hold the color buffer
    color_buffer =
        (uint32_t*)malloc(window_width * window_height * sizeof(uint32_t));
    z_buffer = (float*)malloc(window_width * window_height * sizeof(float));

    // create a buffer texture for SDL that is used to hold the color buffer
    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             window_width, window_height);

    return true;
}

void render_color_buffer() {
    SDL_UpdateTexture(color_buffer_texture, NULL, color_buffer,
                      (int)(window_width * sizeof(uint32_t)));
    SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

float get_zbuffer_at(int x, int y) {
    if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
        return 1.0;
    }
    return z_buffer[(window_width * y + x)];
}
void update_zbuffer_at(int x, int y, float value) {
    if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
        return;
    }
    z_buffer[(window_width * y) + x] = value;
}

void clear_color_buffer(uint32_t color) {
    for (int i = 0, buffer_size = window_height * window_width; i < buffer_size;
         i++) {
        color_buffer[i] = color;
    }
}

void clear_z_buffer(void) {
    for (int i = 0, buffer_size = window_height * window_width; i < buffer_size;
         i++) {
        z_buffer[i] = 1.0;
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
    if (x_pos < 0 || x_pos >= window_width || y_pos < 0 ||
        y_pos >= window_height)
        return;
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
void draw_line(int x0, int y0, float z0, float w0, int x1, int y1, float z1,
               float w1, uint32_t color) {
    int delta_x = (x1 - x0);
    int delta_y = (y1 - y0);

    int longest_side_length =
        (abs(delta_x) >= abs(delta_y)) ? abs(delta_x) : abs(delta_y);

    float x_inc = delta_x / (float)longest_side_length;
    float y_inc = delta_y / (float)longest_side_length;

    float current_x = x0;
    float current_y = y0;

    // [NEW] Depth Interpolation Logic
    // We interpolate 1/w to stay linear in screen space
    float inv_w0 = 1.0 / w0;
    float inv_w1 = 1.0 / w1;
    float inv_w_inc = (inv_w1 - inv_w0) / (float)longest_side_length;
    float current_inv_w = inv_w0;

    for (int i = 0; i <= longest_side_length; i++) {
        int x = round(current_x);
        int y = round(current_y);

        if (x >= 0 && x < window_width && y >= 0 && y < window_height) {
            // Match the Z-buffer logic from triangle.c
            // Logic: depth = 1.0 - (1/w)
            // Smaller 'depth' is closer to camera.
            float depth = 1.0 - current_inv_w - 0.0001f;

            // Check if this pixel is closer than what's in the Z-buffer
            if (depth <= get_zbuffer_at(x, y)) {
                draw_pixel(x, y, color);
                update_zbuffer_at(x, y, depth);
            }
        }

        current_x += x_inc;
        current_y += y_inc;
        current_inv_w += inv_w_inc;
    }
}

void destroy_window(void) {
    free(color_buffer);
    free(z_buffer);
    SDL_DestroyTexture(color_buffer_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}