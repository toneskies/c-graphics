#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "array.h"
#include "camera.h"
#include "display.h"
#include "light.h"
#include "matrix.h"
#include "mesh.h"
#include "texture.h"
#include "triangle.h"
#include "upng.h"
#include "vector.h"

#define MAX_TRIANGLES_PER_MESH 10000
triangle_t triangles_to_render[MAX_TRIANGLES_PER_MESH];
int num_triangles_to_render = 0;

mat4_t world_matrix;
mat4_t proj_matrix;
mat4_t view_matrix;

bool is_running = false;
int previous_frame_time = 0;
float delta_time = 0.0;

// --- CAMERA CONTROL VARIABLES ---
float orbit_yaw = 0.0f;
float orbit_pitch = 0.0f;
float orbit_distance = 5.0f;
vec3_t camera_target = {0, 0, 0};  // Now a global variable

bool is_rotating = false;  // Previously is_mouse_dragging
bool is_panning = false;   // New flag for panning

int last_mouse_x = 0;
int last_mouse_y = 0;
// -------------------------------

void setup(void) {
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

    // allocate the required memory in bytes to hold the color buffer
    color_buffer =
        (uint32_t*)malloc(window_width * window_height * sizeof(uint32_t));
    z_buffer = (float*)malloc(window_width * window_height * sizeof(float));

    if (!color_buffer) {
        printf("Bruh your color buffer is null...\n");
    } else {
        /* success, continue with your code.*/
        printf("Your color buffer is aight.\n");
    }

    // create a buffer texture for SDL that is used to hold the color buffer
    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             window_width, window_height);

    // initialize perspectivep rojection matrix
    float fov = M_PI / 3.0;  // the same as 180deg / 3 = 60deg
    float aspect = (float)window_height / (float)window_width;
    float znear = 0.1;
    float zfar = 100.0;
    proj_matrix = mat4_make_perspective(fov, aspect, znear, zfar);

    // manually load the hardcoded texture data from static array
    // mesh_texture = (uint32_t*)REDBRICK_TEXTURE;
    // texture_width = 64;
    // texture_height = 64;

    // Geometry Loading
    // load_cube_mesh_data();
    load_obj_file_data("./assets/drone.obj");

    load_png_texture_data("./assets/drone.png");
}

