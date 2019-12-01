#pragma once

#include "common.hpp"
#include <list>

class DebugBoundaries : public Entity
{
public:
    // Creates all the associated render resources and default transform
    bool init(vec2 x_level_bounds, vec2 y_level_bounds, vec2 window_size);

    // Releases all associated resources
    void destroy();

    // Renders the water
    void draw(const mat3& projection)override;

private:

    vec2 m_x_level_bounds;
    vec2 m_y_level_bounds;
};
