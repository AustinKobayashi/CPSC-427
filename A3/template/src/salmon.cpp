// Header
#include "salmon.hpp"

// internal
#include "turtle.hpp"
#include "fish.hpp"

// stlib
#include <string>
#include <algorithm>

#include <math.h>
#include <iostream>
#define PI 3.14159265

bool Salmon::init(vec2 x_level_bounds, vec2 y_level_bounds)
{
	m_vertices.clear();
  	m_indices.clear();

	// Reads the salmon mesh from a file, which contains a list of vertices and indices
	FILE* mesh_file = fopen(mesh_path("salmon.mesh"), "r");
	if (mesh_file == nullptr)
		return false;

	// Reading vertices and colors
	size_t num_vertices;
	fscanf(mesh_file, "%zu\n", &num_vertices);

	m_mouth_vec = {0, 0};
    m_left_vec = {0, 0};
    m_top_vec = {0, 0};
    m_bottom_vec = {0, 0};

	for (size_t i = 0; i < num_vertices; ++i)
	{
		float x, y, z;
		float _u[3]; // unused
		int r, g, b;
		fscanf(mesh_file, "%f %f %f %f %f %f %d %d %d\n", &x, &y, &z, _u, _u+1, _u+2, &r, &g, &b);
		Vertex vertex;
		vertex.position = { x, y, -z };
		vertex.color = { (float)r / 255, (float)g / 255, (float)b / 255 };
		m_vertices.push_back(vertex);

		if (x < m_mouth_vec.x)
		    m_mouth_vec = {x, y};
        if (x > m_left_vec.x)
            m_left_vec = {x, y};
        if (y < m_top_vec.y)
            m_top_vec = {x, y};
        if (y > m_bottom_vec.y)
            m_bottom_vec = {x, y};
	}

    // Reading associated indices
	size_t num_indices;
	fscanf(mesh_file, "%zu\n", &num_indices);
	for (size_t i = 0; i < num_indices; ++i)
	{
		int idx[3];
		fscanf(mesh_file, "%d %d %d\n", idx, idx + 1, idx + 2);
		m_indices.push_back((uint16_t)idx[0]);
		m_indices.push_back((uint16_t)idx[1]);
		m_indices.push_back((uint16_t)idx[2]);
	}

	// Done reading
	fclose(mesh_file);

	// Clearing errors
	gl_flush_errors();

	// Vertex Buffer creation
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

	// Index Buffer creation
	glGenBuffers(1, &mesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

	// Vertex Array (Container for Vertex + Index buffer)
	glGenVertexArrays(1, &mesh.vao);
	if (gl_has_errors())
		return false;

	// Loading shaders
	if (!effect.load_from_file(shader_path("salmon.vs.glsl"), shader_path("salmon.fs.glsl")))
		return false;
	
	// Setting initial values
	motion.position = { 300.f, 400.f };
	motion.radians = 0.f;
	motion.speed = 200.f;

	physics.scale = { -35.f, 35.f };

	m_is_alive = true;
	m_light_up_countdown_ms = -1.f;
	m_velocity = {0.f, 0.f};
	m_move_speed = 3.f;

    m_x_level_bounds = x_level_bounds;
    m_y_level_bounds = y_level_bounds;

    m_x_bounds = {-4.2, 4.2};
    m_y_bounds = {-3.8, 4.0};

    m_rotate_amount = 0.035;
    m_rotate = false;

    return true;
}

// Releases all graphics resources
void Salmon::destroy()
{
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ibo);
	glDeleteBuffers(1, &mesh.vao);

	glDeleteShader(effect.vertex);
	glDeleteShader(effect.fragment);
	glDeleteShader(effect.program);
}

// Called on each frame by World::update()
void Salmon::update(float ms)
{
	float step = motion.speed * (ms / 1000);
	if (m_is_alive)
	{
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// UPDATE SALMON POSITION HERE BASED ON KEY PRESSED (World::on_key())
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if (m_rotate) {
		    motion.radians += m_rotate_direction * m_rotate_amount;
		}

		move(mul(m_velocity, m_move_speed));
	}
	else
	{
		// If dead we make it face upwards and sink deep down
		set_rotation(3.1415f);
		move({ 0.f, step });
	}

	if (m_light_up_countdown_ms > 0.f)
		m_light_up_countdown_ms -= ms;
}

