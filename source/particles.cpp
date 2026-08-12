#include "particles.h"

static const ImU32 I_COLOR = (ImU32) ImColor( 100, 200, 200 );
static const ImU32 F_COLOR = (ImU32) ImColor( 200, 200, 100 );
static const ImU32 X_COLOR = (ImU32) ImColor( 200, 100, 100 );
static const ImU32 Y_COLOR = (ImU32) ImColor( 100, 200, 100 );
static const ImU32 Z_COLOR = (ImU32) ImColor( 100, 100, 200 );

Particles::Particles()
{
    window.on_resize.emplace_back( [this]( kl::Int2 size )
        {
            gpu.resize_internal( size );
            gpu.set_viewport_size( size );
            camera.update_aspect_ratio( size );
        } );
    window.maximize();

    const std::initializer_list<kl::dx::LayoutDescriptor> layout_descriptors = {
        { "KL_Home", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Velocity", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Color", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    shaders = gpu.create_shaders( kl::read_file_string( "shaders/render.hlsl" ), layout_descriptors );
    compute_shader = gpu.create_compute_shader( kl::read_file_string( "shaders/compute.hlsl" ) );

    reload_container_mesh();

    camera.speed = 5.0f;       // camera distance
    camera.sensitivity = 0.5f; // deg/px
    camera.background = kl::RGB{ 40, 40, 40 };
    update_camera();

    imgui::CreateContext();
    ImGui_ImplWin32_Init( window.ptr() );
    ImGui_ImplDX11_Init( gpu.device().get(), gpu.context().get() );

    auto& imgui_io = imgui::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    imgui_io.IniFilename = nullptr;

    load_theme();
}

Particles::~Particles()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    imgui::DestroyContext();
}

bool Particles::process()
{
    timer.update();
    handle_keybinds();
    update_camera();
    gpu.clear_internal( camera.background );
    compute_physics();
    render_particles();
    render_ui();
    gpu.swap_buffers( true );
    return window.process();
}

void Particles::load_theme()
{
    const kl::Float4 special_color = kl::colors::WHITE;
    const kl::Float4 alternate_color = kl::colors::BLACK;
    ImGuiStyle& style = imgui::GetStyle();

    style.Colors[ImGuiCol_Text] = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
    style.Colors[ImGuiCol_TextDisabled] = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );

    style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f );
    style.Colors[ImGuiCol_ChildBg] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );
    style.Colors[ImGuiCol_PopupBg] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );

    style.Colors[ImGuiCol_Border] = ImVec4( 0.45f, 0.45f, 0.45f, 0.50f );
    style.Colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );

    style.Colors[ImGuiCol_FrameBg] = style.Colors[ImGuiCol_ChildBg];
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.30f, 0.30f, 0.30f, 1.00f );
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4( 0.60f, 0.60f, 0.60f, 0.40f );

    style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f );
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f );
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.00f, 0.00f, 0.00f, 0.51f );

    style.Colors[ImGuiCol_MenuBarBg] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );

    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.02f, 0.02f, 0.02f, 0.53f );
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.31f, 0.31f, 0.31f, 1.00f );
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.51f, 0.51f, 0.51f, 1.00f );

    style.Colors[ImGuiCol_CheckMark] = (ImVec4&) special_color;
    style.Colors[ImGuiCol_CheckboxSelectedBg] = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );

    style.Colors[ImGuiCol_SliderGrab] = (ImVec4&) special_color;
    style.Colors[ImGuiCol_SliderGrabActive] = (ImVec4&) special_color;

    style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_FrameBg];
    style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_FrameBgHovered];
    style.Colors[ImGuiCol_ButtonActive] = style.Colors[ImGuiCol_FrameBgActive];

    style.Colors[ImGuiCol_Header] = ImVec4( 0.22f, 0.22f, 0.22f, 1.00f );
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
    style.Colors[ImGuiCol_HeaderActive] = ImVec4( 0.67f, 0.67f, 0.67f, 0.39f );

    style.Colors[ImGuiCol_Separator] = ImVec4( 0.45f, 0.45f, 0.45f, 0.50f );
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.44f, 0.44f, 0.44f, 1.00f );
    style.Colors[ImGuiCol_SeparatorActive] = (ImVec4&) special_color;

    style.Colors[ImGuiCol_ResizeGrip] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.30f, 0.30f, 0.30f, 0.67f );
    style.Colors[ImGuiCol_ResizeGripActive] = (ImVec4&) special_color;

    style.Colors[ImGuiCol_Tab] = ImVec4( 0.08f, 0.08f, 0.08f, 0.83f );
    style.Colors[ImGuiCol_TabHovered] = ImVec4( 0.35f, 0.35f, 0.35f, 0.83f );
    style.Colors[ImGuiCol_TabActive] = ImVec4( 0.23f, 0.23f, 0.23f, 1.00f );
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f );
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );
    style.Colors[ImGuiCol_TabSelectedOverline] = (ImVec4&) special_color;

    style.Colors[ImGuiCol_DockingPreview] = (ImVec4&) special_color;
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );

    style.Colors[ImGuiCol_PlotLines] = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );

    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.42f, 0.69f, 0.69f, 0.32f );

    style.Colors[ImGuiCol_DragDropTarget] = (ImVec4&) special_color;

    style.Colors[ImGuiCol_NavHighlight] = (ImVec4&) special_color;
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );

    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );

    style.WindowPadding = ImVec2( 8.00f, 8.00f );
    style.FramePadding = ImVec2( 5.00f, 2.00f );
    style.CellPadding = ImVec2( 6.00f, 6.00f );
    style.ItemSpacing = ImVec2( 6.00f, 6.00f );
    style.ItemInnerSpacing = ImVec2( 6.00f, 6.00f );
    style.TouchExtraPadding = ImVec2( 0.00f, 0.00f );
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;
}

