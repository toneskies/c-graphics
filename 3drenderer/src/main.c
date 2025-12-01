#include <stdio.h>
#include <stdint.h>

#include <stdbool.h>
#include <SDL2/SDL.h>

uint32_t* color_buffer = NULL;
int window_height = 600;
int window_width = 800;

bool is_running = false;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

SDL_Texture* color_buffer_texture = NULL;


bool initialize_window(void) { 
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Use SDL to query what is the fullscreen max. width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    window_width = display_mode.w;
    window_height = display_mode.h;

    // Created a SDL Window
    window = SDL_CreateWindow(
        NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_BORDERLESS
     );

     if(!window) {
        fprintf(stderr, "Error creating SDL window.\n");
        return false;
     }

    // Create a SDL renderer
    renderer = SDL_CreateRenderer(
        window,
        -1,
        0
    );
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    if(!renderer) {
        fprintf(stderr, "Error creating SDL renderer.\n");
        return false;
    }
    
    return true;
}

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

void render_color_buffer() {
    SDL_UpdateTexture(
        color_buffer_texture,
        NULL,
        color_buffer,
        (int)(window_width * sizeof(uint32_t))
    );
    SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
}

void clear_color_buffer(uint32_t color) {
    // for (size_t y = 0; y < window_height; y++) {
    //     for(size_t x = 0; x < window_width; x++) {
    //         color_buffer[(window_width * y) + x] = color;
    //     }
    // }
    for (size_t i = 0, buffer_size = window_height * window_width; i < buffer_size; i++) {
        *(color_buffer + i) = color;
    }
}

void render(void) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    render_color_buffer();

    clear_color_buffer(0xFFFFFF00);

    SDL_RenderPresent(renderer);
}

void destroy_window(void) {
    free(color_buffer);
    SDL_DestroyTexture(color_buffer_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(void) {
    is_running = initialize_window();

    setup();

    while(is_running) {
        process_input();
        update();
        render();
    }

    destroy_window();

    return 0;
}