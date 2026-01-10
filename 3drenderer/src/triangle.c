#include "triangle.h"

#include "display.h"
#include "swap.h"

void draw_triangle(int x0, int y0, float z0, float w0, int x1, int y1, float z1,
                   float w1, int x2, int y2, float z2, float w2,
                   uint32_t color) {
    draw_line(x0, y0, z0, w0, x1, y1, z1, w1, color);
    draw_line(x1, y1, z1, w1, x2, y2, z2, w2, color);
    draw_line(x2, y2, z2, w2, x0, y0, z0, w0, color);
}

void draw_filled_triangle(int x0, int y0, float z0, float w0, int x1, int y1,
                          float z1, float w1, int x2, int y2, float z2,
                          float w2, uint32_t color) {
    // 1. Sort the vertices by y-coordinate ascending (y0 < y1 < y2)
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }

    // Create vector aliases for cleaner barycentric calculation
    vec2_t point_a = {x0, y0};
    vec2_t point_b = {x1, y1};
    vec2_t point_c = {x2, y2};

    // 2. Render the Upper Part (Flat-Bottom)
    float inv_slope_1 = 0;
    float inv_slope_2 = 0;

    if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            if (x_end < x_start) int_swap(&x_start, &x_end);

            for (int x = x_start; x < x_end; x++) {
                // Calculate Barycentric weights for current pixel (x,y)
                vec2_t p = {x, y};
                vec3_t weights =
                    barycentric_weights(point_a, point_b, point_c, p);

                float alpha = weights.x;
                float beta = weights.y;
                float gamma = weights.z;

                // Interpolate the value of 1/w for the current pixel
                float interpolated_reciprocal_w =
                    (1 / w0) * alpha + (1 / w1) * beta + (1 / w2) * gamma;

                // Adjust 1/w so pixels closer to camera have smaller values
                // (Matches the logic in your draw_texel function)
                interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w;

                // Only draw pixel if depth is closer than what's in Z-buffer
                if (interpolated_reciprocal_w < get_zbuffer_at(x, y)) {
                    draw_pixel(x, y, color);
                    update_zbuffer_at(x, y, interpolated_reciprocal_w);
                }
            }
        }
    }

    // 3. Render the Lower Part (Flat-Top)
    inv_slope_1 = 0;
    inv_slope_2 = 0;

    if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            if (x_end < x_start) int_swap(&x_start, &x_end);

            for (int x = x_start; x < x_end; x++) {
                // Calculate Barycentric weights for current pixel (x,y)
                vec2_t p = {x, y};
                vec3_t weights =
                    barycentric_weights(point_a, point_b, point_c, p);

                float alpha = weights.x;
                float beta = weights.y;
                float gamma = weights.z;

                // Interpolate the value of 1/w
                float interpolated_reciprocal_w =
                    (1 / w0) * alpha + (1 / w1) * beta + (1 / w2) * gamma;

                // Adjust 1/w
                interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w;

                // Z-buffer check
                if (interpolated_reciprocal_w < get_zbuffer_at(x, y)) {
                    draw_pixel(x, y, color);
                    update_zbuffer_at(x, y, interpolated_reciprocal_w);
                }
            }
        }
    }
}

vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p) {
    // find the vectors between the vertices ABC and point p
    vec2_t ac = vec2_sub(c, a);
    vec2_t ab = vec2_sub(b, a);
    vec2_t ap = vec2_sub(p, a);
    vec2_t pc = vec2_sub(c, p);
    vec2_t pb = vec2_sub(b, p);

    // compute area of parallelogram/triangle ABC using 2d cross product (z = 0)
    float area_parallelogram_abc = (ac.x * ab.y - ac.y * ab.x);

    // Alpha is area of small parallelogram/triangle PBC divided by full
    // triangle
    float alpha = (pc.x * pb.y - pc.y * pb.x) / area_parallelogram_abc;

    // Beta is triangle APC / ABC
    float beta = (ac.x * ap.y - ac.y * ap.x) / area_parallelogram_abc;

    // Gamma is found from alpha + beta + gamma = 1, but its the last area
    float gamma = 1 - alpha - beta;

    vec3_t weights = {alpha, beta, gamma};
    return weights;
}

