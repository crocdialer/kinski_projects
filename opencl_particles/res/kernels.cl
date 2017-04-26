typedef struct Params
{
    float4 emitter_position;
    float4 gravity;
    float4 velocity_min, velocity_max;
    float bouncyness;
    float life_min, life_max;
    bool debug_life;
}Params;

float random(uint* seed)
{
    uint val = (*seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
    *seed = val;
    return val / (float)(0xffffffff);
}

inline float4 jet(float val)
{
    return (float4)(min(4.0f * val - 1.5f, -4.0f * val + 4.5f),
                    min(4.0f * val - 0.5f, -4.0f * val + 3.5f),
                    min(4.0f * val + 0.5f, -4.0f * val + 2.5f),
                    1.0f);
}

inline float3 reflect(float3 v, float3 n)
{
	return v - 2.0f * dot(v, n) * n;
}

float3 linear_rand(float3 lhs, float3 rhs, uint* seed)
{
    return (float3)(mix(lhs.x, rhs.x, random(seed)),
                    mix(lhs.y, rhs.y, random(seed)),
                    mix(lhs.z, rhs.z, random(seed)));
}

inline void apply_plane_contraint(const float4* the_plane, float3* the_pos, float4* the_velocity,
                                  float bouncyness)
{
    // distance to plane
    float dist = dot(*the_pos, the_plane->xyz) + the_plane->w;

    if(dist < 0)
    {
        // set new position
        *the_pos = *the_pos - the_plane->xyz * dist;

        the_velocity->xyz = bouncyness * reflect(the_velocity->xyz, the_plane->xyz);
    }
}

inline float3 create_radial_force(float3 pos, float3 pos_particle, float strength)
{
    float3 dir = pos_particle - pos;
    float dist2 = dot(dir, dir);
    dir = normalize(dir);
    return strength * dir / (1.f + dist2);
}

__kernel void set_colors_from_image(image2d_t image, __global float3* pos, __global float4* color)
{
    unsigned int i = get_global_id(0);
    int w = get_image_width(image);
    int h = get_image_height(image);

    int2 coords = {pos[i].x + w/2, pos[i].z + h/2};
    //color[i] = read_imagef(image, CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE, coords);
    color[i] = read_imagef(image, coords);
}

// apply forces and change velocities
__kernel void apply_forces( __global float3* pos,
                            __global float4* vel,
                            __constant float4* force_positions,
                            int num_forces,
                            float dt,
                            __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = get_global_id(0);

    float3 p = pos[i];

    // add up all forces
    float3 cumulative_force = params->gravity.xyz;

    for(int j = 0; j < num_forces; ++j)
    {
        float3 force_pos = force_positions[j].xyz;
        float force_strength = force_positions[j].w;
        float3 force = create_radial_force(force_pos, p, force_strength);

        // force always downwards
        force.y *= cumulative_force.y < 0;

        cumulative_force += force;
    }
    // change velocity here
    vel[i] += (float4)(cumulative_force, 0) * dt;
}

__kernel void apply_contraints( __global float3* pos,
                                __global float4* velocity,
                                __constant float4* planes,
                                int num_planes,
                                __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = get_global_id(0);

    float3 p = pos[i];
    float4 v = velocity[i];

    for(int j = 0; j < num_planes; ++j)
    {
        float4 plane = planes[j];
        apply_plane_contraint(&plane, &p, &v, params->bouncyness);
    }

    pos[i] = p;
    velocity[i] = v;
}

__kernel void update_particles(__global float3* pos,
                               __global float4* color,
                               __global float* point_sizes,
                               __global float4* vel,
                               __global float4* pos_gen,
                               __global float4* vel_gen,
                               float dt,
                               __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = get_global_id(0);

    //copy position and velocity for this iteration to a local variable
    //note: if we were doing many more calculations we would want to have opencl
    //copy to a local memory array to speed up memory access (this will be the subject of a later tutorial)
    float3 p = pos[i];
    float4 v = vel[i];

    //we've stored the life in the fourth component of our velocity array
    float life = vel[i].w;

    //decrease the life by the time step (this value could be adjusted to lengthen or shorten particle life
    life -= dt;

    //if the life is 0 or less we reset the particle's values back to the original values and set life to 1
    if(life <= 0)
    {
        // Get the global id in 1D
        uint seed = (uint)(fabs(p.x) * get_global_id(1) * get_global_size(0) + get_global_id(0));

        p = pos_gen[i].xyz + params->emitter_position.xyz;
        //v = vel_gen[i];
        v.xyz = linear_rand(params->velocity_min.xyz, params->velocity_max.xyz, &seed);
        life = mix(params->life_min, params->life_max, random(&seed));//vel_gen[i].w;
    }

    //update the position with the new velocity
    p.xyz += v.xyz * dt;

    //store the updated life in the velocity array
    v.w = life;

    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;

    if(params->debug_life)
    {
        // color code remaining lifetime
        float ratio = life / params->life_max;
        color[i] = jet(ratio);
        color[i].w = ratio;
    }
}