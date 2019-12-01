
#include "sector.hpp"
#include <math.h>

bool Sector::init(vec2 position, vec2 goal, float step, float move_cost) {
    m_position = position;
    m_goal = goal;
    m_step = step;
    m_move_cost = move_cost;
    return true;
}

void Sector::destroy(){}

float Sector::get_f(vec2 cur) {
    vec2 goal_delta = sub(m_position, m_goal);
    vec2 pos_delta = sub(m_position, cur);

    return len(pos_delta) + len(goal_delta);
}

float Sector::get_move_cost() {
    return m_move_cost;
}

bool Sector::valid_sector_for_fish(Salmon &salmon) {
    if (m_position.x < -200 || m_position.x > 1400 ||
            m_position.y < -75|| m_position.y > 875)
        return false;

    float padding = 0.25;

    vec2 salmon_pos = salmon.get_position();
    vec2 salmon_scale = salmon.get_scale();
    vec2 salmon_x_bounds = {salmon.get_x_bounds().x - padding, salmon.get_x_bounds().y + padding};
    vec2 salmon_y_bounds = {salmon.get_y_bounds().x - padding, salmon.get_y_bounds().y + padding};

    return !(m_position.x > salmon_pos.x + salmon_x_bounds.x * abs(salmon_scale.x) &&
             m_position.x < salmon_pos.x + salmon_x_bounds.y * abs(salmon_scale.x) &&
             m_position.y > salmon_pos.y + salmon_y_bounds.x * abs(salmon_scale.y) &&
             m_position.y < salmon_pos.y + salmon_y_bounds.y * abs(salmon_scale.y));
}

bool Sector::valid_sector_for_turtle (std::vector<Fish>& fishes) {
    if (m_position.x < -200 || m_position.x > 1400 ||
        m_position.y < -100 || m_position.y > 900)
        return false;

    return true;
}

vec2 Sector::get_position() {
    return m_position;
}

bool Sector::is_goal() {
    return len(sub(m_goal, m_position)) <= m_step;
}



