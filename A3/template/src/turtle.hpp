#pragma once

#include "common.hpp"
#include "salmon.hpp"
#include "sector.hpp"

// Salmon enemy 
class Turtle : public Entity
{
	// Shared between all turtles, no need to load one for each instance
	static Texture turtle_texture;

public:
	// Creates all the associated render resources and default transform
	bool init(bool m_mode3);

	// Releases all the associated resources
	void destroy();

	// Update turtle due to current
	// ms represents the number of milliseconds elapsed from the previous update() call
	void update(float ms);

	// Renders the salmon
	// projection is the 2D orthographic projection matrix
	void draw(const mat3& projection) override;

	// Returns the current turtle position
	vec2 get_position()const;

	// Sets the new turtle position
	void set_position(vec2 position);

	// Returns the turtle' bounding box for collision detection, called by collides_with()
	vec2 get_bounding_box() const;

    void calculate_path(Salmon& m_salmon, std::vector<Fish>& fishes);

    std::list<vec2> get_path();

    void set_mode(bool mode);

    void update_speed(Salmon& salmon);

    bool load_texture();
    bool default_texture();
    bool reskin();

    void turn_around();

private:
    bool value_in_list (std::list<Sector> open_list, std::list<Sector> closed_list, Sector value);

    std::list<vec2> m_path;

    bool m_mode2;

    vec2 m_reskin_scale;
    vec2 m_default_scale;

    float m_base_speed;
};