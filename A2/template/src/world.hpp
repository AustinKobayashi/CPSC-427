#pragma once

// internal
#include "common.hpp"
#include "salmon.hpp"
#include "turtle.hpp"
#include "fish.hpp"
#include "water.hpp"
#include "pebbles.hpp"
#include "debug_path.hpp"
#include "debug_boundaries.hpp"
#include "debug_collider.hpp"
#include "debug_collision.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

// Container for all our entities and game logic. Individual rendering / update is 
// deferred to the relative update() methods
class World
{
public:
	World();
	~World();

	// Creates a window, sets up events and begins the game
	bool init(vec2 screen);

	// Releases all associated resources
	void destroy();

	// Steps the game ahead by ms milliseconds
	bool update(float ms);

	// Renders our scene
	void draw();

	// Should the game be over ?
	bool is_over()const;

private:
	// Generates a new turtle
	bool spawn_turtle();

	// Generates a new fish
	bool spawn_fish();

	// !!! INPUT CALLBACK FUNCTIONS
	void on_key(GLFWwindow*, int key, int, int action, int mod);
	void on_mouse_move(GLFWwindow* window, double xpos, double ypos);

private:

    void reset_world();

	// Window handle
	GLFWwindow* m_window;
	float m_screen_scale; // Screen to pixel coordinates scale factor

	// Screen texture
	// The draw loop first renders to this texture, then it is used for the water shader
	GLuint m_frame_buffer;
	Texture m_screen_tex;

	// Water effect
	Water m_water;

	DebugPath m_debug_path;
    DebugBoundaries m_debug_boundaries;
    DebugCollider m_debug_collider;
    DebugCollision m_debug_collision;

	// Number of fish eaten by the salmon, displayed in the window title
	unsigned int m_points;

	// Game entities
	Salmon m_salmon;
	std::vector<Turtle> m_turtles;
	std::vector<Fish> m_fish;
	Pebbles m_pebbles_emitter;

	float m_current_speed;
	float m_next_turtle_spawn;
	float m_next_fish_spawn;
	
	float m_move_timer;
	vec2 m_salmon_velocity;

    vec2 m_level_bounds;
    float m_level_bounds_padding;

    float m_freeze_time;
    float m_elapsed_freeze_time;
    vec2 m_collision_velocity;

    int m_frame_skip;
    int m_frame_count = 0;

    bool m_mode1; // m & n
    bool m_mode2; // k & l

	bool m_key_up;
	bool m_freeze;
    bool m_debugging;

	Mix_Music* m_background_music;
	Mix_Chunk* m_salmon_dead_sound;
	Mix_Chunk* m_salmon_eat_sound;

	// C++ rng
	std::default_random_engine m_rng;
	std::uniform_real_distribution<float> m_dist; // default 0..1
};
