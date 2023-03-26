#pragma once

#include "klib.h"

#include "imgui.h"
#include "ImGuizmo.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


struct particle
{
	kl::float3 home = {};
	kl::float3 position = {};
	kl::float3 velocity = {};
	kl::float3 color = {};
};

struct vs_cb
{
	kl::float4x4 VP = {};
};

struct cs_cb
{
	uint32_t particle_count = 0;
	kl::float3 time_info = {}; // (elapsed_t, delta_t, none)

	kl::float4 force_ray_origin = {};    // (origin, use_force?)
	kl::float4 force_ray_direction = {}; // (direction, return_home?)

	kl::float4 container_scale = {}; // (scale, none)
	kl::float4 energy_info = {};     // (force_strength, energy_retain, none, none)
};
