//
//  LED_Grabber.cpp
//  led_grabber
//
//  Created by Fabian on 14.11.17.
//

#include <array>
#include "LED_Grabber.hpp"

#define DEVICE_ID "LEDS"

namespace kinski
{
    
std::array<uint8_t, 256> create_gamma_lut(float the_brighntess, float the_gamma,
                                          uint8_t the_max_out = 255)
{
    std::array<uint8_t, 256> ret;
    
    for(int i = 0; i < 256; ++i)
    {
        ret[i] = roundf(the_max_out * the_brighntess * pow((float)i / 255, the_gamma));
    }
    return ret;
}
    
struct LED_GrabberImpl
{
    ConnectionPtr m_connection;
    std::string m_device_name;
    
    gl::vec4 m_brightness;
    gl::ivec2 m_resolution;
    ImagePtr m_buffer_img;
    bool m_dirty_lut = true;
    
    std::array<uint8_t, 256> m_gamma_r, m_gamma_g, m_gamma_b, m_gamma_w;
    
    void create_lut()
    {
        m_gamma_r = create_gamma_lut(m_brightness.r, 3.2f);
        m_gamma_g = create_gamma_lut(m_brightness.g, 2.2f);
        m_gamma_b = create_gamma_lut(m_brightness.b, 2.8f);
        m_gamma_w = create_gamma_lut(m_brightness.w, 3.2f);
        m_dirty_lut = false;
    }
};

LED_Grabber::LED_Grabber():
m_impl(std::make_unique<LED_GrabberImpl>())
{
    set_resolution(58, 6);
    set_brightness(gl::vec4(0.4f, 0.4f, 0.4f, 0.2f));
}
    
LED_Grabber::~LED_Grabber()
{
    
}
    
LED_GrabberPtr LED_Grabber::create(ConnectionPtr the_device)
{
    LED_GrabberPtr ret(new LED_Grabber());
    if(the_device){ ret->connect(the_device); }
    return ret;
}
    
const std::string LED_Grabber::id()
{
    return DEVICE_ID;
}
    
bool LED_Grabber::connect(ConnectionPtr the_device)
{
    if(the_device /*&& the_uart_device->is_open()*/)
    {
        the_device->drain();
        m_impl->m_connection = the_device;
        return true;
    }
    return false;
}
    
ConnectionPtr LED_Grabber::device_connection() const
{
    return m_impl->m_connection;
}
    
bool LED_Grabber::is_initialized() const
{
    return m_impl->m_connection && m_impl->m_connection->is_open();
}

bool LED_Grabber::grab_from_image(const ImagePtr &the_image)
{
    if(is_initialized())
    {
        size_t num_segs = m_impl->m_resolution.y, num_leds_per_seg = m_impl->m_resolution.x;
        
        // WBRG byte-order in debug SK6812-RGBW strip
        constexpr uint8_t offset_r = 1, offset_g = 3, offset_b = 2, offset_w = 0;
        
        auto resized_img = the_image->resize(num_leds_per_seg, num_segs, 4);
        
        if(!resized_img->data){ return false; }
        
        uint8_t *ptr = resized_img->data;
        uint8_t *end_ptr = resized_img->data + resized_img->num_bytes();
        
        // create/update lookup tables, if necessary
        if(m_impl->m_dirty_lut){ m_impl->create_lut(); }
        
        // swizzle components, apply brightness and gamma
        for(; ptr < end_ptr; ptr += 4)
        {
            uint32_t c = *(reinterpret_cast<uint32_t*>(ptr));
            uint8_t *ch = reinterpret_cast<uint8_t*>(&c);
            
            ptr[offset_b] = m_impl->m_gamma_b[ch[0]];
            ptr[offset_g] = m_impl->m_gamma_g[ch[1]];
            ptr[offset_r] = m_impl->m_gamma_r[ch[2]];
            
            // Y′ = 0.299 R′ + 0.587 G′ + 0.114 B′
            ptr[offset_w] = m_impl->m_gamma_w[(uint8_t)(ch[2] * 0.299f + ch[1] * 0.587f + ch[0] * 0.114f)];
        }
        
        // reverse every second line (using a zigzag wiring-layout)
        for(int i = 0; i < resized_img->height; ++i)
        {
            if(i % 2)
            {
                uint32_t* line_ptr = ((uint32_t*)resized_img->data) + i * resized_img->width;
                std::reverse(line_ptr, line_ptr + resized_img->width);
            }
        }
        
        m_impl->m_connection->write("DATA:" +
                                    to_string(resized_img->num_bytes()) + "\n");
        m_impl->m_connection->write_bytes(resized_img->data, resized_img->num_bytes());
        m_impl->m_buffer_img = resized_img;
        return true;
    }
    return false;
}
    
gl::Texture LED_Grabber::output_texture()
{
    auto ret = gl::create_texture_from_image(m_impl->m_buffer_img);
    ret.set_mag_filter(GL_NEAREST);
//    ret.set_swizzle()
    return ret;
}
    
gl::vec4 LED_Grabber::brightness() const
{
    return m_impl->m_brightness;
}

void LED_Grabber::set_brightness(const gl::vec4 &the_brightness)
{
    m_impl->m_brightness = glm::clamp(the_brightness, glm::vec4(0.f), glm::vec4(1.f));
    m_impl->m_dirty_lut = true;
}
    
void LED_Grabber::set_brightness(float the_brightness)
{
    set_brightness(gl::vec4(the_brightness));
}

gl::ivec2 LED_Grabber::resolution() const
{
    return m_impl->m_resolution;
}


void LED_Grabber::set_resolution(uint32_t the_width, uint32_t the_height)
{
    m_impl->m_resolution = gl::ivec2(the_width, the_height);
}
    
}// namespace
