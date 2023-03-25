#include "klib.h"


// Custom structs
struct particle
{
	kl::float3 position = {};
	kl::float3 velocity = {};
	kl::float3 color = {};
};

// Const-Buffer structs
struct vs_cb
{
	kl::float4x4 VP = {};
};

struct cs_cb
{
	uint32_t particle_count = 0;
	kl::float3 time_info = {}; // (elapsed_t, delta_t, none)

	kl::float4 gravity_ray_origin = {}; // (origin, exists?)
	kl::float4 gravity_ray_direction = {};
};

// System
static kl::window window = { { 1600, 900 }, "Particle Renderer" };
static kl::gpu gpu = { (HWND) window };

// Utility
static kl::timer timer = {};
static kl::camera camera = {};

// Helpers
static const std::string mesh_path = "meshes/table.obj";
static const std::string texture_path = "textures/table.jpg";

static std::vector<kl::triangle> loaded_mesh = {};
static kl::image loaded_texture = {};

static kl::int2 start_mouse_position = {};
static kl::float2 start_rotations = {};
static kl::float2 total_rotations = {};

static bool generate_exploded = false;
static const float walk_increment = 0.01f;

// Init
void reload_resources(float scaling, const kl::float3& offset);
void generate_particle_line(std::vector<particle>& particles, const kl::float3& start, const kl::float3 end);
std::vector<particle> generate_particles(bool generate_from_mesh, bool use_wireframe);

kl::dx::buffer create_particle_buffer(const std::vector<particle>& particles);
kl::dx::buffer create_container_buffer();
kl::render_shaders create_render_shaders();

// Update
void reload_camera();
void update_zoom(int scroll_delta);

// Callbacks
void on_window_resize(kl::int2 new_size);
void on_camera_start();
void on_camera_update();

