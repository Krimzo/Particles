#include "particles.h"


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

    camera.speed = 3.0f;
    camera.sensitivity = 0.01f;
    camera.background = kl::RGB{ 50, 50, 50 };
    update_camera();

    imgui::CreateContext();
    ImGui_ImplWin32_Init( window.ptr() );
    ImGui_ImplDX11_Init( gpu.device().get(), gpu.context().get() );
    imgui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    setup_ui_colors();
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
    update_camera();
    gpu.clear_internal( camera.background );
    compute_physics();
    render_particles();
    render_ui();
    gpu.swap_buffers( true );
    return window.process();
}

void Particles::setup_ui_colors()
{
    imgui::StyleColorsDark();
    ImGuiStyle& style = imgui::GetStyle();

    style.WindowPadding = ImVec2( 15.0f, 15.0f );
    style.WindowRounding = 2.0f;
    style.FramePadding = ImVec2( 5.0f, 5.0f );
    style.FrameRounding = 2.0f;
    style.ItemSpacing = ImVec2( 12.0f, 8.0f );
    style.ItemInnerSpacing = ImVec2( 8.0f, 6.0f );
    style.SelectableTextAlign = ImVec2( 0.5f, 0.5f );
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 5.0f;
    style.GrabRounding = 3.0f;
    style.PopupBorderSize = 1.0f;
    style.PopupRounding = 5.0f;
    style.ChildBorderSize = 1.0f;
    style.ChildRounding = 5.0f;

    const ImVec4 colorNone = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );
    const ImVec4 colorDark = ImVec4( 0.1f, 0.1f, 0.1f, 1.0f );
    const ImVec4 colorMid = ImVec4( 0.2f, 0.2f, 0.2f, 1.0f );
    const ImVec4 colorLight = ImVec4( 0.3f, 0.3f, 0.3f, 1.0f );
    const ImVec4 colorSpec = ImVec4( 0.7f, 0.4f, 0.0f, 1.0f );

    style.Colors[ImGuiCol_Text] = ImVec4( 0.95f, 0.95f, 0.95f, 1.0f );
    style.Colors[ImGuiCol_TextDisabled] = colorLight;
    style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.077f, 0.077f, 0.077f, 1.0f );
    style.Colors[ImGuiCol_ChildBg] = colorDark;
    style.Colors[ImGuiCol_PopupBg] = colorDark;
    style.Colors[ImGuiCol_Border] = colorLight;
    style.Colors[ImGuiCol_BorderShadow] = colorMid;
    style.Colors[ImGuiCol_FrameBg] = colorDark;
    style.Colors[ImGuiCol_FrameBgHovered] = colorMid;
    style.Colors[ImGuiCol_FrameBgActive] = colorLight;
    style.Colors[ImGuiCol_TitleBg] = colorDark;
    style.Colors[ImGuiCol_TitleBgActive] = colorDark;
    style.Colors[ImGuiCol_TitleBgCollapsed] = colorDark;
    style.Colors[ImGuiCol_MenuBarBg] = colorDark;
    style.Colors[ImGuiCol_ScrollbarBg] = colorDark;
    style.Colors[ImGuiCol_ScrollbarGrab] = colorLight;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = colorMid;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = colorLight;
    style.Colors[ImGuiCol_CheckMark] = colorSpec;
    style.Colors[ImGuiCol_SliderGrab] = colorSpec;
    style.Colors[ImGuiCol_SliderGrabActive] = colorSpec;
    style.Colors[ImGuiCol_Button] = colorMid;
    style.Colors[ImGuiCol_ButtonHovered] = colorLight;
    style.Colors[ImGuiCol_ButtonActive] = colorLight;
    style.Colors[ImGuiCol_Header] = colorMid;
    style.Colors[ImGuiCol_HeaderHovered] = colorLight;
    style.Colors[ImGuiCol_HeaderActive] = colorSpec;
    style.Colors[ImGuiCol_Separator] = colorMid;
    style.Colors[ImGuiCol_SeparatorHovered] = colorLight;
    style.Colors[ImGuiCol_SeparatorActive] = colorSpec;
    style.Colors[ImGuiCol_ResizeGrip] = colorMid;
    style.Colors[ImGuiCol_ResizeGripHovered] = colorLight;
    style.Colors[ImGuiCol_ResizeGripActive] = colorSpec;
    style.Colors[ImGuiCol_Tab] = colorMid;
    style.Colors[ImGuiCol_TabHovered] = colorSpec;
    style.Colors[ImGuiCol_TabActive] = colorSpec;
    style.Colors[ImGuiCol_TabUnfocused] = colorMid;
    style.Colors[ImGuiCol_TabUnfocusedActive] = colorLight;
    style.Colors[ImGuiCol_DockingPreview] = colorSpec;
    style.Colors[ImGuiCol_DockingEmptyBg] = colorMid;
    style.Colors[ImGuiCol_PlotLines] = colorSpec;
    style.Colors[ImGuiCol_PlotLinesHovered] = colorLight;
    style.Colors[ImGuiCol_PlotHistogram] = colorSpec;
    style.Colors[ImGuiCol_PlotHistogramHovered] = colorLight;
    style.Colors[ImGuiCol_TableHeaderBg] = colorMid;
    style.Colors[ImGuiCol_TableBorderStrong] = colorDark;
    style.Colors[ImGuiCol_TableBorderLight] = colorLight;
    style.Colors[ImGuiCol_TableRowBg] = colorDark;
    style.Colors[ImGuiCol_TableRowBgAlt] = colorMid;
    style.Colors[ImGuiCol_TextSelectedBg] = colorLight;
    style.Colors[ImGuiCol_DragDropTarget] = colorSpec;
    style.Colors[ImGuiCol_NavHighlight] = colorSpec;
    style.Colors[ImGuiCol_NavWindowingHighlight] = colorSpec;
    style.Colors[ImGuiCol_NavWindowingDimBg] = colorMid;
    style.Colors[ImGuiCol_ModalWindowDimBg] = colorMid;
}

