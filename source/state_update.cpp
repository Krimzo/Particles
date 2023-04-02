#include "state.h"


void state::update_zoom()
{
	static int last_delta = 0;
	const int scroll_delta = window.mouse.scroll();

	if (scroll_delta != last_delta) {
		const int new_delta = (last_delta - scroll_delta);
		last_delta = scroll_delta;

		camera.speed += new_delta * 0.25f;
		camera.speed = max(camera.speed, 0.1f);
		reload_camera();
	}
}

void state::compute_physics()
{
	cs_cb cs_data = {};
	cs_data.particle_count = gpu.get_mesh_vertex_count(particles_mesh, sizeof(particle));
	cs_data.time_info = { timer.get_elapsed(), timer.get_interval(), 0.0f };

	if (window.mouse.left && !is_window_hovered) {
		const kl::float2 ndc = window.mouse.get_normalized_position();
		const kl::ray ray = { camera.origin, kl::inverse(camera.matrix()), ndc };
		cs_data.force_ray_origin = { ray.origin, 1.0f };
		cs_data.force_ray_direction = { ray.get_direction(), 0.0f };
	}

	cs_data.force_ray_direction.w = (float) return_home;
	cs_data.container_scale = { container_scale, 0.0f };
	cs_data.energy_info = { force_strength, energy_retain, 0.0f, 0.0f };

	gpu.bind_compute_shader(compute_shader);
	gpu.bind_cb_for_compute_shader(cs_const_buffer, 0);
	gpu.set_cb_data(cs_const_buffer, cs_data);

	gpu.bind_access_view_for_compute_shader(particles_view, 0);
	gpu.dispatch_compute_shader(cs_data.particle_count / 1024 + 1, 1, 1);
	gpu.unbind_access_view_for_compute_shader(0);
}

void state::render_particles()
{
	vs_cb vs_data = {};
	vs_data.VP = camera.matrix();

	gpu.bind_render_shaders(render_shaders);
	gpu.bind_cb_for_vertex_shader(vs_const_buffer, 0);
	gpu.set_cb_data(vs_const_buffer, vs_data);

	gpu.bind_mesh(particles_mesh, 0, 0, sizeof(particle));
	gpu.set_draw_type(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	gpu.draw(gpu.get_mesh_vertex_count(particles_mesh, sizeof(particle)), 0);

	gpu.bind_mesh(container_mesh, 0, 0, sizeof(particle));
	gpu.set_draw_type(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	gpu.draw(gpu.get_mesh_vertex_count(container_mesh, sizeof(particle)), 0);
}

void state::render_gui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

	if (ImGui::Begin("Scene")) {
		is_window_hovered = ImGui::GetIO().WantCaptureMouse;

		// Mesh
		ImGui::Text("Mesh");
		const size_t vertex_count = loaded_mesh.size();
		ImGui::Text(kl::format("Vertex count: ", vertex_count, " [", vertex_count * sizeof(kl::vertex) * 1e-6, " MB]").c_str());
		
		ImGui::DragFloat("Scaling", &mesh_scaling, 0.1f, -1e6f, 1e6f);
		ImGui::DragFloat3("Offset", mesh_offset, 0.1f, -1e6f, 1e6f);

		if (ImGui::Button("Reload##Mesh")) {
			reload_mesh();
		}
		ImGui::SameLine();

		mesh_path.resize(50);
		ImGui::InputText("##MeshInput", mesh_path.data(), mesh_path.size());

		// Texture
		ImGui::Separator();
		ImGui::Text("Texture");
		const size_t image_size = (size_t) loaded_texture.width() * loaded_texture.height();
		ImGui::Text(kl::format("Size: ", loaded_texture.size(), " [", image_size * sizeof(kl::color) * 1e-6, " MB]").c_str());

		if (ImGui::Button("Reload##Texture")) {
			reload_texture();
		}
		ImGui::SameLine();

		texture_path.resize(50);
		ImGui::InputText("##TextureInput", texture_path.data(), texture_path.size());

		// Particles
		ImGui::Separator();
		ImGui::Text("Particles");

		const size_t cpu_size = particles.size();
		const size_t gpu_size = gpu.get_mesh_vertex_count(particles_mesh, sizeof(particle));
		ImGui::Text(kl::format("CPU count: ", cpu_size, " [", cpu_size * sizeof(particle) * 1e-6, " MB]").c_str());
		ImGui::Text(kl::format("GPU count: ", gpu_size, " [", gpu_size * sizeof(particle) * 1e-6, " MB]").c_str());

		ImGui::DragFloat("Precision", &generation_precision, 0.0001f, 0.0001f, 0.1f, "%.4f");
		ImGui::Checkbox("Wireframe", &use_wireframe);
		ImGui::Checkbox("Exploded", &generate_exploded);
		
		if (ImGui::Button("Generate")) {
			generate_particles();
			create_particle_buffer();
			particles_view = gpu.create_access_view(particles_mesh, nullptr);
			return_home = false;
		}

		// Scene
		ImGui::Separator();
		ImGui::Text("Scene");

		if (ImGui::DragFloat3("Container scale", container_scale, 0.1f, 0.0f, 1e6f)) {
			create_container_buffer();
		}
		ImGui::DragFloat("Force strength", &force_strength, 0.1f);
		ImGui::DragFloat("Energy retain", &energy_retain, 0.1f, 0.0f, 1.0f);
		ImGui::Checkbox("Return home", &return_home);

		// Camera
		ImGui::Separator();
		ImGui::Text("Camera");
		
		kl::float3 background = camera.background;
		if (ImGui::ColorEdit3("Background", background)) {
			camera.background = background;
			create_container_buffer();
		}

		ImGui::Text(kl::format("Origin: ", camera.origin).c_str());
		ImGui::Text(kl::format("Direction: ", camera.get_forward()).c_str());
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