void Particles::handle_keybinds()
{
    if ( window.keyboard.esc.pressed() )
        window.close();

    if ( window.keyboard.f11.pressed() )
        gpu.set_fullscreen( !gpu.fullscreened() );

    if ( window.keyboard.r.pressed() )
        return_home = !return_home;
}

void Particles::update_camera()
{
    static constexpr float VERTICAL_LIMIT = 85.0f;
    static constexpr float ZOOM_LIMIT = 1.0f;

    camera.speed -= window.mouse.scroll();
    camera.speed = kl::max( camera.speed, ZOOM_LIMIT );

    if ( window.mouse.right.pressed() )
    {
        start_mouse_position = window.mouse.position();
        start_camera_rotations = camera_rotations;
    }

    if ( window.mouse.right )
    {
        const kl::Int2 mouse_delta = window.mouse.position() - start_mouse_position;
        camera_rotations.x = start_camera_rotations.x + mouse_delta.x * camera.sensitivity;
        camera_rotations.y = kl::clamp( start_camera_rotations.y + mouse_delta.y * camera.sensitivity, -VERTICAL_LIMIT, VERTICAL_LIMIT );

        if ( camera_rotations.y == -VERTICAL_LIMIT || camera_rotations.y == VERTICAL_LIMIT )
        {
            start_mouse_position.y = window.mouse.position().y;
            start_camera_rotations.y = camera_rotations.y;
        }
    }

    camera.position = { 0.0f, 0.0f, -camera.speed };
    camera.position = kl::rotate( camera.position, { 1.0f, 0.0f, 0.0f }, camera_rotations.y );
    camera.position = kl::rotate( camera.position, { 0.0f, 1.0f, 0.0f }, camera_rotations.x );
    camera.set_forward( -camera.position );
}