void Particles::update_camera()
{
    static int last_delta = 0;
    const int scroll_delta = window.mouse.scroll();
    if ( scroll_delta != last_delta )
    {
        const int new_delta = ( last_delta - scroll_delta );
        last_delta = scroll_delta;

        camera.speed += new_delta * 0.25f;
        camera.speed = kl::max( camera.speed, 0.1f );
    }

    if ( window.mouse.right.pressed() )
    {
        start_mouse_position = window.mouse.position();
        start_mouse_rotations = total_mouse_rotations;
    }
    else if ( window.mouse.right )
    {
        camera.set_forward( -camera.position );
        camera.position = ( camera.forward() * -camera.speed );
    }

    const kl::Int2 delta_mouse = ( window.mouse.position() - start_mouse_position );
    start_mouse_position = window.mouse.position();

    total_mouse_rotations.x = start_mouse_rotations.x + ( delta_mouse.x * camera.sensitivity );
    total_mouse_rotations.y = start_mouse_rotations.y + ( delta_mouse.y * camera.sensitivity );
    total_mouse_rotations.y = kl::clamp( total_mouse_rotations.y, -1.5f, 1.5f );
    start_mouse_rotations = total_mouse_rotations;

    camera.position.x = kl::sin( total_mouse_rotations.x );
    camera.position.y = kl::tan( total_mouse_rotations.y );
    camera.position.z = kl::cos( total_mouse_rotations.x );
}

void Particles::compute_physics()
{
    struct alignas( 16 ) CB
    {
        uint32_t particle_count = 0;
        kl::Float3 time_info;           // (elapsed_t, delta_t, none)
        kl::Float4 force_ray_origin;    // (origin, use_force?)
        kl::Float4 force_ray_direction; // (direction, return_home?)
        kl::Float4 container_scale;     // (scale, none)
        kl::Float4 energy_info;         // (force_strength, energy_retain, none, none)
    } cb = {};

    cb.particle_count = gpu.vertex_buffer_size( particle_buffer, sizeof( Particle ) );
    cb.time_info = { timer.elapsed(), timer.delta(), 0.0f };

    if ( window.mouse.left && !is_window_hovered )
    {
        const kl::Float2 ndc = window.mouse.ndc_pos();
        const kl::Ray ray = { camera.position, kl::inverse( camera.matrix() ), ndc };
        cb.force_ray_origin = { ray.origin, 1.0f };
        cb.force_ray_direction = { ray.direction(), 0.0f };
    }

    cb.force_ray_direction.w = (float) return_home;
    cb.container_scale = { container_scale, 0.0f };
    cb.energy_info = { force_strength, energy_retain, 0.0f, 0.0f };

    gpu.bind_compute_shader( compute_shader.shader );
    compute_shader.upload( cb );

    gpu.bind_access_view_for_compute_shader( particle_buffer_view, 0 );
    gpu.dispatch_compute_shader( cb.particle_count / 1024 + 1, 1, 1 );
    gpu.unbind_access_view_for_compute_shader( 0 );
}

