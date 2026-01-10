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

float orbit_radius = 5.0;
float orbit_theta = 0.0;
float orbit_phi = 0.0;
bool is_mouse_down = false;

typedef enum { PROJ_PERSPECTIVE, PROJ_ORTHOGRAPHIC } projection_type_t;
projection_type_t projection_type = PROJ_PERSPECTIVE;
float ortho_height = 6.0;
const float ORTHO_CAMERA_DISTANCE = 20.0;

void update_projection_matrix(void) {
    float aspect_ratio = (float)get_window_width() / (float)get_window_height();
    float znear = 0.1;
    float zfar =
        400.0;  // Increased ZFar to prevent clipping the back of the mesh

    if (projection_type == PROJ_PERSPECTIVE) {
        float fovy = 3.141592 / 3.0;  // 60 degrees
        float aspecty = (float)get_window_height() / (float)get_window_width();
        proj_matrix = mat4_make_perspective(fovy, aspecty, znear, zfar);

        // Update Frustum Planes for Perspective (Cone)
        float fovx = atan(tan(fovy / 2) * aspect_ratio) * 2.0;
        init_frustum_planes(fovx, fovy, znear, zfar);
    } else {
        float top = ortho_height / 2.0;
        float bottom = -top;
        float right = top * aspect_ratio;
        float left = -right;

        proj_matrix =
            mat4_make_orthographic(left, right, bottom, top, znear, zfar);

        // Update Frustum Planes for Ortho
        // HACK: We use a very wide FOV (170 deg) for the frustum planes
        // logic to effectively disable side-clipping, since the standard
        // init_frustum_planes creates a pyramid. We only care about
        // Near/Far clipping in Ortho here.
        float wide_fov = 3.141592 * (170.0 / 180.0);
        init_frustum_planes_ortho(left, right, top, bottom, znear, zfar);
    }
}

// Calculate the furthest point from the center to ensure the object fits
// regardless of rotation.
float get_mesh_radius(mesh_t* mesh) {
    int num_vertices = array_length(mesh->vertices);
    if (num_vertices == 0) return 2.0;  // Default fallback

    float max_dist_sq = 0.0;

    // Find vertex furthest from local origin (0,0,0)
    for (int i = 0; i < num_vertices; i++) {
        vec3_t v = mesh->vertices[i];

        // Take mesh scale into account
        float sx = v.x * mesh->scale.x;
        float sy = v.y * mesh->scale.y;
        float sz = v.z * mesh->scale.z;

        float dist_sq = (sx * sx) + (sy * sy) + (sz * sz);
        if (dist_sq > max_dist_sq) {
            max_dist_sq = dist_sq;
        }
    }

    return sqrt(max_dist_sq);
}

void fit_camera_to_mesh(void) {
    float radius = 0.0;
    for (int mesh_index = 0; mesh_index < get_num_meshes(); mesh_index++) {
        radius += get_mesh_radius(get_mesh(mesh_index));
    }
    float padding_factor = 1.2;  // 20% padding around the object

    if (projection_type == PROJ_PERSPECTIVE) {
        // Geometry: To fit a sphere of 'radius' in a FOV of 60 degrees:
        // distance = radius / sin(fov / 2)
        // sin(30 deg) = 0.5
        // distance = radius / 0.5 = radius * 2.0
        orbit_radius = (radius * 2.0) * padding_factor;

        // Clamp to a safe minimum
        if (orbit_radius < 2.0) orbit_radius = 2.0;

    } else {
        // For Ortho, the height just needs to be the diameter (radius * 2)
        ortho_height = (radius * 2.0) * padding_factor;

        // Clamp to safe minimum
        if (ortho_height < 1.0) ortho_height = 1.0;
    }

    // Apply the new projection settings immediately
    update_projection_matrix();
}

