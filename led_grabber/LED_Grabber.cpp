//
//  LED_Grabber.cpp
//  led_grabber
//
//  Created by Fabian on 14.11.17.
//

#include "LED_Grabber.hpp"

#define DEVICE_ID "LEDS"

namespace kinski
{

struct LED_GrabberImpl
{
    ConnectionPtr m_connection;
    std::string m_device_name;
};

LED_Grabber::LED_Grabber():
m_impl(std::make_unique<LED_GrabberImpl>())
{
    
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
        uint32_t cols[num_segs] =
        {
            static_cast<uint32_t>((0 << 24) | (0 << 16) | (255 << 8) | 0),
            static_cast<uint32_t>((0 << 24) | (0 << 16) | (0 << 8) | 255),
            static_cast<uint32_t>((0 << 24) | (255 << 16) | (0 << 8) | 0),
        };
        
        std::vector<uint32_t> pixel_values(num_segs * num_leds_per_seg, 0);
        
        for(uint32_t i = 0; i < pixel_values.size(); ++i)
        {
            pixel_values[i] = cols[i / num_leds_per_seg];
        }
        m_impl->m_connection->write("DATA:" +
                                    to_string(pixel_values.size() * sizeof(pixel_values[0])) + "\n");
        m_impl->m_connection->write(pixel_values);
        return true;
    }
    return false;
}

}// namespace
