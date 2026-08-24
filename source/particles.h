#pragma once

#include "klibrary.h"

struct Particle
{
    float3 home;
    float3 position;
    float3 velocity;
    float3 color;
};

enum struct ColorType
{
    SINGLE,
    POSITION,
    RANDOM,
    RANDOM_GRAYSCALE,
};

struct Particles
{
    // System
    kl::Window window{"Particles"};
    kl::GPU gpu{window.ptr()};
    kl::Timer timer{};
    kl::Camera camera{};

    // Selected Mesh
    float3 selected_mesh_scaling{1.0f};
    float3 selected_mesh_offset;
    std::string selected_mesh_path;
    std::vector<kl::Triangle> selected_mesh_triangles;

    // Selected Texture
    std::string selected_texture_path;
    kl::Image selected_texture;

    // Container
    kl::dx::Buffer container_mesh;
    float3 container_scale{1.0f};

    // Particles
    std::vector<Particle> particles;
    kl::dx::Buffer particle_buffer;
    kl::dx::AccessView particle_buffer_view;

    // Shaders
    kl::Shaders shaders;
    kl::ComputeShader compute_shader;

    // Camera Movement
    float2 camera_rotations;
    float2 start_camera_rotations;
    int2 start_mouse_position;

    // Scene
    float force_strength = 1.0f;
    float energy_retain = 0.7f;
    bool return_home = false;
    float return_home_velocity = 0.5f;

    // Particle Box
    int box_particle_count = 1'000'000;
    float box_particle_velocity_limit = 0.1f;
    ColorType box_particle_color_type = ColorType::POSITION;
    float3 box_particle_color_single = kl::colors::WHITE;

    // Particle Mesh
    float generation_precision = 0.005f;
    bool use_wireframe = false;
    bool use_texture = true;
    bool generate_exploded = false;

    // UI
    bool is_ui_hovered = false;

    Particles();
    ~Particles() noexcept;

    bool process();

  private:
    void load_theme();

    void handle_keybinds();
    void update_camera();
    void compute_physics();
    void render_particles();
    void render_ui();

    void reload_selected_mesh();
    void reload_selected_texture();
    void reload_particle_buffer();
    void reload_container_mesh();

    void generate_particle_box();
    void generate_particle_mesh();

    void generate_particle_line(kl::Triangle const& triangle, float3 const& start, float3 const& end);
    void generate_particle_color(Particle& particle) const;
};

void drag_int(std::string_view const& text, int& value, std::function<void()> const& callback, float width = 100.0f);
void drag_float(std::string_view const& text, float& value, std::function<void()> const& callback,
                float width = 100.0f);
void drag_float3(std::string_view const& text, float3& value, std::function<void()> const& callback,
                 float width = 100.0f);
