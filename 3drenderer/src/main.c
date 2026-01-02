#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "array.h"
#include "display.h"
#include "mesh.h"
#include "triangle.h"
#include "vector.h"

triangle_t* triangles_to_render = NULL;

vec3_t camera_position = {0, 0, 0};

float fov_factor = 640;

bool is_running = false;
typedef enum DisplayMode {
    WIREDOT_1,
    WIRE_2,
    FILLED_3,
    FILLEDWIRE_4,
} displaymode_t;
displaymode_t model_style = WIREDOT_1;

bool backface_cull = false;

int previous_frame_time = 0;

void setup(void) {
    // allocate the required memory in bytes to hold the color buffer
    color_buffer =
        (uint32_t*)malloc(window_width * window_height * sizeof(uint32_t));

    if (!color_buffer) {
        printf("Bruh your color buffer is null...\n");
    } else {
        /* success, continue with your code.*/
        printf("Your color buffer is aight.\n");
    }

    // create a buffer texture for SDL that is used to hold the color buffer
    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             window_width, window_height);

    // load_cube_mesh_data();
    load_obj_file_data("./assets/cube.obj");
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
            if (event.key.keysym.sym == SDLK_1) model_style = WIREDOT_1;
            if (event.key.keysym.sym == SDLK_2) model_style = WIRE_2;
            if (event.key.keysym.sym == SDLK_3) model_style = FILLED_3;
            if (event.key.keysym.sym == SDLK_4) model_style = FILLEDWIRE_4;
            if (event.key.keysym.sym == SDLK_c) backface_cull = true;
            if (event.key.keysym.sym == SDLK_d) backface_cull = false;
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
    // Wait some time until the reach the target frame time in milliseconds
    int time_to_wait =
        FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

    // Only delay execution if we are running too fast
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
        SDL_Delay(time_to_wait);
    }

    previous_frame_time = SDL_GetTicks();

    // initialize array of triangles to render
    triangles_to_render = NULL;

    mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.02;
    mesh.rotation.z += 0.01;

    // loop all triangle faces of our mesh
    int num_faces = array_length(mesh.faces);
    for (int i = 0; i < num_faces; i++) {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        triangle_t projected_triangle;
        vec3_t transformed_vertices[3];

        // loop all three vertices of this current face and apply
        // transformations

        for (int j = 0; j < 3; j++) {
            vec3_t transformed_vertex = face_vertices[j];

            transformed_vertex =
                vec3_rotate_x(transformed_vertex, mesh.rotation.x);
            transformed_vertex =
                vec3_rotate_y(transformed_vertex, mesh.rotation.y);
            transformed_vertex =
                vec3_rotate_z(transformed_vertex, mesh.rotation.z);

            // translate verted away from the camera in z
            transformed_vertex.z += 5;

            // Save transformed vertex in the array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        if (backface_cull) {
            // TODO: Check backface culling
            // Note: We defined Clock-wise vertex numbering for our engine
            // 1. Find vectors B-A and C-A
            vec3_t vector_a = transformed_vertices[0]; /*    A   */
            vec3_t vector_b = transformed_vertices[1]; /*   / \  */
            vec3_t vector_c = transformed_vertices[2]; /*  C---B */

            // Get vector sub of B-A and C-A
            vec3_t vector_ab = vec3_sub(vector_b, vector_a);
            vec3_t vector_ac = vec3_sub(vector_c, vector_a);

            // 2. Take their cross product and find the perpendicular normal N
            vec3_t normal = vec3_cross(vector_ab, vector_ac);
            vec3_normalize(&normal);
            // 3. Find the camera ray vector by subtracting the camera position
            // from point A
            vec3_t camera_ray = vec3_sub(camera_position, vector_a);
            // 4. Take the dot product between the normal N and the camera ray
            float dot_normal_camera = vec3_dot(normal, camera_ray);
            // 5. If this dot product is less than zero, then do not display the
            // face
            if (dot_normal_camera < 0) {
                continue;
            }
        }

        // Loop all three vertices to perform projection
        for (int j = 0; j < 3; j++) {
            // project the current vertex
            vec2_t projected_point = project(transformed_vertices[j]);

            // Scale and translate the projected points to the middle of the
            // screen
            projected_point.x += (window_width / 2);
            projected_point.y += (window_height / 2);

            projected_triangle.points[j] = projected_point;
        }

        // triangles_to_render[i] = projected_triangle;
        array_push(triangles_to_render, projected_triangle);
    }
}

void render(void) {
    // exercise 1
    draw_grid(0xFF333333, 10);

    int num_triangles = array_length(triangles_to_render);
    // Loop all projected triangles and render them
    for (int i = 0; i < num_triangles; i++) {
        triangle_t triangle = triangles_to_render[i];

        switch (model_style) {
            case WIREDOT_1:
                draw_triangle(triangle.points[0].x, triangle.points[0].y,
                              triangle.points[1].x, triangle.points[1].y,
                              triangle.points[2].x, triangle.points[2].y,
                              0xFFFFFFFF);
                draw_pixel(triangle.points[0].x, triangle.points[0].y,
                           0xFFFF0000);
                draw_pixel(triangle.points[1].x, triangle.points[1].y,
                           0xFFFF0000);
                draw_pixel(triangle.points[2].x, triangle.points[2].y,
                           0xFFFF0000);

                break;
            case WIRE_2:
                draw_triangle(triangle.points[0].x, triangle.points[0].y,
                              triangle.points[1].x, triangle.points[1].y,
                              triangle.points[2].x, triangle.points[2].y,
                              0xFFFFFFFF);
                break;
            case FILLED_3:
                draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,
                                     triangle.points[1].x, triangle.points[1].y,
                                     triangle.points[2].x, triangle.points[2].y,
                                     0xFFFFFFFF);
                break;
            case FILLEDWIRE_4:
                draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,
                                     triangle.points[1].x, triangle.points[1].y,
                                     triangle.points[2].x, triangle.points[2].y,
                                     0xFFFFFFFF);
                draw_triangle(triangle.points[0].x, triangle.points[0].y,
                              triangle.points[1].x, triangle.points[1].y,
                              triangle.points[2].x, triangle.points[2].y,
                              0xFF000000);
                break;
            default:
                printf(
                    "Model Style invalid choice, defaulting to wireframe with "
                    "dots.\n");
                break;
        }
    }

    // clear the array of triangles to render every frame loop
    array_free(triangles_to_render);

    render_color_buffer();
    clear_color_buffer(0xFF000000);

    SDL_RenderPresent(renderer);
}

/// @brief free memory that was dynamically allocated by the program
/// @param  none
void free_resources(void) {
    free(color_buffer);
    array_free(mesh.faces);
    array_free(mesh.vertices);
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
    free_resources();

    return 0;
}