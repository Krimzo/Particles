#include "state.h"


void state::reload_mesh()
{
	std::vector<kl::vertex> vertices = kl::files::parse_mesh(mesh_path, true);
	for (auto& vertex : vertices) {
		vertex.world *= mesh_scaling;
		vertex.world += mesh_offset;
	}

	loaded_mesh.resize(vertices.size() / 3);
	memcpy(loaded_mesh.data(), vertices.data(), loaded_mesh.size() * sizeof(kl::triangle));
}

void state::reload_texture()
{
	loaded_texture = kl::image(texture_path);
}

void state::generate_particle_line(const kl::triangle& triangle, const kl::float3& start, const kl::float3 end)
{
	const float walk_distance = (end - start).length();
	const int step_count = (int) (walk_distance / generation_precision);
	const kl::float3 walk_direction = kl::math::normalize(end - start);

	for (int i = 0; i <= step_count; i++) {
		particle particle = {};
		particle.home = start + walk_direction * (i * generation_precision);
		particle.position = particle.home;

		if (generate_exploded) {
			particle.velocity = kl::random::get_float3(-0.25f, 0.25f);
		}

		const auto weights = triangle.get_weights(particle.position);
		const float u = kl::triangle::interpolate(weights, { triangle.a.texture.x, triangle.b.texture.x, triangle.c.texture.x });
		const float v = kl::triangle::interpolate(weights, { triangle.a.texture.y, triangle.b.texture.y, triangle.c.texture.y });
		particle.color = loaded_texture.sample({ u, 1 - v });

		particles.push_back(particle);
	}
}

void state::generate_particles()
{
	if (loaded_mesh.empty()) {
		particles.resize(1'000'000);
		std::for_each(std::execution::par, particles.begin(), particles.end(), [&](auto& particle)
		{
			particle.home.x = kl::random::get_float(-container_scale.x, container_scale.x);
			particle.home.y = kl::random::get_float(-container_scale.y, container_scale.y);
			particle.home.z = kl::random::get_float(-container_scale.z, container_scale.z);
			particle.position = particle.home;
			particle.velocity = kl::random::get_float3(-0.1f, 0.1f);
			particle.color.x = (particle.home.x + container_scale.x) / (2 * container_scale.x);
			particle.color.y = (particle.home.y + container_scale.y) / (2 * container_scale.y);
			particle.color.z = (particle.home.z + container_scale.z) / (2 * container_scale.z);
		});
		return;
	}

	particles.clear();
	particles.reserve(1'000'000);

	if (use_wireframe) {
		for (auto& triangle : loaded_mesh) {
			generate_particle_line(triangle, triangle.a.world, triangle.b.world);
			generate_particle_line(triangle, triangle.b.world, triangle.c.world);
			generate_particle_line(triangle, triangle.c.world, triangle.a.world);
		}
	}
	else {
		for (auto& triangle : loaded_mesh) {
			const float a_walk_distance = (triangle.c.world - triangle.a.world).length();
			const float b_walk_distance = (triangle.c.world - triangle.b.world).length();

			const int a_step_count = (int) (a_walk_distance / generation_precision);
			const int b_step_count = (int) (b_walk_distance / generation_precision);
			const int step_count = min(a_step_count, b_step_count);

			const kl::float3 a_walk_direction = kl::math::normalize(triangle.c.world - triangle.a.world);
			const kl::float3 b_walk_direction = kl::math::normalize(triangle.c.world - triangle.b.world);

			for (int i = 0; i <= step_count; i++) {
				const kl::float3 a_walk_point = triangle.a.world + a_walk_direction * (i * generation_precision);
				const kl::float3 b_walk_point = triangle.b.world + b_walk_direction * (i * generation_precision);
				generate_particle_line(triangle, a_walk_point, b_walk_point);
			}
		}
	}
}

void state::create_particle_buffer()
{
	kl::dx::buffer_descriptor descriptor = {};
	descriptor.Usage = D3D11_USAGE_DEFAULT;
	descriptor.StructureByteStride = sizeof(particle);
	descriptor.ByteWidth = (UINT) (particles.size() * sizeof(particle));
	descriptor.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	descriptor.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

	kl::dx::subresource_descriptor subresource_data = {};
	subresource_data.pSysMem = particles.data();

	particles_mesh = gpu.create_buffer(&descriptor, &subresource_data);
}

void state::create_container_buffer()
{
	std::vector<particle> particles = {
		{ {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
		{ {}, {  container_scale.x, -container_scale.y, -container_scale.z }, {}, kl::color(camera.background.r, 0, 0).inverted() },

		{ {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
		{ {}, { -container_scale.x,  container_scale.y, -container_scale.z }, {}, kl::color(0, camera.background.g, 0).inverted() },

		{ {}, { -container_scale.x, -container_scale.y, -container_scale.z }, {}, camera.background },
		{ {}, { -container_scale.x, -container_scale.y,  container_scale.z }, {}, kl::color(0, 0, camera.background.b).inverted() },
	};

	kl::dx::buffer_descriptor descriptor = {};
	descriptor.Usage = D3D11_USAGE_DEFAULT;
	descriptor.ByteWidth = (UINT) (particles.size() * sizeof(particle));
	descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	kl::dx::subresource_descriptor subresource_data = {};
	subresource_data.pSysMem = particles.data();

	container_mesh = gpu.create_buffer(&descriptor, &subresource_data);
}
