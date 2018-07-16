typedef struct
{
    int num_cols, num_rows, mirror, border;
    float depth_min, depth_max, input_depth, input_color;
    float smooth_fall, smooth_rise;
    float z_min, z_max;
    float4 color_min, color_max;
}param_t;

// debug coloring
inline float4 jet(float val)
{
    return (float4)(min(4.0f * val - 1.5f, -4.0f * val + 4.5f),
                    min(4.0f * val - 0.5f, -4.0f * val + 3.5f),
                    min(4.0f * val + 0.5f, -4.0f * val + 2.5f),
                    1.0f);
}

float4 hot_iron(float value)
{
    float4 color8  = (float4)( 255.0 / 255.0, 255.0 / 255.0, 204.0 / 255.0, 1.0 );
    float4 color7  = (float4)( 255.0 / 255.0, 237.0 / 255.0, 160.0 / 255.0, 1.0 );
    float4 color6  = (float4)( 254.0 / 255.0, 217.0 / 255.0, 118.0 / 255.0, 1.0 );
    float4 color5  = (float4)( 254.0 / 255.0, 178.0 / 255.0,  76.0 / 255.0, 1.0 );
    float4 color4  = (float4)( 253.0 / 255.0, 141.0 / 255.0,  60.0 / 255.0, 1.0 );
    float4 color3  = (float4)( 252.0 / 255.0,  78.0 / 255.0,  42.0 / 255.0, 1.0 );
    float4 color2  = (float4)( 227.0 / 255.0,  26.0 / 255.0,  28.0 / 255.0, 1.0 );
    float4 color1  = (float4)( 189.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    float4 color0  = (float4)( 128.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );

    float colorValue = value * 8.0f;
    int sel = (int)( floor( colorValue ) );

    if(sel >= 8){ return color0; }
    else if(sel < 0){ return color0; }
    else
    {
        colorValue -= (float)(sel);
        if(sel < 1){ return ( color1 * colorValue + color0 * ( 1.0f - colorValue ) ); }
        else if(sel < 2){ return ( color2 * colorValue + color1 * ( 1.0f - colorValue ) ); }
        else if(sel < 3){ return ( color3 * colorValue + color2 * ( 1.0f - colorValue ) ); }
        else if(sel < 4){ return ( color4 * colorValue + color3 * ( 1.0f - colorValue ) ); }
        else if(sel < 5){ return ( color5 * colorValue + color4 * ( 1.0f - colorValue ) ); }
        else if(sel < 6){ return ( color6 * colorValue + color5 * ( 1.0f - colorValue ) ); }
        else if(sel < 7){ return ( color7 * colorValue + color6 * ( 1.0f - colorValue ) ); }
        else if(sel < 8){ return ( color8 * colorValue + color7 * ( 1.0f - colorValue ) ); }
        else{ return color0; }
    }
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
    float outval = 0.f;

    if(is_depth_img)
    {
        // depth value in meters
        float depth = sample.x * 65535.f / 1000.f;

        float depth_min = min(p->depth_min, p->depth_max);
        float depth_max = max(p->depth_min, p->depth_max);
        if(depth == 0.f){ depth = depth_max; }
        depth = clamp(depth, p->depth_min, p->depth_max);
        outval = p->input_depth * map_value(depth, depth_min, depth_max, p->z_min, p->z_max);
    }
    // color image input
    // this is optional and depth will always have priority
    else
    {
        outval = pos_gen[i].z;

        // convert to grayscale and map to elevation
        float gray_val = gray(sample).x;
        gray_val = p->depth_min <= p->depth_max ? 1.f - gray_val : gray_val;
        float val = p->input_color * map_value(gray_val, 0.f, 1.f, p->z_min, p->z_max);

        if(p->z_min <= p->z_max){ outval -= val; }
        else{ outval += val; }
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

    float mix_val = (p.z - params->z_min) / (params->z_max - params->z_min);
    if(mix_val < 0.f){ mix_val += 1.f; }

    color[i] = mix(params->color_min, params->color_max, mix_val);
    // color[i] = jet(mix_val);
    // color[i] = hot_iron(mix_val);
}
