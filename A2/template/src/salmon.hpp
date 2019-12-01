#pragma once

#include "common.hpp"
#include <vector>
#include <list>

class Turtle;
class Fish;

class Salmon : public Entity
{
public:
	// Creates all the associated render resources and default transform
	bool init(vec2 x_level_bounds, vec2 y_level_bounds);

	// Releases all associated resources
	void destroy();
	
	// Update salmon position based on direction
	// ms represents the number of milliseconds elapsed from the previous update() call
	void update(float ms);
	
	// Renders the salmon
	void draw(const mat3& projection)override;

	// Collision routines for turtles and fish
	bool collides_with(const Turtle& turtle);
	bool collides_with(const Fish& fish);
    bool collides_with_wall();

	// Returns the current salmon position
	vec2 get_position() const;
	
	// Set the velocity of the salmon
	void set_velocity(vec2 velocity);
	
	// Moves the salmon's position by the specified offset
	void move(vec2 off);

	// Set salmon rotation in radians
	void set_rotation(float radians);

	// True if the salmon is alive
	bool is_alive()const;

	// Kills the salmon, changing its alive state and triggering on death events
	void kill();

	// Called when the salmon collides with a fish, starts lighting up the salmon
	void light_up();
	
	void set_mode (bool mode);

	void reverse_direction(vec2 velocity);

    vec2 get_x_bounds();
    vec2 get_y_bounds();
    vec2 get_scale();
    vec2 get_velocity();
    float get_rotation();

    void start_rotating(int direction);
    void stop_rotating();

    mat3 get_transformation_matrix();

    std::list<vec2> get_collision_points();

private:
	float m_light_up_countdown_ms; // Used to keep track for how long the salmon should be lit up
	bool m_is_alive; // True if the salmon is alive

	bool bounding_box_collision();
    bool mesh_collision();

	vec2 m_velocity;
	vec2 m_x_level_bounds;
    vec2 m_y_level_bounds;

	vec2 m_x_bounds;
	vec2 m_y_bounds;

	float m_move_speed;

	bool m_mode;

    bool m_rotate;
    int m_rotate_direction;
    float m_rotate_amount;

  	std::vector<Vertex> m_vertices;
	std::vector<uint16_t> m_indices;
	std::list<vec2> m_collision_points;
};
