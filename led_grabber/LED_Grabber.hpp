//
//  LED_Grabber.hpp
//  led_grabber
//
//  Created by Fabian on 14.11.17.
//

#pragma once

#include "core/Connection.hpp"
#include "core/Image.hpp"

namespace kinski
{

DEFINE_CLASS_PTR(LED_Grabber)
    
class LED_Grabber
{
public:
    static const std::string id();
    
    static LED_GrabberPtr create(ConnectionPtr the_uart_device = ConnectionPtr());
    
    virtual ~LED_Grabber();
    
    bool connect(ConnectionPtr the_device);
    
    ConnectionPtr device_connection() const;
    
    bool is_initialized() const;
    
    bool grab_from_image(const ImagePtr &the_image);
    
private:
    
    LED_Grabber();
    std::unique_ptr<struct LED_GrabberImpl> m_impl;
};

}// namespace
