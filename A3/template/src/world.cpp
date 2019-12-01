// Header
#include "world.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>

#include <iostream>

// Same as static in c, local to compilation unit
namespace
{
	const size_t MAX_TURTLES = 15;
	const size_t MAX_FISH = 5;
	size_t TURTLE_DELAY_MS = 3000;
	const size_t FISH_DELAY_MS = 2000;

	namespace
	{
		void glfw_err_cb(int error, const char* desc)
		{
			fprintf(stderr, "%d: %s", error, desc);
		}
	}
}

World::World() : 
m_points(0),
m_next_turtle_spawn(0.f),
m_next_fish_spawn(0.f)
{
	// Seeding rng with random device
	m_rng = std::default_random_engine(std::random_device()());
}

World::~World()
{

}

// World initialization
bool World::init(vec2 screen)
{
	//-------------------------------------------------------------------------
	// GLFW / OGL Initialization
	// Core Opengl 3.
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);
	m_window = glfwCreateWindow((int)screen.x, (int)screen.y, "Salmon Game Assignment", nullptr, nullptr);
	if (m_window == nullptr)
		return false;

	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(1); // vsync

	// Load OpenGL function pointers
	gl3w_init();

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(m_window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((World*)glfwGetWindowUserPointer(wnd))->on_key(wnd, _0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((World*)glfwGetWindowUserPointer(wnd))->on_mouse_move(wnd, _0, _1); };
    auto mouse_button_redirect = [](GLFWwindow* wnd,int _0, int _1, int _2) { ((World*)glfwGetWindowUserPointer(wnd))->on_mouse_click(wnd, _0, _1, _2); };
    glfwSetKeyCallback(m_window, key_redirect);
	glfwSetCursorPosCallback(m_window, cursor_pos_redirect);
    glfwSetMouseButtonCallback(m_window, mouse_button_redirect);

	// Create a frame buffer
	m_frame_buffer = 0;
	glGenFramebuffers(1, &m_frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer);

	// For some high DPI displays (ex. Retina Display on Macbooks)
	// https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value
	int fb_width, fb_height;
	glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
	m_screen_scale = static_cast<float>(fb_width) / screen.x;

	// Initialize the screen texture
	m_screen_tex.create_from_screen(m_window);

	//-------------------------------------------------------------------------
	// Loading music and sounds
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "Failed to initialize SDL Audio");
		return false;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
	{
		fprintf(stderr, "Failed to open audio device");
		return false;
	}

    if(!load_default_sounds())
        return false;

	m_current_speed = 0.25f;

	m_salmon_velocity = {0.f, 0.f};
    m_level_bounds = screen;
	m_key_up = true;
    m_frame_skip = 40;

    m_freeze = false;
    m_freeze_time = 500;
    m_elapsed_freeze_time = 0;

    m_debugging = false;

    m_mode1 = false;
    m_mode2 = false;
    m_mode3 = false;

	m_level_bounds_padding = 50;

    m_pebble_group_frequency = 500;
    m_pebble_group_timer = 0;
    m_pebbles_spawned = 0;
    m_pebble_timer = 0;
    m_pebble_frequency = 200;
    m_spawning_pebbles = false;

	m_num_pebbles = {3, 5};
    srand (time(NULL));

    m_gravity = 2;

    m_shoot_pebble_timer = 0;
    m_shoot_pebble_frequency = 1000;
    m_can_shoot = true;

    m_base_turtle_delay = TURTLE_DELAY_MS;
    m_mode3_turtle_delay = (int) (0.5 * m_base_turtle_delay);

    return m_salmon.init({m_level_bounds_padding, m_level_bounds.x - m_level_bounds_padding},
            {m_level_bounds_padding , m_level_bounds.y - m_level_bounds_padding}) &&
           m_water.init() &&
           m_pebbles_emitter.init(m_level_bounds, m_current_speed) &&
           m_debug_path.init(screen) &&
           m_debug_boundaries.init({m_level_bounds_padding, m_level_bounds.x - m_level_bounds_padding},
                                 {m_level_bounds_padding , m_level_bounds.y - m_level_bounds_padding}, m_level_bounds) &&
           m_debug_collider.init(m_salmon.get_x_bounds(), m_salmon.get_y_bounds(), m_level_bounds, m_salmon.get_scale()) &&
           m_debug_collision.init(m_salmon);
}

