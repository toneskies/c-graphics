#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "array.h"
#include "camera.h"
#include "clipping.h"
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

void setup(void) {
    set_render_method(RENDER_WIRE);
    set_cull_method(CULL_BACKFACE);

    // initialize perspectivep rojection matrix
    float aspecty = (float)get_window_height() / (float)get_window_width();
    float aspectx = (float)get_window_width() / (float)get_window_height();
    float fovy = 3.141592 / 3.0;  // the same as 180deg / 3 = 60deg
    float fovx =
        atan(tan(fovy / 2) * aspectx) * 2.0;  // the same as 180deg / 3 = 60deg
    float znear = 0.1;
    float zfar = 20.0;
    proj_matrix = mat4_make_perspective(fovy, aspecty, znear, zfar);

    // initialize frustum
    init_frustum_planes(fovx, fovy, znear, zfar);

    load_obj_file_data("./assets/f117.obj");

    load_png_texture_data("./assets/f117.png");
}

void process_input(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                is_running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    is_running = false;
                    break;
                }
                if (event.key.keysym.sym == SDLK_1) {
                    set_render_method(RENDER_WIRE_VERTEX);
                    break;
                }
                if (event.key.keysym.sym == SDLK_2) {
                    set_render_method(RENDER_WIRE);
                    break;
                }
                if (event.key.keysym.sym == SDLK_3) {
                    set_render_method(RENDER_FILL_TRIANGLE);
                    break;
                }
                if (event.key.keysym.sym == SDLK_4) {
                    set_render_method(RENDER_FILL_TRIANGLE_WIRE);
                    break;
                }

                if (event.key.keysym.sym == SDLK_5) {
                    set_render_method(RENDER_TEXTURED);
                    break;
                }

                if (event.key.keysym.sym == SDLK_6) {
                    set_render_method(RENDER_TEXTURED_WIRE);
                    break;
                }

                if (event.key.keysym.sym == SDLK_c) {
                    set_cull_method(CULL_BACKFACE);
                    break;
                }
                if (event.key.keysym.sym == SDLK_x) {
                    set_cull_method(CULL_NONE);
                    break;
                }
                if (event.key.keysym.sym == SDLK_UP) {
                    camera.position.y += 3.0 * delta_time;
                    break;
                }

                if (event.key.keysym.sym == SDLK_DOWN) {
                    camera.position.y -= 3.0 * delta_time;
                    break;
                }
                if (event.key.keysym.sym == SDLK_a) {
                    camera.yaw += 1.0 * delta_time;
                    break;
                }
                if (event.key.keysym.sym == SDLK_d) {
                    camera.yaw -= 1.0 * delta_time;
                    break;
                }
                if (event.key.keysym.sym == SDLK_w) {
                    camera.forward_velocity =
                        vec3_mul(camera.direction, 5.0 * delta_time);
                    camera.position =
                        vec3_add(camera.position, camera.forward_velocity);
                    break;
                }
                if (event.key.keysym.sym == SDLK_s) {
                    camera.forward_velocity =
                        vec3_mul(camera.direction, 5.0 * delta_time);
                    camera.position =
                        vec3_sub(camera.position, camera.forward_velocity);
                    break;
                }
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

    // delta time factor converted to seconds to be used to update game objects
    delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;

    previous_frame_time = SDL_GetTicks();

    // reset number of triangles to 0
    num_triangles_to_render = 0;

    // Change the mesh scale/rotation values per animation frame
    // mesh.rotation.x += 0.6 * delta_time;
    // mesh.rotation.y += 0.6 * delta_time;
    // mesh.rotation.z += 0.6 * delta_time;
    mesh.translation.z = 5.0;

    // Create a view matrix
    vec3_t up_direction = {0, 1, 0};
    // TODO: find the target point
    vec3_t target = {0, 0, 1};
    mat4_t camera_yaw_rotation = mat4_make_rotation_y(camera.yaw);
    camera.direction = vec3_from_vec4(
        mat4_mul_vec4(camera_yaw_rotation, vec4_from_vec3(target)));

    target = vec3_add(camera.position, camera.direction);

    view_matrix = mat4_look_at(camera.position, target, up_direction);

    // create a scale, rotation and translation matrices that will be used to
    // multiply the mesh vertices
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

        // loop all three vertices of this current face and apply
        // transformations

        for (int j = 0; j < 3; j++) {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Create a world matrix combining scale, rotation and translation
            // matrices
            world_matrix = mat4_identity();

            // Order matters: First scale, rotate, translate [S] * [R] * [T] * v
            world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

            // TODO: multiply the world matrix by the original vector
            transformed_vertex =
                mat4_mul_vec4(world_matrix, transformed_vertex);

            // multiply the vector to transform the scene to camera space
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            // Save transformed vertex in the array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        // TODO: Check backface culling
        // Note: We defined Clock-wise vertex numbering for our engine
        // 1. Find vectors B-A and C-A
        vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*    A   */
        vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*   / \  */
        vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /*  C---B */

        // Get vector sub of B-A and C-A
        vec3_t vector_ab = vec3_sub(vector_b, vector_a);
        vec3_t vector_ac = vec3_sub(vector_c, vector_a);

        // 2. Take their cross product and find the perpendicular normal N
        vec3_t normal = vec3_cross(vector_ab, vector_ac);
        vec3_normalize(&normal);
        // 3. Find the camera ray vector by subtracting the camera position
        // from point A

        vec3_t origin = {0, 0, 0};
        vec3_t camera_ray = vec3_sub(origin, vector_a);
        // 4. Take the dot product between the normal N and the camera ray
        float dot_normal_camera = vec3_dot(normal, camera_ray);
        // 5. If this dot product is less than zero, then do not display the
        // face
        if (is_cull_backface()) {
            if (dot_normal_camera < 0) {
                continue;
            }
        }

        // TODO: CLIPPING
        // create a polygon from original transformed triangle to be clipped
        polygon_t polygon = create_polygon_from_triangle(
            vec3_from_vec4(transformed_vertices[0]),
            vec3_from_vec4(transformed_vertices[1]),
            vec3_from_vec4(transformed_vertices[2]), mesh_face.a_uv,
            mesh_face.b_uv, mesh_face.c_uv);

        // clip the polygon and return a new polygon with potential new vertices
        clip_polygon(&polygon);

        // TODO: after clipping, break the polygon into triangles
        triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
        int num_triangles_after_clipping = 0;

        // Here we will have the array of triangles and also the num of
        // triangles
        triangles_from_polygon(&polygon, triangles_after_clipping,
                               &num_triangles_after_clipping);

        // Loop all the assembled triangles after clipping
        for (int t = 0; t < num_triangles_after_clipping; t++) {
            triangle_t triangle_after_clipping = triangles_after_clipping[t];
            // Loop all three vertices to perform projection
            vec4_t projected_points[3];
            for (int j = 0; j < 3; j++) {
                // project the current vertex
                projected_points[j] = mat4_mul_vec4_project(
                    proj_matrix, triangle_after_clipping.points[j]);
                // scale into viewport
                projected_points[j].y *= -1;

                projected_points[j].x *= (get_window_width() / 2.0);
                projected_points[j].y *= (get_window_height() / 2.0);
                // translate the projected points to the middle of the screen
                projected_points[j].x += (get_window_width() / 2.0);
                projected_points[j].y += (get_window_height() / 2.0);
            }

            // calculate the shade intensity based on how aligned ois the face
            // normal and the light
            float light_intensity_factor =
                -1 *
                vec3_dot(normal,
                         light.direction);  // fixing the light intensity
                                            // factor to point inwards with -1

            // calculate the triangle color based on light angle
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
                .texcoords =
                    {
                        {triangle_after_clipping.texcoords[0].u,
                         triangle_after_clipping.texcoords[0].v},
                        {triangle_after_clipping.texcoords[1].u,
                         triangle_after_clipping.texcoords[1].v},
                        {triangle_after_clipping.texcoords[2].u,
                         triangle_after_clipping.texcoords[2].v},
                    },
                .color = triangle_color};

            // save projected triangle to the array of triangles to render
            if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH) {
                triangles_to_render[num_triangles_to_render] =
                    projected_triangle;
                num_triangles_to_render++;
            }
        }
    }
}

