#pragma once

#include "common.hpp"
#include <list>

class DebugPath : public Entity
{
public:
	// Creates all the associated render resources and default transform
	bool init(vec2 screen_size);

	// Releases all associated resources
	void destroy();

	// Renders the water
	void draw(const mat3& projection)override;

    void clear_paths();

    void add_to_path(std::list<vec2> path);

private:
	vec2 m_screen_size;
	std::list<float> m_paths;
};
