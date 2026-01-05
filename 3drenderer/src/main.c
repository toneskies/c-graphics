#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "array.h"
#include "display.h"
#include "light.h"
#include "matrix.h"
#include "mesh.h"
#include "triangle.h"
#include "vector.h"

triangle_t* triangles_to_render = NULL;

vec3_t camera_position = {0, 0, 0};
mat4_t proj_matrix;

bool is_running = false;
int previous_frame_time = 0;

vec3_t light_direction = {-1.0, 1.0, 0.0};

void setup(void) {
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

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

    // initialize perspectivep rojection matrix
    float fov = M_PI / 3.0;  // the same as 180deg / 3 = 60deg
    float aspect = (float)window_height / (float)window_width;
    float znear = 0.1;
    float zfar = 100.0;
    proj_matrix = mat4_make_perspective(fov, aspect, znear, zfar);

    // Geometry Loading
    // load_cube_mesh_data();
    load_obj_file_data("./assets/woman.obj");
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
            if (event.key.keysym.sym == SDLK_1)
                render_method = RENDER_WIRE_VERTEX;
            if (event.key.keysym.sym == SDLK_2) render_method = RENDER_WIRE;
            if (event.key.keysym.sym == SDLK_3)
                render_method = RENDER_FILL_TRIANGLE;
            if (event.key.keysym.sym == SDLK_4)
                render_method = RENDER_FILL_TRIANGLE_WIRE;
            if (event.key.keysym.sym == SDLK_c) cull_method = CULL_BACKFACE;
            if (event.key.keysym.sym == SDLK_d) cull_method = CULL_NONE;
            break;
    }
}

