#define _USE_MATH_DEFINES

// Header
#include "pebbles.hpp"

#include <cmath>
#include <iostream>
#include <list>

#define PI 3.14159265

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static const int MAX_PEBBLES = 25;
constexpr int NUM_SEGMENTS = 12;

bool Pebbles::init(vec2 level_bounds, float current_speed) {
	std::vector<GLfloat> screen_vertex_buffer_data;
	constexpr float z = -0.1;

	for (int i = 0; i < NUM_SEGMENTS; i++) {
		screen_vertex_buffer_data.push_back(std::cos(M_PI * 2.0 * float(i) / (float)NUM_SEGMENTS));
		screen_vertex_buffer_data.push_back(std::sin(M_PI * 2.0 * float(i) / (float)NUM_SEGMENTS));
		screen_vertex_buffer_data.push_back(z);

		screen_vertex_buffer_data.push_back(std::cos(M_PI * 2.0 * float(i + 1) / (float)NUM_SEGMENTS));
		screen_vertex_buffer_data.push_back(std::sin(M_PI * 2.0 * float(i + 1) / (float)NUM_SEGMENTS));
		screen_vertex_buffer_data.push_back(z);

		screen_vertex_buffer_data.push_back(0);
		screen_vertex_buffer_data.push_back(0);
		screen_vertex_buffer_data.push_back(z);
	}

	// Clearing errors
	gl_flush_errors();

	// Vertex Buffer creation
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, screen_vertex_buffer_data.size()*sizeof(GLfloat), screen_vertex_buffer_data.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &m_instance_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);

	if (gl_has_errors())
		return false;

	// Loading shaders
	if (!effect.load_from_file(shader_path("pebble.vs.glsl"), shader_path("pebble.fs.glsl")))
		return false;

	m_min_radius = 7;
	m_max_radius = 10;
    srand (time(NULL));

    m_x_level_bounds = {0, level_bounds.x};
    m_y_level_bounds = {0, level_bounds.y};

    m_mode3 = false;
    m_current_speed = current_speed;

    return true;
}

// Releases all graphics resources
void Pebbles::destroy() {
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &m_instance_vbo);

	glDeleteShader(effect.vertex);
	glDeleteShader(effect.fragment);
	glDeleteShader(effect.program);

	m_pebbles.clear();
}

void Pebbles::update(float ms, Salmon& salmon) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE PEBBLE UPDATES HERE
	// You will need to handle both the motion of pebbles 
	// and the removal of dead pebbles.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (m_pebbles.empty())
	    return;

	// Delete old pebbles
    for (std::vector<Pebble>::iterator it = m_pebbles.begin(); it != m_pebbles.end(); it++) {
        it->life -= ms;
    }
    int index = 0;
    while (index != m_pebbles.size() && !m_pebbles.empty()) {
        Pebble pebble = *(m_pebbles.begin() + index);
        if (pebble.life < 0 ||
            pebble.position.x + pebble.radius < m_x_level_bounds.x ||
            pebble.position.x - pebble.radius > m_x_level_bounds.y ||
            pebble.position.y + pebble.radius < m_y_level_bounds.x ||
            pebble.position.y - pebble.radius > m_y_level_bounds.y) {
                m_pebbles.erase(m_pebbles.begin() + index);
        } else {
            index++;
        }
    }

    // Move pebbles
	for (auto &it : m_pebbles) {
	    if (m_mode3)
	        it.acceleration.x = -1.f * m_current_speed * ms;
        else
            it.acceleration.x = 0;

	    it.velocity.x += it.acceleration.x;
	    it.velocity.y += it.acceleration.y;

	    it.position.x += it.velocity.x * (ms / 1000);
	    it.position.y += it.velocity.y * (ms / 1000);
	}

    for (auto &it : m_pebbles) {
        if (!it.can_collide_with_salmon) {
            if (len(sub(it.position, salmon.get_mouth_pos())) > 2 * it.radius) {
                salmon.calculate_corners();
                vec2 top_left = salmon.get_top_left_corner();
                vec2 top_right = salmon.get_top_right_corner();
                vec2 bottom_left = salmon.get_bottom_left_corner();
                vec2 bottom_right = salmon.get_bottom_right_corner();

                vec2 delta_pos = sub(salmon.get_position(), it.position);
                vec2 closest = add(it.position, mul(normalize(delta_pos), it.radius));

                if (!is_inside(closest, top_left, top_right, bottom_left) &&
                    !is_inside(closest, bottom_right, top_right, bottom_left))
                    it.can_collide_with_salmon = true;
            }
        }
    }
}