void Particles::compute_physics()
{
    struct alignas( 16 ) CB
    {
        kl::Float3 FORCE_RAY_ORIGIN;
        float USE_RAY_FORCE;
        kl::Float3 FORCE_RAY_DIRECTION;
        float FORCE_STRENGTH;
        kl::Float3 CONTAINER_SCALE;
        float RETURN_HOME;
        float RETURN_HOME_VELOCITY;
        float ENERGY_RETAIN;
        float ELAPSED_TIME;
        float DELTA_TIME;
        UINT PARTICLE_COUNT;
    } cb = {};

    cb.PARTICLE_COUNT = gpu.vertex_buffer_size( particle_buffer, sizeof( Particle ) );
    cb.ELAPSED_TIME = timer.elapsed();
    cb.DELTA_TIME = timer.delta();
    cb.RETURN_HOME = (float) return_home;
    cb.RETURN_HOME_VELOCITY = return_home_velocity;
    cb.CONTAINER_SCALE = container_scale;
    cb.FORCE_STRENGTH = force_strength;
    cb.ENERGY_RETAIN = energy_retain;

    if ( window.mouse.left && !is_ui_hovered )
    {
        const kl::Float2 ndc = window.mouse.ndc_pos();
        const kl::Ray ray = { camera.position, kl::inverse( camera.matrix() ), ndc };
        cb.FORCE_RAY_ORIGIN = ray.origin;
        cb.FORCE_RAY_DIRECTION = ray.direction();
        cb.USE_RAY_FORCE = 1.0f;
    }

    gpu.bind_compute_shader( compute_shader.shader );
    compute_shader.upload( cb );

    gpu.bind_access_view_for_compute_shader( particle_buffer_view, 0 );
    gpu.dispatch_compute_shader( cb.PARTICLE_COUNT / 1024 + 1, 1, 1 );
    gpu.unbind_access_view_for_compute_shader( 0 );
}

void Particles::render_particles()
{
    struct alignas( 16 ) CB
    {
        kl::Float4x4 VP;
    } cb = {};

    cb.VP = camera.matrix();

    gpu.bind_shaders( shaders );
    shaders.upload( cb );

    gpu.draw( particle_buffer, D3D_PRIMITIVE_TOPOLOGY_POINTLIST, sizeof( Particle ) );
    gpu.draw( container_mesh, D3D_PRIMITIVE_TOPOLOGY_LINELIST, sizeof( Particle ) );
}

