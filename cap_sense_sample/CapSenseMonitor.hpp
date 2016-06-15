//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//
#pragma once

#include "app/ViewerApp.hpp"
#include "core/Timer.hpp"

// modules
#include "curl/Downloader.h"
#include "sensors/CapacitiveSensor.hpp"

namespace kinski
{
    class CapSenseMonitor : public ViewerApp
    {
    private:
        
        enum State{STATE_IDLE = 0, STATE_IMAGE = 1};
        enum FontEnum{FONT_SMALL = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        
        UART_Ptr m_uart;
        
        std::vector<CapacitiveSensor> m_sensors;
        bool m_needs_sensor_reset = true;
        
        //! used for http-requests
        net::Downloader m_downloader;
        
        //! timer for periodic udp broadcasts
        Timer m_broadcast_timer;
        
        gl::Scene m_gui_scene;
        
        Property_<string>::Ptr
        m_device_prefix = Property_<string>::create("device prefix", "tty.usb"),
        m_img_url = Property_<string>::create("image url");
        
        Property_<uint32_t>::Ptr
        m_broadcast_udp_port = Property_<uint32_t>::create("broadcast udp port", 4444),
        m_broadcast_frequency = Property_<uint32_t>::create("broadcast frequency (Hz)", 60),
        m_cap_sense_thresh_touch = Property_<uint32_t>::create("capsense: touch threshold", 12),
        m_cap_sense_thresh_release = Property_<uint32_t>::create("capsense: release threshold", 6),
        m_cap_sense_charge_current = Property_<uint32_t>::create("capsense: charge current", 32);
        
        Property_<float>::Ptr
        m_cap_sense_proxi_multiplier = RangedProperty<float>::create("capsense: proximity multiplier",
                                                                     1.f , 0.f, 100.f);
        
        void send_udp_broadcast();
        void reset_sensors();
        
    public:
        
        CapSenseMonitor(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
        
        void sensor_touch(int the_sensor_index, int the_pad_index);
        void sensor_release(int the_sensor_index, int the_pad_index);
    };
}// namespace kinski