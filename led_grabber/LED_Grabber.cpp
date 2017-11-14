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
    m_impl->m_connection && m_impl->m_connection->is_open();
}

bool LED_Grabber::grab_from_texture(const gl::Texture &the_tex)
{
    if(is_initialized())
    {
        return true;
    }
    return false;
}

}// namespace
