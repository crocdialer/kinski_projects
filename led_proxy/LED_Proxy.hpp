// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  LED_Proxy.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "crocore/Timer.hpp"
#include "crocore/Serial.hpp"
#include "crocore/networking.hpp"
#include "app/ViewerApp.hpp"

namespace kinski
{
    class LED_Proxy : public ViewerApp
    {
    public:
        LED_Proxy(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const crocore::PropertyConstPtr &theProperty) override;
        
    private:
        enum FontEnum{FONT_SMALL = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        
        std::set<crocore::ConnectionPtr> m_devices, m_clients;

        crocore::net::tcp_server m_tcp_server;
        crocore::Timer m_udp_broadcast_timer, m_device_scan_timer;
        
        gl::ivec2 m_unit_resolution{58, 14};
        std::vector<uint8_t> m_buffer;
        
        size_t m_bytes_to_write = 0, m_bytes_written = 0;
        void search_devices();
        void new_connection_cb(crocore::net::tcp_connection_ptr the_con);
        void tcp_data_cb(crocore::net::tcp_connection_ptr, const std::vector<uint8_t>&);
        
        void send_data(const uint8_t *the_data, size_t the_num_bytes) const;
        void set_segments(const std::vector<int> &the_segments) const;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::LED_Proxy>(argc, argv);
    LOG_INFO << "local ip: " << crocore::net::local_ip();
    return theApp->run();
}
