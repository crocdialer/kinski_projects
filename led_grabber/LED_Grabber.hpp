//
//  LED_Grabber.hpp
//  led_grabber
//
//  Created by Fabian on 14.11.17.
//

#pragma once

#include "core/Connection.hpp"
#include "gl/Texture.hpp"

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
    
    gl::Texture output_texture();
    
    //!
    gl::vec4 brightness() const;
    
    //!
    void set_brightness(float the_brightness);
    
    //!
    void set_brightness(const gl::vec4 &the_brightness);
    
    //!
    gl::ivec2 resolution() const;
    
    //!
    void set_resolution(uint32_t the_width, uint32_t the_height);
    
private:
    
    LED_Grabber();
    std::unique_ptr<struct LED_GrabberImpl> m_impl;
};

}// namespace
