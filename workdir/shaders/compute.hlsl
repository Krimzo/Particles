struct Particle
{
    float3 home;
    float3 position;
    float3 velocity;
    float3 color;
};

uint PARTICLE_COUNT;
float3 TIME_INFO;           // (elapsed_t, delta_t, none)
float4 FORCE_RAY_ORIGIN;    // (origin, use_force?)
float4 FORCE_RAY_DIRECTION; // (direction, return_home?)
float4 CONTAINER_SCALE;     // (scale, none)
float4 ENERGY_INFO;         // (force_strength, energy_retain, none, none)

RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(1024, 1, 1)]
void c_shader(uint3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x >= PARTICLE_COUNT)
        return;
    
    Particle particle = particles[thread_id.x];
    
    if (FORCE_RAY_DIRECTION.w)
        particle.velocity = (particle.home - particle.position);
    
    if (FORCE_RAY_ORIGIN.w)
    {
        const float distance_t = dot(particle.position - FORCE_RAY_ORIGIN.xyz, FORCE_RAY_DIRECTION.xyz);
        const float3 closest_position = FORCE_RAY_ORIGIN.xyz + FORCE_RAY_DIRECTION.xyz * distance_t;
        float3 acceleration = particle.position - closest_position;
        float force_distance = length(acceleration);
        acceleration /= (force_distance * force_distance);
        acceleration *= ENERGY_INFO.x;
        particle.velocity += (acceleration * TIME_INFO.y);
    }
    
    particle.position += (particle.velocity * TIME_INFO.y);
    
    for (int i = 0; i < 3; i++)
    {
        float3 plane_normal = 0;
        plane_normal[i] = 1.0f;
        if (particle.position[i] < -CONTAINER_SCALE[i])
        {
            particle.position[i] = 1e-3f - CONTAINER_SCALE[i];
            particle.velocity = reflect(particle.velocity, plane_normal) * ENERGY_INFO.y;
        }
        if (particle.position[i] > CONTAINER_SCALE[i])
        {
            particle.position[i] = CONTAINER_SCALE[i] - 1e-3f;
            particle.velocity = reflect(particle.velocity, -plane_normal) * ENERGY_INFO.y;
        }
    }
}
