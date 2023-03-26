// Compute shader
struct particle
{
    float3 home;
    float3 position;
    float3 velocity;
    float3 color;
};

cbuffer cs_cb : register(b0)
{
    uint particle_count;
    float3 time_info; // (elapsed_t, delta_t, none)
    
    float4 force_ray_origin;    // (origin, use_force?)
    float4 force_ray_direction; // (direction, return_home?)
    
    float4 container_scale; // (scale, none)
    float4 energy_info;     // (force_strength, energy_retain, none, none)
};

RWStructuredBuffer<particle> particles : register(u0);

particle update_particle(particle particle);

[numthreads(1024, 1, 1)]
void c_shader(uint3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x < particle_count) {
        particles[thread_id.x] = update_particle(particles[thread_id.x]);
    }
}

particle update_particle(particle particle)
{
    // Return home
    if (force_ray_direction.w) {
        particle.velocity = (particle.home - particle.position);
    }
    
    // Force
    if (force_ray_origin.w) {
        const float distance_t = dot(particle.position - force_ray_origin.xyz, force_ray_direction.xyz);
        const float3 closest_position = force_ray_origin.xyz + force_ray_direction.xyz * distance_t;
        
        float3 acceleration = particle.position - closest_position;
        float force_distance = length(acceleration);
        
        acceleration /= (force_distance * force_distance);
        acceleration *= energy_info.x;
        
        particle.velocity += (acceleration * time_info.y);
    }
    
    // Velocity
    particle.position += (particle.velocity * time_info.y);
    
    // Collision
    for (int i = 0; i < 3; i++) {
        float3 plane_normal = 0;
        plane_normal[i] = 1;
        
        if (particle.position[i] < -container_scale[i]) {
            particle.position[i] = -(container_scale[i] - 1e-3f);
            particle.velocity = reflect(particle.velocity, plane_normal) * energy_info.y;
        }
        
        if (particle.position[i] > container_scale[i]) {
            particle.position[i] = (container_scale[i] - 1e-3f);
            particle.velocity = reflect(particle.velocity, -plane_normal) * energy_info.y;
        }
    }
    
    return particle;
}
