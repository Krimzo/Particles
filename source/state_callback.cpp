#include "state.h"


void state::on_window_resize(const kl::int2 new_size)
{
	if (new_size.x > 0 && new_size.y > 0) {
		gpu.resize_internal(new_size);
		gpu.set_viewport_size(new_size);
		camera.update_aspect_ratio(new_size);
	}
}

void state::reload_camera()
{
	camera.set_forward(camera.origin * -1.0f);
	camera.origin = (camera.get_forward() * -camera.speed);
}

void state::on_camera_start()
{
	start_mouse_position = window.mouse.position();
	start_rotations = total_rotations;
}

void state::on_camera_update()
{
	const kl::int2 delta_mouse = (window.mouse.position() - start_mouse_position);
	start_mouse_position = window.mouse.position();

	total_rotations.x = start_rotations.x + (delta_mouse.x * camera.sensitivity);
	total_rotations.y = start_rotations.y + (delta_mouse.y * camera.sensitivity);
	total_rotations.y = min(max(total_rotations.y, -1.5f), 1.5f);
	start_rotations = total_rotations;

	camera.origin.x = sin(total_rotations.x);
	camera.origin.z = cos(total_rotations.x);
	camera.origin.y = tan(total_rotations.y);

	reload_camera();
}
