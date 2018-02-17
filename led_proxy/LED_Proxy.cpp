//
//  LED_Proxy.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "LED_Proxy.hpp"
#include "sensors/sensors.h"
#include <mutex>

using namespace kinski;

namespace
{
    std::string g_id = "LEDS";
    uint16_t g_udp_broadcast_port = 55555;
    float g_udp_broadcast_interval = 2.f;
    
    uint16_t g_tcp_listen_port = 44444;
    
    float g_device_scan_interval = 5.f;
    
    std::mutex g_device_mutex, g_client_mutex;
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::setup()
{
    ViewerApp::setup();
    fonts()[FONT_MEDIUM].load(fonts()[FONT_SMALL].path(), 34);
    fonts()[FONT_LARGE].load(fonts()[FONT_SMALL].path(), 54);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    load_settings();
    
    // periodic device scan
    m_device_scan_timer = Timer(background_queue().io_service(),
                                std::bind(&LED_Proxy::search_devices, this));
    m_device_scan_timer.set_periodic();
    m_device_scan_timer.expires_from_now(g_device_scan_interval);
    
    // initial scan
    search_devices();
    
    // discovery broadcast
    m_udp_broadcast_timer = Timer(background_queue().io_service(), [this]()
    {
        net::async_send_udp_broadcast(background_queue().io_service(), g_id, g_udp_broadcast_port);
    });
    m_udp_broadcast_timer.set_periodic();
    m_udp_broadcast_timer.expires_from_now(g_udp_broadcast_interval);
    
    // tcp server setup
    m_tcp_server = net::tcp_server(background_queue().io_service(),
                                   std::bind(&LED_Proxy::new_connection_cb, this,
                                             std::placeholders::_1));
    m_tcp_server.start_listen(g_tcp_listen_port);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::draw()
{
    gl::clear();
    gl::draw_text_2D(name(), fonts()[FONT_LARGE], gl::COLOR_WHITE, gl::vec2(25, 20));
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    if(the_touches.size() == 1){ set_display_tweakbar(!display_tweakbar()); }
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void LED_Proxy::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void LED_Proxy::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::new_connection_cb(net::tcp_connection_ptr the_con)
{
    LOG_DEBUG << "client connected: " << the_con->description();
    
    the_con->set_tcp_receive_cb(std::bind(&LED_Proxy::tcp_data_cb, this,
                                std::placeholders::_1, std::placeholders::_2));
    the_con->set_disconnect_cb([this](ConnectionPtr client)
    {
        LOG_DEBUG << "client disconnected: " << client->description();
        std::lock_guard<std::mutex> lock(g_client_mutex);
        m_devices.erase(client);
    });
    
    std::lock_guard<std::mutex> lock(g_client_mutex);
    m_devices.insert(the_con);
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::tcp_data_cb(net::tcp_connection_ptr the_con, const std::vector<uint8_t> &the_data)
{
    if(!m_bytes_to_write)
    {
        std::string str(the_data.begin(), the_data.end());
//        LOG_DEBUG << str;

        auto lines = split(str, '\n');

        for(auto &l : lines)
        {
            auto tokens = split(l, ':');

            if(tokens.size() == 2)
            {
                LOG_TRACE_2 << "cmd: " << tokens[0] << " -> " << tokens[1];
                if(tokens[0] == "DATA"){ m_bytes_to_write = string_to<size_t>(tokens[1]); }
                else if(tokens[0] == "ID"){ the_con->write(g_id + "\n"); }
            }
        }
    }
    else
    {
        LOG_TRACE_2 << "datablob: " << the_data.size() / 1024 << " kB";
        
        m_bytes_to_write -= std::min(m_bytes_to_write, the_data.size());
        m_buffer.insert(m_buffer.end(), the_data.begin(), the_data.end());
        
        if(!m_bytes_to_write)
        {
            send_data(m_buffer.data(), m_buffer.size());
            m_buffer.clear();
        }
    }
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::send_data(const uint8_t *the_data, size_t the_num_bytes) const
{
    size_t max_bytes = 4 * m_unit_resolution.x * m_unit_resolution.y;
    size_t total_bytes_written = 0;
    
    std::list<ConnectionPtr> cons;
    {
        std::unique_lock<std::mutex> lock(g_device_mutex);
        cons.insert(cons.end(), m_devices.begin(), m_devices.end());
        cons.sort([](const ConnectionPtr &lhs, const ConnectionPtr &rhs)
        {
            return lhs->description() < rhs->description();
        });
    }
    
    for(auto &c : cons)
    {
        size_t bytes_to_write = std::min(max_bytes, the_num_bytes);
        if(!bytes_to_write){ return; }
        the_num_bytes -= bytes_to_write;
        
        c->write("DATA:" + to_string(bytes_to_write) + "\n");
        
        while(bytes_to_write)
        {
            size_t num_bytes_tranferred = c->write_bytes(the_data + total_bytes_written,
                                                         bytes_to_write);
            bytes_to_write -= num_bytes_tranferred;
            total_bytes_written += num_bytes_tranferred;
        }
    }
}

/////////////////////////////////////////////////////////////////

void LED_Proxy::search_devices()
{
    // connect serial
    auto query_cb = [this](const std::string &the_id, ConnectionPtr the_device)
    {
        if(the_id == g_id)
        {
            LOG_DEBUG << "LED unit connected: " << the_device->description();
            
            the_device->set_disconnect_cb([this](ConnectionPtr the_device)
            {
                LOG_DEBUG << "LED unit disconnected: " << the_device->description();
                std::lock_guard<std::mutex> lock(g_device_mutex);
                m_devices.erase(the_device);
            });
            
            std::lock_guard<std::mutex> lock(g_device_mutex);
            m_devices.insert(the_device);
        }
    };
    sensors::scan_for_serials(background_queue().io_service(), query_cb);
}