// Releases all the associated resources
void World::destroy()
{
	glDeleteFramebuffers(1, &m_frame_buffer);

	if (m_background_music != nullptr)
		Mix_FreeMusic(m_background_music);
	if (m_salmon_dead_sound != nullptr)
		Mix_FreeChunk(m_salmon_dead_sound);
	if (m_salmon_eat_sound != nullptr)
		Mix_FreeChunk(m_salmon_eat_sound);

	Mix_CloseAudio();

	m_salmon.destroy();
	m_pebbles_emitter.destroy();
	for (auto& turtle : m_turtles)
		turtle.destroy();
	for (auto& fish : m_fish)
		fish.destroy();
	m_turtles.clear();
	m_fish.clear();
	glfwDestroyWindow(m_window);
}

// Update our game world
bool World::update(float elapsed_ms) {

    if (m_mode3 && !m_can_shoot) {
        m_shoot_pebble_timer += elapsed_ms;

        if (m_shoot_pebble_timer > m_shoot_pebble_frequency) {
            m_can_shoot = true;
            m_shoot_pebble_timer = 0;
        }
    }

    if (m_debugging)
        m_debug_collision.update(elapsed_ms);

    if (m_freeze && m_debugging && m_salmon.is_alive()) {
        m_elapsed_freeze_time += elapsed_ms;

        if (m_elapsed_freeze_time > m_freeze_time) {
            m_salmon.reverse_direction(m_collision_velocity);
            m_elapsed_freeze_time = 0;
            m_freeze = false;

            if (m_mode1)
                m_salmon.set_velocity(m_collision_velocity);
        }
    }

    if (!m_freeze || !m_debugging) {
        m_frame_count++;

        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        vec2 screen = {(float) w / m_screen_scale, (float) h / m_screen_scale};

        // Checking Salmon - Turtle collisions
        for (const auto &turtle : m_turtles) {
            if (m_salmon.collides_with(turtle)) {
                if (m_salmon.is_alive()) {
                    Mix_PlayChannel(-1, m_salmon_dead_sound, 0);
                    m_water.set_salmon_dead();
                }
                m_salmon.kill();
                break;
            }
        }

        // Checking Salmon - Fish collisions
        auto fish_it = m_fish.begin();
        while (fish_it != m_fish.end()) {
            if (m_salmon.is_alive() && m_salmon.collides_with(*fish_it)) {
                fish_it = m_fish.erase(fish_it);
                m_salmon.light_up();
                Mix_PlayChannel(-1, m_salmon_eat_sound, 0);
                ++m_points;
            } else
                ++fish_it;
        }

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // SALMON MOMENTUM
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (m_mode1 && m_key_up) {
            m_move_timer -= 0.02f;
            if (m_move_timer < 0.f)
                m_move_timer = 0.f;

            m_salmon.set_velocity(mul(m_salmon_velocity, m_move_timer));
        }


        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // HANDLE SALMON - WALL COLLISIONS HERE
        // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2d
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (m_salmon.is_alive() && m_salmon.collides_with_wall()) {
            if (m_debugging && m_salmon.is_alive()) {
                std::list<vec2> collision_points = m_salmon.get_collision_points();
                for (auto collision_point : collision_points)
                    m_debug_collision.add_collision_point(collision_point);
                m_freeze = true;
                m_elapsed_freeze_time = 0;
                m_collision_velocity = m_salmon.get_velocity();
                m_salmon.set_velocity({0, 0});
            } else {
                m_salmon.reverse_direction(m_salmon.get_velocity());
            }
        }


        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // HANDLE PEBBLE COLLISIONS HERE
        // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        m_pebbles_emitter.collides_with_pebble();
        for (auto &turtle : m_turtles) {
            m_pebbles_emitter.collides_with(turtle);
        }
        for (auto &fish : m_fish) {
            m_pebbles_emitter.collides_with(fish);
        }
        m_pebbles_emitter.collides_with(m_salmon);

        // Updating all entities, making the turtle and fish
        // faster based on current.
        // In a pure ECS engine we would classify entities by their bitmap tags during the update loop
        // rather than by their class.
        m_salmon.update(elapsed_ms);
        m_debug_collider.set_salmon_position(m_salmon.get_position());

        if (m_mode2 && !m_turtles.empty() && m_salmon.is_alive())
            m_turtles[0].update_speed(m_salmon);

        for (auto &turtle : m_turtles)
            turtle.update(elapsed_ms * m_current_speed);
        for (auto &fish : m_fish)
            fish.update(elapsed_ms * m_current_speed);

        m_pebbles_emitter.update(elapsed_ms, m_salmon);


        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // HANDLE PEBBLE SPAWN/UPDATES HERE
        // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (!m_mode3) {
            if (!m_spawning_pebbles) {
                m_pebble_group_timer += elapsed_ms;
            }

            if (m_pebble_group_timer > m_pebble_group_frequency && m_salmon.is_alive()) {
                m_pebbles_to_spawn = rand() % (int) m_num_pebbles.y + m_num_pebbles.x;
                m_spawning_pebbles = true;
                m_pebble_group_timer = 0;
            }

            if (m_spawning_pebbles) {
                m_pebble_timer += elapsed_ms;

                if (m_pebble_timer > m_pebble_frequency) {
                    m_pebbles_emitter.spawn_pebble(m_salmon.get_mouth_pos(), m_salmon.get_rotation(), m_gravity);
                    m_pebbles_spawned++;
                    m_pebble_timer = 0;
                }

                if (m_pebbles_spawned >= m_pebbles_to_spawn) {
                    m_spawning_pebbles = false;
                    m_pebbles_spawned = 0;
                }
            }
        }

        // Removing out of screen turtles
        if (!m_mode2) {
            auto turtle_it = m_turtles.begin();
            while (turtle_it != m_turtles.end()) {
                float w = turtle_it->get_bounding_box().x / 2;
                if (turtle_it->get_position().x + w < 0.f || turtle_it->get_position().x - 200 > m_level_bounds.x) {
                    turtle_it = m_turtles.erase(turtle_it);
                    continue;
                }

                ++turtle_it;
            }
        }

        // Removing out of screen fish
        fish_it = m_fish.begin();
        while (fish_it != m_fish.end()) {
            float w = fish_it->get_bounding_box().x / 2;
            if (fish_it->get_position().x + w < 0.f) {
                fish_it = m_fish.erase(fish_it);
                continue;
            }

            ++fish_it;
        }

        // Spawning new turtles
        if (!m_mode2) {
            m_next_turtle_spawn -= elapsed_ms * m_current_speed;
            if (m_turtles.size() <= MAX_TURTLES && m_next_turtle_spawn < 0.f) {
                if (!spawn_turtle())
                    return false;

                Turtle &new_turtle = m_turtles.back();

                // Setting random initial position
                new_turtle.set_position({screen.x + 150, 50 + m_dist(m_rng) * (screen.y - 100)});

                // Next spawn
                m_next_turtle_spawn = (TURTLE_DELAY_MS / 2) + m_dist(m_rng) * (TURTLE_DELAY_MS / 2);
            }
        }

        // Spawning new fish
        m_next_fish_spawn -= elapsed_ms * m_current_speed;
        if (m_fish.size() <= MAX_FISH && m_next_fish_spawn < 0.f) {
            if (!spawn_fish())
                return false;
            Fish &new_fish = m_fish.back();
            new_fish.set_position({screen.x + 150, 50 + m_dist(m_rng) * (screen.y - 100)});

            if (m_salmon.is_alive()) {
                new_fish.calculate_path(m_salmon);
                m_debug_path.add_to_path(new_fish.get_path());
            }

            m_next_fish_spawn = (FISH_DELAY_MS / 2) + m_dist(m_rng) * (FISH_DELAY_MS / 2);
        }

        if (m_frame_count > m_frame_skip && m_salmon.is_alive()) {
            m_debug_path.clear_paths();

            for (auto &fish : m_fish) {
                fish.calculate_path(m_salmon);
                m_debug_path.add_to_path(fish.get_path());
            }

            if (m_mode2 && !m_turtles.empty()) {
                m_turtles[0].calculate_path(m_salmon, m_fish);
                m_debug_path.add_to_path(m_turtles[0].get_path());
            }

            m_frame_count = 0;
        }

        // If salmon is dead, restart the game after the fading animation
        if (!m_salmon.is_alive() &&
            m_water.get_salmon_dead_time() > 5) {

            reset_world();
        }
    }
	return true;
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void World::draw()
{
	// Clearing error buffer
	gl_flush_errors();

	// Getting size of window
	int w, h;
	glfwGetFramebufferSize(m_window, &w, &h);

	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << m_points;
	glfwSetWindowTitle(m_window, title_ss.str().c_str());

	/////////////////////////////////////
	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer);

	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);
	const float clear_color[3] = { 0.3f, 0.3f, 0.8f };
	glClearColor(clear_color[0], clear_color[1], clear_color[2], 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Fake projection matrix, scales with respect to window coordinates
	// PS: 1.f / w in [1][1] is correct.. do you know why ? (:
	float left = 0.f;// *-0.5;
	float top = 0.f;// (float)h * -0.5;
	float right = (float)w / m_screen_scale;// *0.5;
	float bottom = (float)h / m_screen_scale;// *0.5;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	mat3 projection_2D{ { sx, 0.f, 0.f },{ 0.f, sy, 0.f },{ tx, ty, 1.f } };

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// DRAW DEBUG INFO HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will want to create your own data structures for passing in 
	// relevant information to your debug draw call.
	// The shaders coloured.vs.glsl and coloured.fs.glsl should be helpful.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Drawing entities
	for (auto& turtle : m_turtles)
		turtle.draw(projection_2D);
	for (auto& fish : m_fish)
		fish.draw(projection_2D);
    m_pebbles_emitter.draw(projection_2D);
    m_salmon.draw(projection_2D);

	if (m_debugging) {
        m_debug_boundaries.draw(projection_2D);
        m_debug_collider.draw(projection_2D);
        m_debug_path.draw(projection_2D);
        m_debug_collision.draw(projection_2D);
    }

	/////////////////////
	// Truely render to the screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(0, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_screen_tex.id);

    m_water.draw(projection_2D);

	//////////////////
	// Presenting
	glfwSwapBuffers(m_window);
}