void Particles::render_ui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    imgui::NewFrame();

    if ( imgui::Begin( "Scene" ) )
    {
        is_ui_hovered = imgui::GetIO().WantCaptureMouse;

        imgui::Text( kl::format( "Camera Rotations: ", std::fixed, camera_rotations ).c_str() );
        imgui::Text( kl::format( "Camera Origin: ", std::fixed, camera.position ).c_str() );
        imgui::Text( kl::format( "Camera Direction: ", std::fixed, camera.forward() ).c_str() );
        kl::Float3 background = camera.background;
        if ( imgui::ColorEdit3( "Camera Background", &background.x, ImGuiColorEditFlags_NoInputs ) )
        {
            camera.background = background;
            reload_container_mesh();
        }

        imgui::Separator();

        drag_float3( "Container Scale", container_scale, std::bind( &Particles::reload_container_mesh, this ) );
        drag_float( "Force Strength", force_strength, [] {} );
        drag_float( "Energy Retain", energy_retain, [] {} );
        drag_float( "Return Home Velocity", return_home_velocity, [] {} );
        imgui::Checkbox( "Return Home", &return_home );

        imgui::Separator();

        const size_t cpu_size = particles.size();
        const UINT gpu_size = gpu.vertex_buffer_size( particle_buffer, sizeof( Particle ) );
        imgui::Text( kl::format( "CPU Particle Count: ", cpu_size, " [", cpu_size * sizeof( Particle ) * 1e-6, " MB]" ).c_str() );
        imgui::Text( kl::format( "GPU Particle Count: ", gpu_size, " [", gpu_size * sizeof( Particle ) * 1e-6, " MB]" ).c_str() );
        drag_int( "Box Particle Count", box_particle_count, [] {} );
        drag_float( "Box Particle Velocity Limit", box_particle_velocity_limit, [] {} );
        bool box_color_type = box_particle_color_type == ColorType::SINGLE;
        if ( imgui::Checkbox( "Single##ColorType", &box_color_type ) )
            box_particle_color_type = ColorType::SINGLE;
        imgui::SameLine();
        box_color_type = box_particle_color_type == ColorType::POSITION;
        if ( imgui::Checkbox( "Position##ColorType", &box_color_type ) )
            box_particle_color_type = ColorType::POSITION;
        imgui::SameLine();
        box_color_type = box_particle_color_type == ColorType::RANDOM;
        if ( imgui::Checkbox( "Random##ColorType", &box_color_type ) )
            box_particle_color_type = ColorType::RANDOM;
        imgui::SameLine();
        box_color_type = box_particle_color_type == ColorType::RANDOM_GRAYSCALE;
        if ( imgui::Checkbox( "Random Grayscale##ColorType", &box_color_type ) )
            box_particle_color_type = ColorType::RANDOM_GRAYSCALE;
        if ( box_particle_color_type == ColorType::SINGLE )
            imgui::ColorEdit3( "Box Particle Color Single", &box_particle_color_single.x, ImGuiColorEditFlags_NoInputs );
        if ( imgui::Button( "Generate Box Particles" ) )
        {
            generate_particle_box();
            reload_particle_buffer();
        }

        imgui::Separator();

        const size_t triangle_count = selected_mesh_triangles.size();
        imgui::Text( kl::format( "Mesh Triangle Count: ", triangle_count, " [", triangle_count * sizeof( kl::Triangle ) * 1e-6, " MB]" ).c_str() );
        const size_t image_size = (size_t) selected_texture.width() * selected_texture.height();
        imgui::Text( kl::format( "Texture Resolution: ", selected_texture.size(), " [", image_size * sizeof( kl::RGB ) * 1e-6, " MB]" ).c_str() );
        if ( !selected_mesh_path.empty() )
        {
            if ( imgui::Button( "Erase##SelectedMeshPath" ) )
                selected_mesh_path.erase();
            imgui::SameLine();
        }
        if ( imgui::Button( kl::format( "Mesh Path: ", selected_mesh_path, "##SelectedMeshPath" ).c_str() ) )
        {
            if ( auto opt_file = kl::choose_file( false, { { "Mesh Files", ".obj" } } ) )
                selected_mesh_path = *opt_file;
        }
        if ( !selected_texture_path.empty() )
        {
            if ( imgui::Button( "Erase##SelectedTexturePath" ) )
                selected_texture_path.erase();
            imgui::SameLine();
        }
        if ( imgui::Button( kl::format( "Texture Path: ", selected_texture_path, "##SelectedTexturePath" ).c_str() ) )
        {
            if ( auto opt_file = kl::choose_file( false ) )
            {
                if ( kl::probe_content_type( *opt_file ).value_or( {} ).starts_with( "image" ) )
                    selected_texture_path = *opt_file;
            }
        }
        drag_float3( "Mesh Scaling", selected_mesh_scaling, [] {} );
        drag_float3( "Mesh Offset", selected_mesh_offset, [] {} );
        drag_float( "Generation Precision", generation_precision, [] {} );
        imgui::Checkbox( "Generate As Wireframe", &use_wireframe );
        imgui::Checkbox( "Use Texture", &use_texture );
        imgui::Checkbox( "Generate Exploded", &generate_exploded );
        imgui::BeginDisabled( selected_mesh_path.empty() );
        if ( imgui::Button( "Generate Mesh Particles" ) )
        {
            reload_selected_mesh();
            if ( use_texture )
                reload_selected_texture();
            generate_particle_mesh();
            reload_particle_buffer();
        }
        imgui::EndDisabled();
    }
    imgui::End();

    imgui::Render();
    ImGui_ImplDX11_RenderDrawData( imgui::GetDrawData() );
}

void Particles::reload_selected_mesh()
{
    selected_mesh_triangles.clear();
    std::vector<kl::Vertex> vertices = kl::parse_obj_file( selected_mesh_path, true );
    for ( kl::Vertex& vertex : vertices )
    {
        vertex.position *= selected_mesh_scaling;
        vertex.position += selected_mesh_offset;
    }
    selected_mesh_triangles.resize( vertices.size() / 3 );
    memcpy( selected_mesh_triangles.data(), vertices.data(), selected_mesh_triangles.size() * sizeof( kl::Triangle ) );
}

void Particles::reload_selected_texture()
{
    selected_texture = {};
    selected_texture.load_from_file( selected_texture_path );
}

