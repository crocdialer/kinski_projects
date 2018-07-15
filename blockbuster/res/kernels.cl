typedef struct
{
    int num_cols, num_rows, mirror, border;
    float depth_min, depth_max, depth_multiplier;
    float smooth_fall, smooth_rise;
    float z_min, z_max;
}param_t;

// debug coloring
inline float4 jet(float val)
{
    return (float4)(min(4.0f * val - 1.5f, -4.0f * val + 4.5f),
                    min(4.0f * val - 0.5f, -4.0f * val + 3.5f),
                    min(4.0f * val + 0.5f, -4.0f * val + 2.5f),
                    1.0f);
}

inline float4 gray(float4 color)
{
    float y_val = dot(color.xyz, (float3)(0.299, 0.587, 0.114));
    return (float4)(y_val, y_val, y_val, color.w);
}

inline float map_value(const float val, const float src_min, const float src_max,
                       const float dst_min, const float dst_max)
{
    float mix_val = clamp((val - src_min) / (src_max - src_min), 0.f, 1.f);
    return mix(dst_min, dst_max, mix_val);
}

__kernel void texture_input(read_only image2d_t the_img,
                            __global float4* pos_gen,
                            __constant param_t *p,
                            int is_depth_img)
{
    uint i = get_global_id(0);

    //borders
    int row = i / p->num_cols;
    int col = i % p->num_cols;

    if(row <  p->border || row >= p->num_rows - p->border || col < p->border || col >= p->num_cols - p->border)
    {
        pos_gen[i].z = 0.f;
        return;
    }

    // sample depth texture
    int img_w = get_image_width(the_img);
    int img_h = get_image_height(the_img);

    int2 array_pos = {img_w * (col / (float)(p->num_cols)),
                      img_h * (row / (float)(p->num_rows))};
    array_pos.y = img_h - array_pos.y - 1;
    array_pos.x = p->mirror ? (img_w - array_pos.x - 1) : array_pos.x;

    // either a depth image (depth in mm, 16bit)
    // or 8bit color
    float4 sample = read_imagef(the_img, array_pos);
    float ratio = 0.f;
    float outval;// = pos_gen[i].z;

    if(is_depth_img)
    {
        // depth value in meters
        float depth = sample.x * 65535.f / 1000.f;

        float depth_min = p->depth_min <= p->depth_max ? p->depth_min : p->depth_max;
        float depth_max = p->depth_min <= p->depth_max ? p->depth_max : p->depth_min;
        if(depth == 0.f){ depth = depth_max; }
        depth = clamp(depth, p->depth_min, p->depth_max);

        // if(p->depth_min <= p->depth_max){ depth = depth == 0.f ? p->depth_max : clamp(depth, p->depth_min, p->depth_max); }
        // else{ depth = depth == 0.f ? p->depth_min : clamp(depth, p->depth_max, p->depth_min); }
        outval = map_value(depth, p->depth_min, p->depth_max, p->z_min, p->z_max);
    }
    // color image input
    // this is optional and depth will always have priority
    else
    {
        // convert to grayscale and map to elevation
        float val = map_value(gray(sample).x, 0.f, 1.f, p->z_min, p->z_max);

        // if(p->z_max >= p->z_min){ outval = max(val, outval); }
        // else{ outval = min(val, outval); }
        outval = val;
    }
    pos_gen[i].z = outval;
}

__kernel void update_mesh(__global float4* pos,
                          __global float4* color,
                          __global float* point_sizes,
                          __global float4* pos_gen,
                          float dt,
                          __constant param_t *params)
{
    //get our index in the array
    unsigned int i = get_global_id(0);

    float4 p = pos[i];

    float4 target_pos = pos_gen[i];

    // interpolate toward target position
    p = p.z > target_pos.z ? mix(target_pos, p, params->smooth_fall) : mix(target_pos, p, params->smooth_rise);

    // update array with newly computed values
    pos[i] = p;

    float min_val = min(params->z_min, params->z_max);
    float max_val = max(params->z_min, params->z_max);
    float jet_val = (p.z - min_val) / (max_val - min_val);
    color[i] = jet(jet_val);
}
