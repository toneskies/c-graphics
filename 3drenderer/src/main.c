#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "display.h"
#include "vector.h"

/*
 * @brief Declare an array of vectors/points
 */
#define N_POINTS 9 * 9 * 9
vec3_t cube_points[N_POINTS];  // 9x9x9 cube
vec2_t projected_points[N_POINTS];

vec3_t camera_position = {.x = 0, .y = 0, .z = -5};
vec3_t cube_rotation = {.x = 0, .y = 0, .z = 0};

float fov_factor = 640;

bool is_running = false;
int previous_frame_time = 0;

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

    int point_count = 0;

    // Start loading my array of vectors
    // From -1 to 1 (in this 9x9x9 cube)
    for (float x = -1; x <= 1; x += 0.25) {
        for (float y = -1; y <= 1; y += 0.25) {
            for (float z = -1; z <= 1; z += 0.25) {
                vec3_t new_point = {.x = x, .y = y, .z = z};
                cube_points[point_count++] = new_point;
            }
        }
    }
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
            break;
    }
}

/// @brief Function that receives a 3D vector and returns a projected 2D point
/// @param point
/// @return orthographic projected point
vec2_t project(vec3_t point) {
    vec2_t projected_point = {.x = fov_factor * point.x / point.z,
                              .y = fov_factor * point.y / point.z};
    return projected_point;
}

void update(void) {
    // while (!SDL_TICKS_PASSED(SDL_GetTicks(),
    //                          previous_frame_time + FRAME_TARGET_TIME)) {
    // };

    previous_frame_time = SDL_GetTicks();

    int time_to_wait =
        FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
        SDL_Delay(time_to_wait);
    }

    cube_rotation.y += 0.01;
    cube_rotation.x += 0.01;
    cube_rotation.z += 0.01;

    for (int i = 0; i < N_POINTS; i++) {
        vec3_t point = cube_points[i];

        vec3_t transformed_point = vec3_rotate_x(point, cube_rotation.x);
        transformed_point = vec3_rotate_y(transformed_point, cube_rotation.y);
        transformed_point = vec3_rotate_z(transformed_point, cube_rotation.z);

        // move points camera
        transformed_point.z -= camera_position.z;

        // project the current point
        vec2_t projected_point = project(transformed_point);

        // save the projected 2d vector in the array of projected points
        projected_points[i] = projected_point;
    }
}

void render(void) {
    // exercise 1
    draw_grid(0xFF333333, 10);

    // Loop all projected points
    for (int i = 0; i < N_POINTS; i++) {
        vec2_t projected_point = projected_points[i];
        draw_rect(projected_point.x + window_width / 2,
                  projected_point.y + window_height / 2, 5, 5, 0xFFFFFF00);
    }

    render_color_buffer();
    clear_color_buffer(0xFF000000);

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