void setup(void) {
    set_render_method(RENDER_WIRE);
    set_cull_method(CULL_BACKFACE);

    // Initialize defaults
    projection_type = PROJ_PERSPECTIVE;
    orbit_radius = 5.0;

    // load_mesh("./assets/terrain.obj", "./assets/terrain.png",
    //           vec3_new(0.5, 0.5, 0.5), vec3_new(0, -20.0, 0),
    //           vec3_new(M_PI / 2, 0, 0));
    // load_mesh("./assets/f22.obj", "./assets/f22.png", vec3_new(1, 1, 1),
    //           vec3_new(0, 0, +5), vec3_new(0, 0, 0));
    // load_mesh("./assets/efa.obj", "./assets/efa.png", vec3_new(1, 1, 1),
    //           vec3_new(-2, 0, +9), vec3_new(0, 0, 0));
    // load_mesh("./assets/f117.obj", "./assets/f117.png", vec3_new(1, 1, 1),
    //           vec3_new(+2, 0, +9), vec3_new(0, 0, 0));

    load_mesh("./assets/runway.obj", "./assets/runway.png", vec3_new(1, 1, 1),
              vec3_new(0, -1.5, +23), vec3_new(0, 0, 0));
    load_mesh("./assets/f22.obj", "./assets/f22.png", vec3_new(1, 1, 1),
              vec3_new(0, -1.3, +5), vec3_new(0, -M_PI / 2, 0));
    load_mesh("./assets/efa.obj", "./assets/efa.png", vec3_new(1, 1, 1),
              vec3_new(-2, -1.3, +9), vec3_new(0, -M_PI / 2, 0));
    load_mesh("./assets/f117.obj", "./assets/f117.png", vec3_new(1, 1, 1),
              vec3_new(+2, -1.3, +9), vec3_new(0, -M_PI / 2, 0));

    // fit_camera_to_mesh();  // This will also call update_projection_matrix
    update_projection_matrix();
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
                if (event.key.keysym.sym == SDLK_p) {
                    // PERSPECTIVE
                    projection_type = PROJ_PERSPECTIVE;
                    fit_camera_to_mesh();
                    break;
                }
                if (event.key.keysym.sym == SDLK_o) {
                    // ORTHOGRAPHIC
                    projection_type = PROJ_ORTHOGRAPHIC;
                    orbit_radius = ORTHO_CAMERA_DISTANCE;
                    fit_camera_to_mesh();
                    break;
                }
                if (event.key.keysym.sym == SDLK_f) {
                    fit_camera_to_mesh();
                    break;
                }
            // ORBIT CONTROLS
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    is_mouse_down = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    is_mouse_down = false;
                }
                break;
            case SDL_MOUSEMOTION:
                if (is_mouse_down) {
                    // update angles
                    orbit_theta += event.motion.xrel * 0.01;
                    orbit_phi += event.motion.yrel * 0.01;

                    // clamp so as to not flip
                    if (orbit_phi > 1.5) orbit_phi = 1.5;
                    if (orbit_phi < -1.5) orbit_phi = -1.5;
                }
                break;
            case SDL_MOUSEWHEEL:
                if (projection_type == PROJ_PERSPECTIVE) {
                    orbit_radius -= event.wheel.y * 0.5;
                    if (orbit_radius < 1.0) orbit_radius = 1.0;
                } else {
                    // scale frustum
                    ortho_height -= event.wheel.y * 0.5;
                    if (ortho_height < 0.5) ortho_height = 0.5;
                    update_projection_matrix();
                }
                break;
                // ORBIT CONTROLS END
        }
    }
}