/// @brief Function that receives a 3D vector and returns a projected 2D point
/// @param point
/// @return orthographic projected point
// vec2_t project(vec3_t point) {
//     vec2_t projected_point = {.x = fov_factor * point.x / point.z,
//                               .y = fov_factor * point.y / point.z};
//     return projected_point;
// }

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

    // Change the mesh scale/rotation values per animation frame
    // mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.01;
    // mesh.rotation.z += 0.01;
    // mesh.scale.x += 0.002;
    // mesh.scale.y += 0.001;
    // mesh.translation.x += 0.01;
    mesh.translation.z = 2000.0;
    // create a scale, rotation and translation matrices that will be used to
    // multiply the mesh vertices
    mat4_t scale_matrix =
        mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
    mat4_t translation_matrix = mat4_make_translation(
        mesh.translation.x, mesh.translation.y, mesh.translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

    // find light ray vector by sub light vector and triangle normal
    vec3_normalize(&light_direction);
    // loop all triangle faces of our mesh
    int num_faces = array_length(mesh.faces);
    for (int i = 0; i < num_faces; i++) {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        vec4_t transformed_vertices[3];

        // loop all three vertices of this current face and apply
        // transformations

        for (int j = 0; j < 3; j++) {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Create a world matrix combining scale, rotation and translation
            // matrices
            mat4_t world_matrix = mat4_identity();

            // Order matters: First scale, rotate, translate [S] * [R] * [T] * v
            world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);
            // TODO: multiply the world matrix by the original vector
            transformed_vertex =
                mat4_mul_vec4(world_matrix, transformed_vertex);

            // Save transformed vertex in the array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        if (cull_method == CULL_BACKFACE) {
            // TODO: Check backface culling
            // Note: We defined Clock-wise vertex numbering for our engine
            // 1. Find vectors B-A and C-A
            vec3_t vector_a =
                vec3_from_vec4(transformed_vertices[0]); /*    A   */
            vec3_t vector_b =
                vec3_from_vec4(transformed_vertices[1]); /*   / \  */
            vec3_t vector_c =
                vec3_from_vec4(transformed_vertices[2]); /*  C---B */

            // Get vector sub of B-A and C-A
            vec3_t vector_ab = vec3_sub(vector_b, vector_a);
            vec3_t vector_ac = vec3_sub(vector_c, vector_a);
            vec3_normalize(&vector_ab);
            vec3_normalize(&vector_ac);

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
        vec4_t projected_points[3];
        for (int j = 0; j < 3; j++) {
            // project the current vertex
            projected_points[j] =
                mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);
            // scale into viewport
            projected_points[j].x *= (window_width / 2.0);
            projected_points[j].y *= -1 * (window_height / 2.0);
            // translate the projected points to the middle of the screen
            projected_points[j].x += (window_width / 2.0);
            projected_points[j].y += (window_height / 2.0);
        }

        // calculate the average depth for each face based on the vertices after
        // transformation
        float avg_depth =
            (transformed_vertices[0].z + transformed_vertices[1].z +
             transformed_vertices[2].z) /
            3.0;

        triangle_t projected_triangle = {
            .points =
                {
                    {projected_points[0].x, projected_points[0].y},
                    {projected_points[1].x, projected_points[1].y},
                    {projected_points[2].x, projected_points[2].y},

                },
            .color = mesh_face.color,
            .avg_depth = avg_depth};

        // LIGHTING
        // find normal of triangle using 2 edges
        vec3_t edge_1 = vec3_from_vec4(transformed_vertices[0]); /*    A   */
        vec3_t edge_2 = vec3_from_vec4(transformed_vertices[1]); /*   / \  */
        vec3_t edge_3 = vec3_from_vec4(transformed_vertices[2]); /*  C---B */

        vec3_t vector_1 = vec3_sub(edge_2, edge_1);
        vec3_t vector_2 = vec3_sub(edge_3, edge_1);

        vec3_t tri_normal = vec3_cross(vector_1, vector_2);
        vec3_normalize(&tri_normal);

        float dot_normal_light = vec3_dot(tri_normal, light_direction);

        // // Cel/Toon Shading apparently...
        // if (dot_normal_light < 0) {
        //     projected_triangle.color =
        //         light_apply_intensity(projected_triangle.color, 0.1);
        // } else if (dot_normal_light > 0.9) {
        //     projected_triangle.color =
        //         light_apply_intensity(projected_triangle.color, 1.0);
        // } else if (dot_normal_light > 0.5) {
        //     projected_triangle.color =
        //         light_apply_intensity(projected_triangle.color, 0.7);
        // } else {
        //     projected_triangle.color =
        //         light_apply_intensity(projected_triangle.color, 0.4);
        // }

        // smooth shading
        if (dot_normal_light < 0.1)
            dot_normal_light = 0.1;  // Clamp negative values
        projected_triangle.color =
            light_apply_intensity(projected_triangle.color, dot_normal_light);

        // if less than 0, clamp to 0
        // if 0, fully aligned (full color)
        // if 90 (low percentage)
        // if greater than 90 (fully dark), give it black

        array_push(triangles_to_render, projected_triangle);
    }

    // TODO: sort triangles to render by their average depth, that's the
    // painter's algorithm
    int triangle_count = array_length(triangles_to_render);
    bubble_sort(triangles_to_render, triangle_count);
}

void render(void) {
    // exercise 1
    draw_grid(0xFF333333, 10);

    int num_triangles = array_length(triangles_to_render);
    // Loop all projected triangles and render them
    for (int i = 0; i < num_triangles; i++) {
        triangle_t triangle = triangles_to_render[i];

        if (render_method == RENDER_FILL_TRIANGLE ||
            render_method == RENDER_FILL_TRIANGLE_WIRE) {
            draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,
                                 triangle.points[1].x, triangle.points[1].y,
                                 triangle.points[2].x, triangle.points[2].y,
                                 triangle.color);
        }

        if (render_method == RENDER_WIRE ||
            render_method == RENDER_WIRE_VERTEX ||
            render_method == RENDER_FILL_TRIANGLE_WIRE) {
            draw_triangle(triangle.points[0].x, triangle.points[0].y,
                          triangle.points[1].x, triangle.points[1].y,
                          triangle.points[2].x, triangle.points[2].y,
                          0xFF555555);
        }

        if (render_method == RENDER_WIRE_VERTEX) {
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6,
                      0xFFFF0000);
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6,
                      0xFFFF0000);
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6,
                      0xFFFF0000);
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