void Salmon::draw(const mat3& projection)
{
	transform.begin();

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// SALMON TRANSFORMATION CODE HERE

	// see Transformations and Rendering in the specification pdf
	// the following functions are available:
	transform.translate(motion.position);
	transform.rotate(motion.radians);
	transform.scale(physics.scale);
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	transform.end();

	// Setting shaders
	glUseProgram(effect.program);

	// Enabling alpha channel for textures
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	// Getting uniform locations
	GLint transform_uloc = glGetUniformLocation(effect.program, "transform");
	GLint color_uloc = glGetUniformLocation(effect.program, "fcolor");
	GLint projection_uloc = glGetUniformLocation(effect.program, "projection");
	GLint light_up_uloc = glGetUniformLocation(effect.program, "light_up");

	// Setting vertices and indices
	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

	// Input data location as in the vertex buffer
	GLint in_position_loc = glGetAttribLocation(effect.program, "in_position");
	GLint in_color_loc = glGetAttribLocation(effect.program, "in_color");
	glEnableVertexAttribArray(in_position_loc);
	glEnableVertexAttribArray(in_color_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(vec3));

	// Setting uniform values to the currently bound program
	glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.out);

	// !!! Salmon Color
	if (m_is_alive) {
		float color[] = { 1.f, 1.f, 1.f };
		glUniform3fv(color_uloc, 1, color);
	} else {
		float color[] = { 1.f, 0.5f, 0.5f };
		glUniform3fv(color_uloc, 1, color);
	}
	glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HERE TO SET THE CORRECTLY LIGHT UP THE SALMON IF HE HAS EATEN RECENTLY
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	int light_up = 0;
	if (m_light_up_countdown_ms > 0.f)
		light_up = 1;
	glUniform1iv(light_up_uloc, 1, &light_up);

	// Get number of infices from buffer,
	// we know our vbo contains both colour and position information, so...
	GLint size = 0;
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	GLsizei num_indices = size / sizeof(uint16_t);

	// Drawing!
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
}

// Simple bounding box collision check
// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You don't
// need to try to use this technique.
bool Salmon::collides_with(const Turtle& turtle)
{
	float dx = motion.position.x - turtle.get_position().x;
	float dy = motion.position.y - turtle.get_position().y;
	float d_sq = dx * dx + dy * dy;
	float other_r = std::max(turtle.get_bounding_box().x, turtle.get_bounding_box().y);
	float my_r = std::max(physics.scale.x, physics.scale.y);
	float r = std::max(other_r, my_r);
	r *= 0.6f;
	if (d_sq < r * r)
		return true;
	return false;
}

bool Salmon::collides_with(const Fish& fish)
{
	float dx = motion.position.x - fish.get_position().x;
	float dy = motion.position.y - fish.get_position().y;
	float d_sq = dx * dx + dy * dy;
	float other_r = std::max(fish.get_bounding_box().x, fish.get_bounding_box().y);
	float my_r = std::max(physics.scale.x, physics.scale.y);
	float r = std::max(other_r, my_r);
	r *= 0.6f;
	if (d_sq < r * r)
		return true;
	return false;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// HANDLE SALMON - WALL COLLISIONS HERE
// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
// You will want to write new functions from scratch for checking/handling
// salmon - wall collisions.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool Salmon::collides_with_wall() {
    if (bounding_box_collision())
        return mesh_collision();

    return false;
}

bool Salmon::bounding_box_collision() {
    return motion.position.x + m_x_bounds.x * abs(physics.scale.x) < m_x_level_bounds.x ||
           motion.position.x + m_x_bounds.y * abs(physics.scale.x) > m_x_level_bounds.y ||
           motion.position.y + m_y_bounds.x * abs(physics.scale.y) < m_y_level_bounds.x ||
           motion.position.y + m_y_bounds.y * abs(physics.scale.y) > m_y_level_bounds.y;
}

bool Salmon::mesh_collision() {
    m_collision_points.clear();

    mat3 trans = get_transformation_matrix();

    bool hit = false;
    int index = 0;
    for (auto vert : m_vertices) {
        vert.position.z = 1.f;
        vec3 trans_vert = mul(trans, vert.position);
        if (trans_vert.x < m_x_level_bounds.x ||
            trans_vert.x > m_x_level_bounds.y ||
            trans_vert.y < m_y_level_bounds.x ||
            trans_vert.y > m_y_level_bounds.y) {
                m_collision_points.push_front({vert.position.x, vert.position.y});
                hit = true;
        }
        index++;
    }
    return hit;
}

