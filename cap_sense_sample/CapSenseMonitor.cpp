//
//  CapSenseMonitor.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "CapSenseMonitor.hpp"
#include "core/Serial.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    const double g_scan_for_device_interval = 5.0;
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::setup()
{
    ViewerApp::setup();
    fonts()[FONT_MEDIUM].load(fonts()[FONT_SMALL].path(), 34);
    fonts()[FONT_LARGE].load(fonts()[FONT_SMALL].path(), 54);
    register_property(m_broadcast_udp_port);
    register_property(m_broadcast_frequency);
    register_property(m_cap_sense_thresh_touch);
    register_property(m_cap_sense_thresh_release);
    register_property(m_cap_sense_charge_current);
    register_property(m_cap_sense_proxi_multiplier);
    register_property(m_img_url);
    observe_properties();

    add_tweakbar_for_component(shared_from_this());
    remote_control().set_components({shared_from_this()});

    m_broadcast_timer = Timer(main_queue().io_service(),
                              std::bind(&CapSenseMonitor::send_udp_broadcast, this));
    m_broadcast_timer.set_periodic(true);

    m_scan_for_device_timer = Timer(main_queue().io_service(), [this](){ reset_sensors(); });
    m_scan_for_device_timer.set_periodic();
    m_scan_for_device_timer.expires_from_now(g_scan_for_device_interval);

    reset_sensors();
    load_settings();
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::draw()
{
    if(textures()[0]){ gl::draw_texture(textures()[0], gl::window_dimension()); }

    const int offset_x = 40;
    const gl::vec2 sz = gl::vec2(60, 35);

    gl::draw_text_2D(name(), fonts()[FONT_LARGE], gl::COLOR_WHITE, gl::vec2(25, 20));

    gl::vec2 offset(offset_x, 90), step = sz * 1.2f;

    for(auto &s : m_sensors)
    {
        for(int i = 0; i < 13; i++)
        {
            auto color = s->is_touched(i) ? gl::COLOR_ORANGE : gl::COLOR_GRAY;

            float proxi_val = clamp(*m_cap_sense_proxi_multiplier * s->proximity_values()[12 - i] / 100.f,
                                    0.f, 1.f);

            vec2 pos = offset - sz / 2.f,
            sz_frac = s->is_touched(i) ? vec2(sz) : sz * vec2(1.f, proxi_val);


            gl::draw_quad(gl::COLOR_ORANGE, sz, pos, false);
            gl::draw_quad(color, sz_frac, pos + vec2(0.f, sz.y - sz_frac.y));

            if(i == 12)
            {
                gl::draw_text_2D(to_string(int(std::round(proxi_val * 100.f))) + "%", fonts()[FONT_MEDIUM],
                                 gl::COLOR_WHITE, offset - sz / 3.f);
            }
            offset.x += step.x;
        }
        offset.y += step.y;
        offset.x = offset_x;
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);

    switch(e.getCode())
    {
        case kinski::Key::_W:
        {
#if !defined(KINSKI_EGL)
            auto w = GLFW_Window::create(800, 500, "my super cool window", false, 0, windows().back()->handle());
            add_window(w);
            w->set_draw_function([this]()
            {
                gl::clear();
                gl::draw_texture(textures()[0], gl::window_dimension());
                gl::draw_circle(gl::window_dimension() / 2.f, 50.f, gl::COLOR_ORANGE, true);
            });
#endif
        }
            break;
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_img_url)
    {
        if(!m_img_url->value().empty())
        {
            m_http.async_get(*m_img_url, [this](const net::http::connection_info_t &c,
                                                      const net::http::response_t &response)
            {
                try
                {
                    auto img = create_image_from_data(response.data);

                    main_queue().submit([this, img]()
                    {
                        textures()[0] = gl::create_texture_from_image(img);
                    });
                }
                catch(std::exception &e){ LOG_WARNING << "could not decode: " << c.url; }
            });
        }
        else{ textures()[0].reset(); }
    }
    else if(theProperty == m_cap_sense_thresh_touch ||
            theProperty == m_cap_sense_thresh_release)
    {
        for(auto &s : m_sensors)
        {
            if(s->is_initialized())
            {
                s->set_thresholds(*m_cap_sense_thresh_touch, *m_cap_sense_thresh_release);
            }
        }
    }
    else if(theProperty == m_cap_sense_charge_current)
    {
        for(auto &s : m_sensors)
        {
            if(s->is_initialized())
            {
                s->set_charge_current(*m_cap_sense_charge_current);
            }
        }
    }
    else if(theProperty == m_broadcast_frequency)
    {
        if(*m_broadcast_frequency > 0.f)
        {
            m_broadcast_timer.expires_from_now(1.f / *m_broadcast_frequency);
        }
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::sensor_touch(int the_sensor_index, int the_pad_index)
{
    LOG_TRACE << "touched sensor " << the_sensor_index << ": " << the_pad_index;
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::sensor_release(int the_sensor_index, int the_pad_index)
{
    LOG_TRACE << "released sensor " << the_sensor_index << ": " << the_pad_index;
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::send_udp_broadcast()
{
    string str;
    int i = 0;

    for(auto &s : m_sensors)
    {
        float vals[3];
        for(int j = 0; j < 3; ++j)
        { vals[j] = clamp(s->proximity_values()[j] * *m_cap_sense_proxi_multiplier / 100.f, 0.f, 1.f); }

        str += to_string(i++) + ": " + to_string(s->touch_state()) + " " +
        to_string(vals[0], 3) + " " + to_string(vals[1], 3) + " " + to_string(vals[2], 3) + "\n";
    }
    if(!str.empty())
    {
//        LOG_TRACE_2 << "udp-broadcast (" << *m_broadcast_udp_port << "): " << str.substr(0, str.size() - 1);
        net::async_send_udp_broadcast(background_queue().io_service(), str,
                                      *m_broadcast_udp_port);
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::connect_sensor(ConnectionPtr the_device)
{
    auto cs = CapacitiveSensor::create();
    auto sensor_index = m_sensors.size();
    cs->set_thresholds(*m_cap_sense_thresh_touch, *m_cap_sense_thresh_release);
    cs->set_charge_current(*m_cap_sense_charge_current);
    cs->set_touch_callback(std::bind(&CapSenseMonitor::sensor_touch, this,
                                    sensor_index, std::placeholders::_1));
    cs->set_release_callback(std::bind(&CapSenseMonitor::sensor_release, this,
                                      sensor_index, std::placeholders::_1));

    if(cs->connect(the_device))
    {
        LOG_DEBUG << "connected sensor: " << the_device->description();

        the_device->set_disconnect_cb([this](ConnectionPtr the_device)
        {
            m_scan_for_device_timer.expires_from_now(g_scan_for_device_interval);
        });
        m_sensors.push_back(cs);
        m_scan_for_device_timer.cancel();
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::reset_sensors()
{
    m_sensors.clear();
    
    auto query_cb = [this](const std::string &the_id, ConnectionPtr the_device)
    {
        LOG_TRACE_1 << "discovered device: <" << the_id << "> (" << the_device->description() << ")";
        
        if(the_id == CapacitiveSensor::id())
        {
            main_queue().submit([this, the_device]{ connect_sensor(the_device); });
        }
    };
    
    // network-sensors are supposed to broadcast their ID on port 55555
    m_udp_server.start_listen(55555);
    m_udp_server.set_receive_function([this, query_cb](const std::vector<uint8_t>& the_data,
                                                       const std::string& the_ip,
                                                       uint16_t the_port)
    {
        std::string device_str(the_data.begin(), the_data.end());
        LOG_TRACE << the_ip << ": " << device_str;
        
        if(device_str == CapacitiveSensor::id())
        {
            // check for doubles
            for(auto &s : m_sensors)
            {
                auto tcp_con = std::dynamic_pointer_cast<net::tcp_connection>(s->device_connection());
                if(tcp_con && tcp_con->remote_ip() == the_ip){ return; }
            }
            
            m_udp_server.stop_listen();
            
            // create tcp-connection
            auto con = net::tcp_connection::create(background_queue().io_service(), the_ip, 33333);
            
            con->set_timeout(3.0);
            con->set_connect_cb([query_cb, device_str](ConnectionPtr the_con)
            {
                // this is a workaround for the Arduino/Adafruit Ethernet2 library.
                // it won't establish a connection without seeing at least one byte on the wire
                the_con->write("\n");
                query_cb(device_str, the_con);
            });
        }
    });
    sensors::scan_for_serials(background_queue().io_service(), query_cb);
}
