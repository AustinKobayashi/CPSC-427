#include "debug_boundaries.hpp"

#include <iostream>
#include <math.h>

bool DebugBoundaries::init(vec2 x_level_bounds, vec2 y_level_bounds, vec2 window_size) {
    // Since we are not going to apply transformation to this screen geometry
    // The coordinates are set to fill the standard openGL window [-1, -1 .. 1, 1]
    // Make the size slightly larger then the screen to crop the boundary.
    GLfloat screen_vertex_buffer_data[4 * 6];

    int index = 0;

    // Top left
    screen_vertex_buffer_data[index++] = (x_level_bounds.x / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.x / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;

    // Bottom Left
    screen_vertex_buffer_data[index++] = (x_level_bounds.x / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.y / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;

    // Bottom Left
    screen_vertex_buffer_data[index++] = (x_level_bounds.x / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.y / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;

    // Bottom Right
    screen_vertex_buffer_data[index++] = (x_level_bounds.y / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.y / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;

    // Bottom Right
    screen_vertex_buffer_data[index++] = (x_level_bounds.y / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.y / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;

    // Top Right
    screen_vertex_buffer_data[index++] = (x_level_bounds.y / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.x / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;


    // Top Right
    screen_vertex_buffer_data[index++] = (x_level_bounds.y / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.x / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;

    // Top left
    screen_vertex_buffer_data[index++] = (x_level_bounds.x / (window_size.x / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = (y_level_bounds.x / (window_size.y / 2.f)) - 1.f;
    screen_vertex_buffer_data[index++] = 0.f;


    // Clearing errors
    gl_flush_errors();

    // Vertex Buffer creation
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertex_buffer_data), screen_vertex_buffer_data, GL_STATIC_DRAW);

    if (gl_has_errors())
        return false;

    // Loading shaders
    if (!effect.load_from_file(shader_path("debug_boundaries.vs.glsl"), shader_path("debug_boundaries.fs.glsl")))
        return false;

    return true;
}

// Releases all graphics resources
void DebugBoundaries::destroy() {
    glDeleteBuffers(1, &mesh.vbo);

    glDeleteShader(effect.vertex);
    glDeleteShader(effect.fragment);
    glDeleteShader(effect.program);
}


void DebugBoundaries::draw(const mat3& projection) {

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

    glDrawArrays(GL_LINES, 0, 4*6);
    glLineWidth(1);

    glDisableVertexAttribArray(0);
}