void Pebbles::spawn_pebble(vec2 position, float salmon_rotation, float gravity) {
	if (m_pebbles.size() > MAX_PEBBLES)
	    return;

	Pebble pebble;
	pebble.life = 30000;
	pebble.position = position;
	pebble.radius = rand() % m_max_radius + m_min_radius;

	float angle = salmon_rotation + (rand() % 30 - 15) * (PI / 180);
	vec2 base_speed = {250, 0};

    float x = cos(angle) * base_speed.x - sin(angle) * base_speed.y;
    float y = sin(angle) * base_speed.x + cos(angle) * base_speed.y;

    pebble.velocity = {x, y};
    pebble.acceleration = {0, gravity};
    pebble.can_collide_with_salmon = false;
    m_pebbles.push_back(pebble);
}

void Pebbles::collides_with_pebble() {
    for (auto &outer : m_pebbles) {
        for (auto &inner : m_pebbles) {
            if (outer != inner) {
                float distance = len(sub(outer.position, inner.position));

                if (distance <= outer.radius + inner.radius) {
                    vec2 outer_vel = pebble_pebble_bounce(outer, inner);
                    vec2 inner_vel = pebble_pebble_bounce(inner, outer);

                    outer.velocity = outer_vel;
                    inner.velocity = inner_vel;

                    float overlap = (outer.radius + inner.radius) - distance;

                    if (overlap > 0) {
                        outer.position = add(outer.position, mul(normalize(sub(outer.position, inner.position)), overlap));
                    }
                }
            }
        }
    }
}

void Pebbles::collides_with(Turtle& turtle) {
    for (auto &pebble : m_pebbles) {
        vec2 delta_pos = sub(pebble.position, turtle.get_position());
        float d_sq = delta_pos.x * delta_pos.x + delta_pos.y * delta_pos.y;
        float other_r = std::max(turtle.get_bounding_box().x, turtle.get_bounding_box().y);
        float my_r = pebble.radius;
        float r = std::max(other_r, my_r);
        r *= 0.6f;
        if (d_sq < r * r) {
            vec2 normal = normalize(sub(pebble.position, turtle.get_position()));
            pebble.velocity = sub(pebble.velocity, mul(normal, 2 * dot(normal, pebble.velocity)));

            float overlap = sqrt(r * r) - sqrt(d_sq);
            if (overlap > 0) {
                pebble.position = add(pebble.position, mul(normalize(delta_pos), overlap));
            }

            if (m_mode3)
                turtle.turn_around();
        }
    }
}

void Pebbles::collides_with(Fish& fish) {
    for (auto &pebble : m_pebbles) {
        vec2 delta_pos = sub(pebble.position, fish.get_position());
        float d_sq = delta_pos.x * delta_pos.x + delta_pos.y * delta_pos.y;
        float other_r = std::max(fish.get_bounding_box().x, fish.get_bounding_box().y);
        float my_r = pebble.radius;
        float r = std::max(other_r, my_r);
        r *= 0.6f;

        if (d_sq < r * r) {
            vec2 normal = normalize(sub(pebble.position, fish.get_position()));
            pebble.velocity = sub(pebble.velocity, mul(normal, 2 * dot(normal, pebble.velocity)));

            float overlap = sqrt(r * r) - sqrt(d_sq);
            if (overlap > 0) {
                pebble.position = add(pebble.position, mul(normalize(delta_pos), overlap));
            }

            if (m_mode3)
                fish.slow_down();
        }
    }
}

