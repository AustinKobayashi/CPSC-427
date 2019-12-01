#pragma once

#include <list>
#include "common.hpp"
#include "salmon.hpp"
#include "sector.hpp"

// Salmon food
class Fish : public Entity
{
	// Shared between all fish, no need to load one for each instance
	static Texture fish_texture;

public:
	// Creates all the associated render resources and default transform
	bool init(bool m_mode3);

	// Releases all the associated resources
	void destroy();
	
	// Update fish
	// ms represents the number of milliseconds elapsed from the previous update() call
	void update(float ms);

	// Renders the fish
	// projection is the 2D orthographic projection matrix
	void draw(const mat3& projection) override;

	// Returns the current fish position
	vec2 get_position() const;

	// Sets the new fish position
	void set_position(vec2 position);

	// Returns the fish' bounding box for collision detection, called by collides_with()
	vec2 get_bounding_box() const;

	void calculate_path(Salmon& m_salmon);

    std::list<vec2> get_path();

    bool load_texture();
    bool default_texture();
    bool reskin();

    void slow_down();

private:
    bool value_in_list (std::list<Sector> open_list, std::list<Sector> closed_list, Sector value);

    float m_base_speed;
    float m_slow_speed;
    float m_speed_timer;
    float m_speed_reset;
    bool m_slowed;

    vec2 m_reskin_scale;
    vec2 m_default_scale;

    std::list<vec2> m_path;
};