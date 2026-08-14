struct Particle
{
    float3 home;
    float3 position;
    float3 velocity;
    float3 color;
};

static const float AT_HOME_BIAS = 0.01f;

float3 FORCE_RAY_ORIGIN;
float USE_RAY_FORCE;
float3 FORCE_RAY_DIRECTION;
float FORCE_STRENGTH;
float3 CONTAINER_SCALE;
float RETURN_HOME;
float RETURN_HOME_VELOCITY;
float ENERGY_RETAIN;
float ELAPSED_TIME;
float DELTA_TIME;
uint PARTICLE_COUNT;

RWStructuredBuffer<Particle> PARTICLES : register(u0);

[numthreads(1024, 1, 1)]
void c_shader(uint3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x >= PARTICLE_COUNT)
        return;
    
    Particle particle = PARTICLES[thread_id.x];
    
    if (RETURN_HOME)
    {
        if (distance(particle.position, particle.home) <= AT_HOME_BIAS)
        {
            particle.position = particle.home;
            particle.velocity = 0.0f;
        }
        else
            particle.velocity = normalize(particle.home - particle.position) * RETURN_HOME_VELOCITY;
    }
    
    if (USE_RAY_FORCE)
    {
        const float distance_t = dot(particle.position - FORCE_RAY_ORIGIN, FORCE_RAY_DIRECTION);
        const float3 closest_position = FORCE_RAY_ORIGIN + FORCE_RAY_DIRECTION * distance_t;
        float3 acceleration = particle.position - closest_position;
        const float force_distance = length(acceleration);
        acceleration /= force_distance * force_distance;
        acceleration *= FORCE_STRENGTH;
        particle.velocity += acceleration * DELTA_TIME;
    }
    
    particle.position += particle.velocity * DELTA_TIME;
    
    for (int i = 0; i < 3; i++)
    {
        float3 plane_normal = 0;
        plane_normal[i] = 1.0f;
        if (particle.position[i] < -CONTAINER_SCALE[i])
        {
            particle.position[i] = 1e-3f - CONTAINER_SCALE[i];
            particle.velocity = reflect(particle.velocity, plane_normal) * ENERGY_RETAIN;
        }
        if (particle.position[i] > CONTAINER_SCALE[i])
        {
            particle.position[i] = CONTAINER_SCALE[i] - 1e-3f;
            particle.velocity = reflect(particle.velocity, -plane_normal) * ENERGY_RETAIN;
        }
    }
    
    PARTICLES[thread_id.x] = particle;
}
