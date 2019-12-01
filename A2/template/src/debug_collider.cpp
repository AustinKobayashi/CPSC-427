#include "debug_collider.hpp"

#include <iostream>
#include <math.h>

bool DebugCollider::init(vec2 x_bounds, vec2 y_bounds, vec2 window_size, vec2 salmon_scale) {

    if (gl_has_errors())
        return false;

    // Loading shaders
    if (!effect.load_from_file(shader_path("debug_collider.vs.glsl"), shader_path("debug_collider.fs.glsl")))
        return false;

    m_x_bounds = x_bounds;
    m_y_bounds = y_bounds;
    m_level_size = window_size;
    m_salmon_scale = salmon_scale;

    return true;
}

// Releases all graphics resources
void DebugCollider::destroy() {
    glDeleteBuffers(1, &mesh.vbo);

    glDeleteShader(effect.vertex);
    glDeleteShader(effect.fragment);
    glDeleteShader(effect.program);
}


void DebugCollider::draw(const mat3& projection) {
    vec2 offset = {0, 0};
    offset.x = sub(m_salmon_position, mul(m_level_size, 0.5f)).x;
    offset.y = sub(mul(m_level_size, 0.5f), m_salmon_position).y;

    vec2 top_left = {(offset.x + m_x_bounds.x * abs(m_salmon_scale.x)) / (m_level_size.x / 2.f),
                     (offset.y + m_y_bounds.x * abs(m_salmon_scale.y)) / (m_level_size.y / 2.f)};

    vec2 top_right = {(offset.x  + m_x_bounds.y * abs(m_salmon_scale.x)) / (m_level_size.x / 2.f),
                     (offset.y + m_y_bounds.x * abs(m_salmon_scale.y)) / (m_level_size.y / 2.f)};

    vec2 bottom_left = {(offset.x + m_x_bounds.x * abs(m_salmon_scale.x)) / (m_level_size.x / 2.f),
                     (offset.y + m_y_bounds.y * abs(m_salmon_scale.y)) / (m_level_size.y / 2.f)};

    vec2 bottom_right = {(offset.x  + m_x_bounds.y * abs(m_salmon_scale.x)) / (m_level_size.x / 2.f),
                        (offset.y + m_y_bounds.y * abs(m_salmon_scale.y)) / (m_level_size.y / 2.f)};

    int index = 0;
    float screen_vertex_buffer_data[3 * 3 * 2];

    screen_vertex_buffer_data[index++] = top_left.x;
    screen_vertex_buffer_data[index++] = top_left.y;
    screen_vertex_buffer_data[index++] = 0;

    screen_vertex_buffer_data[index++] = top_right.x;
    screen_vertex_buffer_data[index++] = top_right.y;
    screen_vertex_buffer_data[index++] = 0;

    screen_vertex_buffer_data[index++] = bottom_left.x;
    screen_vertex_buffer_data[index++] = bottom_left.y;
    screen_vertex_buffer_data[index++] = 0;


    screen_vertex_buffer_data[index++] = bottom_left.x;
    screen_vertex_buffer_data[index++] = bottom_left.y;
    screen_vertex_buffer_data[index++] = 0;

    screen_vertex_buffer_data[index++] = top_right.x;
    screen_vertex_buffer_data[index++] = top_right.y;
    screen_vertex_buffer_data[index++] = 0;

    screen_vertex_buffer_data[index++] = bottom_right.x;
    screen_vertex_buffer_data[index++] = bottom_right.y;
    screen_vertex_buffer_data[index++] = 0;

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertex_buffer_data), screen_vertex_buffer_data, GL_STATIC_DRAW);

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Setting shaders
    glUseProgram(effect.program);

    // Setting vertices and indices
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 2*3*3);

    glDisableVertexAttribArray(0);
}

void DebugCollider::set_salmon_position(vec2 salmon_position) {
    m_salmon_position = salmon_position;
}