void process_input(void) {
    SDL_Event event;

    // Use a while loop to process ALL events in the queue for this frame
    while (SDL_PollEvent(&event)) {
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
                if (event.key.keysym.sym == SDLK_5)
                    render_method = RENDER_TEXTURED;
                if (event.key.keysym.sym == SDLK_6)
                    render_method = RENDER_TEXTURED_WIRE;
                if (event.key.keysym.sym == SDLK_c) cull_method = CULL_BACKFACE;
                if (event.key.keysym.sym == SDLK_d) cull_method = CULL_NONE;
                if (event.key.keysym.sym == SDLK_9) {
                    orbit_yaw = 0.0f;
                    orbit_pitch = 0.0f;
                    orbit_distance = 5.0f;
                    camera_target =
                        (vec3_t){0, 0, 0};  // Reset look-at point to origin
                }
                break;

            // --- MOUSE EVENTS ---
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    is_rotating = true;
                    last_mouse_x = event.button.x;
                    last_mouse_y = event.button.y;
                }
                if (event.button.button == SDL_BUTTON_MIDDLE) {
                    is_panning = true;
                    last_mouse_x = event.button.x;
                    last_mouse_y = event.button.y;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) is_rotating = false;
                if (event.button.button == SDL_BUTTON_MIDDLE)
                    is_panning = false;
                break;

            case SDL_MOUSEMOTION: {
                // Calculate mouse delta
                int delta_x = event.motion.x - last_mouse_x;
                int delta_y = event.motion.y - last_mouse_y;
                last_mouse_x = event.motion.x;
                last_mouse_y = event.motion.y;

                // --- ROTATION (Left Click) ---
                if (is_rotating) {
                    float sensitivity = 0.005f;
                    orbit_yaw -= delta_x * sensitivity;
                    orbit_pitch -= delta_y * sensitivity;

                    // Clamp pitch
                    float pitch_limit = M_PI / 2.0f - 0.1f;
                    if (orbit_pitch > pitch_limit) orbit_pitch = pitch_limit;
                    if (orbit_pitch < -pitch_limit) orbit_pitch = -pitch_limit;
                }

                // --- PANNING (Middle Click) ---
                if (is_panning) {
                    // Panning speed should scale with zoom (distance)
                    float pan_speed = orbit_distance * 0.002f;

                    // 1. Calculate Camera Forward Vector (normalized)
                    vec3_t forward = vec3_sub(camera_target, camera.position);
                    vec3_normalize(&forward);

                    // 2. Calculate Camera Right Vector
                    vec3_t up_world = {0, 1, 0};
                    vec3_t right = vec3_cross(
                        up_world, forward);  // Order depends on coord system
                    vec3_normalize(&right);

                    // 3. Calculate Camera Up Vector (Orthogonal to Forward and
                    // Right)
                    vec3_t up_cam = vec3_cross(forward, right);
                    vec3_normalize(&up_cam);

                    // 4. Move the Target Point based on mouse movement
                    // Drag Left (negative x) -> Move Target Left (negative
                    // Right vector) Drag Up (negative y) -> Move Target Up
                    // (positive Up vector)

                    // Note: If panning feels inverted, flip the +/- signs here
                    vec3_t move_right = vec3_mul(right, -delta_x * pan_speed);
                    vec3_t move_up = vec3_mul(up_cam, -delta_y * pan_speed);

                    camera_target = vec3_add(camera_target, move_right);
                    camera_target =
                        vec3_sub(camera_target,
                                 move_up);  // SDL Y is down, World Y is up
                }
                break;
            }
            case SDL_MOUSEWHEEL:
                orbit_distance -= event.wheel.y * 0.5f;
                if (orbit_distance < 1.0f) orbit_distance = 1.0f;
                break;
        }
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

    // delta time factor converted to seconds
    delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;
    previous_frame_time = SDL_GetTicks();

    // reset number of triangles to 0
    num_triangles_to_render = 0;

    // --- 1. UPDATE CAMERA POSITION ---
    // Calculate Cartesian position based on spherical coordinates + target
    // offset
    camera.position.x =
        camera_target.x + (orbit_distance * cos(orbit_pitch) * sin(orbit_yaw));
    camera.position.y = camera_target.y - (orbit_distance * sin(orbit_pitch));
    camera.position.z =
        camera_target.z - (orbit_distance * cos(orbit_pitch) * cos(orbit_yaw));

    // --- 2. UPDATE VIEW MATRIX ---
    vec3_t up_direction = {0, 1, 0};

    // ERROR FIX: Use the global 'camera_target', not a hardcoded {0,0,0} vector
    view_matrix = mat4_look_at(camera.position, camera_target, up_direction);

    // Create scale, rotation, and translation matrices for the mesh
    mat4_t scale_matrix =
        mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
    mat4_t translation_matrix = mat4_make_translation(
        mesh.translation.x, mesh.translation.y, mesh.translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

    // loop all triangle faces of our mesh
    int num_faces = array_length(mesh.faces);
    for (int i = 0; i < num_faces; i++) {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a];
        face_vertices[1] = mesh.vertices[mesh_face.b];
        face_vertices[2] = mesh.vertices[mesh_face.c];

        vec4_t transformed_vertices[3];

        for (int j = 0; j < 3; j++) {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Create a world matrix combining scale, rotation and translation
            world_matrix = mat4_identity();
            world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

            transformed_vertex =
                mat4_mul_vec4(world_matrix, transformed_vertex);
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            transformed_vertices[j] = transformed_vertex;
        }

        // --- BACKFACE CULLING ---
        vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /* A   */
        vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /* / \  */
        vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C---B */

        vec3_t vector_ab = vec3_sub(vector_b, vector_a);
        vec3_t vector_ac = vec3_sub(vector_c, vector_a);
        vec3_normalize(&vector_ab);
        vec3_normalize(&vector_ac);

        vec3_t normal = vec3_cross(vector_ab, vector_ac);
        vec3_normalize(&normal);

        vec3_t origin = {0, 0, 0};
        vec3_t camera_ray = vec3_sub(origin, vector_a);

        float dot_normal_camera = vec3_dot(normal, camera_ray);

        if (cull_method == CULL_BACKFACE) {
            if (dot_normal_camera < 0) {
                continue;
            }
        }

        // Loop all three vertices to perform projection
        vec4_t projected_points[3];
        for (int j = 0; j < 3; j++) {
            projected_points[j] =
                mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);

            // scale into viewport
            projected_points[j].y *= -1;
            projected_points[j].x *= (window_width / 2.0);
            projected_points[j].y *= (window_height / 2.0);

            // translate to center
            projected_points[j].x += (window_width / 2.0);
            projected_points[j].y += (window_height / 2.0);
        }

        float light_intensity_factor = -1 * vec3_dot(normal, light.direction);
        uint32_t triangle_color =
            light_apply_intensity(mesh_face.color, light_intensity_factor);

        triangle_t projected_triangle = {
            .points =
                {
                    {projected_points[0].x, projected_points[0].y,
                     projected_points[0].z, projected_points[0].w},
                    {projected_points[1].x, projected_points[1].y,
                     projected_points[1].z, projected_points[1].w},
                    {projected_points[2].x, projected_points[2].y,
                     projected_points[2].z, projected_points[2].w},
                },
            .texcoords = {{mesh_face.a_uv.u, mesh_face.a_uv.v},
                          {mesh_face.b_uv.u, mesh_face.b_uv.v},
                          {mesh_face.c_uv.u, mesh_face.c_uv.v}},
            .color = triangle_color,
        };

        if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH) {
            triangles_to_render[num_triangles_to_render] = projected_triangle;
            num_triangles_to_render++;
        }
    }
}

