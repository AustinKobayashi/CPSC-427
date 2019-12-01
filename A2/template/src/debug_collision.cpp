#include <iostream>
#include <math.h>
#include "debug_collision.hpp"

bool DebugCollision::init(Salmon& salmon) {

    if (gl_has_errors())
        return false;

    // Loading shaders
    if (!effect.load_from_file(shader_path("coloured.vs.glsl"), shader_path("coloured.fs.glsl")))
        return false;

    m_salmon = &salmon;
    m_point_time = 1500;
    return true;
}

// Releases all graphics resources
void DebugCollision::destroy() {
    glDeleteBuffers(1, &mesh.vbo);

    glDeleteShader(effect.vertex);
    glDeleteShader(effect.fragment);
    glDeleteShader(effect.program);

    m_points.clear();
}


void DebugCollision::update(float ms) {

    for (std::map<pos, float>::iterator it = m_points.begin(); it != m_points.end(); it++) {
        it->second += ms;
    }

    for (auto map_it = m_points.cbegin(); map_it != m_points.cend() /* not hoisted */; /* no increment */) {
        if (map_it->second > m_point_time) {
            m_points.erase(map_it++);
        } else {
            ++map_it;
        }
    }
}

void DebugCollision::draw(const mat3& projection) {

    int count = m_points.size();
    auto *screen_vertex_buffer_data = new float[count * 3];

    int index = 0;
    for (std::map<pos, float>::iterator it = m_points.begin(); it != m_points.end(); it++) {
        screen_vertex_buffer_data[index++] = it->first.first;
        screen_vertex_buffer_data[index++] = it->first.second;
        screen_vertex_buffer_data[index++] = 0;
    }

    glUseProgram(effect.program);

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, count * 3 * sizeof(float), screen_vertex_buffer_data, GL_STATIC_DRAW);

    // Getting uniform locations
    GLint transform_uloc = glGetUniformLocation(effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(effect.program, "projection");
    GLint color_uloc = glGetUniformLocation(effect.program, "color");

    // Pebble color
    float color[] = { 0.0f, 0.0f, 0.0f };
    glUniform3fv(color_uloc, 1, color);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);

    mat3 trans = m_salmon->get_transformation_matrix();
    transform.begin();
    transform.translate(m_salmon->get_position());
    transform.rotate(m_salmon->get_rotation());
    transform.scale(m_salmon->get_scale());
    transform.end();
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&trans);

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Setting shaders
    glUseProgram(effect.program);

    // Setting vertices and indices
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glDrawArrays(GL_POINTS, 0, count);
    glPointSize(10.0);

    glDisableVertexAttribArray(0);
}

void DebugCollision::add_collision_point(vec2 point) {

    auto tup = std::pair<float, float>(point.x, point.y);

    if (m_points.find(tup) == m_points.end() ) {
        m_points.insert( std::pair<pos, float>(tup, 0));
    } else {
        m_points[tup] = 0;
    }
}

