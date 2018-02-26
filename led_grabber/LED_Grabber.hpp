//
//  LED_Grabber.hpp
//  led_grabber
//
//  Created by Fabian on 14.11.17.
//

#pragma once

#include "core/Connection.hpp"
#include "gl/Texture.hpp"
#include "gl/Warp.hpp"

namespace kinski
{

DEFINE_CLASS_PTR(LED_Grabber)
    
class LED_Grabber
{
public:
    static const std::string id();
    
    static LED_GrabberPtr create(ConnectionPtr the_con = ConnectionPtr());
    
    virtual ~LED_Grabber();
    
    bool connect(ConnectionPtr the_device);

    const std::set<ConnectionPtr>& connections() const;
    
    bool is_initialized() const;
    
    void show_segment(size_t the_segment) const;
    
    //!
    bool grab_from_image_calib(const ImagePtr &the_image);
    
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
    
    //!
    gl::ivec2 unit_resolution() const;
    
    //!
    void set_unit_resolution(uint32_t the_width, uint32_t the_height);
    
    //!
    void send_data(const std::vector<uint8_t> &the_data) const;
    
    //!
    void send_data(const uint8_t *the_data, size_t the_num_bytes) const;
    
    std::vector<gl::vec2> run_calibration(int the_cam_index = 0,
                                          int the_thresh = 245,
                                          const gl::Color the_calib_color = gl::COLOR_WHITE);
    
    const std::vector<gl::vec2>& calibration_points() const;
    
    void set_calibration_points(const std::vector<gl::vec2> &the_points);
    
    void set_warp(const gl::Warp &the_warp);
    
private:
    
    LED_Grabber();
    std::unique_ptr<struct LED_GrabberImpl> m_impl;
};

}// namespace
