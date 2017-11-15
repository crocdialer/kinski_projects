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
    ImagePtr m_buffer_img;
    
    std::array<uint8_t, 256> m_gamma_r, m_gamma_g, m_gamma_b, m_gamma_w;
};

LED_Grabber::LED_Grabber():
m_impl(std::make_unique<LED_GrabberImpl>())
{
    float brighntess = 0.4f;
    m_impl->m_gamma_r = create_gamma_lut(brighntess, 3.8f, 220);
    m_impl->m_gamma_g = create_gamma_lut(brighntess, 2.2f);
    m_impl->m_gamma_b = create_gamma_lut(brighntess, 2.8f);
    m_impl->m_gamma_w = create_gamma_lut(brighntess, 2.8f);
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
        constexpr size_t num_segs = 3, num_leds_per_seg = 58;
        
        // WBRG byte-order in debug SK6812-RGBW strip
        constexpr uint8_t offset_r = 1, offset_g = 3, offset_b = 2, offset_w = 0;
        
        auto resized_img = the_image->resize(num_leds_per_seg, num_segs, 4);
        
        if(!resized_img->data){ return false; }
        
        uint8_t *ptr = resized_img->data;
        uint8_t *end_ptr = resized_img->data + resized_img->num_bytes();
        
        // swizzle components
        for(; ptr < end_ptr; ptr += 4)
        {
            uint32_t c = *(reinterpret_cast<uint32_t*>(ptr));
            
            ptr[offset_w] = 0;
            ptr[offset_b] = m_impl->m_gamma_b[(uint8_t)((reinterpret_cast<uint8_t*>(&c))[0])];
            ptr[offset_r] = m_impl->m_gamma_r[(uint8_t)((reinterpret_cast<uint8_t*>(&c))[2])];
            ptr[offset_g] = m_impl->m_gamma_g[(uint8_t)((reinterpret_cast<uint8_t*>(&c))[1])];
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

}// namespace