// function to draw textured pixel at position x and y using interpolation
void draw_texel(int x, int y, upng_t* texture, vec4_t point_a, vec4_t point_b,
                vec4_t point_c, tex2_t a_uv, tex2_t b_uv, tex2_t c_uv

) {
    vec2_t p = {x, y};
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);

    vec3_t weights = barycentric_weights(a, b, c, p);
    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;

    // variables to store interpolated values of U, V and 1/W
    float interpolated_u;
    float interpolated_v;
    float interpolated_reciprocal_w;

    // perform the interpolation of all U/w and V/w values using barycentric
    // weights and factor of 1/2
    interpolated_u = (a_uv.u / point_a.w) * alpha +
                     (b_uv.u / point_b.w) * beta + (c_uv.u / point_c.w) * gamma;
    interpolated_v = (a_uv.v / point_a.w) * alpha +
                     (b_uv.v / point_b.w) * beta + (c_uv.v / point_c.w) * gamma;

    // interpolate value of 1/w for current pixel
    interpolated_reciprocal_w = (1 / point_a.w) * alpha +
                                (1 / point_b.w) * beta +
                                (1 / point_c.w) * gamma;

    // divide by 1/w on interpolated values to reverse the reciprocal
    interpolated_u /= interpolated_reciprocal_w;
    interpolated_v /= interpolated_reciprocal_w;

    // get mesh texture width and height
    int texture_width = upng_get_width(texture);
    int texture_height = upng_get_height(texture);

    // map the UV coordinate to the full texture width and height
    int tex_x = abs((int)(interpolated_u * texture_width)) % texture_width;
    int tex_y = abs((int)(interpolated_v * texture_height)) % texture_height;

    // adjust 1/w so the pixels that are closer to the camera have smaller
    // values
    interpolated_reciprocal_w = 1 - interpolated_reciprocal_w;

    // only draw pixel if the depth value is less than the one previously
    // stored
    // in z_buffer
    if (interpolated_reciprocal_w < get_zbuffer_at(x, y)) {
        // get the buffer of colors from texture
        uint32_t* texture_buffer = upng_get_buffer(texture);

        // traverse texture source and fetch color for that pixel
        draw_pixel(x, y, texture_buffer[texture_width * tex_y + tex_x]);

        // update the z buffer value with the 1/w of this current pixel
        update_zbuffer_at(x, y, interpolated_reciprocal_w);
    }
}

// Draw textured triangle with the flat-top/flat-bottom method
void draw_textured_triangle(int x0, int y0, float z0, float w0, float u0,
                            float v0, int x1, int y1, float z1, float w1,
                            float u1, float v1, int x2, int y2, float z2,
                            float w2, float u2, float v2, upng_t* texture) {
    // TODO:
    // Loop all the pixels of the triangle to render them based on the color
    // that comes from the texture.
    // Step 1: Step vertices by y-coordinate ascending (y0 < y1 < y2)
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&u1, &u2);
        float_swap(&v1, &v2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }

    // Flip the V component to account for the UV-Coordinates (V grows
    // downwards)
    v0 = 1.0 - v0;
    v1 = 1.0 - v1;
    v2 = 1.0 - v2;

    // create vector points and texture coords after we sort vertices
    vec4_t point_a = {x0, y0, z0, w0};
    vec4_t point_b = {x1, y1, z1, w1};
    vec4_t point_c = {x2, y2, z2, w2};
    tex2_t a_uv = {u0, v0};
    tex2_t b_uv = {u1, v1};
    tex2_t c_uv = {u2, v2};

    // Render the upper part of the triangle (flat-bottom)
    float inv_slope_1 = 0;
    float inv_slope_2 = 0;

    if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            if (x_end < x_start) {
                int_swap(&x_start, &x_end);
            }

            for (int x = x_start; x < x_end; x++) {
                // TODO: draw our pixel with the color that comes from the
                // texture
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv,
                           c_uv);
            }
        }
    }

    // Render the lower part of the triangle (flat-top)
    inv_slope_1 = 0;
    inv_slope_2 = 0;

    if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            if (x_end < x_start) {
                int_swap(&x_start, &x_end);
            }

            for (int x = x_start; x < x_end; x++) {
                // TODO: draw our pixel with the color that comes from the
                // texture
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv,
                           c_uv);
            }
        }
    }
}

vec3_t get_triangle_normal(vec4_t vertices[3]) {
    // TODO: Check backface culling
    // Note: We defined Clock-wise vertex numbering for our engine
    // 1. Find vectors B-A and C-A
    vec3_t vector_a = vec3_from_vec4(vertices[0]); /* A   */
    vec3_t vector_b = vec3_from_vec4(vertices[1]); /* / \  */
    vec3_t vector_c = vec3_from_vec4(vertices[2]); /* C---B */

    // Get vector sub of B-A and C-A
    vec3_t vector_ab = vec3_sub(vector_b, vector_a);
    vec3_t vector_ac = vec3_sub(vector_c, vector_a);

    // 2. Take their cross product and find the perpendicular normal N
    vec3_t normal = vec3_cross(vector_ab, vector_ac);
    vec3_normalize(&normal);

    return normal;
}