void Particles::render_particles()
{
    struct alignas( 16 ) CB
    {
        kl::Float4x4 VP{};
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
    imgui::DockSpaceOverViewport();

    if ( imgui::Begin( "Scene" ) )
    {
        is_window_hovered = imgui::GetIO().WantCaptureMouse;

        // Mesh
        imgui::Text( "Mesh" );
        const size_t vertex_count = selected_mesh_triangles.size() * 3;
        imgui::Text( kl::format( "Vertex count: ", vertex_count, " [", vertex_count * sizeof( kl::Vertex ) * 1e-6, " MB]" ).c_str() );

        imgui::DragFloat( "Scaling", &selected_mesh_scaling, 0.1f, -1e6f, 1e6f );
        imgui::DragFloat3( "Offset", &selected_mesh_offset.x, 0.1f, -1e6f, 1e6f );

        if ( imgui::Button( "Reload##Mesh" ) )
            reload_selected_mesh();
        imgui::SameLine();
        imgui::InputText( "##MeshInput", &selected_mesh_path );

        // Texture
        imgui::Separator();
        imgui::Text( "Texture" );
        const size_t image_size = (size_t) selected_texture.width() * selected_texture.height();
        imgui::Text( kl::format( "Size: ", selected_texture.size(), " [", image_size * sizeof( kl::RGB ) * 1e-6, " MB]" ).c_str() );

        if ( imgui::Button( "Reload##Texture" ) )
            reload_selected_texture();
        imgui::SameLine();
        imgui::InputText( "##TextureInput", &selected_texture_path );

        // Particles
        imgui::Separator();
        imgui::Text( "Particles" );

        const size_t cpu_size = particles.size();
        const size_t gpu_size = gpu.vertex_buffer_size( particle_buffer, sizeof( Particle ) );
        imgui::Text( kl::format( "CPU count: ", cpu_size, " [", cpu_size * sizeof( Particle ) * 1e-6, " MB]" ).c_str() );
        imgui::Text( kl::format( "GPU count: ", gpu_size, " [", gpu_size * sizeof( Particle ) * 1e-6, " MB]" ).c_str() );

        imgui::DragFloat( "Precision", &generation_precision, 0.0001f, 0.0001f, 0.1f, "%.4f" );
        imgui::Checkbox( "Wireframe", &use_wireframe );
        imgui::Checkbox( "Exploded", &generate_exploded );

        if ( imgui::Button( "Generate" ) )
        {
            generate_particles();
            reload_particle_buffer();
            particle_buffer_view = gpu.create_access_view( particle_buffer, nullptr );
            return_home = false;
        }

        // Scene
        imgui::Separator();
        imgui::Text( "Scene" );

        if ( imgui::DragFloat3( "Container scale", &container_scale.x, 0.1f, 0.0f, 1e6f ) )
            reload_container_mesh();
        imgui::DragFloat( "Force strength", &force_strength, 0.1f );
        imgui::DragFloat( "Energy retain", &energy_retain, 0.1f, 0.0f, 1.0f );
        imgui::Checkbox( "Return home", &return_home );

        // Camera
        imgui::Separator();
        imgui::Text( "Camera" );

        kl::Float3 background = camera.background;
        if ( imgui::ColorEdit3( "Background", &background.x ) )
        {
            camera.background = background;
            reload_container_mesh();
        }

        imgui::Text( kl::format( "Origin: ", camera.position ).c_str() );
        imgui::Text( kl::format( "Direction: ", camera.forward() ).c_str() );
    }
    imgui::End();

    imgui::Render();
    ImGui_ImplDX11_RenderDrawData( imgui::GetDrawData() );
}

void Particles::reload_selected_mesh()
{
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
}

void Particles::reload_container_mesh()
{
    std::vector<Particle> particles
    {
        { {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
        { {}, { container_scale.x, -container_scale.y, -container_scale.z }, {}, kl::RGB( camera.background.r, 0, 0 ).inverted() },

        { {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
        { {}, { -container_scale.x, container_scale.y, -container_scale.z }, {}, kl::RGB( 0, camera.background.g, 0 ).inverted() },

        { {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
        { {}, { -container_scale.x, -container_scale.y, container_scale.z }, {}, kl::RGB( 0, 0, camera.background.b ).inverted() },
    };

    kl::dx::BufferDescriptor descriptor{};
    descriptor.Usage = D3D11_USAGE_DEFAULT;
    descriptor.ByteWidth = UINT( particles.size() * sizeof( Particle ) );
    descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    kl::dx::SubresourceDescriptor subresource_data{};
    subresource_data.pSysMem = particles.data();

    container_mesh = gpu.create_buffer( &descriptor, &subresource_data );
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
        particle.color = selected_texture.sample( { u, 1 - v } );
    }
}

void Particles::generate_particles()
{
    if ( selected_mesh_triangles.empty() )
    {
        particles.resize( 1'000'000 );
        std::for_each( std::execution::par, particles.begin(), particles.end(), [&]( auto& particle )
            {
                particle.home.x = kl::random::gen_float( -container_scale.x, container_scale.x );
                particle.home.y = kl::random::gen_float( -container_scale.y, container_scale.y );
                particle.home.z = kl::random::gen_float( -container_scale.z, container_scale.z );
                particle.position = particle.home;
                particle.velocity = kl::random::gen_float3( -0.1f, 0.1f );
                particle.color.x = ( particle.home.x + container_scale.x ) / ( 2 * container_scale.x );
                particle.color.y = ( particle.home.y + container_scale.y ) / ( 2 * container_scale.y );
                particle.color.z = ( particle.home.z + container_scale.z ) / ( 2 * container_scale.z );
            } );
        return;
    }

    particles.clear();
    particles.reserve( 1'000'000 );

    if ( use_wireframe )
    {
        for ( kl::Triangle const& triangle : selected_mesh_triangles ) {
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