// Should the game be over ?
bool World::is_over() const
{
	return glfwWindowShouldClose(m_window);
}

// Creates a new turtle and if successfull adds it to the list of turtles
bool World::spawn_turtle()
{
	Turtle turtle;
	if (turtle.init(m_mode3))
	{
		m_turtles.emplace_back(turtle);
		return true;
	}
	fprintf(stderr, "Failed to spawn turtle");
	return false;
}

// Creates a new fish and if successfull adds it to the list of fish
bool World::spawn_fish()
{
	Fish fish;
	if (fish.init(m_mode3))
	{
		m_fish.emplace_back(fish);
		return true;
	}
	fprintf(stderr, "Failed to spawn fish");
	return false;
}

// On key callback
void World::on_key(GLFWwindow*, int key, int, int action, int mod)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE SALMON MOVEMENT HERE
	// key is of 'type' GLFW_KEY_
	// action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if (!m_mode1) {
		//m_salmon.set_velocity({0.f, 0.f});

		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_UP || key == GLFW_KEY_W)) {
            m_salmon.set_velocity({1.f, 0.f});
        }
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)) {
			m_salmon.set_velocity({-1.f, 0.f});
		}
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)) {
            m_salmon.start_rotating(1);
		}
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)) {
            m_salmon.start_rotating(-1);
        }

		if ((action == GLFW_RELEASE) &&
		    ((key == GLFW_KEY_UP || key == GLFW_KEY_W) || (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)))
            m_salmon.set_velocity({0.f, 0.f});

	} else {
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_UP || key == GLFW_KEY_W)) {
			m_salmon_velocity = {1.f, 0.f};
			m_move_timer = 1.f;
			m_key_up = false;
			m_salmon.set_velocity(m_salmon_velocity);
		}
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)) {
			m_salmon_velocity = {-1.f, 0.f};
			m_move_timer = 1.f;
			m_key_up = false;
			m_salmon.set_velocity(m_salmon_velocity);
		}
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)) {
            m_salmon.start_rotating(1);
		}
		if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)) {
            m_salmon.start_rotating(-1);
		}
		
		if (action == GLFW_RELEASE && (key == GLFW_KEY_UP || key == GLFW_KEY_W)) {
			m_key_up = true;
		}
		if (action == GLFW_RELEASE && (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)) {
			m_key_up = true;
		}
	}

    if (action == GLFW_RELEASE && (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)) {
        m_salmon.stop_rotating();
    }
    if (action == GLFW_RELEASE && (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)) {
        m_salmon.stop_rotating();
    }

	if (action == GLFW_RELEASE && key == GLFW_KEY_N) {
		m_mode1 = true;
		m_salmon.set_mode(m_mode1);
	}
	if (action == GLFW_RELEASE && key == GLFW_KEY_M) {
		m_mode1 = false;
		m_salmon.set_mode(m_mode1);
	}
    if (action == GLFW_RELEASE && key == GLFW_KEY_K && !m_mode3) {
        m_mode2 = true;

        m_turtles.resize(1);

        if (!m_turtles.empty())
            m_turtles[0].set_mode(m_mode2);

    }
    if (action == GLFW_RELEASE && key == GLFW_KEY_L) {
        m_mode2 = false;

        if (!m_turtles.empty())
            m_turtles[0].set_mode(m_mode2);
    }

    if (action == GLFW_PRESS && key == GLFW_KEY_B) {
        m_debugging = !m_debugging;

        if (!m_debugging) {
            m_freeze = false;
            m_elapsed_freeze_time = 0;
        }

        m_water.set_debugging(m_debugging);
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_U) {
        m_mode3 = true;
        m_mode2 = false;

        m_pebbles_emitter.set_mode3(m_mode3);
        TURTLE_DELAY_MS = m_mode3_turtle_delay;
        load_dope_sounds();

        for (auto &fish : m_fish)
            fish.reskin();
        for (auto &turtle : m_turtles)
            turtle.reskin();

        if (!m_turtles.empty())
            m_turtles[0].set_mode(m_mode2);
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_I) {
        m_mode3 = false;
        m_pebbles_emitter.set_mode3(m_mode3);
        TURTLE_DELAY_MS = m_base_turtle_delay;
        load_default_sounds();

        for (auto &fish : m_fish)
            fish.default_texture();
        for (auto &turtle : m_turtles)
            turtle.default_texture();
    }


	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
        reset_world();
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) &&  key == GLFW_KEY_COMMA) {
        m_current_speed -= 0.1f;
        m_pebbles_emitter.set_current_speed(m_current_speed);
    }
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
        m_current_speed += 0.1f;
        m_pebbles_emitter.set_current_speed(m_current_speed);
    }


	//Control frame skip
    if (action == GLFW_RELEASE && key == GLFW_KEY_P) {
        m_frame_skip -= 10;
        if (m_frame_skip < 10)
            m_frame_skip = 10;
    }
    if (action == GLFW_RELEASE && key == GLFW_KEY_O) {
        m_frame_skip += 10;
        if (m_frame_skip > 100)
            m_frame_skip = 100;
    }

	m_current_speed = fmax(0.f, m_current_speed);
}

