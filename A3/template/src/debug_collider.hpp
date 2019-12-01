#pragma once

#include "common.hpp"
#include <list>

class DebugCollider : public Entity
{
public:
    // Creates all the associated render resources and default transform
    bool init(vec2 x_bounds, vec2 y_bounds, vec2 window_size, vec2 salmon_scale);

    // Releases all associated resources
    void destroy();

    // Renders the water
    void draw(const mat3& projection)override;

    void set_salmon_position(vec2 salmon_position);

private:

    vec2 m_x_bounds;
    vec2 m_y_bounds;
    vec2 m_level_size;
    vec2 m_salmon_scale;
    vec2 m_salmon_position;
};
