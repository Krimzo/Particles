// Compute shader
struct particle
{
    float3 position;
    float3 velocity;
    float3 color;
};

cbuffer cs_cb : register(b0)
{
    uint particle_count;
    float3 time_info; // (elapsed_t, delta_t, none)
    
    float4 gravity_ray_origin; // (origin, exists?)
    float4 gravity_ray_direction;
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

static const float object_scale = 1.0f;
static const float position_bias = 1e-3f;
static const float energy_retain = 0.7f;

particle update_particle(particle particle)
{
    // Gravity
    if (gravity_ray_origin.w) {
        const float distance_t = dot(particle.position - gravity_ray_origin.xyz, gravity_ray_direction.xyz);
        const float3 closest_position = gravity_ray_origin.xyz + gravity_ray_direction.xyz * distance_t;
        
        float3 acceleration = particle.position - closest_position;
        float gravity_distance = length(acceleration);
        
        acceleration /= (gravity_distance * gravity_distance);
        particle.velocity += (acceleration * time_info.y);
    }
    
    // Velocity
    particle.position += (particle.velocity * time_info.y);
    
    // Collision
    for (int i = 0; i < 3; i++) {
        float3 plane_normal = 0;
        plane_normal[i] = 1;
        
        if (particle.position[i] < -object_scale) {
            particle.position[i] = -(object_scale - position_bias);
            particle.velocity = reflect(particle.velocity, plane_normal) * energy_retain;
        }
        
        if (particle.position[i] > object_scale) {
            particle.position[i] = (object_scale - position_bias);
            particle.velocity = reflect(particle.velocity, -plane_normal) * energy_retain;
        }
    }
    
    return particle;
}