void World::on_mouse_move(GLFWwindow* window, double xpos, double ypos)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE SALMON ROTATION HERE
	// xpos and ypos are relative to the top-left of the window, the salmon's 
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//	if (!m_freeze) {
//        vec2 salmon_pos = m_salmon.get_position();
//        vec2 mouse_pos = {(float) xpos, (float) ypos};
//        vec2 delta = normalize(sub(mouse_pos, salmon_pos));
//
//        float theta = acos(dot(delta, {1.f, 0.f}));
//        if (delta.y < 0.f)
//            theta *= -1.f;
//
//        m_salmon.set_rotation(theta);
//    }
}

void World::on_mouse_click(GLFWwindow* window, int key, int action, int mod) {
    double xposition, yposition;
    glfwGetCursorPos(window, &xposition, &yposition);
    if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && m_mode3 && m_can_shoot) {
        m_can_shoot = false;
        m_pebbles_emitter.spawn_pebble(m_salmon.get_mouth_pos(), m_salmon.get_rotation(), m_gravity);
    }
}

void World::reset_world() {
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    m_salmon.destroy();
    m_salmon.init({m_level_bounds_padding, m_level_bounds.x - m_level_bounds_padding},
                  {m_level_bounds_padding , m_level_bounds.y - m_level_bounds_padding});
    m_pebbles_emitter.destroy();
    m_pebbles_emitter.init(m_level_bounds, m_current_speed);
    m_turtles.clear();
    m_fish.clear();
    m_water.reset_salmon_dead_time();
    m_current_speed = 0.25f;
    m_points = 0;
    m_debug_path.destroy();
    m_debug_path.init(m_level_bounds);
    m_debug_boundaries.destroy();
    m_debug_boundaries.init({m_level_bounds_padding, m_level_bounds.x - m_level_bounds_padding},
                            {m_level_bounds_padding , m_level_bounds.y - m_level_bounds_padding}, m_level_bounds);
    m_debug_collider.destroy();
    m_debug_collider.init(m_salmon.get_x_bounds(), m_salmon.get_y_bounds(), m_level_bounds, m_salmon.get_scale());
    m_debug_collision.destroy();
    m_debug_collision.init(m_salmon);

    m_next_turtle_spawn = 0;
    m_next_fish_spawn = 0;

    m_mode1 = false;
    m_mode2 = false;
    m_mode3 = false;
    m_key_up = false;
    m_freeze = false;
    m_debugging = false;

    m_water.set_debugging(m_debugging);

    load_default_sounds();
}

