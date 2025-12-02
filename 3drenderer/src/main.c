#include <stdio.h>
#include <stdint.h>

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "display.h"

bool is_running = false;


void setup(void) {
    // allocate the required memory in bytes to hold the color buffer
    color_buffer = (uint32_t *)malloc(window_width * window_height * sizeof(uint32_t));

    if (!color_buffer) {
        /* if it's a null pointer, fix this!*/
    } else {
        /* success, continue with your code.*/
    }

    // create a buffer texture for SDL that is used to hold the color buffer
    color_buffer_texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    window_width,
    window_height
    );
    

}

void process_input(void) {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch(event.type) {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                is_running = false;
            break;
        
        }
}

void update(void) {
    // TODO: 
}

void render(void) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    // exercise 1
    // Create a function called draw_grid() that renders a background grid
    // that shows a line every row or column of pixels that is a multiple
    // of 10.
    draw_grid(0xFF333333, 10);

    // exercise 2
    // TODO: Implement draw_rect
    // Create a function called draw_rect() that renders a rectangle
    // on the screen.
    draw_rect(100, 100, 100, 100, 0xFFFFC0CB);

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
    while(is_running) {
        // 3a. process user input
        process_input();

        // 3b. update timestep
        update();

        //3c. render data to screen
        render();
    }

    // 4. collect garbage
    destroy_window();

    return 0;
}