// Entry
int main()
{
	// Callbacks
	window.on_resize.push_back(on_window_resize);
	window.mouse.right.on_press.push_back(on_camera_start);
	window.mouse.right.on_down.push_back(on_camera_update);
	window.maximize();

	// Utility
	camera.speed = 3.0f;
	camera.sensitivity = 0.01f;
	camera.background = kl::color(19, 22, 27);
	on_camera_update();

	// Load resources
	reload_resources(1.25f, { 0.0f, -0.5f, 0.0f });

	// Buffers
	kl::dx::buffer particles_mesh = nullptr;
	kl::dx::buffer container_mesh = create_container_buffer();
	kl::dx::buffer vs_const_buffer = gpu.create_const_buffer(sizeof(vs_cb));
	kl::dx::buffer cs_const_buffer = gpu.create_const_buffer(sizeof(cs_cb));

	// Views
	kl::dx::access_view particles_view = nullptr;

	// Shaders
	kl::render_shaders render_shaders = create_render_shaders();
	kl::dx::compute_shader compute_shader = gpu.create_compute_shader(kl::files::read_string("shaders/compute.hlsl"));

	window.keyboard.space.on_press.push_back([&]
	{
		const auto particles = generate_particles(true, false);
		particles_mesh = create_particle_buffer(particles);
		particles_view = gpu.create_access_view(particles_mesh, nullptr);
	});
	window.keyboard.space.on_press.front()();

	// Loop
	while (window.process(false)) {
		// Utility
		timer.update_interval();
		update_zoom(window.mouse.scroll());

		// Prepare
		gpu.clear_internal(camera.background);
		const auto particle_count = gpu.get_mesh_vertex_count(particles_mesh, sizeof(particle));
		const auto box_line_count = gpu.get_mesh_vertex_count(container_mesh, sizeof(particle));

		// Physics call
		cs_cb cs_data = {};
		cs_data.particle_count = particle_count;
		cs_data.time_info = { timer.get_elapsed(), timer.get_interval(), 0.0f };

		if (window.mouse.left) {
			kl::float2 ndc = window.mouse.get_normalized_position();
			kl::ray ray = { camera.origin, kl::math::inverse(camera.matrix()), ndc };
			cs_data.gravity_ray_origin = { ray.origin, 1.0f };
			cs_data.gravity_ray_direction = { ray.direction, 0.0f };
			generate_exploded = false;
		}

		gpu.bind_compute_shader(compute_shader);
		gpu.bind_cb_for_compute_shader(cs_const_buffer, 0);
		gpu.set_cb_data(cs_const_buffer, cs_data);

		gpu.bind_access_view_for_compute_shader(particles_view, 0);
		gpu.dispatch_compute_shader(particle_count / 1024 + 1, 1, 1);
		gpu.unbind_access_view_for_compute_shader(0);

		// Render call
		vs_cb vs_data = {};
		vs_data.VP = camera.matrix();

		gpu.bind_render_shaders(render_shaders);
		gpu.bind_cb_for_vertex_shader(vs_const_buffer, 0);
		gpu.set_cb_data(vs_const_buffer, vs_data);

		gpu.bind_mesh(particles_mesh, 0, 0, sizeof(particle));
		gpu.set_draw_type(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		gpu.draw(particle_count, 0);

		gpu.bind_mesh(container_mesh, 0, 0, sizeof(particle));
		gpu.set_draw_type(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
		gpu.draw(box_line_count, 0);

		// Display
		gpu.swap_buffers(false);
	}
}

// Init
void reload_resources(const float scaling, const kl::float3& offset)
{
	// Mesh
	std::vector<kl::vertex> vertices = kl::files::parse_mesh(mesh_path, true);
	for (auto& vertex : vertices) {
		vertex.world *= scaling;
		vertex.world += offset;
	}

	loaded_mesh.resize(vertices.size() / 3);
	memcpy(loaded_mesh.data(), vertices.data(), loaded_mesh.size() * sizeof(kl::triangle));

	// Texture
	loaded_texture = kl::image(texture_path);
}

void generate_particle_line(std::vector<particle>& particles, const kl::float3& start, const kl::float3 end)
{
	const float walk_distance = (end - start).length();
	const int step_count = (int) (walk_distance / walk_increment);
	const kl::float3 walk_direction = kl::math::normalize(end - start);

	for (int i = 0; i <= step_count; i++) {
		particle particle = {};
		particle.position = start + walk_direction * (i * walk_increment);

		if (generate_exploded) {
			particle.velocity = kl::random::get_float3(-0.25f, 0.25f);
		}

		particle.color = (particle.position + kl::float3(1.0f)) * 0.5f;
		particles.push_back(particle);
	}
}

std::vector<particle> generate_particles(const bool generate_from_mesh, const bool use_wireframe)
{
	if (!generate_from_mesh) {
		std::vector<particle> particles(500'000);
		std::for_each(std::execution::par, particles.begin(), particles.end(), [](auto& particle)
		{
			particle.position = kl::random::get_float3(-1.0f, 1.0f);
			particle.velocity = kl::random::get_float3(-0.1f, 0.1f);
			particle.color = (particle.position + kl::float3(1.0f)) * 0.5f;
		});
		return particles;
	}

	std::vector<particle> particles = {};
	particles.reserve(1'000'000);

	if (use_wireframe) {
		for (auto& triangle : loaded_mesh) {
			generate_particle_line(particles, triangle.a.world, triangle.b.world);
			generate_particle_line(particles, triangle.b.world, triangle.c.world);
			generate_particle_line(particles, triangle.c.world, triangle.a.world);
		}
	}
	else {
		for (auto& triangle : loaded_mesh) {
			const float a_walk_distance = (triangle.c.world - triangle.a.world).length();
			const float b_walk_distance = (triangle.c.world - triangle.b.world).length();

			const int a_step_count = (int) (a_walk_distance / walk_increment);
			const int b_step_count = (int) (b_walk_distance / walk_increment);
			const int step_count = min(a_step_count, b_step_count);

			const kl::float3 a_walk_direction = kl::math::normalize(triangle.c.world - triangle.a.world);
			const kl::float3 b_walk_direction = kl::math::normalize(triangle.c.world - triangle.b.world);

			for (int i = 0; i <= step_count; i++) {
				const kl::float3 a_walk_point = triangle.a.world + a_walk_direction * (i * walk_increment);
				const kl::float3 b_walk_point = triangle.b.world + b_walk_direction * (i * walk_increment);
				generate_particle_line(particles, a_walk_point, b_walk_point);
			}
		}
	}

	generate_exploded = !generate_exploded;
	return particles;
}

kl::dx::buffer create_particle_buffer(const std::vector<particle>& particles)
{
	// Load particles to GPU
	kl::dx::buffer_descriptor descriptor = {};
	descriptor.Usage = D3D11_USAGE_DEFAULT;
	descriptor.StructureByteStride = sizeof(particle);
	descriptor.ByteWidth = (UINT) (particles.size() * sizeof(particle));
	descriptor.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	descriptor.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

	kl::dx::subresource_descriptor subresource_data = {};
	subresource_data.pSysMem = particles.data();

	return gpu.create_buffer(&descriptor, &subresource_data);
}

kl::dx::buffer create_container_buffer()
{
	std::vector<particle> particles = {
		{ { -1.0f, -1.0f, -1.0f }, {}, kl::float3(1.0f) },
		{ { -1.0f, -1.0f,  1.0f }, {}, kl::float3(1.0f) },
		{ {  1.0f, -1.0f,  1.0f }, {}, kl::float3(1.0f) },
		{ {  1.0f, -1.0f, -1.0f }, {}, kl::float3(1.0f) },
		{ { -1.0f, -1.0f, -1.0f }, {}, kl::float3(1.0f) },
	};

	kl::dx::buffer_descriptor descriptor = {};
	descriptor.Usage = D3D11_USAGE_DEFAULT;
	descriptor.ByteWidth = (UINT) (particles.size() * sizeof(particle));
	descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	kl::dx::subresource_descriptor subresource_data = {};
	subresource_data.pSysMem = particles.data();

	return gpu.create_buffer(&descriptor, &subresource_data);
}

kl::render_shaders create_render_shaders()
{
	const std::vector<kl::dx::layout_descriptor> layout_descriptors = {
		{ "KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "KL_Velocity", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{    "KL_Color", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const std::string shader_source = kl::files::read_string("shaders/render.hlsl");
	const kl::compiled_shader compiled_vertex_shader = gpu.compile_vertex_shader(shader_source);
	const kl::compiled_shader compiled_pixel_shader = gpu.compile_pixel_shader(shader_source);

	kl::render_shaders shaders = {};
	shaders.input_layout = gpu.create_input_layout(compiled_vertex_shader, layout_descriptors);
	shaders.vertex_shader = gpu.device_holder::create_vertex_shader(compiled_vertex_shader);
	shaders.pixel_shader = gpu.device_holder::create_pixel_shader(compiled_pixel_shader);
	return shaders;
}

// Update
void reload_camera()
{
	camera.set_forward(camera.origin * -1.0f);
	camera.origin = (camera.get_forward() * -camera.speed);
}

void update_zoom(const int scroll_delta)
{
	static int last_delta = 0;
	if (scroll_delta != last_delta) {
		const int new_delta = (last_delta - scroll_delta);
		last_delta = scroll_delta;

		camera.speed += new_delta * 0.25f;
		camera.speed = max(camera.speed, kl::float3(1.0f).length());
		reload_camera();
	}
}

// Callback
void on_window_resize(const kl::int2 new_size)
{
	if (new_size.x > 0 && new_size.y > 0) {
		gpu.resize_internal(new_size);
		gpu.set_viewport_size(new_size);
		camera.update_aspect_ratio(new_size);
	}
}

void on_camera_start()
{
	start_mouse_position = window.mouse.position();
	start_rotations = total_rotations;
}

void on_camera_update()
{
	const kl::int2 delta_mouse = (window.mouse.position() - start_mouse_position);
	start_mouse_position = window.mouse.position();
	
	total_rotations.x = start_rotations.x + (delta_mouse.x * camera.sensitivity);
	total_rotations.y = start_rotations.y + (delta_mouse.y * camera.sensitivity);
	total_rotations.y = min(max(total_rotations.y, 0.0f), 1.0f);
	start_rotations = total_rotations;

	camera.origin.x = sin(total_rotations.x);
	camera.origin.z = cos(total_rotations.x);
	camera.origin.y = total_rotations.y;

	reload_camera();
}
