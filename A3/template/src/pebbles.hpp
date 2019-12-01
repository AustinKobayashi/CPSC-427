#pragma once

#include <vector>

#include "common.hpp"
#include "turtle.hpp"
#include "fish.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Salmon pew-pews
class Pebbles : public Entity
{
public:
	// Data structure for pebble contains information needed
	// to render and simulate a basic pebble (apart from mesh.vbo), 
	// we will use this layout to pass information from m_pebbles to the pipeline.
	struct Pebble {
		float life = 0.0f; // remove pebble when its life reaches 0
		vec2 position;
		vec2 velocity;
		vec2 acceleration;
		float radius;
		bool can_collide_with_salmon;
        bool operator==(const Pebble& other) const {
            return life == other.life &&
                    position.x == other.position.x &&
                    position.y == other.position.y &&
                    velocity.x == other.velocity.x &&
                    velocity.y == other.velocity.y &&
                    acceleration.x == other.acceleration.x &&
                    acceleration.y == other.acceleration.y &&
                    radius == other.radius;
        }
        bool operator!=(const Pebble& other) const {
            return life != other.life ||
                   position.x != other.position.x ||
                   position.y != other.position.y ||
                   velocity.x != other.velocity.x ||
                   velocity.y != other.velocity.y ||
                   acceleration.x != other.acceleration.x ||
                   acceleration.y != other.acceleration.y ||
                   radius != other.radius;
        }
	};

	// Creates all the associated render resources
	bool init(vec2 level_bounds, float current_speed);

	// Releases all associated resources
	void destroy();

	// Updates all pebbles
	// ms represents the number of milliseconds elapsed from the previous update() call
	void update(float ms, Salmon& salmon);

	// Renders the pebbles
	// projection is the 2D orthographic projection matrix
	void draw(const mat3& projection) override;

	// Spawn new pebble
	void spawn_pebble(vec2 position, float salmon_rotation, float gravity);

	// Trigger collision checks
	void collides_with_pebble();
    void collides_with(Turtle& turtle);
    void collides_with(Fish& fish);
    void collides_with(Salmon& salmon);

    float sign (vec2 p1, vec2 p2, vec2 p3);
    bool is_inside (vec2 pt, vec2 v1, vec2 v2, vec2 v3);
    float dist (vec2 pt, vec2 p1, vec2 p2);

    void set_mode3 (bool mode3);

    void set_current_speed (float current_speed);

private:

    vec2 pebble_pebble_bounce(Pebble &pebble1, Pebble& pebble2);

    int m_min_radius;
    int m_max_radius;

    float m_current_speed;

    vec2 m_x_level_bounds;
    vec2 m_y_level_bounds;

    bool m_mode3;

	GLuint m_instance_vbo; // vbo for instancing pebbles
	std::vector<Pebble> m_pebbles; // vector of pebbles
};