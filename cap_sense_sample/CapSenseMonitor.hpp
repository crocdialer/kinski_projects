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
#include "http/http.h"
#include "sensors/sensors.h"
#include "bluetooth/Bluetooth_UART.hpp"

namespace kinski
{
    class CapSenseMonitor : public ViewerApp
    {
    private:

        enum State{STATE_IDLE = 0, STATE_IMAGE = 1};
        enum FontEnum{FONT_SMALL = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        
        std::vector<CapacitiveSensorPtr> m_sensors;
        
        bluetooth::Bluetooth_UARTPtr m_bluetooth = bluetooth::Bluetooth_UART::create();
        
        //! used for http-requests
        net::http::Client m_http{background_queue().io_service()};

        //! timer for periodic udp broadcasts
        Timer m_broadcast_timer, m_scan_for_device_timer;

        gl::ScenePtr m_gui_scene = gl::Scene::create();

        Property_<string>::Ptr
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

        void connect_sensor(UARTPtr);
        void reset_sensors();
        void send_udp_broadcast();

    public:

        CapSenseMonitor(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void key_press(const KeyEvent &e) override;
        void key_release(const KeyEvent &e) override;
        void mouse_press(const MouseEvent &e) override;
        void mouse_release(const MouseEvent &e) override;
        void mouse_move(const MouseEvent &e) override;
        void mouse_drag(const MouseEvent &e) override;
        void mouse_wheel(const MouseEvent &e) override;
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const Property::ConstPtr &theProperty) override;

        void sensor_touch(int the_sensor_index, int the_pad_index);
        void sensor_release(int the_sensor_index, int the_pad_index);
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::CapSenseMonitor>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