void Particles::reload_particle_buffer()
{
    kl::dx::BufferDescriptor descriptor{};
    descriptor.Usage = D3D11_USAGE_DEFAULT;
    descriptor.StructureByteStride = sizeof( Particle );
    descriptor.ByteWidth = UINT( particles.size() * sizeof( Particle ) );
    descriptor.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    descriptor.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    kl::dx::SubresourceDescriptor subresource_data{};
    subresource_data.pSysMem = particles.data();

    particle_buffer = gpu.create_buffer( &descriptor, &subresource_data );
    particle_buffer_view = gpu.create_access_view( particle_buffer, nullptr );
}

void Particles::reload_container_mesh()
{
    const std::vector<Particle> particles = {
        { {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
        { {}, { container_scale.x, -container_scale.y, -container_scale.z }, {}, kl::RGB{ 200, 100, 100 } },

        { {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
        { {}, { -container_scale.x, container_scale.y, -container_scale.z }, {}, kl::RGB{ 100, 200, 100 } },

        { {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
        { {}, { -container_scale.x, -container_scale.y, container_scale.z }, {}, kl::RGB{ 100, 100, 200 } },
    };

    kl::dx::BufferDescriptor descriptor{};
    descriptor.Usage = D3D11_USAGE_DEFAULT;
    descriptor.ByteWidth = UINT( particles.size() * sizeof( Particle ) );
    descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    kl::dx::SubresourceDescriptor subresource_data{};
    subresource_data.pSysMem = particles.data();

    container_mesh = gpu.create_buffer( &descriptor, &subresource_data );
}

void Particles::generate_particle_box()
{
    particles.resize( box_particle_count );
    std::for_each( std::execution::par, particles.begin(), particles.end(), [&]( Particle& particle )
        {
            particle.home.x = kl::random::gen_float( -container_scale.x, container_scale.x );
            particle.home.y = kl::random::gen_float( -container_scale.y, container_scale.y );
            particle.home.z = kl::random::gen_float( -container_scale.z, container_scale.z );
            particle.position = particle.home;
            particle.velocity = kl::random::gen_float3( -box_particle_velocity_limit, box_particle_velocity_limit );
            generate_particle_color( particle );
        } );
}

void Particles::generate_particle_mesh()
{
    particles.clear();
    particles.reserve( 1'000'000 );

    if ( use_wireframe )
    {
        for ( kl::Triangle const& triangle : selected_mesh_triangles )
        {
            generate_particle_line( triangle, triangle.a.position, triangle.b.position );
            generate_particle_line( triangle, triangle.b.position, triangle.c.position );
            generate_particle_line( triangle, triangle.c.position, triangle.a.position );
        }
    }
    else
    {
        for ( kl::Triangle const& triangle : selected_mesh_triangles )
        {
            const float a_walk_distance = ( triangle.c.position - triangle.a.position ).length();
            const float b_walk_distance = ( triangle.c.position - triangle.b.position ).length();

            const int a_step_count = int( a_walk_distance / generation_precision );
            const int b_step_count = int( b_walk_distance / generation_precision );
            const int step_count = kl::min( a_step_count, b_step_count );

            const kl::Float3 a_walk_direction = kl::normalize( triangle.c.position - triangle.a.position );
            const kl::Float3 b_walk_direction = kl::normalize( triangle.c.position - triangle.b.position );

            for ( int i = 0; i <= step_count; i++ )
            {
                const kl::Float3 a_walk_point = triangle.a.position + a_walk_direction * ( i * generation_precision );
                const kl::Float3 b_walk_point = triangle.b.position + b_walk_direction * ( i * generation_precision );
                generate_particle_line( triangle, a_walk_point, b_walk_point );
            }
        }
    }
}

void Particles::generate_particle_line( kl::Triangle const& triangle, kl::Float3 const& start, kl::Float3 const& end )
{
    const float walk_distance = ( end - start ).length();
    const int step_count = int( walk_distance / generation_precision );
    const kl::Float3 walk_direction = kl::normalize( end - start );

    for ( int i = 0; i <= step_count; i++ )
    {
        Particle& particle = particles.emplace_back();
        particle.home = start + walk_direction * ( i * generation_precision );
        particle.position = particle.home;

        if ( generate_exploded )
            particle.velocity = kl::random::gen_float3( -0.25f, 0.25f );

        const kl::Float3 weights = triangle.weights( particle.position );
        const float u = kl::Triangle::interpolate( weights, { triangle.a.uv.x, triangle.b.uv.x, triangle.c.uv.x } );
        const float v = kl::Triangle::interpolate( weights, { triangle.a.uv.y, triangle.b.uv.y, triangle.c.uv.y } );
        if ( use_texture )
            particle.color = selected_texture.sample( { u, 1 - v } );
        else
            generate_particle_color( particle );
    }
}

void Particles::generate_particle_color( Particle& particle ) const
{
    switch ( box_particle_color_type )
    {
    default:
        particle.color = {};
        break;

    case ColorType::SINGLE:
        particle.color = box_particle_color_single;
        break;

    case ColorType::POSITION:
        particle.color.x = ( particle.home.x + container_scale.x ) / ( 2 * container_scale.x );
        particle.color.y = ( particle.home.y + container_scale.y ) / ( 2 * container_scale.y );
        particle.color.z = ( particle.home.z + container_scale.z ) / ( 2 * container_scale.z );
        break;

    case ColorType::RANDOM:
        particle.color = kl::random::gen_rgb( false );
        break;

    case ColorType::RANDOM_GRAYSCALE:
        particle.color = kl::random::gen_rgb( true );
        break;
    }
}

void drag_int( std::string_view const& text, int& value, std::function<void()> const& callback, float width )
{
    ImGuiStyle& style = imgui::GetStyle();
    imgui::SetCursorPosY( imgui::GetCursorPosY() + style.FramePadding.y );
    imgui::Text( text.data() );
    imgui::SameLine();

    imgui::SetCursorPosY( imgui::GetCursorPosY() - style.FramePadding.y );
    imgui::SetNextItemWidth( width );
    imgui::PushStyleColor( ImGuiCol_Text, I_COLOR );
    if ( imgui::DragInt( kl::format( "##", text, "I" ).c_str(), &value ) )
        callback();

    imgui::PopStyleColor( 1 );
}

void drag_float( std::string_view const& text, float& value, std::function<void()> const& callback, float width )
{
    ImGuiStyle& style = imgui::GetStyle();
    imgui::SetCursorPosY( imgui::GetCursorPosY() + style.FramePadding.y );
    imgui::Text( text.data() );
    imgui::SameLine();

    imgui::SetCursorPosY( imgui::GetCursorPosY() - style.FramePadding.y );
    imgui::SetNextItemWidth( width );
    imgui::PushStyleColor( ImGuiCol_Text, F_COLOR );
    if ( imgui::DragFloat( kl::format( "##", text, "F" ).c_str(), &value, 0.01f ) )
        callback();

    imgui::PopStyleColor( 1 );
}

void drag_float3( std::string_view const& text, kl::Float3& value, std::function<void()> const& callback, float width )
{
    ImGuiStyle& style = imgui::GetStyle();
    imgui::SetCursorPosY( imgui::GetCursorPosY() + style.FramePadding.y );
    imgui::Text( text.data() );
    imgui::SameLine();

    imgui::SetCursorPosY( imgui::GetCursorPosY() - style.FramePadding.y );
    imgui::SetNextItemWidth( width );
    imgui::PushStyleColor( ImGuiCol_Text, X_COLOR );
    if ( imgui::DragFloat( kl::format( "##", text, "X" ).c_str(), &value.x, 0.01f ) )
        callback();
    imgui::SameLine();

    imgui::SetCursorPosY( imgui::GetCursorPosY() - style.FramePadding.y );
    imgui::SetNextItemWidth( width );
    imgui::PushStyleColor( ImGuiCol_Text, Y_COLOR );
    if ( imgui::DragFloat( kl::format( "##", text, "Y" ).c_str(), &value.y, 0.01f ) )
        callback();
    imgui::SameLine();

    imgui::SetCursorPosY( imgui::GetCursorPosY() - style.FramePadding.y );
    imgui::SetNextItemWidth( width );
    imgui::PushStyleColor( ImGuiCol_Text, Z_COLOR );
    if ( imgui::DragFloat( kl::format( "##", text, "Z" ).c_str(), &value.z, 0.01f ) )
        callback();

    imgui::PopStyleColor( 3 );
}
