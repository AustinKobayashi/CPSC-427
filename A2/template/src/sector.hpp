#ifndef SALMON_SECTOR_HPP
#define SALMON_SECTOR_HPP
#pragma once

#include <list>
#include "common.hpp"
#include "salmon.hpp"

// Salmon food
class Sector
{

public:
    // Creates all the associated render resources and default transform
    bool init(vec2 position, vec2 goal, float step, float move_cost);

    // Releases all the associated resources
    void destroy();

    float get_f(vec2 cur);
    float get_move_cost();

    bool valid_sector_for_fish (Salmon& salmon);
    bool valid_sector_for_turtle (std::vector<Fish>& fishes);

    vec2 get_position();

    bool is_goal();

private:
    vec2 m_position;
    vec2 m_goal;
    float m_step;
    float m_move_cost;
};
#endif //SALMON_SECTOR_HPP