bool World::load_default_sounds() {
    m_background_music = Mix_LoadMUS(audio_path("music.wav"));
    m_salmon_dead_sound = Mix_LoadWAV(audio_path("salmon_dead.wav"));
    m_salmon_eat_sound = Mix_LoadWAV(audio_path("salmon_eat.wav"));

    if (m_background_music == nullptr || m_salmon_dead_sound == nullptr || m_salmon_eat_sound == nullptr) {
        fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
                audio_path("music.wav"),
                audio_path("dead.wav"),
                audio_path("eat.wav"));
        return false;
    }

    Mix_PlayMusic(m_background_music, -1);
    fprintf(stderr, "Loaded music\n");

    return true;
}

bool World::load_dope_sounds() {
    m_background_music = Mix_LoadMUS(audio_path("music2.wav"));
    m_salmon_dead_sound = Mix_LoadWAV(audio_path("dead.wav"));
    m_salmon_eat_sound = Mix_LoadWAV(audio_path("eat.wav"));

    if (m_background_music == nullptr || m_salmon_dead_sound == nullptr || m_salmon_eat_sound == nullptr) {
        fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
                audio_path("music2.wav"),
                audio_path("dead.wav"),
                audio_path("eat.wav"));
        return false;
    }

    Mix_PlayMusic(m_background_music, -1);
    fprintf(stderr, "Loaded dope music\n");

    return true;
}


