#include "water.hpp"
#include "debug_path.hpp"

#include <iostream>
#include <math.h>

bool DebugPath::init(vec2 screen_size) {

	if (gl_has_errors())
		return false;

	// Loading shaders
	if (!effect.load_from_file(shader_path("debug_path.vs.glsl"), shader_path("debug_path.fs.glsl")))
		return false;

	m_screen_size = screen_size;

	return true;
}

// Releases all graphics resources
void DebugPath::destroy() {
	glDeleteBuffers(1, &mesh.vbo);

	glDeleteShader(effect.vertex);
	glDeleteShader(effect.fragment);
	glDeleteShader(effect.program);

	clear_paths();
}


void DebugPath::draw(const mat3& projection) {
    int count = m_paths.size();
    auto* screen_vertex_buffer_data = new float [count];

    int index = 0;
    for (auto it=m_paths.begin(); it != m_paths.end(); ++it) {
        screen_vertex_buffer_data[index++] = *it;
    }

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), screen_vertex_buffer_data, GL_DYNAMIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Setting shaders
    glUseProgram(effect.program);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_POINTS, 0, count/3);
    glPointSize(20.0);

    glDisableVertexAttribArray(0);

    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        std::cout << err << std::endl;
    }

    delete [] screen_vertex_buffer_data;
}


void DebugPath::clear_paths() {
    m_paths.clear();
}

void DebugPath::add_to_path(std::list<vec2> path) {
    for (auto it=path.begin(); it != path.end(); ++it) {
        m_paths.push_back(2.f * (it->x / m_screen_size.x) - 1.f);
        m_paths.push_back(2.f * ((m_screen_size.y - it->y) / m_screen_size.y) - 1.f);
        m_paths.push_back(0);
    }
}