void update(void) {
    // Wait some time until the reach the target frame time in milliseconds
    int time_to_wait =
        FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

    // Only delay execution if we are running too fast
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
        SDL_Delay(time_to_wait);
    }

    // delta time factor converted to seconds to be used to update game
    // objects
    delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;

    previous_frame_time = SDL_GetTicks();

    // reset number of triangles to 0
    num_triangles_to_render = 0;

    // mesh.translation.z = 5.0;

    if (projection_type == PROJ_ORTHOGRAPHIC) {
        orbit_radius = ORTHO_CAMERA_DISTANCE;
    }

    // THIS IS THE ORBIT CONTROLS
    camera.position.x = orbit_radius * cos(orbit_phi) * sin(orbit_theta);
    camera.position.y = orbit_radius * sin(orbit_phi);
    camera.position.z = orbit_radius * cos(orbit_phi) * cos(orbit_theta);

    // look at mesh
    vec3_t target = {0, 0, 5.0};  // align with mesh.translation.z

    // offset camera position by target
    camera.position = vec3_add(camera.position, target);
    vec3_t up_direction = {0, 1, 0};

    // create view matrix
    view_matrix = mat4_look_at(camera.position, target, up_direction);

    // loop all the meshes in our scene
    for (int mesh_index = 0; mesh_index < get_num_meshes(); mesh_index++) {
        mesh_t* mesh = get_mesh(mesh_index);

        mat4_t scale_matrix =
            mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
        mat4_t translation_matrix = mat4_make_translation(
            mesh->translation.x, mesh->translation.y, mesh->translation.z);
        mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh->rotation.x);
        mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh->rotation.y);
        mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh->rotation.z);

        // loop all triangle faces of our mesh
        int num_faces = array_length(mesh->faces);
        for (int i = 0; i < num_faces; i++) {
            face_t mesh_face = mesh->faces[i];

            vec3_t face_vertices[3];
            face_vertices[0] = mesh->vertices[mesh_face.a];
            face_vertices[1] = mesh->vertices[mesh_face.b];
            face_vertices[2] = mesh->vertices[mesh_face.c];

            vec4_t transformed_vertices[3];

            // loop all three vertices of this current face and apply
            // transformations

            for (int j = 0; j < 3; j++) {
                vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

                // Create a world matrix combining scale, rotation and
                // translation matrices
                world_matrix = mat4_identity();

                // Order matters: First scale, rotate, translate [S] * [R] * [T]
                // * v
                world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
                world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
                world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
                world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
                world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

                // TODO: multiply the world matrix by the original vector
                transformed_vertex =
                    mat4_mul_vec4(world_matrix, transformed_vertex);

                // multiply the vector to transform the scene to camera space
                transformed_vertex =
                    mat4_mul_vec4(view_matrix, transformed_vertex);

                // Save transformed vertex in the array of transformed vertices
                transformed_vertices[j] = transformed_vertex;
            }

            // TODO: Check backface culling
            // Note: We defined Clock-wise vertex numbering for our engine
            // 1. Find vectors B-A and C-A
            vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /* A   */
            vec3_t vector_b =
                vec3_from_vec4(transformed_vertices[1]); /* / \  */
            vec3_t vector_c =
                vec3_from_vec4(transformed_vertices[2]); /* C---B */

            // Get vector sub of B-A and C-A
            vec3_t vector_ab = vec3_sub(vector_b, vector_a);
            vec3_t vector_ac = vec3_sub(vector_c, vector_a);

            // 2. Take their cross product and find the perpendicular normal N
            vec3_t normal = vec3_cross(vector_ab, vector_ac);
            vec3_normalize(&normal);

            // 3. Find the camera ray vector
            vec3_t camera_ray;

            if (projection_type == PROJ_PERSPECTIVE) {
                // Perspective: Ray from origin (camera) to vertex
                vec3_t origin = {0, 0, 0};
                camera_ray = vec3_sub(origin, vector_a);
            } else {
                // Orthographic: Parallel rays looking down the Z axis
                // Since View space conventionally looks down -Z, the vector TO
                // camera is +Z
                camera_ray = (vec3_t){0, 0, -1.0};
            }

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

            // clip the polygon and return a new polygon with potential new
            // vertices
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
                triangle_t triangle_after_clipping =
                    triangles_after_clipping[t];
                // Loop all three vertices to perform projection
                vec4_t projected_points[3];
                for (int j = 0; j < 3; j++) {
                    // project the current vertex
                    projected_points[j] = mat4_mul_vec4_project(
                        proj_matrix, triangle_after_clipping.points[j]);

                    if (projection_type == PROJ_ORTHOGRAPHIC) {
                        float z_normalized = projected_points[j].z;
                        projected_points[j].w =
                            1.0 / ((1.0 - z_normalized) + 0.0001);
                    }

                    // scale into viewport
                    projected_points[j].y *= -1;

                    projected_points[j].x *= (get_window_width() / 2.0);
                    projected_points[j].y *= (get_window_height() / 2.0);
                    // translate the projected points to the middle of the
                    // screen
                    projected_points[j].x += (get_window_width() / 2.0);
                    projected_points[j].y += (get_window_height() / 2.0);
                }

                // calculate the shade intensity based on how aligned ois the
                // face normal and the light
                float light_intensity_factor =
                    -1 *
                    vec3_dot(
                        normal,
                        light.direction);  // fixing the light intensity
                                           // factor to point inwards with -1

                // calculate the triangle color based on light angle
                uint32_t triangle_color = light_apply_intensity(
                    mesh_face.color, light_intensity_factor);

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
                    .color = triangle_color,
                    .texture = mesh->texture};

                // save projected triangle to the array of triangles to render
                if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH) {
                    triangles_to_render[num_triangles_to_render] =
                        projected_triangle;
                    num_triangles_to_render++;
                }
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
                triangle.texcoords[2].u, triangle.texcoords[2].v,
                triangle.texture);
        }

        if (should_render_wireframe()) {
            draw_triangle(triangle.points[0].x, triangle.points[0].y,
                          triangle.points[0].z, triangle.points[0].w,
                          triangle.points[1].x, triangle.points[1].y,
                          triangle.points[1].z, triangle.points[1].w,
                          triangle.points[2].x, triangle.points[2].y,
                          triangle.points[2].z, triangle.points[2].w,
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
    free_meshes();
    destroy_window();
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
    free_resources();

    return 0;
}