void render(void) {
    clear_color_buffer(0xFF000000);
    clear_z_buffer();

    draw_grid(0xFF333333, 10);

    // Loop all projected triangles and render them
    for (int i = 0; i < num_triangles_to_render; i++) {
        triangle_t triangle = triangles_to_render[i];

        if (should_render_filled_triangles()) {
            draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,
                                 triangle.points[0].z, triangle.points[0].w,
                                 triangle.points[1].x, triangle.points[1].y,
                                 triangle.points[1].z, triangle.points[1].w,
                                 triangle.points[2].x, triangle.points[2].y,
                                 triangle.points[2].z, triangle.points[2].w,
                                 triangle.color);
        }

        // Draw textured triangle
        if (should_render_textured_triangles()) {
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

        if (should_render_wireframe()) {
            draw_triangle(triangle.points[0].x, triangle.points[0].y,
                          triangle.points[1].x, triangle.points[1].y,
                          triangle.points[2].x, triangle.points[2].y,
                          0xFFFFFFFF);
        }

        if (should_render_wire_vertex()) {
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6,
                      0xFFFF0000);
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6,
                      0xFFFF0000);
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6,
                      0xFFFF0000);
        }
    }

    render_color_buffer();
}

/// @brief free memory that was dynamically allocated by the program
/// @param  none
void free_resources(void) {
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