void Salmon::reverse_direction(vec2 velocity) {
    float min_dist = 1000000;
    bool x_axis_reflect = false; //0 - left wall, 1 - bottom wall, 2 - right wall, 3 - top wall

    // left wall
    if (len(sub(motion.position, vec2{m_x_level_bounds.x, motion.position.y})) < min_dist) {
        min_dist = len(sub(motion.position, vec2{m_x_level_bounds.x, motion.position.y}));
    }

    // bottom wall
    if (len(sub(motion.position, vec2{motion.position.x, m_y_level_bounds.y})) < min_dist) {
        x_axis_reflect = true;
        min_dist = len(sub(motion.position, vec2{motion.position.x, m_y_level_bounds.y}));
    }

    // right wall
    if (len(sub(motion.position, vec2{m_x_level_bounds.y, motion.position.y})) < min_dist) {
        x_axis_reflect = false;
        min_dist = len(sub(motion.position, vec2{m_x_level_bounds.y, motion.position.y}));
    }

    // top wall
    if (len(sub(motion.position, vec2{motion.position.x, m_y_level_bounds.x})) < min_dist)
        x_axis_reflect = true;


    float x = cos(motion.radians) * velocity.x - sin(motion.radians) * velocity.y;
    float y = sin(motion.radians) * velocity.x + cos(motion.radians) * velocity.y;

    if (x_axis_reflect)
        y *= -1.f;
    else
        x *= -1.f;

    float theta = acos (dot({x, y}, velocity) / (len({x, y}) * len(velocity))) ;

    if (abs(theta - motion.radians) < 0.00001)
        theta *= -1.f;

    if(!x_axis_reflect) {
        if (velocity.x != 0 && (y / abs(y) != velocity.x / abs(velocity.x))) {
            theta *= -1.f;
        }
    }

    if (!isnan(theta))
        motion.radians = theta;

    mat3 trans = get_transformation_matrix();

    float min_x = 10000;
    float max_x = 0;
    float min_y = 10000;
    float max_y = 0;

    for (auto vert : m_vertices) {
        vert.position.z = 1.f;
        vec3 trans_vert = mul(trans, vert.position);
        if (trans_vert.x < min_x)
            min_x = trans_vert.x;
        if (trans_vert.x > max_x)
            max_x = trans_vert.x;
        if (trans_vert.y < min_y)
            min_y = trans_vert.y;
        if (trans_vert.y > max_y)
            max_y = trans_vert.y;
    }

    if (min_x < m_x_level_bounds.x)
        motion.position.x += (m_x_level_bounds.x - min_x) + 1;
    if (max_x > m_x_level_bounds.y)
        motion.position.x += (m_x_level_bounds.y - max_x) - 1;
    if (min_y < m_y_level_bounds.x)
        motion.position.y += (m_y_level_bounds.x - min_y) + 1;
    if (max_y > m_y_level_bounds.y)
        motion.position.y += (m_y_level_bounds.y - max_y) - 1;
}


vec2 Salmon::get_position() const
{
	return motion.position;
}

void Salmon::set_velocity(vec2 velocity) {
	m_velocity = velocity;
}

void Salmon::move(vec2 off)
{
	float x = off.x;
	float y = off.y;
	if (m_is_alive) {
		x = cos(motion.radians) * off.x - sin(motion.radians) * off.y;
		y = sin(motion.radians) * off.x + cos(motion.radians) * off.y;
	}
	motion.position.x += x; 
	motion.position.y += y; 
}

void Salmon::set_rotation(float radians)
{
	motion.radians = radians;
}

bool Salmon::is_alive() const
{
	return m_is_alive;
}

// Called when the salmon collides with a turtle
void Salmon::kill()
{
	m_is_alive = false;
}

// Called when the salmon collides with a fish
void Salmon::light_up()
{
	m_light_up_countdown_ms = 1500.f;
}

void Salmon::set_mode(bool mode) {
	m_mode = mode;
}

vec2 Salmon::get_x_bounds() {
    return m_x_bounds;
}

vec2 Salmon::get_y_bounds() {
    return m_y_bounds;
}

vec2 Salmon::get_scale() {
    return physics.scale;
}

std::list<vec2> Salmon::get_collision_points (){
    return m_collision_points;
}

mat3 Salmon::get_transformation_matrix() {
    transform.begin();
    transform.translate(motion.position);
    transform.rotate(motion.radians);
    transform.scale(physics.scale);
    transform.end();
    return transform.out;
}

vec2 Salmon::get_velocity() {
    return m_velocity;
}

float Salmon::get_rotation() {
    return motion.radians;
}

void Salmon::start_rotating(int direction) {
    m_rotate_direction = direction;
    m_rotate = true;
}


void Salmon::stop_rotating() {
    m_rotate = false;
}

vec2 Salmon::get_mouth_pos() {
    vec2 mouth_vec = {m_mouth_vec.x * physics.scale.x, m_mouth_vec.y * physics.scale.y};
    float x = motion.position.x + cos(motion.radians) * mouth_vec.x - sin(motion.radians) * mouth_vec.y;
    float y = motion.position.y + sin(motion.radians) * mouth_vec.x + cos(motion.radians) * mouth_vec.y;
    return {x, y};
}