void render(void) {
    // exercise 1
    draw_grid(0xFF333333, 10);

    // Loop all projected triangles and render them
    for (int i = 0; i < num_triangles_to_render; i++) {
        triangle_t triangle = triangles_to_render[i];

        if (render_method == RENDER_FILL_TRIANGLE ||
            render_method == RENDER_FILL_TRIANGLE_WIRE) {
            draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,
                                 triangle.points[0].z, triangle.points[0].w,
                                 triangle.points[1].x, triangle.points[1].y,
                                 triangle.points[1].z, triangle.points[1].w,
                                 triangle.points[2].x, triangle.points[2].y,
                                 triangle.points[2].z, triangle.points[2].w,
                                 triangle.color);
        }

        // Draw textured triangle
        if (render_method == RENDER_TEXTURED ||
            render_method == RENDER_TEXTURED_WIRE) {
            draw_textured_triangle(
                triangle.points[0].x, triangle.points[0].y,
                triangle.points[0].z, triangle.points[0].w,
                triangle.texcoords[0].u, triangle.texcoords[0].v,
                triangle.points[1].x, triangle.points[1].y,
                triangle.points[1].z, triangle.points[1].w,
                triangle.texcoords[1].u, triangle.texcoords[1].v,
                triangle.points[2].x, triangle.points[2].y,
                triangle.points[2].z, triangle.points[2].w,
                triangle.texcoords[2].u, triangle.texcoords[2].v, mesh_texture);
        }

        if (render_method == RENDER_WIRE ||
            render_method == RENDER_WIRE_VERTEX ||
            render_method == RENDER_FILL_TRIANGLE_WIRE ||
            render_method == RENDER_TEXTURED_WIRE) {
            draw_triangle(triangle.points[0].x, triangle.points[0].y,
                          triangle.points[1].x, triangle.points[1].y,
                          triangle.points[2].x, triangle.points[2].y,
                          0xFFFFFFFF);
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

    render_color_buffer();

    clear_color_buffer(0xFF000000);
    clear_z_buffer();

    SDL_RenderPresent(renderer);
}

/// @brief free memory that was dynamically allocated by the program
/// @param  none
void free_resources(void) {
    free(color_buffer);
    free(z_buffer);
    upng_free(png_texture);
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