void Pebbles::collides_with(Salmon& salmon) {

    salmon.calculate_corners();
    vec2 top_left = salmon.get_top_left_corner();
    vec2 top_right = salmon.get_top_right_corner();
    vec2 bottom_left = salmon.get_bottom_left_corner();
    vec2 bottom_right = salmon.get_bottom_right_corner();

    for (auto &pebble : m_pebbles) {
        if (pebble.can_collide_with_salmon) {
            vec2 delta_pos = sub(salmon.get_position(), pebble.position);

            vec2 closest = add(pebble.position, mul(normalize(delta_pos), pebble.radius));

            if (is_inside(closest, top_left, top_right, bottom_left) ||
                    is_inside(closest, bottom_right, top_right, bottom_left)) {

                float top_dist = dist(closest, top_left, top_right);
                float bottom_dist = dist(closest, bottom_left, bottom_right);
                float left_dist = dist(closest, top_left, bottom_left);
                float right_dist = dist(closest, top_right, bottom_right);

                vec2 normal;

                if (top_dist < std::min(right_dist, std::min(bottom_dist, left_dist))) {
                    normal = normalize(sub(top_left, bottom_left));
                    pebble.position = add(pebble.position, mul(normal, top_dist));

                } else if (bottom_dist < std::min(top_dist, std::min(left_dist, right_dist))) {
                    normal = normalize(sub(bottom_left, top_left));
                    pebble.position = add(pebble.position, mul(normal, bottom_dist));

                } else if (left_dist < std::min(top_dist, std::min(bottom_dist, right_dist))) {
                    normal = normalize(sub(top_left, top_right));
                    pebble.position = add(pebble.position, mul(normal, left_dist));

                } else {
                    normal = normalize(sub(top_right, top_left));
                    pebble.position = add(pebble.position, mul(normal, right_dist));
                }

                pebble.velocity = sub(pebble.velocity, mul(normal, 2 * dot(normal, pebble.velocity)));
            }
        }
    }
}

float Pebbles::sign (vec2 p1, vec2 p2, vec2 p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool Pebbles::is_inside (vec2 pt, vec2 v1, vec2 v2, vec2 v3) {

    float d1 = sign(pt, v1, v2);
    float d2 = sign(pt, v2, v3);
    float d3 = sign(pt, v3, v1);

    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

float Pebbles::dist (vec2 pt, vec2 p1, vec2 p2) {
    float num = abs ((p2.y - p1.y)*pt.x - (p2.x - p1.x)*pt.y + p2.x*p1.y - p2.y*p1.x);
    float den = sqrt((p2.y - p1.y)*(p2.y - p1.y) + (p2.x - p1.x)*(p2.x - p1.x));

    return num/den;
}

vec2 Pebbles::pebble_pebble_bounce(Pebble &pebble1, Pebble& pebble2) {
    float mass_comp = 1;
    vec2 delta_pos = sub(pebble1.position, pebble2.position);
    float delta_vel = dot(sub(pebble1.velocity, pebble2.velocity), delta_pos) / sq_len(delta_pos);
    return sub(pebble1.velocity, mul(delta_pos, mass_comp * delta_vel));
}

// Draw pebbles using instancing
void Pebbles::draw(const mat3& projection) {
	// Setting shaders
	glUseProgram(effect.program);

  	// Enabling alpha channel for textures
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	// Getting uniform locations
	GLint projection_uloc = glGetUniformLocation(effect.program, "projection");
	GLint color_uloc = glGetUniformLocation(effect.program, "color");

	// Pebble color
	float color[] = { 0.4f, 0.4f, 0.4f };
	glUniform3fv(color_uloc, 1, color);
	glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);

	// Draw the screen texture on the geometry
	// Setting vertices
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

	// Mesh vertex positions
	// Bind to attribute 0 (in_position) as in the vertex shader
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribDivisor(0, 0);

	// Load up pebbles into buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_pebbles.size() * sizeof(Pebble), m_pebbles.data(), GL_DYNAMIC_DRAW);

	// Pebble translations
	// Bind to attribute 1 (in_translate) as in vertex shader
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Pebble), (GLvoid*)offsetof(Pebble, position));
	glVertexAttribDivisor(1, 1);

	// Pebble radii
	// Bind to attribute 2 (in_scale) as in vertex shader
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Pebble), (GLvoid*)offsetof(Pebble, radius));
	glVertexAttribDivisor(2, 1);

	// Draw using instancing
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysInstanced.xhtml
	glDrawArraysInstanced(GL_TRIANGLES, 0, NUM_SEGMENTS*3, m_pebbles.size());

  	// Reset divisor
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
}

void Pebbles::set_mode3(bool mode3) {
    m_mode3 = mode3;
}

void Pebbles::set_current_speed(float current_speed) {
    m_current_speed = current_speed;
}
