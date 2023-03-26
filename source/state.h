#pragma once

#include "types.h"


class state
{
    // System
    kl::window window = { { 1600, 900 }, "Particle Renderer" };
    kl::gpu gpu = { (HWND) window };

    // Utility
    kl::timer timer = {};
    kl::camera camera = {};

    // Mesh
    float mesh_scaling = 1.0f;
    kl::float3 mesh_offset = {};
    std::string mesh_path = "";
    std::vector<kl::triangle> loaded_mesh = {};

    // Texture
    std::string texture_path = "";
    kl::image loaded_texture = {};

    // Container
    kl::dx::buffer container_mesh = nullptr;

    // Particles
    std::vector<particle> particles = {};
    kl::dx::buffer particles_mesh = nullptr;
    kl::dx::access_view particles_view = nullptr;

    // Render shaders
    kl::render_shaders render_shaders = {};
    kl::dx::buffer vs_const_buffer = nullptr;

    // Compute shader
    kl::dx::compute_shader compute_shader = nullptr;
    kl::dx::buffer cs_const_buffer = nullptr;

    // Camera movement
    kl::int2 start_mouse_position = {};
    kl::float2 start_rotations = {};
    kl::float2 total_rotations = {};

    // Particle generation
    float generation_precision = 0.005f;
    bool use_wireframe = false;
    bool generate_exploded = false;

    // Scene
    kl::float3 container_scale = kl::float3(1.0f);
    float force_strength = 1.0f;
    float energy_retain = 0.7f;
    bool return_home = false;

    // GUI
    bool is_window_hovered = false;

    // Init
    void create_render_shaders();
    void setup_gui();

    // Update
    void update_zoom();
    void compute_physics();
    void render_particles();
    void render_gui();

    // Callbacks
    void on_window_resize(kl::int2 new_size);
    void reload_camera();
    void on_camera_start();
    void on_camera_update();

    // Reused
    void reload_mesh();
    void reload_texture();

    void generate_particle_line(const kl::triangle& triangle, const kl::float3& start, const kl::float3 end);
    void generate_particles();

    void create_particle_buffer();
    void create_container_buffer();

public:
    state();

    bool process();
    void on_update();
};
