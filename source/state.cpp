#include "state.h"


state::state()
{
	// Window
	window.on_resize.push_back([&](kl::int2 new_size) { on_window_resize(new_size); });
	window.maximize();

	// Mouse
	window.mouse.right.on_press.push_back([&] { if (!is_window_hovered) on_camera_start(); });
	window.mouse.right.on_down.push_back([&] { if (!is_window_hovered) on_camera_update(); });

	// Camera
	camera.speed = 3.0f;
	camera.sensitivity = 0.01f;
	camera.background = kl::color(50, 50, 50);
	on_camera_update();

	// Buffers
	create_container_buffer();
	vs_const_buffer = gpu.create_const_buffer(sizeof(vs_cb));
	cs_const_buffer = gpu.create_const_buffer(sizeof(cs_cb));

	// Shaders
	create_render_shaders();
	compute_shader = gpu.create_compute_shader(kl::files::read_string("shaders/compute.hlsl"));

	// GUI
	setup_gui();
}

bool state::process()
{
	return window.process(false);
}

void state::on_update()
{
	// Prepare
	timer.update_interval();
	update_zoom();

	// Calls
	gpu.clear_internal(camera.background);
	compute_physics();
	render_particles();
	render_gui();

	// Finalize
	gpu.swap_buffers(false);
}
