// Header
#include "turtle.hpp"

#include <cmath>
#include <iostream>

Texture Turtle::turtle_texture;

bool Turtle::init()
{
	// Load shared texture
	if (!turtle_texture.is_valid())
	{
		if (!turtle_texture.load_from_file(textures_path("turtle.png")))
		{
			fprintf(stderr, "Failed to load turtle texture!");
			return false;
		}
	}

	// The position corresponds to the center of the texture
	float wr = turtle_texture.width * 0.5f;
	float hr = turtle_texture.height * 0.5f;

	TexturedVertex vertices[4];
	vertices[0].position = { -wr, +hr, -0.02f };
	vertices[0].texcoord = { 0.f, 1.f };
	vertices[1].position = { +wr, +hr, -0.02f };
	vertices[1].texcoord = { 1.f, 1.f };
	vertices[2].position = { +wr, -hr, -0.02f };
	vertices[2].texcoord = { 1.f, 0.f };
	vertices[3].position = { -wr, -hr, -0.02f };
	vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction
	uint16_t indices[] = { 0, 3, 1, 1, 3, 2 };

	// Clearing errors
	gl_flush_errors();
	
	// Vertex Buffer creation
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TexturedVertex) * 4, vertices, GL_STATIC_DRAW);

	// Index Buffer creation
	glGenBuffers(1, &mesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * 6, indices, GL_STATIC_DRAW);

	// Vertex Array (Container for Vertex + Index buffer)
	glGenVertexArrays(1, &mesh.vao);
	if (gl_has_errors())
		return false;

	// Loading shaders
	if (!effect.load_from_file(shader_path("textured.vs.glsl"), shader_path("textured.fs.glsl")))
		return false;

	motion.radians = 0.f;
	motion.speed = 200.f;

	m_base_speed = motion.speed;
	m_mode2 = false;

	// Setting initial values, scale is negative to make it face the opposite way
	// 1.0 would be as big as the original texture.
	physics.scale = { -0.4f, 0.4f };

	return true;
}

// Releases all graphics resources
void Turtle::destroy()
{
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ibo);
	glDeleteBuffers(1, &mesh.vao);

	glDeleteShader(effect.vertex);
	glDeleteShader(effect.fragment);
	glDeleteShader(effect.program);
}

void Turtle::update(float ms)
{
	// Move fish along -X based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
    if (!m_mode2) {
        float step = -1.f * motion.speed * (ms / 1000);
        motion.position.x += step;
    } else {

        float step = motion.speed * (ms / 1000);

        while (m_path.size() > 1 && len(sub(m_path.front(), motion.position)) < 10) {
            m_path.pop_front();
        }

        vec2 direction = sub(m_path.front(), motion.position);

        if (direction.x != 0)
            motion.position.x += step * (direction.x / abs(direction.x));

        if (direction.y != 0)
            motion.position.y += step * (direction.y / abs(direction.y));
	}
}

void Turtle::draw(const mat3& projection)
{
	// Transformation code, see Rendering and Transformation in the template specification for more info
	// Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
	transform.begin();
	transform.translate(motion.position);
	transform.rotate(motion.radians);
	transform.scale(physics.scale);
	transform.end();

	// Setting shaders
	glUseProgram(effect.program);

	// Enabling alpha channel for textures
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Getting uniform locations for glUniform* calls
	GLint transform_uloc = glGetUniformLocation(effect.program, "transform");
	GLint color_uloc = glGetUniformLocation(effect.program, "fcolor");
	GLint projection_uloc = glGetUniformLocation(effect.program, "projection");

	// Setting vertices and indices
	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

	// Input data location as in the vertex buffer
	GLint in_position_loc = glGetAttribLocation(effect.program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effect.program, "in_texcoord");
	glEnableVertexAttribArray(in_position_loc);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));

	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, turtle_texture.id);

	// Setting uniform values to the currently bound program
	glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.out);
	float color[] = { 1.f, 1.f, 1.f };
	glUniform3fv(color_uloc, 1, color);
	glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);

	// Drawing!
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

vec2 Turtle::get_position()const
{
	return motion.position;
}

void Turtle::set_position(vec2 position)
{
	motion.position = position;
}

vec2 Turtle::get_bounding_box() const
{
	// Returns the local bounding coordinates scaled by the current size of the turtle 
	// fabs is to avoid negative scale due to the facing direction.
	return { std::fabs(physics.scale.x) * turtle_texture.width, std::fabs(physics.scale.y) * turtle_texture.height };
}

void Turtle::calculate_path(Salmon& salmon, std::vector<Fish>& fishes) {
    std::list<Sector> closed_list;
    std::list<Sector> open_list;

    float step = 50.f;

    vec2 position = {motion.position.x, motion.position.y};
    vec2 goal = salmon.get_position();

    Sector sector;
    sector.init(position, goal, step, step);

    open_list.push_front(sector);

    float x_pos[3] = {-step, 0 , step};
    float y_pos[9] = {step, step, step, 0, 0, 0, -step, -step, -step};

    while (!open_list.empty()) {
        Sector cur = open_list.front();
        float best_f = cur.get_f(closed_list.back().get_position());
        int index = 0;
        int best_index = 0;

        for (auto & it : open_list) {
            if (it.get_f(closed_list.back().get_position()) < best_f) {
                best_index = index;
                best_f = it.get_f(closed_list.back().get_position());
                cur = it;
            }
            index++;
        }

        auto it = open_list.begin();
        advance(it, best_index);
        open_list.erase(it);

        if (cur.is_goal())
            break;

        for (int i = 0; i < 9; i++) {
            float x = x_pos[i % 3] + cur.get_position().x;
            float y = y_pos[i] + cur.get_position().y;

            if (!(x == cur.get_position().x && y == cur.get_position().y)) {
                Sector successor;
                successor.init({x, y}, goal, step,
                               abs(cur.get_position().x - x) < 0.001 || abs(cur.get_position().y - y) < 0.001 ?
                               cur.get_move_cost() + step : cur.get_move_cost() + sqrt(pow(step, 2) + pow (step, 2)) - 1);

                if (successor.valid_sector_for_turtle(fishes)) {
                    if (!value_in_list(open_list, closed_list, successor) &&
                        !value_in_list(closed_list, closed_list, successor))
                            open_list.push_front(successor);
                }
            }
        }

        closed_list.push_back(cur);
    }

    Sector goal_sector;
    goal_sector.init(goal, goal, step, 0);

    closed_list.push_back(goal_sector);
    m_path.clear();

    for (auto & pos : closed_list) {
        m_path.push_back(pos.get_position());
    }

    closed_list.clear();
    open_list.clear();
}

bool Turtle::value_in_list(std::list<Sector> open_list, std::list<Sector> closed_list, Sector value) {
    for (auto & it : open_list) {
        if (len(sub(it.get_position(), value.get_position())) < 0.01) {
            if (it.get_f(closed_list.back().get_position()) <= value.get_f(closed_list.back().get_position()))
                return true;
        }
    }
    return false;
}

std::list<vec2> Turtle::get_path() {
    return m_path;
}

void Turtle::set_mode(bool mode) {
    m_mode2 = mode;
    motion.speed = m_base_speed;
    m_path.clear();
}

void Turtle::update_speed(Salmon &salmon) {

    float dist = len(sub(motion.position, salmon.get_position()));

    if (dist > 800)
        return;

    motion.speed = (pow(dist - 800, 2) / 1500.f) + m_base_speed;
}