void Salmon::calculate_corners() {

    float x, y;
    vec2 top_left = {m_left_vec.x * physics.scale.x, m_top_vec.y * physics.scale.y};
    x = motion.position.x + cos(motion.radians) * top_left.x - sin(motion.radians) * top_left.y;
    y = motion.position.y + sin(motion.radians) * top_left.x + cos(motion.radians) * top_left.y;
    top_left = {x, y};

    vec2 top_right = {m_mouth_vec.x * physics.scale.x, m_top_vec.y * physics.scale.y};
    x = motion.position.x + cos(motion.radians) * top_right.x - sin(motion.radians) * top_right.y;
    y = motion.position.y + sin(motion.radians) * top_right.x + cos(motion.radians) * top_right.y;
    top_right = {x, y};

    vec2 bottom_left = {m_left_vec.x * physics.scale.x, m_bottom_vec.y * physics.scale.y};
    x = motion.position.x + cos(motion.radians) * bottom_left.x - sin(motion.radians) * bottom_left.y;
    y = motion.position.y + sin(motion.radians) * bottom_left.x + cos(motion.radians) * bottom_left.y;
    bottom_left = {x, y};

    vec2 bottom_right = {m_mouth_vec.x * physics.scale.x, m_bottom_vec.y * physics.scale.y};
    x = motion.position.x + cos(motion.radians) * bottom_right.x - sin(motion.radians) * bottom_right.y;
    y = motion.position.y + sin(motion.radians) * bottom_right.x + cos(motion.radians) * bottom_right.y;
    bottom_right = {x, y};


    if (top_left.x < motion.position.x) {
        if ((top_left.y < top_right.y && top_right.x < motion.position.x) ||
                (top_left.y < bottom_left.y && bottom_left.x < motion.position.x) ||
                (top_left.y < bottom_right.y && bottom_right.x < motion.position.x))
            m_top_left = top_left;
        else
            m_bottom_left = top_left;
    } else {
        if ((top_left.y < top_right.y && top_right.x > motion.position.x) ||
                (top_left.y < bottom_left.y && bottom_left.x > motion.position.x) ||
                (top_left.y < bottom_right.y && bottom_right.x > motion.position.x))
            m_top_right = top_left;
        else
            m_bottom_right = top_left;
    }

    if (top_right.x < motion.position.x) {
        if ((top_right.y < top_left.y && top_left.x < motion.position.x) ||
            (top_right.y < bottom_left.y && bottom_left.x < motion.position.x) ||
            (top_right.y < bottom_right.y && bottom_right.x < motion.position.x))
            m_top_left = top_right;
        else
            m_bottom_left = top_right;
    } else {
        if ((top_right.y < top_left.y && top_left.x > motion.position.x) ||
            (top_right.y < bottom_left.y && bottom_left.x > motion.position.x) ||
            (top_right.y < bottom_right.y && bottom_right.x > motion.position.x))
            m_top_right = top_right;
        else
            m_bottom_right = top_right;
    }


    if (bottom_left.x < motion.position.x) {
        if ((bottom_left.y < top_left.y && top_left.x < motion.position.x) ||
            (bottom_left.y < top_right.y && top_right.x < motion.position.x) ||
            (bottom_left.y < bottom_right.y && bottom_right.x < motion.position.x))
            m_top_left = bottom_left;
        else
            m_bottom_left = bottom_left;
    } else {
        if ((bottom_left.y < top_left.y && top_left.x > motion.position.x) ||
            (bottom_left.y < top_right.y && top_right.x > motion.position.x) ||
            (bottom_left.y < bottom_right.y && bottom_right.x > motion.position.x))
            m_top_right = bottom_left;
        else
            m_bottom_right = bottom_left;
    }


    if (bottom_right.x < motion.position.x) {
        if ((bottom_right.y < top_left.y && top_left.x < motion.position.x) ||
            (bottom_right.y < top_right.y && top_right.x < motion.position.x) ||
            (bottom_right.y < bottom_left.y && bottom_left.x < motion.position.x))
            m_top_left = bottom_right;
        else
            m_bottom_left = bottom_right;
    } else {
        if ((bottom_right.y < top_left.y && top_left.x > motion.position.x) ||
            (bottom_right.y < top_right.y && top_right.x > motion.position.x) ||
            (bottom_right.y < bottom_left.y && bottom_left.x > motion.position.x))
            m_top_right = bottom_right;
        else
            m_bottom_right = bottom_right;
    }
}

vec2 Salmon::get_top_left_corner() { return m_top_left; }

vec2 Salmon::get_top_right_corner() { return m_top_right; }

vec2 Salmon::get_bottom_left_corner() { return m_bottom_left; }

vec2 Salmon::get_bottom_right_corner() { return m_bottom_right; }