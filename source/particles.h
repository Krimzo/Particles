#pragma once

#include "klibrary.h"


struct Particle
{
    kl::Float3 home;
    kl::Float3 position;
    kl::Float3 velocity;
    kl::Float3 color;
};

struct Particles
{
    // System
    kl::Window window{ "Particles" };
    kl::GPU gpu{ window.ptr() };
    kl::Timer timer{};
    kl::Camera camera{};

    // Selected Mesh
    float selected_mesh_scaling = 1.0f;
    kl::Float3 selected_mesh_offset;
    std::string selected_mesh_path;
    std::vector<kl::Triangle> selected_mesh_triangles;

    // Selected Texture
    std::string selected_texture_path;
    kl::Image selected_texture;

    // Container
    kl::dx::Buffer container_mesh;
    kl::Float3 container_scale{ 1.0f };

    // Particles
    std::vector<Particle> particles;
    kl::dx::Buffer particle_buffer;
    kl::dx::AccessView particle_buffer_view;

    // Shaders
    kl::Shaders shaders;
    kl::ComputeShader compute_shader;

    // Camera movement
    kl::Float2 camera_rotations;
    kl::Float2 start_camera_rotations;
    kl::Int2 start_mouse_position;

    // Particle generation
    float generation_precision = 0.005f;
    bool use_wireframe = false;
    bool generate_exploded = false;

    // Scene
    float force_strength = 1.0f;
    float energy_retain = 0.7f;
    bool return_home = false;

    // GUI
    bool is_window_hovered = false;

    Particles();
    ~Particles() noexcept;

    bool process();

private:
    void setup_ui_colors();

    void update_camera();
    void compute_physics();
    void render_particles();
    void render_ui();

    void reload_selected_mesh();
    void reload_selected_texture();
    void reload_particle_buffer();
    void reload_container_mesh();

    void generate_particle_line( kl::Triangle const& triangle, kl::Float3 const& start, kl::Float3 const& end );
    void generate_particles();
};
