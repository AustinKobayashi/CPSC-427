#pragma once

#include "common.hpp"
#include "salmon.hpp"
#include <list>
#include <map>

typedef std::pair<float, float> pos;

class DebugCollision : public Entity
{
public:
    // Creates all the associated render resources and default transform
    bool init(Salmon& salmon);

    void update(float ms);

    // Releases all associated resources
    void destroy();

    // Renders the water
    void draw(const mat3& projection) override;

    void add_collision_point(vec2 point);


private:

    Salmon* m_salmon;

    std::map<pos, float> m_points;

    float m_point_time;
};
