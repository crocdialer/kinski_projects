typedef struct Params
{
    float4 emitter_position;
    float4 gravity;
    float4 velocity_min, velocity_max;
    float rotation_matrix[9];
    float bouncyness;
    float life_min, life_max;
    bool debug_life;
    unsigned int num_alive;
}Params;

// typedef struct ParticleState
// {
//     __global float3* pos;
//     __global float4* color;
//     __global float* point_sizes;
//     __global float4* vel;
//     __global float4* pos_gen;
// }ParticleState;

inline void swap_indices(__global unsigned int* lhs, __global unsigned int* rhs)
{
    unsigned int tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

__constant float inverse_int_max = 1.0 / 4294967295.0;

// col-major mat3x3 <-> vec3 multiplication
inline float3 matrix_mult_3x3(__constant float* the_mat, float3 the_vec)
{
    float3 ret;
    ret.x = dot((float3){the_mat[0], the_mat[3], the_mat[6]}, the_vec);
    ret.y = dot((float3){the_mat[1], the_mat[4], the_mat[7]}, the_vec);
    ret.z = dot((float3){the_mat[2], the_mat[5], the_mat[8]}, the_vec);
    return ret;
}

// transforms even the sequence 0,1,2,3,... into reasonably good random numbers
// challenge: improve on this in speed and "randomness"!
inline unsigned int randhash(unsigned int seed)
{
   unsigned int i=(seed^12345391u)*2654435769u;
   i^=(i<<6)^(i>>26);
   i*=2654435769u;
   i+=(i<<5)^(i>>12);
   return i;
}

float random(uint seed)
{
    //randhash(seed);//(*seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
    return randhash(seed) * inverse_int_max;
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

float3 linear_rand(float3 lhs, float3 rhs, uint seed)
{
    return (float3)(mix(lhs.x, rhs.x, random(seed++)),
                    mix(lhs.y, rhs.y, random(seed++)),
                    mix(lhs.z, rhs.z, random(seed++)));
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
                            __global unsigned int* indices,
                            __constant float4* force_positions,
                            int num_forces,
                            float dt,
                            __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = indices[get_global_id(0)];

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
                                __global unsigned int* indices,
                                __constant float4* planes,
                                int num_planes,
                                __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = indices[get_global_id(0)];

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

__kernel void spawn_particle(__global float3* pos,
                             __global float4* color,
                             __global float* point_sizes,
                             __global float4* vel,
                             __global float4* pos_gen,
                             __global unsigned int* indices,
                             __constant struct Params *params)
{
    //spawning

    //get our index in the array
    unsigned int i = indices[get_global_id(0)];

    __global float3 *p = pos + i;
    __global float4 *v = vel + i;

    // Get the global id in 1D
    uint seed = get_global_id(1) * get_global_size(0) + get_global_id(0) + dot(*p, *p);

    *p = pos_gen[i].xyz + params->emitter_position.xyz;
    float3 rnd_vel = linear_rand(params->velocity_min.xyz, params->velocity_max.xyz, seed++);
    v->xyz = matrix_mult_3x3(params->rotation_matrix, rnd_vel);
    v->w = mix(params->life_min, params->life_max, random(seed++));
}

__kernel void kill_particles(__global float4* vel,
                             __global unsigned int* indices,
                             __global struct Params *params)
{
    float life = vel[indices[get_global_id(0)]].w;

    //if the life is 0 or less we swap this particle's index out
    if(life <= 0)
    {
        //particle dies -> decrease global counter
        int old_max = atomic_dec(&params->num_alive) - 1;

        // swap with last element in active range
        if(old_max > 0){ swap_indices(indices + get_global_id(0), indices + old_max); }
    }
}

__kernel void update_particles(__global float3* pos,
                               __global float4* color,
                               __global float* point_sizes,
                               __global float4* vel,
                               __global float4* pos_gen,
                               __global unsigned int* indices,
                               float dt,
                               __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = indices[get_global_id(0)];

    //copy position and velocity for this iteration to a local variable
    //note: if we were doing many more calculations we would want to have opencl
    //copy to a local memory array to speed up memory access (this will be the subject of a later tutorial)
    float3 p = pos[i];
    float4 v = vel[i];

    // life is stored in the fourth component of our velocity array
    float life = max(vel[i].w - dt, 0.f);

    //update the position with the velocity
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
        //ratio = get_global_id(0) / (float)get_global_size(0);
        color[i] = jet(ratio);
        color[i].w = ratio;
    }
}
